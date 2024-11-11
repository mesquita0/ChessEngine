#include "MakeMoves.h"
#include "MagicBitboards.h"
#include "Moves.h"
#include "PieceTypes.h"
#include "Zobrist.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"

MoveInfo makeMove(const unsigned short move, Player& player, Player& opponent, unsigned long long hash) {
	unsigned short flag = getMoveFlag(move);
	location start_square = getStartSquare(move);
	location final_square = getFinalSquare(move);
	location initial_square_rook_king_side = player.is_white ? 7 : 63;
	location initial_square_rook_queen_side = player.is_white ? 0 : 56;
	short capture_type = no_capture;

	// Save previous state
	location opponent_en_passant_target = opponent.locations.en_passant_target;
	bool player_could_castle_king_side = player.can_castle_king_side;
	bool player_could_castle_queen_side = player.can_castle_queen_side;
	bool opponent_could_castle_king_side = opponent.can_castle_king_side;
	bool opponent_could_castle_queen_side = opponent.can_castle_queen_side;

	// Reset en passant target
	if (opponent.locations.en_passant_target != 0) {
		hash ^= zobrist_keys.en_passant_file[opponent.locations.en_passant_target % 8];
		opponent.locations.en_passant_target = 0;
	}

	// Update bitboards, hash and NNUE
	location location_en_passant_pawn;
	switch (flag) {
	case pawn_move:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);

		nnue.movePiece(Pawn, start_square, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_pawn[final_square];
		} 
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_pawn[final_square];
		}

		break;

	case pawn_move_two_squares:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);

		nnue.movePiece(Pawn, start_square, final_square, player, opponent);

		player.locations.en_passant_target = player.is_white ? (final_square - 8) : (final_square + 8);

		hash ^= zobrist_keys.en_passant_file[final_square % 8];
		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_pawn[final_square];
		}
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_pawn[final_square];
		}

		break;

	case knight_move:
		player.bitboards.removeKnight(start_square);
		player.bitboards.addKnight(final_square);

		nnue.movePiece(Knight, start_square, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_knight[start_square];
			hash ^= zobrist_keys.white_knight[final_square];
		}							   
		else {						   
			hash ^= zobrist_keys.black_knight[start_square];
			hash ^= zobrist_keys.black_knight[final_square];
		}

		break;

	case bishop_move:
		player.bitboards.removeBishop(start_square);
		player.bitboards.addBishop(final_square);

		nnue.movePiece(Bishop, start_square, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_bishop[start_square];
			hash ^= zobrist_keys.white_bishop[final_square];
		}							   
		else {						   
			hash ^= zobrist_keys.black_bishop[start_square];
			hash ^= zobrist_keys.black_bishop[final_square];
		}

		break;

	case rook_move:
		player.bitboards.removeRook(start_square);
		player.bitboards.addRook(final_square);

		nnue.movePiece(Rook, start_square, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_rook[start_square];
			hash ^= zobrist_keys.white_rook[final_square];
		}							   
		else {						   
			hash ^= zobrist_keys.black_rook[start_square];
			hash ^= zobrist_keys.black_rook[final_square];
		}

		// Remove castle rights if moved the rook for the first time
		if (player.can_castle_king_side && start_square == initial_square_rook_king_side) {
			player.can_castle_king_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_king_side;
			else hash ^= zobrist_keys.black_castle_king_side;
		}
		else if (player.can_castle_queen_side && start_square == initial_square_rook_queen_side) {
			player.can_castle_queen_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_queen_side;
			else hash ^= zobrist_keys.black_castle_queen_side;
		}

		break;

	case queen_move:
		player.bitboards.removeQueen(start_square);
		player.bitboards.addQueen(final_square);

		nnue.movePiece(Queen, start_square, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_queen[start_square];
			hash ^= zobrist_keys.white_queen[final_square];
		}							   
		else {						   
			hash ^= zobrist_keys.black_queen[start_square];
			hash ^= zobrist_keys.black_queen[final_square];
		}

		break;

	case king_move:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);

		nnue.setPosition(player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_king[start_square];
			hash ^= zobrist_keys.white_king[final_square];
		}							   
		else {						   
			hash ^= zobrist_keys.black_king[start_square];
			hash ^= zobrist_keys.black_king[final_square];
		}

		// Update castling rights
		if (player.can_castle_king_side) {
			player.can_castle_king_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_king_side;
			else hash ^= zobrist_keys.black_castle_king_side;
		}
		if (player.can_castle_queen_side) {
 			player.can_castle_queen_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_queen_side;
			else hash ^= zobrist_keys.black_castle_queen_side;
		}

		break;

	case castle_king_side:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);

		// Update castling rights
		player.can_castle_king_side = false;
		if (player.is_white) hash ^= zobrist_keys.white_castle_king_side;
		else hash ^= zobrist_keys.black_castle_king_side;

		if (player.can_castle_queen_side) {
			player.can_castle_queen_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_queen_side;
			else hash ^= zobrist_keys.black_castle_queen_side;
		}

		// Move king side rook
		player.bitboards.removeRook(initial_square_rook_king_side);
		player.bitboards.addRook(final_square - 1);

		nnue.setPosition(player, opponent);

		// Update hash
		if (player.is_white) {
			hash ^= zobrist_keys.white_king[start_square];
			hash ^= zobrist_keys.white_king[final_square];
			hash ^= zobrist_keys.white_rook[initial_square_rook_king_side];
			hash ^= zobrist_keys.white_rook[final_square - 1];
		}							   
		else {						   
			hash ^= zobrist_keys.black_king[start_square];
			hash ^= zobrist_keys.black_king[final_square];
			hash ^= zobrist_keys.black_rook[initial_square_rook_king_side];
			hash ^= zobrist_keys.black_rook[final_square - 1];
		}

		break;

	case castle_queen_side:
		player.bitboards.removeKing(start_square);
		player.bitboards.addKing(final_square);
		player.locations.moveKing(final_square);

		// Update castling rights
		player.can_castle_queen_side = false;
		if (player.is_white) hash ^= zobrist_keys.white_castle_queen_side;
		else hash ^= zobrist_keys.black_castle_queen_side;

		if (player.can_castle_king_side) {
			player.can_castle_king_side = false;
			if (player.is_white) hash ^= zobrist_keys.white_castle_king_side;
			else hash ^= zobrist_keys.black_castle_king_side;
		}

		// Move queen side rook
		player.bitboards.removeRook(initial_square_rook_queen_side);
		player.bitboards.addRook(final_square + 1);

		nnue.setPosition(player, opponent);

		// Update hash
		if (player.is_white) {
			hash ^= zobrist_keys.white_king[start_square];
			hash ^= zobrist_keys.white_king[final_square];
			hash ^= zobrist_keys.white_rook[initial_square_rook_queen_side];
			hash ^= zobrist_keys.white_rook[final_square + 1];
		}
		else {
			hash ^= zobrist_keys.black_king[start_square];
			hash ^= zobrist_keys.black_king[final_square];
			hash ^= zobrist_keys.black_rook[initial_square_rook_queen_side];
			hash ^= zobrist_keys.black_rook[final_square + 1];
		}

		break;

	case en_passant:
		player.bitboards.removePawn(start_square);
		player.bitboards.addPawn(final_square);

		// Remove pawn target of en passant
		location_en_passant_pawn = player.is_white ? (final_square - 8) : (final_square + 8);
		player.bitboards.all_pieces ^= (1LL << location_en_passant_pawn);
		opponent.bitboards.removePawn(location_en_passant_pawn);
		opponent.num_pawns--;

		nnue.movePiece(Pawn, start_square, final_square, player, opponent);
		nnue.removePiece(Pawn, location_en_passant_pawn, opponent, player);

		// Update hash
		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_pawn[final_square];
			hash ^= zobrist_keys.black_pawn[location_en_passant_pawn];
		}							   
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_pawn[final_square];
			hash ^= zobrist_keys.white_pawn[location_en_passant_pawn];
		}

		break;
		
	case promotion_knight:
		player.bitboards.removePawn(start_square);
		player.bitboards.addKnight(final_square);
		player.num_pawns--;
		player.num_knights++;

		nnue.removePiece(Pawn, start_square, player, opponent);
		nnue.addPiece(Knight, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_knight[final_square];
		}
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_knight[final_square];
		}

		break;

	case promotion_bishop:
		player.bitboards.removePawn(start_square);
		player.bitboards.addBishop(final_square);
		player.num_pawns--;
		player.num_bishops++;

		nnue.removePiece(Pawn, start_square, player, opponent);
		nnue.addPiece(Bishop, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_bishop[final_square];
		}
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_bishop[final_square];
		}

		break;

	case promotion_rook:
		player.bitboards.removePawn(start_square);
		player.bitboards.addRook(final_square);
		player.num_pawns--;
		player.num_rooks++;

		nnue.removePiece(Pawn, start_square, player, opponent);
		nnue.addPiece(Rook, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_rook[final_square];
		}
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_rook[final_square];
		}

		break;

	case promotion_queen:
		player.bitboards.removePawn(start_square);
		player.bitboards.addQueen(final_square);
		player.num_pawns--;
		player.num_queens++;

		nnue.removePiece(Pawn, start_square, player, opponent);
		nnue.addPiece(Queen, final_square, player, opponent);

		if (player.is_white) {
			hash ^= zobrist_keys.white_pawn[start_square];
			hash ^= zobrist_keys.white_queen[final_square];
		}
		else {
			hash ^= zobrist_keys.black_pawn[start_square];
			hash ^= zobrist_keys.black_queen[final_square];
		}

		break;

	default: // Null Move
		break;
	}

	// Captures
	unsigned long long intersection = player.bitboards.friendly_pieces & opponent.bitboards.friendly_pieces;
	if (intersection) {
		opponent.bitboards.friendly_pieces ^= intersection;

		// See what piece was captured and update piece count, hash and NNUE
		if (opponent.bitboards.pawns & intersection) {
			opponent.bitboards.pawns ^= intersection;
			opponent.num_pawns--;
			capture_type = pawn_capture;

			nnue.removePiece(Pawn, final_square, opponent, player);

			if (opponent.is_white) hash ^= zobrist_keys.white_pawn[final_square];
			else hash ^= zobrist_keys.black_pawn[final_square];
		}
		else if (opponent.bitboards.knights & intersection) {
			opponent.bitboards.knights ^= intersection;
			opponent.num_knights--;
			capture_type = knight_capture;

			nnue.removePiece(Knight, final_square, opponent, player);

			if (opponent.is_white) hash ^= zobrist_keys.white_knight[final_square];
			else hash ^= zobrist_keys.black_knight[final_square];
		}
		else if (opponent.bitboards.bishops & intersection) {
			opponent.bitboards.bishops ^= intersection;
			opponent.num_bishops--;
			capture_type = bishop_capture;

			nnue.removePiece(Bishop, final_square, opponent, player);

			if (opponent.is_white) hash ^= zobrist_keys.white_bishop[final_square];
			else hash ^= zobrist_keys.black_bishop[final_square];
		}
		else if (opponent.bitboards.rooks & intersection) {
			opponent.bitboards.rooks ^= intersection;
			opponent.num_rooks--;
			capture_type = rook_capture;

			nnue.removePiece(Rook, final_square, opponent, player);

			if (opponent.is_white) hash ^= zobrist_keys.white_rook[final_square];
			else hash ^= zobrist_keys.black_rook[final_square];

			// Remove rights to castle
			location initial_square_rook_king_side_opponent = player.is_white ? 63 : 7;
			location initial_square_rook_queen_side_opponent = player.is_white ? 56 : 0;
			if (opponent.can_castle_king_side && final_square == initial_square_rook_king_side_opponent) {
				opponent.can_castle_king_side = false;
				if (opponent.is_white) hash ^= zobrist_keys.white_castle_king_side;
				else hash ^= zobrist_keys.black_castle_king_side;
			}
			else if (opponent.can_castle_queen_side && final_square == initial_square_rook_queen_side_opponent) {
				opponent.can_castle_queen_side = false;
				if (opponent.is_white) hash ^= zobrist_keys.white_castle_queen_side;
				else hash ^= zobrist_keys.black_castle_queen_side;
			}
		}
		else {
			opponent.bitboards.queens ^= intersection;
			opponent.num_queens--;
			capture_type = queen_capture;

			nnue.removePiece(Queen, final_square, opponent, player);

			if (opponent.is_white) hash ^= zobrist_keys.white_queen[final_square];
			else hash ^= zobrist_keys.black_queen[final_square];
		}
	}

	opponent.bitboards.all_pieces = player.bitboards.all_pieces;

	// Update attacks and squares to uncheck bitboards
	AttacksInfo player_attacks = generateAttacksInfo(player.is_white, player.bitboards, player.bitboards.all_pieces,
													 player.locations.king, opponent.locations.king);

	player.bitboards.attacks = player_attacks.attacks_bitboard;
	opponent.bitboards.squares_to_uncheck = player_attacks.opponent_squares_to_uncheck;
	
	// Set opponent's pins, since opponent is next to play
	player.move_id++;
	opponent.move_id++;
	setPins(opponent, player);

	// Flip turn to move
	hash ^= zobrist_keys.is_black_to_move;
	nnue.flipSides();

	return { 
		player_could_castle_king_side, 
		player_could_castle_queen_side, 
		opponent_could_castle_king_side, 
		opponent_could_castle_queen_side, 
		opponent_en_passant_target, 
		capture_type, 
		hash 
	};
}

