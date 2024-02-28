#include "MakeMoves.h"
#include "MagicBitboards.h"
#include "Moves.h"

constexpr unsigned short flag_mask = 0b1111 << 12;
constexpr unsigned short location_mask = 0b111111;

MoveInfo makeMove(const unsigned short move, Player& player, Player& opponent, const MagicBitboards& magic_bitboards) {
	unsigned short flag = move & flag_mask;
	location start_square = (move >> 6) & location_mask;
	location final_square = move & location_mask;
	location initial_square_rook_king_side = player.is_white ? 7 : 63;
	location initial_square_rook_queen_side = player.is_white ? 0 : 56;
	short capture_type = no_capture;

	// Update bitboards and locations
	location location_en_passant_pawn;
	switch (flag) {
	case pawn_move:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);
		player.locations.movePawn(start_square, final_square);
		break;

	case pawn_move_two_squares:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);
		player.locations.movePawn(start_square, final_square);
		player.locations.en_passant_target = player.is_white ? (final_square - 8) : (final_square + 8);
		break;

	case knight_move:
		player.bitboards.removeKnight(start_square);
		player.bitboards.addKnight(final_square);
		player.locations.moveKnight(start_square, final_square);
		break;

	case bishop_move:
		player.bitboards.removeBishop(start_square);
		player.bitboards.addBishop(final_square);
		player.locations.moveBishop(start_square, final_square);
		break;

	case rook_move:
		player.bitboards.removeRook(start_square);
		player.bitboards.addRook(final_square);
		player.locations.moveRook(start_square, final_square);

		// Remove castle rights if moved the rook for the first time
		if (player.can_castle_king_side && start_square == initial_square_rook_king_side) {
			player.can_castle_king_side = false;
		}
		else if (player.can_castle_queen_side && start_square == initial_square_rook_queen_side) {
			player.can_castle_queen_side = false;
		}

		break;

	case queen_move:
		player.bitboards.removeQueen(start_square);
		player.bitboards.addQueen(final_square);
		player.locations.moveQueen(start_square, final_square);
		break;

	case king_move:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);
		player.can_castle_king_side = false;
		player.can_castle_queen_side = false;
		break;

	case castle_king_side:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);
		player.can_castle_king_side = false;
		player.can_castle_queen_side = false;

		// Move king side rook
		player.bitboards.removeRook(initial_square_rook_king_side);
		player.bitboards.addRook(final_square - 1);
		player.locations.moveRook(initial_square_rook_king_side, final_square - 1);

		break;

	case castle_queen_side:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);
		player.can_castle_king_side = false;
		player.can_castle_queen_side = false;

		// Move queen side rook
		player.bitboards.removeRook(initial_square_rook_queen_side);
		player.bitboards.addRook(final_square + 1);
		player.locations.moveRook(initial_square_rook_queen_side, final_square + 1);

		break;

	case en_passant:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);
		player.locations.movePawn(start_square, final_square);

		// Remove pawn target of en passant
		location_en_passant_pawn = player.is_white ? (final_square - 8) : (final_square + 8);
		player.bitboards.all_pieces ^= (1LL << location_en_passant_pawn);
		opponent.bitboards.removePawn(location_en_passant_pawn);
		opponent.locations.removePawn(location_en_passant_pawn);
		opponent.num_pawns--;

		break;

	case promotion_knight:
		player.bitboards.removePawn(start_square);
		player.bitboards.addKnight(final_square);
		player.locations.removePawn(start_square);
		player.locations.addKnight(final_square);
		player.num_pawns--;
		player.num_knights++;
		break;

	case promotion_bishop:
		player.bitboards.removePawn(start_square);
		player.bitboards.addBishop(final_square);
		player.locations.removePawn(start_square);
		player.locations.addBishop(final_square);
		player.num_pawns--;
		player.num_bishops++;
		break;

	case promotion_rook:
		player.bitboards.removePawn(start_square);
		player.bitboards.addRook(final_square);
		player.locations.removePawn(start_square);
		player.locations.addRook(final_square);
		player.num_pawns--;
		player.num_rooks++;
		break;

	case promotion_queen:
		player.bitboards.removePawn(start_square);
		player.bitboards.addQueen(final_square);
		player.locations.removePawn(start_square);
		player.locations.addQueen(final_square);
		player.num_pawns--;
		player.num_queens++;
		break;

	default:
		break;
	}

	// Captures
	unsigned long long intersection = player.bitboards.friendly_pieces & opponent.bitboards.friendly_pieces;
	if (intersection) {
		opponent.bitboards.friendly_pieces ^= intersection;

		// See what piece was captured and update locations and piece count
		if (opponent.bitboards.pawns & intersection) {
			opponent.locations.removePawn(final_square);
			opponent.bitboards.pawns ^= intersection;
			opponent.num_pawns--;
			capture_type = pawn_capture;
		}
		else if (opponent.bitboards.knights & intersection) {
			opponent.locations.removeKnight(final_square);
			opponent.bitboards.knights ^= intersection;
			opponent.num_knights--;
			capture_type = knight_capture;
		}
		else if (opponent.bitboards.bishops & intersection) {
			opponent.locations.removeBishop(final_square);
			opponent.bitboards.bishops ^= intersection;
			opponent.num_bishops--;
			capture_type = bishop_capture;
		}
		else if (opponent.bitboards.rooks & intersection) {
			opponent.locations.removeRook(final_square);
			opponent.bitboards.rooks ^= intersection;
			opponent.num_rooks--;
			capture_type = rook_capture;

			// Remove rights to castle
			location initial_square_rook_king_side_opponent = player.is_white ? 63 : 7;
			location initial_square_rook_queen_side_opponent = player.is_white ? 56 : 0;
			if (final_square == initial_square_rook_king_side_opponent) opponent.can_castle_king_side = false;
			else if (final_square == initial_square_rook_queen_side_opponent) opponent.can_castle_queen_side = false;
		}
		else {
			opponent.locations.removeQueen(final_square);
			opponent.bitboards.queens ^= intersection;
			opponent.num_queens--;
			capture_type = queen_capture;
		}
	}

	opponent.bitboards.all_pieces = player.bitboards.all_pieces;

	// Update attacks and squares to uncheck bitboards
	/*opponent.bitboards.attacks = generateAttacksBitBoard(opponent.is_white, opponent.locations, opponent.bitboards.all_pieces,
		opponent.num_pawns, opponent.num_knights, opponent.num_bishops,
		opponent.num_rooks, opponent.num_queens, magic_bitboards);*/

	AttacksInfo player_attacks = generateAttacksInfo(player.is_white, player.locations, player.bitboards.all_pieces, player.num_pawns,
		player.num_knights, player.num_bishops, player.num_rooks, player.num_queens,
		opponent.locations.king, magic_bitboards);

	player.bitboards.attacks = player_attacks.attacks_bitboard;
	opponent.bitboards.squares_to_uncheck = player_attacks.opponent_squares_to_uncheck;

	// Reset en passant target
	opponent.locations.en_passant_target = 0;

	// Set opponent's pins, since opponent is next to play
	player.move_id++;
	opponent.move_id++;
	setPins(opponent, player, magic_bitboards);

	return { capture_type };
}

