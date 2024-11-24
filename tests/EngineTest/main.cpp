#include "BitBoards.h"
#include "GameOutcomes.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <string>

using std::cout, std::cin;

constexpr int size_TT = 256; // Size in mb, must be a power of two

void time_engine(Position& initial_pos, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);

int main() {

	// Load Magic Bitboards
	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) {
		cout << "Couldn't load Magic Bitboards file.";
		return 2;
	}

	// Set up initial position
	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	Position initial_pos = FENToPosition(FEN);
	if (initial_pos.full_moves == -1) {
		cout << "Invalid Fen position.\n";
		return 1;
	}

	zobrist_keys = ZobristKeys();

	time_engine(initial_pos, magic_bitboards, zobrist_keys);

}

void time_engine(Position& initial_pos, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	int search_depth;
	cout << "Enter depth to be searched to: ";
	cin >> search_depth;

	unsigned long long hash = zobrist_keys.positionToHash(initial_pos.player, initial_pos.opponent);
	HashPositions positions(hash);
	tt = TranspositionTable(size_TT, initial_pos.player.bitboards.all_pieces);

	nnue.setPosition(initial_pos.player, initial_pos.opponent);

	auto start = std::chrono::high_resolution_clock::now();
	FindBestMoveItrDeepening(search_depth, initial_pos.player, initial_pos.opponent, positions, initial_pos.half_moves);
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = duration_cast<std::chrono::milliseconds>(stop - start);

	cout << "The engine took " << duration.count() << " ms to search to a depth of " << search_depth << '\n';
}
