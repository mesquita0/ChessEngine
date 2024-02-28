#include "Serializer.h"
#include "Square.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cstdlib>
#include <execution>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using std::array, std::cout, std::vector;

// Number of times the program will try to find a magic number for each square
constexpr int num_tries_per_square = 30'000'000;

std::unique_ptr<vector<std::pair<unsigned long long, unsigned long long>>> getLegalMovesPerBitboard(int square, const array<array<int, 8>, 8>& files, 
																									const array<array<int, 8>, 8>& ranks,
																									const array<vector<int>, 64>& indexes_blockers, 
																									unsigned long long mask, bool is_bishop);
Magic calculateMagicNumber(const vector<std::pair<unsigned long long, unsigned long long>>& legal_moves_per_bitboard, const int num_shifts);
unsigned long long randomMagicNumber();
bool shouldSave();
int addToAttacksArray(Square& square, vector<unsigned long long>& attacks_array, int next_available_index);
void printBoard(unsigned long long board);
void printMovementMasks(const array<unsigned long long, 64>& movement_masks);

int main() {
	/*
		This type of magic bitboards works with a fixed number of shifts for every square but leaving upper bits
		with all zeros, so that squares with less relevant bits take less space.
	*/
	constexpr int min_num_shifts_rooks = 52;
	constexpr int min_num_shifts_bishops = 55;

	cout << "Maximum number of bits used for rooks: " << (64 - min_num_shifts_rooks) << "\n";
	cout << "Maximum number of bits used for bishops: " << (64 - min_num_shifts_bishops) << "\n";
	Square a;
	array<Square, 64> squares_rooks = {};
	array<Square, 64> squares_bishops = {};
	array<vector<int>, 64> indexes_blockers_rooks;
	array<vector<int>, 64> indexes_blockers_bishops;

	array<array<int, 8>, 8> files;
	array<array<int, 8>, 8> ranks;

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			files[i][j] = j * 8 + i;
			ranks[j][i] = j * 8 + i;
		}
	}

	// Generate movement masks
	for (int square = 0; square < 64; square++) {
		int file =  square % 8; // 0 for a, 1 for b, ...
		int rank = (square - file) / 8; // 0 for 1st, 1 for 2nd, ...

		// Generate rooks movement mask
		for (int i = 1; i < 7; i++) {
			if (files[file][i] != square) {
				squares_rooks[square].mask |= (1LL << files[file][i]);
				indexes_blockers_rooks[square].push_back(files[file][i]);
			}

			if (ranks[rank][i] != square) {
				squares_rooks[square].mask |= (1LL << ranks[rank][i]);
				indexes_blockers_rooks[square].push_back(ranks[rank][i]);
			}
		}

		// Generate bishops movement mask
		int offsets[] = { -1, 1 };
		int tmp_file = file;
		int tmp_rank = rank;
		for (int offset_file : offsets) {
			for (int offset_rank : offsets) {
				tmp_file += offset_file;
				tmp_rank += offset_rank;
				while (tmp_file < 7 && tmp_file > 0 && tmp_rank < 7 && tmp_rank > 0) {
					squares_bishops[square].mask |= (1LL << files[tmp_file][tmp_rank]);
					indexes_blockers_bishops[square].push_back(files[tmp_file][tmp_rank]);
					tmp_file += offset_file;
					tmp_rank += offset_rank;
				}

				tmp_file = file;
				tmp_rank = rank;
				continue;
			}
		}

		// Invert movement mask, since using Volker Annuss' black magic bitboards
		squares_rooks[square].mask = ~squares_rooks[square].mask;
		squares_bishops[square].mask = ~squares_bishops[square].mask;
	}

	srand(unsigned(time(NULL)));
	size_t size_array_rooks = 0;
	size_t size_array_bishops = 0;

	// Generate rook magic numbers
	for (int square = 0; square < 64; square++) {
		squares_rooks[square].legalMovesPerBitboard = getLegalMovesPerBitboard(square, files, ranks, indexes_blockers_rooks, squares_rooks[square].mask, false);

		auto [magic_number, bits_used] = calculateMagicNumber(*squares_rooks[square].legalMovesPerBitboard, min_num_shifts_rooks);
		if (magic_number) {
			squares_rooks[square].magic = { magic_number, bits_used };
			cout << "Magic Number Rook Square " << square << ": " << magic_number << ", used " << bits_used << " bits." << "\n";
			size_array_rooks += (1LL << bits_used) * sizeof(unsigned long long);
		}
		else {
			cout << "Coundn't find magic number for Rook at square: " << square << "\n";
		}
	}

	// Generate bishop magic number
	for (int square = 0; square < 64; square++) {
		squares_bishops[square].legalMovesPerBitboard = getLegalMovesPerBitboard(square, files, ranks, indexes_blockers_bishops, squares_bishops[square].mask, true);

		auto [magic_number, bits_used] = calculateMagicNumber(*squares_bishops[square].legalMovesPerBitboard, min_num_shifts_bishops);
		if (magic_number) {
			squares_bishops[square].magic = { magic_number, bits_used };
			cout << "Magic Number Bishop Square " << square << ": " << magic_number << ", used " << bits_used << " bits." << "\n";
			size_array_bishops += (1LL << bits_used) * sizeof(unsigned long long);
		}
		else {
			cout << "Coundn't find magic number for Bishop at square: " << square << "\n";
		}
	}
	
	// Print array size for rooks and bishops
	double f_size_array_rooks = size_array_rooks / 1024.0;
	std::string unit_rooks = "KiB";
	if (f_size_array_rooks > 1024) {
		f_size_array_rooks /= 1024;
		unit_rooks = "Mib";
	}
	cout << "Size array rooks: " << f_size_array_rooks << unit_rooks << "\n";

	double f_size_array_bishops = size_array_bishops / 1024.0;
	std::string unit_bishops = "KiB";
	if (f_size_array_bishops > 1024) {
		f_size_array_bishops /= 1024;
		unit_bishops = "Mib";
	}
	cout << "Size array bishops: " << f_size_array_bishops << unit_bishops << "\n";

	if (shouldSave()) {
		// Generate combined attacks array
		vector<unsigned long long> attacks_array;
		int next_available_index = 0;

		for (Square& square : squares_rooks) {
			next_available_index = addToAttacksArray(square, attacks_array, next_available_index);
		}

		for (Square& square : squares_bishops) {
			next_available_index = addToAttacksArray(square, attacks_array, next_available_index);
		}

		// Save to file 
		if (storeMagics(squares_bishops, squares_rooks, attacks_array)) {
			cout << "Saved magic numbers to file magic_numbers.\n";
		}
		else {
			cout << "Couldn't save magic numbers to file.\n";
		}
	}
}

