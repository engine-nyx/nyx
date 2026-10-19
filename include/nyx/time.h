// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_TIME_H
#define NYX_TIME_H

#include <nyx/search.h>
#include <nyx/types.h>
#include <stdatomic.h>

typedef struct
{
	millis start, soft, hard;
	millis remaining, inc;
	atomic_bool *stop;
	limits l;
} time_manager;

struct time_manager tm_init(millis remaining, millis inc, atomic_bool *stop);
bool tm_hard_expired(const time_manager *tm, unsigned depth, node_count nodes, const position *pos);
bool tm_soft_expired(const time_manager *tm, unsigned depth, node_count nodes, const position *pos);
void tm_start(time_manager *tm);
millis now(void);

#endif // NYX_TIME_H
