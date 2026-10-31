// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <compat/threads.h>
#include <future/c2y.h>
#include <nyx/generation.h>
#include <nyx/perft.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/time.h>
#include <nyx/transposition.h>
#include <nyx/types.h>
#include <nyx/uci.h>
#include <nyx/utils.h>
#include <nyx/version.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct
{
	time_manager tm;
	atomic_bool stop;
	position p;
	state_frame sf[150];  // Rule75 guarantees non reversable moves every 150 halfmoves

	transposition_table tt;

	bool quit;

	atomic_bool joined;
	thrd_t search_thread;
	limits lim;
} UCI_state;

static struct
{
	usize transposition_table_capacity;
	usize threads;
} UCI_config;

static const struct
{
	usize min, max, value;
	usize *ptr;
	const char *name;
} UCI_OPTIONS[] =
{
	{ 1, 33554432, 100000, &UCI_config.transposition_table_capacity, "ttszk"   },
	{ 1,        1,      1, &UCI_config.threads                     , "threads" },
};

// quit
// uci: -> uciok
// ucinewgame
// isready -> readyok
// setoption
// position [fenn <fenstr> | startpos] moves <move1> ...
// go [infinite | depth <depth> | nodes | mate] searchmoves <move1> ... ponder wtime <x> btime <x> winc <x> binc <x> movestogo <x> nodes <x> movetime <x> pertf <x> -> bestmove <move> ponder <expected>
// stop
// ponderhit (tells nyx the pondering move has been played)

static void uci_stop(const char *args);
static void uci_position(const char *cmd);

static void
uci_quit(const char *args)
{
	(void) args;

	uci_stop("");
	UCI_state.quit = true;
}

static const char AUTHORS[] =
{
	#embed "AUTHORS" suffix(, 0)
};

static void
uci_id(void)
{
	const char *start, *end;

	puts("id name Nyx v" NYX_VERSION);

	for (start = end = AUTHORS; *start && end; start = end + 1)
	{
		end = strchr(start, '\n');

		printf("id author %.*s\n", (int) (end ? end - start : (int) strlen(start)), start);
	}
}

static void
uci_options(void)
{
	usize i;

	for (i = 0; i < lengthof(UCI_OPTIONS); ++i)
	{
		printf("option name %s ", UCI_OPTIONS[i].name);
		printf("type spin ");
		printf("default %zu ", UCI_OPTIONS[i].value);
		printf("min %zu ", UCI_OPTIONS[i].min);
		printf("max %zu\n", UCI_OPTIONS[i].max);
	}
}

static void
uci_default_options(void)
{
	usize i;

	for (i = 0; i < lengthof(UCI_OPTIONS); ++i)
		*UCI_OPTIONS[i].ptr = UCI_OPTIONS[i].value;
}

static void uci_ucinewgame(const char *args);

static void
uci_uci(const char *)
{
	uci_id();
	puts("");
	uci_options();
	puts("");
	puts("uciok");
}

static void
uci_ucinewgame(const char *)
{
	uci_stop("");
	uci_position("startpos");
	tt_resize(&UCI_state.tt, UCI_config.transposition_table_capacity * 1000);
}

static void
uci_isready(const char *)
{
	puts("readyok");
}

static void
uci_setoption(const char *args)
{
	usize i, value;

	for (i = 0; i < lengthof(UCI_OPTIONS); ++i)
	{
		if (str_consume(&args, UCI_OPTIONS[i].name))
		{
			assert(str_ltrim(&args) && "setting separator");
			value = strtoull(args, nullptr, 10);
			if (value < UCI_OPTIONS[i].min || value > UCI_OPTIONS[i].max)
				assert(false);
			*UCI_OPTIONS[i].ptr = value;
			break;
		}
	}
}

static const char *startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static bool
is_file(char c) { return c >= 'a' && c <= 'h'; }
static bool
is_rank(char c) { return c >= '1' && c <= '8'; }

static bool
is_move(const char *s)
{
	return strlen(s) >= 4 && is_file(s[0]) && is_rank(s[1]) && is_file(s[2]) && is_rank(s[3]);
}

static void
uci_position(const char *args)
{
	usize i;

	if (str_consume(&args, "startpos"))
	{
		parse_fen(startpos, &UCI_state.p, UCI_state.sf);
	}

	else if (str_consume(&args, "fen"))
	{
		assert(str_ltrim(&args) > 0 && "fen separator");
		args += parse_fen(args, &UCI_state.p, UCI_state.sf);
	}

	else
	{
		assert(false && "Invalid position command");
	}

	if (!str_ltrim(&args) || !str_consume(&args, "moves") || !str_ltrim(&args))
		return;

	for (i = 1; is_move(args); ++i, i %= 150)
	{
		args += do_lan_move(&UCI_state.p, args, UCI_state.sf + i);
		if (!str_ltrim(&args)) break;
	}
}

