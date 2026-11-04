/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "../code/game/g_local.h"
#include <limits.h>

#define CHECK( x )                                                                                 \
	do {                                                                                           \
		if ( !( x ) ) {                                                                            \
			fprintf( stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x );                              \
			exit( 1 );                                                                             \
		}                                                                                          \
	} while ( 0 )

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_voteFlags, g_gametype, g_teamSizeMin, g_voteDelay, g_training;
vmCvar_t g_allowVote, g_voteLimit, g_allowSpecVote, g_allowVoteMidGame;
static gclient_t clients[8];
static const char *args[4];
static char config[12][MAX_STRING_CHARS], messages[8192], console[2048];
static char changedCvar[64], changedValue[64], nextmap[MAX_STRING_CHARS];
static int closed, opened, factoryQueries, shuffled;

void QDECL Com_Error( int code, const char *format, ... ) {
	CHECK( 0 );
}
void QDECL Com_Printf( const char *format, ... ) {
	CHECK( 0 );
}

void trap_Argv( int index, char *buffer, int capacity ) {
	Q_strncpyz( buffer, index < 4 && args[index] ? args[index] : "", capacity );
}

void trap_SendServerCommand( int slot, const char *text ) {
	CHECK( slot >= -1 && slot < level.maxclients );
	CHECK( strlen( messages ) + strlen( text ) + 1 < sizeof( messages ) );
	strcat( messages, text );
}

void trap_SetConfigstring( int index, const char *text ) {
	if ( index == CS_INTERMISSION_EXIT )
		return;
	CHECK( index >= CS_VOTE_TIME && index <= CS_VOTE_NO );
	Q_strncpyz( config[index], text, sizeof( config[index] ) );
}

void trap_Cvar_VariableStringBuffer( const char *name, char *text, int capacity ) {
	CHECK( !strcmp( name, "nextmap" ) );
	Q_strncpyz( text, nextmap, capacity );
}

void trap_Cvar_Set( const char *name, const char *value ) {
	Q_strncpyz( changedCvar, name, sizeof( changedCvar ) );
	Q_strncpyz( changedValue, value, sizeof( changedValue ) );
}

void trap_SendConsoleCommand( int when, const char *text ) {
	CHECK( when == EXEC_APPEND );
	CHECK( strlen( console ) + strlen( text ) + 1 < sizeof( console ) );
	strcat( console, text );
}

int trap_FS_FOpenFile( const char *path, fileHandle_t *file, fsMode_t mode ) {
	CHECK( mode == FS_READ );
	++opened;
	*file = !strcmp( path, "maps/campgrounds.bsp" ) ? 7 : 0;
	return *file ? 100 : -1;
}

void trap_FS_FCloseFile( fileHandle_t file ) {
	CHECK( file == 7 );
	++closed;
}
qboolean trap_FactoryExists( const char *name ) {
	++factoryQueries;
	return !strcmp( name, "custom_ffa" );
}
void G_ShuffleTeams( void ) {
	++shuffled;
}
qboolean G_ReadyToStart( void ) {
	return level.warmupTime >= 0;
}

int TeamCount( int ignore, int team ) {
	int i, count = 0;
	for ( i = 0; i < level.maxclients; ++i )
		if ( i != ignore && clients[i].pers.connected == CON_CONNECTED &&
			 clients[i].sess.sessionTeam == team )
			++count;
	return count;
}

void G_ClearVote( void ) {
	int i;
	level.voteTime = level.voteExecuteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
	trap_SetConfigstring( CS_VOTE_STRING, "" );
	for ( i = 0; i < level.maxclients; ++i )
		clients[i].pers.voted = 0;
}

