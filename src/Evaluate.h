#pragma once
#include "Player.h"
#include <climits>

constexpr short checkmated_eval = SHRT_MIN + 1;
constexpr short checkmate_eval = SHRT_MAX;

int Evaluate(const Player& player, const Player& opponent, int num_pieces);
