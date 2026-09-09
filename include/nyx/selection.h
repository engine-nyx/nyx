// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_SELECTION_H
#define NYX_SELECTION_H

#include <nyx/types.h>
#include <nyx/position.h>
#include <nyx/generation.h>

typedef struct
{
	const position *p;
	move ms[MAX_MOVES];
	int stage;
	move tt_move;
	size_t num_moves, current;
} selector;

move select(selector *s);

#endif // NYX_SELECTION_H
