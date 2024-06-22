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
	unsigned short best_move;
	unsigned short depth;
};

// Returns the move with the highest evaluation
SearchResult FindBestMoveItrDeepening(std::chrono::milliseconds time, Player& player, Player& opponent, HashPositions& positions, int half_moves,
										const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt);

// Returns the move with the highest evaluation
SearchResult FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
									  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt);

SearchResult FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
							const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt);
