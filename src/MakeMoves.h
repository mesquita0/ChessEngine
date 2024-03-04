#pragma once
#include "Player.h"
#include "MagicBitboards.h"
#include "Zobrist.h"

constexpr short no_capture = 0;
constexpr short pawn_capture = 1;
constexpr short knight_capture = 2;
constexpr short bishop_capture = 3;
constexpr short rook_capture = 4;
constexpr short queen_capture = 5;

struct MoveInfo {
	short capture_flag;
	unsigned long long hash;
};

// Returns move info to be used in unmakeMove.
MoveInfo makeMove(const unsigned short move, Player& player, Player& opponent, unsigned long long hash, const MagicBitboards& magic_bitboards, const ZobristKeys& zobrist_keys);

void unmakeMove(const unsigned short move, Player& player, Player& opponent, location opponent_en_passant_target, 
				bool player_can_castle_king_side, bool player_can_castle_queen_side, bool opponent_can_castle_king_side, 
				bool opponent_can_castle_queen_side, const MoveInfo& move_info, const MagicBitboards& magic_bitboards);
