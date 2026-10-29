// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
================
G_BounceMissile

Retail qagamex86 0x1005b350.  The hit time counts back from level.time by
the trace fraction of the frame, where Quake III counts forward from the
previous frame, and a half-bounce missile that comes to rest snaps to whole
units one unit above the impact, without Quake III's s.time stamp.
================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	const float	*normal = trace->plane.normal;
	vec3_t	velocity;
	double	scale;
	int		hitTime;
	int		i;

	// reflect the velocity on the trace plane
	hitTime = (int)( (double)level.time - (double)trace->fraction * ( level.time - level.previousTime ) );
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	scale = (float)( (double)normal[2] * velocity[2] + (double)normal[1] * velocity[1] + (double)normal[0] * velocity[0] ) * -2.0;
	for ( i = 0; i < 3; i++ ) {
		ent->s.pos.trDelta[i] = (float)( velocity[i] + normal[i] * scale );
	}

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		for ( i = 0; i < 3; i++ ) {
			ent->s.pos.trDelta[i] = (float)( ent->s.pos.trDelta[i] * 0.65 );
		}
		// check for stop
		if ( normal[2] > 0.2 && (float)sqrt( PM_Dot3( ent->s.pos.trDelta, ent->s.pos.trDelta ) ) < 40.0f ) {
			trace->endpos[0] = (int)trace->endpos[0];
			trace->endpos[1] = (int)trace->endpos[1];
			trace->endpos[2] = (int)(float)( trace->endpos[2] + 1.0 );
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	for ( i = 0; i < 3; i++ ) {
		ent->r.currentOrigin[i] = (float)( (double)normal[i] + ent->r.currentOrigin[i] );
	}
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact (retail qagamex86 0x1005b500)
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
		if ( G_RadiusDamage( ent->r.currentOrigin, ent, ent, NULL, NULL ) ) {
			G_StatsHit( g_entities[ent->r.ownerNum].client, ent->s.weapon );
		}
	}

	trap_LinkEntity( ent );
}


