#ifndef NYX_POSITION_H
#define NYX_POSITION_H

#include <nyx/types.h>

typedef struct state_frame
{
	square ep;
	pctype capture;
	castling_rights castle;
	unsigned rule50;
	int material;

	// transient
	struct state_frame *previous;
	bitboard checkers;
	bitboard blockers[NUM_COLORS];
	bitboard check_squares[NUM_PIECE_TYPES];
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

square king_square(const position *p, color c);

void finalize_position(position *p);

#endif // NYX_POSITION_H
