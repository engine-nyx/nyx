#ifndef NYX_SEARCH_H
#define NYX_SEARCH_H

#include <nyx/types.h>
#include <nyx/perft.h>
#include <nyx/position.h>

enum limit_type
{
	MOVETIME,
	NODES,
	DEPTH,
	INFINITE,
	MATE,

	CLOCK,
};

typedef struct
{
	enum limit_type type;

	union
	{
		millis movetime;
		node_count nodes;
		unsigned depth;
		unsigned mate;
		struct
		{
			unsigned time[NUM_COLORS];
			unsigned inc[NUM_COLORS];
		};
	};
} limits;

struct search_state
{
	node_count nodes;
	unsigned depth;
};

struct search_result
{
	move best;
	unsigned depth;
	node_count nodes;
};

struct search_result search(position *p, limits l);

#endif // NYX_SEARCH_H
