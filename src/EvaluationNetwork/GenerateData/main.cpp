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
#include "../Evaluate/EvaluateNNUE.h"
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

	std::string file_path_input = "F:\\ChessDataSet\\Raw data\\lichess_db_eval.csv";
	std::string file_path_output = "F:\\ChessDataSet\\lichess_db_all_data.bin";

	std::ifstream fin;
	std::ofstream fout;
	fin.open(file_path_input);
	if (write_to_file) fout.open(file_path_output, std::ios_base::binary);
	std::string line, fen, eval, best_move;

	std::getline(fin, line); // Ignore first line of csv
	double error = 0;
	double error_nnue = 0;
	unsigned long long n_positions = 0;

	while (std::getline(fin, line)) {
		std::stringstream s(line);

		std::getline(s, fen,  ',');
		std::getline(s, eval, ',');
		std::getline(s, best_move);

		std::array<uint32_t, 30> locations_1s_sm = {};
		std::array<uint32_t, 30> locations_1s_snm = {};
		Position position = FENToPosition(fen, calculate_loss, calculate_loss);

		if (std::popcount(position.player.bitboards.all_pieces) > 32) continue;

		// Do not store positions if their best move is a capture, since we are only interested in evaluating quiet positions
		location final_square = notationSquareToLocation(best_move.substr(2, 2));
		if ((1LL << final_square) & position.opponent.bitboards.friendly_pieces) 
			continue;

		// Make evaluation relative to side to move
		int32_t evaluation = std::stoi(eval);
		if (abs(evaluation) > Max_Eval) evaluation = Max_Eval * ((evaluation < 0) ? -1 : 1);
		if (!position.player.is_white) evaluation *= -1;

		Player* white = (position.player.is_white) ? &position.player : &position.opponent;
		Player* black = (!position.player.is_white) ? &position.player : &position.opponent;

		// Find all squares with pieces on the board
		std::vector<NNUEIndex> indexes = getIndexesNNUE(position.player, position.opponent);

		int i = 0;
		for (auto& [loc_wk, loc_bk] : indexes) {
			locations_1s_sm[i]  = position.player.is_white ? loc_wk : loc_bk;
			locations_1s_snm[i] = position.player.is_white ? loc_bk : loc_wk;
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
			// Compute loss normal evaluation function
			int ev = Evaluate(position.player, position.opponent, std::popcount(position.player.bitboards.all_pieces));
			if (abs(ev) > Max_Eval) ev = Max_Eval * ((ev < 0) ? -1 : 1);
			error += loss(ev, evaluation);

			// Compute loss nnue
			nnue.setPosition(position.player, position.opponent);
			ev = nnue.evaluate();
			if (abs(ev) > Max_Eval) ev = Max_Eval * ((ev < 0) ? -1 : 1);
			error_nnue += loss(ev, evaluation);

			n_positions++;
		}
	}

	if (calculate_loss) {
		std::cout << "Error evaluation function: " << (error / n_positions) << '\n'
				  << "Error nnue evaluation:     " << (error_nnue / n_positions) << '\n';
	}

	std::cout << "Number of quiet positions: " << n_positions << '\n';

	fin.close();
	fout.close();

	return 0;
}

inline static double sigmoid(double x) {
	return (1.0 / (1.0 + exp(-x)));
}

inline double loss(int y_pred, int y_true) {
	constexpr double max_loss = 100;
	constexpr double scaling = 1.0/410;

	double wdl_eval_true = sigmoid(y_true * scaling);
	double wdl_eval_pred = sigmoid(y_pred * scaling);
	double loss_eval = pow((wdl_eval_pred - wdl_eval_true), 2);

	return loss_eval * max_loss;
}
