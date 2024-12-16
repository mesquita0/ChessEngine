#include "Moves.h"
#include "HistoryTable.h"
#include "Locations.h"
#include "MagicBitboards.h"
#include "PieceTypes.h"
#include "Player.h"
#include "Position.h"
#include "TranspositionTable.h"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>

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
inline unsigned long long squaresToUncheckBishop(location opponent_king_location, location bishop_location);
inline unsigned long long squaresToUncheckRook(location opponent_king_location, location rook_location);
inline bool canMove(bool is_in_check, location final_square, unsigned long long squares_to_uncheck);
inline int getPieceValue(const Player& player, location square);
void setPin(Player& player, const Player& opponent, location piece_location, bool is_pin_diagonal);

void Moves::generateMoves(const Player& player, const Player& opponent) {
	this->num_moves = 0;
	bool is_in_check = false;

	// King moves
	for (const short move : magic_bitboards.king_moves[player.locations.king]) {
		short final_square = getFinalSquare(move);
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
	short capture_right = player.is_white ? 9 : -9;
	short capture_left = player.is_white ? 7 : -7;
	short right_edge = player.is_white ? 7 : 0;
	short left_edge = player.is_white ? 0 : 7;
	short location_unmoved_a_pawn = player.is_white ? 8 : 48;

	unsigned long long cp_pawns_bitboard = player.bitboards.pawns;
	location pawn_location = 0;
	while (cp_pawns_bitboard != 0 && pawn_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_pawns_bitboard);
		pawn_location += squares_to_skip;

		bool is_pinned = player.isPinned(pawn_location);
		short file = pawn_location % 8;

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
			if (is_pinned && !((1LL << new_location) & player.pins[pawn_location].squares_to_unpin)) {
				cp_pawns_bitboard >>= (squares_to_skip + 1);
				pawn_location++;
				continue;
			}

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

			// If pawn can capture other piece at its left
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
				unsigned long long bishops_and_queens = opponent.bitboards.bishops | opponent.bitboards.queens;
				unsigned long long rooks_and_queens   = opponent.bitboards.rooks   | opponent.bitboards.queens;
				unsigned long long opponent_pieces    = bishops_and_queens         | rooks_and_queens;

				bool can_en_passant = true;
				int square = 0;
				while (opponent_pieces != 0 && square <= 63) {
					int squares_to_skip = std::countr_zero(opponent_pieces);
					square += squares_to_skip;

					unsigned long long bitboard_square = (1LL << square);
					unsigned long long squares_to_uncheck = 0;

					if (bitboard_square & bishops_and_queens) squares_to_uncheck |= squaresToUncheckBishop(player.locations.king, square);
					if (bitboard_square & rooks_and_queens) squares_to_uncheck |= squaresToUncheckRook(player.locations.king, square);

					if (((squares_to_uncheck ^ bitboard_square) & all_pieces) == 0) {
						can_en_passant = false;
						break;
					}

					opponent_pieces >>= (squares_to_skip + 1);
					square++;
				}

				// Add move if it won't leave the player's king under attack
				if (can_en_passant) this->addMove(en_passant, pawn_location, opponent.locations.en_passant_target);
			}
		}

		cp_pawns_bitboard >>= (squares_to_skip + 1);
		pawn_location++;
	}

	// Knights moves
	unsigned long long cp_knights_bitboard = player.bitboards.knights;
	location knight_location = 0;
	while (cp_knights_bitboard != 0 && knight_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_knights_bitboard);
		knight_location += squares_to_skip;

		if (!player.isPinned(knight_location)) {
			for (const short move : magic_bitboards.knight_moves[knight_location]) {
				short final_square = getFinalSquare(move);
				if ((1LL << final_square) & player.bitboards.friendly_pieces) continue;
				if (!canMove(is_in_check, final_square, player.bitboards.squares_to_uncheck)) continue;

				this->addMove(knight_move | move);
			}
		}
		
		cp_knights_bitboard >>= (squares_to_skip + 1);
		knight_location++;
	}

	// Bishops moves
	unsigned long long cp_bishops_bitboard = player.bitboards.bishops;
	location bishop_location = 0;
	while (cp_bishops_bitboard != 0 && bishop_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_bishops_bitboard);
		bishop_location += squares_to_skip;
		bool is_pinned = player.isPinned(bishop_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[bishop_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[bishop_location].squares_to_unpin;

		addMovesFromAttacksBitboard(bishop_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, bishop_move, this);
		
		cp_bishops_bitboard >>= (squares_to_skip + 1);
		bishop_location++;
	}

	// Rook moves
	unsigned long long cp_rooks_bitboard = player.bitboards.rooks;
	location rook_location = 0;
	while (cp_rooks_bitboard != 0 && rook_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_rooks_bitboard);
		rook_location += squares_to_skip;
		bool is_pinned = player.isPinned(rook_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[rook_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[rook_location].squares_to_unpin;

		addMovesFromAttacksBitboard(rook_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, rook_move, this);
		
		cp_rooks_bitboard >>= (squares_to_skip + 1);
		rook_location++;
	}

	// Queen moves
	unsigned long long cp_queens_bitboard = player.bitboards.queens;
	location queen_location = 0;
	while (cp_queens_bitboard != 0 && queen_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_queens_bitboard);
		queen_location += squares_to_skip;
		bool is_pinned = player.isPinned(queen_location);

		unsigned long long bitboard_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[queen_location], player.bitboards.all_pieces);
		bitboard_attacks |= slidingMoves(magic_bitboards.rooks_magic_bitboards[queen_location], player.bitboards.all_pieces);
		bitboard_attacks &= (~player.bitboards.friendly_pieces);
		if (is_pinned) bitboard_attacks &= player.pins[queen_location].squares_to_unpin;

		addMovesFromAttacksBitboard(queen_location, is_in_check, player.bitboards.squares_to_uncheck, bitboard_attacks, queen_move, this);
		
		cp_queens_bitboard >>= (squares_to_skip + 1);
		queen_location++;
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

void Moves::generateCaptures(Player& player, const Player& opponent) {
	player.bitboards.attacks = 0;
	unsigned long long opponent_pieces = opponent.bitboards.friendly_pieces;
	if (player.bitboards.king & opponent.bitboards.attacks) opponent_pieces &= player.bitboards.squares_to_uncheck;

	// Pawn moves
	short capture_right = player.is_white ? 9 : -9;
	short capture_left = player.is_white ? 7 : -7;
	short right_edge = player.is_white ? 7 : 0;
	short left_edge = player.is_white ? 0 : 7;

	unsigned long long cp_pawns_bitboard = player.bitboards.pawns;
	location pawn_location = 0;
	while (cp_pawns_bitboard != 0 && pawn_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_pawns_bitboard);
		pawn_location += squares_to_skip;

		short file = pawn_location % 8;
		unsigned long long pawn_attacks = 0;

		// If pawn can capture to at its right
		if (file != right_edge) {
			pawn_attacks |= (1LL << (pawn_location + capture_right));
		}

		// If pawn can capture to at its left
		if (file != left_edge) {
			pawn_attacks |= (1LL << (pawn_location + capture_left));
		}

		// Remove attacks that would leave the king in check if piece is pinned
		if (player.isPinned(pawn_location)) pawn_attacks &= player.pins[pawn_location].squares_to_unpin; 

		player.bitboards.attacks |= pawn_attacks;
		pawn_attacks &= opponent_pieces;

		if (pawn_attacks)
			addMovesFromAttacksBitboard(pawn_location, false, 0, pawn_attacks, pawn_move, this);

		cp_pawns_bitboard >>= (squares_to_skip + 1);
		pawn_location++;
	}

	// Knight moves
	unsigned long long cp_knights_bitboard = player.bitboards.knights;
	location knight_location = 0;
	while (cp_knights_bitboard != 0 && knight_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_knights_bitboard);
		knight_location += squares_to_skip;

		if (!player.isPinned(knight_location)) {
			unsigned long long knight_attacks = magic_bitboards.knights_attacks_array[knight_location];
			player.bitboards.attacks |= knight_attacks;
			knight_attacks &= opponent_pieces;

			if (knight_attacks)
				addMovesFromAttacksBitboard(knight_location, false, 0, knight_attacks, knight_move, this);
		}

		cp_knights_bitboard >>= (squares_to_skip + 1);
		knight_location++;
	}

	// Bishop moves
	unsigned long long cp_bishops_bitboard = player.bitboards.bishops;
	location bishop_location = 0;
	while (cp_bishops_bitboard != 0 && bishop_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_bishops_bitboard);
		bishop_location += squares_to_skip;
		
		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[bishop_location], player.bitboards.all_pieces);

		// Remove attacks that would leave the king in check if piece is pinned
		if (player.isPinned(bishop_location)) bishop_attacks &= player.pins[bishop_location].squares_to_unpin;

		player.bitboards.attacks |= bishop_attacks;
		bishop_attacks &= opponent_pieces;

		if (bishop_attacks)
			addMovesFromAttacksBitboard(bishop_location, false, 0, bishop_attacks, bishop_move, this);

		cp_bishops_bitboard >>= (squares_to_skip + 1);
		bishop_location++;
	}

	// Rook moves
	unsigned long long cp_rooks_bitboard = player.bitboards.rooks;
	location rook_location = 0;
	while (cp_rooks_bitboard != 0 && rook_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_rooks_bitboard);
		rook_location += squares_to_skip;
		
		unsigned long long rook_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[rook_location], player.bitboards.all_pieces);

		// Remove attacks that would leave the king in check if piece is pinned
		if (player.isPinned(rook_location)) rook_attacks &= player.pins[rook_location].squares_to_unpin;

		player.bitboards.attacks |= rook_attacks;
		rook_attacks &= opponent_pieces;

		if (rook_attacks)
			addMovesFromAttacksBitboard(rook_location, false, 0, rook_attacks, rook_move, this);

		cp_rooks_bitboard >>= (squares_to_skip + 1);
		rook_location++;
	}

	// Quuen moves
	unsigned long long cp_queens_bitboard = player.bitboards.queens;
	location queen_location = 0;
	while (cp_queens_bitboard != 0 && queen_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_queens_bitboard);
		queen_location += squares_to_skip;
		
		// Bishop attacks
		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[queen_location], player.bitboards.all_pieces);

		if (player.isPinned(queen_location)) // Remove attacks that would leave the king in check if piece is pinned
			bishop_attacks &= player.pins[queen_location].squares_to_unpin;

		player.bitboards.attacks |= bishop_attacks;
		bishop_attacks &= opponent_pieces;

		if (bishop_attacks)
			addMovesFromAttacksBitboard(queen_location, false, 0, bishop_attacks, queen_move, this);

		// Rook attacks
		unsigned long long rook_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[queen_location], player.bitboards.all_pieces);

		if (player.isPinned(queen_location)) // Remove attacks that would leave the king in check if piece is pinned
			rook_attacks &= player.pins[queen_location].squares_to_unpin;

		player.bitboards.attacks |= rook_attacks;
		rook_attacks &= opponent_pieces;

		if (rook_attacks)
			addMovesFromAttacksBitboard(queen_location, false, 0, rook_attacks, queen_move, this);

		cp_queens_bitboard >>= (squares_to_skip + 1);
		queen_location++;
	}

	// King moves
	unsigned long long king_attacks = magic_bitboards.king_attacks_array[player.locations.king];
	king_attacks &= ~opponent.bitboards.attacks; // Remove defedended squares
	player.bitboards.attacks |= king_attacks;
	king_attacks &= opponent_pieces; 

	if (king_attacks)
		addMovesFromAttacksBitboard(player.locations.king, false, 0, king_attacks, king_move, this);
}

