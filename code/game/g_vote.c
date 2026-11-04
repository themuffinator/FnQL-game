/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "g_local.h"
#include <limits.h>

// Retail qagame 1069: callvote 0x100423a0, electorate 0x10043920,
// ballot 0x10044270, resolution 0x100588f0/0x100591b0. These are private
// module states, not playerState flags or an extension to the native ABI.
enum { BALLOT_NONE, BALLOT_ELIGIBLE, BALLOT_YES, BALLOT_NO, BALLOT_PASS, BALLOT_VETO };
typedef enum {
	V_MAP,
	V_RESTART,
	V_NEXT,
	V_KICK,
	V_CLIENTKICK,
	V_TIME,
	V_FRAGS,
	V_SHUFFLE,
	V_SIZE,
	V_COIN,
	V_RANDOM,
	V_LOADOUT,
	V_AMMO,
	V_TIMERS,
	V_RESPAWN
} voteKind_t;

typedef struct {
	const char *name;
	int disabledBit;
	const char *disabledMessage;
} voteOption_t;

static const voteOption_t options[] = {
	{ "map", 1, "Voting to change the map being played" },
	{ "map_restart", 2, "Voting on a map restart" },
	{ "nextmap", 4, "Voting to move to the next map" },
	{ "kick", 16, "Voting to kick a player" },
	{ "clientkick", 16, "Voting to kick a player" },
	{ "timelimit", 32, "Voting to change the games time limit" },
	{ "fraglimit", 64, "Voting to change the games frag limit" },
	{ "shuffle", 128, "Voting to shuffle the teams" },
	{ "teamsize", 256, "Voting to change team size" },
	{ "cointoss", 512, "Voting to coin toss" },
	{ "random", 512, "Random number generation" },
	{ "loadouts", 1024, "Voting to alter loadouts" },
	{ "ammo", 4096, "Voting to alter the ammo system" },
	{ "timers", 8192, "Voting to alter the item timers" },
	{ "weaprespawn", 16384, "Voting to change the weapon respawn time" } };

static void VoteMessage( int client, const char *message ) {
	trap_SendServerCommand( client, va( "print \"%s\n\"", message ) );
}

static int HelpColor( voteKind_t kind ) {
	return ( g_voteFlags.integer & options[kind].disabledBit ) ? 1 : 5;
}

static void VoteHelp( int client ) {
	VoteMessage( client, "^3Callvote commands:" );
	VoteMessage( client, va( "^%imap           ^%inextmap        ^%imap_restart   ^7",
							 HelpColor( V_MAP ), HelpColor( V_NEXT ), HelpColor( V_RESTART ) ) );
	VoteMessage( client, va( "^%ikick          ^%iclientkick                      ^7",
							 HelpColor( V_KICK ), HelpColor( V_CLIENTKICK ) ) );
	VoteMessage( client, va( "^%ishuffle       ^%iteamsize       ^%icointoss      ^7",
							 HelpColor( V_SHUFFLE ), HelpColor( V_SIZE ), HelpColor( V_COIN ) ) );
	VoteMessage( client, va( "^%itimelimit     ^%ifraglimit      ^%iweaprespawn   ^7",
							 HelpColor( V_TIME ), HelpColor( V_FRAGS ), HelpColor( V_RESPAWN ) ) );
	VoteMessage( client, va( "^%iloadouts      ^%iammo           ^%itimers        ^7",
							 HelpColor( V_LOADOUT ), HelpColor( V_AMMO ), HelpColor( V_TIMERS ) ) );
	VoteMessage( client, "Usage: ^3\\callvote <command> <params>^7" );
}

// 0x10042210: only ASCII letters, digits and underscore. Reject overlong
// input instead of allowing a truncated prefix to name a different target.
static qboolean VoteToken( const char *text, size_t limit ) {
	size_t i;
	for ( i = 0; text[i]; ++i ) {
		unsigned char c = (unsigned char)text[i];
		if ( i >= limit || !( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
							  ( c >= '0' && c <= '9' ) || c == '_' ) )
			return qfalse;
	}
	return qtrue;
}

static qboolean VoteNumber( const char *text, int *number ) {
	unsigned value = 0;
	if ( !*text )
		return qfalse;
	for ( ; *text; ++text ) {
		if ( *text < '0' || *text > '9' || value > ( (unsigned)INT_MAX - ( *text - '0' ) ) / 10 )
			return qfalse;
		value = value * 10 + ( *text - '0' );
	}
	*number = (int)value;
	return qtrue;
}

