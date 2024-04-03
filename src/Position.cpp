#include "Position.h"
#include "BitBoards.h"
#include "MagicBitboards.h"
#include "Moves.h"
#include "Locations.h"
#include <array>
#include <ctype.h>
#include <iostream>
#include <string>
#include <sstream>

using std::array, std::cout, std::string;

static array<array<char, 8>, 8> positionToBoard(const Player& white, const Player& black);
void addToBoard(array<array<char, 8>, 8>& board, unsigned long long bitBoard, char pieceType);

string PositionToFEN(const Player& player, const Player& opponent, int half_moves, int full_moves) {
	const Player& white = player.is_white ? player : opponent;
	const Player& black = player.is_white ? opponent : player;
	array<array<char, 8>, 8> board = positionToBoard(white, black);
	string result;

	// Add Piece Placement to result
	for (int rank = 7; rank >= 0; rank--) {
		int empty_spaces = 0;

		for (int file = 0; file < 8; file++) {
			if (board[rank][file] == '\0') {
				empty_spaces++;
				continue;
			}

			if (empty_spaces != 0) {
				result += std::to_string(empty_spaces);
			}

			result += board[rank][file];
			empty_spaces = 0;
		}

		if (empty_spaces != 0) {
			result += std::to_string(empty_spaces);
		}

		if (rank != 0) result += "/";
	}

	// Add who's to move and rights to castle to result
	result += (player.is_white ? " w " : " b ");
	bool castle = false;
	if (white.can_castle_king_side) {
		castle = true;
		result += "K";
	}

	if (white.can_castle_queen_side) {
		castle = true;
		result += "Q";
	}

	if (black.can_castle_king_side) {
		castle = true;
		result += "k";
	}

	if (black.can_castle_queen_side) {
		castle = true;
		result += "q";
	}
	if (!castle) result += " - ";

	// Add En Passant targets and number of moves to result
	result += " ";
	if (white.locations.en_passant_target != 0) {
		result += locationToNotationSquare(white.locations.en_passant_target) + " ";
	} 
	else if (black.locations.en_passant_target != 0) {
		result += locationToNotationSquare(white.locations.en_passant_target) + " ";
	} 
	else {
		result += "- ";
	}
	
	result += std::to_string(half_moves) + " ";
	result += std::to_string(full_moves);

	return result;
}

