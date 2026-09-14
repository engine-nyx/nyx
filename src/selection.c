// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/generation.h>
#include <nyx/selection.h>
#include <nyx/types.h>

enum stage
{
	STAGE_TT_MAIN,       // TODO
	STAGE_INIT_CAPTURES, // TODO
	STAGE_GOOD_CAPTURES,
	STAGE_INIT_QUIETS,
	STAGE_GOOD_QUIETS,
	STAGE_BAD_CAPTURES,
	STAGE_BAD_QUIETS,

	STAGE_TT_EVASIONS,   // TODO
	STAGE_INIT_EVASIONS, // TODO
	STAGE_EVASIONS,

	STAGE_TT_QUIESCE,    // TODO
	STAGE_INIT_QUIESCE, // TODO
	STAGE_QUIESCE,
};

static void
score_quiets(const move *ms, size_t num_moves, struct scored_move *sms)
{
	size_t i;
	move m;
	int score;

	for (i = 0; i < num_moves; ++i)
	{
		m = ms[i];
		score = 0;

		sms[i] = (struct scored_move)
		{
			.m=m,
			.score=score,
		};
	}
}

static void
score_moves(const move *ms, size_t num_moves, struct scored_move *sms)
{
	size_t i;
	move m;
	int score;

	for (i = 0; i < num_moves; ++i)
	{
		m = ms[i];
		score = 0;

		sms[i] = (struct scored_move)
		{
			.m=m,
			.score=score,
		};
	}
}

static void
insertion_sort(struct scored_move *ms, size_t num_moves)
{
	size_t sorted, i;
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

constexpr int GOOD_CAPTURE_THRESHOLD = 2;
constexpr int GOOD_QUIET_THRESHOLD = -2;

static bool
has_good_captures(const selector *s)
{
	return s->current < s->num_moves && s->sms[s->current].score >= GOOD_CAPTURE_THRESHOLD;
}

static bool
has_good_quiets(const selector *s)
{
	return s->current < s->num_moves && s->sms[s->current].score >= GOOD_QUIET_THRESHOLD;
}

move
select(selector *s)
{
	move ms[MAX_MOVES];
	size_t generated;

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
		s->num_moves = generate(CAPTURES, s->pos, ms);
		score_moves(ms, s->num_moves, s->sms);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		return select(s);

	case STAGE_GOOD_CAPTURES:
		if (has_good_captures(s))
			return select(s);
		s->num_captures = s->num_moves;
		s->num_good_captures = s->current;
		++s->stage;
		[[fallthrough]];

	case STAGE_INIT_QUIETS:
		// put quiet moves after the captures
		generated = generate(QUIETS, s->pos, ms);
		score_quiets(ms, generated, s->sms + s->num_captures);
		insertion_sort(s->sms + s->num_captures, generated);
		s->num_moves += generated;
		s->current = s->num_captures;
		++s->stage;
		[[fallthrough]];

	case STAGE_GOOD_QUIETS:
		if (has_good_quiets(s))
			return select(s);
		s->current = s->num_good_captures;
		++s->stage;
		[[fallthrough]];

	case STAGE_BAD_CAPTURES:
		if (s->current < s->num_captures)
			return select(s);
		++s->stage;
		[[fallthrough]];

	case STAGE_BAD_QUIETS:
		return select(s);

	case STAGE_INIT_EVASIONS:
		s->current = 0;
		s->num_moves = generate(EVASIONS, s->pos, ms);
		score_moves(ms, s->num_moves, s->sms);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		[[fallthrough]];

	case STAGE_EVASIONS:
	case STAGE_QUIESCE:
		return next_move(s);
	}

	assert(false);
}

selector
selector_of(const position *p, move tt, enum search_stage stage)
{
	selector s = (selector)
	{
		.pos=p,
		.tt =tt,
	};

	if (p->sf->checkers)
	{
		s.stage = STAGE_TT_EVASIONS;
	}
	else switch (stage)
	{
	case MAIN       : s.stage = STAGE_TT_MAIN; break;
	case QUIESCENCE : s.stage = STAGE_TT_QUIESCE; break;
	}

	if (is_null_move(tt)) ++stage;

	return s;
}
