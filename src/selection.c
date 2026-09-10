// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/generation.h>
#include <nyx/selection.h>
#include <nyx/types.h>

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

	STAGE_TT_QSEARCH,
	STAGE_INIT_QCAPTURE,
	STAGE_QCAPTURE,
};

static void
score_quiets(const move *ms, size_t num_moves, struct scored_move *sms)
{ }

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
	if (s->current < s->num_moves)
		return s->sms[s->current++].m;

	return NULL_MOVE;
}

move
select(selector *s)
{
	move ms[MAX_MOVES];

	switch (s->stage)
	{
	case STAGE_TT_MAIN:
	case STAGE_TT_EVASIONS:
	case STAGE_TT_QSEARCH:
		++s->stage;
		return s->tt_move;

	case STAGE_INIT_CAPTURES:
	case STAGE_INIT_QCAPTURE:
		s->current = 0;
		s->num_moves = generate(CAPTURES, s->p, ms);
		score_moves(ms, s->num_moves, s->sms);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		return select(s);

	case STAGE_GOOD_CAPTURES:
		// TODO
	case STAGE_INIT_QUIETS:
		// TODO
	case STAGE_GOOD_QUIETS:
		// TODO
	case STAGE_BAD_CAPTURES:
		// TODO
	case STAGE_BAD_QUIETS:
		// TODO

	case STAGE_INIT_EVASIONS:
		s->current = 0;
		s->num_moves = generate(EVASIONS, s->p, ms);
		score_moves(ms, s->num_moves, s->sms);
		insertion_sort(s->sms, s->num_moves);
		++s->stage;
		[[fallthrough]];

	case STAGE_EVASIONS:
	case STAGE_QCAPTURE:
		return next_move(s);
	}

	assert(false);
}
