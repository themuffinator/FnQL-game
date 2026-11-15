/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// Match statistics reports.
//
// Retail qagame describes a match to the engine as eleven events and one
// closing report (qagamex86 0x100716a0-0x10078931).  The engine publishes
// them on its stats socket, where community tools read them, and derives the
// players' Steam statistics from them.  Retail hands them over as JsonCpp
// objects (imports 185 and 186), which only its own runtime can build;
// FnQL-game writes the same documents as JSON text for FnQL's text imports
// (g_native.c) and reports nothing on an engine without them.
//
// Each builder cites the retail builder it was written from.  The keys,
// their nesting and their JSON types are a compatibility surface: gate
// "stats-events" (scripts/stats_probe.py) holds them to
// docs/contract/stats_events.json and to the retail module's own output.

#include "g_local.h"
#include "g_json.h"

#define STATS_DOCUMENT_SIZE	24576	// the engine accepts 32768

static char			statsDocument[STATS_DOCUMENT_SIZE];
static char			statsScratch[STATS_DOCUMENT_SIZE];
static jsonWriter_t	stats;

/*
======================================================================

  retail's vocabularies

======================================================================
*/

// 0x100711b0, by means of death; the rest are "OTHER_DEATH"
static const char *const statsModNames[] = {
	NULL, "SHOTGUN", "GAUNTLET", "MACHINEGUN", "GRENADE", "GRENADE_SPLASH", "ROCKET",
	"ROCKET_SPLASH", "PLASMA", "PLASMA_SPLASH", "RAILGUN", "LIGHTNING", "BFG", "BFG_SPLASH",
	"WATER", "SLIME", "LAVA", "CRUSH", "TELEFRAG", "FALLING", "SUICIDE", "LASER", "HURT",
	"NAILGUN", "CHAINGUN", "PROXMINE", "KAMIKAZE", "JUICED", "GRAPPLE", "SWITCHTEAM", "THAW",
	"LIGHTNING_DISCHARGE", "HMG", "RAILGUN_HEADSHOT"
};
typedef char statsModNamesCheck[ ( MOD_RAILGUN_HEADSHOT == 33 && MOD_SWITCHTEAM == 29 && MOD_NAIL == 23 ) ? 1 : -1 ];

// 0x100713e0, by weapon; the grapple and everything unknown are "OTHER_WEAPON"
static const char *const statsWeaponNames[WP_NUM_WEAPONS] = {
	NULL, "GAUNTLET", "MACHINEGUN", "SHOTGUN", "GRENADE", "ROCKET", "LIGHTNING", "RAILGUN",
	"PLASMA", "BFG", NULL, "NAILGUN", "PROXMINE", "CHAINGUN", "HMG"
};
typedef char statsWeaponNamesCheck[ ( WP_GRAPPLING_HOOK == 10 && WP_HEAVY_MACHINEGUN == 14 ) ? 1 : -1 ];

// 0x10071110, by gametype
static const char *const statsGametypeNames[GT_MAX_GAME_TYPE] = {
	"FFA", "DUEL", "RACE", "TDM", "CA", "CTF", "1FCTF", "OBELISK", "HARVESTER", "FT", "DOM", "AD", "RR"
};

// 0x10071480, by powerup slot; slot 15 (frozen) and everything unknown are "OTHER_POWERUP"
static const char *const statsPowerupNames[PW_NUM_POWERUPS] = {
	NULL, "RED_FLAG", "BLUE_FLAG", "NEUTRAL_FLAG", "QUAD", "BATTLESUIT", "HASTE", "INVIS", "REGEN",
	"FLIGHT", "INVULNERABILITY", "SCOUT", "GUARD", "DOUBLER", "ARMOR_REGEN", NULL
};
typedef char statsPowerupNamesCheck[ ( PW_QUAD == 4 && PW_ARMORREGEN == 14 && PW_FROZEN == 15 ) ? 1 : -1 ];

// by holdable tag (inline switches, e.g. 0x10074304); the flight item is "OTHER_HOLDABLE"
static const char *const statsHoldableNames[HI_NUM_HOLDABLE] = {
	NULL, "TELEPORTER", "MEDKIT", "KAMIKAZE", "PORTAL", "INVULNERABILITY", NULL
};
typedef char statsHoldableNamesCheck[ ( HI_TELEPORTER == 1 && HI_INVULNERABILITY == 5 ) ? 1 : -1 ];

static const char *Stats_ModName( int mod ) {
	if ( mod <= 0 || mod >= (int)ARRAY_LEN( statsModNames ) ) {
		return "OTHER_DEATH";
	}
	return statsModNames[mod];
}

static const char *Stats_WeaponName( int weapon ) {
	if ( weapon <= 0 || weapon >= WP_NUM_WEAPONS || !statsWeaponNames[weapon] ) {
		return "OTHER_WEAPON";
	}
	return statsWeaponNames[weapon];
}

