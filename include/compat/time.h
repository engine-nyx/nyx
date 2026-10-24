#ifndef COMPAT_TIME_H
#define COMPAT_TIME_H

#include <time.h>

#ifndef TIME_MONOTONIC
#define TIME_MONOTONIC TIME_UTC
#endif // TIME_MONOTONIC

#endif // COMPAT_TIME_H
