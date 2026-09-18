// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_TRANSPOSITION_H
#define NYX_TRANSPOSITION_H

#include <nyx/types.h>

typedef enum
{
	EXACT,
	UPPER,
	LOWER,
} bound;

typedef struct
{
	bound type;

	int score;
	move best_move;
	u8 depth;
	u64 key;

	u8 generation;
} tt_entry;

typedef struct
{
	size_t capacity;
	tt_entry *entries;
} transposition_table;

void tt_resize(transposition_table *tt, size_t capacity);
void tt_clear(transposition_table *tt);
bool tt_probe(const transposition_table *tt, u64 key, tt_entry *res);
void tt_store(transposition_table *tt, tt_entry ent);
void tt_free(transposition_table *tt);

#endif // NYX_TRANSPOSITION_H
