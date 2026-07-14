#include <nyx/attacks.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <test/base.h>

BEFORE()
{
	attacks_init();
	return TEST_SUCCESS;
}

TEST(pseudo_bishop_attacks)
{
	bitboard occ, exp;

	occ = strbb
	(
		"        "
		"        "
		"        "
		"        "
		"   X    "
		"        "
		"        "
		"        "
	);

	exp = strbb
	(
		"       X"
		"X     X "
		" X   X  "
		"  X X   "
		"        "
		"  X X   "
		" X   X  "
		"X     X "
	);

	if (attacks_bishop(D4, occ) != exp)
		return TEST_FAILURE("Position 1 failed!");

	occ = strbb
	(
		"        "
		"     X  "
		"  X  X  "
		"  X     "
		"     X  "
		"   X  X "
		"  X XX X"
		"  X     "
	);
	exp = strbb
	(
		" X      "
		"  X     "
		"   X   X"
		"    X X "
		"        "
		"    X X "
		"   X    "
		"  X     "
	);

	return attacks_bishop(F4, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_rook_attacks)
{
	bitboard occ, exp;

	occ = strbb
	(
		"        "
		"X       "
		"        "
		"        "
		"X   X   "
		"        "
		"        "
		"        "
	);
	exp = strbb
	(
		"        "
		"X       "
		"X       "
		"X       "
		" XXXX   "
		"X       "
		"X       "
		"X       "
	);

	if (attacks_rook(A4, occ) != exp)
		return TEST_FAILURE("Position 1 failed!");

	occ = strbb
	(
		"X       "
		"     X  "
		"  X   X "
		"  X X   "
		"     XX "
		"   XX X "
		"  X XX X"
		"  X   X "
	);
	exp = strbb
	(
		"        "
		"        "
		"        "
		"    X   "
		"    X   "
		"   X XX "
		"    X   "
		"        "
	);

	return attacks_rook(E3, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_king_attacks)
{
	bitboard exp;

	exp = strbb
	(
		"        "
		"        "
		"        "
		"  XXX   "
		"  X X   "
		"  XXX   "
		"        "
		"        "
	);

	if (attacks_king(D4) != exp)
		return TEST_FAILURE("Position 1 failed!");

	exp = strbb
	(
		"        "
		"        "
		"        "
		"        "
		"        "
		"        "
		"      XX"
		"      X "
	);

	return attacks_king(H1) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_knight_attacks)
{
	bitboard exp;

	exp = strbb
	(
		"        "
		"        "
		"  X X   "
		" X   X  "
		"        "
		" X   X  "
		"  X X   "
		"        "
	);

	if (attacks_knight(D4) != exp)
		return TEST_FAILURE("Position 1 failed!");

	exp = strbb
	(
		"        "
		"        "
		"        "
		"        "
		"        "
		" X      "
		"  X     "
		"        "
	);

	return attacks_knight(A1) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}
