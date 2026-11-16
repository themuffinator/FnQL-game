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
// pmove_replay.c -- gate G3c: FnQL-game's Pmove on this target against the
// retail Pmove's recorded output.
//
// The retail lockstep harness (pmove_lockstep.c) runs the retail Pmove on
// x86 Windows and records a digest of its output for every scenario in
// tests/lockstep/pmove_golden.txt (--write-golden), and checks the file
// against the retail module on each G3a run (--golden).  This harness runs
// FnQL-game's Pmove alone, over the same scenarios and the same brush world,
// on whatever target it was built for, and compares its digests with that
// file.  A retail client predicts the server's movement with the retail
// (x86) Pmove, so a server on another architecture is only indistinguishable
// from retail if this matches bit for bit.
//
// usage: pmove_replay <pmove_golden.txt> [--scenario name] [--write path]
//        [--trace name]
//

#include <stdio.h>
#include <stdlib.h>

#include "lockstep_world.h"
#include "lockstep_digest.h"

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

/*
==============================================================================

Replay

==============================================================================
*/

static qboolean PR_LittleEndian( void ) {
	const unsigned one = 1;
	byte first;

	memcpy( &first, &one, 1 );
	return first == 1;
}

static const char *PR_TargetName( void ) {
#if defined( _M_IX86 ) || defined( __i386__ )
	return "x86";
#elif defined( _M_AMD64 ) || defined( __x86_64__ )
	return "x86_64";
#elif defined( __aarch64__ )
	return "aarch64";
#elif defined( __arm__ )
	return "arm";
#elif defined( __PPC64__ ) && defined( __LITTLE_ENDIAN__ )
	return "ppc64le";
#elif defined( __PPC64__ )
	return "ppc64";
#else
	return "unknown";
#endif
}

static void PR_ReplaySine( lsRecorder_t *recorder, qboolean trace ) {
	trajectory_t trajectory;
	vec3_t out;
	int index, atTime, derivative;

	LS_RecorderInit( recorder, "sine" );
	for ( index = 0; LS_SineSample( index, &trajectory, &atTime, &derivative ); index++ ) {
		if ( derivative ) {
			BG_EvaluateTrajectoryDelta( &trajectory, atTime, out );
		} else {
			BG_EvaluateTrajectory( &trajectory, atTime, out );
		}
		if ( trace ) {
			printf( "sine %d derivative %d duration %d time %d: %.9g %.9g %.9g\n", index, derivative,
				trajectory.trDuration, atTime, out[0], out[1], out[2] );
		}
		LS_DigestFloat( &recorder->chunk, out[0] );
		LS_DigestFloat( &recorder->chunk, out[1] );
		LS_DigestFloat( &recorder->chunk, out[2] );
		recorder->record.frames++;
	}
	recorder->inChunk = 1;
	LS_RecorderFinish( recorder );
}

static void PR_ReplayScenario( const scenario_t *sc, lsRecorder_t *recorder, qboolean trace ) {
	static playerState_t ps;
	pmoveSettings_t settings;
	lsPmOutputs_t outputs;
	pmove_t pm;
	lsInput_t in;
	usercmd_t cmd;
	unsigned rng = LS_ScenarioSeed( sc );
	int serverTime, frame;

	LS_RecorderInit( recorder, sc->name );
	BG_PmoveSettingsFromInfo( sc->settings, &settings );
	BG_WeaponReloadFromInfo( sc->settings, &settings );
	LS_InitState( sc, &ps );
	if ( sc->prep ) {
		sc->prep( -1, &ps );
	}
	serverTime = ps.commandTime;

	for ( frame = 0; frame < sc->frames; frame++ ) {
		LS_NextFrame( sc, frame, &rng, &in, &serverTime, &cmd );
		if ( sc->prep ) {
			sc->prep( frame, &ps );
		}

		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &ps;
		pm.cmd = cmd;
		pm.tracemask = MASK_PLAYERSOLID;
		pm.trace = LW_Trace;
		pm.pointcontents = LW_PointContents;
		pm.settings = &settings;
		pm.noFootsteps = in.noFootsteps;
		pm.gauntletHit = in.gauntletHit;
		pm.grappleSlow = in.grappleSlow;
		Pmove( &pm );

		LS_PmOutputsFromPmove( &pm, &outputs );
		LS_DigestFrame( &recorder->chunk, &ps, &outputs );
		if ( trace ) {
			lsDigest_t one;

			LS_DigestInit( &one );
			LS_DigestFrame( &one, &ps, &outputs );
			printf( "%s %d %016llx origin %.9g %.9g %.9g velocity %.9g %.9g %.9g\n", sc->name, frame,
				one.hash, ps.origin[0], ps.origin[1], ps.origin[2], ps.velocity[0], ps.velocity[1],
				ps.velocity[2] );
		}
		LS_RecorderEndFrame( recorder );
	}
	LS_RecorderFinish( recorder );
}

