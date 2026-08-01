#ifndef NYX_SEARCH_H
#define NYX_SEARCH_H

#include <nyx/types.h>
#include <nyx/perft.h>
#include <nyx/position.h>
#include <nyx/time.h>

struct search_result
{
	move best;
	unsigned depth;
	node_count nodes;
};

struct search_result search(position *p, time_manager *tm);

#endif // NYX_SEARCH_H
