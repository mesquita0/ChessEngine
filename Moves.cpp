#include "Moves.h"
#include "MagicBitboards.h"
#include "Locations.h"
#include "Player.h"
#include <array>
#include <bit>

constexpr unsigned char location_mask = 0b00111111;
constexpr unsigned long long castle_king_side_white_mask = 0b01100000;
constexpr unsigned long long castle_queen_side_white_pieces_mask = 0b01110;
constexpr unsigned long long castle_queen_side_white_attacks_mask = 0b01100;
constexpr unsigned long long castle_king_side_black_mask = 0b01100000ull << 56;
constexpr unsigned long long castle_queen_side_black_pieces_mask = 0b01110ull << 56;
constexpr unsigned long long castle_queen_side_black_attacks_mask = 0b01100ull << 56;
constexpr std::array<unsigned short, 4> promotions = { promotion_knight, promotion_bishop, promotion_rook, promotion_queen };

using std::array;

inline unsigned long long slidingMoves(const MagicBitboard& magic_bitboard, unsigned long long pieces);
inline void addMovesFromAttacksBitboard(location start_square, bool is_in_check, unsigned long long squares_to_uncheck, 
										unsigned long long bitboard_attacks, unsigned short move_flag, Moves* moves);
inline unsigned long long squaresToUncheckBishop(location opponent_king_location, location bishop_location, const MagicBitboards& magic_bitboards);
inline unsigned long long squaresToUncheckRook(location opponent_king_location, location rook_location, const MagicBitboards& magic_bitboards);
inline bool canMove(bool is_in_check, location final_square, unsigned long long squares_to_uncheck);
void setPin(Player& player, const Player& opponent, location piece_location, bool is_pin_diagonal, const MagicBitboards& magic_bitboards);