#ifdef MISSIONPACK
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer * 1000;	// retail: seconds

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->r.mins, -r, -r, -r );
	VectorSet( trigger->r.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else {
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW ) {
		return;
	}

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else {
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif

/*
================
G_MissileImpact

Retail qagamex86 0x1005bbe0.  A prox mine sticks before any impact damage,
the impact event precedes the impact damage, and the splash knows the
impact trace and the flight direction.  Not yet: retail's grapple impact
(it pulls a hooked player) and rocket damage through thin ceilings
(g_forceDmgThroughSurface, SURF 0x200000, radius damage 0x1004a3e0).
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	vec3_t			dir;
#ifdef MISSIONPACK
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
	int				i;
#endif
	other = &g_entities[trace->entityNum];

	// check for bounce; a bouncing nail stops bouncing after g_nailbounce walls
	if ( !other->takedamage && ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF | EF_NAIL_BOUNCE ) ) &&
		( !( ent->s.eFlags & EF_NAIL_BOUNCE ) || ent->bounceCount < g_nailbounce.integer ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		ent->bounceCount++;
		return;
	}

#ifdef MISSIONPACK
	if ( other->takedamage ) {
		if ( ent->s.weapon != WP_PROX_LAUNCHER ) {
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
#ifdef MISSIONPACK
	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		// retail centres the box 4 units out of the surface
		for ( i = 0; i < 3; i++ ) {
			ent->r.mins[i] = trace->plane.normal[i] * 4.0f - 4.0f;
			ent->r.maxs[i] = trace->plane.normal[i] * 4.0f + 4.0f;
		}
		trap_LinkEntity(ent);

		return;
	}
#endif

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	// the flight direction, for the rocket splash offset
	VectorCopy( ent->s.pos.trDelta, dir );
	PM_VectorNormalize( dir );

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	// impact damage, after the event in retail
	if ( ent->damage && other->takedamage ) {
		vec3_t	velocity;

		if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
			G_StatsHit( g_entities[ent->r.ownerNum].client, ent->s.weapon );
			hitClient = qtrue;
		}
		BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
		if ( (float)sqrt( PM_Dot3( velocity, velocity ) ) == 0.0f ) {
			velocity[2] = 1;	// stepped on a grenade
		}
		G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
			ent->s.origin, ent->damage,
			0, ent->methodOfDeath);
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent, other, trace, dir ) ) {
			if( !hitClient ) {
				G_StatsHit( g_entities[ent->r.ownerNum].client, ent->s.weapon );
			}
		}
	}

	trap_LinkEntity( ent );
}


/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
#ifdef MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
#endif
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}
#ifdef MISSIONPACK
	// if the prox mine wasn't yet outside the player body
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
#endif
	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================

/*
=================
G_MissilePrestep

Retail missiles start one server frame (1000 / sv_fps) into their flight,
where Quake III used a fixed 50 msec.
=================
*/
static int G_MissilePrestep( void ) {
	return sv_fps.integer > 0 ? (int)( 1000.0 / sv_fps.integer ) : 0;
}


/*
=================
G_MissileClipmask

Missiles pass through players under pmove_noPlayerClip.
=================
*/
static int G_MissileClipmask( void ) {
	return pmove_noPlayerClip.integer ? ( MASK_SHOT & ~CONTENTS_BODY ) : MASK_SHOT;
}


/*
=================
G_SpawnMissile

The missile set-up retail's grenade, rocket, plasma and BFG share (qagamex86
0x1005cb30): a +-0.01 box, the origin snapped to whole units, the prestep,
and dir normalized in place and scaled to speed.  Retail picks the
trajectory here from weapon_gravity_rl (unless g_guidedRocket), so the BFG
follows the rocket's gravity setting; grenades and plasma overwrite it.
=================
*/
static gentity_t *G_SpawnMissile( gentity_t *self, const vec3_t start, vec3_t dir, int speed ) {
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->s.eFlags = 0;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->clipmask = G_MissileClipmask();
	bolt->target_ent = NULL;

	VectorCopy( start, bolt->s.pos.trBase );
	SnapVector( bolt->s.pos.trBase );			// save net bandwidth
	VectorCopy( bolt->s.pos.trBase, bolt->r.currentOrigin );

	bolt->s.pos.trType = ( weapon_gravity_rl.integer && !g_guidedRocket.integer ) ? TR_GRAVITY : TR_LINEAR;
	bolt->s.pos.trTime = level.time - G_MissilePrestep();		// move a bit on the very first frame
	PM_VectorNormalize( dir );
	VectorScale( dir, (float)speed, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorSet( bolt->r.mins, -0.01f, -0.01f, -0.01f );
	VectorSet( bolt->r.maxs, 0.01f, 0.01f, 0.01f );

	return bolt;
}


/*
=================
G_AccelerateMissile

g_accelFactor_rl / _pg / _bfg other than 1 (retail qagamex86 0x1005cd70,
0x1005d020, 0x1005d150): every g_accelRate_* msec the velocity is scaled in
place, without rebasing the trajectory, and the missile no longer times out.
=================
*/
static void G_AccelerateMissile( gentity_t *ent ) {
	float	factor, rate;
	int		i;

	switch ( ent->s.weapon ) {
	case WP_PLASMAGUN:
		factor = g_accelFactor_pg.value;
		rate = g_accelRate_pg.value;
		break;
	case WP_BFG:
		factor = g_accelFactor_bfg.value;
		rate = g_accelRate_bfg.value;
		break;
	default:
		factor = g_accelFactor_rl.value;
		rate = g_accelRate_rl.value;
		break;
	}

	for ( i = 0; i < 3; i++ ) {
		ent->s.pos.trDelta[i] = (float)( (double)factor * ent->s.pos.trDelta[i] );
	}
	ent->nextthink = (int)( (double)level.time + rate );
}


/*
=================
G_GuideRocket

g_guidedRocket (retail qagamex86 0x1005cdc0): every 25 msec the rocket
restarts from where it is along its owner's view, at retail's speed factor
of 20 against the trajectory's original start time.
=================
*/
static void G_GuideRocket( gentity_t *ent ) {
	gentity_t	*owner = ent->parent;
	vec3_t		forward;

	ent->nextthink = level.time + 25;
	if ( !owner || !owner->client ) {
		return;
	}

	PM_AngleVectors( owner->client->ps.viewangles, forward, NULL, NULL );
	VectorCopy( forward, ent->movedir );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorScale( forward, 20.0f, ent->s.pos.trDelta );
}


/*
=================
G_MissileAcceleration

Set up the acceleration think when the weapon's g_accelFactor is not 1.
=================
*/
static void G_MissileAcceleration( gentity_t *bolt, float factor, float rate ) {
	if ( factor != 1.0f ) {
		bolt->nextthink = (int)( (double)level.time + rate );
		bolt->think = G_AccelerateMissile;
	}
}


/*
=================
fire_plasma

Retail qagamex86 0x1005d070
=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	bolt = G_SpawnMissile( self, start, dir, (int)g_velocity_pg.value );
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->s.otherEntityNum = self->s.number;
	bolt->damage = (int)g_damage_pg.value;
	bolt->splashDamage = (int)g_splashdamage_pg.value;
	bolt->splashRadius = (int)g_splashradius_pg.value;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->s.pos.trType = weapon_gravity_pg.integer ? TR_GRAVITY : TR_LINEAR;
	G_MissileAcceleration( bolt, g_accelFactor_pg.value, g_accelRate_pg.value );

	return bolt;
}

//=============================================================================


/*
=================
fire_grenade

Retail qagamex86 0x1005ccd0
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	bolt = G_SpawnMissile( self, start, dir, (int)g_velocity_gl.value );
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.otherEntityNum = self->s.number;
	bolt->damage = (int)g_damage_gl.value;
	bolt->splashDamage = (int)g_splashdamage_gl.value;
	bolt->splashRadius = (int)g_splashradius_gl.value;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.clientNum = self->s.clientNum;	// retail marks only grenades with their owner

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg

Retail qagamex86 0x1005d1a0
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	bolt = G_SpawnMissile( self, start, dir, (int)g_velocity_bfg.value );
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->s.weapon = WP_BFG;
	bolt->s.otherEntityNum = self->s.number;
	bolt->damage = (int)g_damage_bfg.value;
	bolt->splashDamage = (int)g_splashdamage_bfg.value;
	bolt->splashRadius = (int)g_splashradius_bfg.value;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	G_MissileAcceleration( bolt, g_accelFactor_bfg.value, g_accelRate_bfg.value );

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket

Retail qagamex86 0x1005cf20.  The splash damage stays 100 here; G_Damage
rebuilds rocket splash from g_splashdamage_rl.
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	bolt = G_SpawnMissile( self, start, dir, (int)g_velocity_rl.value );
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->s.otherEntityNum = self->s.number;
	bolt->damage = (int)g_damage_rl.value;
	bolt->splashDamage = 100;
	bolt->splashRadius = (int)g_splashradius_rl.value;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	if ( g_guidedRocket.integer ) {
		bolt->nextthink = level.time + G_MissilePrestep();
		bolt->think = G_GuideRocket;
	}
	G_MissileAcceleration( bolt, g_accelFactor_rl.value, g_accelRate_rl.value );

	return bolt;
}


/*
=================
fire_grapple

Retail qagamex86 0x1005d270: g_velocity_gh, the hook damage g_damage_gh,
and the common prestep.
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	PM_VectorNormalize( dir );

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = G_MissileClipmask();
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - G_MissilePrestep();		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number;
	hook->damage = (int)g_damage_gh.value;
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, g_velocity_gh.value, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	if ( self->client ) {
		self->client->hook = hook;
	}

	return hook;
}


#ifdef MISSIONPACK
/*
=================
fire_nail

Retail Weapon_Nailgun_Fire (qagamex86 0x1006ee20) fires g_nailcount of these.
The spread and speed come from g_nailspread and g_nailspeed, weapon_gravity_ng
drops the nails, and g_nailbouncepercentage of them bounce off walls up to
g_nailbounce times.  The arithmetic follows retail's x87 evaluation.
=================
*/
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;
	int			i;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 4500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;
	bolt->damage = (int)g_damage_ng.value;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = G_MissileClipmask();
	bolt->target_ent = NULL;

	bolt->s.pos.trType = weapon_gravity_ng.integer ? TR_GRAVITY : TR_LINEAR;
	if ( g_nailbounce.integer && rand() % 100 + 1 > 100 - g_nailbouncepercentage.integer ) {
		bolt->s.eFlags = EF_NAIL_BOUNCE;
	}
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );

	r = (float)( G_Random() * (float)M_PI * 2.0 );
	u = (float)( sin( r ) * G_CRandom() * g_nailspread.integer * 16.0 );
	r = (float)( cos( r ) * G_CRandom() * g_nailspread.integer * 16.0 );
	for ( i = 0; i < 3; i++ ) {
		end[i] = (float)( start[i] + forward[i] * ( 8192.0 * 16.0 ) );
		end[i] = (float)( end[i] + (double)right[i] * r );
		end[i] = (float)( end[i] + (double)up[i] * u );
		dir[i] = end[i] - start[i];
	}
	PM_VectorNormalize( dir );

	scale = (float)( G_Random() * 1800.0 + g_nailspeed.integer );
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	return bolt;
}


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	PM_VectorNormalize( dir );

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->s.otherEntityNum = self->s.number;
	bolt->parent = self;
	bolt->damage = (int)g_damage_pl.value;
	bolt->splashDamage = (int)g_splashdamage_pl.value;
	bolt->splashRadius = (int)g_splashradius_pl.value;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = G_MissileClipmask();
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client ? self->client->sess.sessionTeam : TEAM_FREE;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - G_MissilePrestep();		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}
#endif
