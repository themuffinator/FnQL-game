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
// pmove_lockstep.c -- gate G3: FnQL-game's Pmove against the retail one.
//
// The retail qagamex86.dll (Quake Live 1069, Jun 3 2016; the runner checks
// its SHA-256) is loaded into this 32-bit process and its Pmove is called
// directly.  Both Pmoves run the same scripted and pseudo-random command
// streams against the same brush world (lockstep_world.c), and after every
// command the full playerState and the pmove outputs are compared bit for
// bit.  After a comparison ours is resynchronised to the retail state, so a
// divergence is counted once, in the frame that produced it.
//
// Retail evidence used here (qagamex86.dll, image base 0x10000000):
//   Pmove        0x100323a0, takes the pmove_t pointer in ESI and splits the
//                command into chunks of at most 66 msec (no pmove_fixed).
//   pmove_t      ps +0x00, cmd +0x04, tracemask +0x20, noFootsteps +0x28,
//                gauntletHit +0x2c, numtouch +0x30,
//                touchents +0x34, mins +0xb4, maxs +0xc0, watertype +0xcc,
//                waterlevel +0xd0, xyspeed +0xd4, stepUp +0xd8,
//                stepTime +0xdc, trace +0xe0, pointcontents +0xe4,
//                grappleSlow +0xe8.
//   settings     globals filled from the pmove_* cvars by 0x100549e0; see
//                retailSettings[] below.
//

#ifndef _WIN32
#error "the pmove lockstep harness loads the retail Windows module"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../code/game/q_shared.h"
#include "../../code/game/bg_public.h"
#include "lockstep_world.h"

#define RETAIL_IMAGE_BASE		0x10000000u
#define RETAIL_PMOVE			0x100323a0u
#define RETAIL_PS_SIZE			0x250		// sizeof( playerState_t ) in the retail modules
#define RETAIL_PM_SIZE			0x100

#define RPM_PS					0x00
#define RPM_CMD					0x04
#define RPM_TRACEMASK			0x20
#define RPM_NOFOOTSTEPS			0x28
#define RPM_GAUNTLETHIT			0x2c
#define RPM_NUMTOUCH			0x30
#define RPM_TOUCHENTS			0x34
#define RPM_MINS				0xb4
#define RPM_MAXS				0xc0
#define RPM_WATERTYPE			0xcc
#define RPM_WATERLEVEL			0xd0
#define RPM_XYSPEED				0xd4
#define RPM_STEPUP				0xd8
#define RPM_STEPTIME			0xdc
#define RPM_TRACE				0xe0
#define RPM_POINTCONTENTS		0xe4
#define RPM_GRAPPLESLOW			0xe8

typedef enum { RS_FLOAT, RS_INT, RS_BOOL, RS_FLOAT_MIN } retailSettingType_t;

typedef struct {
	const char			*cvar;
	unsigned			address;
	retailSettingType_t	type;
	const char			*defaultValue;	// the retail cvar default
} retailSetting_t;

