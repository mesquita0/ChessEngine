#pragma once
#include "Player.h"
#include <vector>

enum GameOutcome { ongoing, checkmate, stalemate, draw_by_insufficient_material, draw_by_repetition, draw_by_50_move_rule };

// Updates positions vector and returns new number of half moves
int updatePositions(std::vector<unsigned long long>& positions, const short capture_flag, const unsigned short move_flag, int half_moves);

GameOutcome getGameOutcome(const Player& player, const Player& opponent, const std::vector<unsigned long long>& positions, int half_moves);