void unmakeMove(const unsigned short move, Player& player, Player& opponent, const MoveInfo& move_info) {
	unsigned short flag = getMoveFlag(move);
	location start_square = getStartSquare(move);
	location final_square = getFinalSquare(move);
	location initial_square_rook_king_side = player.is_white ? 7 : 63;
	location initial_square_rook_queen_side = player.is_white ? 0 : 56;
	short capture_type = move_info.capture_flag;

	opponent.locations.en_passant_target = move_info.opponent_en_passant_target;
	player.can_castle_king_side = move_info.player_could_castle_king_side;
	player.can_castle_queen_side = move_info.player_could_castle_queen_side;
	opponent.can_castle_king_side = move_info.opponent_could_castle_king_side;
	opponent.can_castle_queen_side = move_info.opponent_could_castle_queen_side;

	// Revert piece moved to start square
	location location_en_passant_pawn;
	switch (flag) {
	case pawn_move:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);

		nnue.movePiece(Pawn, final_square, start_square, player, opponent);

		break;

	case pawn_move_two_squares:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);

		nnue.movePiece(Pawn, final_square, start_square, player, opponent);

		player.locations.en_passant_target = 0;

		break;

	case knight_move:
		player.bitboards.removeKnight(final_square);
		player.bitboards.addKnight(start_square);

		nnue.movePiece(Knight, final_square, start_square, player, opponent);

		break;

	case bishop_move:
		player.bitboards.removeBishop(final_square);
		player.bitboards.addBishop(start_square);

		nnue.movePiece(Bishop, final_square, start_square, player, opponent);

		break;

	case rook_move:
		player.bitboards.removeRook(final_square);
		player.bitboards.addRook(start_square);

		nnue.movePiece(Rook, final_square, start_square, player, opponent);

		break;

	case queen_move:
		player.bitboards.removeQueen(final_square);
		player.bitboards.addQueen(start_square);

		nnue.movePiece(Queen, final_square, start_square, player, opponent);

		break;

	case king_move:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);

		nnue.setPosition(player, opponent);

		break;

	case castle_king_side:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);

		// Move king side rook
		player.bitboards.removeRook(final_square - 1);
		player.bitboards.addRook(initial_square_rook_king_side);

		nnue.setPosition(player, opponent);

		break;

	case castle_queen_side:
		player.bitboards.removeKing(final_square);
		player.bitboards.addKing(start_square);
		player.locations.moveKing(start_square);

		// Move queen side rook
		player.bitboards.removeRook(final_square + 1);
		player.bitboards.addRook(initial_square_rook_queen_side);

		nnue.setPosition(player, opponent);

		break;

	case en_passant:
		player.bitboards.removePawn(final_square);
		player.bitboards.addPawn(start_square);

		// Add back pawn target of en passant
		location_en_passant_pawn = player.is_white ? (final_square - 8) : (final_square + 8);
		player.bitboards.all_pieces |= (1LL << location_en_passant_pawn);
		opponent.bitboards.addPawn(location_en_passant_pawn);
		opponent.num_pawns++;

		nnue.movePiece(Pawn, final_square, start_square, player, opponent);
		nnue.addPiece(Pawn, location_en_passant_pawn, opponent, player);

		break;

	case promotion_knight:
		player.bitboards.removeKnight(final_square);
		player.bitboards.addPawn(start_square);
		player.num_pawns++;
		player.num_knights--;

		nnue.removePiece(Knight, final_square, player, opponent);
		nnue.addPiece(Pawn, start_square, player, opponent);

		break;

	case promotion_bishop:
		player.bitboards.removeBishop(final_square);
		player.bitboards.addPawn(start_square);
		player.num_pawns++;
		player.num_bishops--;

		nnue.removePiece(Bishop, final_square, player, opponent);
		nnue.addPiece(Pawn, start_square, player, opponent);

		break;

	case promotion_rook:
		player.bitboards.removeRook(final_square);
		player.bitboards.addPawn(start_square);
		player.num_pawns++;
		player.num_rooks--;

		nnue.removePiece(Rook, final_square, player, opponent);
		nnue.addPiece(Pawn, start_square, player, opponent);

		break;

	case promotion_queen:
		player.bitboards.removeQueen(final_square);
		player.bitboards.addPawn(start_square);
		player.num_pawns++;
		player.num_queens--;

		nnue.removePiece(Queen, final_square, player, opponent);
		nnue.addPiece(Pawn, start_square, player, opponent);

		break;

	default: // Null Move
		break;
	}

	// Captures
	// Add back piece that was captured and update piece count
	if (capture_type != no_capture) player.bitboards.all_pieces |= (1LL << final_square);

	switch (capture_type)
	{
	case pawn_capture:
		opponent.bitboards.addPawn(final_square);
		opponent.num_pawns++;
		nnue.addPiece(Pawn, final_square, opponent, player);
		break;

	case knight_capture:
		opponent.bitboards.addKnight(final_square);
		opponent.num_knights++;
		nnue.addPiece(Knight, final_square, opponent, player);
		break;

	case bishop_capture:
		opponent.bitboards.addBishop(final_square);
		opponent.num_bishops++;
		nnue.addPiece(Bishop, final_square, opponent, player);
		break;

	case rook_capture:
		opponent.bitboards.addRook(final_square);
		opponent.num_rooks++;
		nnue.addPiece(Rook, final_square, opponent, player);
		break;

	case queen_capture:
		opponent.bitboards.addQueen(final_square);
		opponent.num_queens++;
		nnue.addPiece(Queen, final_square, opponent, player);
		break;

	default:
		break;
	}

	opponent.bitboards.all_pieces = player.bitboards.all_pieces;

	nnue.flipSides();

	// Reset player's pins, since player is next to play
	player.move_id++;
	opponent.move_id++;
	setPins(player, opponent);
}