int main( int argc, char **argv ) {
	static lsGoldenRecord_t golden[LS_MAX_GOLDEN];
	static lsRecorder_t recorder;
	const char *only = NULL, *tracing = NULL, *writePath = NULL;
	FILE *writeFile = NULL;
	int numGolden, unnamed, i, checked = 0, failed = 0;
	const lsGoldenRecord_t *expected;

	if ( argc < 2 ) {
		fprintf( stderr, "usage: %s <pmove_golden.txt> [--scenario name] [--write path] [--trace name]\n",
			argv[0] );
		return 2;
	}
	for ( i = 2; i < argc; i++ ) {
		if ( !strcmp( argv[i], "--scenario" ) && i + 1 < argc ) {
			only = argv[++i];
		} else if ( !strcmp( argv[i], "--trace" ) && i + 1 < argc ) {
			tracing = argv[++i];
			only = tracing;
		} else if ( !strcmp( argv[i], "--write" ) && i + 1 < argc ) {
			writePath = argv[++i];
		} else {
			fprintf( stderr, "unknown argument %s\n", argv[i] );
			return 2;
		}
	}

	numGolden = LS_GoldenRead( argv[1], golden, ARRAY_LEN( golden ) );
	if ( numGolden < 0 ) {
		fprintf( stderr, "cannot read %s\n", argv[1] );
		return 2;
	}
	// every byte of the state must be named, or the digest would skip it
	unnamed = LS_UnnamedStateBytes();
	if ( unnamed ) {
		printf( "pmove replay: %d bytes of playerState_t are not in lsPsFields\n", unnamed );
		return 1;
	}
	if ( writePath ) {
		writeFile = fopen( writePath, "w" );
		if ( !writeFile ) {
			fprintf( stderr, "cannot write %s\n", writePath );
			return 2;
		}
	}

	LW_Init();
	LS_InitScenarios();
	printf( "pmove replay: %s, %d-bit, %s-endian, %d scenarios, %d golden records\n", PR_TargetName(),
		(int)sizeof( void * ) * 8, PR_LittleEndian() ? "little" : "big", lsNumScenarios, numGolden );

	if ( !only || !Q_stricmp( only, "sine" ) ) {
		qboolean match;

		PR_ReplaySine( &recorder, tracing != NULL );
		if ( writeFile ) {
			LS_GoldenWrite( writeFile, &recorder.record );
		}
		expected = LS_GoldenFind( golden, numGolden, "sine" );
		checked++;
		if ( !expected ) {
			printf( "   sine: no golden record\n" );
			match = qfalse;
		} else {
			match = LS_GoldenCompare( expected, &recorder.record );
		}
		if ( !match ) {
			failed++;
		}
		printf( "%-16s %5d evaluations %s\n", "sine", recorder.record.frames, match ? "match" : "DIFFER" );
	}

	for ( i = 0; i < lsNumScenarios; i++ ) {
		const scenario_t *sc = &lsScenarios[i];
		qboolean match;

		if ( only && Q_stricmp( only, sc->name ) ) {
			continue;
		}
		PR_ReplayScenario( sc, &recorder, tracing != NULL );
		if ( writeFile ) {
			LS_GoldenWrite( writeFile, &recorder.record );
		}
		expected = LS_GoldenFind( golden, numGolden, sc->name );
		checked++;
		if ( !expected ) {
			printf( "   %s: no golden record\n", sc->name );
			match = qfalse;
		} else {
			match = LS_GoldenCompare( expected, &recorder.record );
		}
		if ( !match ) {
			failed++;
		}
		printf( "%-16s %5d frames %s\n", sc->name, recorder.record.frames, match ? "match" : "DIFFER" );
	}

	if ( writeFile ) {
		fclose( writeFile );
	}
	printf( "pmove replay: %d of %d records differ from the retail golden\n", failed, checked );
	return failed ? 1 : 0;
}
