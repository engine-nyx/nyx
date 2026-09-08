// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <stdlib.h>
#include <nyx/attacks.h>
#include <nyx/utils.h>

static u64 zobrist_piece_square[12][64];
static u64 zobrist_stm;
static u64 zobrist_ep[8];
static u64 zobrist_castling[16];

static inline u64
zobrist_of(pctype pc, square sq)
{
	size_t pc_idx;

	pc_idx = (ptype_of(pc) - 1) + (color_of(pc) * 6);

	return zobrist_piece_square[pc_idx][sq];
}

static inline u64
random_key(void)
{
	return (u64)
		((rand() & BITMASK(16)) << 48) |
		((rand() & BITMASK(16)) << 32) |
		((rand() & BITMASK(16)) << 16) |
		((rand() & BITMASK(16)) << 00) ;
}

void
position_init(void)
{
	square sq;
	unsigned piece, file, cr;

	for (piece = 0; piece < 12; ++piece)
	{
		for (sq = 0; sq < NUM_SQUARES; ++sq)
		{
			zobrist_piece_square[piece][sq] = random_key();
		}
	}

	zobrist_stm = random_key();

	for (file = 0; file < 8; ++file)
		zobrist_ep[file] = random_key();

	for (cr = 0; cr < 16; ++cr)
		zobrist_castling[cr] = random_key();
}

square
king_square(const position *p, color c)
{
	return lsb(p->by_ptype[KING] & p->by_color[c]);
}

static int PIECE_VALUE[NUM_PIECE_COLORED_TYPES] =
{
	[EMPTY] = 0,

	[WHITE_PAWN]   = 10,
	[WHITE_KNIGHT] = 20,
	[WHITE_BISHOP] = 25,
	[WHITE_ROOK]   = 50,
	[WHITE_QUEEN]  = 90,
	[WHITE_KING]   = oo,

	[BLACK_PAWN]   = -10,
	[BLACK_KNIGHT] = -20,
	[BLACK_BISHOP] = -25,
	[BLACK_ROOK]   = -50,
	[BLACK_QUEEN]  = -90,
	[BLACK_KING]   = -oo,
};

void
put_piece(position *p, pctype pc, square sq)
{
	p->by_square[sq] = pc;
	p->by_ptype[ALL]          |= bbsq(sq);
	p->by_ptype[ptype_of(pc)] |= bbsq(sq);
	p->by_color[color_of(pc)] |= bbsq(sq);

	p->sf->material += PIECE_VALUE[pc];
	p->key ^= zobrist_of(pc, sq);
}

static void
remove_piece(position *p, square sq)
{
	pctype pc;

	pc = p->by_square[sq];

	p->by_square[sq] = EMPTY;
	p->by_ptype[ALL]          ^= bbsq(sq);
	p->by_ptype[ptype_of(pc)] ^= bbsq(sq);
	p->by_color[color_of(pc)] ^= bbsq(sq);

	p->sf->material -= PIECE_VALUE[pc];
	p->key ^= zobrist_of(pc, sq);
}

static void
move_piece(position *p, square from, square to)
{
	pctype pc;
	bitboard from_to;

	pc = p->by_square[from];
	from_to = bbsq(from) | bbsq(to);

	p->by_square[from] = EMPTY;
	p->by_square[to]   = pc;
	p->by_ptype[ALL]          ^= from_to;
	p->by_ptype[ptype_of(pc)] ^= from_to;
	p->by_color[color_of(pc)] ^= from_to;

	p->key ^= zobrist_of(pc, from) ^ zobrist_of(pc, to);
}

static void
swap_piece(position *p, pctype pc, square sq)
{
	remove_piece(p, sq);
	put_piece(p, pc, sq);
}

extern bitboard between_lut[NUM_SQUARES][NUM_SQUARES];
extern bitboard dia_straight_lut[NUM_SQUARES][NUM_SQUARES];
castling_rights castling_rights_mask[NUM_SQUARES] =
{
	[A1] = WHITE_OOO,
	[E1] = WHITE_CASTLING,
	[H1] = WHITE_OO,
	[A8] = BLACK_OOO,
	[E8] = BLACK_CASTLING,
	[H8] = BLACK_OO,
};

