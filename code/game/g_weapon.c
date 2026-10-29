// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

#define SHOTGUN_PELLETS		20		// the Quake Live pattern

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;



/*
===============
CalcMuzzlePoint

Retail qagamex86 0x1006c8d0: the view directions, and one muzzle point for
every weapon trace and launch: the entity origin at eye height, 5 units
along the view (3 when ducked).  Quake III's 14 units and baseq3a's
separate eye-level trace origin are gone.
===============
*/
static void CalcMuzzlePoint( const gentity_t *ent ) {
	const double	offset = ( ent->client->ps.pm_flags & PMF_DUCKED ) ? 3.0 : 5.0;
	int				i;

	PM_AngleVectors( ent->client->ps.viewangles, forward, right, up );

	VectorCopy( ent->s.pos.trBase, muzzle );
	muzzle[2] = (float)( (double)ent->client->ps.viewheight + muzzle[2] );
	for ( i = 0; i < 3; i++ ) {
		muzzle[i] = (float)( forward[i] * offset + muzzle[i] );
	}
}


/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


/*
======================================================================

GAUNTLET

======================================================================
*/

/*
===============
Weapon_Gauntlet

Retail qagamex86 0x1006ca70: the hit lands on the target CheckGauntletAttack
found this frame.  Retail passes the offset between the two origins as the
damage point.
===============
*/
void Weapon_Gauntlet( gentity_t *ent ) {
	gentity_t	*target = ent->target_ent;
	gentity_t	*tent;
	vec3_t		delta;

	if ( !target ) {
		return;
	}

	VectorSubtract( ent->s.pos.trBase, target->s.pos.trBase, delta );
	if ( target->takedamage && target->client ) {
		tent = G_TempEntity( target->s.pos.trBase, EV_MISSILE_HIT );
		tent->s.otherEntityNum = target->s.number;
		tent->s.eventParm = DirToByte( delta );
		tent->s.weapon = ent->s.weapon;
	}

	G_Damage( target, ent, ent, forward, delta, (int)( (double)s_quadFactor * g_damage_g.value ), 0, MOD_GAUNTLET );
}

/*
===============
CheckGauntletAttack

Retail qagamex86 0x1006cb30: a 43 unit trace from the muzzle.  It only
finds the target; Weapon_Gauntlet hits it when the weapon fires.
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	int			i;

	CalcMuzzlePoint( ent );
	for ( i = 0; i < 3; i++ ) {
		end[i] = (float)( forward[i] * 43.0 + muzzle[i] );
	}

	trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number,
		pmove_noPlayerClip.integer ? CONTENTS_SOLID : MASK_SHOT );
	if ( !( tr.surfaceFlags & SURF_NOIMPACT ) && g_entities[ tr.entityNum ].takedamage ) {
		ent->target_ent = &g_entities[ tr.entityNum ];
		return qtrue;
	}

	ent->target_ent = NULL;
	return qfalse;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.  Retail (qagamex86 0x1006cc20) keeps Quake III's form,
which truncates negative coordinates toward zero first.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}

// Quake Live bullet spreads (retail FireWeapon, qagamex86 0x1006f290)
#define MACHINEGUN_SPREAD	150		// scaled by g_ironsights_mg while crouched
#define HMG_SPREAD			350
#define CHAINGUN_SPREAD_MIN	700		// widening to twice this as the barrel spins up

/*
================
Bullet_Fire

Retail qagamex86 0x1006cc60: the Quake III spread, a capsule trace with
g_playerCylinders, and at most one bounce off an invulnerability sphere.
================
*/
static void Bullet_Fire( gentity_t *ent, float spread, int damage, int mod ) {
	trace_t		tr;
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t		impactpoint, bouncedir;
#endif
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

	const int	mask = pmove_noPlayerClip.integer ? CONTENTS_SOLID : MASK_SHOT;

	damage = (int)( (float)damage * s_quadFactor );

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;

	VectorMA( muzzle, ( 8192.0 * 16.0 ), forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	passent = ent->s.number;
	for ( i = 0; i < 2; i++ ) {

		// unlagged
		G_DoTimeShiftFor( ent );

		if ( g_playerCylinders.integer ) {
			trap_TraceCapsule( &tr, muzzle, NULL, NULL, end, passent, mask );
		} else {
			trap_Trace( &tr, muzzle, NULL, NULL, end, passent, mask );
		}

		// unlagged
		G_UndoTimeShiftFor( ent );

		if ( tr.surfaceFlags & SURF_NOIMPACT )
			return;

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;

			// unlagged
			tent->s.clientNum = ent->s.clientNum;

			if( LogAccuracyHit( traceEnt, ent ) ) {
				G_StatsHit( ent->client, ent->s.weapon );
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage ) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
#endif
				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, mod );
#ifdef MISSIONPACK
			}
#endif
		}
		break;
	}
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire( gentity_t *ent ) {
	gentity_t *m;

	m = fire_bfg( ent, muzzle, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}


/*
================
G_ProjectPointOnPlane

ProjectPointOnPlane in the retail arithmetic (qagamex86 0x100703f0); the
retail cgame rebuilds the shotgun pattern with it.
================
*/
static void G_ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal ) {
	const float invDenom = (float)( 1.0 / (float)PM_Dot3( normal, normal ) );
	const float d = (float)( PM_Dot3( normal, p ) * invDenom );
	int i;

	for ( i = 0; i < 3; i++ ) {
		const float n = normal[i] * invDenom;

		dst[i] = (float)( p[i] - (double)d * n );
	}
}


