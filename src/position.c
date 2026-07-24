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

static bool
gives_check(const position *p, move m)
{
	// TODO: additional checks

	switch (m.type)
	{
	case NORMAL:
		return false;
	case PROMOTION:
		return attacks_piece(promtype_of(m), m.to, p->by_ptype[ALL] ^ sqbb(m.from)) & king_square(p, other_color(p->stm));
	case EN_PASSANT:
		return false; // TODO: crazy logic
	case CASTLING:
		return false; // TODO: crazy logic
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

	// TODO: detect check
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

	sf->checkers = gives_check(p, m) ? attackers(p, lsb(p->by_ptype[KING] & p->by_color[other_color(p->stm)])) : 0;

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
