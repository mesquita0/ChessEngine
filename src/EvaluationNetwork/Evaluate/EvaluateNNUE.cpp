#include "EvaluateNNUE.h"
#include "Player.h"
#include "Accumulator.h"
#include "ClippedReLU.h"
#include "LinearLayer.h"
#include <array>
#include <cstdint>

constexpr double scale = 128;

NNUE::NNUE() {

}

int NNUE::evaluate() {

	// First hidden layer
	process_linear_layer(accumulator.quant_arr, hidden_neuros1, hidden_layer1);
	crelu(hidden_neuros1, quant_hidden_neuros1, 32);

	// Second hidden layer
	process_linear_layer(quant_hidden_neuros1, hidden_neuros2, hidden_layer2);
	crelu(hidden_neuros2, quant_hidden_neuros2, 32);
	
	// Output layer
	process_linear_layer(quant_hidden_neuros2, &output_neuron, hidden_layer3);

	return output_neuron / scale;
}

void NNUE::setPosition(const Player& player, const Player& opponent) {

}

void NNUE::makeMove(const unsigned short move) {

}

void NNUE::unmakeMove(const unsigned short move) {

}
