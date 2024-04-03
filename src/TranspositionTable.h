#pragma once
#include <array>
#include <cstdint>
#include <vector>

class Moves;

enum nodeFlag : uint8_t { Invalid, Exact, UpperBound, LowerBound };
constexpr int bucket_size = 5;

struct Entry {
	uint32_t hash = 0;
	unsigned short best_move = 0;
	uint8_t depth = 0;
	nodeFlag node_flag = Invalid;
	int16_t eval = 0;
	uint8_t generation_last_used = 0;
	uint8_t num_pieces = 0;
};

struct Bucket {
private:
	std::array<Entry, 5> bucket;

public:
	uint8_t index_free = 0;
	uint8_t last_gen_fully_checked = 0;
	uint8_t index_smallest_depth = 0;

	inline Entry* begin() { return &this->bucket[0]; }
	inline const Entry* begin() const { return &this->bucket[0]; }
	inline Entry* end() { return &this->bucket[0] + index_free; }
	inline const Entry* end() const { return &this->bucket[0] + index_free; }

	inline Entry operator[] (int i) const { return bucket[i]; }
	inline Entry& operator[] (int i) { return bucket[i]; }
	
	void updateSmallestDepth();
};

class TranspositionTable {
	std::vector<Bucket> table;
	uint64_t index_mask;
	uint8_t num_pieces_root;
	uint8_t current_generation = 0;

public:
	// Size in mb, must be a power of two.
	TranspositionTable(size_t size_mb, uint64_t all_pieces);

	// pos_entry should be the pointer returned in get, or nullptr if get wasn't used.
	void store(uint64_t hash, unsigned short best_move, uint8_t depth, nodeFlag node_flag, int16_t eval, uint8_t num_pieces, Entry* pos_entry);

	Entry* get(uint64_t hash, uint32_t num_pieces, const Moves& moves);

	void updateMoveRoot(short capture_flag, short move_flag);
};