// qagamex86 0x100549e0 copies these cvars into the globals Pmove reads; the
// globals' initial values in the image are not all the cvar defaults, so
// every scenario writes all of them as the cvar registration would
static const retailSetting_t retailSettings[] = {
	{ "pmove_AirAccel", 0x1008fe3c, RS_FLOAT, "1.0f" },
	{ "pmove_AirStepFriction", 0x1008fe40, RS_FLOAT, "0.03f" },
	{ "pmove_AirStopAccel", 0x1008fe44, RS_FLOAT, "1.0f" },
	{ "pmove_CircleStrafeFriction", 0x1008fe4c, RS_FLOAT, "6.0f" },
	{ "pmove_CrouchSlideFriction", 0x1008fe50, RS_FLOAT, "0.5" },
	{ "pmove_CrouchSlideTime", 0x1008fe54, RS_INT, "2000" },
	{ "pmove_StrafeAccel", 0x1008fe68, RS_FLOAT, "1.0f" },
	{ "pmove_WalkAccel", 0x1008fe6c, RS_FLOAT, "10.0f" },
	{ "pmove_WalkFriction", 0x1008fe70, RS_FLOAT, "6.0f" },
	{ "pmove_WishSpeed", 0x1008fe74, RS_FLOAT, "400.0f" },
	{ "pmove_StepHeight", 0x1008fe78, RS_FLOAT, "22.0f" },
	{ "pmove_ChainJumpVelocity", 0x1008fe7c, RS_FLOAT, "110.0f" },
	{ "pmove_StepJumpVelocity", 0x1008fe80, RS_FLOAT, "48.0f" },
	{ "pmove_JumpVelocity", 0x1008fe84, RS_FLOAT, "275.0f" },
	{ "pmove_JumpVelocityMax", 0x1008fe88, RS_FLOAT, "700.0f" },
	{ "pmove_JumpTimeDeltaMin", 0x1008fe8c, RS_FLOAT, "100.0f" },
	{ "pmove_JumpVelocityScaleAdd", 0x1008fe90, RS_FLOAT, "0.4f" },
	{ "pmove_JumpVelocityTimeThreshold", 0x1008fe94, RS_FLOAT_MIN, "500.0f" },
	{ "pmove_JumpVelocityTimeThresholdOffset", 0x1008fe98, RS_FLOAT_MIN, "0.6f" },
	{ "pmove_WaterSwimScale", 0x1008fe9c, RS_FLOAT, "0.6f" },
	{ "pmove_WaterWadeScale", 0x1008fea0, RS_FLOAT, "0.8f" },
	{ "pmove_WeaponDropTime", 0x1008fea4, RS_INT, "200" },
	{ "pmove_WeaponRaiseTime", 0x1008fea8, RS_INT, "200" },
	{ "pmove_AirSteps", 0x1008feac, RS_BOOL, "1" },
	{ "pmove_ChainJump", 0x1008feb0, RS_INT, "1" },
	{ "pmove_CrouchStepJump", 0x1008feb4, RS_BOOL, "1" },
	{ "pmove_AutoHop", 0x1008feb8, RS_BOOL, "1" },
	{ "pmove_BunnyHop", 0x1008febc, RS_BOOL, "1" },
	{ "pmove_velocity_gh", 0x1008fec0, RS_FLOAT_MIN, "800" },
	{ "pmove_RampJumpScale", 0x1008fec4, RS_FLOAT, "1.0f" },
	{ "pmove_RampJump", 0x104b1398, RS_BOOL, "0" },
	{ "pmove_StepJump", 0x104b139c, RS_BOOL, "1" },
	{ "pmove_noPlayerClip", 0x104b13a0, RS_BOOL, "0" },
	// per-weapon reload times: the first field of a 48-byte record per weapon
	{ "weapon_reload_gauntlet", 0x1008ff40, RS_INT, "400" },
	{ "weapon_reload_mg", 0x1008ff70, RS_INT, "100" },
	{ "weapon_reload_sg", 0x1008ffa0, RS_INT, "1000" },
	{ "weapon_reload_gl", 0x1008ffd0, RS_INT, "800" },
	{ "weapon_reload_rl", 0x10090000, RS_INT, "800" },
	{ "weapon_reload_lg", 0x10090030, RS_INT, "50" },
	{ "weapon_reload_rg", 0x10090060, RS_INT, "1500" },
	{ "weapon_reload_pg", 0x10090090, RS_INT, "100" },
	{ "weapon_reload_bfg", 0x100900c0, RS_INT, "300" },
	{ "weapon_reload_gh", 0x100900f0, RS_INT, "100" },
	{ "weapon_reload_ng", 0x10090120, RS_INT, "1000" },
	{ "weapon_reload_prox", 0x10090150, RS_INT, "800" },
	{ "weapon_reload_cg", 0x10090180, RS_INT, "50" },
	{ "weapon_reload_hmg", 0x100901b0, RS_INT, "75" },
};

static byte	*retailBase;
static void	*retailPmove;

/*
==============================================================================

Symbols the shared bg code expects from its module

==============================================================================
*/

void QDECL Com_Printf( const char *fmt, ... ) {
	(void)fmt;
}

void QDECL Com_Error( int level, const char *fmt, ... ) {
	va_list ap;

	(void)level;
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	fputc( '\n', stderr );
	exit( 2 );
}

void trap_SnapVector( float *v ) {
	v[0] = (float)floor( v[0] + 0.5 );
	v[1] = (float)floor( v[1] + 0.5 );
	v[2] = (float)floor( v[2] + 0.5 );
}

static int QDECL LS_ImportStub( void ) {
	return 0;
}

/*
==============================================================================

Retail module

==============================================================================
*/

static byte *LS_RetailAddress( unsigned address ) {
	return retailBase + ( address - RETAIL_IMAGE_BASE );
}

