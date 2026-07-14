#ifndef NYX_TYPES_H
#define NYX_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint64_t u64;

typedef enum
{
	WHITE,
	BLACK,
} color;
constexpr size_t NUM_COLORS = 2;

typedef enum
{
	NONE,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
	ALL
} ptype;
constexpr size_t NUM_PIECE_TYPES = 8;

typedef enum : u8
{
	EMPTY,

	WHITE_PAWN   = PAWN   + (WHITE << 3),
	WHITE_KNIGHT = KNIGHT + (WHITE << 3),
	WHITE_BISHOP = BISHOP + (WHITE << 3),
	WHITE_ROOK   = ROOK   + (WHITE << 3),
	WHITE_QUEEN  = QUEEN  + (WHITE << 3),
	WHITE_KING   = KING   + (WHITE << 3),

	BLACK_PAWN   = PAWN   + (BLACK << 3),
	BLACK_KNIGHT = KNIGHT + (BLACK << 3),
	BLACK_BISHOP = BISHOP + (BLACK << 3),
	BLACK_ROOK   = ROOK   + (BLACK << 3),
	BLACK_QUEEN  = QUEEN  + (BLACK << 3),
	BLACK_KING   = KING   + (BLACK << 3),
} pctype;
constexpr size_t NUM_PIECE_COLORED_TYPES = 16;

static inline color
other_color(color c) { return WHITE + BLACK - c; }
static inline color
color_of(pctype pc) { return pc < BLACK_PAWN ? WHITE : BLACK; }
static inline ptype
ptype_of(pctype pc) { return ((pc - 1) % 6) + 1; }
static inline pctype
pctype_of(ptype pt, color c) { return (3 << c) + pt; }

typedef enum : u8
{
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	NO_EP,
} square;
constexpr size_t NUM_SQUARES = 64;
static inline unsigned
rank_of(square sq) { return (sq >> 3); }
static inline unsigned
file_of(square sq) { return (sq & 7); }
static inline square
square_of(unsigned file, unsigned rank) { return ((rank << 3) | file); }

typedef enum : u8
{
	NO_CASTLING = 0,
	WHITE_OO    = 1,
	WHITE_OOO   = 2,
	BLACK_OO    = 4,
	BLACK_OOO   = 8,
} castling_rights;


typedef u64 bitboard;
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
