#ifndef NYX_PERFT_H
#define NYX_PERFT_H

#include <nyx/position.h>
#include <nyx/types.h>

node_count perft_verbose(position *p, unsigned depth);
node_count perft(position *p, unsigned depth);

#endif // NYX_PERFT_H
