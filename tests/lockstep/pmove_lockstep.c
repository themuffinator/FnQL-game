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

#include "lockstep_world.h"
#include "lockstep_scenarios.h"
#include "lockstep_digest.h"

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

// The trajectory routines both take time in EAX, trajectory in ESI and
// output in EDI (retail 0x1002d210 / 0x1002d3e0). Test sine movers beyond
// one cycle: early float rounding changes where they push their riders.
static qboolean LS_CheckSineTrajectories( lsRecorder_t *record ) {
	trajectory_t trajectory;
	vec3_t retail, ours;
	int index, derivative, atTime, checks = 0, mismatches = 0;

	LS_RecorderInit( record, "sine" );
	for ( index = 0; LS_SineSample( index, &trajectory, &atTime, &derivative ); index++ ) {
		void *entry = LS_RetailAddress( derivative ? 0x1002d3e0u : 0x1002d210u );

		__asm {
			mov eax, atTime
			lea esi, trajectory
			lea edi, retail
			call entry
		}
		if ( derivative ) {
			BG_EvaluateTrajectoryDelta( &trajectory, atTime, ours );
		} else {
			BG_EvaluateTrajectory( &trajectory, atTime, ours );
		}
		LS_DigestFloat( &record->chunk, retail[0] );
		LS_DigestFloat( &record->chunk, retail[1] );
		LS_DigestFloat( &record->chunk, retail[2] );
		record->record.frames++;
		checks++;
		if ( memcmp(retail, ours, sizeof(vec3_t)) ) {
			if ( mismatches++ < 3 ) {
				printf( "sine trajectory mismatch: derivative %d, duration %d, time %d\n",
					derivative, trajectory.trDuration, atTime );
			}
		}
	}
	record->inChunk = 1;
	LS_RecorderFinish( record );
	printf( "sine trajectories: %d of %d evaluations divergent\n", mismatches, checks );
	return mismatches == 0;
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

// the retail Pmove's results, read at the retail pmove_t offsets
static void LS_DigestRetail( lsDigest_t *digest, const playerState_t *ps, const byte *retailPm ) {
	lsPmOutputs_t out;

	memset( &out, 0, sizeof( out ) );
	memcpy( &out.numtouch, retailPm + RPM_NUMTOUCH, 4 );
	if ( out.numtouch > 0 && out.numtouch <= MAXTOUCH ) {
		memcpy( out.touchents, retailPm + RPM_TOUCHENTS, 4 * out.numtouch );
	}
	memcpy( out.mins, retailPm + RPM_MINS, 12 );
	memcpy( out.maxs, retailPm + RPM_MAXS, 12 );
	memcpy( &out.watertype, retailPm + RPM_WATERTYPE, 4 );
	memcpy( &out.waterlevel, retailPm + RPM_WATERLEVEL, 4 );
	memcpy( &out.xyspeed, retailPm + RPM_XYSPEED, 4 );
	memcpy( &out.stepUp, retailPm + RPM_STEPUP, 4 );
	memcpy( &out.stepTime, retailPm + RPM_STEPTIME, 4 );
	LS_DigestFrame( digest, ps, &out );
}

// also digests the retail output into `record` (gate G3c, lockstep_digest.h)
static qboolean LS_RunScenario( const scenario_t *sc, lsResult_t *result, lsRecorder_t *record ) {
	static psBuffer_t retailPs, ourPs;
	static byte retailPm[RETAIL_PM_SIZE];
	pmoveSettings_t settings;
	pmove_t pm;
	lsInput_t in;
	usercmd_t cmd;
	unsigned rng = LS_ScenarioSeed( sc );
	int serverTime;
	int frame, offset;
	int reported = 0;

	memset( result, 0, sizeof( *result ) );
	LS_RecorderInit( record, sc->name );
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
		LS_NextFrame( sc, frame, &rng, &in, &serverTime, &cmd );
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
		LS_DigestRetail( &record->chunk, &retailPs.ps, retailPm );
		LS_RecorderEndFrame( record );

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

	LS_RecorderFinish( record );
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

// G3c: checks the retail records against the committed golden file, or
// writes them (only for a full run)
static int LS_GoldenRecords( const lsGoldenRecord_t *records, int count, const char *goldenPath,
		const char *writePath ) {
	static lsGoldenRecord_t golden[LS_MAX_GOLDEN];
	int i, numGolden, stale = 0;

	if ( writePath ) {
		FILE *file = fopen( writePath, "w" );

		if ( !file ) {
			fprintf( stderr, "cannot write %s\n", writePath );
			return 1;
		}
		fprintf( file, "# Gate G3c golden: the retail qagamex86.dll Pmove output for every\n"
			"# pmove lockstep scenario (tests/lockstep/lockstep_digest.h), written by\n"
			"# pmove_lockstep --write-golden and checked against the retail module by\n"
			"# every G3a run.  pmove_replay compares FnQL-game's Pmove on each target.\n" );
		for ( i = 0; i < count; i++ ) {
			LS_GoldenWrite( file, &records[i] );
		}
		fclose( file );
		printf( "pmove golden: wrote %d records to %s\n", count, writePath );
	}
	if ( !goldenPath ) {
		return 0;
	}
	numGolden = LS_GoldenRead( goldenPath, golden, ARRAY_LEN( golden ) );
	if ( numGolden < 0 ) {
		printf( "pmove golden: cannot read %s\n", goldenPath );
		return 1;
	}
	for ( i = 0; i < count; i++ ) {
		const lsGoldenRecord_t *expected = LS_GoldenFind( golden, numGolden, records[i].name );

		if ( !expected ) {
			printf( "   %s: not in the golden file\n", records[i].name );
			stale++;
		} else if ( !LS_GoldenCompare( expected, &records[i] ) ) {
			stale++;
		}
	}
	printf( "pmove golden: %d of %d retail records differ from %s\n", stale, count, goldenPath );
	return stale ? 1 : 0;
}

int main( int argc, char **argv ) {
	static lsRecorder_t recorder;
	static lsGoldenRecord_t records[LS_MAX_GOLDEN];
	const char *only = NULL, *goldenPath = NULL, *writePath = NULL;
	int total = 0, divergent = 0, failed = 0, numRecords = 0, i;
	unsigned control = 0;

	if ( argc < 3 ) {
		fprintf( stderr, "usage: %s <qagamex86.dll> <retail install dir> [--scenario name] [--verbose]"
			" [--golden pmove_golden.txt] [--write-golden path]\n", argv[0] );
		return 2;
	}
	for ( i = 3; i < argc; i++ ) {
		if ( !strcmp( argv[i], "--scenario" ) && i + 1 < argc ) {
			only = argv[++i];
		} else if ( !strcmp( argv[i], "--verbose" ) ) {
			verbose = qtrue;
		} else if ( !strcmp( argv[i], "--golden" ) && i + 1 < argc ) {
			goldenPath = argv[++i];
		} else if ( !strcmp( argv[i], "--write-golden" ) && i + 1 < argc ) {
			writePath = argv[++i];
		}
	}
	if ( !LS_LoadRetail( argv[1], argv[2] ) ) {
		return 2;
	}
	if ( only && writePath ) {
		fprintf( stderr, "--write-golden needs every scenario; drop --scenario\n" );
		return 2;
	}
	if ( !LS_CheckSineTrajectories( &recorder ) ) {
		failed++;
	}
	records[numRecords++] = recorder.record;
	LW_Init();
	LS_InitScenarios();
	_controlfp_s( &control, 0, 0 );
	printf( "pmove lockstep: x87 control 0x%04x, %d scenarios\n", control & 0xffff,
		lsNumScenarios );

	for ( i = 0; i < lsNumScenarios; i++ ) {
		lsResult_t result;

		if ( only && Q_stricmp( only, lsScenarios[i].name ) ) {
			continue;
		}
		if ( !LS_RunScenario( &lsScenarios[i], &result, &recorder ) ) {
			failed++;
		}
		if ( numRecords < ARRAY_LEN( records ) ) {
			records[numRecords++] = recorder.record;
		}
		printf( "%-16s %5d frames, %4d divergent\n", lsScenarios[i].name, result.frames, result.divergent );
		if ( result.divergent ) {
			LS_PrintHits( &result );
		}
		total += result.frames;
		divergent += result.divergent;
	}
	printf( "pmove lockstep: %d of %d frames divergent, %d scenario(s) failed\n", divergent, total, failed );
	if ( LS_GoldenRecords( records, numRecords, goldenPath, writePath ) ) {
		failed++;
	}
	return failed ? 1 : 0;
}
