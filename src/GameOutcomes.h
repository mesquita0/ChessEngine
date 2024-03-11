#pragma once
#include "Player.h"
#include <vector>

enum GameOutcome { ongoing, checkmate, stalemate, draw_by_insufficient_material, draw_by_repetition, draw_by_50_move_rule };

struct HashPositions {
	std::vector<unsigned long long> positions;
	int branch_id = 0, start = 0;

	HashPositions(unsigned long long initial_hash);

	// Updates positions vector and returns new number of half moves
	int updatePositions(const short capture_flag, const unsigned short move_flag, const unsigned long long new_hash, int half_moves);

	/* 
	Call branch every time going to a new search, 
	will protected every element in the vector before 
	the branching from being deleted.
	*/ 
	inline void branch() { branch_id = positions.size(); }
	void unbranch(int previous_branch_id, int previous_start);

	inline int num_positions() const { return positions.size() - start; }
	void clear();
	inline unsigned long long last_hash() const { return positions.back(); }
};

GameOutcome getGameOutcome(const Player& player, const Player& opponent, const HashPositions& positions, int half_moves);
	