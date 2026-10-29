// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


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

	s = va("%i %i %i %i %i %i %i %i %i %i", 
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.loadoutWeapon,
		client->sess.spectateOnly,
		client->sess.queuePosition
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
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	Q_sscanf( s, "%i %i %i %i %i %i %i %i %i %i",
		&sessionTeam,
		&client->sess.spectatorTime,
		&spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,
		&client->sess.loadoutWeapon,
		&client->sess.spectateOnly,
		&client->sess.queuePosition
		);

	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;

	if ( (unsigned)client->sess.sessionTeam >= TEAM_NUM_TEAMS ) {
		client->sess.sessionTeam = TEAM_SPECTATOR;
	}
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


/*
================
G_ClientPrivilege

The admin tier retail reads from access.txt (qagamex86 0x10032460): 0 for
a player.  The access list arrives with milestone 5; until then everyone
is a player.
================
*/
int G_ClientPrivilege( int clientNum ) {
	(void)clientNum;
	return 0;
}
