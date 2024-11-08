#include "Player.h"
#include "Position.h"
#include "MagicBitboards.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <iostream>

int main() {
	magic_bitboards = MagicBitboards();
	bool loaded = magic_bitboards.loadMagicBitboards();

	const Position position = FENToPosition("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
	nnue.setPosition(position.player, position.opponent);
	int evaluation = nnue.evaluate();
	std::cout << (evaluation) << '\n';
}
