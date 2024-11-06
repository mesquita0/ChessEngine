#pragma once
#include "Locations.h"
#include "Player.h"
#include "PieceTypes.h"
#include <vector>

/*
	Inputs of NN (example white to move):
		(side to move)
		-> Ka1, Pa1
		-> Ka1, Pb1
		...
		-> Ka1, Ph8
		-> Ka1, pa1
		-> Ka1, pb1
		...
		-> Ka1, qh8
		-> Kb1, Pa1
		...
		-> Kh8, qh8

		(side not to move, flip colors and board vertically)
		-> ka8, pa8
		-> ka8, pb8
		...
		-> ka8, Qh1
		-> kb8, pa8
		...
		-> kh1, Qh1
*/

struct NNUEIndex {
	int index_sm, index_snm;
};

inline int flip_square(int square) {
	/*
		* Flip square using:
			old_rank = (square - file) / 8, new_rank = 7 - old_rank,
			flipped_square = 8*new_rank - file
			=> 56 - square + 2 * file.
	*/
	int file = square % 8;
	return (56 - square + 2 * file);
}

NNUEIndex getIndexNNUE(location square, PieceType piece_type, const Player& player, const Player& opponent);
std::vector<NNUEIndex> getIndexesNNUE(const Player& player, const Player& opponent);
