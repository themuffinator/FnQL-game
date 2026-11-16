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
// lockstep_digest.h -- architecture-independent digests of Pmove output.
//
// Gate G3c: the retail lockstep harness (pmove_lockstep.c, x86 Windows)
// digests the retail Pmove's output for every scenario frame into
// tests/lockstep/pmove_golden.txt, and the portable replay harness
// (pmove_replay.c) digests FnQL-game's Pmove on every enabled target and
// compares.  A frame is serialized field by field, each value in
// little-endian order at its own width, so the digest of the same values is
// the same on little- and big-endian hosts and for 32- and 64-bit pointers.
//

#ifndef LOCKSTEP_DIGEST_H
#define LOCKSTEP_DIGEST_H

#include <stdio.h>

#include "lockstep_scenarios.h"

// frames per golden checkpoint: a divergence is located to this many frames
#define LS_DIGEST_CHUNK		250

// the pmove_t results a frame is digested with, gathered from either Pmove
typedef struct {
	int		numtouch;
	int		touchents[MAXTOUCH];
	vec3_t	mins, maxs;
	int		watertype;
	int		waterlevel;
	float	xyspeed;
	float	stepUp;
	int		stepTime;
} lsPmOutputs_t;

typedef struct {
	unsigned long long	hash;
	unsigned long long	bytes;
} lsDigest_t;

void	LS_DigestInit( lsDigest_t *digest );
void	LS_DigestBytes( lsDigest_t *digest, const void *data, int length );
void	LS_DigestInt( lsDigest_t *digest, int value );
void	LS_DigestFloat( lsDigest_t *digest, float value );
// one Pmove frame: every named playerState_t field, then the pmove results
void	LS_DigestFrame( lsDigest_t *digest, const playerState_t *ps, const lsPmOutputs_t *out );
void	LS_PmOutputsFromPmove( const pmove_t *pm, lsPmOutputs_t *out );
// bytes of playerState_t that no lsPsFields entry names (padding or an
// unnamed field); the replay harness refuses to run when this is not zero
int		LS_UnnamedStateBytes( void );

// the sine trajectory grid both harnesses evaluate: index 0 upwards until it
// returns qfalse
qboolean LS_SineSample( int index, trajectory_t *trajectory, int *atTime, int *derivative );

/*
Golden file (tests/lockstep/pmove_golden.txt), one record per line:

  sine <evaluations> <digest>
  scenario <name> <frames> <digest per LS_DIGEST_CHUNK frames>...

Digests are 16 hex digits.  '#' starts a comment line.
*/
#define LS_MAX_CHUNKS		64
#define LS_MAX_GOLDEN		128

typedef struct {
	char				name[64];		// "sine" for the trajectory record
	int					frames;			// frames, or sine evaluations
	int					numChunks;
	unsigned long long	chunks[LS_MAX_CHUNKS];
} lsGoldenRecord_t;

// accumulates one record, a digest per LS_DIGEST_CHUNK frames
typedef struct {
	lsGoldenRecord_t	record;
	lsDigest_t			chunk;
	int					inChunk;
} lsRecorder_t;

void	LS_RecorderInit( lsRecorder_t *recorder, const char *name );
// the caller digests the frame into recorder->chunk, then calls this
void	LS_RecorderEndFrame( lsRecorder_t *recorder );
void	LS_RecorderFinish( lsRecorder_t *recorder );

void	LS_GoldenWrite( FILE *file, const lsGoldenRecord_t *record );
// reads up to `max` records; returns the count, or -1 when the file is unreadable
int		LS_GoldenRead( const char *path, lsGoldenRecord_t *records, int max );
const lsGoldenRecord_t *LS_GoldenFind( const lsGoldenRecord_t *records, int count, const char *name );
// prints the first differing checkpoint; returns qtrue when the records match
qboolean LS_GoldenCompare( const lsGoldenRecord_t *expected, const lsGoldenRecord_t *got );

#endif
