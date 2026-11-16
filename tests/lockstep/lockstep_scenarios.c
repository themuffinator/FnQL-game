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
// lockstep_scenarios.c -- the pmove lockstep scenarios (lockstep_scenarios.h).
//

#include "lockstep_scenarios.h"

/*
==============================================================================

Player state field names, for reports

==============================================================================
*/


#define PSF( f, n, fl )	{ #f, (int)offsetof( playerState_t, f ), n, (int)sizeof( ((playerState_t *)0)->f ) / ( n ), fl }

const psField_t lsPsFields[] = {
	PSF( commandTime, 1, qfalse ), PSF( pm_type, 1, qfalse ), PSF( bobCycle, 1, qfalse ),
	PSF( pm_flags, 1, qfalse ), PSF( pm_time, 1, qfalse ), PSF( origin, 3, qtrue ),
	PSF( velocity, 3, qtrue ), PSF( weaponTime, 1, qfalse ), PSF( gravity, 1, qfalse ),
	PSF( speed, 1, qfalse ), PSF( delta_angles, 3, qfalse ), PSF( groundEntityNum, 1, qfalse ),
	PSF( legsTimer, 1, qfalse ), PSF( legsAnim, 1, qfalse ), PSF( torsoTimer, 1, qfalse ),
	PSF( torsoAnim, 1, qfalse ), PSF( movementDir, 1, qfalse ), PSF( grapplePoint, 3, qtrue ),
	PSF( eFlags, 1, qfalse ), PSF( eventSequence, 1, qfalse ), PSF( events, MAX_PS_EVENTS, qfalse ),
	PSF( eventParms, MAX_PS_EVENTS, qfalse ), PSF( externalEvent, 1, qfalse ),
	PSF( externalEventParm, 1, qfalse ), PSF( clientNum, 1, qfalse ), PSF( location, 1, qfalse ),
	PSF( weapon, 1, qfalse ), PSF( weaponPrimary, 1, qfalse ), PSF( weaponstate, 1, qfalse ),
	PSF( fov, 1, qfalse ), PSF( viewangles, 3, qtrue ), PSF( viewheight, 1, qfalse ),
	PSF( damageEvent, 1, qfalse ), PSF( damageYaw, 1, qfalse ), PSF( damagePitch, 1, qfalse ),
	PSF( damageCount, 1, qfalse ), PSF( stats, MAX_STATS, qfalse ),
	PSF( persistant, MAX_PERSISTANT, qfalse ), PSF( powerups, MAX_POWERUPS, qfalse ),
	PSF( ammo, MAX_WEAPONS, qfalse ), PSF( generic1, 1, qfalse ), PSF( loopSound, 1, qfalse ),
	PSF( jumppad_ent, 1, qfalse ), PSF( jumpTime, 1, qfalse ), PSF( doubleJumped, 1, qfalse ),
	PSF( crouchTime, 1, qfalse ), PSF( crouchSlideTime, 1, qfalse ), PSF( forwardmove, 1, qfalse ),
	PSF( rightmove, 1, qfalse ), PSF( upmove, 1, qfalse ), PSF( commandMirrorPad, 1, qfalse ),
	PSF( ping, 1, qfalse ), PSF( pmove_framecount, 1, qfalse ), PSF( jumppad_frame, 1, qfalse ),
	PSF( entityEventSequence, 1, qfalse ), PSF( externalEventTime, 1, qfalse ),
	PSF( armorTier, 1, qfalse ),
};

const psField_t *LS_FieldAt( int offset, int *element ) {
	int i;

	for ( i = 0; i < ARRAY_LEN( lsPsFields ); i++ ) {
		const psField_t *f = &lsPsFields[i];

		if ( offset >= f->offset && offset < f->offset + f->count * f->size ) {
			*element = ( offset - f->offset ) / f->size;
			return f;
		}
	}
	return NULL;
}

/*
==============================================================================

Scenarios

==============================================================================
*/


static unsigned LS_Rand( unsigned *rng ) {
	*rng = *rng * 1664525u + 1013904223u;
	return *rng >> 8;
}

