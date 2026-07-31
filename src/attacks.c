#include <assert.h>
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

bitboard
attacks_piece(ptype pt, square sq, bitboard occ)
{
	switch (pt)
	{
	case QUEEN:    return attacks_queen(sq, occ);
	case ROOK:     return attacks_rook(sq, occ);
	case BISHOP:   return attacks_bishop(sq, occ);
	case KNIGHT:   return attacks_knight(sq);
	case KING:     return attacks_king(sq);
	case PAWN:     assert(false && "Pawn attacks not implemented");
	case ALL:      return (bitboard) 0xFFFFFFFFFFFFFFFF;
	case NONE:
	default:       return (bitboard) 0;
	}
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
	attackers |= (sqbb(sq) >> 7 | sqbb(sq) >> 9) & (p->by_ptype[PAWN] & p->by_color[WHITE]);
	attackers |= (sqbb(sq) << 7 | sqbb(sq) << 9) & (p->by_ptype[PAWN] & p->by_color[BLACK]);

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
		(c == WHITE && (sqbb(sq) >> 7 | sqbb(sq) >> 9) & (p->by_ptype[PAWN] & p->by_color[c])) ||
		(c == BLACK && (sqbb(sq) << 7 | sqbb(sq) << 9) & (p->by_ptype[PAWN] & p->by_color[c]));
}
