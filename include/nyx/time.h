#ifndef NYX_TIME_H
#define NYX_TIME_H

#include <stdatomic.h>
#include <stdint.h>

typedef uint_fast64_t millis;

struct time_manager
{
	millis start, soft, hard;
	millis remaining, inc;
	atomic_bool *stop;
};

struct time_manager tm_init(millis remaining, millis inc, atomic_bool *stop);
bool tm_hard_expired(struct time_manager tm);
bool tm_soft_expired(struct time_manager tm);

#endif // NYX_TIME_H