int LS_RandRange( unsigned *rng, int lo, int hi ) {
	return lo + (int)( LS_Rand( rng ) % (unsigned)( hi - lo + 1 ) );
}

static void D_Run( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = frame < sc->frames * 2 / 3 ? 127 : 0;
}

static void D_RunJump( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = 127;
	in->upmove = ( frame / 40 ) % 2 ? 127 : 0;
}

static void D_StrafeJump( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	const int phase = ( frame / 60 ) % 2;

	(void)rng;
	in->forwardmove = 127;
	in->rightmove = phase ? 127 : -127;
	in->upmove = frame > 20 ? 127 : 0;
	in->yaw = sc->yaw + ( phase ? -0.9f : 0.9f ) * (float)( frame % 60 ) + ( phase ? 27.0f : -27.0f );
}

static void D_Crouch( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = 127;
	in->upmove = frame < 60 ? 0 : frame < 64 ? 127 : -127;
	if ( frame > 220 ) {
		in->upmove = 0;
	}
}

static void D_Back( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = frame < 120 ? -127 : 0;
	in->upmove = frame == 60 ? 127 : 0;
}

static void D_Swim( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = 127;
	in->upmove = frame < 120 ? 127 : frame < 200 ? -127 : 127;
	in->pitch = frame < 120 ? -10.0f : 10.0f;
}

static void D_Weapons( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	static const int cycle[] = { WP_MACHINEGUN, WP_SHOTGUN, WP_ROCKET_LAUNCHER, WP_GAUNTLET, WP_RAILGUN };

	(void)rng;
	in->yaw = sc->yaw;
	in->weapon = cycle[( frame / 50 ) % ARRAY_LEN( cycle )];
	in->buttons = ( frame % 50 ) > 25 ? BUTTON_ATTACK : 0;
	in->forwardmove = ( frame / 100 ) % 2 ? 127 : 0;
}


static unsigned LS_Hash( unsigned x ) {
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	return x ^ ( x >> 16 );
}

static int lsFlightItem, lsMedkitItem;

static void D_Ladder( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = frame < 260 ? 127 : -127;
	in->upmove = frame < 30 ? 0 : frame < 150 ? 127 : frame < 220 ? -127 : 0;
	in->pitch = frame < 150 ? -30.0f : 30.0f;
}

static void D_DoubleJump( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	const int t = frame % 90;

	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = 127;
	in->upmove = ( t < 4 || ( t >= 30 && t < 34 ) ) ? 127 : 0;
}

static void D_Flight( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->forwardmove = frame > 40 ? 127 : 0;
	in->buttons = ( frame >= 20 && frame < 260 ) || frame >= 320 ? BUTTON_USE_HOLDABLE : 0;
	in->upmove = frame > 200 && frame < 230 ? -127 : 0;
}

static void D_UseItem( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->buttons = ( frame % 40 ) < 10 ? BUTTON_USE_HOLDABLE : 0;
}

static void D_Gauntlet( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->weapon = WP_GAUNTLET;
	in->buttons = frame > 40 && ( frame % 200 ) < 150 ? BUTTON_ATTACK : 0;
	in->gauntletHit = ( frame / 30 ) % 2;
}

static void D_Chaingun( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->weapon = WP_CHAINGUN;
	in->buttons = frame > 40 && ( frame % 300 ) < 200 ? BUTTON_ATTACK : 0;
}

static void D_Holster( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw;
	in->weapon = ( frame / 60 ) % 2 ? WP_NUM_WEAPONS : WP_MACHINEGUN;
	in->buttons = ( frame % 60 ) > 30 ? BUTTON_ATTACK : 0;
}

static void D_Buttons( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	static const int buttons[] = { BUTTON_TALK, BUTTON_GESTURE, BUTTON_GETFLAG, BUTTON_GUARDBASE,
		BUTTON_PATROL, BUTTON_FOLLOWME, BUTTON_AFFIRMATIVE, BUTTON_NEGATIVE, BUTTON_WALKING };

	(void)rng;
	in->yaw = sc->yaw + frame * 0.5f;
	in->forwardmove = 127;
	in->rightmove = ( frame / 50 ) % 2 ? 64 : 0;
	in->buttons = ( frame % 50 ) < 30 ? buttons[( frame / 50 ) % ARRAY_LEN( buttons )] : 0;
	in->noFootsteps = ( frame / 100 ) % 2;
}

