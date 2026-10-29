// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include "../../version/fnql_game_version.h"

level_locals_t	level;

typedef struct {
	vmCvar_t	*vmCvar;
	const char	*cvarName;
	const char	*defaultString;
	int			cvarFlags;
	int			modificationCount;	// for tracking changes
	qboolean	trackChange;		// track this variable, and announce if changed
	qboolean	teamShader;			// track and if changed, update shader state
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

#define DECLARE_G_CVAR
	#include "g_cvar.h"
#undef DECLARE_G_CVAR

static cvarTable_t gameCvarTable[] = {

	// retail prints gamename and gamedate at init but registers neither
	// FnQL-game identity for capable peers and server browsers.  Retail
	// clients ignore unknown serverinfo keys; extensions are negotiated
	// separately (docs/COMPATIBILITY.md).
	{ NULL, "fnql_game", FNQL_GAME_VERSION_STRING, CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

#define G_CVAR_LIST
	#include "g_cvar.h"
#undef G_CVAR_LIST

};


static void G_InitGame( int levelTime, int randomSeed, int restart );
static void G_RunFrame( int levelTime );
static void G_ShutdownGame( int restart );
static void CheckExitRules( void );
static void G_WarmupEnd( void );
static qboolean G_ShouldForfeit( void );
static void SendScoreboardMessageToAllClients( void );

// extension interface
#ifdef Q3_VM
qboolean (*trap_GetValue)( char *value, int valueSize, const char *key );
#else
int dll_com_trapGetValue;
#endif

int	svf_self_portal2;

/*
================
vmMain

Central game dispatcher.  FnQL-game modules are entered through the retail
native export table (g_native.c), whose thunks all route here so per-call
bookkeeping stays in one place.  It is not exported: a module that exports
vmMain is treated by the engine as a classic Quake III module.
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2 ) {
	int ret;

	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		ret = 0;
		break;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		ret = 0;
		break;
	case GAME_CLIENT_CONNECT:
		ret = (intptr_t)ClientConnect( arg0, arg1, arg2 );
		break;
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		ret = 0;
		break;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		ret = 0;
		break;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		ret = 0;
		break;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0 );
		ret = 0;
		break;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		ret = 0;
		break;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		ret = 0;
		break;
	case GAME_CONSOLE_COMMAND:
		ret = ConsoleCommand();
		break;
	case BOTAI_START_FRAME:
		ret = BotAIStartFrame( arg0 );
		break;
	default:
		ret = -1;
	}


	return ret;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[BIG_INFO_STRING];
	int			len;

	va_start( argptr, fmt );
	len = Q_vsprintf( text, fmt, argptr );
	va_end( argptr );

	text[4095] = '\0'; // truncate to 1.32b/c max print buffer size

	trap_Print( text );
}


void G_BroadcastServerCommand( int ignoreClient, const char *command ) {
	int i;
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( i == ignoreClient )
			continue;
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			trap_SendServerCommand( i, command );
		}
	}
}


#ifdef FNQL_TRACE_RAND
#undef rand
int G_TracedRand( const char *file, int line ) {
	static unsigned	draws;
	const int		value = rand();

	G_Printf( "fnql-game: rand #%i = %d at %d from %s:%d\n", (int)draws++, value, level.time, file, line );
	return value;
}
#define rand() G_TracedRand( __FILE__, __LINE__ )
#endif


void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start( argptr, fmt );
	Q_vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=MAX_CLIENTS, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}


void G_RemapTeamShaders( void ) {
#ifdef MISSIONPACK
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f); 
	AddRemap("textures/ctf2/redteam02", string, f); 
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f); 
	AddRemap("textures/ctf2/blueteam02", string, f); 
	// Quake III published remaps in CS_SHADERSTATE (24), which is a model
	// slot in Quake Live; retail Quake Live has no shader remap channel.
#endif
}


/*
=================
G_UpdatePmoveSettings

Rebuilds level.pmoveSettings from the pmove_* and weapon_reload_* cvars.
The values go through infostrings keyed like CS_PMOVE_SETTINGS and
CS_WEAPON_RELOAD, so the server parses exactly what clients parse.
=================
*/
static void G_SettingsInfo( int group, char *info ) {
	const cvarTable_t *cv;
	int i;

	info[0] = '\0';
	for ( i = 0, cv = gameCvarTable ; i < ARRAY_LEN( gameCvarTable ) ; i++, cv++ ) {
		if ( cv->vmCvar && ( cv->cvarFlags & group ) ) {
			Info_SetValueForKey( info, cv->cvarName, cv->vmCvar->string );
		}
	}
}

static void G_UpdatePmoveSettings( void ) {
	char info[MAX_INFO_STRING];

	G_SettingsInfo( CVAR_QL_PMOVE_SETTINGS, info );
	BG_PmoveSettingsFromInfo( info, &level.pmoveSettings );
	G_SettingsInfo( CVAR_QL_WEAPON_RELOAD, info );
	BG_WeaponReloadFromInfo( info, &level.pmoveSettings );
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	qboolean remapped = qfalse;
	cvarTable_t *cv;
	int i;

	for ( i = 0, cv = gameCvarTable ; i < ARRAY_LEN( gameCvarTable ) ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update( &g_gametype );
	}

	level.warmupModificationCount = g_warmup.modificationCount;

	// force g_doWarmup to 1
	trap_Cvar_Register( NULL, "g_doWarmup", "1", CVAR_ROM );
	trap_Cvar_Set( "g_doWarmup", "1" );
}


/*
=================
G_UpdateCvars
=================
*/
static void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;
	qboolean pmoveChanged = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < ARRAY_LEN( gameCvarTable ) ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->cvarFlags & ( CVAR_QL_PMOVE_SETTINGS | CVAR_QL_WEAPON_RELOAD ) ) {
					pmoveChanged = qtrue;
				}

				if ( cv->trackChange ) {
					G_BroadcastServerCommand( -1, va("print \"Server: %s changed to %s\n\"", 
						cv->cvarName, cv->vmCvar->string ) );
				}

				if (cv->teamShader) {
					remapped = qtrue;
				}
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	if ( pmoveChanged ) {
		G_UpdatePmoveSettings();
	}
}


