#pragma once
#include "Player.h"

constexpr short no_capture = 0;
constexpr short pawn_capture = 1;
constexpr short knight_capture = 2;
constexpr short bishop_capture = 3;
constexpr short rook_capture = 4;
constexpr short queen_capture = 5;

struct MoveInfo {
	bool player_could_castle_king_side, player_could_castle_queen_side, opponent_could_castle_king_side, opponent_could_castle_queen_side;
	location opponent_en_passant_target;
	short capture_flag;
	unsigned long long hash;
};

// Returns move info to be used in unmakeMove.
MoveInfo makeMove(const unsigned short move, Player& player, Player& opponent, unsigned long long hash);

void unmakeMove(const unsigned short move, Player& player, Player& opponent, const MoveInfo& move_info);
