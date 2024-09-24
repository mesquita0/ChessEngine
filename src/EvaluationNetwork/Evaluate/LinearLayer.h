#pragma once
#include <cstdint>

struct LinearLayer {
	int32_t*	bias;
	int8_t**	weights;
	int			num_inputs, num_outputs;

	LinearLayer(int num_inputs, int num_outputs);
	~LinearLayer();
};

void process_linear_layer(const int8_t* input, int32_t* output, LinearLayer& linear_layer);
