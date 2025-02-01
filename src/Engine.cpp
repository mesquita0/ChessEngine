#include "Engine.h"
#include "GameOutcomes.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Position.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <iostream>
#include <thread>

Engine::Engine() {
	tt = TranspositionTable(tt_size_mb);
	bool nnue_loaded = nnue.is_loaded();
	bool magic_bitboards_loaded = magic_bitboards.loadMagicBitboards();

	loaded = nnue_loaded && magic_bitboards_loaded;
}

bool Engine::didLoad() const {
	return loaded;
}

void Engine::setPosition(const Position& position) {
	this->position = position;

	tt.setRoot(position.player1.bitboards.all_pieces);

	hash = zobrist_keys.positionToHash(position.player1, position.player2);

	hash_positions = HashPositions(hash);

	player   = &this->position.player1;
	opponent = &this->position.player2;
}

void Engine::MakeMove(unsigned short move) {
	MoveInfo move_info = makeMove(move, *player, *opponent, hash);
	
	hash = move_info.hash;

	position.half_moves = hash_positions.updatePositions(move_info.capture_flag, getMoveFlag(move), hash, position.half_moves);

	tt.setRoot(player->bitboards.all_pieces);

	Player* tmp = player;
	player = opponent;
	opponent = tmp;
}

void Engine::search(bool print_best_move) {
	if (searcher.joinable()) searcher.join();

	search_id++;
	searcher = std::thread([&, print_best_move]() {
		if (infinite_search)
			FindBestMoveItrDeepening(9999, *player, *opponent, hash_positions, position.half_moves, search_result);
		else if (fixed_depth > 0)
			FindBestMoveItrDeepening(fixed_depth, *player, *opponent, hash_positions, position.half_moves, search_result);
		else
			FindBestMoveItrDeepening(search_time, *player, *opponent, hash_positions, position.half_moves, search_result);

		if (print_best_move) {
			std::cout << "bestmove " << moveToStr(search_result.best_move);
			if (search_result.ponder) std::cout << " ponder " << moveToStr(search_result.ponder);
			std::cout << std::endl;
		}
	});
}

void Engine::stop() const {
	stopSearch(search_id);
}

SearchResult Engine::waitSearchResult() {
	searcher.join();
	return search_result;
}

GameOutcome Engine::getGameStatus() {
	return getGameOutcome(*player, *opponent, hash_positions, position.half_moves);
}

void Engine::setTimeWhite(int time_remaining_white) {
	if (player->is_white) {
		if (position.full_moves < 10)
			setSearchTime(time_remaining_white / 60);
		else if (position.full_moves < 30)
			setSearchTime(time_remaining_white / 10);
		else
			setSearchTime(time_remaining_white / 20);
	}
}

void Engine::setTimeBlack(int time_remaining_black) {
	if (!player->is_white) {
		if (position.full_moves < 10)
			setSearchTime(time_remaining_black / 60);
		else if (position.full_moves < 30)
			setSearchTime(time_remaining_black / 10);
		else
			setSearchTime(time_remaining_black / 20);
	}
}
