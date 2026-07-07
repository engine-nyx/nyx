#include <nyx/uci.h>
#include <nyx/time.h>
#include <nyx/utils.h>
#include <stdatomic.h>
#include <stdio.h>

static struct
{
	struct time_manager tm;
	atomic_bool stop;

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
}

static void
uci_position(const char *args)
{
	(void) args;
}

static void
uci_go(const char *args)
{
	(void) args;
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

const struct
{
	const char *cmd;
	void (*handler)(const char *args);
} UCI_HANDLERS[] =
{
	{ "quit"      , uci_quit },
	{ "ucinewgame", uci_ucinewgame },
	{ "uci"       , uci_uci },
	{ "isready"   , uci_isready },
	{ "setoption" , uci_setoption },
	{ "position"  , uci_position },
	{ "go"        , uci_go },
	{ "stop"      , uci_stop },
	{ "ponderhit" , uci_ponderhit },
};


static void
uci_handle(const char *cmd)
{
	constexpr size_t uci_handler_count = sizeof(UCI_HANDLERS) / sizeof(*UCI_HANDLERS);
	size_t i;

	for (i = 0; i < uci_handler_count; ++i)
	{
		if (str_consume(&cmd, UCI_HANDLERS[i].cmd))
		{
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
	}
}
