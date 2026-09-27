// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/attacks.h>
#include <nyx/evaluation.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <test/base.h>

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

BEFORE()
{
	attacks_init();
	generation_init();
	evaluation_init();
	return TEST_SUCCESS;
}

static int
eval_fen(const char *fen)
{
	position p;
	state_frame sf;

	sf = (state_frame) {};
	parse_fen(fen, &p, &sf);

	return evaluate(&p);
}

// Removes a piece without touching anything the evaluation does not read,
// so both positions can be compared directly.
static void
drop_piece(position *p, square sq)
{
	pctype pc;

	pc = p->by_square[sq];
	if (pc == EMPTY) return;

	p->by_square[sq] = EMPTY;
	p->by_ptype[ALL]            &= ~bbsq(sq);
	p->by_ptype[ptype_of(pc)]    &= ~bbsq(sq);
	p->by_color[color_of(pc)]    &= ~bbsq(sq);
}

// Same position, but the side to move is one piece short of it.
static struct test_result
check_missing_own_piece(const char *fen, square sq)
{
	position full, trimmed;
	state_frame sf;
	int with_piece, without_piece;

	sf = (state_frame) {};
	parse_fen(fen, &full, &sf);
	trimmed = full;

	if (full.by_square[sq] == EMPTY)
		return TEST_FAILURE("piece to drop is not on the board");

	with_piece = evaluate(&full);
	drop_piece(&trimmed, sq);
	without_piece = evaluate(&trimmed);

	test_lt(without_piece, with_piece, "dropping a piece of the side to move");

	return TEST_SUCCESS;
}

// Same position, but the opponent is one piece short of it.
static struct test_result
check_missing_enemy_piece(const char *fen, square sq)
{
	position full, trimmed;
	state_frame sf;
	int with_piece, without_piece;

	sf = (state_frame) {};
	parse_fen(fen, &full, &sf);
	trimmed = full;

	if (full.by_square[sq] == EMPTY)
		return TEST_FAILURE("piece to drop is not on the board");

	with_piece = evaluate(&full);
	drop_piece(&trimmed, sq);
	without_piece = evaluate(&trimmed);

	test_gt(without_piece, with_piece, "dropping a piece of the opponent");

	return TEST_SUCCESS;
}

TEST(eval_start_position_is_equal)
{
	test_eq(eval_fen(START_FEN), 0, "start position");
	return TEST_SUCCESS;
}

// Every piece here sits on the mirror of an opposing piece, so the two sides
// contribute the exact opposite amounts to the same table entries.
TEST(eval_mirrored_material_is_equal)
{
	const char *fen = "4k3/r7/3pq3/2n5/2N5/3PQ3/R7/4K3 w - - 0 1";

	test_eq(eval_fen(fen), 0, "mirrored material");
	return TEST_SUCCESS;
}

TEST(eval_side_to_move_ahead_is_positive)
{
	test_gt(eval_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1"), 0, "white up a queen");
	return TEST_SUCCESS;
}

TEST(eval_side_to_move_behind_is_negative)
{
	test_lt(eval_fen("3qk3/8/8/8/8/8/8/4K3 w - - 0 1"), 0, "white down a queen");
	return TEST_SUCCESS;
}

TEST(eval_missing_own_pawn_lowers_score)
{
	return check_missing_own_piece(START_FEN, D2);
}

TEST(eval_missing_own_queen_lowers_score)
{
	return check_missing_own_piece("4k3/8/8/8/8/8/8/3QK3 w - - 0 1", D1);
}

TEST(eval_missing_enemy_pawn_raises_score)
{
	return check_missing_enemy_piece(START_FEN, D7);
}

TEST(eval_missing_enemy_rook_raises_score)
{
	return check_missing_enemy_piece(START_FEN, D8);
}

TEST(eval_extra_own_piece_raises_score)
{
	test_gt(eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBQR w KQkq - 0 1"), eval_fen(START_FEN), "knight traded for a queen");
	return TEST_SUCCESS;
}

TEST(eval_extra_enemy_piece_lowers_score)
{
	test_lt(eval_fen("rnbqkbqr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), eval_fen(START_FEN), "knight traded for a queen");
	return TEST_SUCCESS;
}

// Same pieces, same side to move, mirrored board, so the score has to match.
TEST(eval_mirrored_position_scores_equally)
{
	int normal = eval_fen("4k3/8/8/8/8/3PPP2/3QK3/8 w - - 0 1");
	int mirrored = eval_fen("8/3qk3/3ppp2/8/8/8/8/4K3 b - - 0 1");

	test_eq(normal, mirrored, "mirrored position");
	return TEST_SUCCESS;
}

TEST(eval_ignores_non_piece_state)
{
	int with_ep = eval_fen("8/8/8/3pP3/8/8/8/4K2k w - d6 0 1");
	int without_ep = eval_fen("8/8/8/3pP3/8/8/8/4K2k w - - 17 3");

	test_eq(with_ep, without_ep, "en passant and rule50 state");
	return TEST_SUCCESS;
}

TEST(eval_survives_make_unmake)
{
	position p;
	state_frame sf;
	move ms[MAX_MOVES];
	size_t i, n;
	int before;

	sf = (state_frame) {};
	parse_fen(START_FEN, &p, &sf);
	before = evaluate(&p);

	n = generate_legals(&p, ms);

	for (i = 0; i < n; ++i)
	{
		do_move(&p, ms[i], &(state_frame) {});
		undo_move(&p, ms[i]);

		test_eq(evaluate(&p), before, "score after undo");
	}

	return TEST_SUCCESS;
}
