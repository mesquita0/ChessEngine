#pragma once
#include <array>

constexpr short PAWN = 1;
constexpr short KNIGHT = 2;
constexpr short BISHOP = 3;
constexpr short ROOK = 4;
constexpr short QUEEN = 5;
constexpr unsigned char PIN = 0b01000000;

typedef unsigned char location;

/*
	A location is a 8-bit unsigned char, first bit is 1 if it is a
	valid location, else 0, second bit represents if piece is pinned,
	last 6 bits represents the square that the piece is in, 0
	corresponds to a8, 1 to b8, ..., 8 to a7, ..., 64 to h1, king
	and en_passant_target do not have flags in the first 2 bits.
*/
class Locations {
public:
	std::array<location, 8> pawns;
	std::array<location, 10> knights, bishops, rooks;
	std::array<location, 9> queens;
	location king;
	location en_passant_target;

	Locations() = default;

	void addPawn(location loc);
	void addKnight(location loc);
	void addBishop(location loc);
	void addRook(location loc);
	void addQueen(location loc);
	void removePawn(location loc);
	void removeKnight(location loc);
	void removeBishop(location loc);
	void removeRook(location loc);
	void removeQueen(location loc);
	void movePawn(location current_location, location new_location);
	void moveKnight(location current_location, location new_location);
	void moveBishop(location current_location, location new_location);
	void moveRook(location current_location, location new_location);
	void moveQueen(location current_location, location new_location);
	inline void moveKing(location new_location) { king = new_location; }
};
