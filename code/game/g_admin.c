/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.
Copyright (C) 2026 FnQL-game contributors.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later version.
===========================================================================
*/
#include "g_local.h"
#include <limits.h>
#include <stdlib.h>

void G_CommandReply( gentity_t *ent, const char *command ) {
	if ( ent ) trap_SendServerCommand( ent - g_entities, command );
	else G_Printf( "%s", command );
}

/* The table's PlayerID grammar is decimal slots, not names or SteamIDs
 * (retail 100611d0). Parse before narrowing so overflow never selects a slot. */
static gentity_t *CommandPlayer( gentity_t *caller ) {
	char text[MAX_TOKEN_CHARS];
	unsigned long long slot;
	if ( trap_Argc() < 2 ) {
		G_CommandReply(caller, "print \"Missing PlayerID (use \\players for a list)\n\"");
		return NULL;
	}
	trap_Argv(1, text, sizeof(text));
	if ( !G_ParseAccount(text, &slot) || slot >= (unsigned)level.maxclients
		|| level.clients[(int)slot].pers.connected == CON_DISCONNECTED ) {
		G_CommandReply(caller, "print \"Invalid PlayerID (use \\players for a list)\n\"");
		return NULL;
	}
	return &g_entities[(int)slot];
}

/* Retail 10061350 accepts the first letter and validates it for the mode. */
static int CommandTeam( gentity_t *caller, int argument ) {
	char text[MAX_TOKEN_CHARS];
	int team = -1;
	if ( trap_Argc() <= argument ) {
		G_CommandReply(caller, "print \"Missing TeamName\n\"");
		return -1;
	}
	trap_Argv(argument, text, sizeof(text));
	switch ( text[0] | 32 ) {
	case 's': team = TEAM_SPECTATOR; break;
	case 'r': if ( g_gametype.integer >= GT_TEAM ) team = TEAM_RED; break;
	case 'b': if ( g_gametype.integer >= GT_TEAM ) team = TEAM_BLUE; break;
	case 'f': if ( g_gametype.integer < GT_TEAM ) team = TEAM_FREE; break;
	}
	if ( team < 0 ) G_CommandReply(caller, g_gametype.integer >= GT_TEAM
		? "print \"Invalid TeamName (use R/B/S)\n\"" : "print \"Invalid TeamName (use F/S)\n\"");
	return team;
}

static qboolean CommandNumber( int argument, int *number ) {
	char text[MAX_TOKEN_CHARS], *end;
	double value;
	trap_Argv(argument, text, sizeof(text));
	value = strtod(text, &end);
	if ( end == text || *end || !(value >= INT_MIN && value <= INT_MAX) ) return qfalse;
	*number = (int)value; /* retail atof -> truncation, 100616a9 / 10062d01 */
	return qtrue;
}

static void Cmd_Lock_f( gentity_t *caller ) {
	char command[32];
	int first = TEAM_RED, last = TEAM_BLUE, team;
	qboolean locked;
	trap_Argv(0, command, sizeof(command));
	locked = !Q_stricmp(command, "lock");
	if ( trap_Argc() > 1 ) {
		first = last = CommandTeam(caller, 1);
		if ( first < 0 ) return;
	}
	for ( team = first; team <= last; ++team ) {
		if ( level.teamLocked[team] == locked ) continue;
		level.teamLocked[team] = locked;
		trap_SendServerCommand(-1, va("print \"The %s team is now %slocked\n\"", TeamName(team), locked ? "" : "un"));
	}
}

static void Cmd_Put_f( gentity_t *caller ) {
	static const char *tokens[] = { "f", "r", "b", "s" };
	gentity_t *target = CommandPlayer(caller);
	int team;
	if ( !target ) return;
	team = CommandTeam(caller, 2);
	if ( team < 0 ) return;
	if ( g_gametype.integer == GT_TOURNAMENT && team == TEAM_FREE && level.numNonSpectatorClients >= 2 ) {
		G_CommandReply(caller, level.warmupTime < 0
			? "print \"The match cannot begin until only two players are in the match.\n\""
			: "print \"A maximum of two players are allowed in the match.\n\"");
		if ( level.warmupTime >= 0 ) return;
	}
	if ( target->client->sess.sessionTeam == team ) {
		G_CommandReply(caller, va("print \"%s is already on the %s team\n\"", target->client->pers.netname, TeamName(team)));
		return;
	}
	trap_SendServerCommand(target - g_entities, va("print \"%s has moved you to the %s team\n\"",
		caller ? caller->client->pers.netname : "The server", TeamName(team)));
	G_ForceTeam(target, tokens[team]);
	CalculateRanks();
}