static void
update_blockers(position *p, color c)
{
	square ksq, sniper;
	bitboard snipers, occ, line;

	ksq = king_square(p, c);
	snipers = EMPTYBB;
	snipers |= attacks_rook  (ksq, EMPTYBB) & (p->by_ptype[QUEEN] | p->by_ptype[ROOK]);
	snipers |= attacks_bishop(ksq, EMPTYBB) & (p->by_ptype[QUEEN] | p->by_ptype[BISHOP]);
	snipers &= p->by_color[other_color(c)];
	occ = p->by_ptype[ALL] ^ snipers;

	p->sf->blockers[c] = EMPTYBB;
	while (snipers)
	{
		sniper = pop_lsb(&snipers);
		line = between_lut[sniper][ksq] & occ;

		if (popcnt(line) == 1)
		{
			p->sf->blockers[c] |= line;
		}
	}
}

static void
update_check_squares(position *p)
{
	square ksq;
	bitboard occ;

	ksq = king_square(p, other_color(p->stm));
	occ = p->by_ptype[ALL];

	p->sf->check_squares[KNIGHT] = attacks_knight(ksq);
	p->sf->check_squares[BISHOP] = attacks_bishop(ksq, occ);
	p->sf->check_squares[ROOK]   = attacks_rook  (ksq, occ);
	p->sf->check_squares[QUEEN]  = attacks_queen (ksq, occ);
	p->sf->check_squares[KING]   = EMPTYBB;
	p->sf->check_squares[PAWN]   =
		bbsq(ksq + white_black(-7, +7, p->stm)) |
		bbsq(ksq + white_black(-9, +9, p->stm));
}

static bool
gives_check(const position *p, move m)
{
	color them;
	bitboard occ, ksq, attacks;

	them = other_color(p->stm);
	ksq = king_square(p, them);

	// direct check
	if (p->sf->check_squares[ptype_of(p->by_square[m.from])] & bbsq(m.to))
		return true;

	// discovered check
	if (p->sf->blockers[them] & bbsq(m.from))
		return !(dia_straight_lut[m.from][m.to] & bbsq(ksq)) || m.type == CASTLING;

	switch (m.type)
	{
	case NORMAL:
		return false;

	case PROMOTION:
		attacks = attacks_piece(promtype_of(m), m.to, p->by_ptype[ALL] ^ bbsq(m.from));
		return attacks & bbsq(king_square(p, them));

	case EN_PASSANT:
		occ = p->by_ptype[ALL];
		occ ^= bbsq(square_of(file_of(m.to), rank_of(m.from)));
		occ ^= bbsq(m.from);
		occ |= bbsq(m.to);

		attacks = EMPTYBB;
		attacks |= attacks_rook  (ksq, occ) & (p->by_ptype[QUEEN] | p->by_ptype[ROOK]);
		attacks |= attacks_bishop(ksq, occ) & (p->by_ptype[QUEEN] | p->by_ptype[BISHOP]);

		return attacks & p->by_color[p->stm];

	case CASTLING:
		return p->sf->check_squares[ROOK] &
			bbsq(square_of(m.from < m.to ? F1 : D1, rank_of(m.from)));
	}

	assert(false);
}

static inline bool
en_passant_capturable(const position *p, move m)
{
	color them;
	square ep, ksq;
	bitboard their_attacks, prev_blockers;
	bool we_discover, they_cover;

	them = other_color(p->stm);
	ep = (m.from + m.to) / 2;
	their_attacks =
		attacks_pawn(ep, p->stm) &
		p->by_ptype[PAWN] &
		p->by_color[them];

	if (!their_attacks) return false;

	ksq = king_square(p, them);
	prev_blockers = p->sf->previous->blockers[them];

	we_discover = (prev_blockers & bbsq(m.from)) && file_of(m.from) != file_of(ksq);
	they_cover = their_attacks & (~prev_blockers | dia_straight_lut[ksq][ep]);

	return !we_discover && they_cover;
}

