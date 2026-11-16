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
// lockstep_digest.c -- architecture-independent digests of Pmove output.
//

#include <stdlib.h>

#include "lockstep_digest.h"

// 64-bit FNV-1a
#define LS_FNV_OFFSET	0xcbf29ce484222325ull
#define LS_FNV_PRIME	0x00000100000001b3ull

void LS_DigestInit( lsDigest_t *digest ) {
	digest->hash = LS_FNV_OFFSET;
	digest->bytes = 0;
}

void LS_DigestBytes( lsDigest_t *digest, const void *data, int length ) {
	const byte *p = (const byte *)data;
	int i;

	for ( i = 0; i < length; i++ ) {
		digest->hash ^= p[i];
		digest->hash *= LS_FNV_PRIME;
	}
	digest->bytes += (unsigned long long)length;
}

// a value of `width` bytes (1, 2 or 4) in little-endian order
static void LS_DigestValue( lsDigest_t *digest, unsigned value, int width ) {
	byte le[4];
	int i;

	for ( i = 0; i < width; i++ ) {
		le[i] = (byte)( value >> ( 8 * i ) );
	}
	LS_DigestBytes( digest, le, width );
}

void LS_DigestInt( lsDigest_t *digest, int value ) {
	LS_DigestValue( digest, (unsigned)value, 4 );
}

void LS_DigestFloat( lsDigest_t *digest, float value ) {
	unsigned bits;

	memcpy( &bits, &value, sizeof( bits ) );
	LS_DigestValue( digest, bits, 4 );
}

// reads one element of a playerState_t field at its own width
static unsigned LS_ReadField( const byte *at, int size ) {
	unsigned char b;
	unsigned short s;
	unsigned u;

	switch ( size ) {
	case 1:
		memcpy( &b, at, 1 );
		return b;
	case 2:
		memcpy( &s, at, 2 );
		return s;
	default:
		memcpy( &u, at, 4 );
		return u;
	}
}

void LS_DigestFrame( lsDigest_t *digest, const playerState_t *ps, const lsPmOutputs_t *out ) {
	const byte *raw = (const byte *)ps;
	int i, element, numtouch;

	for ( i = 0; i < lsNumPsFields; i++ ) {
		const psField_t *f = &lsPsFields[i];

		for ( element = 0; element < f->count; element++ ) {
			LS_DigestValue( digest, LS_ReadField( raw + f->offset + element * f->size, f->size ), f->size );
		}
	}

	numtouch = out->numtouch;
	LS_DigestInt( digest, numtouch );
	if ( numtouch < 0 ) {
		numtouch = 0;
	} else if ( numtouch > MAXTOUCH ) {
		numtouch = MAXTOUCH;
	}
	for ( i = 0; i < numtouch; i++ ) {
		LS_DigestInt( digest, out->touchents[i] );
	}
	for ( i = 0; i < 3; i++ ) {
		LS_DigestFloat( digest, out->mins[i] );
	}
	for ( i = 0; i < 3; i++ ) {
		LS_DigestFloat( digest, out->maxs[i] );
	}
	LS_DigestInt( digest, out->watertype );
	LS_DigestInt( digest, out->waterlevel );
	LS_DigestFloat( digest, out->xyspeed );
	LS_DigestFloat( digest, out->stepUp );
	LS_DigestInt( digest, out->stepTime );
}

void LS_PmOutputsFromPmove( const pmove_t *pm, lsPmOutputs_t *out ) {
	memset( out, 0, sizeof( *out ) );
	out->numtouch = pm->numtouch;
	memcpy( out->touchents, pm->touchents, sizeof( out->touchents ) );
	VectorCopy( pm->mins, out->mins );
	VectorCopy( pm->maxs, out->maxs );
	out->watertype = pm->watertype;
	out->waterlevel = pm->waterlevel;
	out->xyspeed = pm->xyspeed;
	out->stepUp = pm->stepUp;
	out->stepTime = pm->stepTime;
}

int LS_UnnamedStateBytes( void ) {
	byte named[sizeof( playerState_t )];
	int i, unnamed = 0;

	memset( named, 0, sizeof( named ) );
	for ( i = 0; i < lsNumPsFields; i++ ) {
		const psField_t *f = &lsPsFields[i];
		int end = f->offset + f->count * f->size;
		int at;

		for ( at = f->offset; at < end && at < (int)sizeof( named ); at++ ) {
			named[at] = 1;
		}
	}
	for ( i = 0; i < (int)sizeof( named ); i++ ) {
		unnamed += !named[i];
	}
	return unnamed;
}

