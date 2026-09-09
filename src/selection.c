// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/generation.h>
#include <nyx/selection.h>
#include <nyx/types.h>

enum stage
{
	TT_MAIN,
	INIT_CAPTURES,
	GOOD_CAPTURES,
	INIT_QUIETS,
	GOOD_QUIETS,
	BAD_CAPTURES,
	BAD_QUIETS,

	TT_EVASIONS,
	INIT_EVASIONS,
	EVASIONS_,

	TT_QSEARCH,
	INIT_QCAPTURE,
	QCAPTURE,
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
	case TT_MAIN:
	case TT_EVASIONS:
	case TT_QSEARCH:
		++s->stage;
		return s->tt_move;

	case INIT_CAPTURES:
	case INIT_QCAPTURE:
		s->current = 0;
		s->num_moves = generate(CAPTURES, s->p, s->ms);
		partial_insertion_sort();
		++s->stage;
		return select(s);

	case GOOD_CAPTURES:
		// TODO
	case INIT_QUIETS:
		// TODO
	case GOOD_QUIETS:
		// TODO
	case BAD_CAPTURES:
		// TODO
	case BAD_QUIETS:
		// TODO

	case INIT_EVASIONS:
		s->current = 0;
		s->num_moves = generate(EVASIONS, s->p, s->ms);
		partial_insertion_sort();
		++s->stage;

	case EVASIONS_:
	case QCAPTURE:
		return next_move(s);
	}

	assert(false);
}
