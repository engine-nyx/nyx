#include <nyx/time.h>
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <assert.h>

static millis
now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec * 1e3) + (ts.tv_nsec / 1e6);
}

void
tm_start(time_manager *tm)
{
	tm->start = now();
}

bool
tm_hard_expired(const time_manager *tm, struct search_state *ss)
{
	millis ms;

	ms = now();

	switch (tm->l.type)
	{
	case MOVETIME: return ms >= tm->start + tm->l.movetime;
	case DEPTH   : return ss->depth >= tm->l.depth;
	case NODES   : return ss->nodes >= tm->l.nodes;
	case CLOCK   : return true;
	case INFINITE: return false;
	case MATE    : return true;
	}

	return true;
}

bool
tm_soft_expired(const time_manager *tm, struct search_state *ss)
{
	return tm_hard_expired(tm, ss);
}
