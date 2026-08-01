#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/evaluation.h>

static int
alpha_beta(position *p, int alpha, int beta, unsigned depth, time_manager *tm)
{
	state_frame sf;
	selector s;
	move m, best_move;
	int score, best_score;

	(void) best_move;

	if (!depth) return evaluate(p);

	best_score = -oo;
	s = (selector) {};

	while (!tm_hard_expired(tm))
	{
		m = select_move(&s, p);
		if (m.from == 0 && m.to == 0) break;

		do_move(p, m, &sf);
		score = -alpha_beta(p, -beta, -alpha, depth - 1, tm);
		undo_move(p, m);

		if (score >= beta)
		{
			best_score = score;
			best_move = m;
			break;
		}

		if (score > best_score)
		{
			best_score = score;
			best_move = m;

			if (score > alpha)
			{
				alpha = score;
			}
		}
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
	int alpha, beta;

	s = (selector) {};
	best_score = -oo;
	alpha = -5;
	beta  = +5;

	while (!tm_soft_expired(tm))
	{
		m = select_move(&s, p);
		if (m.from == 0 && m.to == 0) break;

		do_move(p, m, &sf);
		score = alpha_beta(p, alpha, beta, 4, tm);
		undo_move(p, m);

		if (score > best_score)
		{
			best_move = m;
		}
	}

	return (struct search_result)
	{
		.best=best_move,
	};
}
