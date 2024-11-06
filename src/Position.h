#pragma once
#include "Player.h"
#include "Locations.h"
#include <string>

struct Position {
	Player player, opponent;
	int half_moves, full_moves;
};

std::string PositionToFEN(const Player& player, const Player& opponent, int half_moves, int full_moves);
Position FENToPosition(const std::string& FEN, bool generate_attacks=true, bool set_pins=true);
std::string locationToNotationSquare(int square);
location notationSquareToLocation(const std::string& square);
void printBoard(const Player& player, const Player& opponent);