static void G_LocateSpawnSpots( void ) 
{
	static const vec3_t playerMins = { -12, -12, -24 };
	static const vec3_t playerMaxs = { 12,  12,  32 };
	vec3_t spawn_origin;
	vec3_t spawn_ground;
	gentity_t *ent;
	trace_t tr;
	int i, n;

	level.spawnSpots[ SPAWN_SPOT_INTERMISSION ] = NULL;

	// locate all spawn spots
	n = 0;
	ent = g_entities + MAX_CLIENTS;
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES - 1; i++, ent++ ) {
		
		if ( !ent->inuse || !ent->classname )
			continue;

		// intermission/ffa spots
		if ( !Q_stricmpn( ent->classname, "info_player_", 12 ) ) {
			if ( !Q_stricmp( ent->classname+12, "intermission" ) ) {
				if ( level.spawnSpots[ SPAWN_SPOT_INTERMISSION ] == NULL ) {
					level.spawnSpots[ SPAWN_SPOT_INTERMISSION ] = ent; // put in the last slot
					ent->fteam = TEAM_FREE;
				}
				continue;
			}
			if ( !Q_stricmp( ent->classname+12, "deathmatch" ) ) {
				level.spawnSpots[n] = ent; n++;
				level.numSpawnSpotsFFA++;
				ent->fteam = TEAM_FREE;
				ent->count = 1;
				continue;
			}
			continue;
		}

		// team spawn spots
		if ( !Q_stricmpn( ent->classname, "team_CTF_", 9 ) ) {
			if ( !Q_stricmp( ent->classname+9, "redspawn" ) ) {
				level.spawnSpots[n] = ent; n++;
				level.numSpawnSpotsTeam++;
				ent->fteam = TEAM_RED;
				ent->count = 1; // means its not initial spawn point
				continue;
			}
			if ( !Q_stricmp( ent->classname+9, "bluespawn" ) ) {
				level.spawnSpots[n] = ent; n++;
				level.numSpawnSpotsTeam++;
				ent->fteam = TEAM_BLUE;
				ent->count = 1;
				continue;
			}
			// base spawn spots
			if ( !Q_stricmp( ent->classname+9, "redplayer" ) ) {
				level.spawnSpots[n] = ent; n++;
				level.numSpawnSpotsTeam++;
				ent->fteam = TEAM_RED;
				ent->count = 0;
				continue;
			}
			if ( !Q_stricmp( ent->classname+9, "blueplayer" ) ) {
				level.spawnSpots[n] = ent; n++;
				level.numSpawnSpotsTeam++;
				ent->fteam = TEAM_BLUE;
				ent->count = 0;
				continue;
			}
		}
	}
	level.numSpawnSpots = n;

	// try to lower spawn spots if possible to minimize nasty bouncing
	for ( i = 0; i < level.numSpawnSpots; i++ ) {
		ent = level.spawnSpots[i];
		
		n = trap_PointContents( ent->s.origin, ent->s.number ) & ~(CONTENTS_FOG | CONTENTS_TRANSLUCENT);
		if ( n != 0 ) {
			continue;
		}

		VectorCopy( ent->s.origin, spawn_origin );
		VectorCopy( ent->s.origin, spawn_ground );
		spawn_ground[2] -= 32.0f;
		trap_Trace( &tr, spawn_origin, playerMins, playerMaxs, spawn_ground, ent->s.number, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );
		if ( tr.fraction != 1.0f && tr.entityNum == ENTITYNUM_WORLD ) {
			ent->s.origin[2] = (int) (tr.endpos[2] + 0.999f);
			ent->s.groundEntityNum = tr.entityNum;
		} else {
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}
}


/*
============
G_InitGame

============
*/
static void G_InitGame( int levelTime, int randomSeed, int restart ) {
	char value[ MAX_CVAR_VALUE_STRING ];
	int	i;

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);

	// extension interface
	trap_Cvar_VariableStringBuffer( "//trap_GetValue", value, sizeof( value ) );
	if ( value[0] ) {
#ifdef Q3_VM
		trap_GetValue = (void*)~atoi( value );
#else
		dll_com_trapGetValue = atoi( value );
#endif
		if ( trap_GetValue( value, sizeof( value ), "SVF_SELF_PORTAL2_Q3E" ) ) {
			svf_self_portal2 = atoi( value );
		} else {
			svf_self_portal2 = 0;
		}
	}

	srand( randomSeed );

	G_RegisterCvars();
	G_UpdateSettingsConfigstrings();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;

	level.startTime = levelTime;

	level.previousTime = levelTime;
	level.msec = FRAMETIME;

	// after the level reset, which would otherwise zero the settings
	G_UpdatePmoveSettings();

	// retail records the wall-clock start in serverinfo; the real-time import
	// keeps it pinned under FnQL's lockstep mode
	{
		qtime_t	now;
		trap_Cvar_Set( "g_levelStartTime", va( "%i", trap_RealTime( &now ) ) );
	}

	// retail registers these three first, so they own sound indexes 1..3
	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime
	G_SoundIndex( "sound/player/gurp1.wav" );
	G_SoundIndex( "sound/player/gurp2.wav" );

	if ( !G_IsSinglePlayer() && g_log.string[0] ) {
		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
		}
		if ( level.logFile == FS_INVALID_HANDLE ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
		g_entities[ i ].classname = "clientslot";
	}

	// a new match GUID for every map, kept across map_restart
	if ( !g_restarted.integer ) {
		char	guid[64];

		trap_GenerateMatchGuid( guid, sizeof( guid ) );
		trap_SetConfigstring( CS_MATCH_GUID, guid );
	}

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( g_gametype.integer >= GT_TEAM ) {
		G_CheckTeamItems();
	}

	SaveRegisteredItems();

	G_UpdateTimeoutConfigstrings();

	G_LocateSpawnSpots();

	// retail finds the intermission view at init, drawing its target pick
	// before the first frame (qagamex86 0x10054f00); the cached spawn spots
	// above are where it looks
	FindIntermissionPoint();

	// CTF's item timers say which half an item is on (retail qagamex86 0x10055475)
	if ( g_gametype.integer == GT_CTF ) {
		G_InitItemSides();
	}

	G_Printf ("-----------------------------------\n");

	if( G_IsSinglePlayer() || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	// don't forget to reset times
	trap_SetConfigstring( CS_INTERMISSION, "" );

	if ( !G_IsSinglePlayer() ) {
		// launch rotation system on first map load
		if ( trap_Cvar_VariableIntegerValue( SV_ROTATION ) == 0 ) {
			trap_Cvar_Set( SV_ROTATION, "1" );
			level.denyMapRestart = qtrue;
			ParseMapRotation();
		}
	}
}


