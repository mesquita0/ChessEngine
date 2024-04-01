#include "Evaluate.h"
#include "MagicBitboards.h"
#include "Player.h"
#include <array>
#include <bitset>
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

static int PawnStructure(const Player& player);

int Evaluate(const Player& player, const Player& opponent, const MagicBitboards& magic_bitboards) {

	// If Checkmate
	if (player.bitboards.king & opponent.bitboards.attacks) {
		unsigned long long king_attacks = magic_bitboards.king_attacks_array[player.locations.king];
		king_attacks &= ~opponent.bitboards.attacks; // Remove defended squares
		king_attacks &= ~player.bitboards.all_pieces; // Remove occupied squares

		// If player's king can't move
		if (!king_attacks) {
			if (!(player.bitboards.squares_to_uncheck & player.bitboards.attacks)) { // Checkmate if player cant get out of check
				return checkmated_eval;
			}
		}
	}

	double score_player = player.num_pawns + 30 * (player.num_knights + player.num_bishops) + player.num_rooks * 50 + player.num_queens * 90;
	double score_opponent = opponent.num_pawns + 30 * (opponent.num_knights + opponent.num_bishops) + opponent.num_rooks * 50 + opponent.num_queens * 90;

	score_player += PawnStructure(player);
	score_opponent += PawnStructure(opponent);

	std::bitset<64> attacks_player{ player.bitboards.attacks };
	std::bitset<64> attacks_opponent{ opponent.bitboards.attacks };
	score_player += attacks_player.count();
	score_opponent += attacks_opponent.count();

	// Castle
	if (player.can_castle_king_side) score_player += 2;
	if (player.can_castle_queen_side) score_player += 2;
	if (opponent.can_castle_king_side) score_opponent += 2;
	if (opponent.can_castle_queen_side) score_opponent += 2;

	return score_player - score_opponent;
}

static int PawnStructure(const Player& player) {
	int score = 0;

	for (int i = 0; i < 8; i++) {
		unsigned long long pawns_file = player.bitboards.pawns & files[i];
		if (!pawns_file) continue;

		// Bit hack to see if there's more than one pawn in the same file (doubled pawns)
		if ((pawns_file & (pawns_file - 1)) != 0) {
			score -= 5;
		}

		unsigned long long pawns_adjacent_file;
		if (i == 0) pawns_adjacent_file = player.bitboards.pawns & files[1];
		else if (i == 7) pawns_adjacent_file = player.bitboards.pawns & files[6];
		else pawns_adjacent_file = player.bitboards.pawns & (files[i - 1] | files[i + 1]);

		// Isolated pawn
		if (!pawns_adjacent_file) {
			score -= 5;
		}
	}

	return score;
}
