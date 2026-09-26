// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#include "nyx/types.h"
#include <nyx/utils.h>
#include <nyx/evaluation.h>

// Notice, these tables are from black's perspective
// since A1 is defined to be 0, aka the top left entry.
// The piece_square_value function inverts the ranks for white.
static int piece_square_tables[NUM_PIECE_TYPES][NUM_SQUARES] =
{
	[PAWN] =
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		50, 50, 50, 50, 50, 50, 50, 50,
		10, 10, 20, 30, 30, 20, 10, 10,
		 5,  5, 10, 25, 25, 10,  5,  5,
		 0,  0,  0, 20, 20,  0,  0,  0,
		 5, -5,-10,  0,  0,-10, -5,  5,
		 5, 10, 10,-20,-20, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0,
	},
	[KNIGHT] =
	{
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50,
	},
	[BISHOP] =
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  5,  0,  0,  0,  0,  5,-10,
		-20,-10,-10,-10,-10,-10,-10,-20,
	},
	[ROOK] =
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		 5, 10, 10, 10, 10, 10, 10,  5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		 0,  0,  0,  5,  5,  0,  0,  0,
	},
	[QUEEN] =
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20,
	},
	[KING] =
	{
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30, 10,  0,  0, 10, 30, 20,
	},
};

static int
piece_square_value(position *p)
{
	int value;
	color c;
	ptype pt;
	bitboard bb;
	square sq;

	value = 0;

	for (c = 0; c < NUM_COLORS; ++c)
	{
		for (pt = PAWN; pt <= KING; ++pt)
		{
			bb = p->by_ptype[pt] & p->by_color[c];

			while (bb)
			{
				sq = pop_lsb(&bb);
				sq = square_of(file_of(sq), white_black(7 - rank_of(sq), rank_of(sq), c));
				value += piece_square_tables[pt][sq] * white_black(+1, -1, c);
			}
		}
	}

	return value;
}

int
evaluate(position *p)
{
	int value;

	value = 0;
	value += p->sf->material;
	value += piece_square_value(p);

	return value * white_black(+1, -1, p->stm);
}
