#include "Search.h"
#include "Evaluate.h"
#include "GameOutcomes.h"
#include "Locations.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Player.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include <array>
#include <climits>
#include <vector>

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, 
		   int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, 
		   std::vector<std::array<unsigned short, 2>>& killer_moves, TranspositionTable& tt);
int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);


SearchResult FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
									  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt) {
	SearchResult result = { 0, 0 };

	for (int i = 1; i <= depth; i++) {
		result = FindBestMove(i, player, opponent, positions, half_moves, magic_bitboards, zobrist_keys, tt);
		if (result.evaluation == checkmated_eval || result.evaluation == checkmate_eval) {
			// TODO: Store checkmate in x moves do be displayed in evaluation
			break;
		}
	}

	return result;
}

SearchResult FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
						  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt) {

	/*
	UnmakeMove (and Search) won't revert back changes in attacks and squares_to_uncheck bitboards, since they are going
	to be recomputed when the next move is made (since the board will change), but that is problematic if FindBestMove
	is called multiples times on the same position (as it is in FindBestMoveItrDeepening), since it won't leave the
	position as it was before, therefore we need to backup attacks and squares to uncheck bitboards manually to make
	sure this function can be called multiple times on the same position without problems.
	*/
	unsigned long long attacks = opponent.bitboards.attacks;
	unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

	unsigned short best_move = 0;
	int alpha = INT_MIN + 1;
	int beta = INT_MAX;
	std::vector<std::array<unsigned short, 2>> killer_moves(depth);

	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.last_hash();
	
	// Lookup transposition table from previous searches
	Entry* position_tt = tt.get(current_hash);
	if (position_tt && position_tt->depth >= depth && moves.isMoveLegal(position_tt->best_move)) {
		switch (position_tt->node_flag) {
		case Exact:
			return { position_tt->eval, position_tt->best_move };

		case UpperBound:
			beta = position_tt->eval;
			break;

		case LowerBound:
			alpha = position_tt->eval;
			best_move = position_tt->best_move;
			break;
		}
	}

	moves.orderMoves(player, opponent, position_tt, nullptr);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		unsigned short move_flag = getMoveFlag(move);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);
		
		int eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys, killer_moves, tt);
		
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		if (eval > alpha) {
			alpha = eval;
			best_move = move;
		}
	}

	positions.unbranch(branch_id, start);
	tt.store(current_hash, best_move, depth, Exact, alpha);

	// Restore attacks and squares to uncheck bitboards
	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	// Make returned evaluation be positive if white is winning and negative if black is winning
	if (!player.is_white) alpha = -alpha;

	return { alpha, best_move };
}

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, 
		   int half_moves, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, 
		   std::vector<std::array<unsigned short, 2>>& killer_moves, TranspositionTable& tt) {

	// Check draws
 	GameOutcome game_outcome = getGameOutcome(player, opponent, positions, half_moves);
	if (game_outcome != ongoing) return 0;

	// Search only captures when desired depth is reached
	if (depth == 0) {
		return quiescenceSearch(alpha, beta, player, opponent, magic_bitboards, zobrist_keys);
	}

	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	// Check Checkamte or Stalemate
	if (moves.num_moves == 0) {
		if (opponent.bitboards.attacks & player.bitboards.king) { // Checkmate
			return checkmated_eval;
		}
		else { // Stalemate
			return 0;
		}
	}

	// Lookup transposition table from previous searches
	unsigned long long current_hash = positions.last_hash();
	unsigned short best_move = 0;
	bool have_low_bound = false;
	Entry* position_tt = tt.get(current_hash);
	if (position_tt && position_tt->depth >= depth && moves.isMoveLegal(position_tt->best_move)) {
		switch (position_tt->node_flag) {
		case Exact:
			return position_tt->eval;

		case UpperBound:
			if (beta > position_tt->eval) beta = position_tt->eval;
			break;

		case LowerBound:
			if (alpha < position_tt->eval) {
				alpha = position_tt->eval;
				best_move = position_tt->best_move;
				have_low_bound = true;
			}
			break;
		}

		if (alpha >= beta) return beta;
	}

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();

	moves.orderMoves(player, opponent, position_tt, &killer_moves[depth]);
	unsigned short move;
	int fail_low_best_eval = INT_MIN + 1;
	bool did_fail_low = true;

	while (move = moves.getNextOrderedMove()) {
		unsigned short move_flag = getMoveFlag(move);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);

		int eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, magic_bitboards, zobrist_keys, killer_moves, tt);

		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		// Fail high
		if (eval >= beta) {
			positions.unbranch(branch_id, start);
			tt.store( current_hash, move, depth, LowerBound, eval );

			// Killer moves
			if (!isCapture(move, opponent.bitboards.friendly_pieces) && killer_moves[depth][0] != move) {
				killer_moves[depth][1] = killer_moves[depth][0];
				killer_moves[depth][0] = move;
			}
			
			return beta;
		}
		if (eval > alpha) {
			alpha = eval;
			best_move = move;
			did_fail_low = false;
		} 
		else if (eval > fail_low_best_eval && did_fail_low && !have_low_bound) {
			/*
				If fails low but has a low bound from previous searches, best move should remain the one from TT instead of the best move from 
				current search, since that can only happen if the result from the TT was searched at a higher depth than current search.
			*/
			fail_low_best_eval = eval;
			best_move = move;
		}
	}

	positions.unbranch(branch_id, start);

	if (did_fail_low) {
		tt.store(current_hash, best_move, depth, UpperBound, fail_low_best_eval);
	}
	else {
		tt.store(current_hash, best_move, depth, Exact, alpha);
	}
	
	return alpha;
}

int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys) {
	Moves moves;

	// Generates captures and updates player attacks bitboard, does not include king attacks to squares 
	// that are defedend or attacks of pinned pieces that would leave the king in check if played
	moves.generateCaptures(player, opponent, magic_bitboards);

	// Low bound on evaluation, since almost always making a move is better than doing nothing
	int standing_eval = Evaluate(player, opponent, magic_bitboards); 
	if (standing_eval >= beta) return beta;
	if (standing_eval > alpha ) alpha = standing_eval;

	moves.orderMoves(player, opponent, nullptr, nullptr);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		MoveInfo mv_inf = makeMove(move, player, opponent, 0, magic_bitboards, zobrist_keys);
		int eval = -quiescenceSearch(-beta, -alpha, opponent, player, magic_bitboards, zobrist_keys);
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);

		if (eval >= beta) return beta;
		if (eval > alpha) alpha = eval;
	}

	return alpha;
}
