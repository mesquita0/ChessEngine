#include "Search.h"
#include "Evaluate.h"
#include "GameOutcomes.h"
#include "HistoryTable.h"
#include "Locations.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Player.h"
#include "TranspositionTable.h"
#include "Zobrist.h"
#include "EvaluationNetwork/Evaluate/EvaluateNNUE.h"
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <climits>
#include <thread>
#include <vector>

int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, 
		   int half_moves, int num_pieces, std::vector<std::array<unsigned short, 2>>& killer_moves, 
		   bool reduced = false, bool used_null_move = false);
int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, int num_pieces);
bool nullMove(Player& player, Player& opponent);
bool reduceMove(int mv_pos, unsigned short mv, int depth, const MoveInfo& mv_info, const Player& player, 
				const Player& opponent, bool player_in_check, const std::array<unsigned short, 2>& killer_moves_at_ply);

void repetition(Player& player, Player& opponent, const HashPositions& positions);
bool deal_repetition(Player& player, Player& opponent, const HashPositions& positions, unsigned long long hash, unsigned long long repeated_position, Entry* entry);

std::atomic_bool timed_out = false;
int search_id = 0; // Only written by main thread, does not need to be atomic

void stopSearch(int id) {
	if (id == search_id) timed_out = true;
}

void FindBestMoveItrDeepening(std::chrono::milliseconds time, Player& player, Player& opponent, HashPositions& positions, int half_moves, SearchResult& result) {
	result = { 0, 0, 0 };

	// Set timer to search
	search_id++;
	timed_out = false;
	std::thread timer([time]() {
		std::this_thread::sleep_for(time);
		stopSearch(search_id);
	});

	// Check for hallucinations of the engine if a position has alredy been repeated twice and principal variation leads to draw by repetition
	repetition(player, opponent, positions);

	int depth = 1;
	while (result.evaluation != checkmated_eval && result.evaluation != checkmate_eval && !timed_out) {
		SearchResult r = FindBestMove(depth, player, opponent, positions, half_moves);
		depth++;
		
		// Ignore result if search was canceled imediately, without being able to look at any moves
		if (r.best_move != 0) result = r;
	}

	timer.detach();
}


void FindBestMoveItrDeepening(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves, SearchResult& result) {
	result = { 0, 0, 0 };

	search_id++;
	timed_out = false;

	// Check for hallucinations of the engine if a position has alredy been repeated twice and principal variation leads to draw by repetition
	repetition(player, opponent, positions);

	for (int i = 1; i <= depth; i++) {
		SearchResult r = FindBestMove(i, player, opponent, positions, half_moves);
		
		// Ignore result if search was canceled imediately, without being able to look at any moves
		if (r.best_move != 0) result = r;

		if (result.evaluation == checkmated_eval || result.evaluation == checkmate_eval || timed_out) break;
	}
}


SearchResult FindBestMove(int depth, Player& player, Player& opponent, HashPositions& positions, int half_moves) {

	 int num_pieces = std::popcount(player.bitboards.all_pieces);
	 nnue.setPosition(player, opponent);

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
	moves.generateMoves(player, opponent);

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();
	unsigned long long current_hash = positions.lastHash();
	
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

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);
		int new_num_pieces = num_pieces - ((mv_inf.capture_flag != no_capture || move_flag == en_passant) ? 1 : 0);
		
		int eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, killer_moves);
		
		unmakeMove(move, player, opponent, mv_inf);
		positions.clear();
		positions.start = start;

		if (eval > alpha) {
			alpha = eval;
			best_move = move;
		}
	}

	positions.unbranch(branch_id, start);
	nodeFlag nf = timed_out ? LowerBound : Exact;
	if (alpha > INT_MIN + 1)
		tt.store(current_hash, best_move, depth, nf, alpha, num_pieces, position_tt);

	// Restore attacks and squares to uncheck bitboards
	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	// Make returned evaluation positive if white is winning and negative if black is winning
	if (!player.is_white) alpha = -alpha;

	if (timed_out) depth--;

	return { alpha, best_move, (unsigned short)depth };
}


