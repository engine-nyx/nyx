#include <nyx/selection.h>

move
select_move(selector *s, const position *p)
{
	while (s->current == s->num_moves)
	{
		if (s->stage == QUIETS) return (move) {};

		s->num_moves = generate(++s->stage, p, s->ms);
		s->current = 0;
	}

	return s->ms[s->current++];
}
