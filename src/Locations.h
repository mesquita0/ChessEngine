#pragma once

/*
	A location stores the square that the piece is in, 
	0 corresponds to a1, 1 to b1, ..., 8 to a2, ..., 64 to h8.
*/
typedef unsigned char location;

class Locations {
public:
	location king;
	location en_passant_target;

	Locations() = default;

	inline void moveKing(location new_location) { king = new_location; }
};
