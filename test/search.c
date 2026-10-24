// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/position.h>
#include <nyx/search.h>
#include <nyx/transposition.h>
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

// Tests that search correctly uses TT mate scores with proper normalization
TEST(search_tt_mate_score_probe_normalization)
{
	transposition_table tt = { 0 };
	position p;
	state_frame sf;
	limits l;
	atomic_bool stop = false;
	struct search_result res;

	tt_resize(&tt, 1024 * 1024);

	// Position: White to move, mate in 1
	// 1rb5/4r3/3p1npb/3kp1P1/1P3P1P/5nR1/2Q1BK2/bN4NR w - - 0 1
	parse_fen("1rb5/4r3/3p1npb/3kp1P1/1P3P1P/5nR1/2Q1BK2/bN4NR w - - 0 1", &p, &sf);

	l = (limits) { .type = DEPTH, .depth = 5 };
	res = search(&p, l, &tt, &stop);

	// Should find mate in 1 -> score = oo - 1 = 32766
	test_gt(res.score, 32667, "found mate score (above MATE_THRESHOLD)");

	// Now verify second search from same position also finds mate using TT
	l = (limits) { .type = DEPTH, .depth = 8 };
	res = search(&p, l, &tt, &stop);
	test_gt(res.score, 32667, "second search also finds mate");

	tt_free(&tt);
	return TEST_SUCCESS;
}

// Tests mate score normalization with qsearch
TEST(qsearch_tt_mate_score_normalization)
{
	transposition_table tt = { 0 };
	position p;
	state_frame sf;
	limits l;
	atomic_bool stop = false;
	struct search_result res;

	tt_resize(&tt, 1024 * 1024);

	// Position where qsearch will find mate
	// Same mate in 1 position
	parse_fen("1rb5/4r3/3p1npb/3kp1P1/1P3P1P/5nR1/2Q1BK2/bN4NR w - - 0 1", &p, &sf);

	l = (limits) { .type = DEPTH, .depth = 1 };
	res = search(&p, l, &tt, &stop);
	test_gt(res.score, 32667, "depth 1 finds mate via qsearch");

	tt_free(&tt);
	return TEST_SUCCESS;
}

// Tests that mate scores are correctly handled across generations
TEST(tt_mate_score_across_generations)
{
	transposition_table tt = { 0 };
	tt_entry ent = { .bound = EXACT, .depth = 10 };
	tt_entry res;

	tt_resize(&tt, 1024);

	ent.score = 32760; // oo - 7
	store(&tt, 0x87654321, ent);

	tt_new_search(&tt); // increment generation

	// Entry should still be there but aged
	res = probe(&tt, 0x87654321);
	test_eq(res.bound, EXACT, "entry survives generation change");
	test_eq(res.score, 32760, "mate score preserved across generation");

	tt_free(&tt);
	return TEST_SUCCESS;
}

// Tests the boundary between mate scores and regular scores
TEST(tt_mate_threshold_boundary)
{
	transposition_table tt = { 0 };
	tt_entry ent = { .bound = EXACT, .depth = 5 };
	tt_entry res;

	tt_resize(&tt, 1024);

	// oo = 32767, MAX_PLY = 100
	// MATE_THRESHOLD = oo - MAX_PLY = 32667
	// Score just below threshold (not a mate score)
	ent.score = 32666;
	store(&tt, 0x11111111, ent);
	res = probe(&tt, 0x11111111);
	test_eq(res.score, 32666, "score below mate threshold unchanged");

	// Score at threshold (is a mate score)
	ent.score = 32667; // MATE_THRESHOLD
	store(&tt, 0x22222222, ent);
	res = probe(&tt, 0x22222222);
	test_eq(res.score, 32667, "score at mate threshold stored as-is");

	// Score above threshold (mate score)
	ent.score = 32668;
	store(&tt, 0x33333333, ent);
	res = probe(&tt, 0x33333333);
	test_eq(res.score, 32668, "score above mate threshold stored as-is");

	// Negative side
	ent.score = -32667; // -MATE_THRESHOLD
	store(&tt, 0x44444444, ent);
	res = probe(&tt, 0x44444444);
	test_eq(res.score, -32667, "negative score at threshold stored as-is");

	tt_free(&tt);
	return TEST_SUCCESS;
}

// Tests negative mate score (being mated) using mated.fen
TEST(search_negative_mate_score)
{
	transposition_table tt = { 0 };
	position p;
	state_frame sf;
	limits l;
	atomic_bool stop = false;
	struct search_result res;

	tt_resize(&tt, 1024 * 1024);

	// Position: Black to move, mated in 1
	// rnbqkbnr/ppppp2p/5p2/6pQ/4P3/3P4/PPP2PPP/RNB1KBNR b KQkq - 0 1
	parse_fen("rnbqkbnr/ppppp2p/5p2/6pQ/4P3/3P4/PPP2PPP/RNB1KBNR b KQkq - 0 1", &p, &sf);

	l = (limits) { .type = DEPTH, .depth = 5 };
	res = search(&p, l, &tt, &stop);

	// Should find mated in 1 -> score = -(oo - 1) = -32766
	test_lt(res.score, -32667, "found negative mate score (below -MATE_THRESHOLD)");

	tt_free(&tt);
	return TEST_SUCCESS;
}