void Moves::generateMoves(const Player& player, const Player& opponent, const MagicBitboards& magic_bitboards) {
	this->num_moves = 0;
	bool is_in_check = false;

	// King moves
	for (const short move : magic_bitboards.king_moves[player.locations.king]) {
		short final_square = move & 0b111111;
		if ((1LL << final_square) & player.bitboards.friendly_pieces) continue;
		else if ((1LL << final_square) & opponent.bitboards.attacks) continue;

		this->addMove(king_move | move);
	}

	// If King is in check
	if (opponent.bitboards.attacks & player.bitboards.king) {
		is_in_check = true;

		// If double check only moves are king moves
		if (!player.bitboards.squares_to_uncheck) return;
	}

	// Pawns moves
	short move_one_square = player.is_white ? 8 : -8;
	short move_two_squares = player.is_white ? 16 : -16;
	short location_unmoved_a_pawn = player.is_white ? 8 : 48;
	short capture_right = player.is_white ? 9 : -9;
	short capture_left = player.is_white ? 7 : -7;
	short right_edge = player.is_white ? 7 : 0;
	short left_edge = player.is_white ? 0 : 7;

	int pawns_searched = 0;
	for (location pawn_location : player.locations.pawns) {
		if (pawns_searched == player.num_pawns) break;
		if (pawn_location == 0) continue;
		pawn_location &= location_mask;
		bool is_pinned = player.isPinned(pawn_location);
		short file = pawn_location % 8;
		pawns_searched++;

		// Pawn moves foward
		if (!is_pinned || !player.pins[pawn_location].is_pin_diagonal) {

			// If pawn can move one square
			location new_location = pawn_location + move_one_square;

			// If pawn is pinned and pin is not diagonal, then theres two possibilities, either the enemy rook (pinner)
			// is in the same rank as the pawn and the king (when the pawn can't move at all) or is in the same file as 
			// the pawn and king (when there's no restriction to the pawn movement). Therefore when the pawn is pinned
			// to a rook in the same rank as itself we can skip all the calculations of moves for that pawn, and when
			// the rook is in the same file as the pawn we can calculate the pawns moves foward as if it wasn't pineed.
			// Check this condition using the fact that if pin is by file then new_location will be in squares to unpin.
			if (is_pinned && !((1LL << new_location) & player.pins[pawn_location].squares_to_unpin)) continue;

			if (!(player.bitboards.all_pieces & (1LL << new_location))) {

				if (canMove(is_in_check, new_location, player.bitboards.squares_to_uncheck)) {
					if (new_location >= 56 || new_location <= 7) {
						for (const short promotion : promotions) this->addMove(promotion, pawn_location, new_location);
					}
					else this->addMove(pawn_move, pawn_location, new_location);
				}

				// If pawn can move two squares
				new_location = pawn_location + move_two_squares;
				if (canMove(is_in_check, new_location, player.bitboards.squares_to_uncheck)) {
					short distance = pawn_location - location_unmoved_a_pawn;
					if ((distance <= 7 && distance >= 0) && !(player.bitboards.all_pieces & (1LL << (new_location)))) {
						this->addMove(pawn_move_two_squares, pawn_location, new_location);
					}
				}
			}
		}

		// Pawn captures
		if (!is_pinned) {
			// If pawn can capture other piece at its right
			location new_location = pawn_location + capture_right;
			if ((opponent.bitboards.friendly_pieces & (1LL << new_location)) && (file != right_edge) && canMove(is_in_check, new_location, player.bitboards.squares_to_uncheck)) {
				if (new_location >= 56 || new_location <= 7) {
					for (const short promotion : promotions) this->addMove(promotion, pawn_location, new_location);
				} 
				else this->addMove(pawn_move, pawn_location, new_location);
			}

			// If pawn can capture other piece at its right
			new_location = pawn_location + capture_left;
			if ((opponent.bitboards.friendly_pieces & (1LL << new_location)) && (file != left_edge) && canMove(is_in_check, new_location, player.bitboards.squares_to_uncheck)) {
				if (new_location >= 56 || new_location <= 7) {
					for (const short promotion : promotions) this->addMove(promotion, pawn_location, new_location);
				}
				else this->addMove(pawn_move, pawn_location, new_location);
			}
		}
		else if (player.pins[pawn_location].is_pin_diagonal && !is_in_check) { // If pawn can capture its pinner
			short distance = player.pins[pawn_location].location_pinner - pawn_location;
			if (distance == capture_right || distance == capture_left) {
				if (player.pins[pawn_location].location_pinner >= 56 || player.pins[pawn_location].location_pinner <= 7) {
					for (const short promotion : promotions) this->addMove(promotion, pawn_location, player.pins[pawn_location].location_pinner);
				}
				else this->addMove(pawn_move, pawn_location, player.pins[pawn_location].location_pinner);
			}
		}
		
		// En Passants
		if ((!is_pinned || player.pins[pawn_location].is_pin_diagonal) && opponent.locations.en_passant_target != 0) {
			short distance = opponent.locations.en_passant_target - pawn_location;
			if ((distance == capture_right && file != right_edge) || (distance == capture_left && file != left_edge)) {

				// Make en passant move
				location location_pawn_captured = opponent.locations.en_passant_target - move_one_square;
				unsigned long long all_pieces = opponent.bitboards.all_pieces ^ (1LL << pawn_location);
				all_pieces ^= (1LL << location_pawn_captured);
				all_pieces |= (1LL << opponent.locations.en_passant_target);
				
				// Check if en passant wont leave king in check
				unsigned long long rooks_and_queens = opponent.bitboards.rooks | opponent.bitboards.queens;
				bool can_en_passant = true;
				int square = 0;
				while (rooks_and_queens != 0) {
					int squares_to_skip = std::countr_zero(rooks_and_queens);
					square += squares_to_skip;

					unsigned long long squares_to_uncheck = squaresToUncheckRook(player.locations.king, square, magic_bitboards);
					if (((squares_to_uncheck ^ (1LL << square)) & all_pieces) == 0) {
						can_en_passant = false;
						break;
					}

					if (square == 63) break;
					rooks_and_queens >>= (squares_to_skip + 1);
					square++;
				}

				// Add move if player king is not under attack
				if (can_en_passant) this->addMove(en_passant, pawn_location, opponent.locations.en_passant_target);
			}
		}
	}

	// Knights moves
	int knights_searched = 0;
	for (location knight_location : player.locations.knights) {
		if (knights_searched == player.num_knights) break;
		if (knight_location == 0) continue;
		knight_location &= location_mask;
		if (player.isPinned(knight_location)) continue;

		for (const short move : magic_bitboards.knight_moves[knight_location]) {
			short final_square = move & 0b111111;
			if ((1LL << final_square) & player.bitboards.friendly_pieces) continue;
			if (!canMove(is_in_check, final_square, player.bitboards.squares_to_uncheck)) continue;

			this->addMove(knight_move | move);
		}
		knights_searched++;
	}

	// Bishops moves
	int bishops_searched = 0;
	for (location bishop_location : player.locations.bishops) {
		if (bishops_searched == player.num_bishops) break;
		if (bishop_location == 0) continue;
		bishop_location &= location_mask;
		bool is_pinned = player.isPinned(bishop_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[bishop_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[bishop_location].squares_to_unpin;

		addMovesFromAttacksBitboard(bishop_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, bishop_move, this);
		bishops_searched++;
	}

	// Rook moves
	int rooks_searched = 0;
	for (location rook_location : player.locations.rooks) {
		if (rooks_searched == player.num_rooks) break;
		if (rook_location == 0) continue;
		rook_location &= location_mask;
		bool is_pinned = player.isPinned(rook_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[rook_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[rook_location].squares_to_unpin;

		addMovesFromAttacksBitboard(rook_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, rook_move, this);
		rooks_searched++;
	}

	// Queen moves
	int queens_searched = 0;
	for (location queen_location : player.locations.queens) {
		if (queens_searched == player.num_queens) break;
		if (queen_location == 0) continue;
		queen_location &= location_mask;
		bool is_pinned = player.isPinned(queen_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[queen_location], player.bitboards.all_pieces);
		bitboard_attacks |= slidingMoves(magic_bitboards.rooks_magic_bitboards[queen_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[queen_location].squares_to_unpin;

		addMovesFromAttacksBitboard(queen_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, queen_move, this);
		queens_searched++;
	}

	// Castle
	if (!is_in_check) {
		unsigned long long mask_king_side = player.is_white ? castle_king_side_white_mask : castle_king_side_black_mask;
		unsigned long long mask_queen_side_pieces = player.is_white ? castle_queen_side_white_pieces_mask : castle_queen_side_black_pieces_mask;
		unsigned long long mask_queen_side_attacks = player.is_white ? castle_queen_side_white_attacks_mask : castle_queen_side_black_attacks_mask;

		if (player.can_castle_king_side && !(player.bitboards.all_pieces & mask_king_side) && !(opponent.bitboards.attacks & mask_king_side)) {
			this->addMove(castle_king_side, player.locations.king, player.locations.king + 2);
		}

		if (player.can_castle_queen_side && !(player.bitboards.all_pieces & mask_queen_side_pieces) && !(opponent.bitboards.attacks & mask_queen_side_attacks)) {
			this->addMove(castle_queen_side, player.locations.king, player.locations.king - 2);
		}
	}
}

AttacksInfo generateAttacksInfo(bool is_white, const Locations& locations, unsigned long long all_pieces,
								int num_pawns, int num_knights, int num_bishops, int num_rooks, int num_queens,
							    location opponent_king_location, const MagicBitboards& magic_bitboards) {
	unsigned long long attacks_bitboard = 0;
	unsigned long long opponent_squares_to_uncheck = 0;
	unsigned long long opponent_king = (opponent_king_location < 64) ? (1LL << opponent_king_location) : 0;

	// Pawn moves
	short capture_right = is_white ? 9 : -9;
	short capture_left = is_white ? 7 : -7;
	short right_edge = is_white ? 7 : 0;
	short left_edge = is_white ? 0 : 7;

	int pawns_searched = 0;
	for (location pawn_location : locations.pawns) {
		if (pawns_searched == num_pawns) break;
		if (pawn_location == 0) continue;
		pawn_location &= location_mask;
		short file = pawn_location % 8;
		unsigned long long pawn_attacks = 0;
		pawns_searched++;

		// If pawn can capture to at its right
		if (file != right_edge) {
			pawn_attacks |= (1LL << (pawn_location + capture_right));
		}

		// If pawn can capture to at its left
		if (file != left_edge) {
			pawn_attacks |= (1LL << (pawn_location + capture_left));
		}

		if (pawn_attacks & opponent_king) {
			opponent_squares_to_uncheck = (1LL << pawn_location);
		}

		attacks_bitboard |= pawn_attacks;
	}

	// Knight moves
	int knights_searched = 0;
	for (location knight_location : locations.knights) {
		if (knights_searched == num_knights) break;
		if (knight_location == 0) continue;
		knight_location &= location_mask;
		knights_searched++;

		unsigned long long knight_attacks = magic_bitboards.knights_attacks_array[knight_location];
		attacks_bitboard |= knight_attacks;

		if (knight_attacks & opponent_king) {
			opponent_squares_to_uncheck = (1LL << knight_location);
		}
	}

	// Bishop moves
	int bishops_searched = 0;
	for (location bishop_location : locations.bishops) {
		if (bishops_searched == num_bishops) break;
		if (bishop_location == 0) continue;
		bishop_location &= location_mask;
		bishops_searched++;

		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[bishop_location], all_pieces ^ opponent_king);
		attacks_bitboard |= bishop_attacks;

		if (bishop_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckBishop(opponent_king_location, bishop_location, magic_bitboards);
			}
		}
	}


	// Rook moves
	int rooks_searched = 0;
	for (location rook_location : locations.rooks) {
		if (rooks_searched == num_rooks) break;
		if (rook_location == 0) continue;
		rook_location &= location_mask;
		rooks_searched++;

		unsigned long long rook_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[rook_location], all_pieces ^ opponent_king);
		attacks_bitboard |= rook_attacks;

		if (rook_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckRook(opponent_king_location, rook_location, magic_bitboards);
			}
		}
	}

	// Quuen moves
	int queens_searched = 0;
	for (location queen_location : locations.queens) {
		if (queens_searched == num_queens) break;
		if (queen_location == 0) continue;
		queen_location &= location_mask;
		queens_searched++;

		// Bishop attacks
		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[queen_location], all_pieces ^ opponent_king);
		attacks_bitboard |= bishop_attacks;

		if (bishop_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckBishop(opponent_king_location, queen_location, magic_bitboards);
			}
		}

		// Rook attacks
		unsigned long long rook_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[queen_location], all_pieces ^ opponent_king);
		attacks_bitboard |= rook_attacks;

		if (rook_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckRook(opponent_king_location, queen_location, magic_bitboards);
			}
		}
	}

	// King moves
	attacks_bitboard |= (magic_bitboards.king_attacks_array[locations.king]);

	return { attacks_bitboard, opponent_squares_to_uncheck };
}