/*
================
ShotgunPellet

One pellet of the Quake Live shotgun (retail qagamex86 0x1006d140): the
inner ring does g_damage_sg, the outer rings g_damage_sg_outer, less
g_damage_sg_falloff for each g_range_sg_falloff past the first.  Returns the
damage done to an enemy player, which the pattern totals.
================
*/
static int ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *ent, qboolean inner ) {
	trace_t		tr;
	int			damage, i, passent, steps, range;
	gentity_t	*traceEnt;
	vec3_t		impactpoint, bouncedir;
	vec3_t		tr_start, tr_end, travel;
	const int	mask = pmove_noPlayerClip.integer ? CONTENTS_SOLID : MASK_SHOT;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );

	for ( i = 0; i < 10; i++ ) {
		if ( g_playerCylinders.integer ) {
			trap_TraceCapsule( &tr, tr_start, NULL, NULL, tr_end, passent, mask );
		} else {
			trap_Trace( &tr, tr_start, NULL, NULL, tr_end, passent, mask );
		}
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return 0;
		}
		if ( !traceEnt->takedamage ) {
			return 0;
		}

		damage = (int)( ( inner ? g_damage_sg.value : g_damage_sg_outer.value ) * s_quadFactor );
		if ( g_damage_sg_falloff.integer ) {
			VectorSubtract( tr.endpos, muzzle, travel );
			steps = (int)( (float)sqrt( PM_Dot3( travel, travel ) ) - g_range_sg_falloff.value );
			range = (int)g_range_sg_falloff.value;
			while ( steps > 0 && range ) {
				steps -= range;
				damage -= (int)g_damage_sg_falloff.value;
			}
		}
		if ( damage < 1 ) {
			damage = 1;
		}

		if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
			if ( G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir ) ) {
				G_BounceProjectile( tr_start, impactpoint, bouncedir, tr_end );
				VectorCopy( impactpoint, tr_start );
				// the player can hit him/herself with the bounced pellet
				passent = ENTITYNUM_NONE;
			} else {
				VectorCopy( tr.endpos, tr_start );
				passent = traceEnt->s.number;
			}
			continue;
		}

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN );
		if ( traceEnt->takedamage && traceEnt != ent && traceEnt->client && ent->client
			&& !OnSameTeam( traceEnt, ent ) && !traceEnt->client->ps.powerups[PW_FROZEN] ) {
			return damage;
		}
		return 0;
	}
	return 0;
}