static void D_Grapple( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	(void)rng;
	in->yaw = sc->yaw + frame * 0.3f;
	in->forwardmove = 64;
	in->grappleSlow = ( frame / 70 ) % 2;
}

static void P_Respawned( int frame, playerState_t *ps ) {
	if ( frame < 0 ) {
		ps->pm_flags |= PMF_RESPAWNED;
	}
}

static void P_Flight( int frame, playerState_t *ps ) {
	if ( frame < 0 ) {
		ps->stats[STAT_HOLDABLE_ITEM] = lsFlightItem;
		ps->stats[STAT_PLAYER_ITEM_THRUST] = 1200;
		ps->stats[STAT_PLAYER_ITEM_TIME_MAX] = 3000;
		ps->stats[STAT_PLAYER_ITEM_TIME] = 3000;
		ps->stats[STAT_PLAYER_ITEM_RECHARGE] = 400;
	}
	// the game latches the use button for the flight holdable
	if ( frame == 20 || frame == 320 ) {
		ps->pm_flags |= PMF_USE_ITEM_HELD;
	}
}

static void P_FlightOnce( int frame, playerState_t *ps ) {
	P_Flight( frame, ps );
	if ( frame < 0 ) {
		ps->stats[STAT_PLAYER_ITEM_RECHARGE] = 0;
		ps->stats[STAT_PLAYER_ITEM_TIME] = 1500;
	}
}

static void P_Medkit( int frame, playerState_t *ps ) {
	if ( frame < 0 || frame % 80 == 0 ) {
		ps->stats[STAT_HOLDABLE_ITEM] = lsMedkitItem;
		ps->stats[STAT_MAX_HEALTH] = 100;
		ps->stats[STAT_HEALTH] = ( frame / 80 ) % 2 ? 125 : 60;
	}
}

static void P_Haste( int frame, playerState_t *ps ) {
	ps->powerups[PW_HASTE] = ( frame / 120 ) % 2 ? 0 : 999999;
	ps->stats[STAT_PERSISTANT_POWERUP] = ( frame / 240 ) % 2 ? PP_SCOUT : PP_NONE;
}

// the battle suit, on and off, while the swimmer's head goes under and out
// (retail 0x10031854 holds back EV_WATER_CLEAR under the suit)
static void P_Battlesuit( int frame, playerState_t *ps ) {
	ps->powerups[PW_BATTLESUIT] = ( frame / 60 ) % 2 ? 0 : 999999;
}

static void P_Chaingun( int frame, playerState_t *ps ) {
	if ( frame < 0 ) {
		ps->stats[STAT_WEAPONS] |= 1 << WP_CHAINGUN;
		ps->ammo[WP_CHAINGUN] = 300;
		ps->weapon = WP_CHAINGUN;
	}
}

static void P_Holster( int frame, playerState_t *ps ) {
	if ( frame < 0 ) {
		ps->stats[STAT_WEAPONS] |= 1 << WP_NUM_WEAPONS;
	}
}

static void P_Lockout( int frame, playerState_t *ps ) {
	if ( ( frame / 50 ) % 2 ) {
		ps->pm_flags |= PMF_ATTACK_LOCKOUT;
	} else {
		ps->pm_flags &= ~PMF_ATTACK_LOCKOUT;
	}
	if ( frame % 75 == 0 ) {
		ps->pm_flags |= PMF_WEAPON_RESET;
	}
}

static void P_Invulnerable( int frame, playerState_t *ps ) {
	ps->powerups[PW_INVULNERABILITY] = ( frame / 60 ) % 3 ? 999999 : 0;
	if ( ( frame / 30 ) % 2 ) {
		ps->pm_flags |= PMF_INVULEXPAND;
	}
}

