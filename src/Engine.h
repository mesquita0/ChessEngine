#pragma once
#include "GameOutcomes.h"
#include "Position.h"
#include "Search.h"
#include <chrono>
#include <thread>

constexpr size_t tt_size_mb = 256; // must be a power of two

class Engine {
	bool loaded = false;
	int search_id = 0;

	unsigned long long hash;
	Position position;
	HashPositions hash_positions = HashPositions(0);

	SearchResult search_result;

	std::thread searcher;

	unsigned time_white = 0, time_black = 0;

	int fixed_depth;
	std::chrono::milliseconds search_time;
	bool infinite_search;

public:
	Player *player, *opponent;

	Engine();
	
	bool didLoad() const;

	void setPosition(const Position& position);
	void MakeMove(unsigned short move);

	void search(bool print_best_move=true);
	void stop() const;

	SearchResult waitSearchResult();
	
	GameOutcome getGameStatus();

	void setTimeWhite(int time_remaining_white);
	void setTimeBlack(int time_remaining_black);

	void setDepth(int depth) { fixed_depth = depth; infinite_search = false; }
	void setSearchTime(int search_time) { this->search_time = std::chrono::milliseconds(search_time); infinite_search = false; }
	void setInfiniteSearch() { infinite_search = true; fixed_depth = 0; }
};
