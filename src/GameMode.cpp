#include "GameMode.h"
#include <iostream>
#include <string>

using std::cout;

Flags process_flags(int argc, char* argv[]) {
	Flags flags;

	for (int i = 1; i < argc; i++) {
		if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
			cout << "Optional arguments:\n";
			cout.width(30); cout << std::left << "-h, --help"; cout << "Show this help message and exits.\n";
			cout.width(30); cout << std::left << "-d DEPTH, --depth DEPTH"; cout << "Sets fixed depth for each move search.\n";
			cout.width(30); cout << std::left << "-q, --quiet"; cout << "Activates quiet mode, won't print board after each move.\n";
			cout.width(30); cout << std::left << "-t TIME, --time TIME"; cout << "Sets fixed time (in ms) for each move search, should be at least 30ms.\n";
			cout.width(30); cout << std::left << "--fen FEN"; cout << "Sets the starting position for the game.\n";

			flags.show_help = true;
			break;
		}
		else if (!strcmp("-g", argv[i]) || !strcmp("--game", argv[i])) {
			flags.game_mode = true;
		}
		else if (!strcmp("-q", argv[i]) || !strcmp("--quiet", argv[i])) {
			flags.quiet_mode = true;
		}
		else if (!strcmp("-t", argv[i]) || !strcmp("--time", argv[i])) {
			if (++i < argc) {
				int time_ms;

				try { time_ms = std::stoi(argv[i]); }
				catch (std::exception const& e) {
					cout << "Couldn't parse time \"" << argv[i] << "\" to a number.\n";
					flags.error_code = 2;
					break;
				}

				if (time_ms < minimum_search_time) time_ms = minimum_search_time;
				flags.search_time = time_ms;
			}
			else {
				cout << "No time was provided with the time flag.\n";
				flags.error_code = 3;
				break;
			}
		}
		else if (!strcmp("-d", argv[i]) || !strcmp("--depth", argv[i])) {
			if (++i < argc) {
				try { flags.fixed_depth = std::stoi(argv[i]); }
				catch (std::exception const& e) {
					cout << "Couldn't parse depth \"" << argv[i] << "\" to a number.\n";
					flags.error_code = 4;
					break;
				}

				if (flags.fixed_depth <= 0) {
					cout << "Depth should be greater than 0.\n";
					flags.error_code = 5;
					break;
				}
			}
			else {
				cout << "No depth was provided with the depth flag.\n";
				flags.error_code = 6;
				break;
			}
		}
		else if (!strcmp("--fen", argv[i])) {
			if (i + 6 >= argc) {
				cout << "Invalid Fen position.\n";
				flags.error_code = 7;
				break;
			}

			std::string FEN = "";
			for (int j = 1; j < 7; j++) {
				FEN.append(argv[i + j]);
				if (j != 6) FEN.append(" ");
			}
			flags.fen = FEN;

			i += 6;
		}
	}

	return flags;
}

int game_mode(Engine& engine, Flags flags) {
	bool quiet_mode = flags.quiet_mode;
	if (flags.fen.length() == 0) flags.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	engine.setDepth(flags.fixed_depth);
	engine.setSearchTime(flags.search_time);

	// Set up initial position
	Position position = FENToPosition(flags.fen);
	auto& [first_to_move, second_to_move, half_moves, full_moves] = position;
	if (full_moves == -1) {
		cout << "Invalid Fen position.\n";
		return 8;
	}

	engine.setPosition(position);

	// Get user's pieces color
	bool is_engine_turn = false;
	char p = 0;
	while (p != 'b' && p != 'w') {
		cout << "Enter w to play as white and b to play as black: ";
		std::cin >> p;
		if (std::cin.eof()) return 0;

		p = std::tolower(p);
		bool is_engine_white = (p == 'b');
		if ((!is_engine_white && !first_to_move.is_white) || (is_engine_white && first_to_move.is_white)) is_engine_turn = true;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	GameOutcome game_outcome = ongoing;
	Moves moves;
	moves.generateMoves(*engine.player, *engine.opponent);

	while (moves.num_moves != 0) {

		cout << "Fen: " << PositionToFEN(*engine.player, *engine.opponent, half_moves, full_moves) << '\n';
		if (!quiet_mode) {
			printBoard(*engine.player, *engine.opponent);
			cout << (((*engine.player).is_white) ? "White to move: " : "Black to move: ");
		}
		unsigned short move = 0;

		if (!is_engine_turn) {
			while (true) {
				std::string move_str;
				std::cin >> move_str;
				if (std::cin.eof()) return 0;

				move = moves.parseMove(move_str);

				if (!move) {
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
			engine.search(false);
			SearchResult search_result = engine.waitSearchResult();

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

		engine.MakeMove(move);

		if (!engine.player->is_white) full_moves++;

		game_outcome = engine.getGameStatus();
		if (game_outcome != ongoing) break;

		moves.generateMoves(*engine.player, *engine.opponent);
	}

	cout << "Fen: " << PositionToFEN(*engine.player, *engine.opponent, half_moves, full_moves) << '\n';
	if (!quiet_mode) {
		printBoard(*engine.player, *engine.opponent);
	}

	if (game_outcome == ongoing) {
		if (engine.player->bitboards.king & engine.opponent->bitboards.attacks) {	// If Player's king is attacked
			game_outcome = checkmate;
		}
		else {
			game_outcome = stalemate;
		}
	}

	switch (game_outcome) {
	case checkmate:
		cout << (engine.player->is_white ? "Black wins by checkmate!\n" : "White wins by checkmate!\n");
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
	cout << "Fen: " << PositionToFEN(*engine.player, *engine.opponent, 0, 1);
}
