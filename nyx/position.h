#ifndef NYX_POSITION_H
#define NYX_POSITION_H

#include <nyx/types.h>

typedef struct
{
	color stm;

	bitboard by_color[NUM_COLORS];
	bitboard by_ptype[NUM_PIECE_TYPES];
	cptype by_square[NUM_SQUARES];
} position;

#endif // NYX_POSITION_H
