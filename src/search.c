// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <inttypes.h>
#include <nyx/evaluation.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/time.h>
#include <nyx/transposition.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <stdio.h>

struct search_state
{
	node_count nodes, qnodes;
	unsigned depth, qsearch_max, ply;

	transposition_table *tt;

	move pv[MAX_PLY][MAX_PLY + 1];
};

constexpr int VALUE_DRAW = 0;
static int
VALUE_MATE(unsigned ply)
{
	return oo - ply;
}
static int
VALUE_QMATE(unsigned ply)
{
	return VALUE_MATE(ply + MAX_PLY + 1);
}

constexpr int MATE_THRESHOLD = oo - MAX_PLY;
constexpr int QMATE_THRESHOLD = MATE_THRESHOLD - (MAX_PLY + 1);
static bool
is_mate_score(int score)
{
	return score <= -MATE_THRESHOLD || score >= MATE_THRESHOLD;
}
static bool
is_qmate_score(int score)
{
	return score <= -QMATE_THRESHOLD || score >= QMATE_THRESHOLD;
}

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

static inline bool
tt_qskip(tt_entry ent, int alpha, int beta)
{
	switch (ent.bound)
	{
	case EXACT : return true;
	case LOWER : return ent.score >= beta;
	case UPPER : return ent.score <= alpha;
	case INVALID: return false;
	}

	assert(false);
}

static inline bool
tt_skip(tt_entry ent, int alpha, int beta, struct search_state ss)
{
	return tt_qskip(ent, alpha, beta) && ent.depth >= ss.depth;
}

static int
qsearch_rec(position *p, int alpha, int beta, time_manager *tm, struct search_state *ss)
{
	selector s;
	move m, best_move;
	int score, best_score;
	state_frame sf;
	tt_entry ent;

	best_score = p->sf->checkers ? -oo : evaluate(p);
	best_move = NULL_MOVE;
	if (best_score >= beta)
		return best_score;

	ent = probe(ss->tt, p->key);

	if (tt_qskip(ent, alpha, beta))
		return is_qmate_score(ent.score) ? VALUE_QMATE(ss->ply) : ent.score;

	++ss->depth, ++ss->ply;
	++ss->nodes;
	++ss->qnodes;
	ss->qsearch_max = MAX(ss->qsearch_max, ss->depth);

	s = selector_of(p, ent.bound == INVALID ? NULL_MOVE : ent.best_move, QUIESCENCE);

	while (!is_null(m = select(&s)))
	{
		if (p->sf->rule50 > 100)
		{
			best_score = VALUE_DRAW;
			break;
		}

		do_move(p, m, &sf);

		if (is_repetition(p))
		{
			score = VALUE_DRAW;
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

	--ss->depth, --ss->ply;

	if (!s.selected)
	{
		best_move = NULL_MOVE;
		if (p->sf->checkers)
			best_score = -VALUE_QMATE(ss->ply);
	}

	store(ss->tt, p->key, (tt_entry)
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
	bool first_move;

	ent = probe(ss->tt, p->key);

	if (!pv && tt_skip(ent, alpha, beta, *ss))
		return is_mate_score(ent.score) ? VALUE_MATE(ss->ply) : ent.score;

	if (!ss->depth)
		return qsearch_rec(p, alpha, beta, tm, ss);

	--ss->depth, ++ss->ply;
	++ss->nodes;

	if (pv) ss->pv[ss->ply][0] = NULL_MOVE;

	s = selector_of(p, ent.bound == INVALID ? NULL_MOVE : ent.best_move, MAIN);
	best_score = -oo;
	first_move = true;

	while (!tm_hard_expired(tm, ss->depth, ss->nodes, p) && !is_null(m = select(&s)))
	{
		if (p->sf->rule50 > 100)
		{
			best_score = VALUE_DRAW;
			best_move = NULL_MOVE;
			break;
		}

		do_move(p, m, &sf);

		if (is_repetition(p))
		{
			score = VALUE_DRAW;
		}
		else if (first_move)
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

			if (pv)
			{
				ss->pv[ss->ply - 1][0] = m;
				*movecpy(ss->pv[ss->ply - 1] + 1, ss->pv[ss->ply], ss->depth) = NULL_MOVE;
			}
		}
	}

	++ss->depth, --ss->ply;

	if (!s.selected)
	{
		best_move = NULL_MOVE;
		best_score = p->sf->checkers ? -VALUE_MATE(ss->ply) : VALUE_DRAW;
	}

	store(ss->tt, p->key, (tt_entry)
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

	return best_score;
}

static void
uci_info(struct search_state *ss, int score, time_manager *tm)
{
	millis spent;
	const char *score_metric;
	int score_value;
	usize i;

	spent = now() - tm->start;
	score_metric = is_mate_score(score) ? "mate" : "cp";
	score_value  = is_mate_score(score) ?
		score < 0 ? -(score + oo) / 2 : -(score - oo - 1) / 2 :
		score;

	printf
	(
		"info "
		"depth %u "
		"seldepth %u "
		"score %s %d "
		"nodes %" PRIuFAST64 " "
		"nps %" PRIuFAST64 " "
		"hashfull %u "
		"time %" PRIuFAST64 " "
		"pv",
		ss->depth,
		ss->depth + ss->qsearch_max,
		score_metric,
		score_value,
		ss->nodes,
		spent ? (ss->nodes * 1000) / spent : 0,
		tt_hashfull(ss->tt),
		spent
	);

	for (i = 0; i < ss->depth; ++i)
	{
		if (is_null(ss->pv[0][i])) break;

		printf(" ");
		print_move(ss->pv[0][i]);
	}
	printf("\n");
	fflush(stdout);
}

struct search_result
search(position *p, limits l, transposition_table *tt, atomic_bool *stop)
{
	time_manager *tm;
	struct search_state *ss;
	struct search_result res;
	int score;

	ss = &(struct search_state) { .tt=tt };
	tm = &(time_manager) { .l=l, .stop=stop };
	res = (struct search_result) {};
	tm_start(tm);
	tt_new_search(tt);

	score = 0;
	for (ss->depth = 0; !tm_soft_expired(tm, ss->depth, ss->nodes, p) && !is_mate_score(score); ++ss->depth)
	{
		score = search_rec(p, -oo, oo, tm, ss, true);

		// do not use partial search results
		if (tm_hard_expired(tm, ss->depth, ss->nodes, p)) break;

		res = (struct search_result)
		{
			.best=ss->pv[0][0],
			.score=score,
			.depth=ss->depth,
		};
		movecpy(res.pv, ss->pv[0], ss->depth);

		uci_info(ss, score, tm);
	}

	res.nodes = ss->nodes;
	res.qnodes = ss->qnodes;

	return res;
}
