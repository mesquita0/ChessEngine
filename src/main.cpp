#include "BitBoards.h"
#include "Moves.h"
#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Search.h"
#include "Zobrist.h"
#include <array>
#include <bit>
#include <iostream>
#include <sstream>
#include <string>

enum GameOutcome {ongoing, checkmate, stalemate, draw_by_insufficient_material, draw_by_repetition, draw_by_50_move_rule};
constexpr unsigned long long white_squares = 0x55AA55AA55AA55AA;
constexpr unsigned long long black_squares = ~white_squares;

using std::cout;

int main() {

	// Load Magic Bitboards
	MagicBitboards magic_bitboards;
	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) {
		cout << "Couldn't load Magic Bitboards file.";
		return 2;
	}

	// Set up initial position
	std::string FEN = "7k/8/8/8/8/1PPPPPPP/q7/7K b - - 51 26";
	Position initial_pos = FENToPosition(FEN, magic_bitboards);
	auto [first_to_move, second_to_move, half_moves, full_moves] = initial_pos;
	if (full_moves == -1) {
		cout << "Invalid Fen position.\n";
		return 1;
	}

	ZobristKeys zobrist_keys = ZobristKeys();
	unsigned long long hash = zobrist_keys.positionToHash(initial_pos);

	Player* player = &first_to_move;
	Player* opponent = &second_to_move;

	// TODO: get user pieces color
	bool is_engine_turn;
	cout << "Enter 0 to play as white and 1 to play as black: ";
	std::cin >> is_engine_turn;

	GameOutcome game_outcome = ongoing;
	Moves moves;
	moves.generateMoves(*player, *opponent, magic_bitboards);
	std::vector<unsigned long long> positions = { hash };

	while (moves.num_moves != 0) {

		printBoard(*player, *opponent);
		cout << (((*player).is_white) ? "White to move: " : "Black to move: ");
		unsigned short move = 0;
	
		if (!is_engine_turn) {
			while (true) {
				std::string move_str;
				std::cin >> move_str;
				location start_square = notationSquareToLocation(move_str.substr(0, 2));
				location final_square = notationSquareToLocation(move_str.substr(2, 2));
				move = moves.isMoveValid(start_square, final_square);
				
				if (move == 0) {
					cout << "Move is not valid, please enter a valid move.\n\n";
				}
				else {
					is_engine_turn = true;
					cout << "\n";
					break;
				}
			}
		}
		else {
			// TODO: Seach function
			move = *moves.begin();
			cout << locationToNotationSquare((move >> 6) & 0b111111);
			cout << locationToNotationSquare(move  & 0b111111);
			cout << "\n\n";
			is_engine_turn = false;
		}

		short capture_flag = makeMove(move, *player, *opponent, magic_bitboards).capture_flag;

		unsigned short move_flag = (move & 0xf000);
		if (capture_flag != no_capture && move_flag != pawn_move && move_flag != pawn_move_two_squares) { // If no capture or pawn moves
			half_moves++;

			if (move_flag == castle_king_side || move_flag == castle_queen_side) {	
				positions.clear();
			}
		}
		else {
			half_moves = 0;
			positions.clear();
		}
		
		// Add new hash to positons
		hash = zobrist_keys.makeMove(hash, move);
		positions.push_back(hash);

		if (!player->is_white) full_moves++;

		if (half_moves == 100) {
			game_outcome = draw_by_50_move_rule;
			break;
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
						game_outcome = draw_by_repetition;
						break;
					}

					i -= 4; // 2 moves for each side
				}
				else {
					i -= 2;
				}
			}
		}

		// Draws by insufficient material
		Player* player_with_only_king = nullptr;
		Player* other_player = nullptr;
		if (player->bitboards.friendly_pieces == player->bitboards.king) {
			player_with_only_king = player;
			other_player = opponent;
		}
		else if (opponent->bitboards.friendly_pieces == opponent->bitboards.king) {
			player_with_only_king = opponent;
			other_player = player;
		}

		if (player_with_only_king != nullptr) {

			if (other_player->bitboards.friendly_pieces == other_player->bitboards.king) { // King vs King
				game_outcome = draw_by_insufficient_material;
				break;
			}
			else if (other_player->bitboards.friendly_pieces == (other_player->bitboards.king | other_player->bitboards.knights)) { // King vs King and Knight
				game_outcome = draw_by_insufficient_material;
				break;
			}
			else if (other_player->bitboards.friendly_pieces == (other_player->bitboards.king | other_player->bitboards.bishops)) { // King vs King and Bishop
				game_outcome = draw_by_insufficient_material;
				break;
			}
		}

		// King and Bishop vs King and Bishop of the same color
		if ((player->bitboards.friendly_pieces == (player->bitboards.king | player->bitboards.bishops)) && (opponent->bitboards.friendly_pieces == (opponent->bitboards.king | opponent->bitboards.bishops))) {
			
			// If bishops are of the same color
			if (((white_squares & player->bitboards.bishops) && (white_squares & opponent->bitboards.bishops)) || (black_squares & player->bitboards.bishops) && (black_squares & opponent->bitboards.bishops)) {
				game_outcome = draw_by_insufficient_material;
				break;
			}
		}

		// Swap turn and generate moves for player
		Player* tmp = player;
		player = opponent;
		opponent = tmp;
		moves.generateMoves(*player, *opponent, magic_bitboards);
	}

	printBoard(*player, *opponent);

	if (game_outcome == ongoing) {
		if (player->bitboards.king & opponent->bitboards.attacks) {	// If Player's king is attacked
			game_outcome = checkmate;
		}
		else {
			game_outcome = stalemate;
		}
	}

	switch (game_outcome) {
	case checkmate:
		cout << ((*player).is_white ? "Black wins by checkmate!\n" : "White wins by checkmate!\n");
		break;

	case stalemate:
		cout << "Draw by Stalemate!\n";
		break;

	case draw_by_insufficient_material:
		cout << "Draw by Insufficient Material!\n";
		break;

	case draw_by_repetition:
		cout << "Draw by Repetition!\n";
		break;

	case draw_by_50_move_rule:
		cout << "Draw by 50-Move Rule!\n";
		break;

	}
}
