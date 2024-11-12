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
}

Accumulator::~Accumulator() {
	_mm_free(bias);
	_mm_free(weights);
}

void Accumulator::flipSides() {
	int16_t* tmp	 = side_to_move;
	side_to_move	 = side_not_to_move;
	side_not_to_move = tmp;
}

void Accumulator::set(const Player& player, const Player& opponent) {
	// Set any previous content of the array to initial state (bias)
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i loaded_bias = _mm256_load_si256((__m256i *) &bias[i]);
		_mm256_store_si256((__m256i*) & side_to_move[i], loaded_bias);
		_mm256_store_si256((__m256i*) & side_not_to_move[i], loaded_bias);
	}

	std::vector<NNUEIndex> indexes = getIndexesNNUE(player, opponent);

	// Add peices
	for (int i = 0; i < num_outputs; i += 16) {
		__m256i acc = _mm256_load_si256((__m256i*) & arr[i]);

		for (auto& [index_wk, index_bk] : indexes) {
			const int16_t* p_weights_sm  = weights + index_wk * num_outputs_side;
			const int16_t* p_weights_snm = weights + index_bk * num_outputs_side;

			const int16_t* p_weights = (i < num_outputs_side) ? p_weights_sm + i : p_weights_snm + i - num_outputs_side;
			const __m256i loaded_weights = _mm256_load_si256((__m256i*) p_weights);

			acc = _mm256_add_epi16(acc, loaded_weights);
		}

		_mm256_store_si256((__m256i*) & arr[i], acc);
	}

	// Reset pointers
	side_to_move	 = player.is_white ? &arr[0]				: &arr[num_outputs_side];
	side_not_to_move = player.is_white ? &arr[num_outputs_side] : &arr[0];
}

void Accumulator::movePiece(PieceType piece_type, location initial_loc, location final_loc, const Player& player, const Player& opponent) {

	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}

	auto [index_sm_add, index_snm_add] = getIndexNNUE(final_loc, piece_type, player, opponent);
	auto [index_sm_rm, index_snm_rm] = getIndexNNUE(initial_loc, piece_type, player, opponent);

	const int16_t* p_weights_sm_add  = weights + index_sm_add  * num_outputs_side;
	const int16_t* p_weights_snm_add = weights + index_snm_add * num_outputs_side;
	const int16_t* p_weights_sm_rm   = weights + index_sm_rm   * num_outputs_side;
	const int16_t* p_weights_snm_rm  = weights + index_snm_rm  * num_outputs_side;

	// Update side to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i]);

		const __m256i loaded_weights_add    = _mm256_load_si256((__m256i*) & p_weights_sm_add[i]);
		const __m256i loaded_weights_remove = _mm256_load_si256((__m256i*) & p_weights_sm_rm[i] );

		const __m256i result = _mm256_sub_epi16(_mm256_add_epi16(acc, loaded_weights_add), loaded_weights_remove);

		_mm256_store_si256((__m256i*) & arr[i], result);
	}

	// Update side not to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i + 256]);

		const __m256i loaded_weights_add    = _mm256_load_si256((__m256i*) & p_weights_snm_add[i]);
		const __m256i loaded_weights_remove = _mm256_load_si256((__m256i*) & p_weights_snm_rm[i] );

		const __m256i result = _mm256_sub_epi16(_mm256_add_epi16(acc, loaded_weights_add), loaded_weights_remove);

		_mm256_store_si256((__m256i*) & arr[i + 256], result);
	}
}

void Accumulator::addPiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
	
	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}
	
	auto [index_sm, index_snm] = getIndexNNUE(loc, piece_type, player, opponent);

	const int16_t* p_weights_sm  = weights + index_sm  * num_outputs_side;
	const int16_t* p_weights_snm = weights + index_snm * num_outputs_side;

	// Update side to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i]);

		const __m256i loaded_weights = _mm256_load_si256((__m256i*) & p_weights_sm[i]);

		const __m256i result = _mm256_add_epi16(acc, loaded_weights);

		_mm256_store_si256((__m256i*) & arr[i], result);
	}

	// Update side not to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i + 256]);

		const __m256i loaded_weights = _mm256_load_si256((__m256i*) & p_weights_snm[i]);

		const __m256i result = _mm256_add_epi16(acc, loaded_weights);

		_mm256_store_si256((__m256i*) & arr[i + 256], result);
	}
}

void Accumulator::removePiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {

	// If king moves, we need to recalculate every neuron of the accumulator
	if (piece_type == King) {
		set(player, opponent);
		return;
	}

	auto [index_sm, index_snm] = getIndexNNUE(loc, piece_type, player, opponent);

	const int16_t* p_weights_sm  = weights + index_sm  * num_outputs_side;
	const int16_t* p_weights_snm = weights + index_snm * num_outputs_side;

	// Update side to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i]);

		const __m256i loaded_weights = _mm256_load_si256((__m256i*) & p_weights_sm[i]);

		const __m256i result = _mm256_sub_epi16(acc, loaded_weights);

		_mm256_store_si256((__m256i*) & arr[i], result);
	}

	// Update side not to move
	for (int i = 0; i < num_outputs_side; i += 16) {
		const __m256i acc = _mm256_load_si256((__m256i*) & arr[i + 256]);

		const __m256i loaded_weights = _mm256_load_si256((__m256i*) & p_weights_snm[i]);

		const __m256i result = _mm256_sub_epi16(acc, loaded_weights);

		_mm256_store_si256((__m256i*) & arr[i + 256], result);
	}
}

void Accumulator::setWeights(std::filesystem::path file_biases, std::filesystem::path file_weights) {
	loadFromFile(bias, num_outputs_side, file_biases);
	loadFromFile(weights, num_inputs * num_outputs_side, file_weights);
}
