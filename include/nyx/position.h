#ifndef NYX_POSITION_H
#define NYX_POSITION_H

#include <nyx/types.h>

typedef struct state_frame
{
	square ep;
	pctype capture;
	castling_rights castle;
	unsigned rule50;

	// transient
	struct state_frame *previous;
	bitboard checkers;
	bitboard blockers[NUM_COLORS];
} state_frame;

typedef struct
{
	color stm;
	unsigned ply;

	state_frame *sf;

	bitboard by_color[NUM_COLORS];
	bitboard by_ptype[NUM_PIECE_TYPES];
	pctype by_square[NUM_SQUARES];
} position;

void put_piece(position *p, pctype pc, square sq);

void do_move(position *p, move m, state_frame *sf);
void undo_move(position *p, move m);

size_t do_lan_move(position *p, const char *lan, state_frame *sf);

#endif // NYX_POSITION_H