inline unsigned long long slidingMoves(const MagicBitboard& magic_bitboard, unsigned long long pieces) {
	int index = ((pieces | magic_bitboard.mask) * magic_bitboard.magic_number) >> magic_bitboard.num_shifts;
	return magic_bitboard.ptr_attacks_array[index];
}

inline void addMovesFromAttacksBitboard(location start_square, bool is_in_check, unsigned long long squares_to_uncheck, 
										unsigned long long bitboard_attacks, unsigned short move_flag, Moves* moves) {
	int final_square = 0;
	while (bitboard_attacks != 0) {
		int squares_to_skip = std::countr_zero(bitboard_attacks);
		final_square += squares_to_skip;

		// Add move if not in check or move covers check
		if (canMove(is_in_check, final_square, squares_to_uncheck)) {
			moves->addMove(move_flag, start_square, final_square);
		}
		
		// If squares to skip is 63 (move to the last square) then squares to skip + 1 will be 64
		// which is undefined behavior when bitwise shifting a 64 bit int
		if (squares_to_skip == 63) break;

		bitboard_attacks >>= (squares_to_skip + 1);
		final_square++;
	}
}

inline unsigned long long squaresToUncheckBishop(location opponent_king_location, location bishop_location, const MagicBitboards& magic_bitboards) {
	return magic_bitboards.bishop_squares_uncheck[bishop_location][opponent_king_location];
}

