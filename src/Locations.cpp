#include "Locations.h"
#include <array>

template <std::size_t N>
inline static void addToArray(std::array<location, N>& array, location new_location) {
	for (location& location : array) {
		if (location) continue;

		location = 0b10000000 | new_location;
		return;
	}
}

void Locations::addPawn(location loc) {
	addToArray(pawns, loc);
}

void Locations::addKnight(location loc) {
	addToArray(knights, loc);
}

void Locations::addBishop(location loc) {
	addToArray(bishops, loc);
}

void Locations::addRook(location loc) {
	addToArray(rooks, loc);
}

void Locations::addQueen(location loc) {
	addToArray(queens, loc);
}

template <std::size_t N>
inline static void movePiece(std::array<location, N>& array, location current_location, location new_location) {
	for (location& loc : array) {
		if (!loc) continue;
		if ((loc & 0b00111111) == current_location) {
			loc = 0b10000000 | new_location;
			return;
		}
	}
}

void Locations::movePawn(location current_location, location new_location) {
	movePiece(pawns, current_location, new_location);
}

void Locations::moveKnight(location current_location, location new_location) {
	movePiece(knights, current_location, new_location);
}

void Locations::moveBishop(location current_location, location new_location) {
	movePiece(bishops, current_location, new_location);
}

void Locations::moveRook(location current_location, location new_location) {
	movePiece(rooks, current_location, new_location);
}

void Locations::moveQueen(location current_location, location new_location) {
	movePiece(queens, current_location, new_location);
}

template <std::size_t N>
inline static void removePiece(std::array<location, N>& array, location piece_location) {
	for (location& location : array) {
		if (!location) continue;
		if ((location & 0b00111111) == piece_location) {
			location = 0;
			return;
		}
	}
}

void Locations::removePawn(location loc) {
	removePiece(pawns, loc);
}

void Locations::removeKnight(location loc) {
	removePiece(knights, loc);
}

void Locations::removeBishop(location loc) {
	removePiece(bishops, loc);
}

void Locations::removeRook(location loc) {
	removePiece(rooks, loc);
}

void Locations::removeQueen(location loc) {
	removePiece(queens, loc);
}