static void Reset( int players ) {
	int i;
	memset( &level, 0, sizeof( level ) );
	memset( clients, 0, sizeof( clients ) );
	memset( g_entities, 0, sizeof( g_entities ) );
	memset( config, 0, sizeof( config ) );
	for ( i = 0; i < ARRAY_LEN( args ); ++i )
		args[i] = NULL;
	messages[0] = console[0] = changedCvar[0] = changedValue[0] = nextmap[0] = 0;
	opened = closed = factoryQueries = shuffled = 0;
	g_allowVote.integer = 1;
	g_teamSizeMin.integer = 1;
	g_gametype.integer = GT_FFA;
	g_voteFlags.integer = g_voteDelay.integer = g_training.integer = g_voteLimit.integer = 0;
	g_allowSpecVote.integer = g_allowVoteMidGame.integer = 0;
	level.time = 1000;
	level.warmupTime = -1;
	level.maxclients = ARRAY_LEN( clients );
	level.clients = clients;
	for ( i = 0; i < level.maxclients; ++i ) {
		g_entities[i].client = &clients[i];
		clients[i].pers.connected = i < players ? CON_CONNECTED : CON_DISCONNECTED;
		clients[i].sess.sessionTeam = TEAM_FREE;
		Com_sprintf( clients[i].pers.netname, MAX_NETNAME, "Player%i", i );
	}
}

static void Call( int slot, const char *command, const char *arg, const char *factory ) {
	args[0] = "callvote";
	args[1] = command;
	args[2] = arg;
	args[3] = factory;
	Cmd_CallVote_f( &g_entities[slot] );
}

static void Ballot( int slot, const char *answer ) {
	args[1] = answer;
	G_CastVote( &g_entities[slot] );
}

static void Finish( void ) {
	G_CheckVote();
	CHECK( !level.voteTime && level.voteExecuteTime == 4000 );
	level.time = 4000;
	G_CheckVote();
	CHECK( !console[0] && !changedCvar[0] );
	level.time = 4001;
	G_CheckVote();
	CHECK( !level.voteExecuteTime );
}

static void TestLifecycle( void ) {
	Reset( 3 );
	Call( 0, "timelimit", "12", NULL );
	CHECK( !strcmp( config[CS_VOTE_STRING], "timelimit \"12\"" ) );
	CHECK( !( clients[0].ps.eFlags & EF_VOTED ) );
	Ballot( 1, "no" );
	G_CheckVote();
	CHECK( level.voteTime ); // one no out of three is not a majority
	Ballot( 2, "Y" );
	Finish();
	CHECK( !strcmp( console, "timelimit \"12\"\n" ) );
	CHECK( !config[CS_VOTE_STRING][0] && !config[CS_VOTE_TIME][0] );
	G_CheckVote();
	CHECK( !strcmp( console, "timelimit \"12\"\n" ) );

	Reset( 2 );
	Call( 0, "fraglimit", "15", NULL );
	Ballot( 1, "no" );
	G_CheckVote();
	CHECK( !level.voteTime && !level.voteExecuteTime && strstr( messages, "Vote failed." ) );
	Reset( 3 );
	Call( 0, "fraglimit", "15", NULL );
	level.time += 29999;
	G_CheckVote();
	CHECK( level.voteTime );
	++level.time;
	G_CheckVote();
	CHECK( !level.voteTime && strstr( messages, "Vote failed." ) );
	Reset( 3 );
	Call( 0, "fraglimit", "15", NULL );
	level.warmupTime = 2000;
	G_CheckVote();
	CHECK( !level.voteTime && strstr( messages, "Voting time has expired." ) );

	Reset( 2 );
	Call( 0, "timelimit", "12", NULL );
	Ballot( 1, "yes" );
	G_CheckVote();
	Call( 0, "fraglimit", "30", NULL );
	CHECK( !strcmp( level.voteString, "timelimit \"12\"" ) );
	CHECK( strstr( messages, "A vote is being executed." ) );
	level.time = 4001;
	G_CheckVote();
	CHECK( !strcmp( console, "timelimit \"12\"\n" ) );
}

