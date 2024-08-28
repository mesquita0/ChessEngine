#include "Evaluate.h"
#include "MagicBitboards.h"
#include "PieceSquareTables.h"
#include "Player.h"
#include <array>
#include <bit>
#include <climits>

constexpr unsigned long long Afile = 0x8080808080808080;
constexpr unsigned long long Bfile = 0x4040404040404040;
constexpr unsigned long long Cfile = 0x2020202020202020;
constexpr unsigned long long Dfile = 0x1010101010101010;
constexpr unsigned long long Efile = 0x0808080808080808;
constexpr unsigned long long Ffile = 0x0404040404040404;
constexpr unsigned long long Gfile = 0x0202020202020202;
constexpr unsigned long long Hfile = 0x0101010101010101;
constexpr std::array<unsigned long long, 8> files = { Afile, Bfile, Cfile, Dfile, Efile, Gfile, Hfile };

constexpr int value_pawn = 100;
constexpr int value_knight = 320;
constexpr int value_bishop = 330;
constexpr int value_rook = 500;
constexpr int value_queen = 900;

static int PawnStructure(const Player& player);
static int MaterialScore(const Player& player);
static int PositionalScore(const Player& player, int num_major_pieces);
static int PieceScore(unsigned long long bitboard, int* piece_square_table, bool is_white);

int Evaluate(const Player& player, const Player& opponent, int num_pieces) {

	// If Checkmate
	if (player.bitboards.king & opponent.bitboards.attacks) {
		unsigned long long king_attacks = magic_bitboards.king_attacks_array[player.locations.king];
		king_attacks &= ~opponent.bitboards.attacks; // Remove defended squares
		king_attacks &= ~player.bitboards.friendly_pieces; // Remove squares occupied by player's pieces

		// If player's king can't move
		if (!king_attacks) {
			if (!(player.bitboards.squares_to_uncheck & player.bitboards.attacks)) { // Checkmate if player cant get out of check
				return checkmated_eval;
			}
		}
	}

	int score_player = MaterialScore(player);
	int score_opponent = MaterialScore(opponent);

	score_player += PawnStructure(player);
	score_opponent += PawnStructure(opponent);

	score_player += (value_pawn / 10) * std::popcount(player.bitboards.attacks);
	score_opponent += (value_pawn / 10) * std::popcount(opponent.bitboards.attacks);

	int num_major_pieces = num_pieces - opponent.num_pawns - player.num_pawns - 2;
	score_player += PositionalScore(player, num_major_pieces);
	score_opponent += PositionalScore(opponent, num_major_pieces);

	// Castle
	if (player.can_castle_king_side) score_player += value_pawn / 4;
	if (player.can_castle_queen_side) score_player += value_pawn / 5;
	if (opponent.can_castle_king_side) score_opponent += value_pawn / 4;
	if (opponent.can_castle_queen_side) score_opponent += value_pawn / 5;

	return score_player - score_opponent;
}


static int MaterialScore(const Player& player) {
	return value_pawn * player.num_pawns +
		value_knight * player.num_knights +
		value_bishop * player.num_bishops +
		value_rook * player.num_rooks +
		value_queen * player.num_queens;
}


static int PawnStructure(const Player& player) {
	int score = 0;

	for (int i = 0; i < 8; i++) {
		unsigned long long pawns_file = player.bitboards.pawns & files[i];
		if (!pawns_file) continue;

		// Bit hack to see if there's more than one pawn in the same file (doubled pawns)
		if ((pawns_file & (pawns_file - 1)) != 0) {
			score -= value_pawn / 2;
		}

		unsigned long long pawns_adjacent_file;
		if (i == 0) pawns_adjacent_file = player.bitboards.pawns & files[1];
		else if (i == 7) pawns_adjacent_file = player.bitboards.pawns & files[6];
		else pawns_adjacent_file = player.bitboards.pawns & (files[i - 1] | files[i + 1]);

		// Isolated pawn
		if (!pawns_adjacent_file) {
			score -= value_pawn / 2;
		}
	}

	return score;
}


static int PositionalScore(const Player& player, int num_major_pieces) {
	int score = 0;

	score += PieceScore(player.bitboards.pawns, pawn, player.is_white);
	score += PieceScore(player.bitboards.knights, knight, player.is_white);
	score += PieceScore(player.bitboards.bishops, bishop, player.is_white);
	score += PieceScore(player.bitboards.rooks, rook, player.is_white);
	score += PieceScore(player.bitboards.queens, queen, player.is_white);

	location loc_king = (player.is_white) ? 63 - player.locations.king : player.locations.king;
	if (num_major_pieces > 3) score += king_middle_game[loc_king];
	else score += king_end_game[loc_king];

	return score;
}


static int PieceScore(unsigned long long bitboard, int* piece_square_table, bool is_white) {
	int score = 0;
	
	location piece_location = 0;
	while (bitboard != 0 && piece_location <= 63) {
		int squares_to_skip = std::countr_zero(bitboard);
		piece_location += squares_to_skip;

		location loc = (is_white) ? 63 - piece_location : piece_location;
		score += piece_square_table[loc];

		bitboard >>= (squares_to_skip + 1);
		piece_location++;
	}

	return score;
}
