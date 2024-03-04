#include "Zobrist.h"
#include "Locations.h"
#include "Position.h"
#include <array>
#include <climits>
#include <random>

template<size_t N>
static u64 addPiecesToHash(u64 hash, std::array<location, N> locations, u64* piece_hash);

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

	en_passant_file = &keys[768];

	is_black_to_move = distribution(generator);
	white_castle_king_side = distribution(generator);
	white_castle_queen_side= distribution(generator);
	black_castle_king_side = distribution(generator);
	black_castle_queen_side = distribution(generator);
}

u64 ZobristKeys::positionToHash(const Position& position) {
	u64 hash = 0;

	const Player& white = position.player.is_white ? position.player : position.opponent;
	const Player& black = position.player.is_white ? position.opponent : position.player;
	
	// Add all pieces to hash
	hash = addPiecesToHash(hash, white.locations.pawns, white_pawn);
	hash = addPiecesToHash(hash, white.locations.knights, white_knight);
	hash = addPiecesToHash(hash, white.locations.bishops, white_bishop);
	hash = addPiecesToHash(hash, white.locations.rooks, white_rook);
	hash = addPiecesToHash(hash, white.locations.queens, white_queen);
	hash = addPiecesToHash(hash, black.locations.pawns, black_pawn);
	hash = addPiecesToHash(hash, black.locations.knights, black_knight);
	hash = addPiecesToHash(hash, black.locations.bishops, black_bishop);
	hash = addPiecesToHash(hash, black.locations.rooks, black_rook);
	hash = addPiecesToHash(hash, black.locations.queens, black_queen);

	hash ^= white_king[white.locations.king];
	hash ^= black_king[black.locations.king];

	if (position.opponent.locations.en_passant_target != 0) hash ^= en_passant_file[position.opponent.locations.en_passant_target % 8];

	if (!position.player.is_white) hash ^= is_black_to_move;
	
	// Castling rights
	if (white.can_castle_king_side) hash ^= white_castle_king_side;
	if (white.can_castle_queen_side) hash ^= white_castle_queen_side;
	if (black.can_castle_king_side) hash ^= black_castle_king_side;
	if (black.can_castle_queen_side) hash ^= black_castle_queen_side;

	return hash;
}

template<size_t N>
static u64 addPiecesToHash(u64 hash, std::array<location, N> locations, u64* piece_hash) {
	for (location loc : locations) {
		if (loc == 0) continue;
		loc &= 0b111111;
		hash ^= piece_hash[loc];
	}

	return hash;
}
