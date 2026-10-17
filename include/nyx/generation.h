// Copyright (c) 2026 Kilian Chung
// SPDX-License-Identifier: NLPL

#ifndef NYX_GENERATION_H
#define NYX_GENERATION_H

#include <nyx/position.h>
#include <nyx/types.h>

constexpr usize MAX_MOVES = 256;

void generation_init(void);

usize generate_captures     (const position *p, move *ms);
usize generate_quiets       (const position *p, move *ms);
usize generate_evasions     (const position *p, move *ms);
usize generate_non_evasions (const position *p, move *ms);
usize generate_legals       (const position *p, move *ms);

enum generation_type
{
	CAPTURES,
	QUIETS,
	EVASIONS,
	NON_EVASIONS,
	LEGALS,
};

usize generate(enum generation_type gt, const position *p, move *ms);

#endif // NYX_GENERATION_H