bool Moves::isMoveLegal (unsigned short move) const {
	for (const unsigned short m : *this) {
		if (m == move) return true;
	}

	return false;
}

void Moves::orderMoves(const Player& player, const Player& opponent, const Entry* tt_entry, const std::array<unsigned short, 2>* killer_moves_at_ply) {
	/*
		Order of moves:
			1. Move from TT if any
			2. Winning captures (ordered by MVV-LVA)
			3. Equal captures
			4. Killer moves if any
			5. Non-captures
			6. Losing captures
	*/
	unsigned short best_cached_move = tt_entry ? tt_entry->best_move : 0;

	for (int i = 0; i < num_moves; i++) {
		scores[i] += i; // Ensure that two moves dont have the same scores

		// Make best cached move first and assign maximum score
		if (moves[i] == best_cached_move) {
			moves[i] = moves[0];
			scores[i] = scores[0];
			
			moves[0] = best_cached_move;
			scores[0] = SHRT_MAX;
			continue;
		}

		// Score of killer moves
		if (killer_moves_at_ply && (moves[i] == (*killer_moves_at_ply)[0] || moves[i] == (*killer_moves_at_ply)[1])) {
			scores[i] += 500;
			continue;
		}

		location start_square = getStartSquare(moves[i]);
		location final_square = getFinalSquare(moves[i]);
		unsigned long long final_square_bitboard = 1LL << final_square;
		unsigned short move_flag = getMoveFlag(moves[i]);

		// The switch is a little bit faster than getPieceValue, increases score for promotions
		int piece_value = 0;
		PieceType piece_type;
		switch (move_flag) {
		case pawn_move:
			piece_value = 1000;
			piece_type = Pawn;
			break;
		case pawn_move_two_squares:
			piece_value = 1000;
			piece_type = Pawn;
			break;
		case knight_move:
			piece_value = 3200;
			piece_type = Knight;
			break;
		case bishop_move:
			piece_value = 3300;
			piece_type = Bishop;
			break;
		case rook_move:
			piece_value = 5000;
			piece_type = Rook;
			break;
		case queen_move:
			piece_value = 9000;
			piece_type = Queen;
			break;
		case king_move:
			piece_value = 0; // If king caputes then it is a free piece (only considering one move)
			piece_type = King;
			break;
		case castle_king_side:
			piece_type = King;
			break;
		case castle_queen_side:
			piece_type = King;
			break;
		case en_passant:
			piece_value = 1000;
			piece_type = Pawn;
			break;
		case promotion_knight:
			piece_value = 3200;
			piece_type = Pawn;
			scores[i] += 2200;
			break;
		case promotion_bishop:
			piece_value = 3300;
			piece_type = Pawn;
			scores[i] += 2300;
			break;
		case promotion_rook:
			piece_value = 5000;
			piece_type = Pawn;
			scores[i] += 4000;
			break;
		case promotion_queen:
			piece_value = 9000;
			piece_type = Pawn;
			scores[i] += 8000;
			break;
		default:
			break;
		}

		// Captures
		if (final_square_bitboard & opponent.bitboards.friendly_pieces) {
			scores[i] += 700; // So that equal captures are searched before non-captures 
			int victim_value = getPieceValue(opponent, final_square);
			scores[i] += victim_value;
		}
		else { // Sort non captures with history heuristic
			int history_value = history_table.get(player.is_white, piece_type, final_square) * 400; // History values from 0 to 400
			scores[i] += history_value;
		}

		// Moving to a defended square
		if (final_square_bitboard & opponent.bitboards.attacks) 
			scores[i] -= piece_value;
	}
}

