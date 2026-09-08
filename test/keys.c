#include <string.h>
#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/position.h>
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

static struct test_result
key_round_trip(position *p, move m, u64 key_before)
{
	if (p->key != key_before)
		return TEST_FAILURE("key changed before making move");

	do_move(p, m, &(state_frame) {});
	if (p->key == key_before)
		return TEST_FAILURE("key did not change after making move");

	undo_move(p, m);
	if (p->key != key_before)
		return TEST_FAILURE("key differs after undo");

	return TEST_SUCCESS;
}

static struct test_result
check_all_legal_keys(const char *fen)
{
	position p;
	state_frame base;
	move ms[MAX_MOVES];
	u64 key;
	size_t i, n;

	parse_fen(fen, &p, &base);
	key = p.key;

	if (p.key == 0)
		return TEST_FAILURE("startposition key is zero");

	n = generate_legals(&p, ms);

	for (i = 0; i < n; ++i)
	{
		struct test_result res = key_round_trip(&p, ms[i], key);
		if (res.failed) return res;
	}

	return TEST_SUCCESS;
}

TEST(key_uniqueness_of_legal_moves)
{
	position p;
	state_frame base;
	move ms[MAX_MOVES];
	u64 before;
	u64 first = 0, second = 0;
	size_t i, n;

	parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &p, &base);
	before = p.key;

	n = generate_legals(&p, ms);

	for (i = 0; i < n; ++i)
	{
		u64 k;

		do_move(&p, ms[i], &(state_frame){ 0 });
		k = p.key;
		undo_move(&p, ms[i]);

		if (k == before)
			return TEST_FAILURE("key collides with parent position");
		if (k == first || k == second)
			return TEST_FAILURE("two legal moves produced the same key");

		second = first;
		first = k;
	}

	return TEST_SUCCESS;
}

TEST(make_unmake_keys_start_position)
{
	return check_all_legal_keys("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST(make_unmake_keys_en_passant)
{
	return check_all_legal_keys("8/8/8/3pP3/8/8/8/4K2k w - d6 0 1");
}

TEST(make_unmake_keys_promotion)
{
	struct test_result r;

	r = check_all_legal_keys("8/P7/8/8/8/8/8/4K2k w - - 0 1");
	if (r.failed) return r;

	return check_all_legal_keys("1b6/P7/8/8/8/8/8/4K2k w - - 0 1");
}

TEST(make_unmake_keys_check)
{
	return check_all_legal_keys("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
}

TEST(make_unmake_keys_castling)
{
	struct test_result r;

	r = check_all_legal_keys("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
	if (r.failed) return r;

	return check_all_legal_keys("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
}

TEST(tt_store_probe_roundtrip)
{
	transposition_table tt = { 0 };
	tt_entry ent = { 0 };
	tt_entry res;

	tt_resize(&tt, 1024);

	ent.type = EXACT;
	ent.score = 42;
	ent.best_move = (move){ .from = E2, .to = E4, .prom = 0, .type = NORMAL };
	ent.depth = 5;
	ent.key = 0xDEADBEEFCAFEBABE;
	ent.generation = 1;

	tt_store(&tt, ent);

	if (!tt_probe(&tt, ent.key, &res))
		return TEST_FAILURE("store followed by probe failed");

	test_eq(res.type, EXACT, "stored type");
	test_eq(res.score, 42, "stored score");
	test_eq(res.best_move.from, E2, "stored move from");
	test_eq(res.best_move.to, E4, "stored move to");
	test_eq(res.depth, 5, "stored depth");
	test_eq(res.generation, 1, "stored generation");

	if (res.key != ent.key)
		return TEST_FAILURE("stored key differs");

	if (tt_probe(&tt, 0x1234567890ABCDEF, &res))
		return TEST_FAILURE("probe found an entry for a different key");

	tt_free(&tt);
	return TEST_SUCCESS;
}

TEST(tt_replacement_rule)
{
	transposition_table tt = { 0 };
	tt_entry a = { 0 }, b = { 0 };
	tt_entry res;

	tt_resize(&tt, 1024);

	a.key = 0x11111111;
	a.score = 1;
	a.generation = 1;
	tt_store(&tt, a);

	b.key = 0x11111111;
	b.score = 2;
	b.generation = 1;
	tt_store(&tt, b);

	if (!tt_probe(&tt, a.key, &res))
		return TEST_FAILURE("probe after replacement failed");
	test_eq(res.score, 2, "equal generation replaces old entry");

	b.score = 3;
	b.generation = 0;
	tt_store(&tt, b);

	if (!tt_probe(&tt, a.key, &res))
		return TEST_FAILURE("probe after stale store failed");
	test_eq(res.score, 2, "stale generation does not replace");

	tt_free(&tt);
	return TEST_SUCCESS;
}
