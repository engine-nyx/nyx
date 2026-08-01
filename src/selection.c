#include <nyx/selection.h>

move
select_move(selector *s, const position *p)
{
	while (s->current == s->num_moves)
	{
		if (s->stage++ >= CAPTURES) return (move) {};

		s->stage = EVASIONS;

		s->num_moves = generate_legals(p, s->ms);
		s->current = 0;
	}

	return s->ms[s->current++];
}
