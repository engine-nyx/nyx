// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/transposition.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/attacks.h>
#include <nyx/uci.h>
#include <nyx/utils.h>
#include <nyx/types.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <signal.h>
#include <string.h>
#include <stdio.h>

static atomic_bool stop = false;

static void
sig_handler(int signum)
{
	if (signum == SIGINT) stop = true;
}

static char fen_arg_buffer[1024];

static void
str_join(int argc, char **argv, char sep_char, char *dest)
{
	size_t i;
	char sep[2];

	dest[0] = '\0';
	sep[0] = sep_char;
	sep[1] = '\0';

	for (i = 0; i < (unsigned) argc; ++i)
	{
		strcat(dest, argv[i]);
		if (argc - 1 - i)
			strcat(dest, sep);
	}
}

int
main(int argc, char **argv)
{
	attacks_init();
	generation_init();
	position_init();

	if (argc == 1)
	{
		uci_loop();
	}
	else
	{
		signal(SIGINT, sig_handler);

		position p;
		state_frame sf;
		transposition_table tt = {};
		limits lim;
		size_t parsed;

		str_join(argc - 1, argv + 1, ' ', fen_arg_buffer);

		parsed = parse_fen(fen_arg_buffer, &p, &sf);
		unsigned long arg_depth = strtoul(fen_arg_buffer + parsed, nullptr, 10);
		lim = arg_depth ? (limits) { .type=DEPTH, .depth=arg_depth} : (limits) { .type=INFINITE };

		tt_resize(&tt, 100000);

		print_board(&p);
		printf("Press Ctrl+C to stop search...");
		fflush(stdout);

		struct search_result res = search(&p, lim, &tt, &stop);

		printf("\nBest move: ");
		print_move(res.best);
		printf(" (%d)\nAt depth: %u (%lu nodes)\n\nPV:\n", res.score, res.depth, res.nodes);
		print_line(res.pv, res.depth);
	}

	return EXIT_SUCCESS;
}