static void Cmd_Mute_f( gentity_t *caller ) {
	gentity_t *target = CommandPlayer(caller);
	char command[32];
	qboolean mute;
	if ( !target ) return;
	trap_Argv(0, command, sizeof(command));
	mute = !Q_stricmp(command, "mute");
	if ( mute && caller && caller->client->pers.privileges <= target->client->pers.privileges ) return;
	target->client->sess.muted = mute;
	trap_SendServerCommand(-1, va("print \"%s has been %smuted\n\"", target->client->pers.netname, mute ? "" : "un"));
}

static qboolean ChangeAccess( gentity_t *caller, gentity_t *target, int tier, qboolean temporary ) {
	if ( G_SetAccess(trap_GetSteamId64(target - g_entities), tier, temporary) ) return qtrue;
	G_CommandReply(caller, "print \"Unable to update the access list\n\"");
	return qfalse;
}

static void Cmd_Privilege_f( gentity_t *caller ) {
	gentity_t *target = CommandPlayer(caller);
	char command[32];
	int tier, current;
	if ( !target ) return;
	trap_Argv(0, command, sizeof(command));
	tier = !Q_stricmp(command, "addadmin") ? 2 : !Q_stricmp(command, "addmod") ? 1 : 0;
	current = target->client->pers.privileges;
	if ( tier ? current >= tier : current == 0 ) return;
	if ( !tier && caller && caller->client->pers.privileges <= current ) {
		G_CommandReply(caller, "print \"Can not demote someone at or above your level.\n\"");
		return;
	}
	if ( tier ) {
		if ( !ChangeAccess(caller, target, tier, qfalse) ) return;
	} else G_RemoveAccess(trap_GetSteamId64(target - g_entities));
	target->client->pers.privileges = tier;
	trap_SendServerCommand(-1, va("print \"%s %s\n\"", target->client->pers.netname,
		tier == 2 ? "has become an administrator" : tier == 1 ? "has become a moderator" : "has had their privileges removed"));
	trap_SendServerCommand(target - g_entities, va("priv %i", tier));
}

static void Cmd_Ban_f( gentity_t *caller ) {
	gentity_t *target = CommandPlayer(caller);
	char command[32];
	if ( !target ) return;
	if ( target->client->pers.privileges > 0 ) {
		G_CommandReply(caller, "print \"Can not kick admins.\n\"");
		return;
	}
	trap_Argv(0, command, sizeof(command));
	if ( ChangeAccess(caller, target, -1, Q_stricmp(command, "ban") != 0) )
		trap_DropClient(target - g_entities, "was kicked");
}

static void Cmd_Unban_f( gentity_t *caller ) {
	char text[MAX_TOKEN_CHARS];
	unsigned long long id;
	if ( trap_Argc() < 2 ) return;
	trap_Argv(1, text, sizeof(text));
	if ( !G_ParseAccount(text, &id) ) { G_CommandReply(caller, "print \"Invalid account\n\""); return; }
	G_RemoveAccess(id);
	G_CommandReply(caller, va("print \"%s has been unbanned\n\"", G_SteamIdString(id)));
}

static void Cmd_ListAccess_f( gentity_t *caller ) {
	int page = 1;
	if ( trap_Argc() > 1 && !CommandNumber(1, &page) ) page = 1;
	G_ListAccess(caller, page > 0 ? page - 1 : 0);
}

