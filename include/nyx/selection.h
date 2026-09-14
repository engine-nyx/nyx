// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_SELECTION_H
#define NYX_SELECTION_H

#include <nyx/types.h>
#include <nyx/position.h>
#include <nyx/generation.h>

struct scored_move
{
	move m;
	int score;
};

typedef struct
{
	const position *pos;
	int stage;
	move tt;

	struct scored_move sms[MAX_MOVES];
	size_t num_moves, current;
	size_t num_captures, num_good_captures;
} selector;

enum search_stage
{
	MAIN,
	QUIESCENCE,
};

selector selector_of(const position *p, move tt, enum search_stage stage);
move select(selector *s);

#endif // NYX_SELECTION_H
