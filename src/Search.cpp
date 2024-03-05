#include "Search.h"
#include "Evaluate.h"
#include "GameOutcomes.h"
#include "Locations.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Player.h"
#include "Zobrist.h"
#include <climits>

constexpr double worst_eval = -100000000;

unsigned short FindBestMove(int depth, Player& player, Player& opponent, std::vector<unsigned long long>& positions, int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	unsigned short best_move = 0;
	double best_move_eval = worst_eval;
	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	unsigned long long hash = (positions.size() != 0) ? positions.back() : 0;
	for (const unsigned short move : moves) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, hash, magic_bitboards, zobrist_keys);
		auto pos = positions;
		int new_half_moves = updatePositions(positions, mv_inf.capture_flag, move_flag, half_moves);

		double eval = -Search(depth - 1, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys);

		positions = pos;
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);

		if (eval > best_move_eval) {
			best_move_eval = eval;
			best_move = move;
		}
	}

	return best_move;
}

double Search(int depth, Player& player, Player& opponent, std::vector<unsigned long long>& positions, int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	
	// Check draws
 	GameOutcome game_outcome = getGameOutcome(player, opponent, positions, half_moves);
	if (game_outcome != ongoing) return 0;

	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	// Check Checkamte or Stalemate
	if (moves.num_moves == 0) {
		if (opponent.bitboards.attacks & player.bitboards.king) { // Checkmate
			return worst_eval + 100 - depth; // Bigger the depth, faster the mate is
		}
		else { // Stalemate
			return 0;
		}
	}

	if (depth == 0) {
		return -Evaluate(opponent, player);
	}

	// TODO: order moves

	double best_eval = worst_eval;
	unsigned long long hash = (positions.size() != 0) ? positions.back() : 0;
	for (const unsigned short move : moves) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, hash, magic_bitboards, zobrist_keys);
		auto pos = positions;
		int new_half_moves = updatePositions(positions, mv_inf.capture_flag, move_flag, half_moves);

		double eval = -Search(depth - 1, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys);

		positions = pos;
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);

		if (eval > best_eval) {
			best_eval = eval;
		}
	}

	return best_eval;
}
