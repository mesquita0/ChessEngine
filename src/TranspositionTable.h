#pragma once
#include <cstdint>
#include <vector>

enum nodeFlag : uint8_t { Exact, UpperBound, LowerBound };

struct Entry {
	uint64_t hash = 0;
	unsigned short best_move = 0;
	int8_t depth = -1;
	nodeFlag node_flag;
	int32_t eval = 0;
};

class TranspositionTable {
	std::vector<Entry> table;
	uint64_t index_mask;

public:
	// Size in mb, must be a power of two.
	TranspositionTable(size_t size_mb);

	void store(uint64_t hash, unsigned short best_move, int8_t depth, nodeFlag node_flag, int32_t eval);

	Entry* get(uint64_t hash);
};
