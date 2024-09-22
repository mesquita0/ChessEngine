#include "HistoryTable.h"
#include "Locations.h"
#include "Moves.h"

void HistoryTable::record(bool is_white, unsigned short move_flag, location final_square, int depth) {
	PieceType piece_type = Pawn;
	switch (move_flag)
	{
	case knight_move:
		piece_type = Knight;
		break;

	case bishop_move:
		piece_type = Bishop;
		break;

	case rook_move:
		piece_type = Rook;
		break;

	case queen_move:
		piece_type = Queen;
		break;

	case king_move:
		piece_type = King;
		break;

	case castle_king_side:
		piece_type = King;
		break;

	case castle_queen_side:
		piece_type = King;
		break;

	default:
		break;
	}

	record(is_white, piece_type, final_square, depth);
}
