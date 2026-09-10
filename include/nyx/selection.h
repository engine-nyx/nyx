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
	const position *p;
	struct scored_move sms[MAX_MOVES];
	int stage;
	move tt_move;
	size_t num_moves, current;
} selector;

move select(selector *s);

#endif // NYX_SELECTION_H
