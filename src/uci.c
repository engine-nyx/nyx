#include <nyx/generation.h>
#include <nyx/search.h>
#include <assert.h>
#include <nyx/perft.h>
#include <nyx/uci.h>
#include <nyx/time.h>
#include <nyx/utils.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct
{
	time_manager tm;
	atomic_bool stop;
	position p;
	state_frame sf;

	bool quit;
} UCI_state;

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

static void
uci_uci(const char *args)
{
	(void) args;

	uci_position("startpos");
	puts("uciok");
}

static void
uci_ucinewgame(const char *args)
{
	(void) args;

	uci_stop("");
	uci_position("startpos");
}

static void
uci_isready(const char *args)
{
	(void) args;

	puts("readyok");
}

static void
uci_setoption(const char *args)
{
	(void) args;

	// TODO
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
	if (str_consume(&args, "startpos"))
	{
		parse_fen(startpos, &UCI_state.p, &UCI_state.sf);
	}

	else if (str_consume(&args, "fen"))
	{
		assert(str_ltrim(&args) > 0 && "fen separator");
		args += parse_fen(args, &UCI_state.p, &UCI_state.sf);
	}

	else
	{
		assert(false && "Invalid position command");
	}

	if (!str_ltrim(&args) || !str_consume(&args, "moves") || !str_ltrim(&args))
		return;

	while (is_move(args))
	{
		args += do_lan_move(&UCI_state.p, args, &UCI_state.sf);
		if (!str_ltrim(&args)) break;
	}
}

static void
uci_go(const char *args)
{
	limits l;

	if (str_consume(&args, "perft"))
	{
		str_ltrim(&args);
		perft(&UCI_state.p, (unsigned) strtoul(args, nullptr, 10));
		return;
	}

	else if (str_consume(&args, "depth"))
	{
		str_ltrim(&args);
		unsigned depth = (unsigned) strtoul(args, nullptr, 10);
		l = (limits)
		{
			.type=DEPTH,
			.depth=depth,
		};
	}

	else if (str_consume(&args, "movetime"))
	{
		str_ltrim(&args);
		millis ms = (millis) (strtoull(args, nullptr, 10));
		l = (limits)
		{
			.type=MOVETIME,
			.movetime=ms,
		};
	}

	else if (str_consume(&args, "nodes"))
	{
		str_ltrim(&args);
		node_count nodes = (node_count) (strtoull(args, nullptr, 10));
		l = (limits)
		{
			.type=NODES,
			.nodes=nodes,
		};
	}

	else
	{
		l = (limits) { .type=CLOCK };

		if (!str_consume(&args, "wtime")) assert(false);
		if (!str_ltrim(&args))            assert(false);
		l.time[WHITE] = (unsigned) strtoul(args, (char **) &args, 10); // TODO: why does strtoul need a char instead of const char
		if (!str_ltrim(&args))            assert(false);
		if (!str_consume(&args, "btime")) assert(false);
		if (!str_ltrim(&args))            assert(false);
		l.time[BLACK] = (unsigned) strtoul(args, (char **) &args, 10);

		if (str_ltrim(&args) && str_consume(&args, "winc"))
		{
			if (!str_ltrim(&args))           assert(false);
			l.inc[WHITE] = (unsigned) strtoul(args, (char **) &args, 10);
			if (!str_ltrim(&args))           assert(false);
			if (!str_consume(&args, "binc")) assert(false);
			if (!str_ltrim(&args))           assert(false);
			l.inc[BLACK] = (unsigned) strtoul(args, nullptr, 10);
		}
	}

	move best;

	best = search(&UCI_state.p, l).best;
	printf("bestmove ");
	print_move(best);
	printf("\n");
}

static void
uci_stop(const char *args)
{
	(void) args;

	UCI_state.stop = true;
}

static void
uci_ponderhit(const char *args)
{
	(void) args;
}

static void
uci_d(const char *args)
{
	(void) args;

	print_board(&UCI_state.p);

	move ms[MAX_MOVES];
	size_t num_moves = generate_legals(&UCI_state.p, ms);
	for (size_t i = 0; i < num_moves; ++i)
	{
		print_move(ms[i]);
		printf("\n");
	}
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
	constexpr size_t num_uci_handlers = sizeof(UCI_HANDLERS) / sizeof(*UCI_HANDLERS);
	size_t i;

	for (i = 0; i < num_uci_handlers; ++i)
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

static constexpr size_t UCI_BUF_SIZE = 400;

void
uci_loop(void)
{
	char line[UCI_BUF_SIZE];

	UCI_state.quit = false;

	while (!UCI_state.quit)
	{
		if (fgets(line, UCI_BUF_SIZE, stdin) == nullptr)
			uci_quit("");
		else
			uci_handle(line);
		fflush(stdout);
	}
}
