// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <string.h>
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
	position_init();
	return TEST_SUCCESS;
}

static struct test_result
make_unmake_compare(position *p, move m, state_frame *base)
{
	position before;
	state_frame sf_before, sf;

	memcpy(&before, p, sizeof(position));
	memcpy(&sf_before, base, sizeof(state_frame));

	do_move(p, m, &sf);
	undo_move(p, m);

	if (memcmp(&before, p, sizeof(position)))
		return TEST_FAILURE("position differs after undo");
	if (memcmp(&sf_before, base, sizeof(state_frame)))
		return TEST_FAILURE("state frame differs after undo");

	return TEST_SUCCESS;
}

static struct test_result
check_all_legals(const char *fen)
{
	position p;
	state_frame base;
	move ms[MAX_MOVES];
	usize i, n;

	base = (state_frame) {}; // zero-initialize everything including padding bytes
	parse_fen(fen, &p, &base);

	n = generate_legals(&p, ms);

	for (i = 0; i < n; ++i)
	{
		struct test_result res = make_unmake_compare(&p, ms[i], &base);
		if (res.failed) return res;
	}

	return TEST_SUCCESS;
}

TEST(make_unmake_start_position)
{
	return check_all_legals("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST(make_unmake_en_passant)
{
	return check_all_legals("8/8/8/3pP3/8/8/8/4K2k w - d6 0 1");
}

TEST(make_unmake_promotion)
{
	struct test_result r;

	r = check_all_legals("8/P7/8/8/8/8/8/4K2k w - - 0 1");
	if (r.failed) return r;

	return check_all_legals("1b6/P7/8/8/8/8/8/4K2k w - - 0 1");
}

TEST(make_unmake_check)
{
	return check_all_legals("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
}

TEST(make_unmake_castling)
{
	struct test_result r;

	r = check_all_legals("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
	if (r.failed) return r;

	return check_all_legals("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
}

TEST(null_move_start_position)
{
	position p, before;
	state_frame base, sf, base_before;
	u64 key_before;

	base = (state_frame) {}; // zero-initialize everything including padding
	parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &p, &base);

	memcpy(&before, &p, sizeof(position));
	memcpy(&base_before, &base, sizeof(state_frame));
	key_before = p.key;

	do_move(&p, NULL_MOVE, &sf);

	test_eq(p.stm, BLACK, "black should be to move after white's null move");
	test_eq(p.ply, before.ply + 1, "ply should increase by one");
	test_eq(sf.rule50, base_before.rule50 + 1, "rule50 should increase by one");
	test_eq(sf.ep, NO_EP, "null move should not create an en passant square");
	test_eq(sf.castle, base_before.castle, "castling rights should not change");
	test_eq(p.sf == &sf, 1, "position should reference the new state frame");
	test_eq(sf.previous == &base, 1, "new state frame should link to the previous one");
	test_eq(sf.key, key_before, "state frame should remember the parent key");
	test_eq(memcmp(p.by_color, before.by_color, sizeof p.by_color), 0, "piece colors should not change");
	test_eq(memcmp(p.by_ptype, before.by_ptype, sizeof p.by_ptype), 0, "piece types should not change");
	test_eq(memcmp(p.by_square, before.by_square, sizeof p.by_square), 0, "board contents should not change");
	test_ne(p.key, key_before, "key should change when the side to move changes");

	undo_move(&p, NULL_MOVE);

	test_eq(p.stm, before.stm, "side to move should be restored");
	test_eq(p.ply, before.ply, "ply should be restored");
	test_eq(memcmp(&p, &before, sizeof(position)), 0, "position should be restored after undo");
	test_eq(memcmp(&base, &base_before, sizeof(state_frame)), 0, "state frame should not be modified");

	return TEST_SUCCESS;
}

TEST(null_move_clears_en_passant)
{
	position p, before;
	state_frame base, sf, base_before;
	u64 key_before;

	base = (state_frame) {};
	parse_fen("8/8/8/3pP3/8/8/8/4K2k w - d6 0 1", &p, &base);

	memcpy(&before, &p, sizeof(position));
	memcpy(&base_before, &base, sizeof(state_frame));
	key_before = p.key;

	test_eq(base_before.ep, D6, "test position should have an en passant square");

	do_move(&p, NULL_MOVE, &sf);

	test_eq(p.stm, BLACK, "black should be to move after white's null move");
	test_eq(sf.ep, NO_EP, "null move should clear the en passant square");
	test_eq(sf.rule50, base_before.rule50 + 1, "rule50 should increase by one");
	test_eq(sf.castle, base_before.castle, "castling rights should not change");
	test_ne(p.key, key_before, "key should change when the en passant square and side to move change");

	undo_move(&p, NULL_MOVE);

	test_eq(memcmp(&p, &before, sizeof(position)), 0, "position should be restored after undo");
	test_eq(memcmp(&base, &base_before, sizeof(state_frame)), 0, "state frame should be fully restored");

	return TEST_SUCCESS;
}

TEST(null_move_then_check_detection)
{
	position p, before;
	state_frame base, sf, move_sf;
	move check_move = { .from = B4, .to = A4 };

	base = (state_frame) {};
	parse_fen("7k/8/8/8/1r6/8/8/K7 w - - 0 1", &p, &base);

	memcpy(&before, &p, sizeof(position));

	test_eq(p.stm, WHITE, "white should be to move");
	test_eq(p.sf->checkers, 0, "white should not be in check");

	do_move(&p, NULL_MOVE, &sf);
	test_eq(p.stm, BLACK, "black should be to move after the null move");
	do_move(&p, check_move, &move_sf);

	test_eq(p.stm, WHITE, "white should be to move after black's rook move");
	test_eq(p.sf->checkers, bbsq(A4), "the rook on a4 should be registered as the checker");

	undo_move(&p, check_move);
	undo_move(&p, NULL_MOVE);

	test_eq(memcmp(&p, &before, sizeof(position)), 0,
		"position should be restored after the null move and the rook move");

	return TEST_SUCCESS;
}
