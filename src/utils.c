#include <assert.h>
#include <stdbit.h>
#include <ctype.h>
#include <nyx/types.h>
#include <nyx/utils.h>
#include <stdio.h>
#include <nyx/position.h>
#include <string.h>

size_t
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
	unsigned i, j;
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
	square sq;

	for (sq = A1; sq < NUM_SQUARES; ++sq)
		p->by_square[sq] = EMPTY;
	p->by_color[WHITE] = p->by_color[BLACK] = EMPTYBB;
	p->by_ptype[ALL] =
		p->by_ptype[PAWN] =
		p->by_ptype[KNIGHT] =
		p->by_ptype[BISHOP] =
		p->by_ptype[ROOK] =
		p->by_ptype[QUEEN] =
		p->by_ptype[KING] = EMPTYBB;

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

static unsigned
get_file(char c)
{
	assert(c >= 'a' && c <= 'h');
	return (unsigned) (c - 'a');
}

static unsigned
get_rank(char c)
{
	assert(c >= '1' && c <= '8');
	return (unsigned) (c - '1');
}

static unsigned char
get_digit(char c)
{
	assert(isdigit(c));
	return (unsigned char) (c - '0');
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

	file = get_file(s[0]);
	rank = get_rank(s[1]);

	assert((rank == 2 || rank == 5) && "Invalid ep rank");

	sf->ep = square_of(file, rank);

	return 2;
}

static size_t
parse_rule50(const char *s, state_frame *sf)
{
	sf->rule50 = get_digit(s[0]);
	if (!sf->rule50 || !isdigit(s[1]))
		return 1;

	sf->rule50 = (10 * sf->rule50) + get_digit(s[0]);
	return 2;
}

static size_t
parse_ply(const char *s, position *p)
{
	size_t i;

	p->ply = get_digit(s[0]);
	if (!p->ply) return 1;

	for (i = 1; isdigit(s[i]); ++i)
		p->ply = (p->ply * 10) + get_digit(s[i]);

	return i;
}

size_t
parse_fen(const char *fen, position *p, state_frame *sf)
{
	size_t i;

	i = 0;
	str_ltrim(&fen);
	p->sf = sf;
	p->sf->material = 0;

	i += parse_board (fen + i, p ); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_stm   (fen + i, p ); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_castle(fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_ep    (fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_rule50(fen + i, sf); assert(fen[i] == ' ' && "Single space separator"); ++i;
	i += parse_ply   (fen + i, p);

	sf->checkers = EMPTYBB;
	sf->previous = nullptr;

	finalize_position(p);

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

size_t
do_lan_move(position *p, const char *lan, state_frame *sf)
{
	move m;
	ptype pt;

	m.from = square_of(get_file(lan[0]), get_rank(lan[1]));
	m.to   = square_of(get_file(lan[2]), get_rank(lan[3]));

	pt = ptype_of(p->by_square[m.from]);

	if (pt == PAWN && (rank_of(m.to) == 0 || rank_of(m.to) == 7))
	{
		m.type = PROMOTION;

		switch (lan[4])
		{
		case 'n': pt = KNIGHT; break;
		case 'b': pt = BISHOP; break;
		case 'r': pt = ROOK  ; break;
		case 'q': pt = QUEEN ; break;
		default: assert(false && "Invalid LAN promotion");
		}

		m = promotion_move(m.from, m.to, pt);
	}

	else if (pt == PAWN && file_of(m.from) != file_of(m.to) && p->by_square[m.to] == EMPTY)
	{
		m.type = EN_PASSANT;
	}

	else if (pt == KING && (file_of(m.to) == file_of(m.from) + 2 || file_of(m.from) == file_of(m.to) + 2))
	{
		m.type = CASTLING;
	}

	else
	{
		m.type = NORMAL;
	}

	do_move(p, m, sf);

	return m.type == PROMOTION ? 5 : 4;
}

unsigned
popcnt(bitboard bb)
{
	return stdc_count_ones(bb);
}

move
promotion_move(square from, square to, ptype pt)
{
	return (move)
	{
		.from=from,
		.to  =to,
		.type=PROMOTION,
		.prom=(pt - KNIGHT) & BITMASK(2),
	};
}
