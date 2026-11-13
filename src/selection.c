// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/selection.h>
#include <nyx/types.h>
#include <nyx/utils.h>

enum stage
{
	STAGE_TT_MAIN,
	STAGE_INIT_CAPTURES,
	STAGE_GOOD_CAPTURES,
	STAGE_INIT_QUIETS,
	STAGE_GOOD_QUIETS,
	STAGE_BAD_CAPTURES,
	STAGE_BAD_QUIETS,

	STAGE_TT_EVASIONS,
	STAGE_INIT_EVASIONS,
	STAGE_EVASIONS,

	STAGE_TT_QUIESCE,
	STAGE_INIT_QUIESCE,
	STAGE_QUIESCE,
};

static const int PIECE_VALUE[NUM_PIECE_TYPES] =
{
	[NONE]   = 0,
	[PAWN]   = 208,
	[KNIGHT] = 781,
	[BISHOP] = 825,
	[ROOK]   = 1276,
	[QUEEN]  = 2538,
	[KING]   = oo,
};

constexpr PARAMETER int CAPTURED_PIECE_SCORE_WEIGHT = 5;

static int
score_capture(move m, const struct search_state *ss)
{
	int score;
	pctype pc, capture;

	score = 0;
	pc = ss->pos->by_square[m.from];
	capture = ss->pos->by_square[m.to];

	score += ss->hist->capture[pc][m.to][ptype_of(capture)];
	score += CAPTURED_PIECE_SCORE_WEIGHT * PIECE_VALUE[ptype_of(capture)];

	return score;
}

static int
score_quiet(move m, const struct search_state *ss)
{
	int score;
	ptype pt;
	color us;
	usize ply;

	score = 0;

	pt = ptype_of(ss->pos->by_square[m.from]);
	us = color_of(ss->pos->by_square[m.from]);

	for (ply = 1; ply <= MAX_CONTINUATION && ply < ss->ply; ++ply)
	{
		score += ss->hist->continuation[ply]
			[ss->moved_pieces[ss->ply - ply - 1]][ss->played_moves[ss->ply - ply - 1].to]
			[pt][m.to];
	}
	score += ss->hist->main[us][m.from][m.to];

	return score;
}

constexpr PARAMETER int CAPTURE_EVASION_BONUS = 1 << 15;

static int
score_evasion(move m, const struct search_state *ss)
{
	return is_capture(ss->pos, m) ?
		score_capture(m, ss) + CAPTURE_EVASION_BONUS :
		score_quiet(m, ss);
}

static void
score(enum generation_type type, const move *ms, usize num_moves, struct scored_move *sms, const struct search_state *ss)
{
	usize i;
	int score;

	for (i = 0; i < num_moves; ++i)
	{
		score = 0;

		switch (type)
		{
		case CAPTURES : score = score_capture(ms[i], ss); break;
		case QUIETS   : score = score_quiet  (ms[i], ss); break;
		case EVASIONS : score = score_evasion(ms[i], ss); break;
		default       : assert(false);
		}

		sms[i].m = ms[i];
		sms[i].score = score;
	}
}

static void
insertion_sort(struct scored_move *ms, usize num_moves)
{
	usize sorted, i;
	struct scored_move temp;

	for (sorted = 0; sorted < num_moves; ++sorted)
	{
		for (i = num_moves - sorted; i < num_moves; ++i)
		{
			if (ms[i - 1].score >= ms[i].score) break;

			temp = ms[i];
			ms[i] = ms[i - 1];
			ms[i - 1] = temp;
		}
	}
}

static move
next_move(selector *s)
{
	if (s->current >= s->num_moves)
		return NULL_MOVE;

	return s->sms[s->current++].m;
}

constexpr PARAMETER int GOOD_SEE_THRESHOLD = 0;
constexpr PARAMETER int GOOD_QUIET_THRESHOLD = -2;

static bool see_ge(const position *pos, move m, int ge);

static bool
has_good_captures(selector *s)
{
	struct scored_move swap;

	while (s->current < s->num_good_captures)
	{
		if (see_ge(s->ss->pos, s->sms[s->current].m, GOOD_SEE_THRESHOLD /* TODO: make dependent on score */))
			return true;

		--s->num_good_captures;
		swap = s->sms[s->current];
		s->sms[s->current] = s->sms[s->num_good_captures];
		s->sms[s->num_good_captures] = swap;
	}

	return false;
}

static bool
has_good_quiets(const selector *s)
{
	return s->current < s->num_moves && s->sms[s->current].score >= GOOD_QUIET_THRESHOLD;
}

