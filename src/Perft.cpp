#include "Perft.h"
#include "MagicBitboards.h"
#include "MakeMoves.h"
#include "Moves.h"
#include "Locations.h"
#include "Player.h"
#include "Position.h"
#include <iostream>

static unsigned long long Perftr(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, Moves& moves);

unsigned long long Perft(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards) {
	/*
	Perftr won't revert back changes in attacks and squares_to_uncheck bitboards, which is problematic if it
	is called multiples times on the same position, since it won't leave the position as it was before, this
	function is only to restore those bitboards to what they were before so that position is not changed at all
	in a perft call and, therefore, can be called multiple times on the same position without problems.
	*/
	Moves moves;

	unsigned long long attacks = opponent.bitboards.attacks;
	unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

	unsigned long long results = Perftr(depth, player, opponent, magic_bitboards, moves);

	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	return results;
}

static unsigned long long Perftr(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards, Moves& moves) {
	if (depth == 0) return 1;
	
	moves.generateMoves(player, opponent, magic_bitboards);

	// Bulk counting
	if (depth == 1) return moves.num_moves;

	Moves new_moves;
	unsigned long long nodes = 0;

	for (const unsigned short move : moves) {

		location opponent_en_passant_target = opponent.locations.en_passant_target;
		bool player_can_castle_king_side = player.can_castle_king_side;
		bool player_can_castle_queen_side = player.can_castle_queen_side;
		bool opponent_can_castle_king_side = opponent.can_castle_king_side;
		bool opponent_can_castle_queen_side = opponent.can_castle_queen_side;

		/*
			unmake move does not reverse changes in attacks / squares to uncheck bitboards, since they are going to 
			be recomputed when the next move is made (since the board will change), and the moves for the current position
			are alredy computed.
		*/

		MoveInfo move_info = makeMove(move, player, opponent, magic_bitboards);
		nodes += Perftr(depth - 1, opponent, player, magic_bitboards, new_moves);
		unmakeMove(move, player, opponent, opponent_en_passant_target, player_can_castle_king_side, player_can_castle_queen_side,
				   opponent_can_castle_king_side, opponent_can_castle_queen_side, move_info, magic_bitboards);
	}

	return nodes;
}

unsigned long long PerftDivide(int depth, Player& player, Player& opponent, const MagicBitboards& magic_bitboards) {
	unsigned long long nodes = 0;

	unsigned long long attacks = opponent.bitboards.attacks;
	unsigned long long squares_to_uncheck = player.bitboards.squares_to_uncheck;

	Moves moves;
	moves.generateMoves(player, opponent, magic_bitboards);

	for (const unsigned short move : moves) {

		location opponent_en_passant_target = opponent.locations.en_passant_target;
		bool player_can_castle_king_side = player.can_castle_king_side;
		bool player_can_castle_queen_side = player.can_castle_queen_side;
		bool opponent_can_castle_king_side = opponent.can_castle_king_side;
		bool opponent_can_castle_queen_side = opponent.can_castle_queen_side;

		std::cout << locationToNotationSquare((move >> 6) & 0x3f) << locationToNotationSquare(move & 0x3f);
		MoveInfo move_info = makeMove(move, player, opponent, magic_bitboards);
		unsigned long long nodes_move = Perft(depth - 1, opponent, player, magic_bitboards);
		nodes += nodes_move;
		std::cout << ": " << nodes_move << '\n';
		unmakeMove(move, player, opponent, opponent_en_passant_target, player_can_castle_king_side, player_can_castle_queen_side,
				   opponent_can_castle_king_side, opponent_can_castle_queen_side, move_info, magic_bitboards);
	}

	opponent.bitboards.attacks = attacks;
	player.bitboards.squares_to_uncheck = squares_to_uncheck;

	return nodes;
}
