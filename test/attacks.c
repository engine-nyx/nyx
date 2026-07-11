#include <nyx/attacks.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <test/base.h>

BEFORE()
{
	attacks_init();
	return TEST_SUCCESS;
}

TEST(bishop_attacks)
{
	bitboard occ, exp;

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

	return attacks_bishop(F4, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 1 failed!");
}

TEST(rook_attacks)
{
	bitboard occ, exp;

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

	return attacks_rook(E3, occ) == exp ? TEST_SUCCESS : TEST_FAILURE("Position 1 failed!");
}
