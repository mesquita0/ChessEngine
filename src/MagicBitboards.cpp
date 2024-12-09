#include "MagicBitboards.h"
#include "Moves.h"
#include "./Generate Magic Bitboards/Serializer.h"
#include <algorithm>
#include <array>

constexpr std::array<short, 8> knight_moves_offsets = { -17, -15, -10, -6, 6, 10, 15, 17 };
constexpr std::array<short, 8> knight_moves_offsets_a_file = { -15, -6, 10, 17 };
constexpr std::array<short, 8> knight_moves_offsets_b_file = { -17, -15, -6, 10, 15, 17 };
constexpr std::array<short, 8> knight_moves_offsets_g_file = { -17, -15, -10, 6, 15, 17 };
constexpr std::array<short, 8> knight_moves_offsets_h_file = { -17, -10, 6, 15 };

constexpr std::array<short, 8> king_moves_offsets = { -9, -8, -7, -1, 1, 7, 8, 9 };
constexpr std::array<short, 8> king_moves_offsets_right_edge = { -9, -8, -1, 7, 8 };
constexpr std::array<short, 8> king_moves_offsets_left_edge = { -8, -7, 1, 8, 9 };

bool MagicBitboards::loadMagicBitboards() {

	// Load pre compted magic numbers for sliding pieces
	SaveFile save_file = loadMagics();
	if (save_file.attacks_array.size() == 0) return false;

	this->sliding_attacks_array = save_file.attacks_array;

	std::transform(save_file.squares_bishops.begin(), save_file.squares_bishops.end(), this->bishops_magic_bitboards.begin(), [&](Square& square) {
		return MagicBitboard(square.mask, square.magic.magic_number, 64 - square.magic.bits_used, &this->sliding_attacks_array[square.index_attack_array]);
	});

	std::transform(save_file.squares_rooks.begin(), save_file.squares_rooks.end(), this->rooks_magic_bitboards.begin(), [&](Square& square) {
		return MagicBitboard(square.mask, square.magic.magic_number, 64 - square.magic.bits_used, &this->sliding_attacks_array[square.index_attack_array]);
	});

	// Pre compute knight moves
	for (int start_square = 0; start_square < 64; start_square++) {
		const std::array<short, 8>* offsets;
		short column = start_square % 8;
		switch (column) {
		case 0:
			offsets = &knight_moves_offsets_a_file;
			break;

		case 1:
			offsets = &knight_moves_offsets_b_file;
			break;

		case 6:
			offsets = &knight_moves_offsets_g_file;
			break;

		case 7:
			offsets = &knight_moves_offsets_h_file;
			break;

		default:
			offsets = &knight_moves_offsets;
			break;
		}

		for (short offset : *offsets) {
			location final_square = start_square + offset;
			if (final_square >= 0 && final_square <= 63 && offset != 0) {
				this->knights_attacks_array[start_square] |= (1LL << (final_square));
				this->knight_moves[start_square].push_back((start_square << 6) | final_square);
			}
		}
	}

	// Pre compute king moves
	for (int start_square = 0; start_square < 64; start_square++) {
		const std::array<short, 8>* offsets;
		short column = start_square % 8;
		if (column == 0) {
			offsets = &king_moves_offsets_left_edge;
		}
		else if (column == 7) {
			offsets = &king_moves_offsets_right_edge;
		}
		else {
			offsets = &king_moves_offsets;
		}

		for (short offset : *offsets) {
			location final_square = start_square + offset;
			if (final_square >= 0 && final_square <= 63 && offset != 0) {
				this->king_attacks_array[start_square] |= (1LL << (final_square));
				this->king_moves[start_square].push_back((start_square << 6) | final_square);
			}
		}
	}

	// Pre compute squares to uncheck
	for (int square_piece = 0; square_piece < 64; square_piece++) {
		for (int square_king = 0; square_king < 64; square_king++) {

			unsigned long long bit_board_king = (1LL << square_king);
			int index_bishop = (bishops_magic_bitboards[square_piece].mask * bishops_magic_bitboards[square_piece].magic_number) >> bishops_magic_bitboards[square_piece].num_shifts;
			int index_rook = (rooks_magic_bitboards[square_piece].mask * rooks_magic_bitboards[square_piece].magic_number) >> rooks_magic_bitboards[square_piece].num_shifts;

			// If king is in check by bishop
			if (bishops_magic_bitboards[square_piece].ptr_attacks_array[index_bishop] & bit_board_king) {
				int file_king = square_king % 8;
				int file_bishop = square_piece % 8;

				int offset_rank = (square_piece > square_king) ? -1 : 1;
				int offset_file = (file_bishop > file_king) ? -1 : 1;

				location square_to_uncheck = square_piece;
				while (square_to_uncheck != square_king) {
					bishop_squares_uncheck[square_piece][square_king] |= (1LL << square_to_uncheck);
					square_to_uncheck += (offset_file + 8 * offset_rank);
				}
			}

			// If king is in check by rook
			if (rooks_magic_bitboards[square_piece].ptr_attacks_array[index_rook] & bit_board_king) {
				int distance = square_piece - square_king;
				int offset;

				if (distance % 8 == 0) {
					if (distance > 0) {
						offset = -8;
					}
					else {
						offset = 8;
					}
				}
				else {
					if (distance > 0) {
						offset = -1;
					}
					else {
						offset = 1;
					}
				}

				location square_to_uncheck = square_piece;
				while (square_to_uncheck != square_king) {
					rook_squares_uncheck[square_piece][square_king] |= (1LL << square_to_uncheck);
					square_to_uncheck += offset;
				}
			}
		}
	}

	return true;
}