/*
=================
G_ShutdownGame
=================
*/
static void G_ShutdownGame( int restart ) 
{
	G_Printf ("==== ShutdownGame ====\n");

	if ( level.logFile != FS_INVALID_HANDLE ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
		level.logFile = FS_INVALID_HANDLE;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error( int level, const char *fmt, ... ) {
	va_list		argptr;
	char		text[4096];

	va_start( argptr, fmt );
	Q_vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}


void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[4096];

	va_start( argptr, fmt );
	Q_vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Print( text );
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

Retail qagamex86 0x10055780: while fewer than two players are in a duel,
the next queued spectator joins, the match goes back to PRE_GAME and
everyone's ready flag is cleared.
=============
*/
void AddTournamentPlayer( void ) {
	gclient_t	*next;
	int			i, players = 0;

	if ( g_gametype.integer != GT_TOURNAMENT || level.intermissionQueued || level.intermissiontime ) {
		return;
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED
			&& level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
			players++;
		}
	}
	if ( players >= 2 ) {
		return;
	}

	next = G_NextInLine();
	if ( !next ) {
		return;
	}

	level.warmupTime = -1;
	trap_SetConfigstring( CS_WARMUP, va( "\\time\\%i", -1 ) );
	trap_Cvar_Set( "g_gameState", "PRE_GAME" );

	SetTeam( &g_entities[ next - level.clients ], "f" );

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( !g_entities[i].inuse ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_CONNECTED && level.clients[i].pers.readyToPlay == 1 ) {
			level.clients[i].pers.readyToPlay = 0;
		}
		ClientUserinfoChanged( i );
	}
}


/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}


/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}


/*
=======================
AdjustTournamentScores

Retail qagamex86 0x10055950, at the intermission: the leading duelist
still in the game wins, any other loses, and their player keys go out.
=======================
*/
void AdjustTournamentScores( void ) {
	gclient_t	*cl;
	int			i;

	for ( i = 0; i < level.numPlayingClients; i++ ) {
		cl = &level.clients[level.sortedClients[i]];
		if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != TEAM_FREE ) {
			continue;
		}
		if ( i == 0 ) {
			cl->sess.wins++;
		} else {
			cl->sess.losses++;
		}
		ClientUserinfoChanged( level.sortedClients[i] );
	}
}


/*
=============
SortRanks
=============
*/
static int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last (retail qagamex86 0x10055eb0 tests only
	// the followed client)
	if ( ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorClient < 0 ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorTime > cb->sess.spectatorTime ) {
			return -1;
		}
		if ( ca->sess.spectatorTime < cb->sess.spectatorTime ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}


/*
==================
G_IsTeamGametype / G_IsRoundBasedGametype

Retail qagamex86 0x1002c9e0 and 0x1002c9a0.  Red rover keeps per-player
scores although it has two teams.
==================
*/
qboolean G_IsTeamGametype( int gametype ) {
	return !( gametype >= GT_FFA && ( gametype <= GT_RACE || gametype == GT_RED_ROVER ) );
}

qboolean G_IsRoundBasedGametype( int gametype ) {
	return gametype == GT_CLAN_ARENA || gametype == GT_FREEZE
		|| gametype == GT_ATTACK_DEFEND || gametype == GT_RED_ROVER;
}


/*
==================
G_RoundInProgress

Retail level +0x2740 (0x105df580) at 3 or more: a round of a round-based
mode is being played.  The rounds arrive with milestone 6.
==================
*/
qboolean G_RoundInProgress( void ) {
	return qfalse;
}


/*
==================
G_ShouldForfeit

Retail qagamex86 0x10057cd0 (and inline in CalculateRanks, 0x1005612b):
past the match's first second, outside a timeout and the intermission, a
match being played (a round of a round-based mode) on other than a
training map is forfeited once a duel or free-for-all (or red rover) is
down to one player, or a team game has an empty team.
==================
*/
static qboolean G_ShouldForfeit( void ) {
	const int	gametype = g_gametype.integer;

	if ( level.timeoutStartTime ) {
		return qfalse;
	}
	if ( ( level.time - level.startTime ) / 1000 <= 1 || level.intermissionQueued || level.intermissiontime ) {
		return qfalse;
	}
	if ( G_IsRoundBasedGametype( gametype ) ? !G_RoundInProgress() : level.warmupTime != 0 ) {
		return qfalse;
	}
	if ( level.trainingMap ) {
		return qfalse;
	}
	if ( gametype < GT_TEAM || gametype == GT_RED_ROVER ) {
		if ( level.numPlayingClients < 2 ) {
			return qtrue;
		}
	}
	return G_IsTeamGametype( gametype )
		&& ( TeamCount( -1, TEAM_RED ) < 1 || TeamCount( -1, TEAM_BLUE ) < 1 );
}


/*
==================
G_Forfeit

Retail qagamex86 0x10057f10: a duelist who is gone or spectating, or an
empty team, scores -999, and the match ends.
==================
*/
void G_Forfeit( void ) {
	const int	gametype = g_gametype.integer;
	gclient_t	*cl;
	int			i;

	if ( gametype == GT_TOURNAMENT ) {
		for ( i = 0; i < 2; i++ ) {
			cl = &level.clients[level.sortedClients[i]];
			if ( cl->pers.connected == CON_DISCONNECTED || cl->sess.sessionTeam != TEAM_FREE ) {
				cl->ps.persistant[PERS_SCORE] = -999;
			}
		}
	} else if ( G_IsTeamGametype( gametype ) ) {
		if ( TeamCount( -1, TEAM_RED ) < 1 ) {
			level.teamScores[TEAM_RED] = -999;
			trap_SetConfigstring( CS_SCORES1, va( "%i", level.teamScores[TEAM_RED] ) );
		}
		if ( TeamCount( -1, TEAM_BLUE ) < 1 ) {
			level.teamScores[TEAM_BLUE] = -999;
			trap_SetConfigstring( CS_SCORES2, va( "%i", level.teamScores[TEAM_BLUE] ) );
		}
	}

	level.forfeited = qtrue;
	trap_SendServerCommand( -1, "print \"Game has been forfeited.\n\"" );
	LogExit( "Players have forfeited." );
}


/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gclient_t	*cl;

	if ( level.restarted )
		return;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots
	for (i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;
			
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	// retail checks for a forfeit here, before the sort, so a duel forfeit
	// reads the first two connected clients in client order
	if ( G_ShouldForfeit() ) {
		G_Forfeit();
	}

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {	
		rank = -1;
		score = MAX_QINT;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone,
	// and the Quake Live placement names next to them (retail CalculateRanks:
	// team modes publish "TEAM_RED"/"TEAM_BLUE"; FFA, Duel, Race and Red Rover
	// publish the leaders' names, or "" while the place is empty)
	if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_RED_ROVER ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
		trap_SetConfigstring( CS_FIRST_PLACE, "TEAM_RED" );
		trap_SetConfigstring( CS_SECOND_PLACE, "TEAM_BLUE" );
	} else {
		// retail counts only the clients in the game (spectators sort last)
		if ( level.numPlayingClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_FIRST_PLACE, "" );
		} else {
			const gclient_t *first = &level.clients[ level.sortedClients[0] ];

			trap_SetConfigstring( CS_SCORES1, va("%i", first->ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_FIRST_PLACE, first->pers.netname );
		}
		if ( level.numPlayingClients <= 1 ) {
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SECOND_PLACE, "" );
		} else {
			const gclient_t *second = &level.clients[ level.sortedClients[1] ];

			trap_SetConfigstring( CS_SCORES2, va("%i", second->ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SECOND_PLACE, second->pers.netname );
		}
	}

	// duel names its two players, lower client number first: score, client
	// number and name (retail qagamex86 0x10056836)
	if ( g_gametype.integer == GT_TOURNAMENT && !level.intermissionQueued && !level.intermissiontime ) {
		int first = -1, second = -1;

		if ( level.numPlayingClients == 1 ) {
			first = level.sortedClients[0];
		} else if ( level.numPlayingClients >= 2 ) {
			first = level.sortedClients[0];
			second = level.sortedClients[1];
			if ( first >= second ) {
				first = level.sortedClients[1];
				second = level.sortedClients[0];
			}
		}
		if ( second < 0 ) {
			trap_SetConfigstring( CS_DUEL_SCORE2, va( "%i", SCORE_NOT_PRESENT ) );
			trap_SetConfigstring( CS_DUEL_CLIENT2, "-1" );
			trap_SetConfigstring( CS_DUEL_NAME2, "" );
		} else {
			trap_SetConfigstring( CS_DUEL_SCORE2, va( "%i", level.clients[second].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_DUEL_CLIENT2, va( "%i", second ) );
			trap_SetConfigstring( CS_DUEL_NAME2, level.clients[second].pers.netname );
		}
		if ( first < 0 ) {
			trap_SetConfigstring( CS_DUEL_SCORE1, va( "%i", SCORE_NOT_PRESENT ) );
			trap_SetConfigstring( CS_DUEL_CLIENT1, "-1" );
			trap_SetConfigstring( CS_DUEL_NAME1, "" );
		} else {
			trap_SetConfigstring( CS_DUEL_SCORE1, va( "%i", level.clients[first].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_DUEL_CLIENT1, va( "%i", first ) );
			trap_SetConfigstring( CS_DUEL_NAME1, level.clients[first].pers.netname );
		}
	}

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
static void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}


/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.

Retail qagamex86 0x10056b30: a follower becomes a free spectator and
leaves the world; everyone looks out from the intermission point, and a
player's body stays there as a plain entity.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	gclient_t	*client = ent->client;

	if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.pm_flags &= ~PMF_FOLLOW;
		client->sess.spectatorState = SPECTATOR_FREE;
		client->ps.clientNum = ent - g_entities;
		if ( ent->r.linked ) {
			trap_UnlinkEntity( ent );
		}
	}

	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, client->ps.origin );
	VectorCopy( level.intermission_angle, client->ps.viewangles );
	client->ps.pm_type = PM_INTERMISSION;

	memset( client->ps.powerups, 0, sizeof( client->ps.powerups ) );

	client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}


