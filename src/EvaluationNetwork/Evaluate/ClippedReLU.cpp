#include "ClippedReLU.h"
#include <cassert>
#include <cstdint>
#include <immintrin.h>

void crelu(const int16_t* input, int8_t* output, int size) {
	assert(size % 32 == 0, "Size must be a multiple of 32.");

	const __m256i zeros = _mm256_setzero_si256();

	for (int i = 0; i < size; i += 32) {
		const __m256i inp_0 = _mm256_load_si256((__m256i*) &input[i +  0]);
		const __m256i inp_1 = _mm256_load_si256((__m256i*) &input[i + 16]);

		// Converts the two vectors of 16 bit ints to 8 bit ints, saturation
		// is signed since if it was unsigned the max would be 255 instead of 127,
		// then the lower bound is established later by taking the max of the results with zero.
		const __m256i packed = _mm256_max_epi8(_mm256_packs_epi16(inp_0, inp_1), zeros);

		// The result in the final vector will be in the following order:
		// 0, 2, 1, 3 
		const __m256i result = _mm256_permute4x64_epi64(packed, 0b11011000); // change the order to the original one

		_mm256_store_si256((__m256i *) &output[i], result);
	}
}

void crelu(const int32_t* input, int8_t* output, int size) {
	assert(size % 32 == 0, "Size must be a multiple of 32.");

	const __m256i zeros = _mm256_setzero_si256();
	const __m256i idx = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);

	for (int i = 0; i < size; i += 32) {
		const __m256i inp_0 = _mm256_load_si256((__m256i*) & input[i +  0]);
		const __m256i inp_1 = _mm256_load_si256((__m256i*) & input[i +  8]);
		const __m256i inp_2 = _mm256_load_si256((__m256i*) & input[i + 16]);
		const __m256i inp_3 = _mm256_load_si256((__m256i*) & input[i + 24]);

		// Converts the vectors of 32 bit ints into two vectors of 16 bit ints, then into a vector of 8 bit 
		// ints, saturation is signed since if it was unsigned the max would be 255 instead of 127
		const __m256i packed_1 = _mm256_packs_epi32(inp_0, inp_1);
		const __m256i packed_2 = _mm256_packs_epi32(inp_2, inp_3);
		const __m256i packed_3 = _mm256_packs_epi16(packed_1, packed_2);

		// Lower bound at 0
		const __m256i packed = _mm256_max_epi8(packed_3, zeros);

		// The result in the final vector will be in the following order:
		// 0, 4, 1, 5, 2, 6, 3, 7
		const __m256i result = _mm256_permutevar8x32_epi32(packed, idx); // change the order to the original one

		_mm256_store_si256((__m256i*) & output[i], result);
	}
}
