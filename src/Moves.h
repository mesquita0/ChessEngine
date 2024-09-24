#pragma once
#include "Locations.h"
#include "Player.h"
#include <array>
#include <climits>

struct Entry;

constexpr unsigned short NULL_MOVE = 0;
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
static constexpr unsigned short move_flag_mask = 0b1111 << 12;
static constexpr unsigned short square_mask = 0b111111;

// Assert all promotion flags start with 01 so that isPromotion can work properly
static constexpr unsigned short promotion_mask = 0b11 << 14;
static constexpr unsigned short promotion = 0b01 << 14;
static_assert((promotion_knight & promotion_mask) == promotion);
static_assert((promotion_bishop & promotion_mask) == promotion);
static_assert((promotion_rook & promotion_mask) == promotion);
static_assert((promotion_queen & promotion_mask) == promotion);

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
	std::array<short, max_num_moves> scores = {};
	int last_score_picked = INT_MAX;

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

	inline unsigned short operator[] (int i) const { return moves[i]; }
	inline unsigned short& operator[] (int i) { return moves[i]; }

	void generateMoves(const Player& player, const Player& opponent);

	/* Updates player's attack bitboard, does not include king attacks to squares that 
	are defedend or attacks of pinned pieces that would leave the king in check if played */
	void generateCaptures(Player& player, const Player& opponent);

	unsigned short isMoveLegal (unsigned short move_flag, location start_square, location final_square) const;
	unsigned short isMoveLegal (location start_square, location final_square) const;
	bool isMoveLegal (unsigned short move) const;

	void orderMoves(const Player& player, const Player& opponent, const Entry* tt_entry, const std::array<unsigned short, 2>* killer_moves_at_ply);
	unsigned short getNextOrderedMove();
};

AttacksInfo generateAttacksInfo(bool is_white, const BitBoards& bitboards, unsigned long long all_pieces,
							    location player_king_location, location opponent_king_location);

inline unsigned long long generateAttacksBitBoard(bool is_white, const BitBoards& bitboards, unsigned long long all_pieces,
												  location player_king_location) {

	// Passing 64 as opponent king location will skip all calculations of squares to uncheck, since they will not be used.
	return generateAttacksInfo(is_white, bitboards, all_pieces, player_king_location, 64).attacks_bitboard;
}

void setPins(Player& player, Player& opponent);

inline bool isPromotion(unsigned short move) { return ((move & promotion_mask) == promotion); }
inline unsigned short getMoveFlag(unsigned short move) { return move & move_flag_mask; }
inline location getStartSquare(unsigned short move) { return (move >> 6) & square_mask; }
inline location getFinalSquare(unsigned short move) { return move & square_mask; }
inline bool isCapture(unsigned short move, unsigned long long opponent_pieces) { return ( (1LL << getFinalSquare(move)) & opponent_pieces ); }

bool isPseudoLegal(unsigned short move, const Player& player);
