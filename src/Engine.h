#pragma once
#include "GameOutcomes.h"
#include "Position.h"
#include "Search.h"
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

public:

	Player *player, *opponent;

	Engine();
	
	bool isReady() const;

	void setPosition(const Position& position);
	void MakeMove(unsigned short move);

	void search();
	void stop() const;

	SearchResult waitSearchResult();
};