int Search(int depth, int alpha, int beta, Player& player, Player& opponent, HashPositions& positions, int half_moves, 
		   int num_pieces, std::vector<std::array<unsigned short, 2>>& killer_moves, bool reduced, bool used_null_move) {

	// Cancel search if timed out
	if (timed_out) return INT_MAX;

	// Check draws
 	GameOutcome game_outcome = getGameOutcome(player, opponent, positions, half_moves);
	if (game_outcome != ongoing) return 0;

	// Search only captures when desired depth is reached
	if (depth == 0) {
		return quiescenceSearch(alpha, beta, player, opponent, num_pieces);
	}

	Moves moves;
	moves.generateMoves(player, opponent);

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
	unsigned long long current_hash = positions.lastHash();
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

	// Null-Move Pruning
	if (!used_null_move && nullMove(player, opponent) && depth >= 3) {

		MoveInfo mv_inf = makeMove(NULL_MOVE, player, opponent, current_hash);
		int eval = -Search(depth - 3, -beta, -beta + 1, opponent, player, positions, half_moves, num_pieces, killer_moves, reduced, true); // R = 2
		unmakeMove(NULL_MOVE, player, opponent, mv_inf);

		if (eval >= beta) return eval;
	}

	int branch_id = positions.branch_id;
	int start = positions.start;
	positions.branch();

	moves.orderMoves(player, opponent, position_tt, &killer_moves[depth]);
	unsigned short move;
	int best_eval = INT_MIN + 1;
	int mv_pos = 0;
	bool pv_search = true;
	bool player_in_check = (player.bitboards.king & opponent.bitboards.attacks);

	while (move = moves.getNextOrderedMove()) {
		if (timed_out) break;

		unsigned short move_flag = getMoveFlag(move);

		MoveInfo mv_inf = makeMove(move, player, opponent, current_hash);
		int new_half_moves = positions.updatePositions(mv_inf.capture_flag, move_flag, mv_inf.hash, half_moves);
		int new_num_pieces = num_pieces - ((mv_inf.capture_flag == no_capture || move_flag == en_passant) ? 0 : 1);

		// PV Search
		int eval;
		if (pv_search)
			eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, killer_moves, reduced);
		else {
			int d = depth - 1;
			bool reduce_search = false;
			if (!reduced && reduceMove(mv_pos, move, d, mv_inf, player, opponent, player_in_check, killer_moves[depth])) {
				reduce_search = true;
				/*
					First 2 moves: do not reduce
					3rd and 4th move: reduce by 1
					5th to 7th: reduce by 2
					Rest of the moves: reduce by 1/3
				*/
				if (mv_pos <= 3) d -= 1;
				else if (mv_pos <= 6) d -= 2;
				else d /= 3;
			}
			eval = -Search(d, -alpha - 1, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, killer_moves, reduce_search);
			
			if (eval > alpha && eval < beta) { // re-search
				AttacksInfo player_attacks = generateAttacksInfo(player.is_white, player.bitboards, player.bitboards.all_pieces,
																 player.locations.king, opponent.locations.king);

				player.bitboards.attacks = player_attacks.attacks_bitboard;
				opponent.bitboards.squares_to_uncheck = player_attacks.opponent_squares_to_uncheck;

				eval = -Search(depth - 1, -beta, -alpha, opponent, player, positions, new_half_moves, new_num_pieces, killer_moves, reduced);
			}
		}

		unmakeMove(move, player, opponent, mv_inf);
		positions.clear();
		positions.start = start;

		// Fail high
		if (eval >= beta) {
			best_eval = eval;
			best_move = move;

			// Killer moves and History heuristic
			if (!isCapture(move, opponent.bitboards.friendly_pieces) && killer_moves[depth][0] != move) {
				killer_moves[depth][1] = killer_moves[depth][0];
				killer_moves[depth][0] = move;

				history_table.record(player.is_white, move_flag, getFinalSquare(move), depth);
			}
			
			break;
		}

		if (eval > best_eval) {
			best_eval = eval;
			best_move = move;
			if (eval > alpha) alpha = eval;
		}

		mv_pos++;

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

	if (best_eval > alpha || !timed_out) // Don't store score if failed low and timed out
		tt.store(current_hash, best_move, depth, nf, best_eval, num_pieces, position_tt);
	
	return best_eval;
}


