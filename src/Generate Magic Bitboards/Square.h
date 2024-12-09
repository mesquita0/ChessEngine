#pragma once
#include <memory>
#include <vector>
#include <utility>

struct Magic {
	unsigned long long magic_number;
	int bits_used;
};

struct Square {
	unsigned long long mask;
	Magic magic;
	int index_attack_array;

	// Only for computational purposes, do not need to store in file
	std::unique_ptr<std::vector<std::pair<unsigned long long, unsigned long long>>> legalMovesPerBitboard = nullptr;

	Square& operator=(Square& other) {
		mask = other.mask;
		magic.magic_number = other.magic.magic_number;
		magic.bits_used = other.magic.bits_used;
		index_attack_array = other.index_attack_array;

		return *this;
	}
};
