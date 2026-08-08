#include <nyx/time.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/evaluation.h>

static int
search_rec(position *p, int alpha, int beta, time_manager *tm, struct search_state *ss)
{
	selector s;
	move m;
	int score, best_score;
	state_frame sf;

	if (!ss->depth) return evaluate(p);
	--ss->depth;
	++ss->nodes;

	s = (selector) { .p=p };
	best_score = alpha;

	while (!tm_hard_expired(tm, ss) && !is_null_move(m = select_move(&s)))
	{
		do_move(p, m, &sf);

		if (sf.material < -oo / 2 || sf.material > oo / 2)
		{
			score = -evaluate(p);
		}
		else
		{
			score = -search_rec(p, -beta, -best_score, tm, ss);
		}

		undo_move(p, m);

		if (score > best_score)
		{
			best_score = score;

			if (score >= beta) break;
		}
	}

	++ss->depth;

	return best_score;
}

struct search_result
search(position *p, limits l)
{
	state_frame sf;
	selector s;
	move m, best_move, best_best_move;
	int score, best_score;
	unsigned depth;
	time_manager *tm;
	struct search_state *ss;

	ss = &(struct search_state) { .p=p };
	tm = &(time_manager) { .l=l };
	tm_start(tm);

	for (depth = 0; !tm_soft_expired(tm, ss); ++depth)
	{
		ss->depth = depth;
		best_score = -oo;
		s = (selector) { .p=p };

		while (true)
		{
			if (is_null_move(m = select_move(&s)))
			{
				best_best_move = best_move;
				break;
			}

			if (tm_hard_expired(tm, ss)) break;

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
	};
}
