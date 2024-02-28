#pragma once
#include "BitBoards.h"
#include "Locations.h"
#include "Pin.h"
#include <array>

class Player {
public:
	bool is_white, can_castle_king_side, can_castle_queen_side;
	int num_pawns, num_knights, num_bishops, num_rooks, num_queens;
	unsigned int move_id = 1;
	BitBoards bitboards;
	Locations locations;
	std::array<Pin, 64> pins;

	Player(bool is_white);
	bool isPinned(location location) const;
};
