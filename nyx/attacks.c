#include <nyx/attacks.h>
#include <nyx/types.h>

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

void
attacks_init(void)
{
	square sq;
	bitboard ray, bb;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
	{
		for (ray = sqbb(sq); ray; ray = ea(ray)) rank_mask[sq] |= ray;
		for (ray = sqbb(sq); ray; ray = we(ray)) rank_mask[sq] |= ray;

		for (ray = sqbb(sq); ray; ray = no(ray)) file_mask[sq] |= ray;
		for (ray = sqbb(sq); ray; ray = so(ray)) file_mask[sq] |= ray;

		for (ray = sqbb(sq); ray; ray = no(ea(ray))) diag_mask[sq] |= ray;
		for (ray = sqbb(sq); ray; ray = so(we(ray))) diag_mask[sq] |= ray;

		for (ray = sqbb(sq); ray; ray = no(we(ray))) anti_mask[sq] |= ray;
		for (ray = sqbb(sq); ray; ray = so(ea(ray))) anti_mask[sq] |= ray;

		bb = sqbb(sq);

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
	}
}

static bitboard
reverse(bitboard bb)
{
	// TODO: make faster
	size_t i;
	bitboard res;

	for (i = 0; i < 64; ++i)
	{
		res <<= 1;
		res |= bb & 1;
		bb >>= 1;
	}

	return res;
}

static bitboard
hyperbola(square sq, bitboard occ, bitboard mask)
{
	bitboard occr;
	bitboard fwd, rev;

	occ  = occ & mask;
	occr = reverse(occ);

	fwd = occ  - (2 * sqbb(sq));
	rev = occr - (2 * reverse(sqbb(sq)));

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

bitboard
attacks_queen(square sq, bitboard occ)
{
	return
		attacks_rook  (sq, occ) |
		attacks_bishop(sq, occ);
}

bitboard attacks_knight(square sq)
{
	return lut_knight_attacks[sq];
}

bitboard attacks_king(square sq)
{
	return lut_king_attacks[sq];
}
