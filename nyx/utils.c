#include <ctype.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <stdio.h>
#include <string.h>

unsigned
str_consume(const char **s, const char *pattern)
{
	size_t len;

	len = strlen(pattern);
	if (strncmp(*s, pattern, len))
		return 0;

	*s += len;
	return len;
}

unsigned
str_ltrim(const char **s)
{
	unsigned count;

	for (count = 0; isblank(**s); ++count)
		++*s;

	return count;
}

void
bb_print(bitboard bb)
{
	size_t i, j;
	bitboard mask;

	printf("┌───┬───┬───┬───┬───┬───┬───┬───┐\n");
	for (i = 0; i < 8; ++i)
	{
		printf("│");

		for (j = 0; j < 8; ++j)
		{
			mask = 1ull << (((7 - i) * 8) + j);
			printf(" %c │", bb & mask ? 'X' : ' ');
		}

		printf("\n");
		if (i < 7)
			printf("├───┼───┼───┼───┼───┼───┼───┼───┤\n");
	}

	printf("└───┴───┴───┴───┴───┴───┴───┴───┘\n");
}