static void Cmd_OpSay_f( gentity_t *caller ) {
	char text[MAX_SAY_TEXT];
	int i;
	if ( trap_Argc() < 2 ) return;
	Q_strncpyz(text, ConcatArgs(1), sizeof(text));
	for ( i = 0; text[i]; ++i ) if ( text[i] == '"' || text[i] == '\r' || text[i] == '\n' ) text[i] = ' ';
	trap_SendServerCommand(-1, va("print \"<@%s^7> %s\n\"", caller ? caller->client->pers.netname : "server", text));
}

static void Cmd_AddScore_f( gentity_t *caller ) {
	gentity_t *target = CommandPlayer(caller);
	int score;
	if ( !target || !CommandNumber(2, &score) || !score ) return;
	if ( (score > 0 && target->client->ps.persistant[PERS_SCORE] > INT_MAX - score)
		|| (score < 0 && target->client->ps.persistant[PERS_SCORE] < INT_MIN - score) ) return;
	if ( g_gametype.integer == GT_TEAM ) {
		int team = target->client->ps.persistant[PERS_TEAM];
		if ( team < 0 || team >= TEAM_NUM_TEAMS ) return;
		if ( (score > 0 && level.teamScores[team] > INT_MAX - score)
			|| (score < 0 && level.teamScores[team] < INT_MIN - score) ) return;
	}
	AddScore(target, target->r.currentOrigin, score);
	trap_SendServerCommand(-1, "pcp \"Player score adjusted.\n\"");
	trap_SendServerCommand(-1, va("print \"%s has had their score %s by %i.\n\"", target->client->pers.netname,
		score > 0 ? "increased" : "decreased", score));
}

static void Cmd_AddTeamScore_f( gentity_t *caller ) {
	int team = CommandTeam(caller, 1), score;
	if ( team < 0 || !CommandNumber(2, &score) || !score ) return;
	if ( (score > 0 && level.teamScores[team] > INT_MAX - score)
		|| (score < 0 && level.teamScores[team] < INT_MIN - score) ) return;
	AddTeamScore(team, score);
	trap_SendServerCommand(-1, "pcp \"Team score adjusted.\n\"");
	trap_SendServerCommand(-1, va("print \"%s has had their score %s by %i.\n\"",
		team == TEAM_RED ? "Red Team" : team == TEAM_BLUE ? "Blue Team" : team == TEAM_FREE ? "Team Free" : "Spectator",
		score > 0 ? "increased" : "decreased", score));
}

static void Cmd_SetMatchTime_f( gentity_t *caller ) {
	int seconds;
	(void)caller;
	if ( !CommandNumber(1, &seconds) || seconds < 0 || seconds > INT_MAX / 1000 ) return;
	if ( level.time < INT_MIN + seconds * 1000 ) return;
	level.startTime = level.time - seconds * 1000;
	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));
	trap_SendServerCommand(-1, va("pcp \"Match time has been set to %i:%02i.\n\"", seconds / 60, seconds % 60));
}

/*
=================
G_MatchCommandAllowed

Retail qagamex86 0x10061090: whether a match command may run now.  At the
intermission it needs MATCHCMD_INTERMISSION and during the warmup
MATCHCMD_WARMUP; during a match it needs MATCHCMD_MATCH, and a round-based
mode refuses it while counting down to a round.
=================
*/
#define	MATCHCMD_WARMUP			1
#define	MATCHCMD_MATCH			2
#define	MATCHCMD_INTERMISSION	4

static qboolean G_MatchCommandAllowed( gentity_t *ent, int flags ) {
	const char	*refusal = NULL;

	if ( level.intermissiontime ) {
		if ( !( flags & MATCHCMD_INTERMISSION ) ) {
			refusal = "print \"This command can not be used during intermission\n\"";
		}
	} else if ( level.warmupTime ) {
		if ( !( flags & MATCHCMD_WARMUP ) ) {
			refusal = "print \"This command can not be used during warmup\n\"";
		}
	} else if ( !( flags & MATCHCMD_MATCH ) ) {
		refusal = "print \"This command can not be used while a match is in progress\n\"";
	} else if ( G_IsRoundBasedGametype( g_gametype.integer ) && G_RoundCountingDown() ) {
		refusal = "print \"This command can not be used during a round countdown\n\"";
	}

	if ( refusal ) {
		G_CommandReply( ent, refusal );
		return qfalse;
	}
	return qtrue;
}