static const char *Stats_GametypeName( int gametype ) {
	if ( gametype < 0 || gametype >= GT_MAX_GAME_TYPE ) {
		return "OTHER_GAME";
	}
	return statsGametypeNames[gametype];
}

static const char *Stats_PowerupName( int powerup ) {
	if ( powerup <= 0 || powerup >= PW_NUM_POWERUPS || !statsPowerupNames[powerup] ) {
		return "OTHER_POWERUP";
	}
	return statsPowerupNames[powerup];
}

static const char *Stats_HoldableName( int tag ) {
	if ( tag <= 0 || tag >= HI_NUM_HOLDABLE || !statsHoldableNames[tag] ) {
		return "OTHER_HOLDABLE";
	}
	return statsHoldableNames[tag];
}

// the builders spell the team out inline: 1 RED, 2 BLUE, 3 SPECTATOR, else FREE
static const char *Stats_TeamName( int team ) {
	switch ( team ) {
	case TEAM_RED:			return "RED";
	case TEAM_BLUE:			return "BLUE";
	case TEAM_SPECTATOR:	return "SPECTATOR";
	default:				return "FREE";
	}
}

/*
======================================================================

  shared pieces

======================================================================
*/

static void Stats_Begin( void ) {
	JSON_Init( &stats, statsDocument, statsScratch, sizeof( statsDocument ) );
	JSON_BeginObject( &stats, NULL );
}

// Sends the open document: player events carry the player's Steam id, the
// rest go without one.  (Retail's third argument, the player's stats key,
// is not read by the engine.)
static void Stats_SendEvent( const gclient_t *client, const char *name ) {
	const char	*document;

	JSON_End( &stats );
	document = JSON_Finish( &stats );
	if ( !document ) {
		G_Printf( S_COLOR_YELLOW "Stats: the %s document did not fit and was not reported\n", name );
		return;
	}
	trap_StatsEventJson( client ? client->pers.steamId : 0, name, document );
}

// TIME is whole seconds into the level, WARMUP is true from the wait for
// players to the end of the countdown, and MATCH_GUID is read back from its
// configstring for every document.
static void Stats_MatchGuid( void ) {
	char	guid[64];

	trap_GetConfigstring( CS_MATCH_GUID, guid, sizeof( guid ) );
	JSON_String( &stats, "MATCH_GUID", guid );
}

static void Stats_Common( void ) {
	JSON_Int( &stats, "TIME", ( level.time - level.startTime ) / 1000 );
	JSON_Bool( &stats, "WARMUP", level.warmupTime != 0 );
	Stats_MatchGuid();
}

// always a string: "0" for a bot, the decimal id otherwise
static void Stats_SteamId( const gentity_t *ent ) {
	if ( ent->r.svFlags & SVF_BOT ) {
		JSON_String( &stats, "STEAM_ID", "0" );
	} else {
		JSON_String( &stats, "STEAM_ID", G_SteamIdString( ent->client->pers.steamId ) );
	}
}

static void Stats_Identity( const gentity_t *ent ) {
	JSON_String( &stats, "NAME", ent->client->pers.netname );
	Stats_SteamId( ent );
}

/*
================
G_StatsNoteScorer

The name the report gives for the match's first and latest scorers: the
"name" the player sent, without colour codes, cut to 39 characters (retail
0x10046620 for every score, 0x10073e7c for a round mode's kills).
================
*/
static void Stats_ScorerName( const gentity_t *ent, char *name, int size ) {
	char	userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
	Q_strncpyz( name, Info_ValueForKey( userinfo, "name" ), size < MAX_NETNAME ? size : MAX_NETNAME );
	Q_CleanStr( name );
}

void G_StatsNoteScorer( const gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return;
	}
	if ( !level.firstScorer[0] ) {
		Stats_ScorerName( ent, level.firstScorer, sizeof( level.firstScorer ) );
	}
	Stats_ScorerName( ent, level.lastScorer, sizeof( level.lastScorer ) );
}

void G_StatsNoteTeamScorer( const gentity_t *ent ) {
	if ( ent && ent->client ) {
		Stats_ScorerName( ent, level.lastTeamScorer, sizeof( level.lastTeamScorer ) );
	}
}

/*
======================================================================

  bookkeeping the reports read

======================================================================
*/

/*
================
G_StatsAccountTime

Retail 0x10078960, from the living player's think: the msec since the last
call count as play time outside the intermission and, for a player who is
not dead, as time holding the current weapon.  The stamp does not move while
the player is at the intermission.
================
*/
void G_StatsAccountTime( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	const int	elapsed = level.time - client->stats.stamp;

	if ( client->ps.pm_type == PM_INTERMISSION ) {
		return;
	}
	client->stats.playMsec += elapsed;
	if ( client->sess.sessionTeam != TEAM_SPECTATOR && client->ps.pm_type != PM_DEAD
		&& client->ps.weapon >= 0 && client->ps.weapon < WP_MAX_WEAPONS ) {
		client->stats.weaponTime[client->ps.weapon] += elapsed;
	}
	client->stats.stamp = level.time;
}

