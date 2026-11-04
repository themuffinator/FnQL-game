/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "g_local.h"

static void G_FlagTravelSound( gentity_t *flag, const char *sound ) {
	gentity_t *event = G_TempEntity( flag->r.currentOrigin, EV_GENERAL_SOUND );
	event->s.eventParm = G_SoundIndex( sound );
}

static void G_PushFlag( gentity_t *flag, const gentity_t *pad ) {
	// Retail qagamex86 0x1005108d: each flag throttles the sound, but the
	// trajectory is refreshed on every overlapping frame, one frame back.
	if ( flag->fly_sound_debounce_time < level.time ) {
		flag->fly_sound_debounce_time = level.time + 1500;
		G_FlagTravelSound( flag, "sound/world/jumppad.wav" );
	}
	VectorCopy( flag->r.currentOrigin, flag->s.pos.trBase );
	VectorCopy( pad->s.origin2, flag->s.pos.trDelta );
	flag->s.pos.trTime = level.time - level.msec;
}

static qboolean G_TeleportFlag( gentity_t *flag, const gentity_t *portal ) {
	gentity_t *destination = G_PickTarget( portal->target );
	vec3_t forward;
	int axis;

	if ( !destination ) {
		G_Printf( "Couldn't find teleporter destination\n" );
		return qfalse;
	}

	// Retail 0x1005114b: use the destination's actual origin and angles.
	// Flags leave at 400 units/sec, with an extra 96 vertical units/sec;
	// round the multiply before the vertical addition, then truncate to ints.
	G_FlagTravelSound( flag, "sound/world/teleout.ogg" );
	VectorCopy( destination->s.origin, flag->r.currentOrigin );
	VectorCopy( destination->s.origin, flag->s.pos.trBase );
	PM_AngleVectors( destination->s.angles, forward, NULL, NULL );
	for ( axis = 0; axis < 3; axis++ ) {
		float speed = (float)( (double)forward[axis] * 400.0 );
		if ( axis == 2 ) {
			speed = (float)( (double)speed + 96.0 );
		}
		flag->s.pos.trDelta[axis] = (float)(int)speed;
	}
	flag->s.pos.trTime = level.time - level.msec;
	G_FlagTravelSound( flag, "sound/world/telein.ogg" );
	return qtrue;
}

void G_RunFlagPhysics( gentity_t *flag ) {
	int nearby[MAX_GENTITIES], count, i, axis;
	vec3_t mins, maxs;
	gentity_t *trigger;

	// Retail 0x10050f80 uses a 30-unit cube even though the flag's pickup
	// box is taller. This is an area query, without a brush-contact trace.
	for ( axis = 0; axis < 3; axis++ ) {
		mins[axis] = (float)( (double)flag->r.currentOrigin[axis] - 15.0 );
		maxs[axis] = (float)( (double)flag->r.currentOrigin[axis] + 15.0 );
	}
	count = trap_EntitiesInBox( mins, maxs, nearby, MAX_GENTITIES );
	for ( i = 0; i < count && i < MAX_GENTITIES && flag->inuse; i++ ) {
		if ( nearby[i] < 0 || nearby[i] >= MAX_GENTITIES ) {
			continue;
		}
		trigger = &g_entities[nearby[i]];
		if ( trigger == flag || !trigger->inuse || !trigger->classname ) {
			continue;
		}
		if ( !strcmp( trigger->classname, "trigger_push" ) ) {
			G_PushFlag( flag, trigger );
		} else if ( !strcmp( trigger->classname, "trigger_teleport" ) ) {
			if ( !G_TeleportFlag( flag, trigger ) ) {
				return;
			}
		} else if ( !strcmp( trigger->classname, "trigger_capturezone" ) && trigger->touch ) {
			trigger->touch( trigger, flag, NULL );
		}
	}
}