static int
search_start(void *)
{
	struct search_result res;

	UCI_state.stop = false;
	res = search_uci(&UCI_state.p, UCI_state.lim, &UCI_state.tt, &UCI_state.stop);
	printf("bestmove ");
	print_move(res.best);
	printf("\n");
	return 0;
}

static void
uci_go(const char *args)
{
	uci_stop("");

	if (str_consume(&args, "perft"))
	{
		str_ltrim(&args);
		perft_verbose(&UCI_state.p, (unsigned) strtoul(args, nullptr, 10));
		return;
	}

	else if (str_consume(&args, "depth"))
	{
		str_ltrim(&args);
		unsigned depth = (unsigned) strtoul(args, nullptr, 10);
		UCI_state.lim = (limits)
		{
			.type=DEPTH,
			.depth=depth,
		};
	}

	else if (str_consume(&args, "movetime"))
	{
		str_ltrim(&args);
		millis ms = (millis) (strtoull(args, nullptr, 10));
		UCI_state.lim = (limits)
		{
			.type=MOVETIME,
			.movetime=ms,
		};
	}

	else if (str_consume(&args, "nodes"))
	{
		str_ltrim(&args);
		node_count nodes = (node_count) (strtoull(args, nullptr, 10));
		UCI_state.lim = (limits)
		{
			.type=NODES,
			.nodes=nodes,
		};
	}

	else if (str_consume(&args, "infinite"))
	{
		UCI_state.lim = (limits) { .type=INFINITE };
	}

	else
	{
		UCI_state.lim = (limits) { .type=CLOCK };

		if (!str_consume(&args, "wtime")) assert(false);
		if (!str_ltrim(&args))            assert(false);
		UCI_state.lim.time[WHITE] = (unsigned) strtoul(args, (char **) &args, 10); // TODO: why does strtoul need a char instead of const char
		if (!str_ltrim(&args))            assert(false);
		if (!str_consume(&args, "btime")) assert(false);
		if (!str_ltrim(&args))            assert(false);
		UCI_state.lim.time[BLACK] = (unsigned) strtoul(args, (char **) &args, 10);

		if (str_ltrim(&args) && str_consume(&args, "winc"))
		{
			if (!str_ltrim(&args))           assert(false);
			UCI_state.lim.inc[WHITE] = (unsigned) strtoul(args, (char **) &args, 10);
			if (!str_ltrim(&args))           assert(false);
			if (!str_consume(&args, "binc")) assert(false);
			if (!str_ltrim(&args))           assert(false);
			UCI_state.lim.inc[BLACK] = (unsigned) strtoul(args, nullptr, 10);
		}
	}

	UCI_state.joined = false;
	thrd_create(&UCI_state.search_thread, &search_start, NULL);
}

static void
uci_stop(const char *)
{
	if (!UCI_state.joined)
	{
		UCI_state.stop = true;
		thrd_join(UCI_state.search_thread, NULL);
		UCI_state.joined = true;
	}
}

static void
uci_ponderhit(const char *)
{
}

static void
uci_d(const char *)
{
	print_board(&UCI_state.p);
}

const struct
{
	const char *cmd;
	void (*handler)(const char *args);
} UCI_HANDLERS[] =
{
	{ "quit"      , uci_quit       },
	{ "ucinewgame", uci_ucinewgame },
	{ "uci"       , uci_uci        },
	{ "isready"   , uci_isready    },
	{ "setoption" , uci_setoption  },
	{ "position"  , uci_position   },
	{ "go"        , uci_go         },
	{ "stop"      , uci_stop       },
	{ "ponderhit" , uci_ponderhit  },
	{ "d"         , uci_d          },
};

static void
uci_handle(const char *cmd)
{
	usize i;

	for (i = 0; i < lengthof(UCI_HANDLERS); ++i)
	{
		if (str_consume(&cmd, UCI_HANDLERS[i].cmd))
		{
			str_ltrim(&cmd);
			UCI_HANDLERS[i].handler(cmd);
			return;
		}
	}

	puts("Invalid command!");
}

static constexpr usize UCI_BUF_SIZE = 4096;

void
uci_loop(void)
{
	char line[UCI_BUF_SIZE];

	UCI_state.quit = false;
	UCI_state.joined = true;

	uci_default_options();
	uci_ucinewgame("");

	while (!UCI_state.quit)
	{
		memset(line, 0, UCI_BUF_SIZE);
		if (fgets(line, UCI_BUF_SIZE, stdin) == nullptr)
			uci_quit("");
		else
			uci_handle(line);
	}

	tt_free(&UCI_state.tt);
}
