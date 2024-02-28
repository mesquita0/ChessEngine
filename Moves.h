#pragma once
#include "Player.h"
#include <array>

class MagicBitboards;

constexpr int max_num_moves = 218;
constexpr unsigned short castle_king_side = 0b0001 << 12;
constexpr unsigned short castle_queen_side = 0b0010 << 12;
constexpr unsigned short en_passant = 0b0011 << 12;
constexpr unsigned short promotion_knight = 0b0100 << 12;
constexpr unsigned short promotion_bishop = 0b0101 << 12;
constexpr unsigned short promotion_rook = 0b0110 << 12;
constexpr unsigned short promotion_queen = 0b0111 << 12;
constexpr unsigned short pawn_move = 0b1000 << 12;
constexpr unsigned short pawn_move_two_squares = 0b1001 << 12;
constexpr unsigned short knight_move = 0b1010 << 12;
constexpr unsigned short bishop_move = 0b1011 << 12;
constexpr unsigned short rook_move = 0b1100 << 12;
constexpr unsigned short queen_move = 0b1101 << 12;
constexpr unsigned short king_move = 0b1110 << 12;

struct AttacksInfo {
	unsigned long long attacks_bitboard;
	unsigned long long opponent_squares_to_uncheck;
};

/* 
	A move is a 16-bit unsigned short, with the first 4 bits representing a flag (Capture, En Passant, Promotion, Castle), 
	the subsequent 6 bits representing the starting square and the last 6 bits the final square. 
*/
class Moves {
	std::array<unsigned short, max_num_moves> moves = {};

public:
	short num_moves = 0;

	inline void addMove(unsigned short move) {
		moves[num_moves] = move;
		num_moves++;
	}

	inline void addMove(unsigned short flag, location start_square, location final_square) {
		addMove(flag | (start_square << 6) | final_square);
	}

	inline unsigned short* begin() { return &this->moves[0]; }
	inline const unsigned short* begin() const { return &this->moves[0]; }
	inline unsigned short* end() { return &this->moves[num_moves]; }
	inline const unsigned short* end() const { return &this->moves[num_moves]; }

	void generateMoves(const Player& player, const Player& opponent, const MagicBitboards& magic_bitboards);
	unsigned short isMoveValid(location start_square, location final_square);
};

AttacksInfo generateAttacksInfo(bool is_white, const Locations& locations, unsigned long long all_pieces,
								int num_pawns, int num_knights, int num_bishops, int num_rooks, int num_queens,
							    location opponent_king_location, const MagicBitboards& magic_bitboards);

inline unsigned long long generateAttacksBitBoard(bool is_white, const Locations& locations, unsigned long long all_pieces,
												  int num_pawns, int num_knights, int num_bishops, int num_rooks, int num_queens,
											      const MagicBitboards& magic_bitboards) {

	// Passing 64 as king location will skip all calculations of squares to uncheck, since they will not be used.
	return generateAttacksInfo(is_white, locations, all_pieces, num_pawns, num_knights, 
						       num_bishops, num_rooks, num_queens, 64, magic_bitboards).attacks_bitboard;
}

void setPins(Player& player, Player& opponent, const MagicBitboards& magic_bitboards);
