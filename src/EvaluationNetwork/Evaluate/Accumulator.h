#pragma once
#include <cstdint>
#include "Player.h"

struct alignas(64) Accumulator{
	int32_t*	bias;
	int16_t*	weights;
	int16_t 	arr[512];
	int16_t* 	side_to_move	 =	&arr[0];
	int16_t* 	side_not_to_move =	&arr[256];
	int8_t 		quant_arr[512];

	Accumulator();

	void set(const Player& player, const Player& opponent);
	void update(const unsigned short move);
};
