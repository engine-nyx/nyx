// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_TIME_H
#define NYX_TIME_H

#include <nyx/search.h>
#include <nyx/types.h>

typedef struct
{
	millis start, soft, hard;
	millis remaining, inc;
	limit l;
} time_manager;

bool tm_hard_expired(const time_manager *tm, const struct search_state *ss);
bool tm_soft_expired(const time_manager *tm, const struct search_state *ss);
void tm_start(time_manager *tm);
millis now(void);

#endif // NYX_TIME_H
