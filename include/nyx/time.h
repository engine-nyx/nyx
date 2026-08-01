#ifndef NYX_TIME_H
#define NYX_TIME_H

#include <stdatomic.h>
#include <stdint.h>

typedef uint_fast64_t millis;

typedef struct
{
	millis start, soft, hard;
	millis remaining, inc;
	atomic_bool *stop;
} time_manager;

struct time_manager tm_init(millis remaining, millis inc, atomic_bool *stop);
bool tm_hard_expired(const time_manager *tm);
bool tm_soft_expired(const time_manager *tm);

#endif // NYX_TIME_H
