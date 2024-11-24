#pragma once
#include "Player.h"
#include "Accumulator.h"
#include "ClippedReLU.h"
#include "LinearLayer.h"
#include "PieceTypes.h"
#include <cstdint>

class NNUE {
	Accumulator				accumulator					= Accumulator();
	alignas(64) int32_t		hidden_neuros1[32]			= {};
	alignas(64) int8_t		quant_hidden_neuros1[32]	= {};
	LinearLayer				hidden_layer1				= LinearLayer(512, 32);
	alignas(64) int32_t		hidden_neuros2[32]			= {};
	alignas(64) int8_t		quant_hidden_neuros2[32]	= {};
	LinearLayer				hidden_layer2				= LinearLayer(32, 32);
	int32_t					output_neuron				= 0;
	LinearLayer				hidden_layer3				= LinearLayer(32, 1);

	bool loaded = false;

public:
	NNUE();

	int evaluate();

	inline void setPosition(const Player& player, const Player& opponent) {
		accumulator.set(player, opponent);
	}

	inline void movePiece(PieceType piece_type, location initial_loc, location final_loc, const Player& player, const Player& opponent) {
		accumulator.movePiece(piece_type, initial_loc, final_loc, player, opponent);
	}

	inline void addPiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
		accumulator.addPiece(piece_type, loc, player, opponent);
	}

	inline void removePiece(PieceType piece_type, location loc, const Player& player, const Player& opponent) {
		accumulator.removePiece(piece_type, loc, player, opponent);
	}

	inline void flipSides() {
		accumulator.flipSides();
	}

	inline bool is_loaded() const { return loaded; }
};

inline NNUE nnue = NNUE(); // Global NNUE