/*
================
ShotgunPattern

The Quake Live pattern is a fixed ring of 20 pellets around the aim
(retail qagamex86 0x1006d460): six at 4000 units off the aim at 131072
units, six at 8000 and eight at 12000.  The angles are computed exactly as
retail computes them, since the retail cgame draws the same pattern.
================
*/
static void ShotgunPattern( const vec3_t origin, const vec3_t target, gentity_t *ent ) {
	vec3_t		forward, right, up, axis, end;
	double		base[3];
	double		angle;
	float		fangle, across, along;
	int			i, j, radius, damage, total;
	qboolean	hitClient = qfalse;
	gclient_t	*client = ent->client;

	VectorSubtract( target, origin, forward );
	PM_VectorNormalize( forward );

	// the axis least aligned with the aim
	j = 0;
	angle = 1.0;
	if ( fabs( forward[0] ) < angle ) {
		angle = fabs( forward[0] );
	}
	if ( fabs( forward[1] ) < angle ) {
		angle = fabs( forward[1] );
		j = 1;
	}
	if ( fabs( forward[2] ) < angle ) {
		j = 2;
	}
	VectorClear( axis );
	axis[j] = 1.0f;
	G_ProjectPointOnPlane( right, axis, forward );
	PM_VectorNormalize( right );
	PM_CrossProduct( forward, right, up );

	for ( j = 0; j < 3; j++ ) {
		base[j] = forward[j] * 131072.0;
	}

	// unlagged
	G_DoTimeShiftFor( ent );

	total = 0;
	for ( i = 0; i < SHOTGUN_PELLETS; i++ ) {
		if ( i < 6 ) {
			angle = ( i - 20 ) * 1.0471975803375244;
			radius = 4000;
		} else if ( i < 12 ) {
			angle = 1.0471975803375244 * i + 30.0;
			radius = 8000;
		} else {
			angle = i * 0.7853981852531433;
			radius = 12000;
		}
		fangle = (float)angle;
		across = (float)( cos( fangle ) * radius );
		along = (float)( sin( fangle ) * radius );
		for ( j = 0; j < 3; j++ ) {
			end[j] = (float)( origin[j] + base[j] );
			end[j] = (float)( (double)right[j] * across + end[j] );
			end[j] = (float)( (double)up[j] * along + end[j] );
		}

		damage = ShotgunPellet( origin, end, ent, i < 6 );
		total += damage;
		if ( damage ) {
			// one overall hit for the blast, one weapon hit for each pellet
			if ( !hitClient ) {
				hitClient = qtrue;
				client->accuracy_hits++;
			}
			client->stats.weaponHits[WP_SHOTGUN]++;
		}
	}

	// unlagged
	G_UndoTimeShiftFor( ent );

	// one hit sound for the blast
	client->ps.generic1 = ( client->ps.generic1 & 0x3f ) | ( G_HitTier( total ) << 6 );

	// and one damage plum per player hit
	if ( g_damagePlums.integer ) {
		for ( i = 0; i < level.maxclients; i++ ) {
			gentity_t *victim = &g_entities[i];

			if ( victim->inuse && victim->client && client->shotgunDamage[i] ) {
				vec3_t at;

				VectorCopy( victim->client->ps.origin, at );
				at[2] += 32;
				G_DamagePlum( ent, at, client->shotgunDamage[i], WP_SHOTGUN );
				client->shotgunDamage[i] = 0;
			}
		}
	}
}


/*
================
weapon_supershotgun_fire

The blast event carries the start and the end point the retail cgame
rebuilds the pattern from (retail qagamex86 0x1006d8d0).
================
*/
static void weapon_supershotgun_fire( gentity_t *ent ) {
	gentity_t		*tent;
	int				i;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	for ( i = 0; i < 3; i++ ) {
		tent->s.origin2[i] = (float)(int)(float)( (double)forward[i] * 4096.0 + muzzle[i] );
	}
	tent->s.eventParm = rand() & 255;
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	PM_VectorNormalize( forward );

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
	m->damageFactor = (int)s_quadFactor;	// G_Damage rebuilds the splash from it
}


/*
======================================================================

PLASMA GUN

======================================================================
*/

void Weapon_Plasmagun_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define	MAX_RAIL_HITS	4

