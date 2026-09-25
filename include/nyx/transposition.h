// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_TRANSPOSITION_H
#define NYX_TRANSPOSITION_H

#include <nyx/types.h>

enum bound_type
{
	EXACT,
	UPPER,
	LOWER,
};

typedef struct
{
	i16 score;
	u16 key;

	move best_move;

	u8 depth;
	u8
		bound      : 2,
		pv         : 1,
		generation : 5;
} tt_entry;

typedef struct
{
	size_t capacity;
	tt_entry *entries;
	u8 generation;
} transposition_table;

void tt_resize(transposition_table *tt, size_t capacity);
void tt_clear(transposition_table *tt);
bool tt_probe(const transposition_table *tt, u64 key, tt_entry *res);
void tt_store(transposition_table *tt, u64 key, tt_entry data);
void tt_free(transposition_table *tt);

#endif // NYX_TRANSPOSITION_H