static move
pseudo_select(selector *s)
{
	move ms[MAX_MOVES];
	usize generated;

	switch (s->stage)
	{
	case STAGE_TT_MAIN:
	case STAGE_TT_EVASIONS:
	case STAGE_TT_QUIESCE:
		++s->stage;
		return s->tt;

	case STAGE_INIT_CAPTURES:
	case STAGE_INIT_QUIESCE:
		s->current = 0;
		s->num_moves = generate(CAPTURES, s->ss->pos, ms);
		s->num_good_captures = s->num_moves;
		score(CAPTURES, ms, s->num_moves, s->sms, s->ss);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		return pseudo_select(s);

	case STAGE_GOOD_CAPTURES:
		if (has_good_captures(s))
			return next_move(s);
		s->num_captures = s->num_moves;
		++s->stage;
		[[fallthrough]];

	case STAGE_INIT_QUIETS:
		// put quiet moves after the captures
		s->current = s->num_captures;
		generated = generate(QUIETS, s->ss->pos, ms);
		s->num_moves += generated;
		score(QUIETS, ms, generated, s->sms + s->num_captures, s->ss);
		insertion_sort(s->sms + s->num_captures, generated);
		++s->stage;
		[[fallthrough]];

	case STAGE_GOOD_QUIETS:
		if (has_good_quiets(s))
			return next_move(s);
		s->num_good_quiets = s->current - s->num_captures;
		s->current = s->num_good_captures;
		++s->stage;
		[[fallthrough]];

	case STAGE_BAD_CAPTURES:
		if (s->current < s->num_captures)
			return next_move(s);
		s->current = s->num_captures + s->num_good_quiets;
		++s->stage;
		[[fallthrough]];

	case STAGE_BAD_QUIETS:
		return next_move(s);

	case STAGE_INIT_EVASIONS:
		s->current = 0;
		s->num_moves = generate(EVASIONS, s->ss->pos, ms);
		score(EVASIONS, ms, s->num_moves, s->sms, s->ss);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		[[fallthrough]];

	case STAGE_EVASIONS:
	case STAGE_QUIESCE:
		return next_move(s);
	}

	assert(false);
	return NULL_MOVE;
}

move
select(selector *s)
{
	move m;

	m = pseudo_select(s);
	if (is_null(m) || is_legal(s->ss->pos, m))
	{
		if (!is_null(m))
			++s->selected;
		return m;
	}
	return select(s);
}

selector
selector_of(const struct search_state *ss, move tt, enum search_stage stage)
{
	selector s;
	bool tt_is_quiesce;

	s = (selector)
	{
		.tt=tt,
		.ss=ss,
	};

	if (ss->pos->sf->checkers)
	{
		s.stage = STAGE_TT_EVASIONS;
	}
	else switch (stage)
	{
	case MAIN       : s.stage = STAGE_TT_MAIN;    break;
	case QUIESCENCE : s.stage = STAGE_TT_QUIESCE; break;
	}

	tt_is_quiesce = ss->pos->sf->checkers ||
		ss->pos->by_square[tt.to] != EMPTY ||
		tt.type == EN_PASSANT ||
		tt.type == PROMOTION;

	if (is_null(tt) || !is_pseudo_legal(ss->pos, tt)) ++s.stage;
	else if (stage == QUIESCENCE && !tt_is_quiesce) ++s.stage;

	return s;
}

static bitboard
lowest_piece_bb(const position *pos, bitboard bb)
{
	ptype pt;
	bitboard subset;

	for (pt = PAWN; pt <= KING; ++pt)
	{
		subset = bb & pos->by_ptype[pt];
		if (subset)
			return subset & -subset;
	}

	return EMPTYBB;
}

static bool
see_ge(const position *pos, move m, int ge)
{
	int value;
	bitboard attackers, attackers_stm, occ, from_bb;
	color stm;
	ptype pt;

	if (m.type != NORMAL) return 0 >= ge;
	value = PIECE_VALUE[ptype_of(pos->by_square[m.to])] - ge;
	if (value < 0) return false;
	value = PIECE_VALUE[ptype_of(pos->by_square[m.from])] - value;
	if (value <= 0) return true;

	occ = pos->by_ptype[ALL] ^ bbsq(m.from) ^ bbsq(m.to);
	stm = other_color(pos->stm);

	for (attackers = attackers_explicit(pos, m.to, occ) & occ; (attackers_stm = attackers & pos->by_color[stm]); stm = other_color(stm))
	{
		// filter out pinned pieces as long as there are pinners
		if (pos->sf->pinners[other_color(stm)] & occ)
		{
			attackers_stm &= ~pos->sf->blockers[stm];
			if (!attackers_stm) break;
		}

		from_bb = lowest_piece_bb(pos, attackers_stm);
		pt = ptype_of(pos->by_square[lsb(from_bb)]);
		value = PIECE_VALUE[pt] - value;
		if (value < (int) (stm == pos->stm))
			return stm == pos->stm;

		occ &= ~from_bb;

		// reveal xray attacks
		switch (pt)
		{
		case PAWN   : attackers |=  attacks_piece(BISHOP, m.to, occ) & (pos->by_ptype[BISHOP] | pos->by_ptype[QUEEN]); break;
		case KNIGHT : break;
		case BISHOP : attackers |=  attacks_piece(BISHOP, m.to, occ) & (pos->by_ptype[BISHOP] | pos->by_ptype[QUEEN]); break;
		case ROOK   : attackers |=  attacks_piece(ROOK  , m.to, occ) & (pos->by_ptype[ROOK]   | pos->by_ptype[QUEEN]); break;
		case QUEEN  : attackers |= (attacks_piece(BISHOP, m.to, occ) & pos->by_ptype[BISHOP]) | (attacks_piece(ROOK, m.to, occ) & pos->by_ptype[ROOK]); break;
		case KING   : return (bool) (attackers & ~pos->by_color[stm]) == (stm == pos->stm);
		default     : assert(false);
		}
		attackers &= occ;
	}

	// return true if it's the enemy's turn but they don't have any more defenders;
	// return false otherwise.
	return stm != pos->stm;
}