/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	if ( level.intermission_spot ) // search only once
		return;

	// find the intermission spot
	ent = level.spawnSpots[ SPAWN_SPOT_INTERMISSION ];

	if ( !ent ) { // the map creator forgot to put in an intermission point...
		SelectSpawnPoint( NULL, vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

	level.intermission_spot = qtrue;
}


/*
==================
G_InfoPrepend

The retail qagame's Info_SetValueForKey puts a new key first; the map vote
configstrings keep that order.
==================
*/
static void G_InfoPrepend( char *info, int size, const char *key, const char *value ) {
	char	pair[MAX_INFO_STRING];
	char	rest[MAX_INFO_STRING];

	Info_SetValueForKey( info, key, "" );	// removes the key
	if ( !value[0] ) {
		return;
	}
	Q_strncpyz( rest, info, sizeof( rest ) );
	Com_sprintf( pair, sizeof( pair ), "\\%s\\%s", key, value );
	if ( strlen( pair ) + strlen( rest ) >= (size_t)size ) {
		G_Printf( "Info string length exceeded\n" );
		return;
	}
	Com_sprintf( info, size, "%s%s", pair, rest );
}


/*
==================
G_CountMapVotes

Retail qagamex86 0x10043d80: each connected client's map vote counted
into CS_ROTATION_VOTES.
==================
*/
void G_CountMapVotes( void ) {
	char	votes[MAX_INFO_STRING];
	int		i, vote;

	memset( level.mapVotes, 0, sizeof( level.mapVotes ) );
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected != CON_CONNECTED ) {
			continue;
		}
		vote = level.clients[i].pers.mapVote;
		if ( vote >= 1 && vote <= 3 ) {
			level.mapVotes[vote - 1]++;
		}
	}

	votes[0] = '\0';
	for ( i = 0; i < 3; i++ ) {
		G_InfoPrepend( votes, sizeof( votes ), va( "%i", i ), va( "%i", level.mapVotes[i] ) );
	}
	trap_SetConfigstring( CS_ROTATION_VOTES, votes );
}


/*
==================
G_ClearVote

Retail qagamex86 0x10059130.
==================
*/
void G_ClearVote( void ) {
	int		i;

	level.voteTime = 0;
	level.voteExecuteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
	trap_SetConfigstring( CS_VOTE_STRING, "" );
	trap_SetConfigstring( CS_INTERMISSION_EXIT, "" );
	for ( i = 0; i < level.maxclients; i++ ) {
		level.clients[i].pers.voted = 0;
	}
}