static qboolean LS_LoadRetail( const char *module, const char *installDir ) {
	typedef void ( QDECL *dllEntry_t )( void **exports, const void *imports, int *apiVersion );
	static void *imports[256];
	static void *exports[64];
	dllEntry_t dllEntry;
	HMODULE handle;
	int apiVersion = 0;
	int i;

	// the retail module links MSVCR100/MSVCP100, which ship in the install
	SetDllDirectoryA( installDir );
	handle = LoadLibraryA( module );
	if ( !handle ) {
		fprintf( stderr, "cannot load %s (error %lu)\n", module, GetLastError() );
		return qfalse;
	}
	dllEntry = (dllEntry_t)GetProcAddress( handle, "dllEntry" );
	if ( !dllEntry ) {
		fprintf( stderr, "%s exports no dllEntry\n", module );
		return qfalse;
	}
	for ( i = 0; i < ARRAY_LEN( imports ); i++ ) {
		imports[i] = (void *)LS_ImportStub;
	}
	dllEntry( exports, imports, &apiVersion );

	retailBase = (byte *)handle;
	retailPmove = LS_RetailAddress( RETAIL_PMOVE );
	return qtrue;
}

// retail Pmove takes its argument in ESI
static void LS_RetailPmove( void *pm ) {
	void *entry = retailPmove;

	__asm {
		mov esi, pm
		call entry
	}
}

// writes the retail settings globals the way the cvar filler would
static void LS_ApplyRetailSettings( const char *info ) {
	int i;

	for ( i = 0; i < ARRAY_LEN( retailSettings ); i++ ) {
		const retailSetting_t *s = &retailSettings[i];
		byte *dest = LS_RetailAddress( s->address );
		const char *value = info ? Info_ValueForKey( info, s->cvar ) : "";
		float f;
		int n;

		if ( !value[0] ) {
			value = s->defaultValue;
		}
		switch ( s->type ) {
		case RS_FLOAT:
			f = (float)atof( value );
			memcpy( dest, &f, 4 );
			break;
		case RS_FLOAT_MIN:
			f = (float)atof( value );
			if ( !( f >= 0.001f ) ) {
				f = 0.001f;
			}
			memcpy( dest, &f, 4 );
			break;
		case RS_INT:
			n = atoi( value );
			memcpy( dest, &n, 4 );
			break;
		case RS_BOOL:
			n = atoi( value ) != 0;
			memcpy( dest, &n, 4 );
			break;
		}
	}
}

/*
==============================================================================

Player state field names, for reports

==============================================================================
*/

typedef struct {
	const char	*name;
	int			offset;
	int			count;
	int			size;
	qboolean	isFloat;
} psField_t;

#define PSF( f, n, fl )	{ #f, (int)offsetof( playerState_t, f ), n, (int)sizeof( ((playerState_t *)0)->f ) / ( n ), fl }

