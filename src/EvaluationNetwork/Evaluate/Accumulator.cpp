#include "Accumulator.h"
#include "ClippedReLU.h"
#include "InputNNUE.h"
#include "LoadWeights.h"
#include <bit>
#include <cstdint>
#include <immintrin.h>
#include <string>
#include <vector>

Accumulator::Accumulator() {
	bias = static_cast<int16_t*>(_mm_malloc(num_outputs_side * sizeof(int16_t), 32));
	weights = static_cast<int16_t*>(_mm_malloc(num_inputs * num_outputs_side * sizeof(int16_t), 32));

	added_pieces.reserve(10);
	removed_pieces.reserve(10);
}

Accumulator::~Accumulator() {
	_mm_free(bias);
	_mm_free(weights);
}

void Accumulator::refresh() {
	if (added_pieces.empty() && removed_pieces.empty()) return;

	for (int i = 0; i < num_outputs; i += 16) {
		__m256i acc = _mm256_load_si256((__m256i*) & arr[i]);

		for (auto& [p_weights_wk, p_weights_bk] : added_pieces) {
			const int16_t* p_weights = (i < num_outputs_side) ? p_weights_wk : p_weights_bk - num_outputs_side;

			acc = _mm256_add_epi16(acc, _mm256_load_si256((__m256i*) & p_weights[i]));
		}

		for (auto& [p_weights_wk, p_weights_bk] : removed_pieces) {
			const int16_t* p_weights = (i < num_outputs_side) ? p_weights_wk : p_weights_bk - num_outputs_side;

			acc = _mm256_sub_epi16(acc, _mm256_load_si256((__m256i*) & p_weights[i]));
		}

		_mm256_store_si256((__m256i*) & arr[i], acc);
	}

	// Reset added and removed pieces
	added_pieces.clear();
	removed_pieces.clear();
}

void Accumulator::set(const Player& player, const Player& opponent) {
	std::vector<NNUEIndex> indexes = getIndexesNNUE(player, opponent);

	// Add peices
	for (int i = 0; i < num_outputs; i += 16) {
		const int idx = (i < num_outputs_side) ? i : i - num_outputs_side;
		__m256i acc = _mm256_load_si256((__m256i*) & bias[idx]);

		for (auto& [index_wk, index_bk] : indexes) {
			const int16_t* p_weights_sm  = weights + index_wk * num_outputs_side;
			const int16_t* p_weights_snm = weights + index_bk * num_outputs_side;

			const int16_t* p_weights = (i < num_outputs_side) ? p_weights_sm + i : p_weights_snm + i - num_outputs_side;

			acc = _mm256_add_epi16(acc, _mm256_load_si256((__m256i*) p_weights));
		}

		_mm256_store_si256((__m256i*) & arr[i], acc);
	}

	// Reset pointers
	side_to_move = player.is_white ? &arr[0] : &arr[num_outputs_side];
	side_not_to_move = player.is_white ? &arr[num_outputs_side] : &arr[0];

	// Reset added and removed pieces
	added_pieces.clear();
	removed_pieces.clear();
}

void Accumulator::flipSides() {
	int16_t* tmp	 = side_to_move;
	side_to_move	 = side_not_to_move;
	side_not_to_move = tmp;
}

void Accumulator::movePiece(PieceType piece_type, location initial_loc, location final_loc, const Player& player, const Player& opponent) {

	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}

	auto [index_wk_add, index_bk_add] = getIndexNNUE(final_loc, piece_type, player, opponent);
	auto [index_wk_rm, index_bk_rm]   = getIndexNNUE(initial_loc, piece_type, player, opponent);

	const int16_t* p_weights_wk_add = weights + index_wk_add * num_outputs_side;
	const int16_t* p_weights_bk_add = weights + index_bk_add * num_outputs_side;
	const int16_t* p_weights_wk_rm  = weights + index_wk_rm  * num_outputs_side;
	const int16_t* p_weights_bk_rm  = weights + index_bk_rm  * num_outputs_side;

	added_pieces.emplace_back(p_weights_wk_add, p_weights_bk_add);
	removed_pieces.emplace_back(p_weights_wk_rm, p_weights_bk_rm);
}

void Accumulator::addPiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
	
	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}
	
	auto [index_wk, index_bk] = getIndexNNUE(loc, piece_type, player, opponent);

	const int16_t* p_weights_wk = weights + index_wk * num_outputs_side;
	const int16_t* p_weights_bk = weights + index_bk * num_outputs_side;

	added_pieces.emplace_back(p_weights_wk, p_weights_bk);
}

void Accumulator::removePiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {

	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}

	auto [index_wk, index_bk] = getIndexNNUE(loc, piece_type, player, opponent);

	const int16_t* p_weights_wk = weights + index_wk * num_outputs_side;
	const int16_t* p_weights_bk = weights + index_bk * num_outputs_side;

	removed_pieces.emplace_back(p_weights_wk, p_weights_bk);
}

bool Accumulator::setWeights(std::filesystem::path file_biases, std::filesystem::path file_weights) {
	bool loaded_bias    = loadFromFile(bias, num_outputs_side, file_biases);
	bool loaded_weights = loadFromFile(weights, num_inputs * num_outputs_side, file_weights);

	return loaded_bias && loaded_weights;
}