unsigned short Moves::getNextOrderedMove() {
	short next_max = SHRT_MIN;
	int next_max_pos = 0;
	for (int i = 0; i < num_moves; i++) {
		if (scores[i] > next_max && scores[i] < last_score_picked) {
			next_max = scores[i];
			next_max_pos = i;
		}
		if (next_max == SHRT_MAX) break;
	}

	// Return 0 if there are no more moves to be searched in the list
	if (next_max == SHRT_MIN) return 0;

	last_score_picked = next_max;
	return moves[next_max_pos];
}

AttacksInfo generateAttacksInfo(bool is_white, const BitBoards& bitboards, unsigned long long all_pieces,
							    location player_king_location, location opponent_king_location) {
	unsigned long long attacks_bitboard = 0;
	unsigned long long opponent_squares_to_uncheck = 0;
	unsigned long long opponent_king = (opponent_king_location < 64) ? (1LL << opponent_king_location) : 0;

	// Pawn moves
	short capture_right = is_white ? 9 : -9;
	short capture_left = is_white ? 7 : -7;
	short right_edge = is_white ? 7 : 0;
	short left_edge = is_white ? 0 : 7;

	unsigned long long cp_pawns_bitboard = bitboards.pawns;
	location pawn_location = 0;
	while (cp_pawns_bitboard != 0 && pawn_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_pawns_bitboard);
		pawn_location += squares_to_skip;
		short file = pawn_location % 8;
		unsigned long long pawn_attacks = 0;

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

		cp_pawns_bitboard >>= (squares_to_skip + 1);
		pawn_location++;
	}

	// Knight moves
	unsigned long long cp_knights_bitboard = bitboards.knights;
	location knight_location = 0;
	while (cp_knights_bitboard != 0 && knight_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_knights_bitboard);
		knight_location += squares_to_skip;

		unsigned long long knight_attacks = magic_bitboards.knights_attacks_array[knight_location];
		attacks_bitboard |= knight_attacks;

		if (knight_attacks & opponent_king) {
			opponent_squares_to_uncheck = (1LL << knight_location);
		}

		cp_knights_bitboard >>= (squares_to_skip + 1);
		knight_location++;
	}

	// Bishop moves
	unsigned long long cp_bishops_bitboard = bitboards.bishops;
	location bishop_location = 0;
	while (cp_bishops_bitboard != 0 && bishop_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_bishops_bitboard);
		bishop_location += squares_to_skip;

		// Remove opponent king from all pieces to ensure that in the next move the king gets out of check
		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[bishop_location], all_pieces ^ opponent_king);
		attacks_bitboard |= bishop_attacks;

		if (bishop_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckBishop(opponent_king_location, bishop_location);
			}
		}

		cp_bishops_bitboard >>= (squares_to_skip + 1);
		bishop_location++;
	}

	// Rook moves
	unsigned long long cp_rooks_bitboard = bitboards.rooks;
	location rook_location = 0;
	while (cp_rooks_bitboard != 0 && rook_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_rooks_bitboard);
		rook_location += squares_to_skip;

		// Remove opponent king from all pieces to ensure that in the next move the king gets out of check
		unsigned long long rook_attacks = slidingMoves(magic_bitboards.rooks_magic_bitboards[rook_location], all_pieces ^ opponent_king);
		attacks_bitboard |= rook_attacks;

		if (rook_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckRook(opponent_king_location, rook_location);
			}
		}

		cp_rooks_bitboard >>= (squares_to_skip + 1);
		rook_location++;
	}

	// Quuen moves
	unsigned long long cp_queens_bitboard = bitboards.queens;
	location queen_location = 0;
	while (cp_queens_bitboard != 0 && queen_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_queens_bitboard);
		queen_location += squares_to_skip;

		// Bishop attacks
		unsigned long long bishop_attacks = slidingMoves(magic_bitboards.bishops_magic_bitboards[queen_location], all_pieces ^ opponent_king);
		attacks_bitboard |= bishop_attacks;

		if (bishop_attacks & opponent_king) {

			// Handle double checks, where only legal move is to move the king, so opponent_squares_to_uncheck should be 0
			if (opponent_squares_to_uncheck) {
				opponent_squares_to_uncheck = 0;
			}
			else {
				opponent_squares_to_uncheck = squaresToUncheckBishop(opponent_king_location, queen_location);
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
				opponent_squares_to_uncheck = squaresToUncheckRook(opponent_king_location, queen_location);
			}
		}

		cp_queens_bitboard >>= (squares_to_skip + 1);
		queen_location++;
	}

	// King moves
	attacks_bitboard |= magic_bitboards.king_attacks_array[player_king_location];

	return { attacks_bitboard, opponent_squares_to_uncheck };
}

