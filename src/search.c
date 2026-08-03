#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/evaluation.h>

static int
search_rec(position *p, unsigned depth)
{
	selector s;
	move m;
	int score, best_score;
	state_frame sf;

	if (!depth) return evaluate(p);

	s = (selector) { .p=p };
	best_score = -oo;

	while (!is_null_move(m = select_move(&s)))
	{
		do_move(p, m, &sf);

		if (sf.material < -oo / 2 || sf.material > oo / 2)
		{
			score = -evaluate(p);
		}
		else
		{
			score = -search_rec(p, depth - 1);
		}

		undo_move(p, m);

		if (score > best_score)
			best_score = score;
	}

	return best_score;
}

struct search_result
search(position *p, time_manager *tm)
{
	state_frame sf;
	selector s;
	move m, best_move;
	int score, best_score;

	s = (selector) { .p=p };
	best_score = -oo;

	while (!tm_soft_expired(tm) && !is_null_move(m = select_move(&s)))
	{
		do_move(p, m, &sf);
		score = -search_rec(p, 3);
		undo_move(p, m);

		score *= white_black(-1, +1, p->stm);

		if (score > best_score)
		{
			best_score = score;
			best_move = m;
		}
	}

	return (struct search_result)
	{
		.best=best_move,
	};
}
