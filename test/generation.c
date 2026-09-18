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
check_legal_count(const position *pos, size_t expected)
{
	move ms[MAX_MOVES];
	size_t n;
	char *msg;

	n = generate_legals(pos, ms);

	if (n == expected)
		return TEST_SUCCESS;

	msg = test_malloc(96);
	if (msg == nullptr) return TEST_FAILURE("test: test_malloc failed");
	snprintf(msg, 96, "expected %zu legal moves, got %zu", expected, n);
	return TEST_FAILURE(msg);
}

static struct test_result
check_legal_count_fen(const char *fen, size_t expected)
{
	position pos;
	state_frame sf;

	parse_fen(fen, &pos, &sf);

	return check_legal_count(&pos, expected);
}

TEST(mated_board_has_no_legal_moves_black)
{
	return check_legal_count_fen("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", 0);
}

TEST(mated_board_has_no_legal_moves_white)
{
	return check_legal_count_fen("7K/6q1/6k1/8/8/8/8/8 w - - 0 1", 0);
}

TEST(non_mated_board_has_legal_moves)
{
	return check_legal_count_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 20);
}

TEST(pawn_check_evasion_check_detection)
{
	position pos;
	state_frame sf;
	move m;

	parse_fen("8/1p6/5k2/2K5/8/8/R7/8 b - - 0 1", &pos, &sf);
	m = (move) { .from=B7, .to=B6 };
	do_move(&pos, m, &sf);

	return check_legal_count(&pos, 8);
}
