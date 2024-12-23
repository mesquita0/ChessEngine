#pragma once
#include "Position.h"
#include <array>
#include <cstdint>

class ZobristKeys {
	std::array<uint64_t, 776> keys = {};

public:
	uint64_t* white_pawn;
	uint64_t* white_knight;
	uint64_t* white_bishop;
	uint64_t* white_rook;
	uint64_t* white_queen;
	uint64_t* white_king;

	uint64_t* black_pawn;
	uint64_t* black_knight;
	uint64_t* black_bishop;
	uint64_t* black_rook;
	uint64_t* black_queen;
	uint64_t* black_king;

	uint64_t* en_passant_file;

	uint64_t is_black_to_move;
	uint64_t white_castle_king_side;
	uint64_t white_castle_queen_side;
	uint64_t black_castle_king_side;
	uint64_t black_castle_queen_side;

	ZobristKeys();

	unsigned long long positionToHash(const Player& player, const Player& opponent);
};

inline ZobristKeys zobrist_keys = ZobristKeys(); // Global Zobrist Keys
