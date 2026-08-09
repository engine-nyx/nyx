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
	size_t i, n;

	parse_fen(fen, &p, &base);

	n = generate_legals(&p, ms);

	for (i = 0; i < n; ++i)
	{
		struct test_result r = make_unmake_compare(&p, ms[i], &base);
		if (r.failed)
		{
			printf("  (failed on move %zu/%zu: ", i + 1, n);
			print_move(ms[i]);
			printf(")\n");
			return r;
		}
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