/*
=================
weapon_railgun_fire

Retail qagamex86 0x1006dae0: g_damage_rg, a capsule trace with
g_playerCylinders, an impact event on everything the slug damages, and the
headshot bonus g_headShotDamage_rg on a trace the engine marks with
contents 0x400 (retail keeps the bonus and MOD_RAILGUN_HEADSHOT for any
target the slug passes through after that).  g_railJump pushes the shooter
away from a wall within 120 units: along the normalized end point of the
shot, as retail computes it, plus 20 up.
=================
*/
void weapon_railgun_fire( gentity_t *ent ) {
	vec3_t		end, wallEnd, push;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	gclient_t	*client;
	int			damage;
	int			mod;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	const int	mask = pmove_noPlayerClip.integer ? CONTENTS_SOLID : MASK_SHOT;

	damage = (int)( (double)s_quadFactor * g_damage_rg.value );
	mod = MOD_RAILGUN;

	for ( i = 0; i < 3; i++ ) {
		end[i] = (float)( forward[i] * 8192.0 + muzzle[i] );
	}

	if ( g_railJump.integer && ent->client ) {
		for ( i = 0; i < 3; i++ ) {
			wallEnd[i] = (float)( muzzle[i] + forward[i] * 120.0 );
		}
		trap_Trace( &trace, muzzle, NULL, NULL, wallEnd, ent->s.number, CONTENTS_SOLID );
		if ( trace.fraction != 1.0f ) {
			VectorCopy( end, push );
			PM_VectorNormalize( push );
			for ( i = 0; i < 3; i++ ) {
				ent->client->ps.velocity[i] = (float)( ent->client->ps.velocity[i] - (double)g_railJump.integer * push[i] );
			}
			ent->client->ps.velocity[2] = (float)( ent->client->ps.velocity[2] + 20.0 );
		}
	}

	// unlagged
	G_DoTimeShiftFor( ent );

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		if ( g_playerCylinders.integer ) {
			trap_TraceCapsule( &trace, muzzle, NULL, NULL, end, passent, mask );
		} else {
			trap_Trace( &trace, muzzle, NULL, NULL, end, passent, mask );
		}
		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if ( traceEnt->takedamage ) {
			client = traceEnt->client;
#ifdef MISSIONPACK
			if ( client && client->invulnerabilityTime > level.time ) {
				if ( G_InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					SnapVectorTowards( trace.endpos, muzzle );
					// send railgun beam effect
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					// set player number for custom colors on the railtrail
					tent->s.clientNum = ent->s.clientNum;
					VectorCopy( muzzle, tent->s.origin2 );
					// move origin a bit to come closer to the drawn gun muzzle
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;	// don't make the explosion at the end
					//
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			} else
#endif
			{
				// retail's hit count: enemies (or anyone but spectators
				// outside team games) that are alive and not frozen
				if ( traceEnt != ent && client && ent->client
					&& ( g_gametype.integer >= GT_TEAM ? client->sess.sessionTeam != ent->client->sess.sessionTeam
						: ( client->sess.sessionTeam != TEAM_SPECTATOR
							|| client->sess.sessionTeam != ent->client->sess.sessionTeam ) )
					&& !( G_FreezeTagActive() && client->ps.powerups[PW_FROZEN] )
					&& traceEnt->health > 0 ) {
					hits++;
				}
				tent = G_TempEntity( trace.endpos, EV_MISSILE_HIT );
				tent->s.otherEntityNum = traceEnt->s.number;
				tent->s.weapon = ent->s.weapon;
				if ( trace.contents & 0x400 ) {
					damage += g_headShotDamage_rg.integer;
					mod = MOD_RAILGUN_HEADSHOT;
				}
				G_Damage( traceEnt, ent, ent, forward, trace.endpos, damage, 0, mod );
			}
		}
		if ( trace.contents & CONTENTS_SOLID ) {
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

	// unlagged
	G_UndoTimeShiftFor( ent );

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	for ( i = 0; i < 3; i++ ) {
		tent->s.origin2[i] = (float)( right[i] * 4.0 + tent->s.origin2[i] );
		tent->s.origin2[i] = tent->s.origin2[i] - up[i];
	}

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if ( hits == 0 ) {
		// complete miss
		ent->client->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->client->accurateCount += hits;
		if ( ent->client->accurateCount >= 2 ) {
			ent->client->accurateCount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
			// add the sprite over the player's head
			ent->client->ps.eFlags &= ~EF_AWARDS;
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
		G_StatsHit( ent->client, WP_RAILGUN );
	}
}


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
	if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);

	ent->client->fireHeld = qtrue;
}


void Weapon_HookFree (gentity_t *ent)
{
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}


void Weapon_HookThink (gentity_t *ent)
{
	if (ent->enemy) {
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		SnapVectorTowards( v, oldorigin );	// save net bandwidth

		G_SetOrigin( ent, v );
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}


/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void Weapon_LightningFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end, delta;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	gentity_t	*traceEnt, *tent;
	int			damage, excess, j, i, passent;

	const int	mask = pmove_noPlayerClip.integer ? CONTENTS_SOLID : MASK_SHOT;

	passent = ent->s.number;

	// retail: the first trace and at most one bounce off an invulnerability sphere
	for ( i = 0; i < 2; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			end[j] = (float)( forward[j] * (double)LIGHTNING_RANGE + muzzle[j] );
		}

		// unlagged
		G_DoTimeShiftFor( ent );

		if ( g_playerCylinders.integer ) {
			trap_TraceCapsule( &tr, muzzle, vec3_origin, vec3_origin, end, passent, mask );
		} else {
			trap_Trace( &tr, muzzle, NULL, NULL, end, passent, mask );
		}

		// unlagged
		G_UndoTimeShiftFor( ent );

#ifdef MISSIONPACK
		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if (i) {
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			//
			tent = G_TempEntity( muzzle, EV_LIGHTNINGBOLT );
			VectorCopy( tr.endpos, end );
			SnapVector( end );
			VectorCopy( end, tent->s.origin2 );
		}
#endif
		if ( tr.entityNum == ENTITYNUM_NONE ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if ( traceEnt->takedamage ) {
			// g_damage_lg_falloff per g_range_lg_falloff past the first g_range_lg_falloff units
			damage = g_damage_lg.integer;
			if ( g_damage_lg_falloff.integer ) {
				VectorSubtract( tr.endpos, muzzle, delta );
				excess = (int)( (double)(float)sqrt( PM_Dot3( delta, delta ) ) - g_range_lg_falloff.integer );
				if ( excess > 0 && g_range_lg_falloff.integer > 0 ) {
					do {
						excess -= g_range_lg_falloff.integer;
						damage -= g_damage_lg_falloff.integer;
					} while ( excess > 0 );
				}
				if ( damage < 1 ) {
					damage = 1;
				}
			}
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					VectorSubtract( end, impactpoint, forward );
					PM_VectorNormalize(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
#endif
			G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				(int)( (double)damage * s_quadFactor ), 0, MOD_LIGHTNING );
		}

		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
			if( LogAccuracyHit( traceEnt, ent ) ) {
				G_StatsHit( ent->client, ent->s.weapon );
			}
		} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}

		break;
	}
}

