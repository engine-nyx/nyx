#include <nyx/time.h>
#include <time.h>
#include <assert.h>

static millis
now(void)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC); /* TODO: change to monotonic clock */

	return (millis) ((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
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