static int NamedVoteTarget( const char *name ) {
	int i;
	for ( i = 0; i < level.maxclients; ++i ) {
		char clean[MAX_NETNAME];
		if ( level.clients[i].pers.connected != CON_CONNECTED )
			continue;
		Q_strncpyz( clean, level.clients[i].pers.netname, sizeof( clean ) );
		Q_CleanStr( clean );
		if ( !Q_stricmp( clean, name ) )
			return i;
	}
	return -1;
}

// The proposed action is built locally. A refused vote cannot overwrite a
// previously passed command still awaiting its three-second execution delay.
static qboolean PrepareVote( int caller, voteKind_t kind, const char *arg, const char *factory,
							 qboolean midgame, char *command, char *display ) {
	int value = 0, i;
	qboolean privileged = level.clients[caller].pers.privileges >= 1;
	const voteOption_t *option = &options[kind];
	const char *missing = NULL, *invalid = NULL;

	if ( kind == V_SIZE && g_gametype.integer == GT_TOURNAMENT ) {
		// The missing closing quote is present in the retail message.
		trap_SendServerCommand( caller, "print \"Teamsize is not available in Duel.\n" );
		return qfalse;
	}
	if ( !privileged && ( g_voteFlags.integer & option->disabledBit ) ) {
		VoteMessage( caller, va( "%s is disabled on this server.", option->disabledMessage ) );
		return qfalse;
	}
	if ( kind == V_SHUFFLE && midgame ) {
		VoteMessage( caller, "Voting to shuffle the teams is only permitted during warmup." );
		return qfalse;
	}
	if ( kind >= V_LOADOUT && kind <= V_TIMERS && midgame ) {
		VoteMessage( caller, va( "%s is only allowed during the warm up period.",
								 option->disabledMessage ) );
		return qfalse;
	}
	switch ( kind ) {
	case V_MAP: {
		fileHandle_t file = 0;
		int length;
		if ( !*arg ) {
			VoteMessage( caller, "Missing map name." );
			return qfalse;
		}
		if ( *factory && !privileged && ( g_voteFlags.integer & 8 ) ) {
			VoteMessage( caller,
						 "Voting to change the gametype being played is disabled on this server." );
			return qfalse;
		}
		length = trap_FS_FOpenFile( va( "maps/%s.bsp", arg ), &file, FS_READ );
		if ( file )
			trap_FS_FCloseFile( file );
		if ( length < 0 ) {
			VoteMessage( caller, "Map does not exist." );
			return qfalse;
		}
		if ( *factory && !trap_FactoryExists( factory ) ) {
			VoteMessage( caller, "Factory does not exist." );
			return qfalse;
		}
		Com_sprintf( command, MAX_STRING_CHARS, *factory ? "%s %s %s" : "%s %s", option->name, arg,
					 factory );
		break;
	}
	case V_NEXT:
		trap_Cvar_VariableStringBuffer( "nextmap", command, MAX_STRING_CHARS );
		if ( !*command || !strcmp( command, "map_restart 0" ) ) {
			VoteMessage( caller, "No nextmap is currently set." );
			return qfalse;
		}
		break;
	case V_KICK:
		if ( !*arg ) {
			VoteMessage( caller, "Missing player name." );
			return qfalse;
		}
		if ( NamedVoteTarget( arg ) < 0 ) {
			VoteMessage( caller, va( "Player %s is not on the server.", arg ) );
			return qfalse;
		}
		if ( !Q_stricmp( arg, "all" ) ) {
			VoteMessage( caller, "Voting to kick all players is not allowed." );
			return qfalse;
		}
		break;
	case V_CLIENTKICK:
		missing = "Missing player id.";
		invalid = "Invalid player id, parameter must be an integer.";
		break;
	case V_TIME:
		missing = "Missing desired timelimit.";
		invalid = "Invalid desired timelimit, parameter must be an integer in minutes.";
		break;
	case V_FRAGS:
		missing = "Missing desired fraglimit.";
		invalid = "Invalid desired fraglimit, parameter must be an integer.";
		break;
	case V_SIZE:
		missing = "Missing desired teamsize.";
		invalid = "Invalid desired teamsize, parameter must be an integer.";
		break;
	case V_RESPAWN:
		missing = "Missing desired weapon respawn time.";
		invalid = "Invalid desired weapon respawn time, parameter must be an integer.";
		break;
	case V_RANDOM:
		invalid = "Invalid upper limit, parameter must be an integer.";
		break;
	case V_COIN:
		if ( *arg && Q_stricmp( arg, "h" ) && Q_stricmp( arg, "heads" ) && Q_stricmp( arg, "t" ) &&
			 Q_stricmp( arg, "tails" ) ) {
			VoteMessage( caller, "Valid cointoss parameters are:    ^5heads    ^5tails ^7" );
			return qfalse;
		}
		break;
	case V_LOADOUT:
	case V_TIMERS:
	case V_AMMO:
		if ( Q_stricmp( arg, kind == V_AMMO ? "global" : "on" ) &&
			 Q_stricmp( arg, kind == V_AMMO ? "weap" : "off" ) ) {
			VoteMessage( caller, kind == V_AMMO
									 ? "^3Valid ammo options are:    ^5GLOBAL    ^5WEAP^7"
								 : kind == V_LOADOUT
									 ? "^3Valid loadout options are:    ^5ON    ^5OFF^7"
									 : "^3Valid item timer options are:    ^5ON    ^5OFF^7" );
			return qfalse;
		}
		break;
	case V_SHUFFLE:
		if ( *arg ) {
			VoteMessage( caller, "Too many parameters called for a shuffle." );
			return qfalse;
		}
		break;
	case V_RESTART:
		break;
	}
	if ( missing && !*arg ) {
		VoteMessage( caller, missing );
		return qfalse;
	}
	if ( invalid && !VoteNumber( arg, &value ) ) {
		VoteMessage( caller, invalid );
		return qfalse;
	}
	if ( kind == V_CLIENTKICK ) {
		if ( value >= level.maxclients || level.clients[value].pers.connected != CON_CONNECTED ) {
			VoteMessage( caller, "Invalid player id." );
			return qfalse;
		}
		if ( level.clients[value].pers.privileges >= 2 ) {
			VoteMessage( caller, "Voting to kick a server admin is not allowed." );
			return qfalse;
		}
	}
	if ( kind == V_RANDOM && ( value < 2 || value > 100 ) ) {
		VoteMessage( caller, "Invalid upper limit. (Valid Range: 2 - 100)" );
		return qfalse;
	}
	if ( kind == V_SIZE ) {
		int counts[TEAM_NUM_TEAMS] = { 0 };
		int maximum = g_gametype.integer >= GT_TEAM ? level.maxclients / 2 : level.maxclients;
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *client = &level.clients[i];
			if ( client->pers.connected == CON_CONNECTED && client->sess.sessionTeam >= TEAM_FREE &&
				 client->sess.sessionTeam < TEAM_NUM_TEAMS )
				++counts[client->sess.sessionTeam];
		}
		if ( value && value <= MAX_CLIENTS ) {
			for ( i = TEAM_FREE; i <= TEAM_BLUE; ++i ) {
				if ( counts[i] <= value )
					continue;
				trap_SendServerCommand(
					caller, i == TEAM_FREE
								? va( "print \"^1The arena has more than %d players. Players must "
									  "leave before this teamsize can be set.^7\"",
									  value )
								: va( "print \"^1%s has more than %d players. Players must leave "
									  "the team before this teamsize can be set.^7\"",
									  i == TEAM_RED ? "Red Team" : "Blue Team", value ) );
				return qfalse;
			}
		}
		if ( value > MAX_CLIENTS ||
			 ( value && ( value < g_teamSizeMin.integer || value > maximum ) ) ) {
			VoteMessage( caller,
						 va( "Invalid team size. (Valid Range: %d - %d)", g_teamSizeMin.integer,
							 value > MAX_CLIENTS ? level.maxclients : maximum ) );
			return qfalse;
		}
	}
	if ( kind != V_MAP && kind != V_NEXT )
		Com_sprintf( command, MAX_STRING_CHARS, kind == V_CLIENTKICK ? "%s %s" : "%s \"%s\"",
					 option->name, arg );
	Q_strncpyz( display, command, MAX_STRING_CHARS );
	if ( kind == V_CLIENTKICK )
		Com_sprintf( display, MAX_STRING_CHARS, "kick %s", level.clients[value].pers.netname );
	return qtrue;
}

