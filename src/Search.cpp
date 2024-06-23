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
#include <atomic>
#include <bit>
#include <chrono>
#include <climits>
#include <thread>
#include <vector>

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, 
		   int half_moves, int num_pieces, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, 
		   std::vector<std::array<unsigned short, 2>>& killer_moves, TranspositionTable& tt);
int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);

std::atomic_bool timed_out = false;
int search_id = 0; // Only written by main thread, does not need to be atomic

static void stopSearch(int id) {
	if (id == search_id) timed_out = true;
}

SearchResult FindBestMoveItrDeepening(std::chrono::milliseconds time, Player& player, Player& opponent, HashPositions& positions, int half_moves,
									  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt) {
	SearchResult result = { 0, 0, 0 };

	// Set timer to search
	search_id++;
	timed_out = false;
	std::thread timer([time]() {
		std::this_thread::sleep_for(time);
		stopSearch(search_id);
	});
	
	int depth = 1;
	while (result.evaluation != checkmated_eval && result.evaluation != checkmate_eval && !timed_out) {
		SearchResult r = FindBestMove(depth, player, opponent, positions, half_moves, magic_bitboards, zobrist_keys, tt);
		depth++;
		
		// Ignore result if search was canceled imediately, without being able to look at any moves
		if (r.best_move != 0) result = r;
	}

	timer.detach();

	return result;
}


SearchResult FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
									  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt) {
	SearchResult result = { 0, 0, 0 };
	timed_out = false;

	for (int i = 1; i <= depth; i++) {
		result = FindBestMove(i, player, opponent, positions, half_moves, magic_bitboards, zobrist_keys, tt);
		
		if (result.evaluation == checkmated_eval || result.evaluation == checkmate_eval) break;
	}

	return result;
}


SearchResult FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves,
						  const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, TranspositionTable& tt) {

	 int num_pieces = std::popcount(player.bitboards.all_pieces);

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
	Entry* position_tt = tt.get(current_hash, num_pieces, moves);
	if (position_tt && position_tt->depth >= depth) {
		switch (position_tt->node_flag) {
		case Exact:
			return { position_tt->eval, position_tt->best_move, position_tt->depth };

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
		if (timed_out) break;

		unsigned short move_flag = getMoveFlag(move);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);
		int new_num_pieces = num_pieces - ((mv_inf.capture_flag != no_capture || move_flag == en_passant) ? 1 : 0);
		
		int eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, magic_bitboards, zobrist_keys, killer_moves, tt);
		
		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		if (eval > alpha) {
			alpha = eval;
			best_move = move;
		}
	}

	positions.unbranch(branch_id, start);
	nodeFlag nf = timed_out ? LowerBound : Exact;
	tt.store(current_hash, best_move, depth, nf, alpha, num_pieces, position_tt);

	// Restore attacks and squares to uncheck bitboards
	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	// Make returned evaluation positive if white is winning and negative if black is winning
	if (!player.is_white) alpha = -alpha;

	if (timed_out) depth--;

	return { alpha, best_move, (unsigned short)depth };
}


int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, 
		   int half_moves, int num_pieces, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys, 
		   std::vector<std::array<unsigned short, 2>>& killer_moves, TranspositionTable& tt) {

	// Cancel search if timed out
	if (timed_out) return INT_MAX;

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
	Entry* position_tt = tt.get(current_hash, num_pieces, moves);
	if (position_tt && position_tt->depth >= depth) {
		switch (position_tt->node_flag) {
		case Exact:
			return position_tt->eval;

		case UpperBound:
			if (alpha >= position_tt->eval) return position_tt->eval;
			if (beta > position_tt->eval) beta = position_tt->eval;
			break;

		case LowerBound:
			if (position_tt->eval >= beta) return position_tt->eval;
			if (alpha < position_tt->eval) {
				alpha = position_tt->eval;
				best_move = position_tt->best_move;
			}
			break;
		}
	}

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();

	moves.orderMoves(player, opponent, position_tt, &killer_moves[depth]);
	unsigned short move;
	int best_eval = INT_MIN + 1;
	bool pv_search = true;

	while (move = moves.getNextOrderedMove()) {
		if (timed_out) break;

		unsigned short move_flag = getMoveFlag(move);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash, magic_bitboards, zobrist_keys);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);
		int new_num_pieces = num_pieces - ((mv_inf.capture_flag == no_capture || move_flag == en_passant) ? 0 : 1);

		// PV Search
		int eval;
		if (pv_search)
			eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, magic_bitboards, zobrist_keys, killer_moves, tt);
		else {
			eval = -Search(depth - 1, -alpha-1, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, magic_bitboards, zobrist_keys, killer_moves, tt);
			
			if (eval > alpha && eval < beta) { // re-search
				AttacksInfo player_attacks = generateAttacksInfo(player.is_white, player.bitboards, player.bitboards.all_pieces,
																 player.locations.king, opponent.locations.king, magic_bitboards);

				player.bitboards.attacks = player_attacks.attacks_bitboard;
				opponent.bitboards.squares_to_uncheck = player_attacks.opponent_squares_to_uncheck;

				eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, magic_bitboards, zobrist_keys, killer_moves, tt);
			}
		}

		unmakeMove(move, player, opponent, mv_inf, magic_bitboards);
		positions.clear();
		positions.start = start;

		// Fail high
		if (eval >= beta) {
			best_eval = eval;
			best_move = move;

			// Killer moves
			if (!isCapture(move, opponent.bitboards.friendly_pieces) && killer_moves[depth][0] != move) {
				killer_moves[depth][1] = killer_moves[depth][0];
				killer_moves[depth][0] = move;
			}
			
			break;
		}

		if (eval > best_eval) {
			best_eval = eval;
			best_move = move;
			if (eval > alpha) alpha = eval;
		}

		// Search only the first move with an open window
		if (depth > 4) pv_search = false;
	}

	positions.unbranch(branch_id, start);

	// Ignore result of the search if couldnt complete search and failed low, since we cant draw any conclusions from that
	if (best_eval <= alpha && timed_out) {
		return INT_MAX;
	}

	nodeFlag nf;
	if (best_eval <= alpha) nf = UpperBound; // Fail Low
	else if (timed_out || best_eval >= beta) nf = LowerBound; // Timed out or Fail High
	else nf = Exact;

	tt.store(current_hash, best_move, depth, nf, best_eval, num_pieces, position_tt);
	
	return best_eval;
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
