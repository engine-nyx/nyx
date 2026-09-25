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
#include <string.h>

constexpr int VALUE_DRAW = 0;
static int
VALUE_MATE(unsigned ply)
{
	return (oo / 2) - ply;
}

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

static inline bool
tt_skip(tt_entry ent, int alpha, int beta, struct search_state ss)
{
	if (ent.depth < ss.depth)
		return false;

	switch (ent.bound)
	{
	case EXACT : return true;
	case LOWER : return ent.score >= beta;
	case UPPER : return ent.score <= alpha;
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
	unsigned move_count;

	best_score = evaluate(p);
	best_move = NULL_MOVE;
	if (best_score >= beta)
		return best_score;

	tt_probe_success = tt_probe(ss->tt, p->key, &ent);

	if (tt_probe_success && tt_skip(ent, alpha, beta, *ss))
		return ent.score;

	--ss->depth, ++ss->ply;
	++ss->nodes;

	s = selector_of(p, tt_probe_success ? ent.best_move : NULL_MOVE, QUIESCENCE);

	move_count = 0;
	while (!is_null(m = select(&s)))
	{
		++move_count;
		do_move(p, m, &sf);

		score = -qsearch_rec(p, -beta, -MAX(best_score, alpha), tm, ss);

		undo_move(p, m);

		if (score > best_score)
		{
			best_score = score;
			best_move = m;

			if (score >= beta) break;
		}
	}

	++ss->depth, --ss->ply;
	if (!move_count)
	{
		best_move = NULL_MOVE;
		if (p->sf->checkers)
			best_score = -VALUE_MATE(ss->ply);
	}
	tt_store(ss->tt, p->key, (tt_entry)
	{
		.depth=0,
		.best_move=best_move,
		.score=best_score,
		.bound=
		(
			best_score >= beta ? LOWER :
			best_score <= alpha ? UPPER :
			EXACT
		)
	});

	return best_score;
}

static int
search_rec(position *p, int alpha, int beta, time_manager *tm, struct search_state *ss, bool pv)
{
	selector s;
	move m, best_move;
	int score, best_score;
	state_frame sf;
	tt_entry ent;
	bool tt_probe_success, first_move;
	unsigned move_count;

	tt_probe_success = tt_probe(ss->tt, p->key, &ent);

	if (tt_probe_success && tt_skip(ent, alpha, beta, *ss))
		return ent.score;

	if (!ss->depth)
		return qsearch_rec(p, alpha, beta, tm, ss);

	--ss->depth, ++ss->ply;
	++ss->nodes;

	s = selector_of(p, tt_probe_success ? ent.best_move : NULL_MOVE, MAIN);
	best_score = -oo;
	first_move = true;
	move_count = 0;

	while (!tm_hard_expired(tm, ss) && !is_null(m = select(&s)))
	{
		++move_count;
		do_move(p, m, &sf);

		if (first_move)
		{
			score = -search_rec(p, -beta, -alpha, tm, ss, pv);

			first_move = false;
		}
		else
		{
			score = -search_rec(p, -(best_score + 1), -best_score, tm, ss, false);

			// re-search fail-high nodes
			if (score > best_score && pv)
				score = -search_rec(p, -beta, -MAX(alpha, best_score), tm, ss, true);
		}

		undo_move(p, m);

		if (score > best_score)
		{
			best_score = score;
			best_move = m;

			if (score >= beta) break;
		}
	}

	++ss->depth, --ss->ply;
	if (!move_count)
	{
		best_move = NULL_MOVE;
		best_score = p->sf->checkers ? -VALUE_MATE(ss->ply) : VALUE_DRAW;
	}

	tt_store(ss->tt, p->key, (tt_entry)
	{
		.depth=(u8)ss->depth,
		.best_move=best_move,
		.score=best_score,
		.bound=
		(
			best_score >= beta  ? LOWER :
			best_score <= alpha ? UPPER :
			EXACT
		),
		.pv=pv
	});
	if (pv)
	{
		ss->pv[ss->ply] = best_move;
		ss->score[ss->ply] = best_score;
	}

	return best_score;
}

static bool
is_mate_score(int score)
{
	return score < -oo / 3 || score > oo / 3;
}

struct search_result
search(position *p, limits l, transposition_table *tt, atomic_bool *stop)
{
	unsigned depth;
	time_manager *tm;
	struct search_state *ss;
	struct search_result res;

	ss = &(struct search_state) { .p=p, .tt=tt };
	tm = &(time_manager) { .l=l, .stop=stop };
	res = (struct search_result) {};
	tm_start(tm);

	for (depth = 0; !tm_soft_expired(tm, ss) && !is_mate_score(ss->score[0]); ++depth)
	{
		ss->depth = depth;

		search_rec(p, -oo, oo, tm, ss, true);

		// do not use partial search results
		if (tm_hard_expired(tm, ss)) break;

		res = (struct search_result)
		{
			.best=ss->pv[0],
			.score=ss->score[0],
			.depth=depth,
			.nodes=ss->nodes,
		};
		memcpy(&res.pv, &ss->pv, depth * sizeof(move));
	}

	return res;
}