void Cmd_CallVote_f( gentity_t *ent ) {
	char name[MAX_STRING_TOKENS], arg[MAX_STRING_TOKENS], factory[MAX_STRING_TOKENS];
	char command[MAX_STRING_CHARS], display[MAX_STRING_CHARS];
	int caller = ent - g_entities, kind, i;
	gclient_t *client = ent->client;
	qboolean privileged = client->pers.privileges >= 1;
	qboolean midgame = G_ReadyToStart() || level.warmupTime == 0;
	// Retail compares level.time directly with g_voteDelay (milliseconds),
	// not seconds since the last call (0x100423c3).
	if ( level.time < g_voteDelay.integer ) {
		// The shared baseline formatter truncates floats. Format this retail
		// rounded tenth explicitly without changing other users of that formatter.
		float seconds = (float)( ( g_voteDelay.integer - (double)level.time ) / 1000.0 );
		int tenths = (int)( (double)seconds * 10.0 + 0.5 );
		VoteMessage( caller,
					 va( "Voting will be allowed in %d.%d seconds.", tenths / 10, tenths % 10 ) );
		return;
	}
	if ( g_training.integer ) {
		VoteMessage( caller, "Voting is not allowed in training." );
		return;
	}
	if ( !g_allowVote.integer && !privileged ) {
		VoteMessage( caller, "Public voting is not allowed here." );
		return;
	}
	if ( level.voteTime ) {
		VoteMessage( caller, "A vote is already in progress." );
		return;
	}
	if ( level.voteExecuteTime > level.time ) {
		VoteMessage( caller, "A vote is being executed." );
		return;
	}
	if ( g_voteLimit.integer && client->pers.voteCount >= g_voteLimit.integer && !privileged ) {
		VoteMessage( caller, "You have called the maximum number of votes." );
		return;
	}
	if ( !g_allowSpecVote.integer && client->sess.sessionTeam == TEAM_SPECTATOR && !privileged ) {
		VoteMessage( caller, "Not allowed to call a vote as spectator." );
		return;
	}
	if ( !g_allowVoteMidGame.integer && midgame && !privileged ) {
		VoteMessage( caller, "Voting is only allowed during the warm up period." );
		return;
	}
	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, arg, sizeof( arg ) );
	trap_Argv( 3, factory, sizeof( factory ) );
	if ( !VoteToken( name, sizeof( name ) - 1 ) || !VoteToken( arg, 31 ) ||
		 !VoteToken( factory, 31 ) ) {
		VoteMessage( caller, "Invalid vote string." );
		return;
	}
	for ( kind = 0; kind < ARRAY_LEN( options ); ++kind )
		if ( !Q_stricmp( name, options[kind].name ) )
			break;
	if ( kind == ARRAY_LEN( options ) ) {
		VoteHelp( caller );
		return;
	}
	if ( !PrepareVote( caller, (voteKind_t)kind, arg, factory, midgame, command, display ) )
		return;
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}
	Q_strncpyz( level.voteString, command, sizeof( level.voteString ) );
	Q_strncpyz( level.voteDisplayString, display, sizeof( level.voteDisplayString ) );
	if ( client->pers.voteCount < INT_MAX )
		++client->pers.voteCount;
	// Calls during the ready countdown are accepted but do not start a vote.
	if ( level.warmupTime > 0 )
		return;
	VoteMessage( -1, va( "%s called a vote.", client->pers.netname ) );
	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *voter = &level.clients[i];
		voter->pers.voted =
			voter->pers.connected == CON_CONNECTED && !( g_entities[i].r.svFlags & SVF_BOT ) &&
					( g_allowSpecVote.integer || voter->sess.sessionTeam != TEAM_SPECTATOR )
				? BALLOT_ELIGIBLE
				: BALLOT_NONE;
	}
	client->pers.voted = BALLOT_YES;
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, "1" );
	trap_SetConfigstring( CS_VOTE_NO, "0" );
}

