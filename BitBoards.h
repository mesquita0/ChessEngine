#pragma once
#include "Locations.h"

/*
	A bitboard is a 64-bit unsigned long long, where the nth bit represents if there is a piece
	at square n, the number of the squares are the same as in Locations.
*/
class BitBoards {
public:
	unsigned long long all_pieces, friendly_pieces, pawns, knights, bishops, rooks, queens, king, attacks, squares_to_uncheck;

	BitBoards() = default;

	inline void addPawn(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		pawns |= (1LL << loc);
	}

	inline void addKnight(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		knights |= (1LL << loc);
	}

	inline void addBishop(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		bishops |= (1LL << loc);
	}

	inline void addRook(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		rooks |= (1LL << loc);
	}

	inline void addQueen(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		queens |= (1LL << loc);
	}

	inline void addKing(location loc) {
		all_pieces |= (1LL << loc);
		friendly_pieces |= (1LL << loc);
		king |= (1LL << loc);
	}

	inline void removePawn(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		pawns ^= bitboard_piece;
	}

	inline void removeKnight(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		knights ^= bitboard_piece;
	}

	inline void removeBishop(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		bishops ^= bitboard_piece;
	}

	inline void removeRook(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		rooks ^= bitboard_piece;
	}

	inline void removeQueen(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		queens ^= bitboard_piece;
	}

	inline void removeKing(location loc) {
		unsigned long long bitboard_piece = (1LL << loc);
		all_pieces ^= bitboard_piece;
		friendly_pieces ^= bitboard_piece;
		king ^= bitboard_piece;
	}
};
