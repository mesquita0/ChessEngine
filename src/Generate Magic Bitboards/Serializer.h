#pragma once
#include "Square.h"
#include <array>
#include <fstream>
#include <vector>

struct SaveFile {
	std::array<Square, 64> squares_bishops = {};
	std::array<Square, 64> squares_rooks = {};
	std::vector<unsigned long long> attacks_array;

	SaveFile(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array);
	SaveFile() = default;
	SaveFile(SaveFile&);

	void store(std::ofstream& file) const;
	void load(std::ifstream& file);
};

SaveFile loadMagics();
bool storeMagics(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array);