inline unsigned long long squaresToUncheckRook(location opponent_king_location, location rook_location, const MagicBitboards& magic_bitboards) {
	return magic_bitboards.rook_squares_uncheck[rook_location][opponent_king_location];
}

inline bool canMove(bool is_in_check, location final_square, unsigned long long squares_to_uncheck) {
	return (!is_in_check || ((1LL << final_square) & squares_to_uncheck));
}

void setPins(Player& player, Player& opponent, const MagicBitboards& magic_bitboards) {
	if (player.move_id == 0) { // Reset pins if moves id overflowed
		for (Pin& pin : player.pins) {
			pin.id_move_pinned = 0;
		}

		for (Pin& pin : opponent.pins) {
			pin.id_move_pinned = 0;
		}

		player.move_id = 1;
		opponent.move_id = 1;
	}

	// Bishops pins
	int bishops_searched = 0;
	for (location bishop_location : opponent.locations.bishops) {
		if (bishops_searched == opponent.num_bishops) break;
		if (bishop_location == 0) continue;
		bishop_location &= location_mask;
		bishops_searched++;

		setPin(player, opponent, bishop_location, true, magic_bitboards);
	}

	// Rooks pins
	int rooks_searched = 0;
	for (location rook_location : opponent.locations.rooks) {
		if (rooks_searched == opponent.num_rooks) break;
		if (rook_location == 0) continue;
		rook_location &= location_mask;
		rooks_searched++;

		setPin(player, opponent, rook_location, false, magic_bitboards);
	}

	// Queens pins
	int queens_searched = 0;
	for (location queen_location : opponent.locations.queens) {
		if (queens_searched == opponent.num_queens) break;
		if (queen_location == 0) continue;
		queen_location &= location_mask;
		queens_searched++;

		setPin(player, opponent, queen_location, true, magic_bitboards);
		setPin(player, opponent, queen_location, false, magic_bitboards);
	}
}

