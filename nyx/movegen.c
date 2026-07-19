#include <nyx/position.h>
#include <nyx/utils.h>
#include <assert.h>
#include <nyx/attacks.h>
#include <nyx/movegen.h>
#include <nyx/types.h>

static square
king_square(const position *p, color c)
{
	return lsb(p->by_ptype[KING] & p->by_color[c]);
}

static size_t
splat_moves(square from, bitboard target, move *ms)
{
	size_t num_moves;
	square to;

	for (num_moves = 0; target; ++num_moves)
	{
		to = pop_lsb(&target);

		ms[num_moves] = (move)
		{
			.from=from,
			.to  =to,
		};
	}

	return num_moves;
}

static size_t
generate_piece_moves(const position *p, ptype pt, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard bb, moves;
	square sq;

	num_moves = 0;
	bb = p->by_ptype[pt] & p->by_color[p->stm];

	while (bb)
	{
		sq = pop_lsb(&bb);
		moves = attacks_piece(pt, sq, p->by_ptype[ALL]) & target;
		num_moves += splat_moves(sq, moves, ms + num_moves);
	}

	return num_moves;
}

static constexpr bitboard RANK_1 = 0xFF;
static constexpr bitboard RANK_8 = 0xFF00000000000000;

static size_t
generate_pawn_quiet_promotion_moves(const position *p, bitboard target, ptype promtype, move *ms)
{
	size_t num_moves;
	bitboard promotions;
	square from, to;

	promotions = p->by_ptype[PAWN] & p->by_color[p->stm];
	promotions = white_black(promotions << 8, promotions >> 8, p->stm);
	promotions &= RANK_1 | RANK_8;
	promotions &= target;

	for (num_moves = 0; promotions; ++num_moves)
	{
		to = pop_lsb(&promotions);
		from = to + white_black(-8, +8, p->stm);
		ms[num_moves] = (move)
		{
			.from=from,
			.to  =to,
			.type=PROMOTION,
			.promotion=promtype_of(promtype),
		};
	}

	return num_moves;
}

// pawn promotions: quiet knight, quiet bishop
static size_t
generate_quiet_pawn_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard singles, doubles;
	square from, to;

	singles = p->by_ptype[PAWN] & p->by_color[p->stm];
	singles = white_black(singles << 8, singles >> 8, p->stm);
	singles &= target;
	singles &= ~(RANK_1 | RANK_8);

	doubles = white_black((singles << 8) & 0xFF000000ull, (singles >> 8) & 0xFF00000000ull, p->stm);
	doubles &= target;

	num_moves = 0;
	num_moves += generate_pawn_quiet_promotion_moves(p, target, KNIGHT, ms + num_moves);
	num_moves += generate_pawn_quiet_promotion_moves(p, target, BISHOP, ms + num_moves);

	while (singles)
	{
		to = pop_lsb(&singles);
		from = to + white_black(-8, +8, p->stm);
		ms[num_moves++] = (move) { .from=from, .to=to };
	}

	while (doubles)
	{
		to = pop_lsb(&doubles);
		from = to + white_black(-16, +16, p->stm);
		ms[num_moves++] = (move) { .from=from, .to=to };
	}

	return num_moves;
}

static size_t
pawn_all_promotions(square from, square to, move *ms)
{
	ms[0] = (move) { .from=from, .to=to, .type=PROMOTION, .promotion=promtype_of(QUEEN ) };
	ms[1] = (move) { .from=from, .to=to, .type=PROMOTION, .promotion=promtype_of(KNIGHT) };
	ms[2] = (move) { .from=from, .to=to, .type=PROMOTION, .promotion=promtype_of(ROOK  ) };
	ms[3] = (move) { .from=from, .to=to, .type=PROMOTION, .promotion=promtype_of(BISHOP) };

	return 4;
}

// pawn promotions += quiet queen
static size_t
generate_capture_pawn_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard singles, east, west, east_prom, west_prom, ep;
	square from, to;

	singles = p->by_ptype[PAWN] & p->by_color[p->stm];
	singles = white_black(singles << 8, singles >> 8, p->stm);
	east = (singles << 1) & 0xFEFEFEFEFEFEFEFEull & target;
	west = (singles >> 1) & 0x7F7F7F7F7F7F7F7Full & target;
	east_prom = east & (RANK_1 | RANK_8);
	west_prom = west & (RANK_1 | RANK_8);
	east ^= east_prom;
	west ^= west_prom;

	num_moves = 0;
	num_moves += generate_pawn_quiet_promotion_moves(p, target, QUEEN, ms + num_moves);

	while (east_prom)
	{
		to = pop_lsb(&east_prom);
		from = to + white_black(-9, +7, p->stm);
		num_moves += pawn_all_promotions(from, to, ms + num_moves);
	}

	while (west_prom)
	{
		to = pop_lsb(&west_prom);
		from = to + white_black(-7, +9, p->stm);
		num_moves += pawn_all_promotions(from, to, ms + num_moves);
	}

	while (east)
	{
		to = pop_lsb(&east);
		from = to + white_black(-9, +7, p->stm);
		ms[num_moves++] = (move) { .from=from, .to=to };
	}

	while (west)
	{
		to = pop_lsb(&west);
		from = to + white_black(-7, +9, p->stm);
		ms[num_moves++] = (move) { .from=from, .to=to };
	}

	if (p->sf->ep != NO_EP)
	{
		ep = sqbb(p->sf->ep) & target;
		if (!ep) return num_moves;

		east = (ep << 1) & 0xFEFEFEFEFEFEFEFEull;
		west = (ep >> 1) & 0x7F7F7F7F7F7F7F7Full;
		to = p->sf->ep;

		if (singles & east)
		{
			from = lsb(east) + white_black(-8, +8, p->stm);
			ms[num_moves++] = (move)
			{
				.from=from,
				.to  =to,
				.type=EN_PASSANT,
			};
		}

		if (singles & west)
		{
			from = lsb(west) + white_black(-8, +8, p->stm);
			ms[num_moves++] = (move)
			{
				.from=lsb(west),
				.to  =to,
				.type=EN_PASSANT,
			};
		}
	}

	return num_moves;
}

