#ifndef NYX_MOVEGEN_H
#define NYX_MOVEGEN_H

#include <nyx/position.h>
#include <nyx/types.h>
#include <stddef.h>

constexpr size_t MAX_MOVES = 256;

size_t generate_captures     (const position *p, move *ms);
size_t generate_quiets       (const position *p, move *ms);
size_t generate_evasions     (const position *p, move *ms);
size_t generate_non_evasions (const position *p, move *ms);
size_t generate_legals       (const position *p, move *ms);

#endif // NYX_MOVEGEN_H