/*
=================
G_StartTimeout

Retail qagamex86 0x10061db0: the match stops for the given seconds, or
until a time in when there are none (a pause), and everyone is told who
called it.
=================
*/
static void G_StartTimeout( gentity_t *ent, int seconds ) {
	if ( !G_MatchCommandAllowed( ent, MATCHCMD_MATCH ) ) {
		return;
	}

	if ( seconds < 0 || seconds > (INT_MAX - (long long)level.time) / 1000 ) return;
	level.timeoutStartTime = level.time;
	level.timeoutEndTime = seconds ? level.time + seconds * 1000 : 0;
	level.timeoutCaller = ent ? (int)(ent - g_entities) : -1;
	level.timeoutByServer = ent == NULL;
	trap_SetConfigstring( CS_TIMEOUT_START_TIME, va( "%i", level.timeoutStartTime ) );
	trap_SetConfigstring( CS_TIMEOUT_END_TIME, va( "%i", level.timeoutEndTime ) );
	if ( seconds ) {
		trap_SendServerCommand( -1, va( "pcp \"%s has called timeout (%ds)\n\"", ent ? ent->client->pers.netname : "The server", seconds ) );
	} else {
		trap_SendServerCommand( -1, va( "pcp \"%s has paused the match\n\"", ent ? ent->client->pers.netname : "The server" ) );
	}
}


/*
=================
Cmd_Timeout_f

Retail qagamex86 0x100621c0: a player in the game stops the match for
g_timeoutLen seconds.  g_timeoutCount limits the calls, per team in the
team modes (the counts left go out in configstrings) and per player
otherwise.  The call is counted before the match state is checked, as in
retail.
=================
*/
static void Cmd_Timeout_f( gentity_t *ent ) {
	gclient_t	*client;
	if ( !ent ) { G_StartTimeout( NULL, g_timeoutLen.integer ); return; }
	client = ent->client;
	const int	team = client->sess.sessionTeam;

	if ( team == TEAM_SPECTATOR || level.timeoutStartTime ) {
		return;
	}
	if ( !g_timeoutCount.integer ) {
		G_CommandReply( ent, "print \"Timeouts are not enabled on this server.\n\"" );
		return;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		if ( team != TEAM_RED && team != TEAM_BLUE ) {
			return;
		}
		if ( level.timeoutsUsed[team - TEAM_RED] >= g_timeoutCount.integer ) {
			G_CommandReply( ent, "print \"Your team has no timeouts left to call\n\"" );
			return;
		}
		level.timeoutsUsed[team - TEAM_RED]++;
		trap_SetConfigstring( CS_RED_TIMEOUTS_LEFT, va( "%d", g_timeoutCount.integer - level.timeoutsUsed[0] ) );
		trap_SetConfigstring( CS_BLUE_TIMEOUTS_LEFT, va( "%d", g_timeoutCount.integer - level.timeoutsUsed[1] ) );
	} else {
		if ( client->pers.timeoutsUsed >= g_timeoutCount.integer ) {
			G_CommandReply( ent, "print \"You have no timeouts left to call\n\"" );
			return;
		}
		client->pers.timeoutsUsed++;
	}

	G_StartTimeout( ent, g_timeoutLen.integer );
}


/*
=================
Cmd_Timein_f

Retail qagamex86 0x10062330: the player who called the timeout, or a
moderator, ends it five seconds from now; with less than that left, or
for a timeout the server called, nothing happens.  G_CheckTimein restarts
the match when the time comes.
=================
*/
static void Cmd_Timein_f( gentity_t *ent ) {
	if ( !level.timeoutStartTime ) {
		return;
	}
	if ( level.time > INT_MAX - 5000 ) return;
	if ( level.timeoutEndTime && level.timeoutEndTime < level.time + 5000 ) {
		return;
	}
	if ( ent && ( level.timeoutByServer
		|| ( (int)(ent - g_entities) != level.timeoutCaller && ent->client->pers.privileges < 1 ) ) ) {
		G_CommandReply( ent, "print \"You did not call the current timeout\n\"" );
		return;
	}

	level.timeoutEndTime = level.time + 5000;
	trap_SetConfigstring( CS_TIMEOUT_END_TIME, va( "%i", level.timeoutEndTime ) );
	trap_SendServerCommand( -1, va( "pcp \"%s has called timein\n\"", ent ? ent->client->pers.netname : "The server" ) );
}