// the sine mover grid of the retail lockstep check (retail 0x1002d210 /
// 0x1002d3e0): beyond one cycle, early float rounding changes where a mover
// pushes its riders
qboolean LS_SineSample( int index, trajectory_t *trajectory, int *atTime, int *derivative ) {
	static const int durations[] = { 4000, 3000, 731 };
	const int timesPerDuration = ( 20000 - -1000 ) / 37 + 1;
	const int perDerivative = timesPerDuration * ARRAY_LEN( durations );

	if ( index < 0 || index >= 2 * perDerivative ) {
		return qfalse;
	}
	memset( trajectory, 0, sizeof( *trajectory ) );
	trajectory->trType = TR_SINE;
	trajectory->trTime = 173;
	VectorSet( trajectory->trBase, 10.25f, -17.75f, 32 );
	VectorSet( trajectory->trDelta, -11.5f, 63.25f, 4 );
	*derivative = index / perDerivative;
	index %= perDerivative;
	trajectory->trDuration = durations[index / timesPerDuration];
	*atTime = -1000 + 37 * ( index % timesPerDuration );
	return qtrue;
}

/*
==============================================================================

Golden records

==============================================================================
*/

void LS_RecorderInit( lsRecorder_t *recorder, const char *name ) {
	memset( recorder, 0, sizeof( *recorder ) );
	Q_strncpyz( recorder->record.name, name, sizeof( recorder->record.name ) );
	LS_DigestInit( &recorder->chunk );
}

void LS_RecorderEndFrame( lsRecorder_t *recorder ) {
	recorder->record.frames++;
	if ( ++recorder->inChunk == LS_DIGEST_CHUNK ) {
		LS_RecorderFinish( recorder );
	}
}

void LS_RecorderFinish( lsRecorder_t *recorder ) {
	if ( !recorder->inChunk ) {
		return;
	}
	if ( recorder->record.numChunks < LS_MAX_CHUNKS ) {
		recorder->record.chunks[recorder->record.numChunks++] = recorder->chunk.hash;
	}
	LS_DigestInit( &recorder->chunk );
	recorder->inChunk = 0;
}

void LS_GoldenWrite( FILE *file, const lsGoldenRecord_t *record ) {
	int i;

	if ( !Q_stricmp( record->name, "sine" ) ) {
		fprintf( file, "sine %d", record->frames );
	} else {
		fprintf( file, "scenario %s %d", record->name, record->frames );
	}
	for ( i = 0; i < record->numChunks; i++ ) {
		fprintf( file, " %016llx", record->chunks[i] );
	}
	fputc( '\n', file );
}

int LS_GoldenRead( const char *path, lsGoldenRecord_t *records, int max ) {
	char line[4096];
	FILE *file = fopen( path, "r" );
	int count = 0;

	if ( !file ) {
		return -1;
	}
	while ( count < max && fgets( line, sizeof( line ), file ) ) {
		lsGoldenRecord_t *r = &records[count];
		char *cursor = line, *end;
		char kind[16];
		int used = 0;

		if ( line[0] == '#' || line[0] == '\n' || line[0] == '\r' ) {
			continue;
		}
		memset( r, 0, sizeof( *r ) );
		if ( sscanf( cursor, "%15s%n", kind, &used ) != 1 ) {
			continue;
		}
		cursor += used;
		if ( !strcmp( kind, "sine" ) ) {
			Q_strncpyz( r->name, "sine", sizeof( r->name ) );
		} else if ( !strcmp( kind, "scenario" ) ) {
			if ( sscanf( cursor, "%63s%n", r->name, &used ) != 1 ) {
				continue;
			}
			cursor += used;
		} else {
			continue;
		}
		r->frames = (int)strtol( cursor, &end, 10 );
		if ( end == cursor ) {
			continue;
		}
		cursor = end;
		while ( r->numChunks < LS_MAX_CHUNKS ) {
			unsigned long long value = strtoull( cursor, &end, 16 );

			if ( end == cursor ) {
				break;
			}
			r->chunks[r->numChunks++] = value;
			cursor = end;
		}
		count++;
	}
	fclose( file );
	return count;
}

const lsGoldenRecord_t *LS_GoldenFind( const lsGoldenRecord_t *records, int count, const char *name ) {
	int i;

	for ( i = 0; i < count; i++ ) {
		if ( !Q_stricmp( records[i].name, name ) ) {
			return &records[i];
		}
	}
	return NULL;
}

qboolean LS_GoldenCompare( const lsGoldenRecord_t *expected, const lsGoldenRecord_t *got ) {
	int i;

	if ( expected->frames != got->frames || expected->numChunks != got->numChunks ) {
		printf( "   %s: %d frames in %d checkpoints, golden has %d in %d\n", got->name, got->frames,
			got->numChunks, expected->frames, expected->numChunks );
		return qfalse;
	}
	for ( i = 0; i < got->numChunks; i++ ) {
		if ( expected->chunks[i] != got->chunks[i] ) {
			printf( "   %s: first divergence in frames %d-%d (digest %016llx, golden %016llx)\n", got->name,
				i * LS_DIGEST_CHUNK, ( i + 1 ) * LS_DIGEST_CHUNK - 1, got->chunks[i], expected->chunks[i] );
			return qfalse;
		}
	}
	return qtrue;
}