/*
==================
BeginIntermission

Retail qagamex86 0x10056cb0: the duel result is booked, everyone moves to
the intermission point, and when the server has a map pool the three
"nextmaps" choices go up for the end-of-match vote.
==================
*/
void BeginIntermission( void ) {
	char		nextMaps[MAX_INFO_STRING];
	char		titles[MAX_INFO_STRING];
	char		votes[MAX_INFO_STRING];
	gentity_t	*ent;
	int			i;

	if ( level.intermissiontime ) {
		return;	// already active
	}

	trap_SetConfigstring( CS_INTERMISSION, "1" );
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	for ( i = 0; i < level.maxclients; i++ ) {
		ent = g_entities + i;
		if ( !ent->inuse ) {
			continue;
		}
		if ( ent->client->pers.connected == CON_CONNECTED ) {
			ent->client->pers.voted = 1;
		}
		MoveClientToIntermission( ent );
	}

	titles[0] = votes[0] = '\0';
	trap_Cvar_VariableStringBuffer( "nextmaps", nextMaps, sizeof( nextMaps ) );
	G_ClearVote();
	if ( nextMaps[0] ) {
		for ( i = 0; i < 3; i++ ) {
			Q_strncpyz( level.voteMaps[i], Info_ValueForKey( nextMaps, va( "map_%i", i ) ), sizeof( level.voteMaps[i] ) );
			G_InfoPrepend( titles, sizeof( titles ), va( "map_%i", i ), level.voteMaps[i] );
			Q_strncpyz( level.voteTitles[i], Info_ValueForKey( nextMaps, va( "title_%i", i ) ), sizeof( level.voteTitles[i] ) );
			G_InfoPrepend( titles, sizeof( titles ), va( "title_%i", i ), level.voteTitles[i] );
			G_InfoPrepend( titles, sizeof( titles ), va( "gt_%i", i ), Info_ValueForKey( nextMaps, va( "gt_%i", i ) ) );
			Q_strncpyz( level.voteFactories[i], Info_ValueForKey( nextMaps, va( "cfg_%i", i ) ), sizeof( level.voteFactories[i] ) );
			level.mapVotes[i] = 0;
			G_InfoPrepend( votes, sizeof( votes ), va( "%i", i ), "0" );
		}
		level.voteTime = level.time;
		trap_SetConfigstring( CS_ROTATION_TITLES, titles );
		trap_SetConfigstring( CS_ROTATION_VOTES, votes );
		trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	}

	// send the current scoring to all clients; retail then names the match
	// awards (0x100559e0), which arrive with the match statistics
	SendScoreboardMessageToAllClients();
}


/*
=============
ExitLevel

Retail qagamex86 0x10057000: a training server shuts down and
sv_quitOnExitLevel quits; otherwise the duel loser steps out, the map
voted for most (a tie drawn at random) becomes "nextmap", and the server
restarts the map when that is the same map (or the vote failed) or runs
nextmap.
=============
*/
void ExitLevel( void ) {
	char		serverinfo[MAX_INFO_STRING];
	char		nextMaps[MAX_INFO_STRING];
	char		current[128];
	char		chosen[128];
	qboolean	failed = qfalse;
	qboolean	same = qfalse;
	int			i;

	trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
	Q_strncpyz( current, Info_ValueForKey( serverinfo, "mapname" ), sizeof( current ) );

	if ( g_training.integer ) {
		trap_SendConsoleCommand( EXEC_APPEND, "killserver\n" );
		return;
	}
	if ( trap_Cvar_VariableValue( "sv_quitOnExitLevel" ) != 0.0f ) {
		trap_SendConsoleCommand( EXEC_APPEND, "quit\n" );
		return;
	}

	BotInterbreedEndMatch();
	trap_SetConfigstring( CS_INTERMISSION, "0" );
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		RemoveTournamentLoser();
	}

	if ( !( g_voteFlags.integer & 0x800 ) ) {
		chosen[0] = '\0';
		trap_Cvar_VariableStringBuffer( "nextmaps", nextMaps, sizeof( nextMaps ) );
		if ( nextMaps[0] ) {
			qboolean	tied[3] = { qfalse, qfalse, qfalse };
			int			best = 0, ties, pick;

			for ( i = 1; i < 3; i++ ) {
				if ( !level.voteMaps[i][0] ) {
					continue;
				}
				if ( level.mapVotes[i] == level.mapVotes[best] ) {
					tied[i] = tied[best] = qtrue;
				} else if ( level.mapVotes[i] > level.mapVotes[best] ) {
					best = i;
					tied[0] = tied[1] = tied[2] = qfalse;
				}
			}
			ties = tied[0] + tied[1] + tied[2];
			pick = best;
			if ( ties > 1 ) {
				pick = (int)( (double)( rand() & 0x7fff ) / 32767.0 * (double)ties );
				// retail can draw one past the last tied slot; FnQL keeps it in range
				if ( pick > 2 ) {
					pick = 2;
				}
				if ( !tied[pick] ) {
					pick += tied[0] ? 1 : 2;
					if ( pick > 2 ) {
						pick = 2;
					}
				}
			}

			if ( !Q_stricmp( "default", level.voteMaps[pick] ) ) {
				failed = qtrue;
			} else {
				if ( level.voteFactories[pick][0] ) {
					trap_Cvar_Set( "nextmap", va( "map %s %s", level.voteMaps[pick], level.voteFactories[pick] ) );
				} else {
					trap_Cvar_Set( "nextmap", va( "map %s", level.voteMaps[pick] ) );
				}
				Q_strncpyz( chosen, level.voteMaps[pick], sizeof( chosen ) );
			}
		}
		same = !Q_stricmp( current, chosen );
	}

	if ( !( g_voteFlags.integer & 0x800 ) && ( same || failed ) ) {
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.intermissiontime = 0;
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}

	G_ClearVote();
	level.intermissiontime = 0;

	// reset the scores and the team powerup records so we don't enter the
	// intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i = TEAM_RED; i <= TEAM_BLUE; i++ ) {
		level.teamStats[TEAM_STAT_RED_ARMOR][i] = 0;
		level.teamStats[TEAM_STAT_YELLOW_ARMOR][i] = 0;
		level.teamStats[TEAM_STAT_GREEN_ARMOR][i] = 0;
		level.teamStats[TEAM_STAT_MEGA_HEALTH][i] = 0;
		level.teamStats[TEAM_STAT_QUAD][i] = 0;
		level.teamStats[TEAM_STAT_BATTLESUIT][i] = 0;
		level.teamStats[TEAM_STAT_QUAD_TIME][i] = 0;
		level.teamStats[TEAM_STAT_BATTLESUIT_TIME][i] = 0;
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].ps.persistant[PERS_SCORE] = 0;
		}
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}


/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[BIG_INFO_STRING];
	int			min, tsec, sec, len, n;

	tsec = level.time / 100;
	sec = tsec / 10;
	tsec %= 10;
	min = sec / 60;
	sec -= min * 60;

	len = Com_sprintf( string, sizeof( string ), "%3i:%02i.%i ", min, sec, tsec );

	va_start( argptr, fmt );
	Q_vsprintf( string + len, fmt,argptr );
	va_end( argptr );

	n = (int)strlen( string );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + len );
	}

	if ( level.logFile == FS_INVALID_HANDLE ) {
		return;
	}

	trap_FS_Write( string, n, level.logFile );
}


