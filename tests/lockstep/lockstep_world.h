/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
//
// lockstep_world.h -- a small brush world for the pmove lockstep harness.
//
// Both the retail Pmove and FnQL-game's Pmove trace against this world
// through the same callbacks, so it only has to be deterministic and rich
// enough to reach every movement path: flat ground, steps of several
// heights, walkable and steep slopes, a low ceiling, a water pool with a
// ledge, a slick patch and a high platform to fall from.
//

#ifndef LOCKSTEP_WORLD_H
#define LOCKSTEP_WORLD_H

#include "../../code/game/q_shared.h"

void	LW_Init( void );
void	QDECL LW_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
			const vec3_t end, int passEntityNum, int contentMask );
int		QDECL LW_PointContents( const vec3_t point, int passEntityNum );

#endif