static void P_Grapple( int frame, playerState_t *ps ) {
	if ( frame < 0 ) {
		VectorSet( ps->grapplePoint, 400, 3000, 300 );
	}
	if ( frame % 150 < 100 ) {
		ps->pm_flags |= PMF_GRAPPLE_PULL;
	} else {
		ps->pm_flags &= ~PMF_GRAPPLE_PULL;
	}
}

// the game toggles the per-player movement options at spawn; flip them now and then
static void P_FuzzFlags( int frame, playerState_t *ps ) {
	static const int bits[] = { PMF_AIR_CONTROL, PMF_CROUCH_SLIDE, PMF_DOUBLE_JUMP, PMF_REQUIRE_JUMP_RELEASE };
	unsigned h = LS_Hash( (unsigned)frame * 0x9e3779b9u + 77u );

	if ( frame >= 0 && ( h & 255 ) < 3 ) {
		ps->pm_flags ^= bits[( h >> 8 ) % ARRAY_LEN( bits )];
	}
	if ( frame >= 0 && ( h & 1023 ) == 512 ) {
		ps->powerups[PW_HASTE] = ps->powerups[PW_HASTE] ? 0 : 999999;
	}
}

static void D_FuzzGame( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in );

static void D_Fuzz( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	static lsInput_t held;
	static const int weapons[] = { WP_GAUNTLET, WP_MACHINEGUN, WP_SHOTGUN, WP_ROCKET_LAUNCHER, WP_RAILGUN };

	if ( frame == 0 ) {
		memset( &held, 0, sizeof( held ) );
		held.yaw = sc->yaw;
		held.weapon = WP_MACHINEGUN;
	}
	if ( LS_RandRange( rng, 0, 9 ) == 0 ) {
		held.forwardmove = LS_RandRange( rng, -1, 1 ) * 127;
	}
	if ( LS_RandRange( rng, 0, 9 ) == 0 ) {
		held.rightmove = LS_RandRange( rng, -1, 1 ) * 127;
	}
	if ( LS_RandRange( rng, 0, 7 ) == 0 ) {
		held.upmove = LS_RandRange( rng, -1, 1 ) * 127;
	}
	if ( LS_RandRange( rng, 0, 29 ) == 0 ) {
		held.buttons ^= BUTTON_ATTACK;
	}
	if ( LS_RandRange( rng, 0, 59 ) == 0 ) {
		held.buttons ^= BUTTON_WALKING;
	}
	if ( LS_RandRange( rng, 0, 99 ) == 0 ) {
		held.weapon = weapons[LS_RandRange( rng, 0, ARRAY_LEN( weapons ) - 1 )];
	}
	held.yaw += (float)LS_RandRange( rng, -40, 40 ) * 0.1f;
	held.pitch = (float)LS_RandRange( rng, -30, 30 );
	*in = held;
	in->msec = 0;
}

static void D_FuzzGame( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in ) {
	D_Fuzz( sc, frame, rng, in );
	in->gauntletHit = LS_RandRange( rng, 0, 1 );
	if ( LS_RandRange( rng, 0, 39 ) == 0 ) {
		in->buttons ^= BUTTON_USE_HOLDABLE;
	}
}

