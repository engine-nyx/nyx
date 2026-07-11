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

	occ = 0b\
00000000\
00000000\
00000000\
00000000\
00010000\
00000000\
00000000\
00000000\
;
	exp = 0b\
10000000\
01000001\
00100010\
00010100\
00000000\
00010100\
00100010\
01000001\
;

	if (attacks_bishop(F4, occ) != exp)
		return TEST_FAILURE("Position 1 failed!");

	occ = 0b\
00000000\
00100000\
00100100\
00000100\
00100000\
01001000\
10110100\
00000100\
;
	exp = 0b\
00000010\
00000100\
10001000\
01010000\
00000000\
01010000\
00001000\
00000100\
;

	return attacks_bishop(D4, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_rook_attacks)
{
	bitboard occ, exp;

	occ = 0b\
00000000\
00000001\
00000000\
00000000\
00010001\
00000000\
00000000\
00000000\
;
	exp = 0b\
00000000\
00000001\
00000001\
00000001\
00011110\
00000001\
00000001\
00000001\
;

	if (attacks_rook(E3, occ) != exp)
		return TEST_FAILURE("Position 1 failed!");

	occ = 0b\
00000001\
00100000\
01000100\
00010100\
01100000\
01011000\
10110100\
01000100\
;
	exp = 0b\
00000000\
00000000\
00000000\
00010000\
00010000\
01101000\
00010000\
00000000\
;

	return attacks_rook(A4, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_king_attacks)
{
	bitboard exp;

	exp = 0b\
00000000\
00000000\
00000000\
00011100\
00010100\
00011100\
00000000\
00000000\
;

	if (attacks_king(D4) != exp)
		return TEST_FAILURE("Position 1 failed!");

	exp = 0b\
00000000\
00000000\
00000000\
00000000\
00000000\
00000000\
11000000\
01000000\
;

	return attacks_king(H1) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}

TEST(pseudo_knight_attacks)
{
	bitboard exp;

	exp = 0b\
00000000\
00000000\
00010100\
00100010\
00000000\
00100010\
00010100\
00000000\
;

	if (attacks_knight(D4) != exp)
		return TEST_FAILURE("Position 1 failed!");

	exp = 0b\
00000000\
00000000\
00000000\
00000000\
00000000\
00000010\
00000100\
00000000\
;

	return attacks_knight(A1) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 2 failed!");
}
