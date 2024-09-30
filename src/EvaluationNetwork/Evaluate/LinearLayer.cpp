#include "LinearLayer.h"
#include <cassert>
#include <cstdint>
#include <immintrin.h>

LinearLayer::LinearLayer(int num_inputs, int num_outputs) {
	this->num_inputs = num_inputs;
	this->num_outputs = num_outputs;
}

LinearLayer::~LinearLayer() {

}

void LinearLayer::process_linear_layer(const int8_t* input, int32_t* output) const {

	assert(num_inputs % 32 == 0, "Number of input neurons has to be a multiple of 32.");
	assert((num_outputs % 4 == 0) || (num_outputs == 1), "Number of output neurons has to be a multiple of 4 (except output layer).");

	if (num_outputs == 1) {
		assert(num_inputs == 32, "Can only have 32 neuros connected to the output layer.");

		// Load inputs
		__m256i inp = _mm256_load_si256((__m256i*) input);

		// (32 i8s) * (32 i8s) -> (16 i16s)
		// Multiplies input vector by weights and add neighbour results.
		// Input will always be positive because of the activation function, so passing it to maddubs
		// which requires the first vector to be of unsigned ints is fine, saturation does not matter
		// since inp and weights can be at most 127, and 127 * 128 * 2 still fits in an i16.
		__m256i result = _mm256_maddubs_epi16(inp, _mm256_load_si256((__m256i*) weights));

		// (16 i8s) -> (8 i32s)
		// Add neighbour values and convert to 32 bit ints
		const __m256i ones = _mm256_set1_epi16(1);
		result = _mm256_madd_epi16(result, ones);

		// Accumulate values to two scalars at the least significant bits of low and high bits of the register
		const __m256i zeros = _mm256_setzero_si256();
		result = _mm256_hadd_epi32(result, zeros); // (8 i32s) -> (4 i32s)
		result = _mm256_hadd_epi32(result, zeros); // (4 i32s) -> (2 i32s)

		// Separate the two scalars into different registers
		__m128i result_low = _mm256_castsi256_si128(result);
		__m128i result_high = _mm256_extracti128_si256(result, 1);

		// Add them and the bias, then store the result in the output
		__m128i out = _mm_add_epi32(result_low, result_high);
		output[0] = _mm_extract_epi32(out, 0) + bias[0];
	}
	else {
		/*
		* Calculate 4 outputs at a time so that we need to load the bias less times,
		* since bias are 32 bits we can load 4 of them at a time with 128 bits.
		* Calculating 8 outputs at a time is not feasible since there's only 16
		* avx registers, so some data would inevitably spill to memory.
		*/

		for (int i = 0; i < num_outputs; i += 4) {

			int8_t* const weights_0 = weights + (i + 0) * num_outputs;
			int8_t* const weights_1 = weights + (i + 1) * num_outputs;
			int8_t* const weights_2 = weights + (i + 2) * num_outputs;
			int8_t* const weights_3 = weights + (i + 3) * num_outputs;

			__m256i accumulator_0 = _mm256_setzero_si256();
			__m256i accumulator_1 = _mm256_setzero_si256();
			__m256i accumulator_2 = _mm256_setzero_si256();
			__m256i accumulator_3 = _mm256_setzero_si256();

			for (int j = 0; j < num_inputs; j += 32) {
				// Load inputs
				__m256i inp = _mm256_load_si256((__m256i*) &input[j]);
				
				// (32 i8s) * (32 i8s) -> (16 i16s)
				// Multiplies input vector by weights and add neighbour results
				// Input will always be positive because of the activation function, so passing it to maddubs
				// which requires the first vector to be of unsigned ints is fine.
				__m256i result_0 = _mm256_maddubs_epi16(inp, _mm256_load_si256((__m256i*) &weights_0[j]));
				__m256i result_1 = _mm256_maddubs_epi16(inp, _mm256_load_si256((__m256i*) &weights_1[j]));
				__m256i result_2 = _mm256_maddubs_epi16(inp, _mm256_load_si256((__m256i*) &weights_2[j]));
				__m256i result_3 = _mm256_maddubs_epi16(inp, _mm256_load_si256((__m256i*) &weights_3[j]));

				// (16 i8s) -> (8 i32s)
				// Add neighbour values and convert to 32 bit ints
				const __m256i ones = _mm256_set1_epi16(1);
				result_0 = _mm256_madd_epi16(result_0, ones);
				result_1 = _mm256_madd_epi16(result_1, ones);
				result_2 = _mm256_madd_epi16(result_2, ones);
				result_3 = _mm256_madd_epi16(result_3, ones);

				// Add results to accumulators
				accumulator_0 = _mm256_add_epi32(result_0, accumulator_0);
				accumulator_1 = _mm256_add_epi32(result_1, accumulator_1);
				accumulator_2 = _mm256_add_epi32(result_2, accumulator_2);
				accumulator_3 = _mm256_add_epi32(result_3, accumulator_3);
			}

			// horizontal adding will result in the output like:
			// (acc0, acc0, acc1, acc1, acc0, acc0, acc1, acc1)
			__m256i tmp_1 = _mm256_hadd_epi32(accumulator_0, accumulator_1);
			__m256i tmp_2 = _mm256_hadd_epi32(accumulator_2, accumulator_3);

			// Then by doing it again we get the following:
			// (acc0, acc1, acc2, acc3, acc0, acc1, acc2, acc3)
			__m256i tmp_3 = _mm256_hadd_epi32(tmp_1, tmp_2);

			// Then add low and high parts of the register to get the final result
			__m128i tmp_3_low = _mm256_castsi256_si128(tmp_3);
			__m128i tmp_3_high = _mm256_extracti128_si256(tmp_3, 1);
			__m128i result = _mm_add_epi32(tmp_3_low, tmp_3_high);

			// Add bias to all 4 outputs
			result = _mm_add_epi32(result, _mm_load_si128((__m128i*) &bias[i]));
			
			_mm_store_si128((__m128i *) &output[i], result);
		}
	}
}
