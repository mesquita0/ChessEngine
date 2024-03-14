#pragma once
#include "GameOutcomes.h"
#include "MagicBitboards.h"
#include "Moves.h"
#include "Player.h"
#include "Zobrist.h"

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions,
		   int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, int depth_searched);

// Returns the move with the highest evaluation
unsigned short FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions,
							int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);
