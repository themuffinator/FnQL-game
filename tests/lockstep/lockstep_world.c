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
// lockstep_world.c -- convex brushes and a swept-box trace against them.
//
// A brush is the intersection of half-spaces (normal . p <= dist).  A box
// sweep expands every plane by the box's support along the normal and clips
// the segment, backing off by a small epsilon so the box stops just short of
// the surface, which is the contract Pmove's slide and ground code relies on.
//

#include "lockstep_world.h"
#include "../../code/game/surfaceflags.h"

#define LW_MAX_PLANES	8
#define LW_MAX_BRUSHES	64
#define LW_EPSILON		0.125f		// distance a trace stops short of a surface

typedef struct {
	vec3_t	normal;
	float	dist;
	int		type;
	int		signbits;
} lwPlane_t;

typedef struct {
	int			numPlanes;
	lwPlane_t	planes[LW_MAX_PLANES];
	int			contents;
	int			surfaceFlags;
} lwBrush_t;

static lwBrush_t	lwBrushes[LW_MAX_BRUSHES];
static int			lwNumBrushes;

static void LW_SetPlane( lwPlane_t *plane, float nx, float ny, float nz, float dist ) {
	int i;

	VectorSet( plane->normal, nx, ny, nz );
	plane->dist = dist;
	plane->type = 3;
	for ( i = 0; i < 3; i++ ) {
		if ( plane->normal[i] == 1.0f || plane->normal[i] == -1.0f ) {
			plane->type = i;
		}
	}
	plane->signbits = 0;
	for ( i = 0; i < 3; i++ ) {
		if ( plane->normal[i] < 0.0f ) {
			plane->signbits |= 1 << i;
		}
	}
}

static lwBrush_t *LW_NewBrush( int contents, int surfaceFlags ) {
	lwBrush_t *brush;

	if ( lwNumBrushes >= LW_MAX_BRUSHES ) {
		return NULL;
	}
	brush = &lwBrushes[lwNumBrushes++];
	memset( brush, 0, sizeof( *brush ) );
	brush->contents = contents;
	brush->surfaceFlags = surfaceFlags;
	return brush;
}

static void LW_AddPlane( lwBrush_t *brush, float nx, float ny, float nz, float dist ) {
	if ( brush && brush->numPlanes < LW_MAX_PLANES ) {
		LW_SetPlane( &brush->planes[brush->numPlanes++], nx, ny, nz, dist );
	}
}

static void LW_AddBox( float x0, float y0, float z0, float x1, float y1, float z1,
	int contents, int surfaceFlags ) {
	lwBrush_t *brush = LW_NewBrush( contents, surfaceFlags );

	LW_AddPlane( brush, 1, 0, 0, x1 );
	LW_AddPlane( brush, -1, 0, 0, -x0 );
	LW_AddPlane( brush, 0, 1, 0, y1 );
	LW_AddPlane( brush, 0, -1, 0, -y0 );
	LW_AddPlane( brush, 0, 0, 1, z1 );
	LW_AddPlane( brush, 0, 0, -1, -z0 );
}

// a wedge along +x from (x0, base) rising by `rise` over its length
static void LW_AddRamp( float x0, float x1, float y0, float y1, float base, float rise ) {
	lwBrush_t *brush = LW_NewBrush( CONTENTS_SOLID, 0 );
	const float run = x1 - x0;
	const float length = (float)sqrt( run * run + rise * rise );
	const float nx = -rise / length;
	const float nz = run / length;

	LW_AddPlane( brush, nx, 0, nz, nx * x0 + nz * base );
	LW_AddPlane( brush, 1, 0, 0, x1 );
	LW_AddPlane( brush, -1, 0, 0, -x0 );
	LW_AddPlane( brush, 0, 1, 0, y1 );
	LW_AddPlane( brush, 0, -1, 0, -y0 );
	LW_AddPlane( brush, 0, 0, -1, -( base - 64.0f ) );
}

