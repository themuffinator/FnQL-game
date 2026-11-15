/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// Chat tokens (retail qagamex86 0x10041450): expansion, the characters that
// are not tokens, the spectator copy and every bound.

#include "../code/game/g_local.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static int notes;

void QDECL G_Printf( const char *fmt, ... ) { (void)fmt; notes++; }

void Q_strncpyz( char *dest, const char *src, int destsize ) {
	CHECK( dest && src && destsize >= 1 );
	strncpy( dest, src, destsize - 1 );
	dest[destsize - 1] = '\0';
}

int QDECL Com_sprintf( char *dest, int size, const char *fmt, ... ) {
	va_list	args;
	int		len;

	va_start( args, fmt );
	len = vsnprintf( dest, size, fmt, args );
	va_end( args );
	CHECK( len >= 0 && len < size );
	return len;
}

static const char *Expand( const playerState_t *ps, qboolean spectator, const char *input, qboolean *complete ) {
	static unsigned char guarded[16 + MAX_SAY_TEXT + 16];
	qboolean	fit;
	int			i;

	memset( guarded, 0xa5, sizeof( guarded ) );
	fit = G_ExpandChatTokens( ps, spectator, input, (char *)guarded + 16, MAX_SAY_TEXT );
	for ( i = 0; i < 16; i++ ) {
		CHECK( guarded[i] == 0xa5 && guarded[16 + MAX_SAY_TEXT + i] == 0xa5 );
	}
	CHECK( memchr( guarded + 16, '\0', MAX_SAY_TEXT ) != NULL );
	if ( complete ) {
		*complete = fit;
	}
	return (const char *)guarded + 16;
}

int main( void ) {
	playerState_t	ps;
	char			text[1024];
	qboolean		complete;
	int				i;

	memset( &ps, 0, sizeof( ps ) );
	ps.stats[STAT_HEALTH] = 87;
	ps.stats[STAT_ARMOR] = 123;
	ps.weapon = WP_ROCKET_LAUNCHER;
	ps.ammo[WP_ROCKET_LAUNCHER] = 25;

	CHECK( !strcmp( Expand( &ps, qfalse, "plain text", &complete ), "plain text" ) && complete );
	CHECK( !strcmp( Expand( &ps, qfalse, "#h/#a", NULL ), "87/123" ) );
	CHECK( !strcmp( Expand( &ps, qfalse, "have #w", NULL ), "have Rocket Launcher [25]" ) );
	CHECK( !strcmp( Expand( &ps, qfalse, "100## #x#", NULL ), "100# x" ) );		// "##", an unknown token, a trailing '#'
	CHECK( !strcmp( Expand( &ps, qfalse, "#H #A #W", NULL ), "H A W" ) );		// tokens are lower case only
	CHECK( !strcmp( Expand( &ps, qfalse, "", &complete ), "" ) && complete );
	CHECK( !strcmp( Expand( &ps, qfalse, NULL, &complete ), "" ) && complete );

	// a spectator's text, and text without a player state, is copied as typed
	CHECK( !strcmp( Expand( &ps, qtrue, "#h #a #w ##", NULL ), "#h #a #w ##" ) );
	CHECK( !strcmp( Expand( NULL, qfalse, "#h", NULL ), "#h" ) );

	// health below one reads "0"; armor is printed as it is
	ps.stats[STAT_HEALTH] = 0;
	CHECK( !strcmp( Expand( &ps, qfalse, "#h", NULL ), "0" ) );
	ps.stats[STAT_HEALTH] = -40;
	ps.stats[STAT_ARMOR] = 0;
	CHECK( !strcmp( Expand( &ps, qfalse, "#h #a", NULL ), "0 0" ) );

	// the gauntlet has no count; the grapple shows its sentinel; no weapon is "None"
	ps.weapon = WP_GAUNTLET;
	CHECK( !strcmp( Expand( &ps, qfalse, "#w", NULL ), "Gauntlet" ) );
	ps.weapon = WP_GRAPPLING_HOOK;
	ps.ammo[WP_GRAPPLING_HOOK] = -1;
	CHECK( !strcmp( Expand( &ps, qfalse, "#w", NULL ), "Grappling Hook [-1]" ) );
	ps.weapon = WP_NONE;
	CHECK( !strcmp( Expand( &ps, qfalse, "#w", NULL ), "None [0]" ) );
	ps.weapon = WP_HEAVY_MACHINEGUN;
	ps.ammo[WP_HEAVY_MACHINEGUN] = 150;
	CHECK( !strcmp( Expand( &ps, qfalse, "#w", NULL ), "Heavy Machinegun [150]" ) );
	CHECK( !strcmp( G_ChatWeaponName( -1 ), "None" ) && !strcmp( G_ChatWeaponName( WP_NUM_WEAPONS ), "None" ) );
	CHECK( !strcmp( G_ChatWeaponName( WP_MACHINEGUN ), "Machine Gun" ) && !strcmp( G_ChatWeaponName( WP_PROX_LAUNCHER ), "Prox Launcher" ) );
	ps.weapon = 99;	// out of every table: named "None", no stray ammo read
	CHECK( !strcmp( Expand( &ps, qfalse, "#w", NULL ), "None [0]" ) );

	// input longer than the say limit: read stops at 149 characters with a note
	memset( text, 'x', sizeof( text ) );
	text[sizeof( text ) - 1] = '\0';
	notes = 0;
	CHECK( strlen( Expand( &ps, qfalse, text, &complete ) ) == MAX_SAY_TEXT - 1 && !complete && notes == 1 );
	text[MAX_SAY_TEXT - 1] = '\0';
	notes = 0;
	CHECK( strlen( Expand( &ps, qfalse, text, &complete ) ) == MAX_SAY_TEXT - 1 && complete && notes == 0 );
	CHECK( strlen( Expand( &ps, qtrue, text, &complete ) ) == MAX_SAY_TEXT - 1 && complete );

	// expansions that outgrow the buffer are cut at its end, never past it
	ps.weapon = WP_ROCKET_LAUNCHER;
	for ( i = 0; i < 70; i++ ) {
		text[i * 2] = '#';
		text[i * 2 + 1] = 'w';
	}
	text[140] = '\0';
	CHECK( strlen( Expand( &ps, qfalse, text, &complete ) ) == MAX_SAY_TEXT - 1 && !complete );
	CHECK( !strncmp( Expand( &ps, qfalse, text, NULL ), "Rocket Launcher [25]Rocket Launcher [25]", 40 ) );

	// degenerate destinations
	CHECK( !G_ExpandChatTokens( &ps, qfalse, "#h", NULL, 10 ) );
	CHECK( !G_ExpandChatTokens( &ps, qfalse, "#h", text, 0 ) );
	text[0] = 'q';
	ps.stats[STAT_HEALTH] = 100;
	CHECK( !G_ExpandChatTokens( &ps, qfalse, "#h", text, 1 ) && text[0] == '\0' );
	CHECK( !G_ExpandChatTokens( &ps, qfalse, "#h", text, 3 ) && !strcmp( text, "10" ) );
	CHECK( G_ExpandChatTokens( &ps, qfalse, "#h", text, 4 ) && !strcmp( text, "100" ) );
	return 0;
}