Position FENToPosition(const string& FEN, const MagicBitboards& magic_bitboards) {
	Player white = Player(true);
	Player black = Player(false);
	const Position invalid_fen = { white, black, -1, -1 };
	
	string position, turn, castle, en_passant, half_moves_str, full_moves_str;
	std::stringstream s(FEN);
	s >> position >> turn >> castle >> en_passant >> half_moves_str >> full_moves_str;

	// Set bitboards
	bool is_white_king_set = false;
	bool is_black_king_set = false;
	int row = 7;
	int column = 0;
	for (char& c : position) {
		unsigned long long square = row * 8 + column;
		if (isdigit(c)) {
			int n = c - '0';
			if (n > 8 || n < 0) return invalid_fen;
			column += n;
			continue;
		}
		else if (c == '/') {
			row--;
			column = 0;
			continue;
		}

		if (row < 0 || column > 7) {
			return invalid_fen;
		}

		Player& current_player = (isupper(c) ? white : black);
		c = tolower(c);

		switch (c) {
		case 'p':
			current_player.bitboards.pawns |= (1LL << square);
			current_player.num_pawns++;
			break;

		case 'n':
			current_player.bitboards.knights |= (1LL << square);
			current_player.num_knights++;
			break;

		case 'b':
			current_player.bitboards.bishops |= (1LL << square);
			current_player.num_bishops++;
			break;

		case 'r':
			current_player.bitboards.rooks |= (1LL << square);
			current_player.num_rooks++;
			break;

		case 'q':
			current_player.bitboards.queens |= (1LL << square);
			current_player.num_queens++;
			break;

		case 'k':
			current_player.bitboards.king |= (1LL << square);
			current_player.locations.king = square;

			// Assure just one king per side
			if (current_player.is_white) {
				if (!is_white_king_set) is_white_king_set = true;
				else return invalid_fen;
			}
			else {
				if (!is_black_king_set) is_black_king_set = true;
				else return invalid_fen;
			}

			break;

		default:
			return invalid_fen;
			break;
		}
		column++;
	}

	if (!(is_white_king_set && is_black_king_set)) return invalid_fen;

	white.bitboards.friendly_pieces = (white.bitboards.pawns | white.bitboards.knights | white.bitboards.bishops |
										white.bitboards.rooks | white.bitboards.queens | white.bitboards.king);
	black.bitboards.friendly_pieces = (black.bitboards.pawns | black.bitboards.knights | black.bitboards.bishops |
										black.bitboards.rooks | black.bitboards.queens | black.bitboards.king);

	white.bitboards.all_pieces = white.bitboards.friendly_pieces | black.bitboards.friendly_pieces;
	black.bitboards.all_pieces = white.bitboards.friendly_pieces | black.bitboards.friendly_pieces;

	AttacksInfo white_attack = generateAttacksInfo(true, white.bitboards, white.bitboards.all_pieces,
												   white.locations.king, black.locations.king, magic_bitboards);
	AttacksInfo black_attack = generateAttacksInfo(false, black.bitboards, black.bitboards.all_pieces,
												   black.locations.king, white.locations.king, magic_bitboards);

	white.bitboards.attacks = white_attack.attacks_bitboard;
	white.bitboards.squares_to_uncheck = black_attack.opponent_squares_to_uncheck;
	black.bitboards.attacks = black_attack.attacks_bitboard;
	black.bitboards.squares_to_uncheck = white_attack.opponent_squares_to_uncheck;

	setPins(white, black, magic_bitboards);
	setPins(black, white, magic_bitboards);

	// Set turn
	bool is_white_to_move;
	Player *player;
	Player *opponent;
	if (turn == "w") {
		player = &white;
		opponent = &black;
		is_white_to_move = true;

		// Make sure side to move can't capture the enemy king
		if (white.bitboards.attacks & black.bitboards.king) return invalid_fen;
	}
	else if (turn == "b") {
		player = &black;
		opponent = &white;
		is_white_to_move = false;

		// Make sure side to move can't capture the enemy king
		if (black.bitboards.attacks & white.bitboards.king) return invalid_fen;
	}
	else {
		return invalid_fen;
	}

	// Set castle rights
	white.can_castle_king_side = false;
	white.can_castle_queen_side = false;
	black.can_castle_king_side = false;
	black.can_castle_queen_side = false;

	if (castle != "-") {
		for (char& c : castle) {
			Player& current_player = (isupper(c) ? white : black);
			c = tolower(c);

			switch (c) {
			case 'k':
				current_player.can_castle_king_side = true;
				break;

			case 'q':
				current_player.can_castle_queen_side = true;
				break;

			default:
				return invalid_fen;
				break;
			}
		}
	}

	// Set En Passant
	if (en_passant != "-") {
		location square = notationSquareToLocation(en_passant);
		if (square == -1) return invalid_fen;
		
		if (is_white_to_move) {
			// Check if the En Passant square is valid 
			if (square < 40 || square > 47) return invalid_fen;
			if (!(black.bitboards.pawns & 1LL << (square - 8))) return invalid_fen;

			black.locations.en_passant_target = square;
		}
		else {
			// Check if the En Passant square is valid 
			if (square < 16 || square > 23) return invalid_fen;
			if (!(white.bitboards.pawns & 1LL << (square + 8))) return invalid_fen;

			white.locations.en_passant_target = square;
		}
	}

	// Set half and full moves
	int half_moves;
	int full_moves;
	try {
		half_moves = std::stoi(half_moves_str);
		full_moves = std::stoi(full_moves_str);
	}
	catch (...) {
		half_moves = 0;
		full_moves = 0;
	}

	return { *player, *opponent, half_moves, full_moves };
}

