// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_SEARCH_H
#define NYX_SEARCH_H

#include <nyx/position.h>
#include <nyx/transposition.h>
#include <nyx/types.h>
#include <stdatomic.h>

enum limit_type
{
	MOVETIME,
	NODES,
	DEPTH,
	INFINITE,
	MATE,

	CLOCK,
};

typedef struct
{
	enum limit_type type;

	union
	{
		millis movetime;
		node_count nodes;
		unsigned depth;
		unsigned mate;
		struct
		{
			unsigned time[NUM_COLORS];
			unsigned inc[NUM_COLORS];
		};
	};
} limits;

constexpr size_t MAX_PLY = 100;

struct search_state
{
	node_count nodes, qnodes;
	unsigned depth, qsearch_max, ply;

	position *p;
	transposition_table *tt;

	move pv[MAX_PLY];
};

struct search_result
{
	move best, pv[MAX_PLY];
	unsigned depth;
	node_count nodes, qnodes;
	int score;
};

struct search_result search(position *p, limits l, transposition_table *tt, atomic_bool *stop);

#endif // NYX_SEARCH_H