/*
================
G_StatsLeadChange

The tail of retail's rank calculation (0x10056587): when the lead last
changed hands.  The first call only starts the clock, with the level time;
after that a change stores the time into the match.  In the team modes the
leader is the side ahead, and a tie changes nothing; elsewhere it is the top
player, once nobody shares the top score.
================
*/
void G_StatsLeadChange( void ) {
	int		leader;

	if ( level.lastLeadChangeTime == 0 ) {
		level.leadHolder = -1;
		level.lastLeadChangeTime = level.time;
		return;
	}
	if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_RED_ROVER ) {
		if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
			return;
		}
		leader = level.teamScores[TEAM_RED] <= level.teamScores[TEAM_BLUE] ? 1 : 0;
	} else {
		if ( level.numPlayingClients <= 0 ) {
			return;
		}
		leader = level.sortedClients[0];
		if ( level.numPlayingClients > 1 && level.clients[leader].ps.persistant[PERS_SCORE]
			== level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] ) {
			return;
		}
	}
	if ( leader != level.leadHolder ) {
		level.leadHolder = leader;
		level.lastLeadChangeTime = level.time - level.startTime;
	}
}

/*
================
Stats_Ranks

A player's place among the players and within its team, as retail's rank
calculation stores them (0x10056284): dense ranks from 0 over the sorted
clients, equal scores sharing a rank and marked tied.
================
*/
static void Stats_Ranks( const gclient_t *client, int *rank, qboolean *tied, int *teamRank, qboolean *teamTied ) {
	const int	score = client->ps.persistant[PERS_SCORE];
	const int	team = client->sess.sessionTeam;
	int			i, place = -1, teamPlace = -1, previous = 0, teamPrevious = 0;

	*rank = *teamRank = 0;
	*tied = *teamTied = qfalse;
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		const gclient_t	*other = &level.clients[level.sortedClients[i]];
		const int		otherScore = other->ps.persistant[PERS_SCORE];

		if ( other->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		// each new score down the sorted list is the next rank
		if ( place < 0 || otherScore != previous ) {
			place++;
		}
		previous = otherScore;
		if ( other == client ) {
			*rank = place;
		} else if ( otherScore == score ) {
			*tied = qtrue;
		}

		if ( other->sess.sessionTeam == team ) {
			if ( teamPlace < 0 || otherScore != teamPrevious ) {
				teamPlace++;
			}
			teamPrevious = otherScore;
			if ( other == client ) {
				*teamRank = teamPlace;
			} else if ( otherScore == score ) {
				*teamTied = qtrue;
			}
		}
	}
}

/*
======================================================================

  events

======================================================================
*/

