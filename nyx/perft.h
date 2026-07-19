#ifndef NYX_PERFT_H
#define NYX_PERFT_H

#include <nyx/movegen.h>
#include <nyx/position.h>
#include <nyx/types.h>

static inline uint_fast64_t
perft(position *p, unsigned depth)
{
	if (!depth) return 1;

	uint_fast64_t nodes;
	move ms[MAX_MOVES], m;
	size_t num_moves, i;
	state_frame sf;

	nodes = 0;
	num_moves = generate_legals(p, ms);

	for (i = 0; i < num_moves; ++i)
	{
		m = ms[i];

		sf = do_move(p, m), p->sf = &sf;
		nodes += perft(p, depth - 1);
		undo_move(p, m);
	}

	return nodes;
}

#endif // NYX_PERFT_H
