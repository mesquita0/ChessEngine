#include "GameOutcomes.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Player.h"
#include <vector>

constexpr unsigned long long white_squares = 0x55AA55AA55AA55AA;
constexpr unsigned long long black_squares = ~white_squares;

int updatePositions(std::vector<unsigned long long>& positions, const short capture_flag, const unsigned short move_flag, int half_moves) {
	if (capture_flag == no_capture && move_flag != pawn_move && move_flag != pawn_move_two_squares) { // If no capture or pawn moves
		half_moves++;

		if (move_flag == castle_king_side || move_flag == castle_queen_side) {
			positions.clear();
		}
	}
	else {
		half_moves = 0;
		positions.clear();
	}

	return half_moves;
}

GameOutcome getGameOutcome(const Player& player, const Player& opponent, const std::vector<unsigned long long>& positions, int half_moves) {
	if (half_moves == 100) {
		return draw_by_50_move_rule;
	}

	// Check draw by repetition
	if (positions.size() > 8) { // Can only repeat position 3 times after at leat 9 moves without captures or pawn moves

		// Position to be repeated is the last one, since if another position
		// was repeated 3 times the game would have alredy ended.
		unsigned long long repeated_pos = positions.back();
		int count = 1;

		// A position can only repeat after at leat two moves for each side
		int i = positions.size() - 5;

		while (i >= 0) {
			if (positions[i] == repeated_pos) {
				if (++count == 3) {
					return draw_by_repetition;
				}

				i -= 4; // 2 moves for each side
			}
			else {
				i -= 2;
			}
		}
	}

	// Draws by insufficient material
	const Player* player_with_only_king = nullptr;
	const Player* other_player = nullptr;
	if (player.bitboards.friendly_pieces == player.bitboards.king) {
		player_with_only_king = &player;
		other_player = &opponent;
	}
	else if (opponent.bitboards.friendly_pieces == opponent.bitboards.king) {
		player_with_only_king = &opponent;
		other_player = &player;
	}

	if (player_with_only_king != nullptr) {

		if (other_player->bitboards.friendly_pieces == other_player->bitboards.king) { // King vs King
			return draw_by_insufficient_material;
		}
		else if (other_player->bitboards.friendly_pieces == (other_player->bitboards.king | other_player->bitboards.knights)) { // King vs King and Knight
			return draw_by_insufficient_material;
		}
		else if (other_player->bitboards.friendly_pieces == (other_player->bitboards.king | other_player->bitboards.bishops)) { // King vs King and Bishop
			return draw_by_insufficient_material;
		}
	}

	// King and Bishop vs King and Bishop of the same color
	if ((player.bitboards.friendly_pieces == (player.bitboards.king | player.bitboards.bishops)) && (opponent.bitboards.friendly_pieces == (opponent.bitboards.king | opponent.bitboards.bishops))) {

		// If bishops are of the same color
		if (((white_squares & player.bitboards.bishops) && (white_squares & opponent.bitboards.bishops)) || (black_squares & player.bitboards.bishops) && (black_squares & opponent.bitboards.bishops)) {
			return draw_by_insufficient_material;
		}
	}

	return ongoing;
}