#ifdef MISSIONPACK
/*
======================================================================

NAILGUN

======================================================================
*/

// retail qagamex86 0x1006ee20: g_nailcount nails per shot
void Weapon_Nailgun_Fire (gentity_t *ent) {
	gentity_t	*m;
	int			count;

	for( count = 0; count < g_nailcount.integer; count++ ) {
		m = fire_nail (ent, muzzle, forward, right, up );
		m->damage = (int)( (float)m->damage * s_quadFactor );
		m->splashDamage = (int)( (float)m->splashDamage * s_quadFactor );
	}
}


/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void weapon_proxlauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	PM_VectorNormalize( forward );

	m = fire_prox (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

#endif

//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	// retail qagamex86 0x1006f240: no health test, but frozen players do not count
	if ( !target->takedamage || target == attacker ) {
		return qfalse;
	}
	if ( !target->client || !attacker->client ) {
		return qfalse;
	}
	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}
	if ( G_FreezeTagActive() && target->client->ps.powerups[PW_FROZEN] ) {
		return qfalse;
	}
	return qtrue;
}


/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	gclient_t *client = ent->client;

	// quad, or the doubler's one and a half
	if ( client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = (float)g_quadDamageFactor.integer;
	} else if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_DOUBLER ) {
		s_quadFactor = 1.5f;
	} else {
		s_quadFactor = 1.0f;
	}

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked;
	// the weapon's own count takes every shotgun pellet and nail (retail qagamex86 0x1006f2e4)
	if ( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET && ent->s.weapon != WP_NUM_WEAPONS ) {
		if ( ent->s.weapon == WP_SHOTGUN ) {
			client->accuracy_shots++;
			client->stats.weaponShots[WP_SHOTGUN] += SHOTGUN_PELLETS;
		} else if ( ent->s.weapon == WP_NAILGUN ) {
			client->accuracy_shots += g_nailcount.integer;
			client->stats.weaponShots[WP_NAILGUN] += g_nailcount.integer;
		} else {
			client->accuracy_shots++;
			client->stats.weaponShots[ent->s.weapon]++;
		}
	}

	// set aiming directions and the muzzle
	CalcMuzzlePoint( ent );

	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire( ent );
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		break;
	case WP_MACHINEGUN:
		Bullet_Fire( ent, (float)(int)( ( ( client->ps.pm_flags & PMF_DUCKED ) ? g_ironsights_mg.value : 1.0f )
			* (double)MACHINEGUN_SPREAD ), (int)g_damage_mg.value, MOD_MACHINEGUN );
		break;
	case WP_HEAVY_MACHINEGUN:
		Bullet_Fire( ent, HMG_SPREAD, (int)g_damage_hmg.value, MOD_HMG );
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire( ent );
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire( ent );
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire( ent );
		break;
	case WP_RAILGUN:
		weapon_railgun_fire( ent );
		break;
	case WP_BFG:
		BFG_Fire( ent );
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		Weapon_Nailgun_Fire( ent );
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire( ent );
		break;
	case WP_CHAINGUN: {
		// the spread widens as the barrel spins up
		const float spin = (float)( 1.0 - ( 1000.0 - client->ps.stats[STAT_CHAINGUN_SPINUP] ) / 1000.0 );
		float spread = (float)(int)( spin * (double)CHAINGUN_SPREAD_MIN + CHAINGUN_SPREAD_MIN );

		if ( spread < 0 ) {
			spread = 0;
		} else if ( spread > 2 * CHAINGUN_SPREAD_MIN ) {
			spread = 2 * CHAINGUN_SPREAD_MIN;
		}
		Bullet_Fire( ent, spread, (int)g_damage_cg.value, MOD_CHAINGUN );
		break;
	}