const scenario_t lsScenarios[] = {
	{ "run", { 0, 0, 25 }, 0, 400, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "run-16ms", { 0, 0, 25 }, 0, 200, PM_NORMAL, 0, NULL, D_Run, 16 },
	{ "run-3ms", { 0, 0, 25 }, 45, 900, PM_NORMAL, 0, NULL, D_Run, 3 },
	{ "run-jump", { -2048, 1600, 25 }, 0, 600, PM_NORMAL, 0, NULL, D_RunJump, 8 },
	{ "strafe-jump", { -3000, 2600, 25 }, 0, 1200, PM_NORMAL, 0, NULL, D_StrafeJump, 8 },
	{ "strafe-jump-pql", { -3000, 2600, 25 }, 0, 1200, PM_NORMAL, PMF_AIR_CONTROL, NULL, D_StrafeJump, 8 },
	{ "crouch-slide", { -3000, -2000, 25 }, 0, 300, PM_NORMAL, 0, NULL, D_Crouch, 8 },
	{ "back-jump", { 2000, 2000, 25 }, 0, 200, PM_NORMAL, 0, NULL, D_Back, 8 },
	{ "stairs", { 128, 0, 25 }, 0, 260, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "stairs-jump", { 128, 0, 25 }, 0, 260, PM_NORMAL, 0, NULL, D_RunJump, 8 },
	{ "stairs-18", { 128, 0, 25 }, 0, 260, PM_NORMAL, 0, "\\pmove_StepHeight\\18", D_Run, 8 },
	{ "ramp", { -600, 384, 25 }, 0, 300, PM_NORMAL, 0, NULL, D_RunJump, 8 },
	{ "ramp-jump", { -600, 384, 25 }, 0, 300, PM_NORMAL, 0, "\\pmove_RampJump\\1", D_RunJump, 8 },
	{ "steep", { -600, 768, 25 }, 0, 200, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "tunnel", { 64, -448, 25 }, 180, 300, PM_NORMAL, 0, NULL, D_Crouch, 8 },
	{ "water", { -1100, 0, 25 }, 180, 400, PM_NORMAL, 0, NULL, D_Swim, 8 },
	{ "fall", { -192, 832, 281 }, 90, 300, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "slick", { 1100, 1000, 25 }, 0, 200, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "playerclip", { 1500, 0, 25 }, 0, 120, PM_NORMAL, 0, NULL, D_Run, 8 },
	{ "weapons", { 0, 2000, 25 }, 0, 600, PM_NORMAL, 0, NULL, D_Weapons, 8 },
	{ "knockback", { 0, -2000, 25 }, 0, 200, PM_NORMAL, PMF_TIME_KNOCKBACK, NULL, D_Run, 8, { 400, 200, 500 }, 300 },
	{ "dead", { 0, -2500, 60 }, 0, 120, PM_DEAD, 0, NULL, D_Run, 8, { 200, 0, 100 } },
	// waist deep in the pool, head out: the dead view height drops the head
	// sample under water, and retail sends no head events for the dead
	{ "dead-water", { -1100, 0, 80 }, 0, 160, PM_DEAD, 0, NULL, D_Run, 8, { 100, 0, 0 } },
	{ "water-battlesuit", { -1100, 0, 25 }, 180, 400, PM_NORMAL, 0, NULL, D_Swim, 8, { 0, 0, 0 }, 0, P_Battlesuit },
	{ "spectator", { 0, -3000, 60 }, 0, 200, PM_SPECTATOR, 0, NULL, D_Swim, 8 },
	{ "noclip", { 0, -3000, 60 }, 0, 120, PM_NOCLIP, 0, NULL, D_Swim, 8 },
	{ "fuzz-1", { 0, 0, 25 }, 0, 3000, PM_NORMAL, 0, NULL, D_Fuzz, 0 },
	{ "fuzz-2", { 300, 0, 25 }, 90, 3000, PM_NORMAL, 0, NULL, D_Fuzz, 0 },
	{ "fuzz-3", { -1200, 0, 25 }, 0, 3000, PM_NORMAL, 0, NULL, D_Fuzz, 0 },
	{ "fuzz-pql", { -400, 400, 25 }, 0, 3000, PM_NORMAL, PMF_AIR_CONTROL, NULL, D_Fuzz, 0 },
	{ "fuzz-settings", { 0, 0, 25 }, 0, 3000, PM_NORMAL, 0,
		"\\pmove_JumpVelocity\\300\\pmove_ChainJump\\3\\pmove_RampJump\\1\\pmove_StepJump\\0"
		"\\pmove_AutoHop\\0\\pmove_CrouchSlideTime\\1200\\pmove_WalkFriction\\8\\pmove_StepHeight\\20"
		"\\pmove_AirStepFriction\\0.1\\pmove_WeaponRaiseTime\\150", D_Fuzz, 0 },
	{ "ladder", { 2370, 0, 25 }, 0, 320, PM_NORMAL, 0, NULL, D_Ladder, 8 },
	{ "ladder-pql", { 2370, 0, 25 }, 0, 320, PM_NORMAL, PMF_AIR_CONTROL, NULL, D_Ladder, 8 },
	{ "double-jump", { -3000, 1200, 25 }, 0, 600, PM_NORMAL, PMF_DOUBLE_JUMP, NULL, D_DoubleJump, 8 },
	{ "crouch-slide-on", { -3000, -2000, 25 }, 0, 600, PM_NORMAL, PMF_CROUCH_SLIDE, NULL, D_Crouch, 8 },
	{ "crouch-slide-pql", { -3000, -2000, 25 }, 0, 600, PM_NORMAL, PMF_CROUCH_SLIDE | PMF_AIR_CONTROL,
		NULL, D_Crouch, 8 },
	{ "jump-release", { -2048, 1600, 25 }, 0, 400, PM_NORMAL, PMF_REQUIRE_JUMP_RELEASE, NULL, D_RunJump, 8 },
	{ "no-autohop", { -2048, 1600, 25 }, 0, 400, PM_NORMAL, 0, "\\pmove_AutoHop\\0", D_RunJump, 8 },
	{ "respawned", { 0, 0, 25 }, 0, 200, PM_NORMAL, 0, NULL, D_Weapons, 8, { 0, 0, 0 }, 0, P_Respawned },
	{ "flight", { -2000, -1000, 25 }, 0, 420, PM_NORMAL, 0, NULL, D_Flight, 8, { 0, 0, 0 }, 0, P_Flight },
	{ "flight-once", { -2000, -1000, 25 }, 0, 420, PM_NORMAL, 0, NULL, D_Flight, 8, { 0, 0, 0 }, 0, P_FlightOnce },
	{ "medkit", { 0, 2000, 25 }, 0, 320, PM_NORMAL, 0, NULL, D_UseItem, 8, { 0, 0, 0 }, 0, P_Medkit },
	{ "gauntlet", { 0, 2000, 25 }, 0, 400, PM_NORMAL, 0, NULL, D_Gauntlet, 8 },
	{ "chaingun", { 0, 2000, 25 }, 0, 900, PM_NORMAL, 0, NULL, D_Chaingun, 8, { 0, 0, 0 }, 0, P_Chaingun },
	{ "haste-scout", { 0, 2000, 25 }, 0, 960, PM_NORMAL, 0, NULL, D_Weapons, 8, { 0, 0, 0 }, 0, P_Haste },
	{ "holster", { 0, 2000, 25 }, 0, 360, PM_NORMAL, 0, NULL, D_Holster, 8, { 0, 0, 0 }, 0, P_Holster },
	{ "lockout", { 0, 2000, 25 }, 0, 400, PM_NORMAL, 0, NULL, D_Weapons, 8, { 0, 0, 0 }, 0, P_Lockout },
	{ "invulnerable", { 0, 2600, 25 }, 0, 360, PM_NORMAL, 0, NULL, D_Run, 8, { 0, 0, 0 }, 0, P_Invulnerable },
	{ "buttons", { 2000, -2000, 25 }, 0, 500, PM_NORMAL, 0, NULL, D_Buttons, 8 },
	{ "grapple", { 0, 2600, 25 }, 0, 450, PM_NORMAL, 0, NULL, D_Grapple, 8, { 0, 0, 0 }, 0, P_Grapple },
	{ "grapple-gh0", { 0, 2600, 25 }, 0, 200, PM_NORMAL, 0, "\\pmove_velocity_gh\\0", D_Grapple, 8,
		{ 0, 0, 0 }, 0, P_Grapple },
	{ "frozen", { 0, -2000, 25 }, 0, 200, PM_FREEZE, 0, NULL, D_Run, 8, { 300, 0, 200 } },
	{ "intermission", { 0, -2000, 60 }, 0, 120, PM_INTERMISSION, 0, NULL, D_Run, 8, { 100, 0, 0 } },
	{ "tutorial", { 0, -2000, 60 }, 0, 120, PM_TUTORIAL, 0, NULL, D_Run, 8, { 100, 0, 0 } },
	{ "fuzz-game", { 0, 0, 25 }, 0, 6000, PM_NORMAL, 0, NULL, D_FuzzGame, 0, { 0, 0, 0 }, 0, P_FuzzFlags },
	{ "fuzz-game-2", { -1200, 0, 25 }, 90, 6000, PM_NORMAL, PMF_AIR_CONTROL, NULL, D_FuzzGame, 0,
		{ 0, 0, 0 }, 0, P_FuzzFlags },
};