/*
=================
Cmd_Players_f

Retail qagamex86 0x10060ee0: every connected player's slot, Steam id,
privilege (moderator M, administrator A) and name.
=================
*/
static void Cmd_AllReady_f( gentity_t *ent ) {
	int i;
	if ( !G_MatchCommandAllowed(ent, MATCHCMD_WARMUP) ) return;
	if ( g_gametype.integer == GT_TOURNAMENT && level.numNonSpectatorClients != 2 ) {
		G_CommandReply(ent, "print \"Allready may not be used until two players are in the match.\n\"");
		return;
	}
	for ( i = 0; i < level.maxclients; ++i )
		if ( g_entities[i].inuse && level.clients[i].pers.connected == CON_CONNECTED )
			level.clients[i].pers.readyToPlay = 1;
}

static void Cmd_Abort_f( gentity_t *ent ) {
	if ( !G_MatchCommandAllowed(ent, MATCHCMD_MATCH) ) return;
	if ( level.timeoutStartTime ) {
		G_CommandReply(ent, "print \"Match is currently paused. You must unpause before aborting the match.\n\"");
		return;
	}
	trap_SendServerCommand(-1, va("pcp \"%s has aborted the match\n\"", ent ? ent->client->pers.netname : "The server"));
	level.warmupTime = -1;
	trap_SetConfigstring(CS_WARMUP, "\\time\\-1");
	G_AutoAction();
	trap_Cvar_Set("g_gameState", "PRE_GAME");
	trap_SendConsoleCommand(EXEC_APPEND, "map_restart 3\n");
}

static void Cmd_Pause_f( gentity_t *ent ) {
	if ( !level.timeoutStartTime ) G_StartTimeout(ent, 0);
	else if ( ent && level.timeoutByServer && level.timeoutCaller != (int)(ent - g_entities) ) {
		level.timeoutCaller = ent - g_entities;
		G_CommandReply(ent, "print \"You have taken ownership of the current timeout\n\"");
	}
}

