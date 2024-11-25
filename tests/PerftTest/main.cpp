#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "Moves.h"
#include "MagicBitboards.h"
#include "Zobrist.h"
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <tuple>

int main() {
	int start_depth = 1;
	int max_depth = 6;
	bool test_passed = true;
	std::chrono::milliseconds total_duration(0);
	unsigned long long total_positions = 0;

	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) return -1;

	// Perft test suit positions from http://www.rocechess.ch/perft.html with a few added positions
	std::ifstream in_file;

	// Get file's directory
	std::string file_path = __FILE__;
	std::string dir_path = file_path.substr(0, file_path.rfind("\\"));

	in_file.open(dir_path + "\\perftsuite.epd");
	if (!in_file.is_open()) return -2;

	std::string line;
	while (std::getline(in_file, line)) {
		std::stringstream perft_str(line);
		std::string seg;
		std::vector<std::string> perft;

		while (std::getline(perft_str, seg, ';')) {
			perft.push_back(seg);
		}
		std::cout << "Fen: " << perft[0] << "\n";

		auto [player, opponent, half_moves, full_moves] = FENToPosition(perft[0]);
		if (full_moves == -1) return -3;

		for (int depth = start_depth; depth <= max_depth; depth++) {
			if (depth + 1 > perft.size()) break;
			unsigned long long expected_num_nodes = std::stoull(perft[depth].substr(3));

			auto start = std::chrono::high_resolution_clock::now();
			unsigned long long num_nodes = Perft(depth, player, opponent);
			auto stop = std::chrono::high_resolution_clock::now();

			if (expected_num_nodes == num_nodes) {
				std::cout << "Depth " << depth << ": Passed. Positions searched: " << num_nodes << ". ";
			}
			else {
				std::cout << "Depth " << depth << ": Failed (expected: " << expected_num_nodes << ", result: " << num_nodes << "). ";
				test_passed = false;
			}

			auto duration = duration_cast<std::chrono::milliseconds>(stop - start);
			total_duration += duration;
			std::cout << "Time: " << duration.count() << " ms.\n";
			total_positions += num_nodes;
		}
	}

	std::cout << '\n' << (test_passed ? "Test Suite Passed." : "Test Suite Failed.") 
			  << " Total positions searched: " << total_positions 
			  << ". Total Time: " << total_duration.count() << " ms.\n";
}
