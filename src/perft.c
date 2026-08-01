#include <nyx/perft.h>
#include <stdio.h>
#include <nyx/utils.h>
#include <nyx/generation.h>

static node_count
perft_rec(position *p, unsigned depth)
{
	if (!depth) return 1;

	node_count nodes;
	move ms[MAX_MOVES];
	size_t num_moves, i;
	state_frame sf;

	nodes = 0;
	num_moves = generate_legals(p, ms);

	for (i = 0; i < num_moves; ++i)
	{
		do_move(p, ms[i], &sf);
		nodes += perft_rec(p, depth - 1);
		undo_move(p, ms[i]);
	}

	return nodes;
}

node_count
perft(position *p, unsigned depth)
{
	if (!depth) return 1;

	node_count total, nodes;
	move ms[MAX_MOVES];
	size_t num_moves, i;
	state_frame sf;

	total = 0;
	num_moves = generate_legals(p, ms);

	for (i = 0; i < num_moves; ++i)
	{
		do_move(p, ms[i], &sf);
		nodes = perft_rec(p, depth - 1);
		undo_move(p, ms[i]);
		total += nodes;

		print_move(ms[i]);
		printf(": %lu\n", nodes);
	}

	printf("\nTotal nodes: %lu\n", total);

	return total;
}
