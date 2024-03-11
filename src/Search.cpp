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

unsigned short FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves, 
						    const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	unsigned short best_move = 0;
	double best_move_eval = worst_eval;
	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.last_hash();

	for (const unsigned short move : moves) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);

		double eval = -Search(depth - 1, INT_MIN + 1, INT_MAX, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys);
		
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		if (eval > best_move_eval) {
			best_move_eval = eval;
			best_move = move;
		}
	}

	positions.unbranch(branch_id, start);

	return best_move;
}


double Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions,
			  int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	
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

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.last_hash();

	for (const unsigned short move : moves) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);

		// Make move does not update player attack bitboard, which is used in Evaluate, update when next call of Search will have a depth of 0
		if (depth == 1) {
			player.bitboards.attacks = generateAttacksBitBoard(player.is_white, player.locations, player.bitboards.all_pieces, player.num_pawns, 
															   player.num_knights, player.num_bishops, player.num_rooks, player.num_queens, magic_bitboards);
		}

		double eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys);

		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		if (eval >= beta) {
			return beta;
		}
		if (eval > alpha) {
			alpha = eval;
		}
	}

	positions.unbranch(branch_id, start);

	return alpha;
}
