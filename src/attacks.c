// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/attacks.h>
#include <nyx/types.h>
#include <nyx/utils.h>

#ifdef __BMI2__
#include <x86intrin.h>
#endif // __BMI2__

static bitboard rank_mask[NUM_SQUARES];
static bitboard file_mask[NUM_SQUARES];
static bitboard diag_mask[NUM_SQUARES];
static bitboard anti_mask[NUM_SQUARES];

static bitboard lut_knight_attacks[NUM_SQUARES];
static bitboard lut_king_attacks  [NUM_SQUARES];

static inline bitboard no(bitboard bb) { return (bb << 8); }
static inline bitboard so(bitboard bb) { return (bb >> 8); }
static inline bitboard ea(bitboard bb) { return ((bb << 1) & 0xFEFEFEFEFEFEFEFE); }
static inline bitboard we(bitboard bb) { return ((bb >> 1) & 0x7F7F7F7F7F7F7F7F); }

#ifdef __BMI2__
static bitboard lut_rook_attacks  [102400];
static bitboard lut_bishop_attacks[5248];
static bitboard lut_rook_mask     [NUM_SQUARES];
static bitboard lut_rook_offset   [NUM_SQUARES];
static bitboard lut_bishop_mask   [NUM_SQUARES];
static bitboard lut_bishop_offset [NUM_SQUARES];

constexpr bitboard NO_EDGES     = 0x007E7E7E7E7E7E00;
constexpr bitboard RANK_ATTACKS = 0x000000000000007E;
constexpr bitboard FILE_ATTACKS = 0x0001010101010100;

static bitboard
calc_rook_attacks(bitboard occ, square sq)
{
	bitboard ray, attacks;

	attacks = EMPTYBB;

	for (ray = bbsq(sq); ray; ray &= ~occ) ray = ea(ray), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = we(ray), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = no(ray), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = so(ray), attacks |= ray;

	return attacks;
}

static bitboard
calc_bishop_attacks(bitboard occ, square sq)
{
	bitboard ray, attacks;

	attacks = EMPTYBB;

	for (ray = bbsq(sq); ray; ray &= ~occ) ray = no(ea(ray)), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = so(we(ray)), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = no(we(ray)), attacks |= ray;
	for (ray = bbsq(sq); ray; ray &= ~occ) ray = so(ea(ray)), attacks |= ray;

	return attacks;
}
#endif // __BMI2__

void
attacks_init(void)
{
	square sq;
	bitboard ray, bb;
#ifdef __BMI2__
	usize rook_offset, bishop_offset;
	usize rook_popcnt, bishop_popcnt;

	rook_offset = bishop_offset = 0;
#endif // __BMI2__

	for (sq = 0; sq < NUM_SQUARES; ++sq)
	{
		for (ray = bbsq(sq); ray; ray = ea(ray)) rank_mask[sq] |= ray;
		for (ray = bbsq(sq); ray; ray = we(ray)) rank_mask[sq] |= ray;

		for (ray = bbsq(sq); ray; ray = no(ray)) file_mask[sq] |= ray;
		for (ray = bbsq(sq); ray; ray = so(ray)) file_mask[sq] |= ray;

		for (ray = bbsq(sq); ray; ray = no(ea(ray))) diag_mask[sq] |= ray;
		for (ray = bbsq(sq); ray; ray = so(we(ray))) diag_mask[sq] |= ray;

		for (ray = bbsq(sq); ray; ray = no(we(ray))) anti_mask[sq] |= ray;
		for (ray = bbsq(sq); ray; ray = so(ea(ray))) anti_mask[sq] |= ray;

		bb = bbsq(sq);

		lut_knight_attacks[sq] |= no(no(ea(bb)));
		lut_knight_attacks[sq] |= no(ea(ea(bb)));
		lut_knight_attacks[sq] |= so(ea(ea(bb)));
		lut_knight_attacks[sq] |= so(so(ea(bb)));
		lut_knight_attacks[sq] |= no(no(we(bb)));
		lut_knight_attacks[sq] |= no(we(we(bb)));
		lut_knight_attacks[sq] |= so(we(we(bb)));
		lut_knight_attacks[sq] |= so(so(we(bb)));

		lut_king_attacks[sq] |= no(bb);
		lut_king_attacks[sq] |= so(bb);
		lut_king_attacks[sq] |= ea(bb);
		lut_king_attacks[sq] |= we(bb);
		lut_king_attacks[sq] |= no(ea(bb));
		lut_king_attacks[sq] |= so(ea(bb));
		lut_king_attacks[sq] |= no(we(bb));
		lut_king_attacks[sq] |= so(we(bb));

#ifdef __BMI2__
		lut_rook_offset[sq] = rook_offset;
		lut_rook_mask[sq] = (RANK_ATTACKS << (rank_of(sq) * 8)) | (FILE_ATTACKS << file_of(sq));
		lut_rook_mask[sq] &= ~bbsq(sq);
		rook_popcnt = popcnt(lut_rook_mask[sq]);
		for (bb = 0; bb < (1ull << rook_popcnt); ++bb)
			lut_rook_attacks[rook_offset++] = calc_rook_attacks(_pdep_u64(bb, lut_rook_mask[sq]), sq);

		lut_bishop_offset[sq] = bishop_offset;
		lut_bishop_mask[sq] = diag_mask[sq] ^ anti_mask[sq];
		lut_bishop_mask[sq] &= NO_EDGES;
		bishop_popcnt = popcnt(lut_bishop_mask[sq]);
		for (bb = 0; bb < (1ull << bishop_popcnt); ++bb)
			lut_bishop_attacks[bishop_offset++] = calc_bishop_attacks(_pdep_u64(bb, lut_bishop_mask[sq]), sq);
#endif // __BMI2__
	}
}

