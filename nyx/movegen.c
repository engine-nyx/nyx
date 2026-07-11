#include <assert.h>
#include <nyx/attacks.h>
#include <nyx/movegen.h>
#include <nyx/types.h>
#include <stdbit.h>

static inline square
pop_lsb(bitboard* bb)
{
	assert(*bb && "Cannot pop empty bitboard");

	square lsb;

	lsb = stdc_first_trailing_one(*bb);
	*bb &= *bb - 1;

	return lsb;
}

static size_t
splat_moves(square from, bitboard target, move *ms)
{
	size_t num_moves;
	square to;

	for (num_moves = 0; target; to = pop_lsb(&target))
	{
		ms[num_moves++] = (move)
		{
			.from=from,
			.to  =to,
		};
	}

	return num_moves;
}

static size_t
generate_rook_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard bb, moves;
	square sq;

	num_moves = 0;

	for (bb = p->by_ptype[ROOK] & p->by_color[p->stm]; bb; sq = pop_lsb(&bb))
	{
		moves = attacks_rook(sq, p->by_ptype[ALL]) & target;
		num_moves += splat_moves(sq, moves, ms + num_moves);
	}

	return num_moves;
}

static size_t
generate_bishop_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard bb, moves;
	square sq;

	num_moves = 0;

	for (bb = p->by_ptype[BISHOP] & p->by_color[p->stm]; bb; sq = pop_lsb(&bb))
	{
		moves = attacks_rook(sq, p->by_ptype[ALL]) & target;
		num_moves += splat_moves(sq, moves, ms + num_moves);
	}

	return num_moves;
}

static size_t
generate_queen_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard bb, moves;
	square sq;

	num_moves = 0;

	for (bb = p->by_ptype[BISHOP] & p->by_color[p->stm]; bb; sq = pop_lsb(&bb))
	{
		moves =
			attacks_rook(sq, p->by_ptype[ALL]) |
			attacks_bishop(sq, p->by_ptype[ALL]) &
			target;

		num_moves += splat_moves(sq, moves, ms + num_moves);
	}

	return num_moves;
}

static size_t
generate_knight_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;
	bitboard bb, moves;
	square sq;

	num_moves = 0;

	for (bb = p->by_ptype[KNIGHT] & p->by_color[p->stm]; bb; sq = pop_lsb(&bb))
	{
		moves = attacks_knight(sq) & target;
		num_moves += splat_moves(sq, moves, ms + num_moves);
	}

	return num_moves;
}

static size_t
generate_all_moves(const position *p, bitboard target, move *ms)
{
	size_t num_moves;

	num_moves = 0;

	num_moves += generate_knight_moves(p, target, ms + num_moves);
	num_moves += generate_bishop_moves(p, target, ms + num_moves);
	num_moves += generate_rook_moves  (p, target, ms + num_moves);
	num_moves += generate_queen_moves (p, target, ms + num_moves);

	return num_moves;
}

size_t
generate_captures(const position *p, move *ms)
{
	bitboard target;

	target = p->by_color[other_color(p->stm)];

	return generate_all_moves(p, target, ms);
}

size_t generate_quiets(const position *p, move *ms);
size_t generate_evasions(const position *p, move *ms);
size_t generate_non_evasions(const position *p, move *ms);
size_t generate_legals(const position *p, move *ms);