const int lsNumPsFields = ARRAY_LEN( lsPsFields );
const int lsNumScenarios = ARRAY_LEN( lsScenarios );

void LS_InitScenarios( void ) {
	int i;

	for ( i = 1; i < bg_numItems; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == HI_FLIGHT ) {
			lsFlightItem = i;
		}
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == HI_MEDKIT ) {
			lsMedkitItem = i;
		}
	}
}

unsigned LS_ScenarioSeed( const scenario_t *sc ) {
	unsigned rng = 0x9e3779b9u;
	const char *name;

	for ( name = sc->name; *name; name++ ) {
		rng = rng * 31u + (byte)*name;
	}
	return rng;
}

void LS_InitState( const scenario_t *sc, playerState_t *ps ) {
	memset( ps, 0, sizeof( *ps ) );
	ps->commandTime = 1000;
	ps->pm_type = sc->pmType;
	ps->pm_flags = sc->pmFlags;
	ps->pm_time = sc->pmTime;
	VectorCopy( sc->origin, ps->origin );
	VectorCopy( sc->velocity, ps->velocity );
	ps->gravity = 800;
	ps->speed = 320;
	ps->groundEntityNum = ENTITYNUM_NONE;
	ps->clientNum = 0;
	ps->viewheight = DEFAULT_VIEWHEIGHT;
	ps->viewangles[YAW] = sc->yaw;
	ps->stats[STAT_HEALTH] = sc->pmType == PM_DEAD ? 0 : 100;
	ps->stats[STAT_WEAPONS] = ( 1 << WP_GAUNTLET ) | ( 1 << WP_MACHINEGUN ) | ( 1 << WP_SHOTGUN )
		| ( 1 << WP_ROCKET_LAUNCHER ) | ( 1 << WP_RAILGUN );
	ps->ammo[WP_GAUNTLET] = -1;
	ps->ammo[WP_MACHINEGUN] = 100;
	ps->ammo[WP_SHOTGUN] = 10;
	ps->ammo[WP_ROCKET_LAUNCHER] = 10;
	ps->ammo[WP_RAILGUN] = 10;
	ps->weapon = WP_MACHINEGUN;
	ps->weaponPrimary = WP_MACHINEGUN;
	ps->weaponstate = WEAPON_READY;
	ps->fov = 90;
}

