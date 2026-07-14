#include <nyx/utils.h>
#include <assert.h>
#include <nyx/attacks.h>
#include <nyx/movegen.h>
#include <nyx/types.h>
#include <stdbit.h>

static inline square
lsb(bitboard bb)
{
	assert(bb && "No lsb of empty bitboard");

	return stdc_first_trailing_one(bb) - 1;
}

static inline square
pop_lsb(bitboard* bb)
{
	square sq;

	sq = lsb(*bb);
	*bb &= *bb - 1;

	return sq;
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

static size_t
generate_quiet_pawn_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard singles, doubles;
	square from, to;

	singles = p->by_ptype[PAWN] & p->by_color[p->stm];
	if (p->stm == WHITE) singles <<= 8;
	if (p->stm == BLACK) singles >>= 8;
	singles &= target;
	if (p->stm == WHITE) doubles = singles << 8;
	if (p->stm == BLACK) doubles = singles >> 8;
	doubles &= target;

	for (num_moves = 0; singles; ++num_moves)
	{
		to = pop_lsb(&singles);

		if (p->stm == WHITE) from = to - 8;
		if (p->stm == BLACK) from = to + 8;

		ms[num_moves] = (move) { .from=from, .to=to };
	}

	for (; doubles; ++num_moves)
	{
		to = pop_lsb(&doubles);

		if (p->stm == WHITE) from = to - 16;
		if (p->stm == BLACK) from = to + 16;

		ms[num_moves] = (move) { .from=from, .to=to };
	}

	// TODO: handle/exclude promotions?

	return num_moves;
}

static size_t
generate_capture_pawn_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard singles, east, west, ep;
	square from, to;

	singles = p->by_ptype[PAWN] & p->by_color[p->stm];
	if (p->stm == WHITE) singles <<= 8;
	if (p->stm == BLACK) singles >>= 8;
	east = (singles << 1) & 0x7F7F7F7F7F7F7F7Full & target;
	west = (singles >> 1) & 0xFEFEFEFEFEFEFEFEull & target;

	for (num_moves = 0; east; ++num_moves)
	{
		to = pop_lsb(&east);

		if (p->stm == WHITE) from = to - 9;
		if (p->stm == BLACK) from = to + 7;

		ms[num_moves] = (move) { .from=from, .to=to };
	}

	for (; west; ++num_moves)
	{
		to = pop_lsb(&west);

		if (p->stm == WHITE) from = to - 7;
		if (p->stm == BLACK) from = to + 9;

		ms[num_moves] = (move) { .from=from, .to=to };
	}

	// TODO: what about capture promotions?
	if (p->sf->ep != NO_EP)
	{
		ep = sqbb(p->sf->ep);
		east = (ep << 1) & 0x7F7F7F7F7F7F7F7Full;
		west = (ep >> 1) & 0xFEFEFEFEFEFEFEFEull;
		if (p->stm == WHITE) to = p->sf->ep + 8;
		if (p->stm == BLACK) to = p->sf->ep - 8;

		if (singles & east) ms[num_moves++] = (move)
		{
			.from=lsb(east),
			.to  =to,
			.type=EN_PASSANT,
		};

		if (singles & west) ms[num_moves++] = (move)
		{
			.from=lsb(west),
			.to  =to,
			.type=EN_PASSANT,
		};
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

	return num_moves;
}

size_t
generate_captures(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	num_moves = 0;
	target = p->by_color[other_color(p->stm)];

	num_moves += generate_all_piece_moves   (p, target, ms + num_moves);
	num_moves += generate_capture_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

size_t
generate_quiets(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	num_moves = 0;
	target = ~p->by_ptype[ALL];

	num_moves += generate_all_piece_moves (p, target, ms + num_moves);
	num_moves += generate_quiet_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

size_t generate_evasions(const position *p, move *ms);

size_t
generate_non_evasions(const position *p, move *ms)
{
	size_t num_moves;
	bitboard target;

	num_moves = 0;
	target = ~p->by_color[p->stm];

	num_moves += generate_all_piece_moves   (p, target, ms + num_moves);
	num_moves += generate_quiet_pawn_moves  (p, target, ms + num_moves);
	num_moves += generate_capture_pawn_moves(p, target, ms + num_moves);

	return num_moves;
}

static bool
is_square_checked(const position *p, square sq)
{
	// TODO: replace with cached stockfish version
	bitboard enemy;

	enemy = p->by_color[other_color(p->stm)];
	if (attacks_bishop(sq, p->by_ptype[ALL]) & enemy & (p->by_ptype[BISHOP] | p->by_ptype[QUEEN])) return true;
	if (attacks_rook  (sq, p->by_ptype[ALL]) & enemy & (p->by_ptype[ROOK]   | p->by_ptype[QUEEN])) return true;
	if (attacks_knight(sq)                   & enemy & p->by_ptype[KNIGHT]) return true;
	if (attacks_king  (sq)                   & enemy & p->by_ptype[KING])   return true;

	if (p->stm == WHITE)
	{
		if (rank_of(sq) != 7 && (p->by_square[sq + 7] == BLACK_PAWN || p->by_square[sq + 9] == BLACK_PAWN))
			return true;
	}
	if (p->stm == BLACK)
	{
		if (rank_of(sq) != 0 && (p->by_square[sq - 9] == WHITE_PAWN || p->by_square[sq - 7] == WHITE_PAWN))
			return true;
	}

	return false;
}

size_t
generate_legals(const position *p, move *ms)
{
	size_t num_moves;
	(void) is_square_checked;

	num_moves = 0;

	num_moves += generate_captures(p, ms + num_moves);
	num_moves += generate_quiets(p, ms + num_moves);

	return num_moves;
}