inline unsigned long long slidingMoves(const MagicBitboard& magic_bitboard, unsigned long long pieces) {
	int index = ((pieces | magic_bitboard.mask) * magic_bitboard.magic_number) >> magic_bitboard.num_shifts;
	return magic_bitboard.ptr_attacks_array[index];
}

inline void addMovesFromAttacksBitboard(location start_square, bool is_in_check, unsigned long long squares_to_uncheck, 
										unsigned long long bitboard_attacks, unsigned short move_flag, Moves* moves) {
	int final_square = 0;
	while (bitboard_attacks != 0 && final_square <= 63) {
		int squares_to_skip = std::countr_zero(bitboard_attacks);
		final_square += squares_to_skip;

		// Add move if not in check or move covers check
		if (canMove(is_in_check, final_square, squares_to_uncheck)) {
			moves->addMove(move_flag, start_square, final_square);
		}

		bitboard_attacks >>= (squares_to_skip + 1);
		final_square++;
	}
}

inline unsigned long long squaresToUncheckBishop(location opponent_king_location, location bishop_location) {
	return magic_bitboards.bishop_squares_uncheck[bishop_location][opponent_king_location];
}

inline unsigned long long squaresToUncheckRook(location opponent_king_location, location rook_location) {
	return magic_bitboards.rook_squares_uncheck[rook_location][opponent_king_location];
}

