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
partial_insertion_sort(void) {}

static move
next_move(selector *s)
{
	if (s->current < s->num_moves)
		return s->ms[s->current++];

	return NULL_MOVE;
}

move
select(selector *s)
{
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
		s->num_moves = generate(CAPTURES, s->p, s->ms);
		partial_insertion_sort();
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
		s->num_moves = generate(EVASIONS, s->p, s->ms);
		partial_insertion_sort();
		++s->stage;
		[[fallthrough]];

	case STAGE_EVASIONS:
	case STAGE_QCAPTURE:
		return next_move(s);
	}

	assert(false);
}
