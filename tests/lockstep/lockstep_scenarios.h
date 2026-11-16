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
// lockstep_scenarios.h -- the pmove lockstep scenarios and their inputs.
//
// The scripted and pseudo-random command streams, the initial player states
// and the game-side state changes are shared by the retail lockstep harness
// (pmove_lockstep.c, x86 Windows) and the portable replay harness
// (pmove_replay.c, every enabled target), so both drive Pmove with exactly
// the same inputs.
//

#ifndef LOCKSTEP_SCENARIOS_H
#define LOCKSTEP_SCENARIOS_H

#include "../../code/game/q_shared.h"
#include "../../code/game/bg_public.h"

typedef struct {
	const char	*name;
	int			offset;
	int			count;
	int			size;
	qboolean	isFloat;
} psField_t;

extern const psField_t	lsPsFields[];
extern const int		lsNumPsFields;
const psField_t			*LS_FieldAt( int offset, int *element );

typedef struct {
	int		msec;
	int		forwardmove, rightmove, upmove;
	int		buttons;
	int		weapon;
	float	yaw, pitch;			// view angles in degrees
	int		gauntletHit;		// pmove inputs the game sets
	int		noFootsteps;
	int		grappleSlow;
} lsInput_t;

typedef struct scenario_s scenario_t;
typedef void ( *lsDriver_t )( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in );
// game-side state changes, applied to both player states before a frame
// (frame -1: once after the initial state)
typedef void ( *lsPrep_t )( int frame, playerState_t *ps );

struct scenario_s {
	const char	*name;
	vec3_t		origin;
	float		yaw;
	int			frames;
	int			pmType;
	int			pmFlags;
	const char	*settings;			// pmove_* infostring, NULL for retail defaults
	lsDriver_t	driver;
	int			msec;				// 0 = pseudo-random per frame
	vec3_t		velocity;
	int			pmTime;
	lsPrep_t	prep;
};

extern const scenario_t	lsScenarios[];
extern const int		lsNumScenarios;

int		LS_RandRange( unsigned *rng, int lo, int hi );
// finds the holdable items the scenarios hand out; call once before running
void	LS_InitScenarios( void );
unsigned LS_ScenarioSeed( const scenario_t *sc );
void	LS_InitState( const scenario_t *sc, playerState_t *ps );
// the next frame's input and command: the driver's input and the frame time
// (fixed, or pseudo-random with occasional hitches that Pmove splits)
void	LS_NextFrame( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in, int *serverTime,
			usercmd_t *cmd );
void	LS_BuildCommand( const lsInput_t *in, int serverTime, usercmd_t *cmd );

#endif