inline bool canMove(bool is_in_check, location final_square, unsigned long long squares_to_uncheck) {
	return (!is_in_check || ((1LL << final_square) & squares_to_uncheck));
}

inline int getPieceValue(const Player& player, location square) {
	unsigned long long bitboard = 1LL << square;
	if (player.bitboards.pawns & bitboard) return 1000;
	else if ((player.bitboards.knights & bitboard) || (player.bitboards.bishops & bitboard)) return 3000;
	else if (player.bitboards.rooks & bitboard) return 5000;
	return 9000; // Queen
}

void setPins(Player& player, Player& opponent) {
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

	// Pins by bishops
	unsigned long long cp_bishops_bitboard = opponent.bitboards.bishops;
	location bishop_location = 0;
	while (cp_bishops_bitboard != 0 && bishop_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_bishops_bitboard);
		bishop_location += squares_to_skip;

		setPin(player, opponent, bishop_location, true);

		cp_bishops_bitboard >>= (squares_to_skip + 1);
		bishop_location++;
	}

	// Pins by rooks
	unsigned long long cp_rooks_bitboard = opponent.bitboards.rooks;
	location rook_location = 0;
	while (cp_rooks_bitboard != 0 && rook_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_rooks_bitboard);
		rook_location += squares_to_skip;

		setPin(player, opponent, rook_location, false);

		cp_rooks_bitboard >>= (squares_to_skip + 1);
		rook_location++;
	}

	// Pins by queens
	unsigned long long cp_queens_bitboard = opponent.bitboards.queens;
	location queen_location = 0;
	while (cp_queens_bitboard != 0 && queen_location <= 63) {
		int squares_to_skip = std::countr_zero(cp_queens_bitboard);
		queen_location += squares_to_skip;

		setPin(player, opponent, queen_location, true);
		setPin(player, opponent, queen_location, false);

		cp_queens_bitboard >>= (squares_to_skip + 1);
		queen_location++;
	}
}

