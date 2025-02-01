#include "Player.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "EvaluateNNUE.h"
#include "Evaluate.h"
#include <iostream>

int main() {
	magic_bitboards = MagicBitboards();
	bool loaded = magic_bitboards.loadMagicBitboards();

	if (!nnue.is_loaded()) {
		std::cout << "Couldn't load weights of NNUE.";
		return 10;
	}

	Position position = FENToPosition("2k1r3/1pp3pp/5pq1/1pP1pn2/1B6/2P2N1P/3Q1PP1/R4RK1 b - - 0 25");
	nnue.setPosition(position.player1, position.player2);
	int ev_root_position = nnue.evaluate();
	std::cout << "Evaluation NNUE:                 " << (ev_root_position) << '\n';
	std::cout << "Evaluation handcrafted function: " << Evaluate(position.player1, position.player2, std::popcount(position.player1.bitboards.all_pieces)) << '\n';
	
	Moves moves;
	moves.generateMoves(position.player1, position.player2);

	for (const unsigned short move : moves) {
		nnue.setPosition(position.player1, position.player2);

		MoveInfo mv_inf = makeMove(move, position.player1, position.player2, 0);

		int ev = nnue.evaluate();
		nnue.setPosition(position.player2, position.player1);
		int expected_ev = nnue.evaluate();

		if (ev != expected_ev)
			std::cout << "Failed (make move), expected " << expected_ev << ", got " << ev << '\n';

		unmakeMove(move, position.player1, position.player2, mv_inf);

		int new_ev_root_position = nnue.evaluate();
		if (new_ev_root_position != ev_root_position) 
			std::cout << "Failed (unmake move), expected " << ev_root_position << ", got " << new_ev_root_position << '\n';
	}
}
