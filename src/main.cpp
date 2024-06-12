#include "BitBoards.h"
#include "GameOutcomes.h"
#include "Moves.h"
#include "Perft.h"
#include "Player.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

constexpr std::chrono::milliseconds search_time(5000);

using std::cout;

int main() {

	// Load Magic Bitboards
	MagicBitboards magic_bitboards;
	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) {
		cout << "Couldn't load Magic Bitboards file.";
		return 1;
	}

	// Set up initial position
	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	auto [first_to_move, second_to_move, half_moves, full_moves] = FENToPosition(FEN, magic_bitboards);
	if (full_moves == -1) {
		cout << "Invalid Fen position.\n";
		return 2;
	}

	// Generate Zobrist keys and allocate transposition table
	ZobristKeys zobrist_keys;
	unsigned long long hash = zobrist_keys.positionToHash(first_to_move, second_to_move);
	TranspositionTable transposition_table(256, first_to_move.bitboards.all_pieces); // Size in mb, must be a power of two
	
	Player* player = &first_to_move;
	Player* opponent = &second_to_move;

	// Get user's pieces color
	bool is_engine_turn, is_engine_white;
	cout << "Enter 0 to play as white and 1 to play as black: ";
	std::cin >> is_engine_white;
	if ((!is_engine_white && !player->is_white) || (is_engine_white && player->is_white)) is_engine_turn = true;
	else is_engine_turn = false;

	GameOutcome game_outcome = ongoing;
	Moves moves;
	moves.generateMoves(*player, *opponent, magic_bitboards);
	HashPositions hash_positions(hash);

	while (moves.num_moves != 0) {

		cout << "Fen: " << PositionToFEN(*player, *opponent, half_moves, full_moves) << '\n';
		printBoard(*player, *opponent);
		cout << (((*player).is_white) ? "White to move: " : "Black to move: ");
		unsigned short move = 0;
	
		if (!is_engine_turn) {
			while (true) {
				std::string move_str;
				std::cin >> move_str;
				std::transform(move_str.begin(), move_str.end(), move_str.begin(), (int (*)(int))std::tolower);

				// Moves that are not promotions
				if (move_str.length() == 4) {
					location start_square = notationSquareToLocation(move_str.substr(0, 2));
					location final_square = notationSquareToLocation(move_str.substr(2, 2));
					unsigned short tmp_move = moves.isMoveLegal(start_square, final_square);
					if (!isPromotion(tmp_move)) move = tmp_move;
				}

				// Promotions
				else if (move_str.length() == 5) {
					location start_square = notationSquareToLocation(move_str.substr(0, 2));
					location final_square = notationSquareToLocation(move_str.substr(2, 2));
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

					move = moves.isMoveLegal(promotion_flag, start_square, final_square);
				}
				
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
			SearchResult search_result = FindBestMoveItrDeepening(search_time, *player, *opponent, hash_positions, half_moves, magic_bitboards, zobrist_keys, transposition_table);
			move = search_result.best_move;
			cout << locationToNotationSquare((move >> 6) & 0b111111);
			cout << locationToNotationSquare(move & 0b111111);
			cout << " Evaluation: " << search_result.evaluation;
			cout << " Depth: " << search_result.depth;
			cout << "\n\n";
			is_engine_turn = false;
		}

		MoveInfo move_info = makeMove(move, *player, *opponent, hash, magic_bitboards, zobrist_keys);

		short move_flag = getMoveFlag(move);
		transposition_table.updateMoveRoot(move_info.capture_flag, move_flag);
		half_moves = hash_positions.updatePositions(move_info.capture_flag, move_flag, move_info.hash, half_moves);

		if (!player->is_white) full_moves++;

		game_outcome = getGameOutcome(*player, *opponent, hash_positions, half_moves);
		if (game_outcome != ongoing) break;

		// Swap turn and generate moves for player
		Player* tmp = player;
		player = opponent;
		opponent = tmp;
		moves.generateMoves(*player, *opponent, magic_bitboards);
	}

	cout << "Fen: " << PositionToFEN(*player, *opponent, half_moves, full_moves) << '\n';
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
	cout << PositionToFEN(*player, *opponent, 0, 1);
}
