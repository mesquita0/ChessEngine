#pragma once
#include "Locations.h"

struct Pin {
	unsigned int id_move_pinned = 0; // player move id starts at 1
	bool is_pin_diagonal;
	location location_pinner;
	unsigned long long squares_to_unpin;
};
