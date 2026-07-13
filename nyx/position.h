#ifndef NYX_POSITION_H
#define NYX_POSITION_H

#include <nyx/types.h>

typedef struct
{
	square ep;
	cptype capture;
	castling_rights castle;
	unsigned rule50;
	unsigned plies;
} state_frame;

typedef struct
{
	color stm;

	state_frame *sf;

	bitboard by_color[NUM_COLORS];
	bitboard by_ptype[NUM_PIECE_TYPES];
	cptype by_square[NUM_SQUARES];
} position;

state_frame do_move(position *p, move m);
void undo_move(position *p, move m, state_frame sf);

#endif // NYX_POSITION_H
