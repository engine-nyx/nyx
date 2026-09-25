// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/transposition.h>
#include <nyx/types.h>
#include <test/base.h>

TEST(move_type_size)
{
	test_eq(sizeof(move), 2, "Move should be 16 bits");
	return TEST_SUCCESS;
}

TEST(tt_entry_type_size)
{
	test_eq(sizeof(tt_entry), 8, "TT Entry should be 64 bits");
	return TEST_SUCCESS;
}
