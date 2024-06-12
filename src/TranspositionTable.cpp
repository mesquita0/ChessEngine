#include "TranspositionTable.h"
#include "MakeMoves.h"
#include "Moves.h"
#include <bit>
#include <cstdint>

void Bucket::updateSmallestDepth() {
	uint8_t smallest_depth = 0xff;
	for (int i = 0; i < bucket_size; i++) {
		if (bucket[i].depth < smallest_depth) {
			smallest_depth = bucket[i].depth;
			index_smallest_depth = i;
		}
	}
}

TranspositionTable::TranspositionTable(size_t size_mb, uint64_t all_pieces) {
	size_t max_entries = size_mb * 1024 * 1024 / sizeof(Bucket);
	table.resize(max_entries);
	index_mask = max_entries - 1; // max_entries is a power of two (...0010000...) so it becomes ...0001111...

	num_pieces_root = std::popcount(all_pieces);
}

void TranspositionTable::store(uint64_t hash, unsigned short best_move, uint8_t depth, nodeFlag node_flag, int16_t eval, uint8_t num_pieces, Entry* pos_entry) {

	// Avoid storing the same position multiple times
	if (pos_entry) {
		if (pos_entry->depth < depth || (pos_entry->depth == depth && pos_entry->node_flag != Exact && pos_entry->node_flag == Exact)) {
			*pos_entry = Entry{ unsigned(hash >> 32), best_move, depth, node_flag, eval, current_generation, num_pieces };
		}
		return;
	}

	int index = hash & index_mask;
	Bucket& bucket = table[index];

	// Populate free first space if any
	if (bucket.index_free < bucket_size) {
		bucket[bucket.index_free] = Entry{ unsigned(hash >> 32), best_move, depth, node_flag, eval, current_generation, num_pieces };
		if (++bucket.index_free == bucket_size) 
			bucket.updateSmallestDepth();

		return;
	}
	
	// Check if any positions from previous searches (different root nodes) are unreachable
	if (bucket.last_gen_fully_checked != current_generation) {
		for (Entry& entry : bucket) {
			if (entry.num_pieces > num_pieces_root) { // If the position stored has more pieces than current root position it will never be reached
				entry = Entry{ unsigned(hash >> 32), best_move, depth, node_flag, eval, current_generation, num_pieces };
				return;
			}
	
			if (entry.generation_last_used <= current_generation - 2) { // Replace if wasn't used in the last move calculation, likely to be unreachable
				entry = Entry{ unsigned(hash >> 32), best_move, depth, node_flag, eval, current_generation, num_pieces };
				return;
			}
		}
		bucket.last_gen_fully_checked = current_generation;
	}
	
	// Always replace entry with smallest depth
	uint8_t prev_smallest_depth = bucket[bucket.index_smallest_depth].depth;
	bucket[bucket.index_smallest_depth] = Entry{ unsigned(hash >> 32), best_move, depth, node_flag, eval, current_generation, num_pieces };
	if (depth > prev_smallest_depth) 
		bucket.updateSmallestDepth();
}

Entry* TranspositionTable::get(uint64_t hash, uint32_t num_pieces, const Moves& moves) {
	uint32_t index = hash & index_mask;
	uint32_t upper_bits_hash = hash >> 32;
	for (Entry& entry : table[index]) {
		if (entry.hash == upper_bits_hash && entry.num_pieces == num_pieces && moves.isMoveLegal(entry.best_move)) {
			entry.generation_last_used = current_generation;
			return &entry;
		}
	}
	return nullptr;
}

void TranspositionTable::updateMoveRoot(short capture_flag, short move_flag) {
	current_generation++; 
	if (capture_flag != no_capture || move_flag == en_passant) 
		num_pieces_root--; 
}
