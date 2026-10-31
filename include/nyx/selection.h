// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_SELECTION_H
#define NYX_SELECTION_H

#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/types.h>

struct scored_move
{
	move m;
	int score;
};

typedef struct
{
	int stage;
	move tt;
	const struct search_state *ss;

	struct scored_move sms[MAX_MOVES];
	usize num_moves, current;
	usize num_captures, num_good_captures, num_good_quiets;
	usize selected;
} selector;

enum search_stage
{
	MAIN,
	QUIESCENCE,
};

selector selector_of(const struct search_state *ss, move tt, enum search_stage stage);
move select(selector *s);

#endif // NYX_SELECTION_H