/*
================
LogExit

Retail qagamex86 0x10057510: the end of the match is logged with every
player's score, the intermission is queued, and everyone hears the result
(the winning team, or the game-over call).  The first end since the match
started also goes to the online stats report, which FnQL-game leaves to
milestone 11; a flawless duel winner earns the Steam achievement when an
online provider backs it.
================
*/
void LogExit( const char *string ) {
	gentity_t	*te;
	gclient_t	*cl;
	qboolean	report;
	int			i, ping, clientNum;

	G_LogPrintf( "Exit: %s\n", string );
	level.intermissionQueued = level.time;

	if ( G_IsTeamGametype( g_gametype.integer ) ) {
		G_LogPrintf( "red:%i  blue:%i\n", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	report = !level.matchReported && trap_Cvar_VariableValue( "g_restarted" ) == 0.0f;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		clientNum = level.sortedClients[i];
		cl = &level.clients[clientNum];
		if ( cl->pers.connected == CON_CONNECTING || cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// "Flawless" (achievement 55): won a duel without dying
		if ( g_gametype.integer == GT_TOURNAMENT && report && !trap_HasSteamAchievement( clientNum, 55 ) && cl->ps.persistant[PERS_KILLED] == 0
			&& ( cl->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG ) == 0 ) {
			trap_UnlockSteamAchievement( clientNum, 55 );
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		G_LogPrintf( "score: %i  ping: %i  client: %i %s stats: %d \n",
			cl->ps.persistant[PERS_SCORE], ping, clientNum, cl->pers.netname, cl->stats.key );
	}

	if ( G_IsTeamGametype( g_gametype.integer ) ) {
		te = G_TempEntity( vec3_origin, EV_GLOBAL_TEAM_SOUND );
		te->s.eventParm = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ? GTS_REDTEAM_WINS : GTS_BLUETEAM_WINS;
	} else {
		te = G_TempEntity( vec3_origin, EV_GAMEOVER );
	}
	te->r.svFlags |= SVF_BROADCAST;

	if ( report ) {
		level.matchReported = qtrue;
	}
}


/*
=================
CheckIntermissionExit

Retail qagamex86 0x10057ad0: the intermission lasts ten seconds, or twenty
while the end-of-match map vote runs; CS_INTERMISSION_EXIT then closes the
vote, and three seconds later the level exits.  Who has readied up shows
on the scoreboard meanwhile.  Retail also takes the auto-action screenshot
here (g_autoAction; not yet in FnQL-game).
=================
*/
void CheckIntermissionExit( void ) {
	char		nextMaps[MAX_INFO_STRING];
	gclient_t	*cl;
	int			duration, readyMask, i;

	duration = 10000;
	trap_Cvar_VariableStringBuffer( "nextmaps", nextMaps, sizeof( nextMaps ) );
	if ( nextMaps[0] && !( g_voteFlags.integer & 0x800 ) ) {
		duration = 20000;
	}

	readyMask = 0;
	for ( i = 0; i < level.maxclients && i < 16; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_CONNECTED && !( g_entities[i].r.svFlags & SVF_BOT ) && cl->pers.readyToPlay ) {
			readyMask |= 1 << i;
		}
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_CONNECTED ) {
			cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
		}
	}

	if ( ( g_voteFlags.integer & 0x800 )
		|| ( !level.intermissionExitShown && level.time > level.intermissiontime + duration ) ) {
		trap_SetConfigstring( CS_INTERMISSION_EXIT, "1" );
		level.intermissionExitShown = qtrue;
	}

	if ( level.time >= level.intermissiontime + duration + 3000 ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
		ExitLevel();
	} else {
		level.readyToExit = qfalse;
	}
}


/*
=============
ScoreIsTied

Retail qagamex86 0x10057c60.
=============
*/
static qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( G_IsTeamGametype( g_gametype.integer ) ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}


/*
=================
CheckExitRules

Retail qagamex86 0x10058030, once a frame.  The intermission starts 200
msec after the exit qualifies, so the last frag can be seen.  In a match
being played: forfeits, then (outside clan arena, attack and defend, red
rover and freeze tag with a round delay, which end by their rounds) the
timelimit and the score limits, unless the leaders are tied, when
g_overtime extends the timelimit.
=================
*/
static void CheckExitRules( void ) {
	const int	gametype = g_gametype.integer;
	int			red, blue, timelimit, mercyTime;
	gclient_t	*cl;
	gentity_t	*te;
	int			i;

	// if at the intermission, wait for the map vote, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit();
		return;
	}

	if ( level.intermissionQueued ) {
		if ( level.time - level.intermissionQueued >= 200 ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	if ( level.warmupTime ) {
		return;
	}

	if ( G_ShouldForfeit() ) {
		G_Forfeit();
	}

	if ( G_IsRoundBasedGametype( gametype ) && !( gametype == GT_FREEZE && !g_freezeRoundDelay.integer ) ) {
		return;
	}

	timelimit = g_timelimit.integer * 60000 + level.overtimeAdded;
	mercyTime = g_mercytime.integer * 60000 + level.overtimeAdded;

	if ( ScoreIsTied() ) {
		if ( g_timelimit.integer && g_overtime.integer && level.time - level.startTime >= timelimit ) {
			te = G_TempEntity( vec3_origin, EV_OVERTIME );
			te->r.svFlags |= SVF_BROADCAST;
			level.overtimeAdded += g_overtime.integer * 1000;
		}
		return;
	}

	if ( g_timelimit.integer && level.time - level.startTime >= timelimit ) {
		trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"" );
		LogExit( "Timelimit hit." );
		return;
	}

	red = level.teamScores[TEAM_RED];
	blue = level.teamScores[TEAM_BLUE];

	if ( ( gametype == GT_FFA || gametype == GT_TOURNAMENT || gametype == GT_TEAM ) && g_fraglimit.integer ) {
		if ( red >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}
		if ( blue >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}
		for ( i = 0; i < level.maxclients; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}
			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Fraglimit hit." );
				trap_SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
					cl->pers.netname ) );
				return;
			}
		}
	}

	if ( gametype >= GT_CTF && gametype <= GT_HARVESTER && g_capturelimit.integer ) {
		if ( red >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}
		if ( blue >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}
	}

	if ( gametype == GT_DOMINATION && scorelimit.integer ) {
		if ( red >= scorelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the scorelimit.\n\"" );
			LogExit( "Scorelimit hit." );
			return;
		}
		if ( blue >= scorelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the scorelimit.\n\"" );
			LogExit( "Scorelimit hit." );
			return;
		}
	}

	if ( gametype >= GT_TEAM && mercylimit.integer && level.time - level.startTime >= mercyTime ) {
		if ( red - blue >= mercylimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the mercylimit.\n\"" );
			LogExit( "Mercylimit hit." );
			return;
		}
		if ( blue - red >= mercylimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the mercylimit.\n\"" );
			LogExit( "Mercylimit hit." );
			return;
		}
	}
}


