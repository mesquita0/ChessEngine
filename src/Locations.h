#pragma once

/*
	A location stores the square that the piece is in, 
	0 corresponds to a8, 1 to b8, ..., 8 to a7, ..., 64 to h1.
*/
typedef unsigned char location;

class Locations {
public:
	location king;
	location en_passant_target;

	Locations() = default;

	inline void moveKing(location new_location) { king = new_location; }
};
