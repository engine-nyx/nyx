#ifndef NYX_UTILS_H
#define NYX_UTILS_H

#include <nyx/types.h>
#include <nyx/position.h>

#define BITMASK(bits) (bits >= sizeof(unsigned long long) * 8 ? ((unsigned long long) 0) - 1 : ((unsigned long long) 1 << (bits)) - 1)
unsigned str_consume(const char **s, const char *pattern);
unsigned str_ltrim(const char **s);
void bb_print(bitboard bb);

void print_board(position *p);
size_t parse_fen(const char *fen, position *p, state_frame *sf);

#endif // NYX_UTILS_H
