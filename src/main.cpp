#include "BitBoards.h"
#include "Engine.h"
#include "GameMode.h"
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

constexpr bool enable_game_mode = true;

using std::cout;

int main(int argc, char* argv[]) {

	Engine engine = Engine();

	if (!engine.didLoad()) {
		cout << "Engine could not load properly.\n";
		return 1;
	}

	if (enable_game_mode) {
		Flags game_mode_flags = process_flags(argc, argv);

		if (game_mode_flags.show_help) return 0;

		if (game_mode_flags.error_code) return game_mode_flags.error_code;

		if (game_mode_flags.game_mode) return game_mode(engine, game_mode_flags);
	}

	const std::string initial_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	const Position start_position = FENToPosition(initial_FEN);
	engine.setPosition(start_position);

	std::string command, ln;

	while (command != "quit") {
		command = "";
		std::getline(std::cin, ln);
		if (std::cin.eof()) return 0;

		std::stringstream line(ln);
		line >> command;

		if (command == "uci") cout << "uciok" << '\n';

		else if (command == "isready") cout << "readyok\n";

		else if (command == "ucinewgame") {}

		else if (command == "position") {
			Position position = start_position;
			std::string buffer;
			line >> buffer;

			if (buffer == "fen") {
				line >> buffer;
				position = FENToPosition(buffer);
			}
			else if (buffer == "startpos") {
				position = start_position;
			}

			engine.setPosition(position);

			line >> buffer;

			if (buffer == "moves") {
				Moves moves;

				while (line >> buffer) {
					moves.generateMoves(*engine.player, *engine.opponent);
					unsigned short move = moves.parseMove(buffer);

					if (!move) {
						cout << "Invalid sequence of moves.\n";
						break;
					}

					engine.MakeMove(move);
				}
			}
		}

		else if (command == "go") {
			std::string buffer;

			// Search until stop command by default
			engine.setInfiniteSearch();

			while (line >> buffer) {
				if (buffer == "wtime") {
					int time_remaining_white;
					line >> time_remaining_white;
					engine.setTimeWhite(time_remaining_white);
				}
				else if (buffer == "btime") {
					int time_remaining_black;
					line >> time_remaining_black;
					engine.setTimeBlack(time_remaining_black);
				}
				else if (buffer == "depth") {
					int depth;
					line >> depth;
					engine.setDepth(depth);
				}
				else if (buffer == "movetime") {
					int time;
					line >> time;
					engine.setSearchTime(time);
				}
			}

			engine.search();
		}

		else if (command == "stop") {
			engine.stop();
		}

		else if (command != "quit") {
			cout << "Unkown command: " << command << std::endl;
		}
	}
}