std::unique_ptr<vector<std::pair<unsigned long long, unsigned long long>>> getLegalMovesPerBitboard(int square, const array<array<int, 8>, 8>& files, 
																									const array<array<int, 8>, 8>& ranks,
																									const array<vector<int>, 64>& indexes_blockers, 
																									unsigned long long mask, bool is_bishop) {
	auto legal_moves_per_bitboard = std::make_unique<vector<std::pair<unsigned long long, unsigned long long>>>();
	int num_possible_bitboards = 1 << indexes_blockers[square].size();
	(*legal_moves_per_bitboard).reserve(num_possible_bitboards);

	// Generate all possible bitboards of blockers
	for (int i = 0; i < num_possible_bitboards; i++) {
		unsigned long long new_bitboard = 0;

		// Put bits of i in positions of blockers, since i goes from 0 to
		// 2^number of blockers every possible arrangement of blockers will
		// be considered.
		for (int j = 0; j < indexes_blockers[square].size(); j++) {
			unsigned long long bit = (i >> j) & 1; // Get jth bit of i
			new_bitboard |= bit << indexes_blockers[square][j];
		}

		unsigned long long legal_moves = 0;
		int file = square % 8; // 0 for a, 1 for b, ...
		int rank = (square - file) / 8; // 0 for 1st, 1 for 2nd, ...

		// Compute legal moves for bitboard
		if (is_bishop) {
			int offsets[] = { -1, 1 };
			int tmp_file = file;
			int tmp_rank = rank;

			// Go in every direction until a block is hit
			for (int offset_file : offsets) {
				for (int offset_rank : offsets) {
					tmp_file += offset_file;
					tmp_rank += offset_rank;

					while (tmp_file <= 7 && tmp_file >= 0 && tmp_rank <= 7 && tmp_rank >= 0) {
						int square_to_move = files[tmp_file][tmp_rank];
						legal_moves |= (1LL << square_to_move);
						tmp_file += offset_file;
						tmp_rank += offset_rank;

						// Stop loop when a block is hit
						if (new_bitboard & (1LL << square_to_move)) break;
					}

					tmp_file = file;
					tmp_rank = rank;
					continue;
				}
			}
		}
		else {
			// Moves to the right
			int new_file = file + 1;
			while (new_file <= 7) {
				int square_to_move = files[new_file][rank];
				legal_moves |= (1LL << square_to_move);

				// Stop loop when a block is hit
				if (new_bitboard & (1LL << square_to_move)) break;

				new_file++;
			}

			// Moves to the left
			new_file = file - 1;
			while (new_file >= 0) {
				int square_to_move = files[new_file][rank];
				legal_moves |= (1LL << square_to_move);

				// Stop loop when a block is hit
				if (new_bitboard & (1LL << square_to_move)) break;

				new_file--;
			}

			// Moves up
			int new_rank = rank + 1;
			while (new_rank <= 7) {
				int square_to_move = files[file][new_rank];
				legal_moves |= (1LL << square_to_move);

				// Stop loop when a block is hit
				if (new_bitboard & (1LL << square_to_move)) break;

				new_rank++;
			}

			// Moves down
			new_rank = rank - 1;
			while (new_rank >= 0) {
				int square_to_move = files[file][new_rank];
				legal_moves |= (1LL << square_to_move);

				// Stop loop when a block is hit
				if (new_bitboard & (1LL << square_to_move)) break;

				new_rank--;
			}
		}

		(*legal_moves_per_bitboard).emplace_back(new_bitboard | mask, legal_moves);
	}

	return legal_moves_per_bitboard;
}

