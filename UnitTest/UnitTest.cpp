#include "pch.h"
#include "CppUnitTest.h"
#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "Moves.h"
#include "MagicBitboards.h"
#include <array>
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
			MagicBitboards magic_bitboards;
			bool loaded = magic_bitboards.loadMagicBitboards();
			Assert::IsTrue(loaded);

			auto [pl, op, half_moves, full_moves] = FENToPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", magic_bitboards);

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

				std::vector<std::tuple<std::vector<unsigned char>, unsigned long long, std::vector<int>>> pieces_to_positions;
				pieces_to_positions.emplace_back(std::vector<unsigned char>(player.locations.pawns.begin(), player.locations.pawns.end()), player.bitboards.pawns, pawns_positions[i]);
				pieces_to_positions.emplace_back(std::vector<unsigned char>(player.locations.knights.begin(), player.locations.knights.end()), player.bitboards.knights, knights_positions[i]);
				pieces_to_positions.emplace_back(std::vector<unsigned char>(player.locations.bishops.begin(), player.locations.bishops.end()), player.bitboards.bishops, bishops_positions[i]);
				pieces_to_positions.emplace_back(std::vector<unsigned char>(player.locations.rooks.begin(), player.locations.rooks.end()), player.bitboards.rooks, rooks_positions[i]);
				pieces_to_positions.emplace_back(std::vector<unsigned char>(player.locations.queens.begin(), player.locations.queens.end()), player.bitboards.queens, queens_positions[i]);

				unsigned char mask = 0x3f;
				for (auto& [pieces_locations, bitboard, positions] : pieces_to_positions) {

					// Check if pieces are in the correct positions
					for (unsigned char location : pieces_locations) {
						if (location == 0) break;
						location &= mask;

						bool isInVector = false;
						auto pos = positions.begin();
						for (auto it = positions.begin(); it != positions.end(); it++) {
							if (*it == location) {
								isInVector = true;
								pos = it;
							}
						}
						Assert::IsTrue(isInVector);
						positions.erase(pos);
						
						Assert::IsTrue((bitboard & (1LL) << location) != 0);
						bitboard ^= (1LL) << location;
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

			MagicBitboards magic_bitboards;
			bool loaded = magic_bitboards.loadMagicBitboards();
			Assert::IsTrue(loaded);

			// Perft test suit positions from http://www.rocechess.ch/perft.html
			std::ifstream in_file;
			in_file.open("C:\\Users\\mesqu\\OneDrive\\Documentos\\Estudo\\Projetos\\ChessEngine\\UnitTest\\perftsuite.epd");
			Assert::IsTrue(in_file.is_open());
			
			std::string line;
			while (std::getline(in_file, line)) {
				std::stringstream perft_str(line);
				std::string seg;
				std::vector<std::string> perft;

				while (std::getline(perft_str, seg, ';')) {
					perft.push_back(seg);
				}

				auto [player, opponent, half_moves, full_moves] = FENToPosition(perft[0], magic_bitboards);
				Assert::AreNotEqual(full_moves, -1);

				for (int depth = start_depth; depth <= max_depth; depth++) {
					unsigned long long expected_num_nodes = std::stoull(perft[depth].substr(3));
					Assert::AreEqual(expected_num_nodes, Perft(depth, player, opponent, magic_bitboards));
				}
			}
		}
	};
}
