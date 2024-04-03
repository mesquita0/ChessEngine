#include "Zobrist.h"
#include "Locations.h"
#include "Position.h"
#include <array>
#include <bit>
#include <climits>
#include <random>

static uint64_t addPiecesToHash(uint64_t hash, uint64_t piece_bitboard, uint64_t* piece_hash);

ZobristKeys::ZobristKeys() {
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<uint64_t> distribution(0, ULLONG_MAX);

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

uint64_t ZobristKeys::positionToHash(const Player& player, const Player& opponent) const {
	uint64_t hash = 0;

	const Player& white = player.is_white ? player : opponent;
	const Player& black = player.is_white ? opponent : player;
	
	// Add all pieces to hash
	hash = addPiecesToHash(hash, white.bitboards.pawns, white_pawn);
	hash = addPiecesToHash(hash, white.bitboards.knights, white_knight);
	hash = addPiecesToHash(hash, white.bitboards.bishops, white_bishop);
	hash = addPiecesToHash(hash, white.bitboards.rooks, white_rook);
	hash = addPiecesToHash(hash, white.bitboards.queens, white_queen);
	hash = addPiecesToHash(hash, black.bitboards.pawns, black_pawn);
	hash = addPiecesToHash(hash, black.bitboards.knights, black_knight);
	hash = addPiecesToHash(hash, black.bitboards.bishops, black_bishop);
	hash = addPiecesToHash(hash, black.bitboards.rooks, black_rook);
	hash = addPiecesToHash(hash, black.bitboards.queens, black_queen);

	hash ^= white_king[white.locations.king];
	hash ^= black_king[black.locations.king];

	if (opponent.locations.en_passant_target != 0) hash ^= en_passant_file[opponent.locations.en_passant_target % 8];

	if (!player.is_white) hash ^= is_black_to_move;
	
	// Castling rights
	if (white.can_castle_king_side) hash ^= white_castle_king_side;
	if (white.can_castle_queen_side) hash ^= white_castle_queen_side;
	if (black.can_castle_king_side) hash ^= black_castle_king_side;
	if (black.can_castle_queen_side) hash ^= black_castle_queen_side;

	return hash;
}

static uint64_t addPiecesToHash(uint64_t hash, uint64_t piece_bitboard, uint64_t* piece_hash) {
	
	location loc = 0;
	while (piece_bitboard != 0 && loc <= 63) {
		int squares_to_skip = std::countr_zero(piece_bitboard);
		loc += squares_to_skip;

		hash ^= piece_hash[loc];

		piece_bitboard >>= (squares_to_skip + 1);
		loc++;
	}

	return hash;
}