static size_t
generate_all_piece_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;

	num_moves = 0;
	num_moves += generate_piece_moves(p, KNIGHT, target, ms + num_moves);
	num_moves += generate_piece_moves(p, BISHOP, target, ms + num_moves);
	num_moves += generate_piece_moves(p, ROOK  , target, ms + num_moves);
	num_moves += generate_piece_moves(p, QUEEN , target, ms + num_moves);
	num_moves += generate_piece_moves(p, KING  , target, ms + num_moves);

	return num_moves;
}

size_t
generate_captures(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	target = p->by_color[other_color(p->stm)];

	num_moves = 0;
	num_moves += generate_all_piece_moves   (p, target, ms + num_moves);
	num_moves += generate_capture_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

size_t
generate_quiets(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	target = ~p->by_ptype[ALL];

	num_moves = 0;
	num_moves += generate_all_piece_moves (p, target, ms + num_moves);
	num_moves += generate_quiet_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

static bitboard between_lut[NUM_SQUARES][NUM_SQUARES];
size_t
generate_evasions(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	target = between_lut[king_square(p, p->stm)][lsb(p->sf->checkers)];

	num_moves = 0;
	num_moves += generate_all_piece_moves   (p, target, ms + num_moves);
	num_moves += generate_quiet_pawn_moves  (p, target, ms + num_moves);
	num_moves += generate_capture_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

size_t
generate_non_evasions(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	target = ~p->by_color[p->stm];

	num_moves = 0;
	num_moves += generate_all_piece_moves   (p, target, ms + num_moves);
	num_moves += generate_quiet_pawn_moves  (p, target, ms + num_moves);
	num_moves += generate_capture_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

static bitboard dia_straight_lut[NUM_SQUARES][NUM_SQUARES];
static bitboard between_lut[NUM_SQUARES][NUM_SQUARES];

void
movegen_init(void)
{
	square sq1, sq2;

	for (sq1 = A1; sq1 < NUM_SQUARES; ++sq1)
	{
		for (sq2 = A1; sq2 < NUM_SQUARES; ++sq2)
		{
			if (attacks_rook(sq1, 0) & sqbb(sq2))
			{
				dia_straight_lut[sq1][sq2] = attacks_rook(sq1, 0) & attacks_rook(sq2, 0);
				between_lut[sq1][sq2] = attacks_rook(sq1, sqbb(sq2)) & attacks_rook(sq2, sqbb(sq1));
			}

			else if (attacks_bishop(sq1, 0) & sqbb(sq2))
			{
				dia_straight_lut[sq1][sq2] = attacks_bishop(sq1, 0) & attacks_bishop(sq2, 0);
				between_lut[sq1][sq2] = attacks_bishop(sq1, sqbb(sq2)) & attacks_bishop(sq2, sqbb(sq1));
			}

			else
			{
				continue;
			}

			dia_straight_lut[sq1][sq2] |= sqbb(sq1);
			dia_straight_lut[sq1][sq2] |= sqbb(sq2);
			// TODO: stockfish added sq2 to between_lut, think about it when using between_lut
		}
	}
}

// we assume no check
static bool
is_legal(const position *p, move m)
{
	pctype pc;
	bitboard enemy;

	pc = p->by_square[m.from];
	enemy = p->by_color[other_color(p->stm)];

	if (m.type == CASTLING)
		return (attackers(p, m.to) | attackers(p, (m.from + m.to) / 2)) & enemy;

	if (ptype_of(pc) == KING)
		return attackers(p, m.to) & enemy;

	if (!(p->sf->blockers[p->stm] & sqbb(m.from)))
		return true;

	return dia_straight_lut[m.from][m.to] & king_square(p, p->stm);
}

size_t
generate_legals(const position *p, move *ms)
{
	size_t num_moves, i;
	bool check_legal;
	move m;
	bitboard pinned;

	pinned = p->sf->blockers[p->stm] & p->by_color[p->stm];

	num_moves = p->sf->checkers ?
		generate_evasions    (p, ms):
		generate_non_evasions(p, ms);

	for (i = 0; i < num_moves; ++i)
	{
		m = ms[i];

		check_legal =
			(m.from & pinned) ||
			m.from == king_square(p, p->stm) ||
			m.type == EN_PASSANT;

		if (check_legal && !is_legal(p, ms[i]))
			ms[i--] = ms[--num_moves];
	}

	return num_moves;
}
