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
	std::unique_ptr<std::vector<std::pair<unsigned long long, unsigned long long>>> legalMovesPerBitboard;
};
