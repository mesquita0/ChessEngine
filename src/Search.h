#pragma once
#include "MagicBitboards.h"
#include "Moves.h"
#include "Player.h"
#include "Zobrist.h"
#include <vector>

double Search(int depth, Player& player, Player& opponent, std::vector<unsigned long long>& positions, int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);

// Returns the move with the highest evaluation
unsigned short FindBestMove(int depth, Player& player, Player& opponent, std::vector<unsigned long long>& positions, int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);
