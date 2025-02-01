#pragma once
#include "GameOutcomes.h"
#include "MagicBitboards.h"
#include "Moves.h"
#include "Player.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include <chrono>

struct SearchResult {
	int evaluation;
	unsigned short best_move, ponder, depth;
};

void stopSearch(int id);

// Returns the move with the highest evaluation
void FindBestMoveItrDeepening(std::chrono::milliseconds time, Player& player, Player& opponent, HashPositions& positions, int half_moves, SearchResult& result);
inline SearchResult FindBestMoveItrDeepening(std::chrono::milliseconds time, Player& player, Player& opponent, HashPositions& positions, int half_moves) {
	SearchResult result;
	FindBestMoveItrDeepening(time, player, opponent, positions, half_moves, result);
	return result;
}

// Returns the move with the highest evaluation
void FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves, SearchResult& result);
inline SearchResult FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves) {
	SearchResult result;
	FindBestMoveItrDeepening(depth, player, opponent, positions, half_moves, result);
	return result;
}

SearchResult FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves);
