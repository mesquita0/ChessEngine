#pragma once
#include "MagicBitboards.h"
#include "Player.h"
#include <climits>

constexpr int checkmated_eval = INT_MIN + 11;
constexpr int checkmate_eval = -checkmated_eval;

int Evaluate(const Player& player, const Player& opponent, const MagicBitboards& magic_bitboards);
