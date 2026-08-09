#include <nyx/attacks.h>
#include <nyx/generation.h>
#include <nyx/perft.h>
#include <nyx/position.h>
#include <nyx/utils.h>
#include <test/base.h>

BEFORE()
{
	attacks_init();
	generation_init();
	return TEST_SUCCESS;
}

static struct test_result
check_perft(const char *fen, unsigned depth, node_count expected)
{
	position p;
	state_frame sf;
	node_count n;
	char *msg;

	parse_fen(fen, &p, &sf);
	n = perft_nodes(&p, depth);

	if (n == expected)
		return TEST_SUCCESS;

	msg = test_malloc(128);
	if (!msg) return TEST_FAILURE("test: test_malloc failed");
	snprintf(msg, 128, "depth %u: expected %llu, got %llu",
		depth, (unsigned long long) expected, (unsigned long long) n);
	return TEST_FAILURE(msg);
}

static struct test_result
check_depths(const char *fen, const node_count *expected, unsigned max_depth)
{
	struct test_result r;
	unsigned d;

	for (d = 1; d <= max_depth; ++d)
	{
		r = check_perft(fen, d, expected[d - 1]);
		if (r.failed) return r;
	}

	return TEST_SUCCESS;
}

TEST(perft_start_position)
{
	const node_count expected[] = { 20, 400, 8902, 197281, 4865609 };

	return check_depths("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", expected, 5);
}

TEST(perft_kiwipete)
{
	const node_count expected[] = { 48, 2039, 97862, 4085603 };

	return check_depths("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", expected, 4);
}

TEST(perft_position_3)
{
	const node_count expected[] = { 14, 191, 2812, 43238, 674624 };

	return check_depths("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", expected, 5);
}

TEST(perft_position_4)
{
	const node_count expected[] = { 6, 264, 9467, 422333 };

	return check_depths("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", expected, 4);
}

TEST(perft_position_5)
{
	const node_count expected[] = { 44, 1486, 62379, 2103487 };

	return check_depths("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", expected, 4);
}

TEST(perft_position_6)
{
	const node_count expected[] = { 46, 2079, 89890, 3894594 };

	return check_depths("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", expected, 4);
}