void LS_BuildCommand( const lsInput_t *in, int serverTime, usercmd_t *cmd ) {
	memset( cmd, 0, sizeof( *cmd ) );
	cmd->serverTime = serverTime;
	cmd->angles[YAW] = ANGLE2SHORT( in->yaw );
	cmd->angles[PITCH] = ANGLE2SHORT( in->pitch );
	cmd->forwardmove = (signed char)in->forwardmove;
	cmd->rightmove = (signed char)in->rightmove;
	cmd->upmove = (signed char)in->upmove;
	cmd->buttons = in->buttons;
	cmd->weapon = (byte)in->weapon;
	cmd->weaponPrimary = (byte)in->weapon;
	cmd->fov = 90;
}

void LS_NextFrame( const scenario_t *sc, int frame, unsigned *rng, lsInput_t *in, int *serverTime,
		usercmd_t *cmd ) {
	memset( in, 0, sizeof( *in ) );
	sc->driver( sc, frame, rng, in );
	if ( sc->msec ) {
		in->msec = sc->msec;
	} else if ( LS_RandRange( rng, 0, 49 ) == 0 ) {
		in->msec = LS_RandRange( rng, 40, 120 );	// a hitch, split into chunks
	} else {
		in->msec = LS_RandRange( rng, 1, 30 );
	}
	*serverTime += in->msec;
	LS_BuildCommand( in, *serverTime, cmd );
}
