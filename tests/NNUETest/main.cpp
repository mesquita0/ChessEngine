#include "Player.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <iostream>

int main() {
	magic_bitboards = MagicBitboards();
	bool loaded = magic_bitboards.loadMagicBitboards();

	Position position = FENToPosition("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
	nnue.setPosition(position.player, position.opponent);
	int ev_root_position = nnue.evaluate();
	std::cout << (ev_root_position) << '\n';
	
	Moves moves;
	moves.generateMoves(position.player, position.opponent);

	for (const unsigned short move : moves) {
		nnue.setPosition(position.player, position.opponent);

		MoveInfo mv_inf = makeMove(move, position.player, position.opponent, 0);

		int ev = nnue.evaluate();
		nnue.setPosition(position.player, position.opponent);
		int expected_ev = nnue.evaluate();

		if (ev != expected_ev) 
			std::cout << "Failed (make move), expected " << expected_ev << ", got " << ev << '\n';

		unmakeMove(move, position.player, position.opponent, mv_inf);
		
		int new_ev_root_position = nnue.evaluate();
		if (new_ev_root_position != ev_root_position) 
			std::cout << "Failed! (unmake move), expected " << ev_root_position << ", got " << new_ev_root_position << '\n';
	}
}
