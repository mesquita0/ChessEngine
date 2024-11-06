#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include "../Evaluate/InputNNUE.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "Evaluate.h"

/*
	Neural network has 64*64*10 inputs for each side, with a total of
	81,920 binary inputs, but only 60 of them will be 1 (at most).
	Therefore store only the locations of the 1's.
*/ 

inline double loss(int y_pred, int y_true);

constexpr bool calculate_loss = true;
constexpr bool write_to_file = false;
constexpr int Max_Eval = 1000;

// Take csv file (FEN, eval) and convert to locations inputs 1.
int main() {

	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) {
		std::cout << "Couldn't load Magic Bitboards file.";
		return 9;
	}

	std::string file_path_input = "F:\\ChessDataSet\\Raw data\\lichess_db_Fen_Eval.csv";
	std::string file_path_output = "F:\\ChessDataSet\\lichess_db_all_data.bin";

	std::ifstream fin;
	std::ofstream fout;
	fin.open(file_path_input);
	if (write_to_file) fout.open(file_path_output, std::ios_base::binary);
	std::string line, fen, eval;

	std::getline(fin, line); // Ignore first line of csv
	double error = 0;
	unsigned long long n_positions = 0;

	while (std::getline(fin, line)) {
		std::stringstream s(line);

		std::getline(s, fen, ',');
		std::getline(s, eval);
		
		std::array<uint32_t, 30> locations_1s_sm = {};
		std::array<uint32_t, 30> locations_1s_snm = {};
		Position position = FENToPosition(fen, calculate_loss, calculate_loss);
		uint64_t all_pieces = position.player.bitboards.all_pieces;

		if (std::popcount(all_pieces) > 32) continue;

		// Make evaluation relative to side to move
		int32_t evaluation = std::stoi(eval);
		if (abs(evaluation) > Max_Eval) evaluation = Max_Eval * ((evaluation < 0) ? -1 : 1);
		if (!position.player.is_white) evaluation *= -1;

		Player* white = (position.player.is_white) ? &position.player : &position.opponent;
		Player* black = (!position.player.is_white) ? &position.player : &position.opponent;

		// Find all squares with pieces on the board
		std::vector<NNUEIndex> indexes = getIndexesNNUE(position.player, position.opponent);

		int i = 0;
		for (auto& [loc_sm, loc_snm] : indexes) {
			locations_1s_sm[i] = loc_sm;
			locations_1s_snm[i] = loc_snm;
			i++;
		}

		// Sort arrays of locations in ascending order.
		std::sort(locations_1s_sm.begin(), locations_1s_sm.end());
		std::sort(locations_1s_snm.begin(), locations_1s_snm.end());

		if (write_to_file) {
			fout.write(reinterpret_cast<const char*>(&locations_1s_sm), sizeof locations_1s_sm);
			fout.write(reinterpret_cast<const char*>(&locations_1s_snm), sizeof locations_1s_snm);
			fout.write(reinterpret_cast<const char*>(&evaluation), sizeof evaluation);
		}
		if (calculate_loss) {
			int ev = Evaluate(position.player, position.opponent, std::popcount(position.player.bitboards.all_pieces));
			if (abs(ev) > Max_Eval) ev = Max_Eval * ((ev < 0) ? -1 : 1);
			error += loss(ev, evaluation);
			n_positions++;
		}
	}

	if (calculate_loss) {
		std::cout << (error / n_positions);
	}

	fin.close();
	fout.close();

	return 0;
}

inline static double sigmoid(double x) {
	return (1.0 / (1.0 + exp(-x)));
}

inline double loss(int y_pred, int y_true) {
	constexpr double max_loss = 100;
	constexpr double scalling = 1.0/120;

	double wdl_eval_true = sigmoid(y_true * scalling);
	double wdl_eval_pred = sigmoid(y_pred * scalling);
	double loss_eval = pow((wdl_eval_pred - wdl_eval_true), 2);

	return loss_eval * max_loss;
}
