#pragma once
#include "Square.h"
#include <array>
#include <vector>

struct MagicFile {
	unsigned long long magic_number;
	int bits_used;

	MagicFile(Magic& magic);
	MagicFile() = default;

	template<class Archive>
	void serialize(Archive& archive) {
		archive(magic_number, bits_used);
	}
};

struct SquareFile {
	unsigned long long mask;
	MagicFile magic;
	int index_attack_array;

	SquareFile(Square& square);
	SquareFile() = default;

	template<class Archive>
	void serialize(Archive& archive) {
		archive(mask, magic, index_attack_array);
	}
};

struct SaveFile {
	std::array<SquareFile, 64> squares_bishops = {};
	std::array<SquareFile, 64> squares_rooks = {};
	std::vector<unsigned long long> attacks_array;

	SaveFile(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array);
	SaveFile() = default;

	template<class Archive>
	void serialize(Archive& archive) {
		archive(squares_bishops, squares_rooks, attacks_array);
	}
};

SaveFile loadMagics();
bool storeMagics(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array);
