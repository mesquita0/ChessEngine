#include "TranspositionTable.h"
#include <cstdint>

TranspositionTable::TranspositionTable(size_t size_mb) {
	size_t max_entries = size_mb * 1024 * 1024 / sizeof(Entry);
	table.resize(max_entries);
	index_mask = max_entries - 1; // max_entries is a power of two (...0010000...) so it becomes ...0001111...
}

void TranspositionTable::store(uint64_t hash, unsigned short best_move, int8_t depth, nodeFlag node_flag, int32_t eval) {
	int index = hash & index_mask;
	if (table[index].depth <= depth) {
		table[index] = Entry{ hash, best_move, depth, node_flag, eval };
	}
}

Entry* TranspositionTable::get(uint64_t hash) {
	int index = hash & index_mask;
	if (table[index].hash == hash) return &table[index]; 
	return nullptr;
}
