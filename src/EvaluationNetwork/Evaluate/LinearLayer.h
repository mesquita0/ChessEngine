#pragma once
#include <cstdint>
#include <filesystem>

struct LinearLayer {
private:
	int32_t*	bias;
	int8_t*		weights;
	int			num_inputs, num_outputs;

public:
	LinearLayer(int num_inputs, int num_outputs);
	~LinearLayer();
	
	void processLinearLayer(int8_t* const input, int32_t* output) const;

	bool setWeights(std::filesystem::path file_biases, std::filesystem::path file_weights);
};