#endif
	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
}


#ifdef MISSIONPACK

/*
===============
KamikazeRadiusDamage
===============
*/
static void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (!ent->takedamage) {
			continue;
		}

		// dont hit things we have already hit
		if( ent->kamikazeTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		// dont hit things we have already hit
		if( ent->kamikazeShockTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			//
			dir[2] = 0;
			VectorNormalize(dir);
			if ( ent->client ) {
				ent->client->ps.velocity[0] = dir[0] * push;
				ent->client->ps.velocity[1] = dir[1] * push;
				ent->client->ps.velocity[2] = 100;
			}
			ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void KamikazeDamage( gentity_t *self ) {
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if (self->count >= KAMI_SHOCKWAVE_STARTTIME) {
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400,	(int) (float) t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME) );
	}
	//
	if (self->count >= KAMI_EXPLODE_STARTTIME) {
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage( self->s.pos.trBase, self->activator, 400,	(int) (float) t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME) );
	}

	// either cycle or kill self
	if( self->count >= KAMI_SHOCKWAVE_ENDTIME ) {
		G_FreeEntity( self );
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for (i = 0; i < level.maxclients; i++)
	{
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE) {
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void G_StartKamikaze( gentity_t *ent ) {
	gentity_t	*explosion;
	gentity_t	*te;
	vec3_t		snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if ( ent->client ) {
		VectorCopy( ent->s.pos.trBase, snapped );
	}
	else {
		VectorCopy( ent->activator->s.pos.trBase, snapped );
	}
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( explosion, snapped );

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	trap_LinkEntity( explosion );

	if (ent->client) {
		//
		explosion->activator = ent;
		//
		ent->s.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		G_Damage( ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE );
	}
	else {
		if ( !strcmp(ent->activator->classname, "bodyque") ) {
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		}
		else {
			explosion->activator = ent->activator;
		}
	}

	// play global sound at all clients
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}
#endif