void G_CastVote( gentity_t *ent ) {
	char answer[64];
	int caller = ent - g_entities;
	gclient_t *client = ent->client;
	qboolean privileged = client->pers.privileges >= 1;
	if ( !level.voteTime ) {
		VoteMessage( caller, "No vote in progress." );
		return;
	}
	if ( !privileged && client->pers.voted != BALLOT_ELIGIBLE ) {
		VoteMessage( caller, client->pers.voted == BALLOT_NONE
								 ? "You may not participate in this vote."
								 : "Vote already cast." );
		return;
	}
	VoteMessage( caller, "Vote cast." );
	trap_Argv( 1, answer, sizeof( answer ) );
	if ( answer[0] == 'y' || answer[0] == 'Y' || answer[0] == '1' ) {
		client->pers.voted = privileged ? BALLOT_PASS : BALLOT_YES;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", ++level.voteYes ) );
	} else {
		client->pers.voted = privileged ? BALLOT_VETO : BALLOT_NO;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", ++level.voteNo ) );
	}
}

void G_RevertVote( gclient_t *client ) {
	// Disconnected clients leave the electorate; resolution recounts it.
	client->pers.voted = BALLOT_NONE;
}

static void ExecuteVote( void ) {
	char command[MAX_STRING_CHARS], kind[32], *cursor, *name, *arg;
	int value;
	Q_strncpyz( command, level.voteString, sizeof( command ) );
	cursor = command;
	name = COM_Parse( &cursor );
	// COM_Parse reuses one token buffer.
	Q_strncpyz( kind, name, sizeof( kind ) );
	arg = COM_Parse( &cursor );
	if ( !Q_stricmp( kind, "cointoss" ) ||
		 ( !Q_stricmp( kind, "random" ) && VoteNumber( arg, &value ) && value == 2 ) ) {
		int draw = (int)( (double)( rand() & 32767 ) * 2.0 / 32767.0 );
		VoteMessage( -1, va( "^3The coin is: ^5%s^7", draw == 1 ? "HEADS" : "TAILS" ) );
	} else if ( !Q_stricmp( kind, "random" ) && VoteNumber( arg, &value ) && value >= 3 &&
				value <= 100 ) {
		int draw = (int)( (double)( rand() & 32767 ) * value / 32767.0 ) + 1;
		VoteMessage( -1, va( "^3Random number is: ^5%d^7", draw ) );
	} else if ( !Q_stricmp( kind, "loadouts" ) ) {
		trap_Cvar_Set( "g_loadout", !Q_stricmp( arg, "on" ) ? "1" : "0" );
	} else if ( !Q_stricmp( kind, "timers" ) ) {
		trap_Cvar_Set( "g_itemTimers", !Q_stricmp( arg, "on" ) ? "1" : "0" );
	} else if ( !Q_stricmp( kind, "ammo" ) ) {
		trap_Cvar_Set( "g_ammoPack", !Q_stricmp( arg, "global" ) ? "1" : "0" );
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	} else if ( !Q_stricmp( kind, "weaprespawn" ) && VoteNumber( arg, &value ) ) {
		trap_Cvar_Set( "g_weaponRespawn", va( "%i", value ) );
	} else if ( !Q_stricmp( kind, "shuffle" ) && g_gametype.integer >= GT_TEAM ) {
		G_ShuffleTeams();
	} else if ( !Q_stricmp( kind, "teamsize" ) && g_gametype.integer >= GT_TEAM &&
				VoteNumber( arg, &value ) && value &&
				( TeamCount( -1, TEAM_RED ) > value || TeamCount( -1, TEAM_BLUE ) > value ) ) {
		VoteMessage( -1,
					 va( "^1%s has more than %d players.",
						 TeamCount( -1, TEAM_RED ) > value ? "Red Team" : "Blue Team", value ) );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}
}

