#pragma once
#include <cstdint>
#include "Player.h"

struct alignas(64) Accumulator{
	int8_t**	weights;
	int16_t 	arr[512];
	int8_t 		quant_arr[512];

	Accumulator();

	void set(const Player& player, const Player& opponent);
	void update(const unsigned short move);
};
