#pragma once
#include <array>
#include <vector>

struct MagicBitboard {
	unsigned long long mask, magic_number;
	int num_shifts;
	unsigned long long* ptr_attacks_array;
};

struct MagicBitboards {
	std::array<unsigned long long, 64> knights_attacks_array = {};
	std::array<std::vector<unsigned short>, 64> knight_moves;
	std::array<unsigned long long, 64> king_attacks_array = {};
	std::array<std::vector<unsigned short>, 64> king_moves;
	std::array<MagicBitboard, 64> bishops_magic_bitboards;
	std::array<std::array<unsigned long long, 64>, 64> bishop_squares_uncheck = {};
	std::array<MagicBitboard, 64> rooks_magic_bitboards;
	std::array<std::array<unsigned long long, 64>, 64> rook_squares_uncheck = {};

	bool loadMagicBitboards();

private:
	std::vector<unsigned long long> sliding_attacks_array;

};

inline MagicBitboards magic_bitboards; // Global Magic Bitboards
