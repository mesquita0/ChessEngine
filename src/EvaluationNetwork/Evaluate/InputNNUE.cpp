#include "InputNNUE.h"
#include "PieceTypes.h"
#include <bit>
#include <cstdint>
#include <vector>

NNUEIndex getIndexNNUE(location square, PieceType piece_type, const Player& piece_owner, const Player& opponent) {
	location white_king = piece_owner.is_white ? piece_owner.locations.king : opponent.locations.king;
	location black_king = opponent.is_white ? piece_owner.locations.king : opponent.locations.king;
	
	// 10 piece types with 64 possible squares each
	int start_index_white = white_king * 10 * 64;
	int start_index_black = flip_square(black_king) * 10 * 64;

	int w_offset = (!piece_owner.is_white ? 64 : 0);
	int b_offset = (piece_owner.is_white ? 64 : 0);

	int loc_white = start_index_white + 2 * 64 * piece_type + square + w_offset;
	int loc_black = start_index_black + 2 * 64 * piece_type + flip_square(square) + b_offset;

	return { loc_white, loc_black };
}

std::vector<NNUEIndex> getIndexesNNUE(const Player& player, const Player& opponent) {
	std::vector<NNUEIndex> indexes;
	indexes.reserve(30);

	uint64_t all_pieces = player.bitboards.all_pieces & ~player.bitboards.king & ~opponent.bitboards.king;
	int current_square = 0;

	// Get all indexes
	while (all_pieces != 0 && current_square <= 63) {
		int squares_to_skip = std::countr_zero(all_pieces);
		current_square += squares_to_skip;

		const uint64_t bitboard_piece = (1LL << current_square);
		const Player* piece_owner = (player.bitboards.friendly_pieces & bitboard_piece) ? &player   : &opponent;
		const Player* opp		  =	(player.bitboards.friendly_pieces & bitboard_piece) ? &opponent : &player  ;

		// Find out which piece is on that square
		PieceType piece_type;
		if (piece_owner->bitboards.pawns & bitboard_piece)
			piece_type = Pawn;
		else if (piece_owner->bitboards.knights & bitboard_piece)
			piece_type = Knight;
		else if (piece_owner->bitboards.bishops & bitboard_piece)
			piece_type = Bishop;
		else if (piece_owner->bitboards.rooks & bitboard_piece)
			piece_type = Rook;
		else
			piece_type = Queen;

		indexes.push_back(getIndexNNUE(current_square, piece_type, *piece_owner, *opp));

		all_pieces >>= (squares_to_skip + 1);
		current_square++;
	}

	return indexes;
}