static void Cmd_Players_f( gentity_t *ent ) {
	static const char	privilegeMarks[] = " MA*";
	gclient_t			*cl;
	int					i;

	for ( i = 0; i < level.maxclients; i++ ) {
		cl = &level.clients[i];
		if ( !g_entities[i].inuse || cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		// retail prints the id with %llu, which the game's formatter lacks
		G_CommandReply( ent, va( "print \"%2d %s %c %s\n\"", i, G_SteamIdString( trap_GetSteamId64( i ) ),
			privilegeMarks[cl->pers.privileges & 3], cl->pers.netname ) );
	}
}


/*
=================
G_AdminCommand

Retail qagamex86 0x10062e20 looks every client command up in this table
(0x10080750) before the others.  A player below a command's privilege (1
moderator, 2 administrator) is told so; retail's rcon entry has no
handler and falls through. ConsoleCommand 0x10066ef2 invokes this table
with a null caller; that context is explicit throughout the handlers.
=================
*/
typedef struct {
	int			privilege;
	const char	*name;
	void		(*handler)( gentity_t *ent );
	qboolean	retailHandler;		// rcon has none and falls through
	const char	*help;
} adminCommand_t;

static void Cmd_AdminHelp_f( gentity_t *ent );

static const adminCommand_t adminCommands[] = {
	{ 0, "?", Cmd_AdminHelp_f, qtrue, "" },
	{ 0, "players", Cmd_Players_f, qtrue, "^3 show currently connected players and their status" },
	{ 0, "timeout", Cmd_Timeout_f, qtrue, "^3 call a time out, temporarily pausing the game" },
	{ 0, "timein", Cmd_Timein_f, qtrue, "^3 call a time in, unpausing the game" },
	{ 1, "allready", Cmd_AllReady_f, qtrue, "^3 force all players to 'ready' and start the match" },
	{ 1, "pause", Cmd_Pause_f, qtrue, "^3 pause the current match indefinitely" },
	{ 1, "unpause", Cmd_Timein_f, qtrue, "^3 unpause the current match" },
	{ 1, "lock", Cmd_Lock_f, qtrue, " [team]^3 stops players from joining the team" },
	{ 1, "unlock", Cmd_Lock_f, qtrue, " [team]^3 allows players to join the team" },
	{ 1, "put", Cmd_Put_f, qtrue, " <playerid> <team>^3 move a player to a team, use (R/B/S) or (F/S)" },
	{ 1, "mute", Cmd_Mute_f, qtrue, " <playerid>^3 temporarily mutes a player on the server" },
	{ 1, "unmute", Cmd_Mute_f, qtrue, " <playerid>^3 restores a muted player's ability to chat" },
	{ 1, "tempban", Cmd_Ban_f, qtrue, " <playerid>^3 temporarily kicks a player from the server for the match" },
	{ 1, "ban", Cmd_Ban_f, qtrue, " <playerid>^3 permenantly bans a player from the server" },
	{ 1, "listaccess", Cmd_ListAccess_f, qtrue, " <page>^3 lists the accounts with access " },
	{ 1, "unban", Cmd_Unban_f, qtrue, " <account>^3 removes a player from the banlist" },
	{ 1, "opsay", Cmd_OpSay_f, qtrue, " <message> ^3send a message to everyone on the server" },
	{ 2, "addadmin", Cmd_Privilege_f, qtrue, " <playerid>^3 give a player admin privileges" },
	{ 2, "addmod", Cmd_Privilege_f, qtrue, " <playerid>^3 give a player moderator privileges" },
	{ 2, "demote", Cmd_Privilege_f, qtrue, " <playerid>^3 remove a player's privileges" },
	{ 1, "abort", Cmd_Abort_f, qtrue, "^3 abandon the current game and go back to warmup" },
	{ 1, "addscore", Cmd_AddScore_f, qtrue, " <playerid> <score>^3 add points to a player's score" },
	{ 1, "addteamscore", Cmd_AddTeamScore_f, qtrue, " <team> <score>^3 add points to a team's score, use (R/B)" },
	{ 1, "setmatchtime", Cmd_SetMatchTime_f, qtrue, " <time> ^3set match time in seconds" },
	{ 2, "rcon", NULL, qfalse, " <command> ^3run command on server" },
};


/*
=================
Cmd_AdminHelp_f

Retail qagamex86 0x10062d60 ("?"): the commands the player may use, with
their help.
=================
*/
static void Cmd_AdminHelp_f( gentity_t *ent ) {
	int		i;

	for ( i = 0; i < ARRAY_LEN( adminCommands ); i++ ) {
		if ( !ent || ent->client->pers.privileges >= adminCommands[i].privilege ) {
			G_CommandReply( ent, va( "print \"%s%s\n\"", adminCommands[i].name, adminCommands[i].help ) );
		}
	}
}


qboolean G_AdminCommand( gentity_t *ent, const char *cmd ) {
	const adminCommand_t	*command;
	int						i;

	for ( i = 0; i < ARRAY_LEN( adminCommands ); i++ ) {
		command = &adminCommands[i];
		if ( Q_stricmp( cmd, command->name ) ) {
			continue;
		}
		if ( !command->retailHandler ) {
			return qfalse;
		}
		if ( ent && ent->client->pers.privileges < command->privilege ) {
			G_CommandReply( ent,
				"print \"You do not have the privileges required to use this command\n\"" );
			return qtrue;
		}
		if ( !command->handler ) {
			return qfalse;	// only entries with a game handler execute here
		}
		command->handler( ent );
		return qtrue;
	}
	return qfalse;
}


