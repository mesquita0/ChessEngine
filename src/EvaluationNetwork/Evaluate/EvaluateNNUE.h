#pragma once
#include "Player.h"
#include "Accumulator.h"
#include "ClippedReLU.h"
#include "LinearLayer.h"
#include <cstdint>

class NNUE {
	Accumulator				accumulator;
	alignas(64) int32_t		hidden_neuros1[32];
	alignas(64) int8_t		quant_hidden_neuros1[32];
	LinearLayer				hidden_layer1				= LinearLayer(512, 32);
	alignas(64) int32_t		hidden_neuros2[32];
	alignas(64) int8_t		quant_hidden_neuros2[32];
	LinearLayer				hidden_layer2				= LinearLayer(32, 32);
	int32_t					output_neuron;
	LinearLayer				hidden_layer3				= LinearLayer(32, 1);
	
public:
	// TODO: Initialize weight and biases in layers
	NNUE();

	int evaluate();

	void setPosition(const Player& player, const Player& opponent);
	void makeMove(const unsigned short move);
	void unmakeMove(const unsigned short move);

};

inline NNUE nnue = NNUE(); // Global NNUE
