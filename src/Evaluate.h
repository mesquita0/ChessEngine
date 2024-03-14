#pragma once
#include "MagicBitboards.h"
#include "Player.h"
#include <climits>

constexpr int checkmated_eval = INT_MIN + 1;

int Evaluate(const Player& player, const Player& opponent, const MagicBitboards& magic_bitboards, int depth_searched);