static const psField_t psFields[] = {
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

static const psField_t *LS_FieldAt( int offset, int *element ) {
	int i;

	for ( i = 0; i < ARRAY_LEN( psFields ); i++ ) {
		const psField_t *f = &psFields[i];

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

static unsigned LS_Rand( unsigned *rng ) {
	*rng = *rng * 1664525u + 1013904223u;
	return *rng >> 8;
}

static int LS_RandRange( unsigned *rng, int lo, int hi ) {
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

static const scenario_t scenarios[] = {
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

/*
==============================================================================

Lockstep

==============================================================================
*/

typedef union {
	playerState_t	ps;
	byte			raw[0x400];
} psBuffer_t;

typedef struct {
	int		frames;
	int		divergent;
	int		fieldHits[RETAIL_PS_SIZE / 4 + 16];
} lsResult_t;

static qboolean verbose;

static void LS_InitState( const scenario_t *sc, playerState_t *ps ) {
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

static void LS_BuildCommand( const lsInput_t *in, int serverTime, usercmd_t *cmd ) {
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

static void LS_Describe( const char *what, int offset, const byte *retail, const byte *ours ) {
	int element = 0;
	const psField_t *field = LS_FieldAt( offset, &element );
	int r, o;

	memcpy( &r, retail + offset, 4 );
	memcpy( &o, ours + offset, 4 );
	if ( field && field->isFloat ) {
		float rf, of;

		memcpy( &rf, &r, 4 );
		memcpy( &of, &o, 4 );
		printf( "      %s %s[%d]: retail %.9g ours %.9g (diff %.3g)\n", what, field->name, element,
			rf, of, rf - of );
	} else if ( field ) {
		if ( field->size == 1 ) {
			r = (signed char)retail[offset];
			o = (signed char)ours[offset];
		}
		printf( "      %s %s[%d]: retail %d (0x%x) ours %d (0x%x)\n", what, field->name, element, r, r, o, o );
	} else {
		printf( "      %s +0x%x: retail 0x%08x ours 0x%08x\n", what, offset, r, o );
	}
}

static qboolean LS_RunScenario( const scenario_t *sc, lsResult_t *result ) {
	static psBuffer_t retailPs, ourPs;
	static byte retailPm[RETAIL_PM_SIZE];
	pmoveSettings_t settings;
	pmove_t pm;
	lsInput_t in;
	usercmd_t cmd;
	unsigned rng = 0x9e3779b9u;
	int serverTime;
	int frame, offset;
	int reported = 0;
	const char *name;

	for ( name = sc->name; *name; name++ ) {
		rng = rng * 31u + (byte)*name;
	}

	memset( result, 0, sizeof( *result ) );
	BG_PmoveSettingsFromInfo( sc->settings, &settings );
	BG_WeaponReloadFromInfo( sc->settings, &settings );
	LS_ApplyRetailSettings( sc->settings );

	memset( &retailPs, 0, sizeof( retailPs ) );
	memset( &ourPs, 0, sizeof( ourPs ) );
	LS_InitState( sc, &retailPs.ps );
	LS_InitState( sc, &ourPs.ps );
	if ( sc->prep ) {
		sc->prep( -1, &retailPs.ps );
		sc->prep( -1, &ourPs.ps );
	}
	serverTime = retailPs.ps.commandTime;

	for ( frame = 0; frame < sc->frames; frame++ ) {
		memset( &in, 0, sizeof( in ) );
		sc->driver( sc, frame, &rng, &in );
		if ( sc->msec ) {
			in.msec = sc->msec;
		} else if ( LS_RandRange( &rng, 0, 49 ) == 0 ) {
			in.msec = LS_RandRange( &rng, 40, 120 );	// a hitch, split into chunks
		} else {
			in.msec = LS_RandRange( &rng, 1, 30 );
		}
		serverTime += in.msec;
		LS_BuildCommand( &in, serverTime, &cmd );
		if ( sc->prep ) {
			sc->prep( frame, &retailPs.ps );
			sc->prep( frame, &ourPs.ps );
		}

		// retail
		memset( retailPm, 0, sizeof( retailPm ) );
		*(playerState_t **)( retailPm + RPM_PS ) = &retailPs.ps;
		memcpy( retailPm + RPM_CMD, &cmd, sizeof( cmd ) );
		*(int *)( retailPm + RPM_TRACEMASK ) = MASK_PLAYERSOLID;
		*(int *)( retailPm + RPM_NOFOOTSTEPS ) = in.noFootsteps;
		*(int *)( retailPm + RPM_GAUNTLETHIT ) = in.gauntletHit;
		*(int *)( retailPm + RPM_GRAPPLESLOW ) = in.grappleSlow;
		*(void **)( retailPm + RPM_TRACE ) = (void *)LW_Trace;
		*(void **)( retailPm + RPM_POINTCONTENTS ) = (void *)LW_PointContents;
		LS_RetailPmove( retailPm );

		// ours
		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &ourPs.ps;
		pm.cmd = cmd;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.trace = LW_Trace;
		pm.pointcontents = LW_PointContents;
		pm.settings = &settings;
		pm.noFootsteps = in.noFootsteps;
		pm.gauntletHit = in.gauntletHit;
		pm.grappleSlow = in.grappleSlow;
		Pmove( &pm );

		result->frames++;
		{
			qboolean diverged = qfalse;
			qboolean reportThis = verbose || reported < 3;

			for ( offset = 0; offset < (int)sizeof( playerState_t ); offset += 4 ) {
				if ( memcmp( retailPs.raw + offset, ourPs.raw + offset, 4 ) ) {
					if ( !diverged && reportThis ) {
						printf( "   frame %d (msec %d, move %d %d %d, buttons 0x%x, yaw %.1f):\n", frame,
							in.msec, in.forwardmove, in.rightmove, in.upmove, in.buttons, in.yaw );
					}
					diverged = qtrue;
					result->fieldHits[offset / 4]++;
					if ( reportThis ) {
						LS_Describe( "ps", offset, retailPs.raw, ourPs.raw );
					}
				}
			}
			for ( offset = sizeof( playerState_t ); offset < RETAIL_PS_SIZE; offset += 4 ) {
				if ( memcmp( retailPs.raw + offset, ourPs.raw + offset, 4 ) ) {
					diverged = qtrue;
					result->fieldHits[offset / 4]++;
					if ( reportThis ) {
						LS_Describe( "ps(retail tail)", offset, retailPs.raw, ourPs.raw );
					}
				}
			}

#define PM_CHECK( label, retailOffset, ourField, bytes ) \
			if ( memcmp( retailPm + ( retailOffset ), &( ourField ), bytes ) ) { \
				diverged = qtrue; \
				result->fieldHits[RETAIL_PS_SIZE / 4 + ( retailOffset ) / 0x10]++; \
				if ( reportThis ) { \
					printf( "      pm %s differs\n", label ); \
				} \
			}
			PM_CHECK( "watertype", RPM_WATERTYPE, pm.watertype, 4 );
			PM_CHECK( "waterlevel", RPM_WATERLEVEL, pm.waterlevel, 4 );
			PM_CHECK( "mins", RPM_MINS, pm.mins, 12 );
			PM_CHECK( "maxs", RPM_MAXS, pm.maxs, 12 );
			PM_CHECK( "numtouch", RPM_NUMTOUCH, pm.numtouch, 4 );
			PM_CHECK( "xyspeed", RPM_XYSPEED, pm.xyspeed, 4 );
			PM_CHECK( "stepUp", RPM_STEPUP, pm.stepUp, 4 );
			PM_CHECK( "stepTime", RPM_STEPTIME, pm.stepTime, 4 );
			if ( !memcmp( retailPm + RPM_NUMTOUCH, &pm.numtouch, 4 ) ) {
				PM_CHECK( "touchents", RPM_TOUCHENTS, pm.touchents, 4 * pm.numtouch );
			}
#undef PM_CHECK

			if ( diverged ) {
				result->divergent++;
				if ( reportThis ) {
					reported++;
				}
				// resynchronise so the next frame starts from the same state
				memcpy( ourPs.raw, retailPs.raw, sizeof( ourPs.raw ) );
			}
		}
	}

	return result->divergent == 0;
}

static void LS_PrintHits( const lsResult_t *result ) {
	int i, element;

	for ( i = 0; i < ARRAY_LEN( result->fieldHits ); i++ ) {
		if ( result->fieldHits[i] ) {
			const psField_t *field = i < RETAIL_PS_SIZE / 4 ? LS_FieldAt( i * 4, &element ) : NULL;

			if ( field ) {
				printf( "      %s[%d] x%d\n", field->name, element, result->fieldHits[i] );
			} else if ( i < RETAIL_PS_SIZE / 4 ) {
				printf( "      ps+0x%x x%d\n", i * 4, result->fieldHits[i] );
			} else {
				printf( "      pm+0x%x x%d\n", ( i - RETAIL_PS_SIZE / 4 ) * 0x10, result->fieldHits[i] );
			}
		}
	}
}

int main( int argc, char **argv ) {
	const char *only = NULL;
	int total = 0, divergent = 0, failed = 0, i;
	unsigned control = 0;

	if ( argc < 3 ) {
		fprintf( stderr, "usage: %s <qagamex86.dll> <retail install dir> [--scenario name] [--verbose]\n",
			argv[0] );
		return 2;
	}
	for ( i = 3; i < argc; i++ ) {
		if ( !strcmp( argv[i], "--scenario" ) && i + 1 < argc ) {
			only = argv[++i];
		} else if ( !strcmp( argv[i], "--verbose" ) ) {
			verbose = qtrue;
		}
	}
	if ( !LS_LoadRetail( argv[1], argv[2] ) ) {
		return 2;
	}
	LW_Init();
	for ( i = 1; i < bg_numItems; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == HI_FLIGHT ) {
			lsFlightItem = i;
		}
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == HI_MEDKIT ) {
			lsMedkitItem = i;
		}
	}
	_controlfp_s( &control, 0, 0 );
	printf( "pmove lockstep: x87 control 0x%04x, %d scenarios\n", control & 0xffff,
		(int)ARRAY_LEN( scenarios ) );

	for ( i = 0; i < ARRAY_LEN( scenarios ); i++ ) {
		lsResult_t result;

		if ( only && Q_stricmp( only, scenarios[i].name ) ) {
			continue;
		}
		if ( !LS_RunScenario( &scenarios[i], &result ) ) {
			failed++;
		}
		printf( "%-16s %5d frames, %4d divergent\n", scenarios[i].name, result.frames, result.divergent );
		if ( result.divergent ) {
			LS_PrintHits( &result );
		}
		total += result.frames;
		divergent += result.divergent;
	}
	printf( "pmove lockstep: %d of %d frames divergent, %d scenario(s) failed\n", divergent, total, failed );
	return failed ? 1 : 0;
}
