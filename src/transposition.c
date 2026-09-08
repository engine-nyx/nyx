// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/transposition.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

bool
tt_probe(const transposition_table *tt, u64 key, tt_entry *res)
{
	tt_entry ent;

	ent = tt->entries[key % tt->capacity];
	if (ent.key != key)
		return false;

	*res = ent;
	return true;
}

void
tt_store(transposition_table *tt, tt_entry ent)
{
	size_t idx;

	idx = ent.key % tt->capacity;

	if (tt->entries[idx].generation <= ent.generation)
		tt->entries[idx] = ent;
}

void
tt_resize(transposition_table *tt, size_t capacity)
{
	tt->capacity = capacity;

	if (tt->entries) free(tt->entries);
	tt->entries = calloc(tt->capacity, sizeof(tt_entry));
	assert(tt && "Not enough memory for transposition table");
}

void
tt_clear(transposition_table *tt)
{
	memset(tt->entries, 0, tt->capacity * sizeof(tt_entry));
}

void
tt_free(transposition_table *tt)
{
	if (tt->entries)
		free(tt->entries);
}
