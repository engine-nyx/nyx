#include <assert.h>
#include <stdbit.h>
#include <ctype.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <stdio.h>
#include <nyx/position.h>
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
print_bitboard(bitboard bb)
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

static char PIECE_CHAR[NUM_PIECE_COLORED_TYPES] =
{
	[EMPTY]=' ',
	[WHITE_PAWN]='P', [WHITE_KNIGHT]='N', [WHITE_BISHOP]='B', [WHITE_ROOK]='R', [WHITE_QUEEN]='Q', [WHITE_KING]='K',
	[BLACK_PAWN]='p', [BLACK_KNIGHT]='n', [BLACK_BISHOP]='b', [BLACK_ROOK]='r', [BLACK_QUEEN]='q', [BLACK_KING]='k',
};

void
print_board(position *p)
{
	size_t i, j;
	square sq;

	printf("┌───┬───┬───┬───┬───┬───┬───┬───┐\n");
	for (i = 0; i < 8; ++i)
	{
		printf("│");

		for (j = 0; j < 8; ++j)
		{
			sq = square_of(j, 7 - i);
			printf(" %c │", PIECE_CHAR[p->by_square[sq]]);
		}

		printf("\n");
		if (i < 7)
			printf("├───┼───┼───┼───┼───┼───┼───┼───┤\n");
	}

	printf("└───┴───┴───┴───┴───┴───┴───┴───┘\n");
}

static size_t
parse_board(const char *s, position *p)
{
	size_t i;
	unsigned file, rank;

	i = 0;
	for (rank = 7; rank < 8; --rank)
	{
		for (file = 0; file < 8; ++file)
		{
			switch (s[i++])
			{
				case '8': file += 7; break;
				case '7': file += 6; break;
				case '6': file += 5; break;
				case '5': file += 4; break;
				case '4': file += 3; break;
				case '3': file += 2; break;
				case '2': file += 1; break;
				case '1': file += 0; break;

				case 'P': put_piece(p, WHITE_PAWN  , square_of(file, rank)); break;
				case 'N': put_piece(p, WHITE_KNIGHT, square_of(file, rank)); break;
				case 'B': put_piece(p, WHITE_BISHOP, square_of(file, rank)); break;
				case 'R': put_piece(p, WHITE_ROOK  , square_of(file, rank)); break;
				case 'Q': put_piece(p, WHITE_QUEEN , square_of(file, rank)); break;
				case 'K': put_piece(p, WHITE_KING  , square_of(file, rank)); break;

				case 'p': put_piece(p, BLACK_PAWN  , square_of(file, rank)); break;
				case 'n': put_piece(p, BLACK_KNIGHT, square_of(file, rank)); break;
				case 'b': put_piece(p, BLACK_BISHOP, square_of(file, rank)); break;
				case 'r': put_piece(p, BLACK_ROOK  , square_of(file, rank)); break;
				case 'q': put_piece(p, BLACK_QUEEN , square_of(file, rank)); break;
				case 'k': put_piece(p, BLACK_KING  , square_of(file, rank)); break;

				default: assert(false && "Invalid symbol");
			}
		}

		assert(file == 8 && "Must fill all files");
		if (rank > 0)
			assert(s[i++] == '/' && "Rank separator");
	}

	return i;
}

static size_t
parse_stm(const char *s, position *p)
{
	assert((*s == 'w' || *s == 'b') && "Invalid color");

	switch (*s)
	{
	case 'w': p->stm = WHITE; break;
	case 'b': p->stm = BLACK; break;
	}

	return 1;
}

static size_t
parse_castle(const char *s, state_frame *sf)
{
	size_t i;

	i = 0;
	sf->castle = NO_CASTLING;

	if (s[i] == '-') return 1;
	if (s[i] == 'K') { sf->castle |= WHITE_OO ; ++i; }
	if (s[i] == 'Q') { sf->castle |= WHITE_OOO; ++i; }
	if (s[i] == 'k') { sf->castle |= BLACK_OO ; ++i; }
	if (s[i] == 'q') { sf->castle |= BLACK_OOO; ++i; }

	assert(i && "Invalid castle string");

	return i;
}

static size_t
parse_ep(const char *s, state_frame *sf)
{
	unsigned file, rank;

	if (*s == '-')
	{
		sf->ep = NO_EP;
		return 1;
	}

	file = s[0] - 'a';
	rank = s[1] - '1';

	assert((file < 8)               && "Invalid ep file");
	assert((rank == 2 || rank == 5) && "Invalid ep rank");

	sf->ep = square_of(file, rank);

	return 2;
}

static size_t
parse_rule50(const char *s, state_frame *sf)
{
	assert(isdigit(s[0]) && "Invalid halfmove clock");

	sf->rule50 = s[0] - '0';
	if (!sf->rule50 || !isdigit(s[1]))
		return 1;

	sf->rule50 = (10 * sf->rule50) + s[1] - '0';
	return 2;
}

static size_t
parse_ply(const char *s, position *p)
{
	size_t i;

	assert(isdigit(s[0]) && "Invalid fullmove counter");

	p->ply = s[0] - '0';
	if (!p->ply) return 1;

	for (i = 1; isdigit(s[i]); ++i)
		p->ply = (p->ply * 10) + (s[i] - '0');

	return i;
}

size_t
parse_fen(const char *fen, position *p, state_frame *sf)
{
	size_t i;

	i = 0;
	str_ltrim(&fen);

	i += parse_board (fen + i, p ); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_stm   (fen + i, p ); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_castle(fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_ep    (fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_rule50(fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_ply   (fen + i, p);

	p->sf = sf;

	return i;
}

void
print_square(square sq)
{
	printf("%c%c", file_of(sq) + 'a', rank_of(sq) + '1');
}

void
print_move(move m)
{
	print_square(m.from);
	print_square(m.to);
}

bitboard
strbb(const char *s)
{
	bitboard res;
	unsigned file, rank;

	res = 0;

	for (rank = 7; rank < 8; --rank)
	{
		for (file = 0; file < 8; ++file)
		{
			switch (*s++)
			{
			case '\0': return res;
			case ' ' : break;
			default  : res |= sqbb(square_of(file, rank));
			}
		}
	}

	return res;
}

inline square
lsb(bitboard bb)
{
	assert(bb && "No lsb of empty bitboard");

	return stdc_first_trailing_one(bb) - 1;
}

inline square
pop_lsb(bitboard* bb)
{
	square sq;

	sq = lsb(*bb);
	*bb &= *bb - 1;

	return sq;
}
