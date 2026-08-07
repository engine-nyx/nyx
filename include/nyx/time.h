#ifndef NYX_TIME_H
#define NYX_TIME_H

#include <stdatomic.h>
#include <nyx/search.h>
#include <nyx/types.h>

typedef struct
{
	millis start, soft, hard;
	millis remaining, inc;
	atomic_bool *stop;
	limits l;
} time_manager;

struct time_manager tm_init(millis remaining, millis inc, atomic_bool *stop);
bool tm_hard_expired(const time_manager *tm, struct search_state *ss);
bool tm_soft_expired(const time_manager *tm, struct search_state *ss);
void tm_start(time_manager *tm);

#endif // NYX_TIME_H
