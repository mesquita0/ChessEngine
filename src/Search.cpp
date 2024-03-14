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

constexpr int min_eval_checkmate_player = INT_MAX - 1000;

int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, int depth_searched);

unsigned short FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves, 
						    const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	unsigned short best_move = 0;
	int best_move_eval = INT_MIN + 1;
	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.last_hash();
	
	moves.orderMoves(player, opponent);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);

		int eval = -Search(depth - 1, INT_MIN + 1, -best_move_eval, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys, 1);
		
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

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions,
		   int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, int depth_searched) {
	
	// Check draws
 	GameOutcome game_outcome = getGameOutcome(player, opponent, positions, half_moves);
	if (game_outcome != ongoing) return 0;

	// Search only captures when desired depth is reached
	if (depth == 0) {
		return quiescenceSearch(alpha, beta, player, opponent, magic_bitboards, zobrist_keys, depth_searched);
	}

	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	// Check Checkamte or Stalemate
	if (moves.num_moves == 0) {
		if (opponent.bitboards.attacks & player.bitboards.king) { // Checkmate
			return checkmated_eval + depth_searched; // Bigger the depth searched, slower the mate is (better for player)
		}
		else { // Stalemate
			return 0;
		}
	}

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.last_hash();
	depth_searched++;

	moves.orderMoves(player, opponent);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		unsigned short move_flag = (move & 0xf000);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);

		int eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys, depth_searched);

		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		if (eval >= beta) {
			positions.unbranch(branch_id, start);
			return beta;
		}
		if (eval > alpha) alpha = eval;
		if (alpha > min_eval_checkmate_player) break; // Break if checkmate is guaranteed for player
	}

	positions.unbranch(branch_id, start);

	return alpha;
}

int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, int depth_searched) {
	Moves moves;

	// Generates captures and updates player attacks bitboard, does not include king attacks to squares 
	// that are defedend or attacks of pinned pieces that would leave the king in check if played
	moves.generateCaptures(player, opponent, magic_bitboards);

	// Low bound on evaluation, since almost always making a move is better than doing nothing
	int standing_eval = Evaluate(player, opponent, magic_bitboards, depth_searched); 
	if (standing_eval >= beta) return beta;
	if (standing_eval > alpha ) alpha = standing_eval;
	if (alpha > min_eval_checkmate_player) return alpha;
	depth_searched++;

	moves.orderMoves(player, opponent);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		MoveInfo mv_inf = makeMove(move, player, opponent, 0, magic_bitboards, zobrist_keys);
		int eval = -quiescenceSearch(-beta, -alpha, opponent, player, magic_bitboards, zobrist_keys, depth_searched);
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);

		if (eval >= beta) return beta;
		if (eval > alpha) alpha = eval;
		if (alpha > min_eval_checkmate_player) break; // Break if checkmate is guaranteed for player
	}

	return alpha;
}
