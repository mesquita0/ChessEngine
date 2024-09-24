#pragma once
#include "Player.h"
#include <algorithm>
#include <vector>

enum GameOutcome { ongoing, checkmate, stalemate, draw_by_insufficient_material, draw_by_repetition, draw_by_50_move_rule };

struct HashPositions {
private:
	std::vector<unsigned long long> positions;

public:
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

	inline int numPositions() const { return positions.size() - start; }
	inline unsigned long long lastHash() const { return positions.back(); }

	inline unsigned long long operator[] (int i) const { return positions[i + start]; }
	inline unsigned long long& operator[] (int i) { return positions[i + start]; }

	inline bool contains(unsigned long long hash) const { return std::find(positions.begin(), positions.end(), hash) != positions.end(); }

	void clear();
};

GameOutcome getGameOutcome(const Player& player, const Player& opponent, const HashPositions& positions, int half_moves);
