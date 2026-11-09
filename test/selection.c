// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/selection.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <test/base.h>

BEFORE()
{
	attacks_init();
	generation_init();
	position_init();
	return TEST_SUCCESS;
}

// Histories are too large for the stack and are only read while selecting.
static struct histories hist;

static int
same_squares(move m, square from, square to)
{
	return m.from == from && m.to == to;
}

// Runs the main-stage selector for `n` moves and stores the selected moves.
static void
select_first(const char *fen, move *ms, usize n)
{
	position p;
	state_frame sf;
	struct search_state ss;
	selector s;
	usize i;

	parse_fen(fen, &p, &sf);

	ss = (struct search_state)
	{
		.pos = &p,
		.hist = &hist,
		.ply = 0,
	};

	s = selector_of(&ss, NULL_MOVE, MAIN);

	for (i = 0; i < n; ++i)
		ms[i] = select(&s);
}

// Returns 1 if `from -> to` is selected before the first quiet move. The
// selector returns good captures first, then quiets, then bad captures.
static int
capture_selected_before_quiet(const char *fen, square from, square to)
{
	position p;
	state_frame sf;
	struct search_state ss;
	selector s;
	move m;

	parse_fen(fen, &p, &sf);

	ss = (struct search_state)
	{
		.pos = &p,
		.hist = &hist,
		.ply = 0,
	};

	s = selector_of(&ss, NULL_MOVE, MAIN);

	for (;;)
	{
		m = select(&s);
		if (is_null(m)) return 0;
		if (same_squares(m, from, to)) return 1;
		if (!is_capture(&p, m)) return 0;
	}
}

// A rook takes an undefended queen, so the capture cannot lose material.
TEST(see_ge_winning_capture_is_good)
{
	move ms[1];

	// White: Ra1, Ke1. Black: Qa8, Ke8. White to move.
	select_first("q3k3/8/8/8/8/8/8/R3K3 w - - 0 1", ms, 1);

	test_eq(same_squares(ms[0], A1, A8), 1,
		"winning capture should be selected before quiet moves");

	return TEST_SUCCESS;
}

// A queen takes a rook that is defended by a pawn; the pawn recaptures.
TEST(see_ge_losing_capture_is_bad)
{
	move ms[1];

	// White: Qd4, Ka1. Black: pd5, Nc7, Kg8. White to move.
	select_first("6k1/2n5/8/3p4/3Q4/8/8/K7 w - - 0 1", ms, 1);

	test_eq(same_squares(ms[0], D4, D5), 0,
		"losing capture should not be selected before quiet moves");

	return TEST_SUCCESS;
}

// A rook takes a rook that is recaptured by a knight: an even trade.
TEST(see_ge_even_trade_is_good)
{
	move ms[1];

	// White: Ra1, Ke1. Black: Ra8, Nb6, Ke8. White to move.
	select_first("r3k3/8/1n6/8/8/8/8/R3K3 w - - 0 1", ms, 1);

	test_eq(same_squares(ms[0], A1, A8), 1,
		"capture with zero net gain should be considered good");

	return TEST_SUCCESS;
}

// The move with the higher capture score is bad, the lower-scored one is good.
// This checks that the good capture is found even after a bad capture is skipped.
TEST(see_ge_good_capture_after_bad_capture)
{
	move ms[2];

	// White: Qd4, Rb1, Ka1. Black: re5, pf6, pb7, Kh8. White to move.
	// Qxe5 loses the queen to fxe5; Rxb7 wins a pawn.
	select_first("7k/1p6/5p2/4r3/3Q4/8/8/KR6 w - - 0 1", ms, 2);

	test_eq(same_squares(ms[0], B1, B7), 1,
		"good capture should be selected before quiet moves even when scored lower");

	test_eq(same_squares(ms[1], D4, E5), 0,
		"bad capture should be deferred until after the quiet moves");

	return TEST_SUCCESS;
}

// Rxf2 is met by Qxf2, then the other rook recaptures on f2.
TEST(see_ge_capture_after_blocker_moves_aside)
{
	// White: Kb8, Rf6, Ra2. Black: Qf3, Bf2, Ke1. White to move.
	test_eq(capture_selected_before_quiet(
		"1K6/8/5R2/8/8/5q2/R4b2/4k3 w - - 0 1", A2, F2), 1,
		"capture should be considered good when a blocker moves aside");

	return TEST_SUCCESS;
}
