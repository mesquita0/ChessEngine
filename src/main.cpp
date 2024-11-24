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
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>

constexpr int default_time = 5000;

using std::cout;

int main(int argc, char* argv[]) {

	bool quiet_mode = false;
	int fixed_depth = 0;
	std::chrono::milliseconds search_time(default_time);
	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	
	// Flags 
	for (int i = 1; i < argc; i++) {
		if (!strcmp("-h", argv[i])) {
			cout << "Optional arguments:\n";
			cout.width(30); cout << std::left << "-h, --help"; cout << "Show this help message and exits.\n";
			cout.width(30); cout << std::left << "-d DEPTH, --depth DEPTH"; cout << "Sets fixed depth for each move search.\n";
			cout.width(30); cout << std::left << "-q, --quiet"; cout << "Activates quiet mode, won't print board after each move.\n";
			cout.width(30); cout << std::left << "-t TIME, --time TIME"; cout << "Sets fixed time (in ms) for each move search, should be at least 30ms.\n";
			cout.width(30); cout << std::left << "--fen FEN"; cout << "Sets the starting position for the game.\n";
			return 0;
		}
		else if (!strcmp("-q", argv[i]) || !strcmp("--quiet", argv[i])) {
			quiet_mode = true;
		}
		else if (!strcmp("-t", argv[i]) || !strcmp("--time", argv[i])) {
			if (++i < argc) {
				int time_ms;

				try { time_ms = std::stoi(argv[i]); }
				catch (std::exception const& e) {
					cout << "Couldn't parse time \"" << argv[i] << "\" to a number.\n";
					return 1;
				}

				search_time = std::chrono::milliseconds(time_ms);
			}
			else {
				cout << "No time was provided with the time flag.\n";
				return 2;
			}
		}
		else if (!strcmp("-d", argv[i]) || !strcmp("--depth", argv[i])) {
			if (++i < argc) {
				try { fixed_depth = std::stoi(argv[i]); }
				catch (std::exception const& e) {
					cout << "Couldn't parse depth \"" << argv[i] << "\" to a number.\n";
					return 3;
				}

				if (fixed_depth <= 0) {
					cout << "Depth should be greater than 0.\n";
					return 4;
				}
			}
			else {
				cout << "No depth was provided with the depth flag.\n";
				return 5;
			}
		}
		else if (!strcmp("--fen", argv[i])) {
			if (i + 6 >= argc) {
				cout << "Invalid Fen position.\n";
				return 10;
			}

			FEN = "";
			for (int j = 1; j < 7; j++) {
				FEN.append(argv[i + j]);
				if (j != 6) FEN.append(" ");
			}
			i += 6;
		}
		else {
			cout << "Unknown parameter: " << argv[i] << '\n';
			return 6;
		}
	}

	if (search_time.count() != default_time && fixed_depth != 0) {
		cout << "Specifying both search time and depth is not supported.\n";
		return 7;
	}

	if (search_time.count() < 30) {
		cout << "Time per move should be at least 30 ms.\n";
		return 8;
	}

	// Load Magic Bitboards
	bool loaded = magic_bitboards.loadMagicBitboards();
	if (!loaded) {
		cout << "Couldn't load Magic Bitboards file.";
		return 9;
	}

	if (!nnue.is_loaded()) {
		cout << "Couldn't load weights of NNUE.";
		return 10;
	}

	// Set up initial position
	auto [first_to_move, second_to_move, half_moves, full_moves] = FENToPosition(FEN);
	if (full_moves == -1) {
		cout << "Invalid Fen position.\n";
		return 11;
	}

	// Generate Zobrist keys and allocate transposition table
	unsigned long long hash = zobrist_keys.positionToHash(first_to_move, second_to_move);
	tt = TranspositionTable(256, first_to_move.bitboards.all_pieces); // Size in mb, must be a power of two

	// Set up nnue
	nnue.setPosition(first_to_move, second_to_move);

	Player* player = &first_to_move;
	Player* opponent = &second_to_move;

	// Get user's pieces color
	bool is_engine_turn = false;
	char p = 0;
	while (p != 'b' && p != 'w') {
		cout << "Enter w to play as white and b to play as black: ";
		std::cin >> p;
		if (std::cin.eof()) return 0;

		p = std::tolower(p);
		bool is_engine_white = (p == 'b');
		if ((!is_engine_white && !player->is_white) || (is_engine_white && player->is_white)) is_engine_turn = true;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	GameOutcome game_outcome = ongoing;
	Moves moves;
	moves.generateMoves(*player, *opponent);
	HashPositions hash_positions(hash);

	while (moves.num_moves != 0) {

		cout << "Fen: " << PositionToFEN(*player, *opponent, half_moves, full_moves) << '\n';
		if (!quiet_mode) {
			printBoard(*player, *opponent);
			cout << (((*player).is_white) ? "White to move: " : "Black to move: ");
		}
		unsigned short move = 0;
	
		if (!is_engine_turn) {
			while (true) {
				std::string move_str;
				std::cin >> move_str;
				if (std::cin.eof()) return 0;
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
			// Search best move
			SearchResult search_result;
			if (fixed_depth)
				search_result = FindBestMoveItrDeepening(fixed_depth, *player, *opponent, hash_positions, half_moves);
			else
				search_result = FindBestMoveItrDeepening(search_time, *player, *opponent, hash_positions, half_moves);
			
			// Print best move
			move = search_result.best_move;
			cout << locationToNotationSquare((move >> 6) & 0b111111);
			cout << locationToNotationSquare(move & 0b111111);
			if (isPromotion(move)) {
				unsigned short promotion_flag = getMoveFlag(move);

				switch (promotion_flag) {
				case promotion_knight:
					cout << 'n';
					break;
				case promotion_bishop:
					cout << 'b';
					break;
				case promotion_rook:
					cout << 'r';
					break;
				case promotion_queen:
					cout << 'q';
					break;
				}
			}

			if (!quiet_mode) {
				cout << " Evaluation: " << search_result.evaluation;
				cout << " Depth: " << search_result.depth;
				cout << '\n';
			}
			cout << "\n";
			is_engine_turn = false;
		}

		MoveInfo move_info = makeMove(move, *player, *opponent, hash);
		hash = move_info.hash;

		short move_flag = getMoveFlag(move);
		tt.updateMoveRoot(move_info.capture_flag, move_flag);
		half_moves = hash_positions.updatePositions(move_info.capture_flag, move_flag, hash, half_moves);

		if (!player->is_white) full_moves++;

		game_outcome = getGameOutcome(*player, *opponent, hash_positions, half_moves);
		if (game_outcome != ongoing) break;

		// Swap turn and generate moves for player
		Player* tmp = player;
		player = opponent;
		opponent = tmp;
		moves.generateMoves(*player, *opponent);
	}

	cout << "Fen: " << PositionToFEN(*player, *opponent, half_moves, full_moves) << '\n';
	if (!quiet_mode) {
		printBoard(*player, *opponent);
	}

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
	cout << "Fen: " << PositionToFEN(*player, *opponent, 0, 1);
}
