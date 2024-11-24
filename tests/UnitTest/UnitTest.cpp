#include "CppUnitTest.h"
#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "Moves.h"
#include "MagicBitboards.h"
#include "Zobrist.h"
#include "EvaluationNetwork/Evaluate/LinearLayer.h"
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <vector>
#include <sstream>
#include <tuple>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(UnitTest)
	{
	public:
		
		TEST_METHOD(FenToMemoryTest) // Test conversion from FEN to position in memory
		{
			bool loaded = magic_bitboards.loadMagicBitboards();
			Assert::IsTrue(loaded);

			auto [pl, op, half_moves, full_moves] = FENToPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

			// 0 is white, 1 is black
			std::vector<std::vector<int>> pawns_positions = { { 48, 49, 50, 51, 52, 53, 54, 55},
															  { 8, 9, 10, 11, 12, 13, 14, 15} };
			std::vector<std::vector<int>> knights_positions = { { 57, 62  }, { 1, 6 } };
			std::vector<std::vector<int>> bishops_positions = { { 58, 61 }, { 2, 5 } };
			std::vector<std::vector<int>> rooks_positions = { {	56, 63 }, { 0, 7 } };
			std::vector<std::vector<int>> queens_positions = { { 59 }, { 3 } };

			// Check overlap between pieces of different colors
			Assert::IsTrue((pl.bitboards.friendly_pieces & op.bitboards.friendly_pieces) == 0);

			Player& white = pl.is_white ? pl : op;
			Player& black = pl.is_white ? op : pl;

			Player& player = black;
			for (int i = 0; i < 2; i++) {
				if (i == 1) player = white;

				// Check overlap between pieces of same color
				Assert::AreEqual(
					(player.bitboards.pawns & player.bitboards.knights & player.bitboards.bishops & player.bitboards.rooks & player.bitboards.queens), 0ull
				);

				std::vector<std::tuple<unsigned long long, std::vector<int>>> pieces_to_positions;
				pieces_to_positions.emplace_back(player.bitboards.pawns, pawns_positions[i]);
				pieces_to_positions.emplace_back(player.bitboards.knights, knights_positions[i]);
				pieces_to_positions.emplace_back(player.bitboards.bishops, bishops_positions[i]);
				pieces_to_positions.emplace_back(player.bitboards.rooks, rooks_positions[i]);
				pieces_to_positions.emplace_back(player.bitboards.queens, queens_positions[i]);

				unsigned char mask = 0x3f;
				for (auto& [bitboard, positions] : pieces_to_positions) {

					// Check if pieces are in the correct positions
					int final_square = 0;
					while (bitboard != 0 && final_square <= 63) {
						int squares_to_skip = std::countr_zero(bitboard);
						final_square += squares_to_skip;

						bool isInVector = false;
						auto pos = positions.begin();
						for (auto it = positions.begin(); it != positions.end(); it++) {
							if (*it == final_square) {
								isInVector = true;
								pos = it;
							}
						}
						Assert::IsTrue(isInVector);
						positions.erase(pos);

						bitboard >>= (squares_to_skip + 1);
						final_square++;
					}

					// Check if all positions were set
					int a = int(positions.size());
					Assert::AreEqual(int(positions.size()), 0);
					Assert::AreEqual(bitboard, 0ull);
				}
			}
		}

		TEST_METHOD(PerftShallowTest) { // Tests perft, but only to depth 4, PerftTest has the full test suite.
			int start_depth = 1;
			int max_depth = 4;

			// Perft test suit positions from http://www.rocechess.ch/perft.html
			std::ifstream in_file;

			// Get file's directory
			std::string file_path = __FILE__;
			std::string dir_path = file_path.substr(0, file_path.rfind("\\"));

			in_file.open(dir_path + "\\perftsuite.epd");
			Assert::IsTrue(in_file.is_open());
			
			std::string line;
			while (std::getline(in_file, line)) {
				std::stringstream perft_str(line);
				std::string seg;
				std::vector<std::string> perft;

				while (std::getline(perft_str, seg, ';')) {
					perft.push_back(seg);
				}

				auto [player, opponent, half_moves, full_moves] = FENToPosition(perft[0]);
				Assert::AreNotEqual(full_moves, -1);

				for (int depth = start_depth; depth <= max_depth; depth++) {
					if (depth + 1 > perft.size()) break;
					unsigned long long expected_num_nodes = std::stoull(perft[depth].substr(3));
					unsigned long long num_nodes = Perft(depth, player, opponent);
					Assert::AreEqual(expected_num_nodes, num_nodes);
				}
			}
		}

		TEST_METHOD(LinearLayerTest) { // Tests the linear layer of the NNUE
			LinearLayer linear_layers[2] = { LinearLayer(32, 1), LinearLayer(32, 32) };

			alignas(64) int8_t weights[32][32] = {};
			alignas(64) int32_t bias[32] = {};
			alignas(64) int8_t inputs[32] = {};
			alignas(64) int32_t outputs[32] = {};

			for (LinearLayer& ln : linear_layers) {
				// Generate weights and inputs
				for (int i = 0; i < ln.num_inputs; i++) {
					bias[i] = 1;
					inputs[i] = i;
					for (int j = 0; j < ln.num_outputs; j++) {
						weights[j][i] = i;
					}
				}

				// Set weights and bias
				ln.weights = (int8_t*)weights;
				ln.bias = (int32_t*)bias;

				ln.process_linear_layer(inputs, outputs);

				for (int j = 0; j < ln.num_outputs; j++) {
					Assert::AreEqual(outputs[j], 10417);
				}
			}
		}
	};
}