void setPin(Player& player, const Player& opponent, location piece_location, bool is_pin_diagonal) {
	unsigned long long squares_to_uncheck;
	if (is_pin_diagonal) squares_to_uncheck = squaresToUncheckBishop(player.locations.king, piece_location);
	else squares_to_uncheck = squaresToUncheckRook(player.locations.king, piece_location);
	if (!squares_to_uncheck) return;

	unsigned long long friendly_pieces_covering_check = squares_to_uncheck & player.bitboards.friendly_pieces;
	if (!friendly_pieces_covering_check) return;
	unsigned long long opponent_pieces_covering_check = (squares_to_uncheck & opponent.bitboards.friendly_pieces) ^ (1LL << piece_location);

	// Bit hack to see if only one bit is set in friendly_pieces_covering_check
	bool only_one_friendly_piece_covering_check = ((friendly_pieces_covering_check & (friendly_pieces_covering_check - 1)) == 0);

	if (only_one_friendly_piece_covering_check && !opponent_pieces_covering_check) {
		int index_pinned_piece = std::countr_zero(friendly_pieces_covering_check);

		// Add pin
		player.pins[index_pinned_piece].is_pin_diagonal = is_pin_diagonal;
		player.pins[index_pinned_piece].location_pinner = piece_location;
		player.pins[index_pinned_piece].squares_to_unpin = squares_to_uncheck;
		player.pins[index_pinned_piece].id_move_pinned = player.move_id;
	}
}

