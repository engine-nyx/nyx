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

constexpr usize MAX_PLY = 100;

struct search_result
{
	move best, pv[MAX_PLY];
	unsigned depth;
	node_count nodes, qnodes;
	int score;
};

struct histories
{
	i16 butterfly[NUM_PIECE_TYPES][NUM_SQUARES];
	i16 continuation[6][NUM_PIECE_TYPES][NUM_SQUARES][NUM_PIECE_TYPES][NUM_SQUARES];
};

struct search_state
{
	node_count nodes, qnodes;
	unsigned depth, qsearch_max, ply;

	transposition_table *tt;
	position *pos;

	struct histories hist;
	move pv[MAX_PLY][MAX_PLY + 1];
	move played_moves[MAX_PLY];
	ptype moved_pieces[MAX_PLY];
};

struct search_result search(position *pos, limits lim, transposition_table *tt, atomic_bool *stop);
struct search_result search_uci(position *pos, limits lim, transposition_table *tt, atomic_bool *stop);

#endif // NYX_SEARCH_H