void setPin(Player& player, const Player& opponent, location piece_location, bool is_pin_diagonal, const MagicBitboards& magic_bitboards) {
	unsigned long long squares_to_uncheck;
	if (is_pin_diagonal) squares_to_uncheck = squaresToUncheckBishop(player.locations.king, piece_location, magic_bitboards);
	else squares_to_uncheck = squaresToUncheckRook(player.locations.king, piece_location, magic_bitboards);
	if (!squares_to_uncheck) return;

	unsigned long long friendly_pieces_covering_check = squares_to_uncheck & player.bitboards.friendly_pieces;
	if (!friendly_pieces_covering_check) return;
	unsigned long long opponent_pieces_covering_check = (squares_to_uncheck & opponent.bitboards.friendly_pieces) ^ (1LL << piece_location);

	// Bit hack to see if only one bit is set in friendly_pieces_covering_check
	bool only_one_friendly_piece_covering_check = ((friendly_pieces_covering_check & (friendly_pieces_covering_check - 1)) == 0);

	if (only_one_friendly_piece_covering_check && !opponent_pieces_covering_check) {
		int index_pinned_piece = std::countr_zero(friendly_pieces_covering_check);
			
		// Set piece type
		short piece_type;
		if (player.bitboards.pawns & friendly_pieces_covering_check) {
			piece_type = PAWN;
		}
		else if (player.bitboards.knights & friendly_pieces_covering_check) {
			piece_type = KNIGHT;
		}
		else if (player.bitboards.bishops & friendly_pieces_covering_check) {
			piece_type = BISHOP;
		}
		else if (player.bitboards.rooks & friendly_pieces_covering_check) {
			piece_type = ROOK;
		}
		else {
			piece_type = QUEEN;
		}

		// Add pin
		player.pins[index_pinned_piece].is_pin_diagonal = is_pin_diagonal;
		player.pins[index_pinned_piece].location_pinner = piece_location;
		player.pins[index_pinned_piece].squares_to_unpin = squares_to_uncheck;
		player.pins[index_pinned_piece].id_move_pinned = player.move_id;
	}
}

unsigned short Moves::isMoveValid(location start_square, location final_square) {
	unsigned short desired_move = (start_square << 6 | final_square);
	for (const unsigned short move : moves) {
		if (desired_move == (move & 0xfff)) {
			return move;
		}
	}

	return 0;
}