unsigned short Moves::parseMove(std::string& move_str) {
	unsigned short move = 0;

	if (move_str.length() < 4 || move_str.length() > 5) return move;

	std::transform(move_str.begin(), move_str.end(), move_str.begin(), (int (*)(int))std::tolower);

	unsigned short start_square = notationSquareToLocation(move_str.substr(0, 2));
	unsigned short final_square = notationSquareToLocation(move_str.substr(2, 2));

	// Moves that are not promotions
	if (move_str.length() == 4) {
		unsigned short tmp_move = (start_square << 6) | final_square;
		if (!isPromotion(tmp_move)) move = tmp_move;
	}

	// Promotions
	else if (move_str.length() == 5) {
		unsigned short promotion_flag = 0;

		switch (move_str[4]) {
		case 'n':
			promotion_flag = promotion_knight;
			break;
		case 'b':
			promotion_flag = promotion_bishop;
			break;
		case 'r':
			promotion_flag = promotion_rook;
			break;
		case 'q':
			promotion_flag = promotion_queen;
			break;
		default:
			break;
		}

		move = promotion_flag | (start_square << 6) | final_square;
	}

	// Get the move flag if it is valid
	for (const unsigned short m : *this) {
		if ((m & 0xfff) == move) return m;
	}

	return 0;
}

PieceType getPieceType(unsigned short move) {
	unsigned short flag = getMoveFlag(move);

	switch (flag) {
		case pawn_move: return Pawn;
		case pawn_move_two_squares: return Pawn;
		case knight_move: return Knight;
		case bishop_move: return Bishop;
		case rook_move: return Rook;
		case queen_move: return Queen;
		case king_move: return King;
		case castle_king_side: return King;
		case castle_queen_side: return King;
		case en_passant: return Pawn;
		case promotion_knight: return Pawn;
		case promotion_bishop: return Pawn;
		case promotion_rook: return Pawn;
		case promotion_queen: return Pawn;
		default: return InvalidPiece;
	}
}

bool isPseudoLegal(unsigned short move, const Player& player) {
	PieceType piece_type = getPieceType(move);
	location start_square = getStartSquare(move);
	location final_square = getFinalSquare(move);

	// Check if there isn't a player's piece in the final square
	if ((1LL << final_square) & player.bitboards.friendly_pieces) return false;

	unsigned long long piece_bitboard = 0;
	switch (piece_type) {
	case Pawn:
		piece_bitboard = player.bitboards.pawns;
		break;
	case Knight:
		piece_bitboard = player.bitboards.knights;
		break;
	case Bishop:
		piece_bitboard = player.bitboards.bishops;
		break;
	case Rook:
		piece_bitboard = player.bitboards.rooks;
		break;
	case Queen:
		piece_bitboard = player.bitboards.queens;
		break;
	case King:
		piece_bitboard = player.bitboards.king;
		break;
	default: 
		return false; // move is not valid so it is not legal
	}

	// Check if there's a player piece of the correct type in the start square
	if (!((1LL << start_square) & piece_bitboard)) return false;

	return true;
}