int quiescenceSearch(int alpha, int beta, Player& player, Player& opponent, int num_pieces) {
	Moves moves;

	// Generates captures updates player attacks bitboard (needed in Evaluate), does not
	// include king attacks to squares that are defedend or attacks of pinned pieces that 
	// would leave the king in check if played.
	moves.generateCaptures(player, opponent);

	// Low bound on evaluation, since almost always making a move is better than doing nothing
	int standing_eval = nnue.evaluate();
	if (standing_eval >= beta) return standing_eval;
	if (standing_eval > alpha ) alpha = standing_eval;

	moves.orderMoves(player, opponent, nullptr, nullptr);
	unsigned short move;

	while (move = moves.getNextOrderedMove()) {
		MoveInfo mv_inf = makeMove(move, player, opponent, 0);
		int eval = -quiescenceSearch(-beta, -alpha, opponent, player, num_pieces - 1);
		unmakeMove(move, player, opponent, mv_inf);

		if (eval >= beta) return eval;
		if (eval > alpha) alpha = eval;
	}

	return alpha;
}


bool nullMove(Player& player, Player& opponent) {
	/*
		Do not null move if:
			- Player has only kings and pawns left
			- Player has < 4 pieces
			- Player is in check
	*/
	int num_pieces_player = std::popcount(player.bitboards.friendly_pieces);
	if ((player.bitboards.pawns | player.bitboards.king) == player.bitboards.friendly_pieces || 
		num_pieces_player < 4 || (player.bitboards.king & opponent.bitboards.attacks)) return false;
	
	return true;
}


bool reduceMove(int mv_pos, unsigned short mv, int depth, const MoveInfo& mv_info, const Player& player, 
				const Player& opponent, bool player_in_check, const std::array<unsigned short, 2>& killer_moves_at_ply) {
	/*
		Do not reduce for:
			- Killer moves
			- Moves while in check
			- Moves that give check
			- Captures or promotions
			- Depth < 3
			- First 2 moves
	*/

	for (unsigned short killer_move : killer_moves_at_ply) if (killer_move == mv) return false;

	if (player_in_check || (player.bitboards.attacks & opponent.bitboards.king) || 
		mv_info.capture_flag != no_capture || isPromotion(mv) || depth < 3 || mv_pos < 2) 
		return false;

	return true;
}

void repetition(Player& player, Player& opponent, const HashPositions& positions) {

	// Check draw by repetition
	if (positions.numPositions() >= 5) { // Can only repeat a position twice after at leat 5 moves without captures or pawn moves
		for (int i = positions.numPositions() - 5; i >= 0; i -= 2) {
				
			// Repeated position
			// Only need to check last position for repetition, since if it was before that, it would have been caught alredy
			if (positions[i] == positions.lastHash()) {

				unsigned long long attacks = opponent.bitboards.attacks;
				unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

				unsigned long long hash = positions.lastHash();
				Entry* entry = tt.get(hash, std::popcount(player.bitboards.all_pieces), player);

				// Check if Principal Variation leads to draw by repetition
				deal_repetition(player, opponent, positions, hash, positions[i], entry);

				// Restore attacks and squares to uncheck bitboards
				opponent.bitboards.attacks = attacks;
				player.bitboards.squares_to_uncheck = squares_to_uncheck;

				return;
			}
		}
	}
}

bool deal_repetition(Player& player, Player& opponent, const HashPositions& positions, unsigned long long hash, unsigned long long repeated_position, Entry* entry) {
	if (!entry) return false;

	int eval = entry->eval;
	MoveInfo mv_inf = makeMove(entry->best_move, player, opponent, hash);
	hash = mv_inf.hash;

	// Repetition of moves
	if (hash == repeated_position) {
		
		// If player is winning then it is an hallucination (since next move is a draw), so the evaluation is an upper bound.
		// Otherwise, player can get a guaranteed draw, so evaluation is 0.
		if (eval > 0) entry->node_flag = LowerBound;
		else entry->node_flag = Exact;
		entry->eval = 0;
		
		unmakeMove(entry->best_move, player, opponent, mv_inf);
		return true;
	}
	else if (!positions.contains(hash)) { // Return false if new position is not repeated
		unmakeMove(entry->best_move, player, opponent, mv_inf);
		return false;
	}

	Entry* new_entry = tt.get(hash, std::popcount(player.bitboards.all_pieces), opponent);

	bool result = deal_repetition(opponent, player, positions, hash, repeated_position, new_entry);
	unmakeMove(entry->best_move, player, opponent, mv_inf);

	// Delete the rest of the PV line from the tt
	if (result)
		entry->num_pieces = 100;

	return result;
}
