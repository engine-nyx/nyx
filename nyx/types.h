#ifndef NYX_TYPES_H
#define NYX_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
} ptype;

typedef enum
{
	WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
	BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
} cptype;
constexpr size_t NUM_COLORED_PIECE_TYPES = 12;

typedef enum
{
	WHITE = WHITE_PAWN - PAWN,
	BLACK = BLACK_PAWN - PAWN,
} color;
static inline color
other_color(color c) { return WHITE + BLACK - c; }

typedef enum
{
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
} square;
constexpr size_t NUM_SQUARES = 64;
static inline unsigned
rank_of(square sq) { return (sq >> 3); }
static inline unsigned
file_of(square sq) { return (sq & 7); }


typedef uint64_t bitboard;
static inline bitboard
sqbb(square sq) { return (((bitboard) 1) << sq); }

typedef enum
{
	NORMAL,
	PROMOTION,
	EN_PASSANT,
	CASTLING,
} mtype;

typedef struct
{
	uint16_t
		to        : 6,
		from      : 6,
		promotion : 2,
		type      : 2;
} move;

#endif // NYX_TYPES_H