/*
==================
G_WarmupEnd

Retail qagamex86 0x10058860: the countdown is over, so the level restarts
straight into the match (G_InitGame reads g_restarted).  The warmup clock
moves on ten seconds so that nothing counts down again before the restart
runs.  Retail first reports the match start to the online stats service,
which FnQL-game leaves to milestone 11.
==================
*/
static void G_WarmupEnd( void ) {
	level.warmupTime += 10000;
	trap_Cvar_Set( "g_restarted", "1" );
	trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	if ( g_forceSendConfigstring.integer ) {
		trap_ResendConfigstring( CS_WARMUP );
		trap_ResendConfigstring( CS_LEVEL_START_TIME );
	}
	level.restarted = qtrue;
}


/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
/*
==================
G_SetWarmupTime

Retail qagamex86 0x10059c10: the warmup clock, its configstring and the
published game state (PRE_GAME while waiting, COUNT_DOWN, IN_PROGRESS).
==================
*/
static void G_SetWarmupTime( int time ) {
	level.warmupTime = time;
	trap_SetConfigstring( CS_WARMUP, va( "\\time\\%i", time ) );
	trap_Cvar_Set( "g_gameState", time < 0 ? "PRE_GAME" : ( time > 0 ? "COUNT_DOWN" : "IN_PROGRESS" ) );
}


/*
==================
G_ReadyToStart

Retail qagamex86 0x100577a0.  A match may count down once the warmup delay
of a dedicated server has passed, enough players are present (and the
teams within one of each other), and the ready players reach
sv_warmupReadyPercentage of those playing, with at least one human ready;
bots count as ready under bot_autoReady.  While waiting, everyone sees the
ready players in STAT_CLIENTS_READY.  g_doWarmup 0 starts at once.
==================
*/
static qboolean G_ReadyToStart( void ) {
	gclient_t	*cl;
	int			i, ready = 0, readyHumans = 0, readyMask = 0;
	int			red, blue;

	if ( !g_doWarmup.integer ) {
		return qtrue;
	}
	if ( g_dedicated.integer && g_gametype.integer != GT_TOURNAMENT
		&& level.time - level.startTime < g_warmupDelay.integer * 1000 ) {
		return qfalse;
	}

	if ( level.warmupTime < 0 ) {
		for ( i = 0; i < level.maxclients; i++ ) {
			cl = &level.clients[i];
			if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR ) {
				continue;
			}
			if ( cl->pers.readyToPlay ) {
				readyHumans++;
			} else if ( !( ( g_entities[i].r.svFlags & SVF_BOT ) && bot_autoReady.integer ) ) {
				continue;
			}
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		}
		for ( i = 0; i < level.maxclients; i++ ) {
			if ( level.clients[i].pers.connected == CON_CONNECTED ) {
				level.clients[i].ps.stats[STAT_CLIENTS_READY] = readyMask;
			}
		}
	}

	red = TeamConnectedCount( -1, TEAM_RED );
	blue = TeamConnectedCount( -1, TEAM_BLUE );
	if ( g_teamForcePresent.integer ) {
		if ( g_gametype.integer < GT_TEAM ) {
			if ( level.numPlayingClients < 2 ) {
				return qfalse;
			}
		} else if ( red < g_teamSizeMin.integer || blue < g_teamSizeMin.integer ) {
			return qfalse;
		}
	}
	if ( g_teamForceBalance.integer && g_gametype.integer >= GT_TEAM && ( red > blue + 1 || blue > red + 1 ) ) {
		return qfalse;
	}

	if ( level.warmupTime >= 0 ) {
		// nobody fires during the countdown (retail qagamex86 0x10057a9b);
		// the restart into the match clears the flag
		if ( level.warmupTime > 0 ) {
			for ( i = 0; i < level.maxclients; i++ ) {
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.clients[i].ps.pm_flags |= PMF_ATTACK_LOCKOUT;
				}
			}
		}
		return qtrue;
	}
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		return level.numPlayingClients == 2 && ready == level.numPlayingClients;
	}
	if ( sv_warmupReadyPercentage.value == 0.0f ) {
		return qtrue;
	}
	return readyHumans && (float)ready / (float)level.numPlayingClients >= sv_warmupReadyPercentage.value;
}


/*
==================
G_CheckWarmup

Retail qagamex86 0x100586e0 outside duel: PRE_GAME until G_ReadyToStart,
then g_warmup seconds of countdown, then the match; back to PRE_GAME when
the conditions lapse during the countdown.
==================
*/
static void G_CheckWarmup( void ) {
	int	i;

	if ( level.warmupTime == 0 ) {
		if ( G_ShouldForfeit() ) {
			G_Forfeit();
		}
		return;
	}

	if ( G_ReadyToStart() ) {
		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			G_SetWarmupTime( -1 );
		}
		if ( level.warmupTime < 0 ) {
			G_SetWarmupTime( level.time + g_warmup.integer * 1000 );
			return;
		}
		if ( level.time >= level.warmupTime ) {
			G_WarmupEnd();
		}
		return;
	}

	if ( level.warmupTime != -1 ) {
		G_SetWarmupTime( -1 );
		G_LogPrintf( "Warmup:\n" );
		// a cancelled countdown releases the attack lockout too (retail
		// qagamex86 0x10058796)
		for ( i = 0; i < level.maxclients; i++ ) {
			if ( level.clients[i].pers.connected != CON_CONNECTED ) {
				continue;
			}
			level.clients[i].ps.pm_flags &= ~PMF_ATTACK_LOCKOUT;
			if ( level.clients[i].pers.readyToPlay ) {
				level.clients[i].pers.readyToPlay = qfalse;
				ClientUserinfoChanged( i );
			}
		}
	}
}


static void CheckTournament( void ) {
	// retail G_RunFrame (qagamex86 0x100598c1): a duel pulls in the next
	// queued spectator and resends the queue's player keys, then every mode
	// runs the same warmup check
	AddTournamentPlayer();
	G_RefreshDuelQueue();

	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( !G_IsSinglePlayer() ) {
		G_CheckWarmup();
	}
}


/*
==================
CheckVote
==================
*/
static void CheckVote( void ) {

	// the intermission's map vote ends with the intermission
	if ( level.intermissiontime ) {
		return;
	}

	if ( level.voteExecuteTime ) {
		 if ( level.voteExecuteTime < level.time ) {
			level.voteExecuteTime = 0;
			trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
		 }
		 return;
	}

	if ( !level.voteTime ) {
		return;
	}

	if ( level.time - level.voteTime >= VOTE_TIME ) {
		G_BroadcastServerCommand( -1, "print \"Vote failed.\n\"" );
	} else {
		// ATVI Q3 1.32 Patch #9, WNF
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			G_BroadcastServerCommand( -1, "print \"Vote passed.\n\"" );
			level.voteExecuteTime = level.time + 3000;
		} else if ( level.voteNo >= level.numVotingClients/2 ) {
			// same behavior as a timeout
			G_BroadcastServerCommand( -1, "print \"Vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
}


/*
==================
PrintTeam
==================
*/
static void PrintTeam( team_t team, const char *message ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam != team )
			continue;
		if ( level.clients[i].pers.connected != CON_CONNECTED )
			continue;
		trap_SendServerCommand( i, message );
	}
}