void
do_move(position *p, move m, state_frame *sf)
{
	assert(color_of(p->by_square[m.from]) == p->stm && "Wrong color moved");

	color them;
	pctype pc;
	bool check;

	*sf = *p->sf;
	sf->previous = p->sf;
	p->sf = sf;

	them = other_color(p->stm);
	pc = p->by_square[m.from];
	sf->capture = (m.type == EN_PASSANT) ? pctype_of(PAWN, them) : p->by_square[m.to];
	check = gives_check(p, m);

	switch (m.type)
	{
	case CASTLING:
		// king
		move_piece(p, m.from, m.to);
		// rook
		move_piece(p, m.from + (m.from < m.to ? +3 : -4), (m.from + m.to) / 2);
		break;
	case PROMOTION:
		pc = pctype_of(promtype_of(m), p->stm);

		if (sf->capture)
		{
			remove_piece(p, m.from);
			swap_piece(p, pc, m.to);
		}
		else
		{
			remove_piece(p, m.from);
			put_piece(p, pc, m.to);
		}
		break;
	case NORMAL:
		if (sf->capture)
			remove_piece(p, m.to);
		move_piece(p, m.from, m.to);
		break;
	case EN_PASSANT:
		remove_piece(p, p->sf->ep + white_black(-8, +8, p->stm));
		move_piece(p, m.from, m.to);
		break;
	}

	if (sf->ep != NO_EP)
	{
		p->key ^= zobrist_ep[file_of(sf->ep)];
		sf->ep = NO_EP;
	}
	if (ptype_of(pc) == PAWN && (m.from ^ m.to) == 16 && en_passant_capturable(p, m))
	{
		sf->ep = (m.from + m.to) / 2;
		p->key ^= zobrist_ep[file_of(sf->ep)];
	}

	p->key ^= zobrist_castling[p->sf->castle];
	p->sf->castle &= ~(castling_rights_mask[m.from] | castling_rights_mask[m.to]);
	p->key ^= zobrist_castling[p->sf->castle];


	sf->checkers = check ? attackers(p, king_square(p, them)) & p->by_color[p->stm] : 0;

	p->stm = them;
	p->key ^= zobrist_stm;
	++p->ply;

	if (!(ptype_of(sf->capture) == KING)) // TODO outsource this check to prev ply in search
		finalize_position(p);
}

void
finalize_position(position *p)
{
	update_blockers(p, WHITE);
	update_blockers(p, BLACK);
	update_check_squares(p);
}

void
undo_move(position *p, move m)
{
	--p->ply;
	p->stm = other_color(p->stm);
	p->key ^= zobrist_stm;

	switch (m.type)
	{
	case CASTLING:
		// king
		move_piece(p, m.to, m.from);
		// rook
		move_piece(p, (m.from + m.to) / 2, m.from + (m.from < m.to ? +3 : -4));
		break;
	case PROMOTION:
		swap_piece(p, pctype_of(PAWN, p->stm), m.to);
		move_piece(p, m.to, m.from);
		if (p->sf->capture)
			put_piece(p, p->sf->capture, m.to);
		break;
	case EN_PASSANT:
		put_piece(p, p->sf->capture, m.to + white_black(-8, +8, p->stm));
		move_piece(p, m.to, m.from);
		break;
	case NORMAL:
		move_piece(p, m.to, m.from);
		if (p->sf->capture)
			put_piece(p, p->sf->capture, m.to);
		break;
	}

	p->key ^= zobrist_castling[p->sf->castle];
	if (p->sf->ep != NO_EP) p->key ^= zobrist_ep[file_of(p->sf->ep)];

	p->sf = p->sf->previous;

	p->key ^= zobrist_castling[p->sf->castle];
	if (p->sf->ep != NO_EP) p->key ^= zobrist_ep[file_of(p->sf->ep)];
}