void unmakeMove(const unsigned short move, Player& player, Player& opponent, location opponent_en_passant_target,
	bool player_can_castle_king_side, bool player_can_castle_queen_side, bool opponent_can_castle_king_side,
	bool opponent_can_castle_queen_side, const MoveInfo& move_info, const MagicBitboards& magic_bitboards) {
	unsigned short flag = move & flag_mask;
	location start_square = (move >> 6) & location_mask;
	location final_square = move & location_mask;
	location initial_square_rook_king_side = player.is_white ? 7 : 63;
	location initial_square_rook_queen_side = player.is_white ? 0 : 56;
	short capture_type = move_info.capture_flag;

	opponent.locations.en_passant_target = opponent_en_passant_target;
	player.can_castle_king_side = player_can_castle_king_side;
	player.can_castle_queen_side = player_can_castle_queen_side;
	opponent.can_castle_king_side = opponent_can_castle_king_side;
	opponent.can_castle_queen_side = opponent_can_castle_queen_side;

	// Add start square to bitboards and update locations
	location location_en_passant_pawn;
	switch (flag) {
	case pawn_move:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.movePawn(final_square, start_square);
		break;

	case pawn_move_two_squares:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.movePawn(final_square, start_square);
		player.locations.en_passant_target = 0;
		break;

	case knight_move:
		player.bitboards.removeKnight(final_square);
		player.bitboards.addKnight(start_square);
		player.locations.moveKnight(final_square, start_square);
		break;

	case bishop_move:
		player.bitboards.removeBishop(final_square);
		player.bitboards.addBishop(start_square);
		player.locations.moveBishop(final_square, start_square);
		break;

	case rook_move:
		player.bitboards.removeRook(final_square);
		player.bitboards.addRook(start_square);
		player.locations.moveRook(final_square, start_square);

		break;

	case queen_move:
		player.bitboards.removeQueen(final_square);
		player.bitboards.addQueen(start_square);
		player.locations.moveQueen(final_square, start_square);
		break;

	case king_move:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);
		break;

	case castle_king_side:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);

		// Move king side rook
		player.bitboards.removeRook(final_square - 1);
		player.bitboards.addRook(initial_square_rook_king_side);
		player.locations.moveRook(final_square - 1, initial_square_rook_king_side);

		break;

	case castle_queen_side:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);

		// Move queen side rook
		player.bitboards.removeRook(final_square + 1);
		player.bitboards.addRook(initial_square_rook_queen_side);
		player.locations.moveRook(final_square + 1, initial_square_rook_queen_side);

		break;

	case en_passant:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.movePawn(final_square, start_square);

		// Add back pawn target of en passant
		location_en_passant_pawn = player.is_white ? (final_square - 8) : (final_square + 8);
		player.bitboards.all_pieces |= (1LL << location_en_passant_pawn);
		opponent.bitboards.addPawn(location_en_passant_pawn);
		opponent.locations.addPawn(location_en_passant_pawn);
		opponent.num_pawns++;

		break;

	case promotion_knight:
		player.bitboards.removeKnight(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.removeKnight(final_square);
		player.locations.addPawn(start_square);
		player.num_pawns++;
		player.num_knights--;
		break;

	case promotion_bishop:
		player.bitboards.removeBishop(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.removeBishop(final_square);
		player.locations.addPawn(start_square);
		player.num_pawns++;
		player.num_bishops--;
		break;

	case promotion_rook:
		player.bitboards.removeRook(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.removeRook(final_square);
		player.locations.addPawn(start_square);
		player.num_pawns++;
		player.num_rooks--;
		break;

	case promotion_queen:
		player.bitboards.removeQueen(final_square);
		player.bitboards.addPawn(start_square);
		player.locations.removeQueen(final_square);
		player.locations.addPawn(start_square);
		player.num_pawns++;
		player.num_queens--;
		break;

	default:
		break;
	}

	// Captures
	// Add back piece that was captured and update locations and piece count
	if (capture_type != no_capture) player.bitboards.all_pieces |= (1LL << final_square);

	switch (capture_type)
	{
	case pawn_capture:
		opponent.bitboards.addPawn(final_square);
		opponent.locations.addPawn(final_square);
		opponent.num_pawns++;
		break;

	case knight_capture:
		opponent.bitboards.addKnight(final_square);
		opponent.locations.addKnight(final_square);
		opponent.num_knights++;
		break;

	case bishop_capture:
		opponent.bitboards.addBishop(final_square);
		opponent.locations.addBishop(final_square);
		opponent.num_bishops++;
		break;

	case rook_capture:
		opponent.bitboards.addRook(final_square);
		opponent.locations.addRook(final_square);
		opponent.num_rooks++;
		break;

	case queen_capture:
		opponent.bitboards.addQueen(final_square);
		opponent.locations.addQueen(final_square);
		opponent.num_queens++;
		break;

	default:
		break;
	}

	opponent.bitboards.all_pieces = player.bitboards.all_pieces;

	// Reset player's pins, since player is next to play
	player.move_id++;
	opponent.move_id++;
	setPins(player, opponent, magic_bitboards);
}
