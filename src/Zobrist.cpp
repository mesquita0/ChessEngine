#include "Zobrist.h"
#include "Position.h"
#include <climits>
#include <random>

ZobristKeys::ZobristKeys() {
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<u64> distribution(0, ULLONG_MAX);

	for (auto& key : this->keys) {
		key = distribution(generator);
	}

	white_pawn = &keys[0];
	white_knight = &keys[64];
	white_bishop = &keys[128];
	white_rook = &keys[192];
	white_queen = &keys[256];
	white_king = &keys[320];

	black_pawn = &keys[384];
	black_knight = &keys[448];
	black_bishop = &keys[512];
	black_rook = &keys[576];
	black_queen = &keys[640];
	black_king = &keys[704];

	is_black_to_move = &keys[768];
}

u64 ZobristKeys::positionToHash(const Position& position) {

}

u64 ZobristKeys::makeMove(u64 current_hash, unsigned short move) {

}