static void TestElectorate( void ) {
	Reset( 4 );
	clients[3].sess.sessionTeam = TEAM_SPECTATOR;
	g_entities[2].r.svFlags = SVF_BOT;
	Call( 0, "timelimit", "12", NULL );
	Ballot( 3, "yes" );
	Ballot( 2, "yes" );
	CHECK( level.voteYes == 1 );
	CHECK( strstr( messages, "You may not participate in this vote." ) );
	clients[4].pers.connected = CON_CONNECTED;
	Ballot( 4, "yes" );
	CHECK( level.voteYes == 1 );
	Ballot( 0, "yes" );
	CHECK( strstr( messages, "Vote already cast." ) );
	clients[0].sess.sessionTeam = TEAM_SPECTATOR; // ballot stays latched across team changes
	Ballot( 1, "yes" );
	Finish();
	CHECK( console[0] );

	Reset( 3 );
	Call( 0, "timelimit", "12", NULL );
	clients[1].pers.connected = clients[2].pers.connected = CON_DISCONNECTED;
	G_RevertVote( &clients[1] );
	G_RevertVote( &clients[2] );
	Finish();
	CHECK( console[0] );
	Reset( 2 );
	g_allowSpecVote.integer = 1;
	clients[1].sess.sessionTeam = TEAM_SPECTATOR;
	Call( 1, "timelimit", "12", NULL );
	Ballot( 0, "yes" );
	Finish();
	CHECK( console[0] );
}

static void TestPrivileges( void ) {
	Reset( 4 );
	clients[3].pers.privileges = 1;
	clients[3].sess.sessionTeam = TEAM_SPECTATOR;
	Call( 0, "timelimit", "12", NULL );
	Ballot( 3, "yes" );
	Finish();
	CHECK( strstr( messages, "Player3 passed the vote." ) );
	Reset( 4 );
	clients[3].pers.privileges = 1;
	Call( 0, "timelimit", "12", NULL );
	Ballot( 3, "no" );
	G_CheckVote();
	CHECK( !level.voteTime && !level.voteExecuteTime &&
		   strstr( messages, "Player3 vetoed the vote." ) );
	Reset( 2 );
	clients[0].pers.privileges = 1;
	clients[0].sess.sessionTeam = TEAM_SPECTATOR;
	g_allowVote.integer = 0;
	g_voteFlags.integer = 32767;
	g_voteLimit.integer = 1;
	clients[0].pers.voteCount = 5;
	Call( 0, "timelimit", "12", NULL );
	CHECK( level.voteTime );
	Reset( 2 );
	clients[1].pers.privileges = 2;
	Call( 0, "clientkick", "1", NULL );
	CHECK( !level.voteTime && strstr( messages, "server admin" ) );
}

