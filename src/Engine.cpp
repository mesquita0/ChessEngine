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
	zobrist_keys = ZobristKeys();
	tt = TranspositionTable(tt_size_mb);
	bool nnue_loaded = nnue.is_loaded();
	bool magic_bitboards_loaded = magic_bitboards.loadMagicBitboards();

	loaded = nnue_loaded && magic_bitboards_loaded;
}

bool Engine::isReady() const {
	return loaded;
}

void Engine::setPosition(const Position& position) {
	this->position = position;

	tt.setRoot(position.player1.bitboards.all_pieces);

	zobrist_keys.setRoot(position.player1, position.player2);
	hash = zobrist_keys.hash;

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

void Engine::search() {
	if (searcher.joinable()) searcher.join();

	search_id++;
	searcher = std::thread([&]() {
		FindBestMoveItrDeepening(60, *player, *opponent, hash_positions, position.half_moves, search_result);
		std::cout << "bestmove ";

		unsigned short move = search_result.best_move;
		std::cout << locationToNotationSquare((move >> 6) & 0b111111);
		std::cout << locationToNotationSquare(move & 0b111111);
		if (isPromotion(move)) {
			unsigned short promotion_flag = getMoveFlag(move);

			switch (promotion_flag) {
			case promotion_knight:
				std::cout << 'n';
				break;
			case promotion_bishop:
				std::cout << 'b';
				break;
			case promotion_rook:
				std::cout << 'r';
				break;
			case promotion_queen:
				std::cout << 'q';
				break;
			}
		}

		std::cout << std::endl;
	});
}

void Engine::stop() const {
	stopSearch(search_id);
}

SearchResult Engine::waitSearchResult() {
	searcher.join();
	return search_result;
}
