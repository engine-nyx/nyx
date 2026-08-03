#include <nyx/selection.h>

move
select_move(selector *s)
{
	while (s->current == s->num_moves)
	{
		if (s->stage++ >= CAPTURES) return (move) {};

		s->stage = EVASIONS;

		s->num_moves = generate_legals(s->p, s->ms);
		s->current = 0;
	}

	return s->ms[s->current++];
}
