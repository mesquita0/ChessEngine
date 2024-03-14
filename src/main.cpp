#include "BitBoards.h"
#include "GameOutcomes.h"
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
#include <vector>
#include <string>

constexpr int search_depth = 6;

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
	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
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

		printBoard(*player, *opponent);
		cout << (((*player).is_white) ? "White to move: " : "Black to move: ");
		unsigned short move = 0;
	
		if (!is_engine_turn) {
			while (true) {
				std::string move_str;
				std::cin >> move_str;
				if (move_str.length() != 4) {
					cout << "Move is not valid, please enter a valid move.\n\n";
					continue;
				}

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
			move = FindBestMove(search_depth, *player, *opponent, hash_positions, half_moves, magic_bitboards, zobrist_keys);
			cout << locationToNotationSquare((move >> 6) & 0b111111);
			cout << locationToNotationSquare(move & 0b111111);
			cout << "\n\n";
			is_engine_turn = false;
		}

		MoveInfo move_info = makeMove(move, *player, *opponent, hash, magic_bitboards, zobrist_keys);

		unsigned short move_flag = (move & 0xf000);
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
