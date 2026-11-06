// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"




/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float		left, front, up;
	float		kick;
	int			health;
	int			attacker;
	float		scale;
	vec3_t		dir;
	vec3_t		angles;
	float		dist;
	float		yaw, pitch;
	const char	*info;

	// show the attacking player's head and name in corner
	// but only if client is valid

	//attacker = cg.predictedPlayerState.persistant[PERS_ATTACKER]; ?
	attacker = cg.snap->ps.persistant[PERS_ATTACKER]; 

	if ( (unsigned)attacker < MAX_CLIENTS && attacker != cg.snap->ps.clientNum ) {
		// dont do it when cg_draw2d = 0 ?
		info = CG_ConfigString( CS_PLAYERS + attacker );
		if ( *info ) {
			BG_CleanName( Info_ValueForKey( info, "n" ), cg.attackerName, sizeof( cg.attackerName ), "???" );
			cg.attackerClientNum = attacker;
			cg.attackerTime = cg.time;
		}
	}

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 ) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );
		VectorSubtract( vec3_origin, dir, dir );

		front = DotProduct (dir, cg.refdef.viewaxis[0] );
		left = DotProduct (dir, cg.refdef.viewaxis[1] );
		up = DotProduct (dir, cg.refdef.viewaxis[2] );

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength( dir );
		if ( dist < 0.1 ) {
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;
		
		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1 ) {
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if ( cg.damageX > 1.0 ) {
		cg.damageX = 1.0;
	}
	if ( cg.damageX < - 1.0 ) {
		cg.damageX = -1.0;
	}

	if ( cg.damageY > 1.0 ) {
		cg.damageY = 1.0;
	}
	if ( cg.damageY < - 1.0 ) {
		cg.damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if ( kick > 10 ) {
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}


/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// Retail preference order and queued primary, with server fallback.
	CG_SelectSpawnWeapon();

	cg.timeResidual = cg.snap->ps.commandTime + 1000;
}


/*
==============
CG_CheckPlayerstateEvents
==============
*/
extern int		eventStack;
extern int		eventParm2[ MAX_PREDICTED_EVENTS ];

static void CG_CheckPlayerstateEvents( const playerState_t *ps, const playerState_t *ops ) {
	int			i, n;
	int			event;
	centity_t	*cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, cent->lerpOrigin, -1 );
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	n = eventStack - MAX_PS_EVENTS;
	if ( n < 0 ) n  = 0;
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
			|| (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)]) ) {

			event = ps->events[ i & (MAX_PS_EVENTS-1) ];
			if ( event == EV_NONE ) // ignore empty events
				continue;
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];

			CG_EntityEvent( cent, cent->lerpOrigin, eventParm2[ n++ ] );

			cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS-1) ] = event;

			cg.eventSequence++;
		}
	}
}


/*
==================
CG_CheckLocalSounds
==================
*/
void CG_CheckLocalSounds( playerState_t *ps, playerState_t *ops ) {
	// health changes of more than -1 should make pain sounds (inherited)
	if ( ps->persistant[PERS_TEAM] == ops->persistant[PERS_TEAM] &&
		ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 && ps->stats[STAT_HEALTH] > 0 ) {
		CG_PainEvent( &cg.predictedPlayerEntity, ps->stats[STAT_HEALTH] );
	}

	// hit and kill beeps, medals, announcer, lead/time/frag warnings
	// follow retail 0x100435b0 (cg_localsounds.c)
	CG_QLCheckLocalSounds( ps, ops );
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops ) {
	qboolean respawn;

	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// the time-warning join guard (see CG_MatchClockWarnings)
	if ( ops->persistant[PERS_TEAM] == TEAM_SPECTATOR && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		cg.teamJoinTime = cg.time;
	}

	// respawning / map restart
	respawn = ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT];
	if ( respawn || cg.mapRestart ) {
		cg.mapRestart = qfalse;
		CG_Respawn();
	}

	if ( cg.snap->ps.pm_type != PM_INTERMISSION 
		&& ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		CG_CheckLocalSounds( ps, ops );
		CG_RecordCrosshairHit( ps, ops );	// retail 0x10043c2c
	}

	// check for going low on ammo
	CG_CheckAmmo();

	 // try to play potentially dropped events
	CG_PlayDroppedEvents( ps, ops );

	// run events
	CG_CheckPlayerstateEvents( ps, ops );

	// reset event stack
	eventStack = 0;

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight && !respawn ) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}