static array<array<char, 8>, 8> positionToBoard(const Player& white, const Player& black) {
	array<array<char, 8>, 8> board = { {} };

	addToBoard(board, white.bitboards.pawns, 'P');
	addToBoard(board, white.bitboards.knights, 'N');
	addToBoard(board, white.bitboards.bishops, 'B');
	addToBoard(board, white.bitboards.rooks, 'R');
	addToBoard(board, white.bitboards.queens, 'Q');
	addToBoard(board, white.bitboards.king, 'K');
	addToBoard(board, black.bitboards.pawns, 'p');
	addToBoard(board, black.bitboards.knights, 'n');
	addToBoard(board, black.bitboards.bishops, 'b');
	addToBoard(board, black.bitboards.rooks, 'r');
	addToBoard(board, black.bitboards.queens, 'q');
	addToBoard(board, black.bitboards.king, 'k');

	return board;
}

void addToBoard(array<array<char, 8>, 8>& board, unsigned long long bitBoard, char pieceType) {
	int row = 0;
	int column = 0;

	while (bitBoard != 0) {
		if (column > 7) {
			column = 0;
			row++;
		}

		unsigned long long bit = bitBoard & 1;

		if (bit) {
			board[row][column] = pieceType;
		}

		bitBoard >>= 1;
		column++;
	}

}
 
string locationToNotationSquare(int square) {
	int file_int = square % 8;
	int rank_int = (square - file_int) / 8;

	char file = file_int + 'a';
	char rank = rank_int + '1';

	string result{ file, rank };

	return result;
}

location notationSquareToLocation(const string& square) {
	if (square.length() != 2) return -1;

	char file = square[0];
	char rank = square[1];
	int file_int = file - 'a';
	int rank_int = rank - '1';

	if (file_int < 0 || file_int > 7) return -1;
	if (rank_int < 0 || rank_int > 7) return -1;

	return (rank_int * 8 + file_int);
}

void printBoard(const Player& player, const Player& opponent) {
	const Player& white = player.is_white ? player : opponent;
	const Player& black = player.is_white ? opponent : player;
	array<array<char, 8>, 8> board = positionToBoard(white, black);
	char top_left_corner = unsigned char(218);
	char top_right_corner = unsigned char(191);
	char bottom_left_corner = unsigned char(192);
	char bottom_right_corner = unsigned char(217);
	char edge_intersections_top = unsigned char(194);
	char edge_intersections_bottom = unsigned char(193);
	char edge_intersections_left = unsigned char(195);
	char edge_intersections_right = unsigned char(180);
	char four_way_intersection = unsigned char(197);
	char vertical_bar = unsigned char(179);
	char horizontal_bar = unsigned char(196);

	cout << "   " << top_left_corner;
	for (int i = 0; i < 8; i++) {
		cout << horizontal_bar << horizontal_bar << horizontal_bar;
		if (i != 7) cout << edge_intersections_top;
		else cout << top_right_corner;
	}
	cout << "\n";

	for (int rank = 7; rank >= 0; rank--) {
		cout << ' ' << (rank + 1) << " " << vertical_bar << " ";
		for (int file = 0; file < 8; file++) {
			char piece = board[rank][file];
			if (piece == '\0') piece = ' ';
			cout << piece << " " << vertical_bar << " "; // Least significant bit is a1, then a2 ...
		}

		cout << "\n";
		cout << "   ";
		cout << ((rank != 0) ? edge_intersections_left : bottom_left_corner);

		for (int i = 0; i < 8; i++) {
			cout << horizontal_bar << horizontal_bar << horizontal_bar;
			if (i != 7) {
				if (rank == 0) cout << edge_intersections_bottom;
				else cout << four_way_intersection;
			}
		}

		cout << ((rank != 0) ? edge_intersections_right : bottom_right_corner);
		cout << "\n";
	}

	cout << "     a   b   c   d   e   f   g   h\n\n";
}
