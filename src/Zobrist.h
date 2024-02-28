#pragma once
#include "Position.h"
#include <array>

typedef unsigned long long u64;

class ZobristKeys {
	std::array<u64, 781> keys = {};

	u64* white_pawn;
	u64* white_knight;
	u64* white_bishop;
	u64* white_rook;
	u64* white_queen;
	u64* white_king;

	u64* black_pawn;
	u64* black_knight;
	u64* black_bishop;
	u64* black_rook;
	u64* black_queen;
	u64* black_king;

	u64* is_black_to_move;
	u64* white_castle_queen_side;
	u64* white_castle_king_side;
	u64* black_castle_queen_side;
	u64* black_castle_king_side;

public:
	ZobristKeys();

	u64 positionToHash(const Position& position);
	u64 makeMove(u64 current_hash, unsigned short move);
	u64 unmakeMove(u64 current_hash, unsigned short move);
};