#ifdef __BMI2__

bitboard
attacks_bishop(square sq, bitboard occ)
{
	return lut_bishop_attacks[lut_bishop_offset[sq] + _pext_u64(occ, lut_bishop_mask[sq])];
}

bitboard
attacks_rook(square sq, bitboard occ)
{
	return lut_rook_attacks[lut_rook_offset[sq] + _pext_u64(occ, lut_rook_mask[sq])];
}

#else // __BMI2__

static bitboard
reverse(bitboard bb)
{
	bb = ((bb >> 1)  & 0x5555555555555555ull) | ((bb & 0x5555555555555555ull) << 1);
	bb = ((bb >> 2)  & 0x3333333333333333ull) | ((bb & 0x3333333333333333ull) << 2);
	bb = ((bb >> 4)  & 0x0F0F0F0F0F0F0F0Full) | ((bb & 0x0F0F0F0F0F0F0F0Full) << 4);
	bb = ((bb >> 8)  & 0x00FF00FF00FF00FFull) | ((bb & 0x00FF00FF00FF00FFull) << 8);
	bb = ((bb >> 16) & 0x0000FFFF0000FFFFull) | ((bb & 0x0000FFFF0000FFFFull) << 16);
	return (bb >> 32) | (bb << 32);
}

static bitboard
hyperbola(square sq, bitboard occ, bitboard mask)
{
	bitboard occr;
	bitboard fwd, rev;

	occ  = occ & mask;
	occr = reverse(occ);

	fwd = occ  - (2 * bbsq(sq));
	rev = occr - (2 * reverse(bbsq(sq)));

	return ((fwd ^ reverse(rev)) & mask);
}

bitboard
attacks_bishop(square sq, bitboard occ)
{
	return
		hyperbola(sq, occ, diag_mask[sq]) |
		hyperbola(sq, occ, anti_mask[sq]);
}

bitboard
attacks_rook(square sq, bitboard occ)
{
	return
		hyperbola(sq, occ, rank_mask[sq]) |
		hyperbola(sq, occ, file_mask[sq]);
}

#endif // __BMI2__

bitboard
attacks_queen(square sq, bitboard occ)
{
	return
		attacks_rook  (sq, occ) |
		attacks_bishop(sq, occ);
}

bitboard
attacks_knight(square sq)
{
	return lut_knight_attacks[sq];
}

bitboard
attacks_king(square sq)
{
	return lut_king_attacks[sq];
}

static bitboard
attacks_white_pawn(square sq)
{
	return no(ea(bbsq(sq))) | no(we(bbsq(sq)));
}

static bitboard
attacks_black_pawn(square sq)
{
	return so(ea(bbsq(sq))) | so(we(bbsq(sq)));
}

bitboard
attacks_pawn(square sq, color c)
{
	return white_black(attacks_white_pawn(sq), attacks_black_pawn(sq), c);
}

bitboard
attacks_piece(ptype pt, square sq, bitboard occ)
{
	switch (pt)
	{
	case QUEEN:  return attacks_queen(sq, occ);
	case ROOK:   return attacks_rook(sq, occ);
	case BISHOP: return attacks_bishop(sq, occ);
	case KNIGHT: return attacks_knight(sq);
	case KING:   return attacks_king(sq);
	case PAWN:   assert(false && "Pawn attacks not implemented here");
	case ALL:    return FULLBB;
	case NONE:   return EMPTYBB;
	}

	assert(false);
}

bitboard
attackers(const position *p, square sq)
{
	bitboard attackers;

	attackers = 0;

	attackers |= attacks_piece(ROOK  , sq, p->by_ptype[ALL]) & p->by_ptype[ROOK];
	attackers |= attacks_piece(ROOK  , sq, p->by_ptype[ALL]) & p->by_ptype[QUEEN];
	attackers |= attacks_piece(BISHOP, sq, p->by_ptype[ALL]) & p->by_ptype[BISHOP];
	attackers |= attacks_piece(BISHOP, sq, p->by_ptype[ALL]) & p->by_ptype[QUEEN];
	attackers |= attacks_piece(KNIGHT, sq, p->by_ptype[ALL]) & p->by_ptype[KNIGHT];
	attackers |= attacks_piece(KING  , sq, p->by_ptype[ALL]) & p->by_ptype[KING];
	attackers |= attacks_pawn(sq, BLACK) & p->by_ptype[PAWN] & p->by_color[WHITE];
	attackers |= attacks_pawn(sq, WHITE) & p->by_ptype[PAWN] & p->by_color[BLACK];

	return attackers;
}

bool
attackers_exist(const position *p, square sq, bitboard occ, color c)
{
	return
		attacks_piece(ROOK  , sq, occ) & p->by_color[c] & p->by_ptype[ROOK] ||
		attacks_piece(ROOK  , sq, occ) & p->by_color[c] & p->by_ptype[QUEEN] ||
		attacks_piece(BISHOP, sq, occ) & p->by_color[c] & p->by_ptype[BISHOP] ||
		attacks_piece(BISHOP, sq, occ) & p->by_color[c] & p->by_ptype[QUEEN] ||
		attacks_piece(KNIGHT, sq, occ) & p->by_color[c] & p->by_ptype[KNIGHT] ||
		attacks_piece(KING  , sq, occ) & p->by_color[c] & p->by_ptype[KING] ||
		attacks_pawn(sq, other_color(c)) & p->by_color[c] & p->by_ptype[PAWN];
}
