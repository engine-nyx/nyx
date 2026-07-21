#include <assert.h>
#include <nyx/position.h>
#include <nyx/types.h>
#include <nyx/attacks.h>
#include <nyx/utils.h>

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
	bool gives_check;

	*sf = *p->sf;
	sf->previous = p->sf;

	// TODO: detect check
	gives_check = false;
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
		remove_piece(p, p->sf->ep + white_black(-8, +8, p->stm));
		move_piece(p, m.from, m.to);
		break;
	}

	sf->checkers = gives_check ? attackers(p, lsb(p->by_ptype[KING] & p->by_color[other_color(p->stm)])) : 0;

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
		break;
	case PROMOTION:
		swap_piece(p, pctype_of(PAWN, p->stm), m.to);
		move_piece(p, m.to, m.from);
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

size_t
do_lan_move(position *p, const char *lan, state_frame *sf)
{
	move m;
	ptype pt;

	m.from = square_of(lan[0] - 'a', lan[1] - '1');
	m.to   = square_of(lan[2] - 'a', lan[3] - '1');

	pt = ptype_of(p->by_square[m.from]);

	if (pt == PAWN && (rank_of(m.to) == 0 || rank_of(m.to) == 7))
	{
		m.type = PROMOTION;

		switch (lan[4])
		{
		case 'n': m.promotion = promtype_of(KNIGHT); break;
		case 'b': m.promotion = promtype_of(BISHOP); break;
		case 'r': m.promotion = promtype_of(ROOK  ); break;
		case 'q': m.promotion = promtype_of(QUEEN ); break;
		default: assert(false && "Invalid LAN promotion");
		}
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
