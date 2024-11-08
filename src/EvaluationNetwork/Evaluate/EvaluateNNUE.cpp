#include "EvaluateNNUE.h"
#include "Player.h"
#include "Accumulator.h"
#include "ClippedReLU.h"
#include "LinearLayer.h"
#include <array>
#include <cstdint>
#include <filesystem>

constexpr double scale = 128;

NNUE::NNUE() {
	std::filesystem::path weights_dir = std::filesystem::path(__FILE__).parent_path().parent_path() / "Weights";

	accumulator.setWeights(weights_dir / "accb.bin", weights_dir / "accw.bin");
	hidden_layer1.setWeights(weights_dir / "lin1b.bin", weights_dir / "lin1w.bin");
	hidden_layer2.setWeights(weights_dir / "lin2b.bin", weights_dir / "lin2w.bin");
	hidden_layer3.setWeights(weights_dir / "lin3b.bin", weights_dir / "lin3w.bin");

	// TODO: Throw error if files not found
}

int NNUE::evaluate() {

	// Quantitize accumulator
	crelu(accumulator.arr, accumulator.quant_arr, 512);

	// First hidden layer
	hidden_layer1.processLinearLayer(accumulator.quant_arr, hidden_neuros1);
	crelu(hidden_neuros1, quant_hidden_neuros1, 32);

	// Second hidden layer
	hidden_layer2.processLinearLayer(quant_hidden_neuros1, hidden_neuros2);
	crelu(hidden_neuros2, quant_hidden_neuros2, 32);

	// Output layer
	hidden_layer3.processLinearLayer(quant_hidden_neuros2, &output_neuron);

	return output_neuron / scale;
}

void NNUE::setPosition(const Player& player, const Player& opponent) {
	accumulator.set(player, opponent);
}

void NNUE::movePiece(PieceType piece_type, location initial_loc, location final_loc, const Player& player, const Player& opponent) {
	accumulator.movePiece(piece_type, initial_loc, final_loc, player, opponent);
}

void NNUE::addPiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
	accumulator.addPiece(piece_type, loc, player, opponent);
}

void NNUE::removePiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
	accumulator.removePiece(piece_type, loc, player, opponent);
}

void NNUE::flipSides() {
	accumulator.flipSides();
}
