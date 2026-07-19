#ifndef NYX_ATTACKS_H
#define NYX_ATTACKS_H

#include <nyx/types.h>
#include <nyx/position.h>

void attacks_init(void);

bitboard attacks_bishop(square sq, bitboard occ);
bitboard attacks_rook  (square sq, bitboard occ);
bitboard attacks_queen (square sq, bitboard occ);
bitboard attacks_knight(square sq);
bitboard attacks_king  (square sq);
bitboard attacks_piece (ptype pt, square sq, bitboard occ);

bitboard attackers(const position *p, square sq);

#endif // NYX_ATTACKS_H
