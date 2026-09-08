// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <test/base.h>

BEFORE()
{
	attacks_init();
	generation_init();
	return TEST_SUCCESS;
}

static struct test_result
check_legal_count(const char *fen, size_t expected)
{
	position p;
	state_frame sf;
	move ms[MAX_MOVES];
	size_t n;
	char *msg;

	parse_fen(fen, &p, &sf);
	n = generate_legals(&p, ms);

	if (n == expected)
		return TEST_SUCCESS;

	msg = test_malloc(96);
	if (msg == nullptr) return TEST_FAILURE("test: test_malloc failed");
	snprintf(msg, 96, "expected %zu legal moves, got %zu", expected, n);
	return TEST_FAILURE(msg);
}

TEST(mated_board_has_no_legal_moves_black)
{
	return check_legal_count("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", 0);
}

TEST(mated_board_has_no_legal_moves_white)
{
	return check_legal_count("7K/6q1/6k1/8/8/8/8/8 w - - 0 1", 0);
}

TEST(non_mated_board_has_legal_moves)
{
	return check_legal_count("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 20);
}