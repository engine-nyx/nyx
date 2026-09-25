// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/transposition.h>
#include <nyx/utils.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t cluster_size = 4;

static tt_entry *
cluster_of(const transposition_table *tt, u64 key)
{
	return &tt->entries[(key % (tt->capacity / cluster_size)) * cluster_size];
}

bool
tt_probe(const transposition_table *tt, u64 key, tt_entry *res)
{
	tt_entry *ent;
	size_t i;

	ent = cluster_of(tt, key);
	for (i = 0; i < cluster_size; ++i)
	{
		if (ent[i].key == (key & BITMASK(16)))
		{
			*res = ent[i];
			return true;
		}
	}

	return false;
}

static u8
age(const transposition_table *tt, u8 generation)
{
	return (tt->generation - generation) & BITMASK(5);
}

void
tt_store(transposition_table *tt, u64 key, tt_entry data)
{
	tt_entry *dest, *ent;
	size_t i;

	data.generation = tt->generation;
	data.key = (u16) key;

	// replace data if entry already present
	ent = cluster_of(tt, key);
	for (i = 0; i < cluster_size; ++i)
	{
		if (ent[i].key != (key & BITMASK(16))) continue;

		if (data.depth > ent[i].depth)
			ent[i] = data;

		return;
	}

	// replace older entries with strategy
	dest = ent = cluster_of(tt, key);
	for (i = 1; i < cluster_size; ++i)
		if (ent[i].depth - (8 * age(tt, ent[i].generation)) < dest->depth - (8 * age(tt, dest->generation)))
			dest = ent;
	*dest = data;
}

void
tt_resize(transposition_table *tt, size_t capacity)
{
	assert(capacity && "TT cannot have capacity 0");
	tt->capacity = ((capacity + (cluster_size - 1)) / cluster_size) * cluster_size;
	tt->entries = realloc(tt->entries, tt->capacity * sizeof(tt_entry));
	assert(tt->entries && "Not enough memory for transposition table");
	tt_clear(tt);
}

bool
is_empty_entry(tt_entry ent)
{
	return !memcmp(&ent, &(tt_entry) {}, sizeof(tt_entry));
}

void
tt_clear(transposition_table *tt)
{
	size_t i;

	memset(tt->entries, 0, tt->capacity * sizeof(tt_entry));

	for (i = 0; i < tt->capacity; ++i)
		assert(is_empty_entry(tt->entries[i]) && "Entry not empty after tt_clear");

	tt->generation = 0;
}

void
tt_free(transposition_table *tt)
{
	if (tt->entries)
		free(tt->entries);
}