void G_CheckVote( void ) {
	int i, eligible = 0, yes = 0, no = 0, outcome = 0;
	if ( level.intermissiontime )
		return;
	if ( level.voteExecuteTime && level.time > level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		ExecuteVote();
	}
	if ( !level.voteTime )
		return;
	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED )
			continue;
		switch ( client->pers.voted ) {
		case BALLOT_ELIGIBLE:
			++eligible;
			break;
		case BALLOT_YES:
			++eligible;
			++yes;
			break;
		case BALLOT_NO:
			++eligible;
			++no;
			break;
		case BALLOT_PASS:
		case BALLOT_VETO:
			outcome = client->pers.voted == BALLOT_PASS ? 1 : -1;
			VoteMessage( -1, va( "%s %s the vote.", client->pers.netname,
								 outcome == 1 ? "passed" : "vetoed" ) );
			break;
		}
		if ( outcome )
			break;
	}
	if ( !outcome ) {
		if ( yes * 2 > eligible )
			outcome = 1;
		else if ( no * 2 >= eligible )
			outcome = -1;
		else {
			if ( level.voteYes != yes )
				trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes = yes ) );
			if ( level.voteNo != no )
				trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo = no ) );
			if ( (unsigned)( level.time - (unsigned)level.voteTime ) >= VOTE_TIME )
				outcome = -1;
			else if ( level.warmupTime > 0 )
				outcome = 2;
		}
	}
	if ( !outcome )
		return;
	VoteMessage( -1, outcome == 1	? "Vote passed."
					 : outcome == 2 ? "Voting time has expired."
									: "Vote failed." );
	G_ClearVote();
	if ( outcome == 1 )
		level.voteExecuteTime = level.time + 3000;
}