void LW_Init( void ) {
	int i;
	float height;
	static const float stepRises[] = { 8, 16, 18, 20, 22, 24, 30 };

	lwNumBrushes = 0;

	// ground, split so one tile can be slick
	LW_AddBox( -4096, -4096, -64, 1024, 4096, 0, CONTENTS_SOLID, 0 );
	LW_AddBox( 1024, -4096, -64, 1536, 4096, 0, CONTENTS_SOLID, SURF_SLICK );
	LW_AddBox( 1536, -4096, -64, 4096, 4096, 0, CONTENTS_SOLID, SURF_NODAMAGE );

	// boundary walls
	LW_AddBox( -4160, -4160, -64, -4096, 4160, 1024, CONTENTS_SOLID, 0 );
	LW_AddBox( 4096, -4160, -64, 4160, 4160, 1024, CONTENTS_SOLID, 0 );
	LW_AddBox( -4096, -4160, -64, 4096, -4096, 1024, CONTENTS_SOLID, 0 );
	LW_AddBox( -4096, 4096, -64, 4096, 4160, 1024, CONTENTS_SOLID, 0 );

	// a staircase along +x at y in [-128, 128], each step 64 deep
	height = 0.0f;
	for ( i = 0; i < ARRAY_LEN( stepRises ); i++ ) {
		height += stepRises[i];
		LW_AddBox( 256 + 64 * i, -128, -64, 256 + 64 * ( i + 1 ), 128, height, CONTENTS_SOLID, 0 );
	}
	LW_AddBox( 256 + 64 * ARRAY_LEN( stepRises ), -128, -64, 896, 128, height, CONTENTS_SOLID, 0 );

	// a walkable 30 degree ramp and a steep 55 degree one, rising along +x
	LW_AddRamp( -512, 0, 256, 512, 0, 295.6f );
	LW_AddBox( 0, 256, -64, 256, 512, 295.6f, CONTENTS_SOLID, 0 );
	LW_AddRamp( -512, -256, 640, 896, 0, 365.6f );

	// a crouch tunnel: ceiling 44 units over the floor
	LW_AddBox( -768, -512, 44, 0, -384, 128, CONTENTS_SOLID, 0 );
	LW_AddBox( -768, -544, 0, 0, -512, 128, CONTENTS_SOLID, 0 );
	LW_AddBox( -768, -384, 0, 0, -352, 128, CONTENTS_SOLID, 0 );

	// a water pool on the floor with a ledge to climb out over
	LW_AddBox( -1536, -256, 0, -1024, 256, 96, CONTENTS_WATER, 0 );
	LW_AddBox( -1600, -256, 0, -1536, 256, 112, CONTENTS_SOLID, 0 );
	LW_AddBox( -1536, -256, 0, -1504, 256, 64, CONTENTS_SOLID, 0 );

	// a high platform to fall from, and a player-clip wall
	LW_AddBox( -256, 768, 0, -128, 896, 256, CONTENTS_SOLID, 0 );
	LW_AddBox( 1600, -64, 0, 1632, 64, 128, CONTENTS_PLAYERCLIP, 0 );

	// a ladder wall and the platform at its top
	LW_AddBox( 2400, -64, 0, 2416, 64, 400, CONTENTS_SOLID, SURF_LADDER );
	LW_AddBox( 2416, -64, 0, 2600, 64, 400, CONTENTS_SOLID, 0 );
}

