#pragma once
#include "Locations.h"

enum PieceType { Pawn, Knight, Bishop, Rook, Queen, King};

struct HistoryTable
{
private:
	unsigned int table[2][6][64] = {};
	unsigned int max_value = 0;

public:
	// Return relative number of times a move is in the history table, values range from 0 to 1.
	inline double get(bool is_white, int piece_type, location final_square) const { return static_cast<double>(table[is_white][piece_type][final_square]) / max_value; }

	// Records the depth at which the move caused a cutoff.
	inline void record(bool is_white, int piece_type, location final_square, int depth) {
		table[is_white][piece_type][final_square] =+ depth * depth; 
		if (table[is_white][piece_type][final_square] > max_value) max_value = table[is_white][piece_type][final_square];
	}

	// Records the depth at which the move caused a cutoff.
	void record(bool is_white, unsigned short move_flag, location final_square, int depth);
};

inline HistoryTable history_table; // Global history table
