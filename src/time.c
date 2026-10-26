// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <assert.h>
#include <compat/time.h>
#include <nyx/time.h>
#include <stdatomic.h>

millis
now(void)
{
	struct timespec ts;

	timespec_get(&ts, TIME_MONOTONIC);

	return (millis) ((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}

void
tm_start(time_manager *tm)
{
	tm->start = now();
}

bool
tm_hard_expired(const time_manager *tm, unsigned depth, node_count nodes, const position *pos)
{
	millis ms;

	if (atomic_load_explicit(tm->stop, memory_order_relaxed) || depth >= MAX_PLY) return true;

	ms = now();

	if (tm->l.type == MOVETIME) assert(ms >= tm->start);

	switch (tm->l.type)
	{
	case MOVETIME : return ms - tm->start > tm->l.movetime;
	case DEPTH    : return depth > tm->l.depth;
	case NODES    : return nodes > tm->l.nodes;
	case CLOCK    : return ms - tm->start > tm->l.time[pos->root_stm] / (40 - pos->root_fullmove); /* TODO really bad approximator */
	case INFINITE : return false;
	case MATE     : return true;
	}

	return true;
}

bool
tm_soft_expired(const time_manager *tm, unsigned depth, node_count nodes, const position *pos)
{
	return tm_hard_expired(tm, depth, nodes, pos);
}