static void LW_TraceBrush( trace_t *trace, const lwBrush_t *brush, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs ) {
	const lwPlane_t *clip = NULL;
	float enterFrac = -1.0f;
	float leaveFrac = 1.0f;
	qboolean startOut = qfalse;
	qboolean getOut = qfalse;
	int i, j;

	for ( i = 0; i < brush->numPlanes; i++ ) {
		const lwPlane_t *plane = &brush->planes[i];
		float dist = plane->dist;
		float d1, d2, f;

		// push the plane out by the box corner that leads along the normal
		for ( j = 0; j < 3; j++ ) {
			dist -= plane->normal[j] * ( plane->normal[j] < 0.0f ? maxs[j] : mins[j] );
		}
		d1 = DotProduct( start, plane->normal ) - dist;
		d2 = DotProduct( end, plane->normal ) - dist;

		if ( d2 > 0.0f ) {
			getOut = qtrue;
		}
		if ( d1 > 0.0f ) {
			startOut = qtrue;
		}
		// entirely in front of this face: no contact with the brush
		if ( d1 > 0.0f && ( d2 >= LW_EPSILON || d2 >= d1 ) ) {
			return;
		}
		if ( d1 <= 0.0f && d2 <= 0.0f ) {
			continue;
		}
		if ( d1 > d2 ) {
			f = ( d1 - LW_EPSILON ) / ( d1 - d2 );
			if ( f < 0.0f ) {
				f = 0.0f;
			}
			if ( f > enterFrac ) {
				enterFrac = f;
				clip = plane;
			}
		} else {
			f = ( d1 + LW_EPSILON ) / ( d1 - d2 );
			if ( f > 1.0f ) {
				f = 1.0f;
			}
			if ( f < leaveFrac ) {
				leaveFrac = f;
			}
		}
	}

	if ( !startOut ) {
		trace->startsolid = qtrue;
		if ( !getOut ) {
			trace->allsolid = qtrue;
			trace->fraction = 0.0f;
		}
		trace->contents = brush->contents;
		return;
	}

	if ( clip && enterFrac < leaveFrac && enterFrac > -1.0f && enterFrac < trace->fraction ) {
		trace->fraction = enterFrac < 0.0f ? 0.0f : enterFrac;
		VectorCopy( clip->normal, trace->plane.normal );
		trace->plane.dist = clip->dist;
		trace->plane.type = (byte)clip->type;
		trace->plane.signbits = (byte)clip->signbits;
		trace->surfaceFlags = brush->surfaceFlags;
		trace->contents = brush->contents;
		trace->entityNum = ENTITYNUM_WORLD;
	}
}

void QDECL LW_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
	const vec3_t end, int passEntityNum, int contentMask ) {
	static const vec3_t zero = { 0, 0, 0 };
	trace_t trace;
	int i;

	(void)passEntityNum;
	if ( !mins ) {
		mins = zero;
	}
	if ( !maxs ) {
		maxs = zero;
	}

	memset( &trace, 0, sizeof( trace ) );
	trace.fraction = 1.0f;
	trace.entityNum = ENTITYNUM_NONE;
	for ( i = 0; i < lwNumBrushes; i++ ) {
		if ( lwBrushes[i].contents & contentMask ) {
			LW_TraceBrush( &trace, &lwBrushes[i], start, end, mins, maxs );
			if ( trace.allsolid ) {
				break;
			}
		}
	}
	if ( trace.allsolid || trace.startsolid ) {
		trace.entityNum = ENTITYNUM_WORLD;
	}
	if ( trace.fraction == 1.0f ) {
		VectorCopy( end, trace.endpos );
	} else {
		for ( i = 0; i < 3; i++ ) {
			trace.endpos[i] = start[i] + trace.fraction * ( end[i] - start[i] );
		}
	}
	*results = trace;
}

int QDECL LW_PointContents( const vec3_t point, int passEntityNum ) {
	int contents = 0;
	int i, j;

	(void)passEntityNum;
	for ( i = 0; i < lwNumBrushes; i++ ) {
		const lwBrush_t *brush = &lwBrushes[i];

		for ( j = 0; j < brush->numPlanes; j++ ) {
			if ( DotProduct( point, brush->planes[j].normal ) > brush->planes[j].dist ) {
				break;
			}
		}
		if ( j == brush->numPlanes ) {
			contents |= brush->contents;
		}
	}
	return contents;
}