/*
================
G_StatsReportConnect / G_StatsReportDisconnect

Retail 0x10078450 and 0x100786d0.  The caller decides when: a first
connection outside training, and every disconnect.
================
*/
static void Stats_ConnectionEvent( const gentity_t *ent, const char *name ) {
	if ( !trap_StatsAvailable() || !ent || !ent->client ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	Stats_Identity( ent );
	Stats_SendEvent( ent->client, name );
}

void G_StatsReportConnect( const gentity_t *ent ) {
	Stats_ConnectionEvent( ent, "PLAYER_CONNECT" );
}

void G_StatsReportDisconnect( const gentity_t *ent ) {
	Stats_ConnectionEvent( ent, "PLAYER_DISCONNECT" );
}

/*
================
G_StatsReportSwitchTeam

Retail 0x10072350, the last thing a team change does.  The player sits in
an object that really is named KILLER.
================
*/
void G_StatsReportSwitchTeam( const gentity_t *ent, int oldTeam, int newTeam ) {
	if ( !trap_StatsAvailable() || !ent || !ent->client ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	JSON_BeginObject( &stats, "KILLER" );
	Stats_Identity( ent );
	JSON_String( &stats, "OLD_TEAM", Stats_TeamName( oldTeam ) );
	JSON_String( &stats, "TEAM", Stats_TeamName( newTeam ) );
	JSON_End( &stats );
	Stats_SendEvent( ent->client, "PLAYER_SWITCHTEAM" );
}

// one side of a kill (retail 0x100726e0): weapon is the killer's by the
// means of death, the victim's by what it holds
static void Stats_KillPlayer( const char *key, const gentity_t *ent, int weapon, qboolean victim ) {
	const gclient_t	*client = ent->client;
	char			userinfo[MAX_INFO_STRING];
	float			speed;
	int				i, holdable;
	qboolean		any = qfalse;

	JSON_BeginObject( &stats, key );
	Stats_Identity( ent );
	JSON_String( &stats, "WEAPON", Stats_WeaponName( weapon ) );
	JSON_Bool( &stats, "AIRBORNE", client->ps.groundEntityNum == ENTITYNUM_NONE || client->ps.powerups[PW_FLIGHT] != 0 );
	// the victim's are what it had when an enemy last hurt it
	JSON_Int( &stats, "HEALTH", victim ? client->stats.lastHealth : client->ps.stats[STAT_HEALTH] );
	JSON_Int( &stats, "ARMOR", victim ? client->stats.lastArmor : client->ps.stats[STAT_ARMOR] );
	// the killer's for the weapon that killed, the victim's for the one in hand
	i = victim ? client->ps.weapon : weapon;
	JSON_Int( &stats, "AMMO", ( i >= 0 && i < MAX_WEAPONS ) ? client->ps.ammo[i] : 0 );
	JSON_Int( &stats, "TEAM", client->sess.sessionTeam );
	// horizontal speed, in single precision as retail computes it
	speed = (float)sqrt( client->ps.velocity[0] * client->ps.velocity[0] + client->ps.velocity[1] * client->ps.velocity[1] );
	JSON_Real( &stats, "SPEED", speed );
	JSON_Bool( &stats, "SUBMERGED", ent->waterlevel != 0 && ( ent->watertype & CONTENTS_WATER ) != 0 );
	JSON_BeginObject( &stats, "POSITION" );
	JSON_Real( &stats, "X", client->ps.origin[0] );
	JSON_Real( &stats, "Y", client->ps.origin[1] );
	JSON_Real( &stats, "Z", client->ps.origin[2] );
	JSON_End( &stats );
	JSON_BeginObject( &stats, "VIEW" );
	JSON_Real( &stats, "X", client->ps.viewangles[0] );
	JSON_Real( &stats, "Y", client->ps.viewangles[1] );
	JSON_Real( &stats, "Z", client->ps.viewangles[2] );
	JSON_End( &stats );

	// the powerups proper (flags are not listed); none at all is a null
	for ( i = PW_QUAD; i < PW_NUM_POWERUPS; i++ ) {
		if ( client->ps.powerups[i] ) {
			if ( !any ) {
				JSON_BeginArray( &stats, "POWERUPS" );
				any = qtrue;
			}
			JSON_String( &stats, NULL, Stats_PowerupName( i ) );
		}
	}
	if ( any ) {
		JSON_End( &stats );
	} else {
		JSON_Null( &stats, "POWERUPS" );
	}

	holdable = client->ps.stats[STAT_HOLDABLE_ITEM];
	if ( holdable > 0 && holdable < bg_numItems ) {
		JSON_String( &stats, "HOLDABLE", Stats_HoldableName( bg_itemlist[holdable].giTag ) );
	} else {
		JSON_Null( &stats, "HOLDABLE" );
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
		JSON_Bool( &stats, "BOT", qtrue );
		JSON_Real( &stats, "BOT_SKILL", atof( Info_ValueForKey( userinfo, "skill" ) ) );
	} else {
		JSON_Bool( &stats, "BOT", qfalse );
		JSON_Null( &stats, "BOT_SKILL" );
	}
	if ( victim ) {
		JSON_Int( &stats, "STREAK", client->stats.killStreak );
	}
	JSON_End( &stats );
}

/*
================
G_StatsReportKill

Retail 0x100726e0: one document, sent as PLAYER_KILL for the killer (when a
player did it) and then as PLAYER_DEATH for the victim.  Called from the
death routine before the victim's streak is reset and before any scoring.
A world kill has a null KILLER; SUICIDE means the victim did it itself.
================
*/
void G_StatsReportKill( const gentity_t *victim, const gentity_t *attacker, int mod ) {
	const int		weapon = G_WeaponForMod( mod );
	const qboolean	byPlayer = attacker && attacker->client;
	int				i, alive[TEAM_NUM_TEAMS], dead[TEAM_NUM_TEAMS];
	int				side, other;
	const char		*document;

	if ( !trap_StatsAvailable() || !victim || !victim->client ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	JSON_String( &stats, "MOD", Stats_ModName( mod ) );
	// teammates only where there are teams (two spectators also count as a team)
	JSON_Bool( &stats, "TEAMKILL", byPlayer && attacker != victim
		&& attacker->client->sess.sessionTeam == victim->client->sess.sessionTeam
		&& ( g_gametype.integer >= GT_TEAM || attacker->client->sess.sessionTeam == TEAM_SPECTATOR ) );
	JSON_Bool( &stats, "SUICIDE", attacker == victim );

	if ( byPlayer ) {
		Stats_KillPlayer( "KILLER", attacker, weapon, qfalse );
	} else {
		JSON_Null( &stats, "KILLER" );
	}
	Stats_KillPlayer( "VICTIM", victim, victim->s.weapon, qtrue );

	// the round, in the round modes; Attack and Defend counts both turns
	switch ( g_gametype.integer ) {
	case GT_ATTACK_DEFEND:
		JSON_Int( &stats, "ROUND", level.adTurn + 2 * level.roundNumber );
		break;
	case GT_CLAN_ARENA:
	case GT_FREEZE:
	case GT_RED_ROVER:
		JSON_Int( &stats, "ROUND", level.roundNumber );
		break;
	default:
		JSON_Null( &stats, "ROUND" );
		break;
	}

	// who is left standing on each side (0x10071520); the victim is already down
	if ( g_gametype.integer >= GT_TEAM ) {
		memset( alive, 0, sizeof( alive ) );
		memset( dead, 0, sizeof( dead ) );
		for ( i = 0; i < level.maxclients; i++ ) {
			const gclient_t	*cl = &level.clients[i];

			if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam < 0 || cl->sess.sessionTeam >= TEAM_NUM_TEAMS ) {
				continue;
			}
			if ( cl->ps.pm_type == PM_NORMAL ) {
				alive[cl->sess.sessionTeam]++;
			} else {
				dead[cl->sess.sessionTeam]++;
			}
		}
		side = victim->client->sess.sessionTeam == TEAM_RED ? TEAM_RED : TEAM_BLUE;
		other = side == TEAM_RED ? TEAM_BLUE : TEAM_RED;
		JSON_Int( &stats, "TEAM_ALIVE", alive[side] );
		JSON_Int( &stats, "TEAM_DEAD", dead[side] );
		JSON_Int( &stats, "OTHER_TEAM_ALIVE", alive[other] );
		JSON_Int( &stats, "OTHER_TEAM_DEAD", dead[other] );
	} else {
		JSON_Null( &stats, "TEAM_ALIVE" );
		JSON_Null( &stats, "TEAM_DEAD" );
		JSON_Null( &stats, "OTHER_TEAM_ALIVE" );
		JSON_Null( &stats, "OTHER_TEAM_DEAD" );
	}

	JSON_End( &stats );
	document = JSON_Finish( &stats );
	if ( !document ) {
		G_Printf( S_COLOR_YELLOW "Stats: the kill document did not fit and was not reported\n" );
		return;
	}
	if ( byPlayer ) {
		trap_StatsEventJson( attacker->client->pers.steamId, "PLAYER_KILL", document );
	}
	trap_StatsEventJson( victim->client->pers.steamId, "PLAYER_DEATH", document );
}

/*
================
G_StatsReportMedal

Retail 0x100716a0, right after an award is counted: total is the player's
count of that award including this one.
================
*/
void G_StatsReportMedal( const gentity_t *ent, const char *medal, int total ) {
	if ( !trap_StatsAvailable() || !ent || !ent->client ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	Stats_Identity( ent );
	JSON_String( &stats, "MEDAL", medal );
	JSON_Int( &stats, "TOTAL", total );
	Stats_SendEvent( ent->client, "PLAYER_MEDAL" );
}

/*
================
G_StatsReportFlagStatus

Retail 0x10071aa0, from the flag status setter before it looks whether the
status changes, so every call reports.  The player is whoever took the flag.
Both carrier statuses of the neutral flag read FLAG_TAKEN.
================
*/
void G_StatsReportFlagStatus( const gentity_t *player, int team, int status ) {
	const char	*name;

	if ( !trap_StatsAvailable() ) {
		return;
	}
	switch ( status ) {
	case FLAG_ATBASE:		name = "FLAG_ATBASE"; break;
	case FLAG_DROPPED:		name = "FLAG_DROPPED"; break;
	case FLAG_TAKEN:
	case FLAG_TAKEN_RED:
	case FLAG_TAKEN_BLUE:	name = "FLAG_TAKEN"; break;
	default:				return;		// retail has no name for anything else
	}
	if ( player && !player->client ) {
		player = NULL;
	}
	Stats_Begin();
	Stats_Common();
	if ( player ) {
		Stats_Identity( player );
	}
	JSON_String( &stats, "FLAG_TEAM", Stats_TeamName( team ) );
	JSON_String( &stats, "STATUS", name );
	Stats_SendEvent( player ? player->client : NULL, "FLAG_STATUS" );
}

/*
================
G_StatsReportRoundOver

Retail 0x100720e0, after the round's accuracy and perfect medals.  A round
nobody won reads DRAW.
================
*/
void G_StatsReportRoundOver( int round, int winner ) {
	if ( !trap_StatsAvailable() ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	JSON_Int( &stats, "ROUND", round );
	JSON_String( &stats, "TEAM_WON", winner == TEAM_FREE ? "DRAW" : Stats_TeamName( winner ) );
	Stats_SendEvent( NULL, "ROUND_OVER" );
}

/*
================
G_StatsReportRaceComplete

Retail 0x10071dd0: a finished run, its time in msec, and whether the racer
was pushed by a weapon on the way.
================
*/
void G_StatsReportRaceComplete( const gentity_t *ent, int raceTime, qboolean weaponsUsed ) {
	if ( !trap_StatsAvailable() || !ent || !ent->client ) {
		return;
	}
	Stats_Begin();
	Stats_Common();
	Stats_Identity( ent );
	JSON_Bool( &stats, "WEAPONS_USED", weaponsUsed );
	JSON_Int( &stats, "RACE_TIME", raceTime );
	Stats_SendEvent( ent->client, "PLAYER_RACECOMPLETE" );
}

/*
================
G_StatsReportPlayer

Retail 0x10074360: one player's whole match, sent when it leaves the match
(quit: a disconnect or a team change) and for every ranked player when the
match ends (aborted when the level is shut down under it).  Players still
connecting, at the intermission or spectating have nothing to report.

The ranks and the result are numbers, 0 or 1, not booleans.
================
*/
void G_StatsReportPlayer( const gclient_t *client, qboolean aborted, qboolean quit ) {
	const clientStats_t	*st;
	char			userinfo[MAX_INFO_STRING];
	const qboolean	teams = g_gametype.integer >= GT_TEAM;
	int				i, rank, teamRank, win = 0, lose = 0;
	qboolean		tied, teamTied;

	if ( !trap_StatsAvailable() || !client ) {
		return;
	}
	if ( client->pers.connected == CON_CONNECTING || client->ps.pm_type == PM_INTERMISSION
		|| client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	st = &client->stats;

	Stats_Begin();
	JSON_String( &stats, "NAME", client->pers.netname );
	JSON_String( &stats, "STEAM_ID", G_SteamIdString( client->pers.steamId ) );
	Stats_MatchGuid();
	JSON_Bool( &stats, "WARMUP", level.warmupTime != 0 );
	JSON_Bool( &stats, "ABORTED", aborted );
	trap_GetUserinfo( (int)( client - level.clients ), userinfo, sizeof( userinfo ) );
	JSON_String( &stats, "MODEL", Info_ValueForKey( userinfo, "model" ) );
	JSON_Int( &stats, "PLAY_TIME", ( level.time - client->pers.enterTime ) / 1000 );
	// a racer without a finished run holds the largest score: reported as -1
	JSON_Int( &stats, "SCORE", ( g_gametype.integer == GT_RACE && client->ps.persistant[PERS_SCORE] == 0x7fffffff )
		? -1 : client->ps.persistant[PERS_SCORE] );

	// every weapon by name; the grapple is the one OTHER_WEAPON
	JSON_BeginObject( &stats, "WEAPONS" );
	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) {
		JSON_BeginObject( &stats, Stats_WeaponName( i ) );
		JSON_Int( &stats, "K", st->weaponKills[i] );
		JSON_Int( &stats, "D", st->weaponDeaths[i] );
		JSON_Int( &stats, "S", st->weaponShots[i] );
		JSON_Int( &stats, "H", st->weaponHits[i] );
		JSON_Int( &stats, "DG", st->weaponDamageGiven[i] );
		JSON_Int( &stats, "DR", st->weaponDamageTaken[i] );
		JSON_Int( &stats, "T", st->weaponTime[i] / 1000 );
		JSON_Int( &stats, "P", st->weaponPickups[i] );
		JSON_End( &stats );
	}
	JSON_End( &stats );
	JSON_Int( &stats, "MAX_STREAK", st->maxKillStreak );

	// Powerups by slot, then holdables by tag: the holdable invulnerability
	// is written after the powerup of the same name and replaces it.
	JSON_BeginObject( &stats, "PICKUPS" );
	for ( i = PW_QUAD; i < PW_NUM_POWERUPS; i++ ) {
		JSON_Int( &stats, Stats_PowerupName( i ), st->powerupPickups[i] );
	}
	for ( i = HI_TELEPORTER; i < HI_NUM_HOLDABLE; i++ ) {
		JSON_Int( &stats, Stats_HoldableName( i ), st->holdablePickups[i] );
	}
	JSON_Int( &stats, "MEGA_HEALTH", st->mega.pickups );
	JSON_Int( &stats, "AMMO", st->ammoPickups );
	JSON_Int( &stats, "HEALTH", st->healthPickups );
	JSON_Int( &stats, "ARMOR", st->shardPickups );
	JSON_Int( &stats, "RED_ARMOR", st->redArmor.pickups );
	JSON_Int( &stats, "GREEN_ARMOR", st->greenArmor.pickups );
	JSON_Int( &stats, "YELLOW_ARMOR", st->yellowArmor.pickups );
	JSON_End( &stats );

	// the average wait, in msec, of each controlled item the player took
	if ( st->mega.pickups || st->redArmor.pickups || st->yellowArmor.pickups || st->greenArmor.pickups ) {
		JSON_BeginObject( &stats, "ITEM_TIMING" );
		if ( st->mega.pickups ) {
			JSON_Int( &stats, "MEGA_HEALTH", st->mega.waited / st->mega.pickups );
		}
		if ( st->redArmor.pickups ) {
			JSON_Int( &stats, "RED_ARMOR", st->redArmor.waited / st->redArmor.pickups );
		}
		if ( st->yellowArmor.pickups ) {
			JSON_Int( &stats, "YELLOW_ARMOR", st->yellowArmor.waited / st->yellowArmor.pickups );
		}
		if ( st->greenArmor.pickups ) {
			JSON_Int( &stats, "GREEN_ARMOR", st->greenArmor.waited / st->greenArmor.pickups );
		}
		JSON_End( &stats );
	}

	JSON_BeginObject( &stats, "DAMAGE" );
	JSON_Int( &stats, "DEALT", st->damageGiven );
	JSON_Int( &stats, "TAKEN", st->damageTaken );
	JSON_End( &stats );
	JSON_Int( &stats, "KILLS", st->kills );
	JSON_Int( &stats, "DEATHS", st->deaths );

	JSON_BeginObject( &stats, "MEDALS" );
	JSON_Int( &stats, "IMPRESSIVE", client->ps.persistant[PERS_IMPRESSIVE_COUNT] );
	JSON_Int( &stats, "EXCELLENT", client->ps.persistant[PERS_EXCELLENT_COUNT] );
	JSON_Int( &stats, "HUMILIATION", client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT] );
	JSON_Int( &stats, "REVENGE", client->medals[MEDAL_REVENGE] );
	JSON_Int( &stats, "COMBOKILL", client->medals[MEDAL_COMBOKILL] );
	JSON_Int( &stats, "RAMPAGE", client->medals[MEDAL_RAMPAGE] );
	JSON_Int( &stats, "MIDAIR", client->medals[MEDAL_MIDAIR] );
	JSON_Int( &stats, "PERFORATED", client->medals[MEDAL_PERFORATED] );
	JSON_Int( &stats, "HEADSHOT", client->medals[MEDAL_HEADSHOT] );
	JSON_Int( &stats, "ACCURACY", client->medals[MEDAL_ACCURACY] );
	JSON_Int( &stats, "QUADGOD", client->medals[MEDAL_QUADGOD] );
	JSON_Int( &stats, "FIRSTFRAG", client->medals[MEDAL_FIRSTFRAG] );
	JSON_Int( &stats, "PERFECT", client->medals[MEDAL_PERFECT] );
	JSON_Int( &stats, "DEFENDS", st->defends );
	JSON_Int( &stats, "ASSISTS", st->assists );
	JSON_Int( &stats, "CAPTURES", st->captures );
	JSON_End( &stats );

	if ( teams ) {
		JSON_Int( &stats, "TEAM", client->sess.sessionTeam );
		JSON_Int( &stats, "HOLY_SHITS", st->holyShits );
		JSON_Int( &stats, "TEAM_JOIN_TIME", st->startMatchTime / 1000 );
		JSON_Int( &stats, "RED_FLAG_PICKUPS", st->flagPickups[0] );
		JSON_Int( &stats, "BLUE_FLAG_PICKUPS", st->flagPickups[1] );
		JSON_Int( &stats, "NEUTRAL_FLAG_PICKUPS", st->flagPickups[2] );
	}

	if ( quit ) {
		// a player who left has no rank; Red Rover gets team keys only here
		JSON_Int( &stats, "RANK", -1 );
		JSON_Int( &stats, "TIED_RANK", 1 );
		if ( teams ) {
			JSON_Int( &stats, "TEAM_RANK", -1 );
			JSON_Int( &stats, "TIED_TEAM_RANK", 1 );
		}
	} else {
		Stats_Ranks( client, &rank, &tied, &teamRank, &teamTied );
		JSON_Int( &stats, "RANK", rank + 1 );
		JSON_Int( &stats, "TIED_RANK", tied ? 1 : 0 );
		if ( teams && g_gametype.integer != GT_RED_ROVER ) {
			const int	own = client->sess.sessionTeam;
			const int	enemy = own == TEAM_RED ? TEAM_BLUE : TEAM_RED;

			JSON_Int( &stats, "TEAM_RANK", teamRank + 1 );
			JSON_Int( &stats, "TIED_TEAM_RANK", teamTied ? 1 : 0 );
			// the side ahead wins, and so does a side left alone; a
			// forfeited side (score -999) loses
			if ( ( own == TEAM_RED || own == TEAM_BLUE ) && level.teamScores[own] != -999
				&& ( level.teamScores[own] > level.teamScores[enemy] || TeamConnectedCount( -1, enemy ) == 0 ) ) {
				win = 1;
			}
		} else if ( client->ps.persistant[PERS_SCORE] != -999 && ( rank == 0 || level.forfeited ) ) {
			win = 1;
		}
		lose = !win;
	}
	JSON_Int( &stats, "QUIT", quit ? 1 : 0 );
	JSON_Int( &stats, "WIN", win );
	JSON_Int( &stats, "LOSE", lose );

	Stats_SendEvent( client, "PLAYER_STATS" );
}

// the match's settings, shared by MATCH_STARTED and the report
static void Stats_MatchSettings( void ) {
	char	text[MAX_INFO_STRING];
	char	hostname[64];
	char	*c;

	JSON_String( &stats, "GAME_TYPE", Stats_GametypeName( g_gametype.integer ) );
	JSON_String( &stats, "FACTORY", g_factory.string );
	JSON_String( &stats, "FACTORY_TITLE", g_factoryTitle.string );
	trap_GetServerinfo( text, sizeof( text ) );
	Q_strncpyz( text, Info_ValueForKey( text, "mapname" ), sizeof( text ) );
	for ( c = text; *c; c++ ) {
		*c = (char)tolower( (unsigned char)*c );
	}
	JSON_String( &stats, "MAP", text );
	JSON_Int( &stats, "FRAG_LIMIT", g_fraglimit.integer );
	JSON_Int( &stats, "CAPTURE_LIMIT", g_capturelimit.integer );
	JSON_Int( &stats, "SCORE_LIMIT", scorelimit.integer );
	JSON_Int( &stats, "MERCY_LIMIT", mercylimit.integer );
	JSON_Int( &stats, "TIME_LIMIT", g_timelimit.integer );
	JSON_Int( &stats, "ROUND_LIMIT", roundlimit.integer );
	JSON_Int( &stats, "TRAINING", g_training.integer );
	JSON_Int( &stats, "INSTAGIB", g_instaGib.integer );
	JSON_Int( &stats, "QUADHOG", g_quadHog.integer );
	JSON_Int( &stats, "INFECTED", g_rrInfected.integer );
	Stats_MatchGuid();
	trap_Cvar_VariableStringBuffer( "sv_hostname", hostname, sizeof( hostname ) );
	JSON_String( &stats, "SERVER_TITLE", hostname );
}

/*
================
G_StatsReportMatchStarted

Retail 0x10077a30, once, when the countdown runs out and just before the
restart into the match: the settings and everyone connected, spectators and
bots included, in slot order.  Nobody at all is a null, not an empty list.
================
*/
void G_StatsReportMatchStarted( void ) {
	int			i;
	qboolean	any = qfalse;

	if ( !trap_StatsAvailable() ) {
		return;
	}
	Stats_Begin();
	Stats_MatchSettings();
	for ( i = 0; i < level.maxclients; i++ ) {
		const gentity_t	*ent = &g_entities[i];

		if ( level.clients[i].pers.connected == CON_DISCONNECTED || !ent->client ) {
			continue;
		}
		if ( !any ) {
			JSON_BeginArray( &stats, "PLAYERS" );
			any = qtrue;
		}
		JSON_BeginObject( &stats, NULL );
		Stats_Identity( ent );
		JSON_Int( &stats, "TEAM", ent->client->sess.sessionTeam );
		JSON_End( &stats );
	}
	if ( any ) {
		JSON_End( &stats );
	} else {
		JSON_Null( &stats, "PLAYERS" );
	}
	Stats_SendEvent( NULL, "MATCH_STARTED" );
}

/*
================
G_StatsReportMatch

Retail 0x10076fe0, the closing report, after every player's PLAYER_STATS.
aborted is a level shut down under the match, restarted the shutdown's
restart flag, and the message is why the match ended.
================
*/
void G_StatsReportMatch( qboolean aborted, int restarted, const char *exitMessage ) {
	const char	*document;

	if ( !trap_StatsAvailable() ) {
		return;
	}
	Stats_Begin();
	Stats_MatchSettings();
	JSON_Int( &stats, "TSCORE0", level.teamScores[TEAM_RED] );
	JSON_Int( &stats, "TSCORE1", level.teamScores[TEAM_BLUE] );
	JSON_Int( &stats, "GAME_LENGTH", ( level.time - level.startTime ) / 1000 );
	JSON_Int( &stats, "RESTARTED", restarted );
	JSON_Bool( &stats, "ABORTED", aborted );
	JSON_String( &stats, "EXIT_MSG", exitMessage ? exitMessage : "" );
	JSON_String( &stats, "FIRST_SCORER", level.firstScorer[0] ? level.firstScorer : "none" );
	JSON_String( &stats, "LAST_SCORER", level.lastScorer[0] ? level.lastScorer : "none" );
	JSON_String( &stats, "LAST_TEAMSCORER", level.lastTeamScorer[0] ? level.lastTeamScorer : "none" );
	JSON_Int( &stats, "LAST_LEAD_CHANGE_TIME", level.lastLeadChangeTime );
	JSON_End( &stats );
	document = JSON_Finish( &stats );
	if ( !document ) {
		G_Printf( S_COLOR_YELLOW "Stats: the match report did not fit and was not sent\n" );
		return;
	}
	trap_StatsReportJson( document );
}
