// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_UTILS_H
#define NYX_UTILS_H

#include <nyx/position.h>
#include <nyx/types.h>

#define BITMASK(bits) (bits >= sizeof(unsigned long long) * 8 ? ((unsigned long long) 0) - 1 : ((unsigned long long) 1 << (bits)) - 1)
usize str_consume(const char **s, const char *pattern);
unsigned str_ltrim(const char **s);
void print_bitboard(bitboard bb);

void print_board(const position *p);
usize parse_fen(const char *fen, position *p, state_frame *sf);

void print_square(square sq);
void print_move(move m);
void print_line(move *ms, usize len);

bitboard strbb(const char *s);

#define white_black(w, b, c) ((c) == WHITE ? (w) : (b))

square lsb(bitboard bb);
square pop_lsb(bitboard *bb);
unsigned popcnt(bitboard bb);

usize do_lan_move(position *p, const char *lan, state_frame *sf);
move promotion_move(square from, square to, ptype prom);

#endif // NYX_UTILS_H
