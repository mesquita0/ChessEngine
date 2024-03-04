#pragma once
#include "MagicBitboards.h"
#include "Player.h"
#include "Zobrist.h"

/*
	Used to test and measure performance of move generation function as well 
	as make and unmake moves functions.
	Ignores draws by repetition, 50 moves rule and insufficient material, terminal
	nodes (checkmate or stalemate) are not counted, it uses bulk counting.
*/
unsigned long long Perft(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);

unsigned long long PerftDivide(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);
