// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: GPL-3.0-or-later

#include <nyx/types.h>
#include <test/base.h>

TEST(move_type_size)
{
	test_eq(sizeof(move), 2, "Move should be 16 bits");
	return TEST_SUCCESS;
}
