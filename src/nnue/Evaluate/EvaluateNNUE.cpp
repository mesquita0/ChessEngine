#include "EvaluateNNUE.h"
#include "Player.h"
#include "Accumulator.h"
#include "ClippedReLU.h"
#include "LinearLayer.h"
#include <array>
#include <cstdint>
#include <filesystem>

NNUE::NNUE() {
	std::filesystem::path weights_dir = std::filesystem::path(__FILE__).parent_path().parent_path() / "Weights";

	bool loaded = accumulator.setWeights(weights_dir / "accb.bin", weights_dir / "accw.bin");
	loaded &= hidden_layer1.setWeights(weights_dir / "lin1b.bin", weights_dir / "lin1w.bin");
	loaded &= hidden_layer2.setWeights(weights_dir / "lin2b.bin", weights_dir / "lin2w.bin");
	loaded &= hidden_layer3.setWeights(weights_dir / "lin3b.bin", weights_dir / "lin3w.bin");

	this->loaded = loaded;
}

int NNUE::evaluate() {
	accumulator.refresh();

	// Quantitize accumulator
	crelu(accumulator.side_to_move, accumulator.quant_arr, 256);
	crelu(accumulator.side_not_to_move, accumulator.quant_arr + 256, 256);

	// First hidden layer
	hidden_layer1.processLinearLayer(accumulator.quant_arr, hidden_neuros1);
	crelu(hidden_neuros1, quant_hidden_neuros1, 32);

	// Second hidden layer
	hidden_layer2.processLinearLayer(quant_hidden_neuros1, hidden_neuros2);
	crelu(hidden_neuros2, quant_hidden_neuros2, 32);

	// Output layer
	hidden_layer3.processLinearLayer(quant_hidden_neuros2, &output_neuron);

	return output_neuron;
}
