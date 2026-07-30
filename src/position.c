#include <assert.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/attacks.h>
#include <nyx/utils.h>

square
king_square(const position *p, color c)
{
	return lsb(p->by_ptype[KING] & p->by_color[c]);
}

void
put_piece(position *p, pctype pc, square sq)
{
	p->by_square[sq] = pc;
	p->by_ptype[ALL]          |= sqbb(sq);
	p->by_ptype[ptype_of(pc)] |= sqbb(sq);
	p->by_color[color_of(pc)] |= sqbb(sq);
}

static void
remove_piece(position *p, square sq)
{
	pctype pc;

	pc = p->by_square[sq];

	p->by_ptype[ALL]          ^= sqbb(sq);
	p->by_ptype[ptype_of(pc)] ^= sqbb(sq);
	p->by_color[color_of(pc)] ^= sqbb(sq);
	p->by_square[sq] = EMPTY;
}

static void
move_piece(position *p, square from, square to)
{
	pctype pc;
	bitboard from_to;

	pc = p->by_square[from];
	from_to = sqbb(from) | sqbb(to);

	p->by_ptype[ALL]          ^= from_to;
	p->by_ptype[ptype_of(pc)] ^= from_to;
	p->by_color[color_of(pc)] ^= from_to;
	p->by_square[from] = EMPTY;
	p->by_square[to]   = pc;
}

static void
swap_piece(position *p, pctype pc, square sq)
{
	remove_piece(p, sq);
	put_piece(p, pc, sq);
}

extern bitboard between_lut[NUM_SQUARES][NUM_SQUARES];
extern bitboard dia_straight_lut[NUM_SQUARES][NUM_SQUARES];

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
		line = between_lut[ksq][sniper] & occ;

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
		sqbb(ksq + white_black(-7, +7, p->stm)) |
		sqbb(ksq + white_black(-9, +9, p->stm));
}

static bool
gives_check(const position *p, move m)
{
	color them;
	bitboard occ, ksq, attacks;

	them = other_color(p->stm);
	ksq = king_square(p, them);

	// direct check
	if (p->sf->check_squares[ptype_of(p->by_square[m.from])] & m.to)
		return true;

	// discovered check
	if (p->sf->blockers[them] & sqbb(m.from))
		return !(dia_straight_lut[m.from][m.to] & ksq) || m.type == CASTLING;

	switch (m.type)
	{
	case NORMAL:
		return false;

	case PROMOTION:
		attacks = attacks_piece(promtype_of(m), m.to, p->by_ptype[ALL] ^ sqbb(m.from));
		return attacks & king_square(p, them);

	case EN_PASSANT:
		occ = p->by_ptype[ALL];
		occ ^= sqbb(square_of(file_of(m.to), rank_of(m.from)));
		occ ^= sqbb(m.from);
		occ |= sqbb(m.to);

		attacks = EMPTYBB;
		attacks |= attacks_rook  (ksq, occ) & (p->by_ptype[QUEEN] | p->by_ptype[ROOK]);
		attacks |= attacks_bishop(ksq, occ) & (p->by_ptype[QUEEN] | p->by_ptype[BISHOP]);

		return attacks & p->by_color[p->stm];

	case CASTLING:
		return p->sf->check_squares[ROOK] &
			sqbb(square_of(m.from < m.to ? F1 : D1, rank_of(m.from)));
	}

	assert(false);
}

void
do_move(position *p, move m, state_frame *sf)
{
	assert(color_of(p->by_square[m.from]) == p->stm && "Wrong color moved");

	pctype pc;

	*sf = *p->sf;
	sf->previous = p->sf;

	update_blockers(p, WHITE);
	update_blockers(p, BLACK);
	update_check_squares(p);

	pc = p->by_square[m.from];
	sf->capture = (m.type == EN_PASSANT) ? pctype_of(PAWN, other_color(p->stm)) : p->by_square[m.to];

	sf->ep = NO_EP;
	if (ptype_of(pc) == PAWN)
	{
		if ((m.from ^ m.to) == 16)
		{
			sf->ep = (m.from + m.to) / 2;

			// TODO: skip if no one can take ep
		}
	}

	switch (m.type)
	{
	case CASTLING:
		// king
		move_piece(p, m.from, m.to);
		// rook
		move_piece(p, m.from + (m.from < m.to ? +3 : -4), (m.from + m.to) / 2);
		break;
	case PROMOTION:
		pc = pctype_of(m.promotion, p->stm);

		if (sf->capture)
		{
			remove_piece(p, m.from);
			swap_piece(p, pc, m.to);
		}
		else
		{
			remove_piece(p, m.to);
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

	sf->checkers = gives_check(p, m) ? attackers(p, lsb(p->by_ptype[KING])) & p->by_color[other_color(p->stm)] : 0;

	p->sf = sf;
	p->stm = other_color(p->stm);
	++p->ply;
}

void
undo_move(position *p, move m)
{
	--p->ply;
	p->stm = other_color(p->stm);

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
		move_piece(p, m.to, m.from);
		put_piece(p, pctype_of(PAWN, other_color(p->stm)), m.to + white_black(-8, +8, p->stm));
		break;
	case NORMAL:
		move_piece(p, m.to, m.from);
		if (p->sf->capture)
			put_piece(p, p->sf->capture, m.to);
		break;
	}

	p->sf = p->sf->previous;
}