/*
==================
SetLeader
==================
*/
void SetLeader( team_t team, int client ) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam( team, va("print \"%s "S_COLOR_STRIP"is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam( team, va("print \"%s "S_COLOR_STRIP"is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged( i );
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	// retail announces no new team leader (qagamex86 has no such string)
}


/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( team_t team ) {
	int i;
	int	max_score, max_id;
	int	max_bot_score, max_bot_id;

	for ( i = 0 ; i < level.maxclients ; i++ ) {

		if ( level.clients[i].sess.sessionTeam != team || level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;

		if ( level.clients[i].sess.teamLeader )
			return;
	}

	// no leaders? find player with highest score
	max_score = SHRT_MIN;
	max_id = -1;
	max_bot_score = SHRT_MIN;
	max_bot_id = -1;

	for ( i = 0 ; i < level.maxclients ; i++ ) {

		if ( level.clients[i].sess.sessionTeam != team )
			continue;

		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			if ( level.clients[i].ps.persistant[PERS_SCORE] > max_bot_score ) {
				max_bot_score = level.clients[i].ps.persistant[PERS_SCORE];
				max_bot_id = i;
			}
		} else {
			if ( level.clients[i].ps.persistant[PERS_SCORE] > max_score ) {
				max_score = level.clients[i].ps.persistant[PERS_SCORE];
				max_id = i;
			}
		}
	}

	if ( max_id != -1 ) {
		SetLeader( team, max_id ); 
		return;
	}

	if ( max_bot_id != -1 ) {
		SetLeader( team, max_bot_id );
		return;
	}
}


/*
==================
CheckCvars

Every frame (retail qagamex86 0x100592c0): g_needpass follows g_password,
and practiceflags and g_teamSpecFreeCam are republished to clients.
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( lastMod != g_password.modificationCount ) {
		lastMod = g_password.modificationCount;
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) != 0 ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}

	trap_SetConfigstring( CS_PRACTICE_FLAGS, va( "%d", practiceflags.integer ) );
	trap_SetConfigstring( CS_TEAM_SPEC_FREECAM, va( "%d", g_teamSpecFreeCam.integer ) );
}


/*
==================
G_UpdateSettingsConfigstrings

The settings retail publishes as configstrings when the game starts
(qagamex86 0x100549e0).  Two of the configstring names in bg_public.h
predate this evidence: CS_FACTORY_FLAGS carries g_debugFlags and
CS_DOM_CAPTURE_TIME carries g_dmgThroughSurfaceDistance.
==================
*/
void G_UpdateSettingsConfigstrings( void ) {
	trap_SetConfigstring( CS_PLAYER_CYLINDERS, va( "%d", g_playerCylinders.integer ) );
	trap_SetConfigstring( CS_FACTORY_FLAGS, va( "%d", g_debugFlags.integer ) );
	trap_SetConfigstring( CS_RR_PING_RATE, va( "%f", g_rrInfectedSurvivorMinSpeed.value ) );
	trap_SetConfigstring( CS_DOM_CAPTURE_TIME, va( "%f", g_dmgThroughSurfaceDistance.value ) );
	trap_SetConfigstring( CS_SETTINGS_MASK, va( "%d", g_customSettings.integer ) );
	trap_SetConfigstring( CS_STARTING_WEAPONS, va( "%d", g_startingWeapons.integer ) );
	if ( g_loadout.integer ) {
		trap_SetConfigstring( CS_DISABLED_LOADOUTS, va( "%d", g_disableLoadout.integer ) );
	}
}


/*
==================
G_UpdateTimeoutConfigstrings

Retail qagamex86 0x10063450: the timeouts each team has left, and the
current timeout's start and end.
==================
*/
void G_UpdateTimeoutConfigstrings( void ) {
	trap_SetConfigstring( CS_RED_TIMEOUTS_LEFT, va( "%d", g_timeoutCount.integer - level.timeoutsUsed[0] ) );
	trap_SetConfigstring( CS_BLUE_TIMEOUTS_LEFT, va( "%d", g_timeoutCount.integer - level.timeoutsUsed[1] ) );
	trap_SetConfigstring( CS_TIMEOUT_END_TIME, va( "%i", level.timeoutEndTime ) );
	trap_SetConfigstring( CS_TIMEOUT_START_TIME, va( "%i", level.timeoutStartTime ) );
}


/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t *ent ) {
	int	thinktime;

	// retail (qagamex86 0x10059370) runs the per-frame hook first
	if ( ent->frameThink ) {
		ent->frameThink( ent );
	}

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}
	
	ent->nextthink = 0;
	if ( !ent->think ) {
		G_Error ( "NULL ent->think");
	} else {
		ent->think (ent);
	}
}


/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
static void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	gclient_t	*client;
	static	gentity_t *missiles[ MAX_GENTITIES - MAX_CLIENTS ];
	int		numMissiles;
	
	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	level.msec = level.time - level.previousTime;

	// at the intermission a map voter's vote menu comes back 2.25 seconds
	// after the vote (retail qagamex86 0x10059603)
	if ( level.intermissiontime ) {
		for ( i = 0; i < level.maxclients; i++ ) {
			const int since = level.time - level.clients[i].pers.mapVoteTime;

			if ( level.clients[i].pers.connected == CON_CONNECTED && since > 2250 && since - level.msec <= 2250 ) {
				trap_SendServerCommand( i, "enable_vote_ui" );
			}
		}
	}

	// get any cvar changes
	G_UpdateCvars();

	numMissiles = 0;

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			// queue for unlagged pass
			missiles[ numMissiles ] = ent;
			numMissiles++;
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			client = ent->client;
			client->sess.spectatorTime += level.msec; 
			if ( client->pers.connected == CON_CONNECTED )
				G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}

	if ( numMissiles ) {
		// unlagged
		G_TimeShiftAllClients( level.previousTime, NULL );
		// run missiles
		for ( i = 0; i < numMissiles; i++ )
			G_RunMissile( missiles[ i ] );
		// unlagged
		G_UnTimeShiftAllClients( NULL );
	}

	// perform final fixups on the players
	ent = &g_entities[0];
	for (i = 0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	// Quake Live has no team votes (retail qagame registers only callvote and vote).

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	// unlagged
	level.frameStartTime = trap_Milliseconds();
}
