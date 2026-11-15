/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// The stats documents' JSON writer: objects sorted by key with the last
// value of a repeated key (as JsonCpp's maps give retail), every value type,
// escaping, and every bound.

#include "../code/game/g_json.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

#define SIZE 4096
static char			buffer[SIZE + 32], scratch[SIZE + 32];
static jsonWriter_t	w;

static void Begin( int size ) {
	memset( buffer, 0x5a, sizeof( buffer ) );
	memset( scratch, 0x5a, sizeof( scratch ) );
	JSON_Init( &w, buffer, scratch, size );
}

// the finished text, with both buffers intact beyond their size
static const char *Finish( int size ) {
	const char	*text = JSON_Finish( &w );
	int			i;

	for ( i = size; i < (int)sizeof( buffer ); i++ ) {
		CHECK( buffer[i] == 0x5a && scratch[i] == 0x5a );
	}
	return text;
}

static const char *Real( double value ) {
	Begin( SIZE );
	JSON_Real( &w, NULL, value );
	return Finish( SIZE );
}

int main( void ) {
	const char	*text;
	char		key[8];
	int			i;

	// scalars as root values
	Begin( SIZE ); JSON_Null( &w, NULL ); CHECK( !strcmp( Finish( SIZE ), "null" ) );
	Begin( SIZE ); JSON_Bool( &w, NULL, qtrue ); CHECK( !strcmp( Finish( SIZE ), "true" ) );
	Begin( SIZE ); JSON_Bool( &w, NULL, qfalse ); CHECK( !strcmp( Finish( SIZE ), "false" ) );
	Begin( SIZE ); JSON_Int( &w, NULL, -2147483647 - 1 ); CHECK( !strcmp( Finish( SIZE ), "-2147483648" ) );
	Begin( SIZE ); JSON_Int( &w, NULL, 2147483647 ); CHECK( !strcmp( Finish( SIZE ), "2147483647" ) );
	Begin( SIZE ); JSON_String( &w, NULL, NULL ); CHECK( !strcmp( Finish( SIZE ), "\"\"" ) );

	// a real always reads as a real: one zero is kept after the last digit that matters
	CHECK( !strcmp( Real( 0.0 ), "0.0" ) );
	CHECK( !strcmp( Real( 4.0 ), "4.0" ) );
	CHECK( !strcmp( Real( -15.0 ), "-15.0" ) );
	CHECK( !strcmp( Real( 1200.0 ), "1200.0" ) );
	CHECK( !strcmp( Real( 0.5 ), "0.50" ) );
	CHECK( !strcmp( Real( 306.8167724609375 ), "306.8167724609375" ) );
	CHECK( !strcmp( Real( (float)24.125003814697266 ), "24.12500381469727" ) );
	CHECK( strstr( Real( 1e20 ), "e+" ) != NULL );		// an exponent is left as printed
	// values with no JSON form
	CHECK( !strcmp( Real( 1e308 * 10.0 ), "0.0" ) );
	CHECK( !strcmp( Real( -1e308 * 10.0 ), "0.0" ) );
	CHECK( !strcmp( Real( ( 1e308 * 10.0 ) - ( 1e308 * 10.0 ) ), "0.0" ) );

	// strings: quotes, backslashes, controls and bytes beyond ASCII
	Begin( SIZE );
	JSON_String( &w, NULL, "a\"b\\c\n\r\t\b\f\x01\x7f\xe9z" );
	CHECK( !strcmp( Finish( SIZE ), "\"a\\\"b\\\\c\\n\\r\\t\\b\\f\\u0001\\u007f\\u00e9z\"" ) );

	// an object comes out sorted by key, whatever the writing order
	Begin( SIZE );
	JSON_BeginObject( &w, NULL );
	JSON_Int( &w, "TIME", 11 );
	JSON_Bool( &w, "WARMUP", qtrue );
	JSON_String( &w, "MATCH_GUID", "guid" );
	JSON_Null( &w, "KILLER" );
	JSON_Int( &w, "TEAM_ALIVE", 2 );
	JSON_Bool( &w, "TEAMKILL", qfalse );
	JSON_Int( &w, "TEAM", 1 );
	JSON_End( &w );
	CHECK( !strcmp( Finish( SIZE ),
		"{\"KILLER\":null,\"MATCH_GUID\":\"guid\",\"TEAM\":1,\"TEAMKILL\":false,\"TEAM_ALIVE\":2,\"TIME\":11,\"WARMUP\":true}" ) );

	// a key written twice keeps its last value, in the key's sorted place
	Begin( SIZE );
	JSON_BeginObject( &w, NULL );
	JSON_Int( &w, "INVULNERABILITY", 7 );
	JSON_Int( &w, "QUAD", 1 );
	JSON_Int( &w, "INVULNERABILITY", 3 );
	JSON_Int( &w, "AMMO", 9 );
	JSON_End( &w );
	CHECK( !strcmp( Finish( SIZE ), "{\"AMMO\":9,\"INVULNERABILITY\":3,\"QUAD\":1}" ) );

	// nesting: arrays keep their order, objects inside sort on their own
	Begin( SIZE );
	JSON_BeginObject( &w, NULL );
	JSON_BeginArray( &w, "PLAYERS" );
	for ( i = 0; i < 2; i++ ) {
		JSON_BeginObject( &w, NULL );
		JSON_Int( &w, "TEAM", i );
		JSON_String( &w, "NAME", i ? "b" : "a" );
		JSON_End( &w );
	}
	JSON_End( &w );
	JSON_BeginObject( &w, "EMPTY" );
	JSON_End( &w );
	JSON_BeginArray( &w, "A" );
	JSON_End( &w );
	JSON_BeginObject( &w, "POSITION" );
	JSON_Real( &w, "Z", 1.5 );
	JSON_Real( &w, "X", -2.0 );
	JSON_End( &w );
	JSON_End( &w );
	CHECK( !strcmp( Finish( SIZE ), "{\"A\":[],\"EMPTY\":{},\"PLAYERS\":[{\"NAME\":\"a\",\"TEAM\":0},{\"NAME\":\"b\",\"TEAM\":1}],"
		"\"POSITION\":{\"X\":-2.0,\"Z\":1.50}}" ) );

	// keys that are prefixes of one another
	Begin( SIZE );
	JSON_BeginObject( &w, NULL );
	JSON_Int( &w, "AB", 2 );
	JSON_Int( &w, "A", 1 );
	JSON_Int( &w, "ABC", 3 );
	JSON_End( &w );
	CHECK( !strcmp( Finish( SIZE ), "{\"A\":1,\"AB\":2,\"ABC\":3}" ) );

	// misuse fails the whole document: a second root, an unclosed container,
	// a stray close, a missing or unusable key
	Begin( SIZE ); JSON_Int( &w, NULL, 1 ); JSON_Int( &w, NULL, 2 ); CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE ); JSON_BeginObject( &w, NULL ); CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE ); JSON_End( &w ); CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE ); CHECK( Finish( SIZE ) == NULL );		// nothing written
	Begin( SIZE ); JSON_BeginObject( &w, NULL ); JSON_Int( &w, NULL, 1 ); JSON_End( &w ); CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE ); JSON_BeginObject( &w, NULL ); JSON_Int( &w, "", 1 ); JSON_End( &w ); CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE ); JSON_BeginObject( &w, NULL ); JSON_Int( &w, "A\"B", 1 ); JSON_End( &w ); CHECK( Finish( SIZE ) == NULL );
	JSON_Init( &w, NULL, scratch, SIZE ); JSON_Int( &w, NULL, 1 ); CHECK( JSON_Finish( &w ) == NULL );
	JSON_Init( &w, buffer, NULL, SIZE ); JSON_Int( &w, NULL, 1 ); CHECK( JSON_Finish( &w ) == NULL );
	JSON_Init( &w, buffer, scratch, 4 ); JSON_Int( &w, NULL, 1 ); CHECK( JSON_Finish( &w ) == NULL );

	// too deep, and too many members
	Begin( SIZE );
	for ( i = 0; i <= JSON_MAX_DEPTH; i++ ) {
		JSON_BeginArray( &w, NULL );
	}
	for ( i = 0; i <= JSON_MAX_DEPTH; i++ ) {
		JSON_End( &w );
	}
	CHECK( Finish( SIZE ) == NULL );
	Begin( SIZE );
	JSON_BeginArray( &w, NULL );
	for ( i = 0; i < JSON_MAX_MEMBERS; i++ ) {
		JSON_Int( &w, NULL, i );
	}
	JSON_End( &w );
	CHECK( Finish( SIZE ) != NULL );
	Begin( SIZE );
	JSON_BeginArray( &w, NULL );
	for ( i = 0; i <= JSON_MAX_MEMBERS; i++ ) {
		JSON_Int( &w, NULL, i );
	}
	JSON_End( &w );
	CHECK( Finish( SIZE ) == NULL );

	// every buffer size from nothing up: the document either fits whole and
	// terminated, or fails; neither buffer is ever written past its size
	for ( i = 8; i <= 96; i++ ) {
		Begin( i );
		JSON_BeginObject( &w, NULL );
		JSON_String( &w, "NAME", "Sarge" );
		JSON_BeginArray( &w, "POWERUPS" );
		JSON_String( &w, NULL, "QUAD" );
		JSON_String( &w, NULL, "HASTE" );
		JSON_End( &w );
		JSON_Int( &w, "AMMO", 25 );
		JSON_End( &w );
		text = Finish( i );
		if ( text ) {
			CHECK( !strcmp( text, "{\"AMMO\":25,\"NAME\":\"Sarge\",\"POWERUPS\":[\"QUAD\",\"HASTE\"]}" ) );
			CHECK( (int)strlen( text ) < i );
		}
	}
	CHECK( text != NULL );		// and at 96 bytes it does fit

	// many members sort correctly
	Begin( SIZE );
	JSON_BeginObject( &w, NULL );
	for ( i = 25; i >= 0; i-- ) {
		key[0] = (char)( 'A' + ( i * 7 ) % 26 );
		key[1] = '\0';
		JSON_Int( &w, key, i );
	}
	JSON_End( &w );
	text = Finish( SIZE );
	CHECK( text != NULL );
	for ( i = 0; i < 25; i++ ) {
		const char	*a, *b;

		key[0] = '"'; key[1] = (char)( 'A' + i ); key[2] = '"'; key[3] = '\0';
		a = strstr( text, key );
		key[1] = (char)( 'A' + i + 1 );
		b = strstr( text, key );
		CHECK( a && b && a < b );
	}
	return 0;
}