static void TestArguments( void ) {
	static const char *bad[] = { "12;quit", "12\nquit", "12\rquit", "12\"",
								 "12\\",	"12%",		"../map",	"\xff" };
	char longToken[1100];
	int i;
	for ( i = 0; i < ARRAY_LEN( bad ); ++i ) {
		Reset( 2 );
		Call( 0, "timelimit", bad[i], NULL );
		CHECK( !level.voteTime && !console[0] && strstr( messages, "Invalid vote string." ) );
	}
	memset( longToken, 'a', sizeof( longToken ) );
	longToken[sizeof( longToken ) - 1] = 0;
	Reset( 2 );
	Call( 0, "map", longToken, NULL );
	CHECK( !opened && !level.voteTime );
	Reset( 2 );
	Call( 0, "map", "campgrounds", longToken );
	CHECK( !opened && !factoryQueries );
	Reset( 2 );
	Call( 0, "timelimit", "2147483648", NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	Call( 0, "random", "0", NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	Call( 0, "random", "101", NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	Call( 0, "map", "absent", "custom_ffa" );
	CHECK( opened == 1 && closed == 0 && factoryQueries == 0 );
	Reset( 2 );
	Call( 0, "map", "campgrounds", "absent" );
	CHECK( opened == 1 && closed == 1 && factoryQueries == 1 && !level.voteTime );
	Reset( 2 );
	Call( 0, "map", "campgrounds", "custom_ffa" );
	CHECK( closed == 1 && factoryQueries == 1 && level.voteTime );
	CHECK( !strcmp( level.voteString, "map campgrounds custom_ffa" ) );
	Reset( 2 );
	Call( 0, "nextmap", NULL, NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	strcpy( nextmap, "map_restart 0" );
	Call( 0, "nextmap", NULL, NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	strcpy( nextmap, "map campgrounds custom_ffa" );
	Call( 0, "nextmap", NULL, NULL );
	CHECK( level.voteTime && !strcmp( level.voteString, nextmap ) );
}

static void TestPolicyAndActions( void ) {
	static const char *commands[] = { "map",		"map_restart", "nextmap",	 "kick",
									  "clientkick", "timelimit",   "fraglimit",	 "shuffle",
									  "teamsize",	"cointoss",	   "random",	 "loadouts",
									  "ammo",		"timers",	   "weaprespawn" };
	int i;
	for ( i = 0; i < ARRAY_LEN( commands ); ++i ) {
		Reset( 2 );
		g_voteFlags.integer = 32767;
		Call( 0, commands[i], "1", NULL );
		CHECK( !level.voteTime && strstr( messages, "disabled on this server" ) );
	}
	Reset( 2 );
	g_voteDelay.integer = 2750;
	Call( 0, "timelimit", "12", NULL );
	CHECK( !level.voteTime && strstr( messages, "1.8 seconds." ) );
	Reset( 2 );
	level.warmupTime = 0;
	Call( 0, "timelimit", "12", NULL );
	CHECK( !level.voteTime );
	g_allowVoteMidGame.integer = 1;
	Call( 0, "timelimit", "12", NULL );
	CHECK( level.voteTime );
	Reset( 2 );
	g_voteLimit.integer = 1;
	clients[0].pers.voteCount = 1;
	Call( 0, "timelimit", "12", NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	g_training.integer = 1;
	clients[0].pers.privileges = 2;
	Call( 0, "timelimit", "12", NULL );
	CHECK( !level.voteTime );
	Reset( 2 );
	Call( 0, "loadouts", "on", NULL );
	Ballot( 1, "yes" );
	Finish();
	CHECK( !strcmp( changedCvar, "g_loadout" ) && !strcmp( changedValue, "1" ) );
	Reset( 2 );
	Call( 0, "timers", "off", NULL );
	Ballot( 1, "yes" );
	Finish();
	CHECK( !strcmp( changedCvar, "g_itemTimers" ) && !strcmp( changedValue, "0" ) );
	Reset( 2 );
	Call( 0, "ammo", "weap", NULL );
	Ballot( 1, "yes" );
	Finish();
	CHECK( !strcmp( changedCvar, "g_ammoPack" ) && !strcmp( console, "map_restart 0\n" ) );
	Reset( 2 );
	Call( 0, "weaprespawn", "7", NULL );
	Ballot( 1, "yes" );
	Finish();
	CHECK( !strcmp( changedCvar, "g_weaponRespawn" ) && !strcmp( changedValue, "7" ) );
	Reset( 2 );
	Call( 0, "clientkick", "1", NULL );
	CHECK( !strcmp( level.voteDisplayString, "kick Player1" ) );
	Ballot( 1, "yes" );
	Finish();
	CHECK( !strcmp( console, "clientkick 1\n" ) );
}

static void TestShuffle( void ) {
	Reset( 2 );
	g_gametype.integer = GT_TEAM;
	clients[0].sess.sessionTeam = TEAM_RED;
	clients[1].sess.sessionTeam = TEAM_BLUE;
	Call( 0, "shuffle", NULL, NULL );
	Ballot( 1, "yes" );
	Finish();
	CHECK( shuffled == 1 && !console[0] );
}

int main( void ) {
	TestLifecycle();
	TestElectorate();
	TestPrivileges();
	TestArguments();
	TestPolicyAndActions();
	TestShuffle();
	puts( "voting policy, ballots, validation and execution OK" );
	return 0;
}
