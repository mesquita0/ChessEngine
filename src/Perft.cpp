#include "Perft.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Locations.h"
#include "Player.h"
#include "Position.h"
#include "Zobrist.h"
#include <iostream>

static unsigned long long Perftr(int depth, Player& player, Player& opponent, Moves& moves);

unsigned long long Perft(int depth, Player& player, Player& opponent) {
	/*
	Perftr won't revert back changes in attacks and squares_to_uncheck bitboards, which is problematic if it
	is called multiples times on the same position, since it won't leave the position as it was before, this
	function is only to restore those bitboards to what they were before so that position is not changed at all
	in a perft call and, therefore, can be called multiple times on the same position without problems.
	*/
	Moves moves;

	unsigned long long attacks = opponent.bitboards.attacks;
	unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

	unsigned long long results = Perftr(depth, player, opponent, moves);

	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	return results;
}

static unsigned long long Perftr(int depth, Player& player, Player& opponent, Moves& moves) {
	if (depth == 0) return 1;
	
	moves.generateMoves(player, opponent);

	// Bulk counting
	if (depth == 1) return moves.num_moves;

	Moves new_moves;
	unsigned long long nodes = 0;

	for (const unsigned short move : moves) {
		/*
			unmake move does not reverse changes in attacks / squares to uncheck bitboards, since they are going to 
			be recomputed when the next move is made (since the board will change), and the moves for the current position
			are alredy computed.
		*/

		MoveInfo move_info = makeMove(move, player, opponent, 0);
		nodes += Perftr(depth - 1, opponent, player, new_moves);
		unmakeMove(move, player, opponent, move_info);
	}

	return nodes;
}

unsigned long long PerftDivide(int depth, Player& player, Player& opponent) {
	unsigned long long nodes = 0;

	unsigned long long attacks = opponent.bitboards.attacks;
	unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

	Moves moves;
	moves.generateMoves(player, opponent);

	for (const unsigned short move : moves) {
		std::cout << locationToNotationSquare((move >> 6) & 0x3f) << locationToNotationSquare(move & 0x3f);
		MoveInfo move_info = makeMove(move, player, opponent, 0);
		unsigned long long nodes_move = Perft(depth - 1, opponent, player);
		nodes += nodes_move;
		std::cout << ": " << nodes_move << '\n';
		unmakeMove(move, player, opponent, move_info);
	}

	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	return nodes;
}
