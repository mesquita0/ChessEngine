#include "Serializer.h"
#include "Square.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

MagicFile::MagicFile(Magic& magic) {
	this->magic_number = magic.magic_number;
	this->bits_used = magic.bits_used;
}

SquareFile::SquareFile(Square& square) {
	this->mask = square.mask;
	this->magic = MagicFile(square.magic);
	this->index_attack_array = square.index_attack_array;
}

SaveFile::SaveFile(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array) {

	//
	std::transform(squares_bishops.begin(), squares_bishops.end(), this->squares_bishops.begin(), [](Square& square) {
		return SquareFile(square);
		});
	std::transform(squares_rooks.begin(), squares_rooks.end(), this->squares_rooks.begin(), [](Square& square) {
		return SquareFile(square);
		});

	this->attacks_array = attacks_array;
}

bool storeMagics(std::array<Square, 64>& squares_bishops, std::array<Square, 64>& squares_rooks, std::vector<unsigned long long>& attacks_array) {
	SaveFile save_file(squares_bishops, squares_rooks, attacks_array);

	std::ofstream out_file;

	// Get file's directory
	std::filesystem::path dir_path = std::filesystem::path(__FILE__).parent_path();

	out_file.open(dir_path / "magic_numbers.bin", std::ios::binary);
	if (!out_file) return false;

	cereal::BinaryOutputArchive oarchive(out_file);
	oarchive(save_file);

	return true;
} 

SaveFile loadMagics() {
	SaveFile save_file;
	std::ifstream in_file;

	// Get file's directory
	std::filesystem::path dir_path = std::filesystem::path(__FILE__).parent_path();

	in_file.open(dir_path / "magic_numbers.bin", std::ios::binary);
	if (!in_file.is_open()) return save_file;

	cereal::BinaryInputArchive iarchive(in_file);
	iarchive(save_file);

	return save_file;
}
