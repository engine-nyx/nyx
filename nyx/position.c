#include <assert.h>
#include <nyx/position.h>
#include <nyx/types.h>

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

void
do_move(position *p, move m, state_frame *sf)
{
	assert(color_of(p->by_square[m.from]) == p->stm && "Wrong color moved");

	pctype pc;

	*sf = *p->sf;
	sf->previous = p->sf;
	p->sf = sf;

	pc = p->by_square[m.from];
	sf->capture = (m.type == EN_PASSANT) ? pctype_of(PAWN, other_color(p->stm)) : p->by_square[m.to];

	switch (m.type)
	{
	case CASTLING: break;
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
		move_piece(p, m.from, m.to);
		break;
	}

	p->stm = other_color(p->stm);
	++p->ply;
}

void
undo_move(position *p, move m, state_frame sf)
{
	--p->ply;
	p->stm = other_color(p->stm);

	switch (m.type)
	{
	case CASTLING:
		break;
	case PROMOTION:
		swap_piece(p, pctype_of(PAWN, p->stm), m.to);
		move_piece(p, m.to, m.from);
		break;
	case EN_PASSANT:
		move_piece(p, m.to, m.from);
		if (p->stm == WHITE)
			put_piece(p, pctype_of(PAWN, other_color(p->stm)), m.to - 8);
		if (p->stm == BLACK)
			put_piece(p, pctype_of(PAWN, other_color(p->stm)), m.to + 8);
		break;
	case NORMAL:
		move_piece(p, m.to, m.from);
		if (sf.capture)
			put_piece(p, sf.capture, m.to);
		break;
	}

	p->sf = sf.previous;
}