Magic calculateMagicNumber(const vector<std::pair<unsigned long long, unsigned long long>>& legal_moves_per_bitboard, const int num_shifts) {
	std::atomic<unsigned long long> magic_number(0);
	vector<bool> tries(num_tries_per_square);

	std::for_each(std::execution::par_unseq, tries.begin(), tries.end(), [&](bool _) {
		/*
			For each possible bitboard:
				Compute magic number * bitboard >> num_shifts
				if result was alredy found and corresponding legal moves are different: skip magic
		*/
		if (magic_number) return;
		unsigned long long tmp_magic_number = randomMagicNumber();
		vector<unsigned long long> index_to_legal_moves(1LL << (64 - num_shifts));

		for (auto& [bitboard, legal_moves] : legal_moves_per_bitboard) {
			int index = (tmp_magic_number * bitboard) >> num_shifts;
			
			if (index_to_legal_moves[index] == 0) {
				index_to_legal_moves[index] = legal_moves;
			}
			else if (index_to_legal_moves[index] != legal_moves) return;
		}

		magic_number = tmp_magic_number;
	});

	// Try to get better magic number
	if (magic_number) {
		auto [new_magic_number, bits_used] = calculateMagicNumber(legal_moves_per_bitboard, num_shifts + 1);
		if (new_magic_number) {
			return { new_magic_number, bits_used };
		}
	}

	return { magic_number, 64 - num_shifts };
}

unsigned long long randomMagicNumber() {
	/*
		10% chance for each index (from 0 to 7), representing the bits among the first
		8 that will get 0 on the final number, this garantees that at least 6 of the first
		8 bits are one. 20% chance to get a number outside valid range so that there can be
		more than 6 bits with 1's in the first 8 bits of the number.
	*/
	unsigned long long p1 = rand() % 10;
	unsigned long long p2 = rand() % 10;
	unsigned long long p3 = rand() % 10;

	unsigned long long x = (1LL << (p1 + 56));
	unsigned long long y = (1LL << (p2 + 56));
	unsigned long long z = (1LL << (p3 + 56));

	unsigned long long n1 = 0xff00000000000000 ^ x ^ y ^ z;
	n1 |= (rand() >> 8);

	// AND 3 rands so that there's a low number of 1's in the rest of the number
	unsigned long long n2 = rand() & rand() & rand();
	unsigned long long n3 = rand() & rand() & rand();
	unsigned long long n4 = rand() & rand() & rand();
	return n1 | (n2 << 32) | (n3 << 16) | n4;
}

bool shouldSave() {
	bool should_save;
	std::string ans;
	while (true) {
		cout << "Save arrays to file? (Y/n) ";
		std::cin >> ans;
		std::transform(ans.begin(), ans.end(), ans.begin(), std::tolower);
		if (ans == "yes" || ans == "y") {
			should_save = true;
			break;
		}
		else if (ans == "no" || ans == "n") {
			should_save = false;
			break;
		}
	}

	return should_save;
}

int addToAttacksArray(Square& square, vector<unsigned long long>& attacks_array, int next_available_index) {
	int square_array_length = 1 << square.magic.bits_used;
	attacks_array.resize(next_available_index + square_array_length);
	square.index_attack_array = next_available_index;

	for (auto& [bitboard, legal_moves] : *square.legalMovesPerBitboard) {
		int index = (square.magic.magic_number * bitboard) >> (64 - square.magic.bits_used);
		attacks_array[square.index_attack_array + index] = legal_moves;
	}

	// Update next available index
	for (int i = attacks_array.size() - 1; i >= 0; i--) {
		if (attacks_array[i] != 0) {
			next_available_index = i + 1;
			break;
		}
	}

	return next_available_index;
}

void printBoard(unsigned long long board) {
	std::string board_str = std::bitset<64>(board).to_string();
	cout << "   a b c d e f g h\n";

	for (int i = 7; i >= 0; i--) {
		cout << (i + 1) << ". ";
		for (int j = 0; j < 8; j++) {
			cout << board_str[63 - (i*8 + j)] << " "; // Least significant bit is a1, then a2 ...
		}
		cout << "\n";
	}
}

void printMovementMasks(const array<unsigned long long, 64>& movement_masks) {
	for (int square = 0; square < 64; square++) {
		cout << "Mask at square " << square << ":\n";
		printBoard(movement_masks[square]); 
		cout << "\n";
	}
}
