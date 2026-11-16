/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// The native atof() and Q_sscanf %f (code/game/bg_lib.c): correctly rounded
// like the runtime the retail modules import, exponents accepted, what that
// runtime does not read (hexadecimal, inf, nan) read as zero, and the
// process locale ignored.

#include "../code/game/q_shared.h"
#include <locale.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

int Q_sscanf( const char *buffer, const char *fmt, ... );	// bg_public.h

void QDECL Com_Printf( const char *fmt, ... ) { (void)fmt; }
void QDECL Com_Error( int code, const char *fmt, ... ) { (void)code; (void)fmt; abort(); }

static unsigned FloatBits( float f ) {
	unsigned bits;

	memcpy( &bits, &f, sizeof( bits ) );
	return bits;
}

static void CheckParsing( void ) {
	float v[3];
	int count;

	// fluorescent's target_speaker angle (retail 0xc333ffff; float summing
	// gave 0xc333fffe)
	CHECK( FloatBits( (float)atof( "-179.999985" ) ) == 0xc333ffffu );
	CHECK( FloatBits( (float)atof( "179.999985" ) ) == 0x4333ffffu );
	CHECK( (float)atof( "0.9" ) == 0.9f );
	CHECK( atof( "1e3" ) == 1000.0 );
	CHECK( atof( "-2.5E-1" ) == -0.25 );
	CHECK( atof( "  \t42" ) == 42.0 );
	CHECK( atof( "+.5" ) == 0.5 );
	CHECK( atof( "-.5" ) == -0.5 );
	CHECK( atof( "7.25xyz" ) == 7.25 );
	CHECK( atof( "" ) == 0.0 );
	CHECK( atof( "-" ) == 0.0 );
	CHECK( atof( "." ) == 0.0 );
	CHECK( atof( "abc" ) == 0.0 );
	CHECK( atof( "0x10" ) == 0.0 );
	CHECK( atof( "inf" ) == 0.0 );
	CHECK( atof( "-nan" ) == 0.0 );

	// jumpwerkz's armor shard origin
	v[0] = v[1] = v[2] = 99.0f;
	count = Q_sscanf( "-2.42338e-027 608 504", "%f %f %f", &v[0], &v[1], &v[2] );
	CHECK( count == 3 );
	CHECK( fabs( v[0] ) < 1e-20f && v[1] == 608.0f && v[2] == 504.0f );
	CHECK( Q_sscanf( "1.5 -2 3e1", "%f %f %f", &v[0], &v[1], &v[2] ) == 3 );
	CHECK( v[0] == 1.5f && v[1] == -2.0f && v[2] == 30.0f );
	CHECK( FloatBits( ( Q_sscanf( "-179.999985", "%f", &v[0] ), v[0] ) ) == 0xc333ffffu );
	// a field that is not a number stops the scan
	CHECK( Q_sscanf( "4 x 6", "%f %f %f", &v[0], &v[1], &v[2] ) == 1 && v[0] == 4.0f );
}

int main( void ) {
	static const char *const commaLocales[] = { "de_DE.UTF-8", "de_DE", "fr_FR.UTF-8", "German", "French" };
	int i;

	CheckParsing();
	// A library that switches the process to a comma-decimal locale must not
	// change what the module reads.
	for ( i = 0; i < (int)ARRAY_LEN( commaLocales ); i++ ) {
		if ( setlocale( LC_NUMERIC, commaLocales[i] ) ) {
			CHECK( atof( "1.5" ) == 1.5 );
			CheckParsing();
			printf( "game-numbers: also checked under %s\n", commaLocales[i] );
			break;
		}
	}
	setlocale( LC_NUMERIC, "C" );
	printf( "game-numbers: ok\n" );
	return 0;
}
