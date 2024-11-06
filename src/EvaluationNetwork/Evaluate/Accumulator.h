#pragma once
#include "PieceTypes.h"
#include "Player.h"
#include <cstdint>
#include <filesystem>

constexpr int num_inputs = 64 * 64 * 10;
constexpr int num_outputs = 512;
constexpr int num_outputs_side = num_outputs / 2;

struct alignas(32) Accumulator{
	int16_t 	arr[num_outputs] = {};
	int8_t 		quant_arr[num_outputs] = {};

private:
	int16_t*	bias;
	int16_t*	weights;
	int16_t* 	side_to_move	 =	&arr[0];
	int16_t* 	side_not_to_move =	&arr[num_outputs_side];

public:
	Accumulator();
	~Accumulator();

	void flipSides();
	void set(const Player& player, const Player& opponent);
	void movePiece(PieceType piece_type, location initial_loc, location final_loc, const Player& player, const Player& opponent);
	void addPiece(PieceType piece_type, location loc, const Player& player, const Player& opponent);
	void removePiece(PieceType piece_type, location loc, const Player& player, const Player& opponent);

	void setWeights(std::filesystem::path file_biases, std::filesystem::path file_weights);
};
