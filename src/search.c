// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <nyx/time.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/evaluation.h>
#include <nyx/transposition.h>

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

static inline bool
tt_skip(tt_entry ent, int alpha, int beta, struct search_state ss)
{
	if (ent.depth < ss.depth)
		return false;

	switch (ent.type)
	{
	case EXACT     : return true;
	case FAIL_HIGH : return ent.score >= beta;
	case FAIL_LOW  : return ent.score <= alpha;
	}

	assert(false);
}

static int
qsearch_rec(position *p, int alpha, int beta, time_manager *tm, struct search_state *ss)
{
	selector s;
	move m, best_move;
	int score, best_score;
	state_frame sf;
	tt_entry ent;
	bool tt_probe_success;

	best_score = evaluate(p);
	best_move = NULL_MOVE;
	if (best_score >= beta)
		return best_score;

	tt_probe_success = tt_probe(ss->tt, p->key, &ent);

	if (tt_probe_success && tt_skip(ent, alpha, beta, *ss))
		return ent.score;

	--ss->depth;
	++ss->nodes;

	s = selector_of(p, tt_probe_success ? ent.best_move : NULL_MOVE, QUIESCENCE);

	while (!is_null(m = select(&s)))
	{
		do_move(p, m, &sf);

		if (sf.material < -oo / 2 || sf.material > oo / 2)
		{
			score = -evaluate(p);
		}
		else
		{
			score = -qsearch_rec(p, -beta, -MAX(best_score, alpha), tm, ss);
		}

		undo_move(p, m);

		if (score > best_score)
		{
			best_score = score;
			best_move = m;

			if (score >= beta) break;
		}
	}

	++ss->depth;
	tt_store(ss->tt, (tt_entry)
	{
		.depth=(u8)ss->depth,
		.best_move=best_move,
		.key=p->key,
		.score=best_score,
		.type=
		(
			best_score >= beta ? FAIL_HIGH :
			best_score <= alpha ? FAIL_LOW :
			EXACT
		),
	});

	return best_score;
}

static int
search_rec(position *p, int alpha, int beta, time_manager *tm, struct search_state *ss)
{
	selector s;
	move m, best_move;
	int score, best_score;
	state_frame sf;
	tt_entry ent;
	bool tt_probe_success;

	tt_probe_success = tt_probe(ss->tt, p->key, &ent);

	if (tt_probe_success && tt_skip(ent, alpha, beta, *ss))
		return ent.score;

	if (!ss->depth)
		return qsearch_rec(p, alpha, beta, tm, ss);

	--ss->depth;
	++ss->nodes;

	s = selector_of(p, tt_probe_success ? ent.best_move : NULL_MOVE, MAIN);
	best_score = -oo;

	while (!tm_hard_expired(tm, ss) && !is_null(m = select(&s)))
	{
		do_move(p, m, &sf);

		if (sf.material < -oo / 2 || sf.material > oo / 2)
		{
			score = -evaluate(p);
		}
		else
		{
			score = -search_rec(p, -beta, -MAX(best_score, alpha), tm, ss);
		}

		undo_move(p, m);

		if (score > best_score)
		{
			best_score = score;
			best_move = m;

			if (score >= beta) break;
		}
	}

	++ss->depth;
	tt_store(ss->tt, (tt_entry)
	{
		.depth=(u8)ss->depth,
		.best_move=best_move,
		.key=p->key,
		.score=best_score,
		.type=
		(
			best_score >= beta ? FAIL_HIGH :
			best_score <= alpha ? FAIL_LOW :
			EXACT
		),
	});

	return best_score;
}

struct search_result
search(position *p, limits l, transposition_table *tt, atomic_bool *stop)
{
	state_frame sf;
	selector s;
	move m, best_move, best_best_move;
	int score, best_score;
	unsigned depth;
	time_manager *tm;
	struct search_state *ss;

	ss = &(struct search_state) { .p=p, .tt=tt };
	tm = &(time_manager) { .l=l, .stop=stop };
	tm_start(tm);
	tt_clear(tt);

	for (depth = 0; !tm_soft_expired(tm, ss); ++depth)
	{
		ss->depth = depth;
		best_score = -oo;
		s = selector_of(p, NULL_MOVE, MAIN);

		while (true)
		{
			if (is_null(m = select(&s)))
			{
				best_best_move = best_move;
				break;
			}

			if (tm_hard_expired(tm, ss))
			{
				--depth;
				break;
			}

			do_move(p, m, &sf);
			score = -search_rec(p, -oo, -best_score, tm, ss);
			undo_move(p, m);

			if (score > best_score)
			{
				best_score = score;
				best_move = m;
			}
		}
	}

	return (struct search_result)
	{
		.best=best_best_move,
		.depth=depth,
		.nodes=ss->nodes,
	};
}
