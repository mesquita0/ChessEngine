#include "Player.h"
#include "BitBoards.h"
#include "Locations.h"
#include "MagicBitboards.h"

constexpr unsigned char location_mask = 0b00111111;

Player::Player(bool is_white) {
	this->is_white = is_white;
	can_castle_king_side = true;
	can_castle_queen_side = true;
	num_pawns = 0;
	num_knights = 0;
	num_bishops = 0;
	num_rooks = 0;
	num_queens = 0;
	bitboards = BitBoards();
	locations = Locations();
	pins = {};
}

bool Player::isPinned(location location) const {
	return (this->pins[location].id_move_pinned == this->move_id);
}
