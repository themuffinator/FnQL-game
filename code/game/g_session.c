// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include <errno.h>
#include <limits.h>


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	// Retail 0x10065880: thirteen signed 32-bit fields, including the
	// selected weapon before the duel record and the reserved final word.
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i",
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.loadoutWeapon,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->pers.privileges,
		client->sess.spectateOnly,
		client->sess.queuePosition,
		client->sess.muted,
		client->sess.sessionReservedTail
		);

	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );
}


/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadClientSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;
	const char *cursor;
	char *end;
	int fields[13], count = 0;
	long value;
	clientSession_t decoded;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	// Decode atomically, with explicit 32-bit limits even on LP64 hosts.
	// Retail's sscanf (0x10065930) leaves malformed input partially read;
	// use a safe spectator session instead of uninitialized enum values.
	memset( &decoded, 0, sizeof(decoded) );
	decoded.sessionTeam = TEAM_SPECTATOR;
	decoded.spectatorState = SPECTATOR_FREE;
	cursor = s;
	while ( *cursor ) {
		while ( isspace( (unsigned char)*cursor ) ) ++cursor;
		if ( !*cursor ) break;
		if ( count == ARRAY_LEN(fields) ) goto invalid;
		errno = 0;
		value = strtol( cursor, &end, count == 1 ? 10 : 0 );
		if ( end == cursor || errno == ERANGE || value < INT_MIN || value > INT_MAX
			|| (*end && !isspace( (unsigned char)*end )) ) goto invalid;
		fields[count++] = (int)value;
		cursor = end;
	}
	if ( count < 7 ) goto invalid;
	decoded.sessionTeam = (unsigned)fields[0] < TEAM_NUM_TEAMS ? (team_t)fields[0] : TEAM_SPECTATOR;
	decoded.spectatorTime = fields[1];
	decoded.spectatorState = (unsigned)fields[2] <= SPECTATOR_SCOREBOARD ? (spectatorState_t)fields[2] : SPECTATOR_FREE;
	decoded.spectatorClient = fields[3];
	if ( count == 13 ) {
		decoded.loadoutWeapon = fields[4];
		decoded.wins = fields[5];
		decoded.losses = fields[6];
		decoded.teamLeader = (qboolean)fields[7];
		client->pers.privileges = fields[8];
		decoded.spectateOnly = fields[9];
		decoded.queuePosition = fields[10];
		decoded.muted = (qboolean)fields[11];
		decoded.sessionReservedTail = fields[12];
	} else {
		// Previous FnQL-game releases wrote a seven-field Q3 prefix, then
		// loadout/so/pq, then mute/privilege. Read it once; always write retail.
		decoded.wins = fields[4];
		decoded.losses = fields[5];
		decoded.teamLeader = (qboolean)fields[6];
		if ( count > 7 ) decoded.loadoutWeapon = fields[7];
		if ( count > 8 ) decoded.spectateOnly = fields[8];
		if ( count > 9 ) decoded.queuePosition = fields[9];
		if ( count > 10 ) decoded.muted = (qboolean)fields[10];
		if ( count > 11 ) client->pers.privileges = fields[11];
	}
	// A session is local state, but a bad loadout must never become an
	// out-of-range shift or ammo index when the next spawn consumes it.
	if ( (unsigned)decoded.loadoutWeapon >= WP_NUM_WEAPONS ) decoded.loadoutWeapon = 0;
invalid:
	client->sess = decoded;
}


/*
================
G_ClearClientSessionData
================
*/
void G_ClearClientSessionData( gclient_t *client )
{
	const char	*var;

	var = va( "session%i", client - level.clients );
	trap_Cvar_Set( var, "" );
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, const char *team, qboolean isBot ) {
	clientSession_t	*sess;

	sess = &client->sess;

	// Quake Live (retail qagamex86 0x10065a40): everyone, bots included,
	// starts as a spectator, except that g_teamAutoJoin places new players
	// in team games.  Players join with the team command; bots send theirs
	// once their AI is set up.
	if ( g_gametype.integer >= GT_TEAM && g_teamAutoJoin.integer ) {
		sess->sessionTeam = PickTeam( -1 );
		BroadcastTeamChange( client, -1 );
	} else {
		sess->sessionTeam = TEAM_SPECTATOR;
	}

	sess->spectatorState = SPECTATOR_FREE;
	// the duel queue orders spectators by when they arrived, in wall-clock
	// seconds; a duel spectator starts out spectate-only
	sess->spectatorTime = trap_RealTime( NULL );
	sess->spectateOnly = g_gametype.integer == GT_TOURNAMENT;
	sess->queuePosition = 0;
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( !*s || g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}


/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
