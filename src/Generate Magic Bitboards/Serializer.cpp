#include "Serializer.h"
#include "Square.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

SaveFile::SaveFile(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array) {
	this->squares_bishops = squares_bishops;
	this->squares_rooks = squares_rooks;
	this->attacks_array = attacks_array;
}

SaveFile::SaveFile(SaveFile& other) {
	this->squares_bishops = other.squares_bishops;
	this->squares_rooks = other.squares_rooks;
	this->attacks_array = other.attacks_array;
}

void SaveFile::store(std::ofstream& file) const {
	file.write((char*) squares_bishops.data(), squares_bishops.size() * sizeof(Square));
	file.write((char*) squares_rooks.data(), squares_rooks.size() * sizeof(Square));

	size_t attacks_array_size = attacks_array.size();
	file.write((char*) &attacks_array_size, sizeof(size_t));
	file.write((char*) attacks_array.data(), attacks_array.size() * sizeof(unsigned long long));
}

void SaveFile::load(std::ifstream& file) {
	file.read((char*) squares_bishops.data(), squares_bishops.size() * sizeof(Square));
	file.read((char*) squares_rooks.data(), squares_rooks.size() * sizeof(Square));

	size_t attacks_array_size;
	file.read((char*) &attacks_array_size, sizeof(size_t));
	attacks_array.reserve(attacks_array_size);

	for (int i = 0; i < attacks_array_size; i++) {
		unsigned long long attacks;
		file.read((char*) &attacks, sizeof(unsigned long long));
		attacks_array.push_back(attacks);
	}
}

bool storeMagics(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array) {
	SaveFile save_file(squares_bishops, squares_rooks, attacks_array);

	// Get file's directory
	std::filesystem::path dir_path = std::filesystem::path(__FILE__).parent_path();

	std::ofstream file_out(dir_path / "magic_numbers.bin", std::ios::binary);
	if (!file_out) return false;

	save_file.store(file_out);

	return true;
} 

SaveFile loadMagics() {
	SaveFile save_file;

	// Get file's directory
	std::filesystem::path dir_path = std::filesystem::path(__FILE__).parent_path();

	std::ifstream file_in(dir_path / "magic_numbers.bin", std::ios::binary);
	if (!file_in.is_open()) return save_file;

	save_file.load(file_in);

	return save_file;
}
