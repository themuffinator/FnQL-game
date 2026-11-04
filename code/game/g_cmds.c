// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#ifdef MISSIONPACK
#include "bg_voicechat.h"			// for the voice chats
#endif

/*
==================
G_ScoreboardPing
==================
*/
static int G_ScoreboardPing( const gclient_t *cl ) {
	if ( cl->pers.connected == CON_CONNECTING ) {
		return -1;
	}
	return cl->ps.ping < 999 ? cl->ps.ping : 999;
}


/*
==================
G_BestWeapon

Retail qagamex86 0x100789d0: the weapon that has dealt the most damage
this match, or else the best starting weapon.
==================
*/
static int G_BestWeapon( const gclient_t *cl ) {
	int		i, best, most, fallback;

	fallback = WP_GAUNTLET;
	for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
		if ( g_startingWeapons.integer & ( 1 << ( i - 1 ) ) ) {
			fallback = i;
		}
	}

	best = WP_NONE;
	most = 0;
	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) {
		if ( cl->stats.weaponDamageGiven[i] > most ) {
			most = cl->stats.weaponDamageGiven[i];
			best = i;
		}
	}
	return best != WP_NONE ? best : fallback;
}


/*
==================
G_ScoreboardFFA

Retail qagamex86 0x1003d090: the free-for-all scoreboard, or NULL when its
entries outgrow a 1024-byte buffer.
==================
*/
static const char *G_ScoreboardFFA( void ) {
	char		entry[MAX_STRING_CHARS];
	char		string[MAX_STRING_CHARS];
	int			stringlength, i, j, accuracy, perfect, best, bestAccuracy;
	gclient_t	*cl;

	string[0] = '\0';
	stringlength = 0;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		cl = &level.clients[level.sortedClients[i]];

		accuracy = cl->accuracy_shots ? cl->accuracy_hits * 100 / cl->accuracy_shots : 0;
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;
		best = G_BestWeapon( cl );
		bestAccuracy = cl->stats.weaponShots[best] > 0
			? (int)( (double)cl->stats.weaponHits[best] * 100.0 / (double)cl->stats.weaponShots[best] ) : 0;

		j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],
			G_ScoreboardPing( cl ),
			(level.time - cl->pers.enterTime)/60000,
			accuracy,
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->ps.persistant[PERS_DEFEND_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT],
			perfect,
			cl->ps.persistant[PERS_CAPTURES],
			cl->ps.pm_type == PM_NORMAL,
			cl->stats.kills,
			cl->stats.deaths,
			best,
			bestAccuracy,
			cl->stats.damageGiven );

		if ( stringlength + j > 1023 ) {
			return NULL;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	return va( "scores_ffa %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
}


/*
==================
G_ScoreboardDuelRow

One duelist's scores_duel entry (retail qagamex86 0x1003d2b0): the match
figures, the item control of the red, yellow and green armor and the mega
health (pickups, then the average seconds each stood waiting before a
pickup that was not its first), and five figures per weapon.  Without
withItems the item control reads zero.
==================
*/
static void G_ScoreboardDuelRow( int clientNum, qboolean withItems, char *out, int outSize ) {
	const gclient_t		*cl = &level.clients[clientNum];
	const itemControl_t	*items[4];
	char				weapons[MAX_STRING_CHARS];
	char				entry[MAX_STRING_CHARS];
	int					pickups[4];
	float				waited[4];
	int					length, i, j, hits, shots, accuracy, perfect;

	weapons[0] = '\0';
	length = 0;
	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) {
		hits = cl->stats.weaponHits[i];
		shots = cl->stats.weaponShots[i];
		accuracy = ( hits && shots ) ? hits * 100 / shots : 0;
		j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i",
			hits, shots, accuracy, cl->stats.weaponDamageGiven[i], cl->stats.weaponKills[i] );
		if ( length + j > 1023 ) {
			break;
		}
		strcpy( weapons + length, entry );
		length += j;
	}

	items[0] = &cl->stats.redArmor;
	items[1] = &cl->stats.yellowArmor;
	items[2] = &cl->stats.greenArmor;
	items[3] = &cl->stats.mega;
	for ( i = 0; i < 4; i++ ) {
		const int later = items[i]->pickups - items[i]->first;

		pickups[i] = withItems ? items[i]->pickups : 0;
		waited[i] = ( withItems && later > 0 ) ? (float)( (double)items[i]->waited / 1000.0 / (double)later ) : 0.0f;
	}

	accuracy = cl->accuracy_shots ? cl->accuracy_hits * 100 / cl->accuracy_shots : 0;
	perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

	Com_sprintf( out, outSize,
		"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %3.2f %i %3.2f %i %3.2f %i %3.2f%s",
		clientNum,
		cl->ps.persistant[PERS_SCORE],
		G_ScoreboardPing( cl ),
		(level.time - cl->pers.enterTime)/60000,
		cl->stats.kills,
		cl->stats.deaths,
		accuracy,
		G_BestWeapon( cl ),
		cl->stats.damageGiven,
		cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
		cl->ps.persistant[PERS_EXCELLENT_COUNT],
		cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
		perfect,
		pickups[0], waited[0], pickups[1], waited[1], pickups[2], waited[2], pickups[3], waited[3],
		weapons );
}


/*
==================
G_ScoreboardDuel

Retail qagamex86 0x1003d2b0: the two duelists in client order.  A player
sees its opponent's item control only from the spectators or at the
intermission, which shows the duelists' last full entries even after one
has gone.
==================
*/
static const char *G_ScoreboardDuel( gentity_t *viewer ) {
	char		plain[2][MAX_STRING_CHARS];
	char		full[2][MAX_STRING_CHARS];
	int			duelists[2] = { -1, -1 };
	int			count, i;
	qboolean	open;

	if ( level.numPlayingClients == 1 ) {
		duelists[0] = level.sortedClients[0];
	} else if ( level.numPlayingClients > 1 ) {
		const qboolean ordered = level.sortedClients[0] < level.sortedClients[1];

		duelists[0] = level.sortedClients[ordered ? 0 : 1];
		duelists[1] = level.sortedClients[ordered ? 1 : 0];
		level.duelRowClient = duelists[0];
	}

	for ( i = 0; i < 2; i++ ) {
		plain[i][0] = full[i][0] = '\0';
		if ( duelists[i] < 0 ) {
			continue;
		}
		G_ScoreboardDuelRow( duelists[i], qfalse, plain[i], sizeof( plain[i] ) );
		G_ScoreboardDuelRow( duelists[i], qtrue, full[i], sizeof( full[i] ) );
		Q_strncpyz( level.duelRows[( i == 0 && duelists[i] == level.duelRowClient ) ? 0 : 1], full[i],
			sizeof( level.duelRows[0] ) );
	}

	if ( level.intermissionQueued || level.intermissiontime ) {
		return va( "scores_duel 2 %s %s", level.duelRows[0], level.duelRows[1] );
	}

	count = level.numPlayingClients < 2 ? level.numPlayingClients : 2;
	if ( count < 2 ) {
		return va( "scores_duel %i %s", count, full[0] );
	}

	for ( i = 0; i < 2; i++ ) {
		open = viewer->client == &level.clients[duelists[i]]
			|| viewer->client->sess.sessionTeam == TEAM_SPECTATOR;
		if ( !open ) {
			Q_strncpyz( full[i], plain[i], sizeof( full[i] ) );
		}
	}
	return va( "scores_duel %i %s %s", count, full[0], full[1] );
}


/*
==================
G_ScoreboardRace

Retail qagamex86 0x1003dbb0: each racer's best time (-1 for none), or NULL
when the entries outgrow a 1024-byte buffer.  Retail counts the players in
the game for the header but lists everyone connected.
==================
*/
static const char *G_ScoreboardRace( void ) {
	char		entry[MAX_STRING_CHARS];
	char		string[MAX_STRING_CHARS];
	int			stringlength, i, j, best;
	gclient_t	*cl;

	string[0] = '\0';
	stringlength = 0;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		cl = &level.clients[level.sortedClients[i]];

		best = cl->ps.persistant[PERS_SCORE];
		if ( best == 0x7fffffff ) {
			best = -1;
		}
		j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i",
			level.sortedClients[i],
			best,
			G_ScoreboardPing( cl ),
			(level.time - cl->pers.enterTime)/60000 );

		if ( stringlength + j >= 1024 ) {
			return NULL;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	return va( "scores_race %i%s", level.numPlayingClients, string );
}


/*
==================
G_ScoreboardPlayer

The fields the retail per-mode scoreboards draw from (qagamex86
0x1003d090-0x1003f260).
==================
*/
typedef struct {
	int		clientNum, team, score, ping, minutes;
	int		accuracy, perfect, alive, best, bestAccuracy;
} scoreboardPlayer_t;

static void G_ScoreboardPlayer( int sortedIndex, scoreboardPlayer_t *out ) {
	const int	clientNum = level.sortedClients[sortedIndex];
	gclient_t	*cl = &level.clients[clientNum];

	out->clientNum = clientNum;
	out->team = cl->sess.sessionTeam;
	out->score = cl->ps.persistant[PERS_SCORE];
	out->ping = G_ScoreboardPing( cl );
	out->minutes = (level.time - cl->pers.enterTime)/60000;
	out->accuracy = cl->accuracy_shots ? cl->accuracy_hits * 100 / cl->accuracy_shots : 0;
	out->perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;
	out->alive = cl->ps.pm_type == PM_NORMAL;
	out->best = G_BestWeapon( cl );
	out->bestAccuracy = cl->stats.weaponShots[out->best] > 0
		? (int)( (double)cl->stats.weaponHits[out->best] * 100.0 / (double)cl->stats.weaponShots[out->best] ) : 0;
}


/*
==================
G_TeamStatsHeader

The team item-control numbers the team scoreboards open with, red's then
blue's.  Outside the intermission a player does not see the other team's,
except for its invisibility pickups and, in the flag modes, its medkit
pickups and flag time: retail zeroes the rest only (qagamex86 0x1003e029,
0x1003ea93, 0x1003f0bb).
==================
*/
static qboolean G_TeamStatHidden( teamStat_t stat ) {
	return stat != TEAM_STAT_INVIS && stat != TEAM_STAT_MEDKIT && stat != TEAM_STAT_FLAG_TIME;
}

static const teamStat_t tdmTeamStats[] = {
	TEAM_STAT_RED_ARMOR, TEAM_STAT_YELLOW_ARMOR, TEAM_STAT_GREEN_ARMOR, TEAM_STAT_MEGA_HEALTH,
	TEAM_STAT_QUAD, TEAM_STAT_BATTLESUIT, TEAM_STAT_REGEN, TEAM_STAT_HASTE, TEAM_STAT_INVIS,
	TEAM_STAT_QUAD_TIME, TEAM_STAT_BATTLESUIT_TIME, TEAM_STAT_REGEN_TIME, TEAM_STAT_HASTE_TIME,
	TEAM_STAT_INVIS_TIME
};

static const teamStat_t ctfTeamStats[] = {
	TEAM_STAT_RED_ARMOR, TEAM_STAT_YELLOW_ARMOR, TEAM_STAT_GREEN_ARMOR, TEAM_STAT_MEGA_HEALTH,
	TEAM_STAT_QUAD, TEAM_STAT_BATTLESUIT, TEAM_STAT_REGEN, TEAM_STAT_HASTE, TEAM_STAT_INVIS,
	TEAM_STAT_FLAG_PICKUPS, TEAM_STAT_MEDKIT, TEAM_STAT_QUAD_TIME, TEAM_STAT_BATTLESUIT_TIME,
	TEAM_STAT_REGEN_TIME, TEAM_STAT_HASTE_TIME, TEAM_STAT_INVIS_TIME, TEAM_STAT_FLAG_TIME
};

static const char *G_TeamStatsHeader( gentity_t *viewer, const teamStat_t *stats, int count ) {
	static char	header[MAX_STRING_CHARS];
	const int	viewerTeam = viewer->client->sess.sessionTeam;
	int			length = 0, team, i, value;

	header[0] = '\0';
	for ( team = TEAM_RED; team <= TEAM_BLUE; team++ ) {
		const qboolean hidden = !level.intermissiontime
			&& ( ( viewerTeam == TEAM_RED && team == TEAM_BLUE ) || ( viewerTeam == TEAM_BLUE && team == TEAM_RED ) );

		for ( i = 0; i < count; i++ ) {
			value = ( hidden && G_TeamStatHidden( stats[i] ) ) ? 0 : level.teamStats[stats[i]][team];
			length += Com_sprintf( header + length, sizeof( header ) - length, " %i", value );
		}
	}
	return header;
}


/*
==================
G_ScoreboardTeam

The team scoreboards: retail qagamex86 0x1003dd20 (team deathmatch),
0x1003eda0 (freeze tag), 0x1003e300 (clan arena) and 0x1003e6d0 (the flag
modes and domination).  Each returns NULL when its entries outgrow a
1024-byte buffer.
==================
*/
static const char *G_ScoreboardTeam( gentity_t *viewer ) {
	char				entry[MAX_STRING_CHARS];
	char				string[MAX_STRING_CHARS];
	int					stringlength, i, j;
	scoreboardPlayer_t	p;
	gclient_t			*cl;
	const int			gametype = g_gametype.integer;

	string[0] = '\0';
	stringlength = 0;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		G_ScoreboardPlayer( i, &p );
		cl = &level.clients[p.clientNum];

		switch ( gametype ) {
		case GT_TEAM:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				p.clientNum, p.team, p.score, p.ping, p.minutes, cl->stats.kills, cl->stats.deaths,
				p.accuracy, p.best, cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
				cl->stats.teamKills, cl->stats.teamKilled, cl->stats.damageGiven );
			break;
		case GT_FREEZE:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				p.clientNum, p.team, p.score, p.ping, p.minutes, cl->stats.kills, cl->stats.deaths,
				p.accuracy, p.best, cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
				cl->ps.persistant[PERS_ASSIST_COUNT], cl->stats.teamKills, cl->stats.teamKilled,
				cl->stats.damageGiven, p.alive );
			break;
		case GT_CLAN_ARENA:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				p.clientNum, p.team, p.score, p.ping, p.minutes, cl->stats.kills, cl->stats.deaths,
				p.accuracy, p.best, p.bestAccuracy, cl->stats.damageGiven,
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT], cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], p.perfect, p.alive );
			break;
		default:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				p.clientNum, p.team, p.score, p.ping, p.minutes, cl->stats.kills, cl->stats.deaths,
				p.accuracy, p.best, cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
				cl->ps.persistant[PERS_DEFEND_COUNT], cl->ps.persistant[PERS_ASSIST_COUNT],
				cl->ps.persistant[PERS_CAPTURES], p.perfect, p.alive );
			break;
		}

		if ( stringlength + j > 1023 ) {
			return NULL;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	switch ( gametype ) {
	case GT_TEAM:
		return va( "scores_tdm%s %i %i %i%s", G_TeamStatsHeader( viewer, tdmTeamStats, ARRAY_LEN( tdmTeamStats ) ),
			i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
	case GT_FREEZE:
		return va( "scores_ft%s %i %i %i%s", G_TeamStatsHeader( viewer, tdmTeamStats, ARRAY_LEN( tdmTeamStats ) ),
			i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
	case GT_CLAN_ARENA:
		return va( "scores_ca %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
	default:
		return va( "scores_ctf%s %i %i %i%s", G_TeamStatsHeader( viewer, ctfTeamStats, ARRAY_LEN( ctfTeamStats ) ),
			i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
	}
}


/*
==================
G_ScoreboardOverload

Retail qagamex86 0x1003cec0: Overload keeps the Quake III command name
with Quake Live's fields, and the entries that fit.
==================
*/
static const char *G_ScoreboardOverload( void ) {
	char				entry[MAX_STRING_CHARS];
	char				string[MAX_STRING_CHARS];
	int					stringlength, i, j;
	scoreboardPlayer_t	p;
	gclient_t			*cl;

	string[0] = '\0';
	stringlength = 0;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		G_ScoreboardPlayer( i, &p );
		cl = &level.clients[p.clientNum];

		j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			p.clientNum, p.score, p.ping, p.minutes, 0, 0, p.accuracy,
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT], cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], cl->ps.persistant[PERS_DEFEND_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT], p.perfect, cl->ps.persistant[PERS_CAPTURES], p.alive,
			cl->stats.kills, cl->stats.deaths, p.best );

		if ( stringlength + j > 1023 ) {
			break;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	return va( "scores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
}


/* Retail 1003f260: Red Rover's nineteen-field player rows. */
static const char *G_ScoreboardRedRover( void ) {
	char rows[MAX_STRING_CHARS] = "", row[256];
	int i, length = 0, bestAccuracy = 0;
	for ( i = 0; i < level.numConnectedClients; ++i ) {
		scoreboardPlayer_t p;
		gclient_t *cl;
		int n;
		G_ScoreboardPlayer(i, &p);
		cl = &level.clients[p.clientNum];
		if ( cl->stats.weaponShots[p.best] ) bestAccuracy = p.bestAccuracy;
		n = Com_sprintf(row, sizeof(row), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			p.clientNum, p.score, cl->roundScore, p.ping, p.minutes, cl->stats.kills, cl->stats.deaths,
			p.accuracy, p.best, bestAccuracy, cl->stats.damageGiven,
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT], cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_DEFEND_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT], p.perfect, cl->ps.persistant[PERS_CAPTURES], p.alive);
		if ( length + n >= sizeof(rows) ) break;
		memcpy(rows + length, row, n + 1);
		length += n;
	}
	return va("scores_rr %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], rows);
}


/*
==================
G_SendIntermissionTeamStats

At the intermission retail sends each player's team statistics after the
scoreboard, one command per player in scoreboard order: tdmstats
(qagamex86 0x1003e1b0) for team deathmatch and freeze tag, castats
(0x1003e510: damage, then accuracy and kills per weapon) for clan arena,
ctfstats (0x1003ec40) for the flag modes and domination.
==================
*/
static void G_SendIntermissionTeamStats( gentity_t *viewer ) {
	char		entry[MAX_STRING_CHARS];
	gclient_t	*cl;
	int			i, w, j, length;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		cl = &level.clients[level.sortedClients[i]];

		switch ( g_gametype.integer ) {
		case GT_TEAM:
		case GT_FREEZE:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i",
				cl->stats.suicides, cl->stats.teamKills, cl->stats.teamKilled, cl->stats.damageGiven,
				cl->stats.damageTaken, cl->stats.redArmor.pickups, cl->stats.yellowArmor.pickups,
				cl->stats.greenArmor.pickups, cl->stats.mega.pickups, cl->stats.quadPickups,
				cl->stats.battleSuitPickups );
			if ( j > 1023 ) {
				return;
			}
			trap_SendServerCommand( viewer - g_entities, va( "tdmstats %i%s", i, entry ) );
			break;

		case GT_CLAN_ARENA:
			length = Com_sprintf( entry, sizeof( entry ), " %i %i", cl->stats.damageGiven, cl->stats.damageTaken );
			for ( w = WP_GAUNTLET; w < WP_GAUNTLET + 15; w++ ) {
				const int shots = cl->stats.weaponShots[w];
				const int accuracy = cl->stats.weaponHits[w] && shots ? cl->stats.weaponHits[w] * 100 / shots : 0;
				char pair[64];

				j = Com_sprintf( pair, sizeof( pair ), " %i %i", accuracy, cl->stats.weaponKills[w] );
				if ( length + j > 1023 ) {
					break;
				}
				strcpy( entry + length, pair );
				length += j;
			}
			trap_SendServerCommand( viewer - g_entities, va( "castats %i%s", i, entry ) );
			break;

		case GT_CTF:
		case GT_1FCTF:
		case GT_HARVESTER:
		case GT_DOMINATION:
		case GT_ATTACK_DEFEND:
			j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i %i %i %i",
				cl->stats.suicides, cl->stats.damageGiven, cl->stats.damageTaken,
				cl->stats.redArmor.pickups, cl->stats.yellowArmor.pickups, cl->stats.greenArmor.pickups,
				cl->stats.mega.pickups, cl->stats.quadPickups, cl->stats.battleSuitPickups,
				cl->stats.regenPickups, cl->stats.hastePickups, cl->stats.invisPickups );
			if ( j > 1023 ) {
				return;
			}
			trap_SendServerCommand( viewer - g_entities, va( "ctfstats %i%s", i, entry ) );
			break;

		default:
			return;
		}
	}
}


/*
==================
G_ScoreboardSmall

Retail qagamex86 0x1003cd70: the short scoreboard for when the full one is
too long or g_forceSmallScoreboardMessage asks for it; it keeps the
entries that fit.
==================
*/
static const char *G_ScoreboardSmall( void ) {
	char		entry[MAX_STRING_CHARS];
	char		string[MAX_STRING_CHARS];
	int			stringlength, i, j;
	gclient_t	*cl;

	string[0] = '\0';
	stringlength = 0;

	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		cl = &level.clients[level.sortedClients[i]];

		j = Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i",
			level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],
			G_ScoreboardPing( cl ),
			(level.time - cl->pers.enterTime)/60000,
			cl->ps.persistant[PERS_CAPTURES],
			cl->ps.pm_type == PM_NORMAL,
			cl->stats.kills,
			cl->stats.deaths );

		if ( stringlength + j > 1023 ) {
			break;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	return va( "smscores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
}


/*
==================
G_ScoreboardLegacy

The Quake III scoreboard that duel and red rover still send until their
retail grammars land (scores_duel with the duel statistics, scores_rr with
red rover).
==================
*/
static const char *G_ScoreboardLegacy( void ) {
	char		entry[256]; // enough to hold 14 integers
	char		string[MAX_STRING_CHARS-1];
	int			stringlength;
	int			i, j, prefix;
	gclient_t	*cl;
	int			numSorted, accuracy, perfect;

	// send the latest information on all clients
	string[0] = '\0';
	stringlength = 0;

	numSorted = level.numConnectedClients;

	// estimate prefix length to avoid oversize of final string
	prefix = BG_sprintf( entry, "scores %i %i %i", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], numSorted );

	for ( i = 0 ; i < numSorted ; i++ ) {

		cl = &level.clients[level.sortedClients[i]];

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		} else {
			accuracy = 0;
		}

		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		j = BG_sprintf( entry, " %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],
			G_ScoreboardPing( cl ),
			(level.time - cl->pers.enterTime)/60000,
			0,
			g_entities[level.sortedClients[i]].s.powerups,
			accuracy,
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->ps.persistant[PERS_DEFEND_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT],
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);

		if ( stringlength + j + prefix >= sizeof( string ) )
			break;

		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	return va( "scores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string );
}


/*
==================
DeathmatchScoreboardMessage

Retail qagamex86 0x1003cbf0: each mode sends its own scoreboard, falling
back to the short one, and at the intermission the team modes follow it
with each player's team statistics.
==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	const char	*message;

	switch ( g_gametype.integer ) {
	case GT_FFA:
		message = G_ScoreboardFFA();
		break;
	case GT_TOURNAMENT:
		message = G_ScoreboardDuel( ent );
		break;
	case GT_RACE:
		message = G_ScoreboardRace();
		break;
	case GT_OBELISK:
		message = G_ScoreboardOverload();
		break;
	case GT_RED_ROVER:
		message = G_ScoreboardRedRover();
		break;
	default:
		message = G_ScoreboardTeam( ent );
		break;
	}

	if ( !message || strlen( message ) > 1023 || g_forceSmallScoreboardMessage.integer ) {
		message = G_ScoreboardSmall();
	}
	trap_SendServerCommand( ent-g_entities, message );

	if ( level.intermissiontime ) {
		G_SendIntermissionTeamStats( ent );
	}
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}


/*
==================
Cmd_WeaponAccuracy_f

The acc and pstats commands (retail qagamex86 0x1003f4d0, 0x1003f660):
the accuracy of every weapon slot from WP_NONE on, hits * 100 / shots or
0, for the player or for the one it follows.  Retail takes the followed
player only while the requester's own player state is a spectator's.
==================
*/
static void Cmd_WeaponAccuracy_f( gentity_t *ent, const char *command ) {
	char		string[MAX_STRING_CHARS];
	char		entry[MAX_STRING_CHARS];
	gclient_t	*cl;
	int			i, length, entryLength;

	cl = ent->client;
	if ( !cl ) {
		return;
	}
	if ( cl->ps.pm_type == PM_SPECTATOR && cl->sess.spectatorState == SPECTATOR_FOLLOW ) {
		cl = &level.clients[cl->sess.spectatorClient];
	}

	string[0] = '\0';
	length = 0;
	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		if ( cl->stats.weaponHits[i] && cl->stats.weaponShots[i] ) {
			entryLength = Com_sprintf( entry, sizeof( entry ), " %i", cl->stats.weaponHits[i] * 100 / cl->stats.weaponShots[i] );
		} else {
			entryLength = Com_sprintf( entry, sizeof( entry ), " 0" );
		}
		if ( length + entryLength + 1 <= (int)sizeof( string ) ) {
			Q_strncpyz( string + length, entry, sizeof( string ) - length );
			length += entryLength;
		}
	}

	trap_SendServerCommand( ent - g_entities, va( "%s %s", command, string ) );
}


/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, "print \"You must be alive to use this command.\n\"");
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char *ConcatArgs( int start ) {
	static char line[MAX_STRING_CHARS];
	char	arg[MAX_STRING_CHARS];
	int		i, c, tlen;
	int		len;

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = (int)strlen( arg );
		if ( len + tlen >= sizeof( line )-1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = '\0';

	return line;
}


/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( const char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < ' ' ) {
			in++;
			continue;
		}
		*out = tolower( *in );
		out++;
		in++;
	}

	*out = '\0';
}


/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( (unsigned) idnum >= (unsigned)level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}


/*
==================
Cmd_Give_f

Give items to a client.  Retail qagamex86 0x1003fb50: the weapons are every
bit but WP_NONE's, infinite ammo stays infinite, and armor also reaches the
entity's replicated armor.
==================
*/
void Cmd_Give_f( gentity_t *ent )
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t	*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = 0xfffe;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			if ( ent->client->ps.ammo[i] != -1 ) {
				ent->client->ps.ammo[i] = 999;
			}
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->s.armor = 200;
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItemForced( it_ent, it );
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent )
{
	const char *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	const char *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	const char *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( G_IsSinglePlayer() ) {
		trap_SendServerCommand( ent-g_entities,
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	if ( !ent->client->pers.localClient )
	{
		trap_SendServerCommand( ent - g_entities,
			"print \"The levelshot command must be executed by a local client\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_TeamTask_f
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo( client, userinfo, sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, "teamtask", va( "%d", task ) );
	trap_SetUserinfo( client, userinfo );
	ClientUserinfoChanged( client );
}


/*
=================
Cmd_Kill_f

Retail qagamex86 0x100401d0: g_allowKill 0 refuses, and the suicide is
not stopped by god mode.
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}
	if ( !g_allowKill.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Kill is not enabled on this server.\n\"" );
		return;
	}
	ent->s.health = ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}


/*
=================
BroadcastTeamChange

Let everyone know about a team change.  Retail (qagamex86 0x10040260)
stays quiet in training and in the first five seconds of a level, and puts
a long name on its own line.
=================
*/
void BroadcastTeamChange( gclient_t *client, team_t oldTeam )
{
	char		name[40], clean[40];
	const char	*sep;

	if ( g_training.integer || level.shuffling || level.time < 5000 ) {
		return;
	}

	Q_strncpyz( name, client->pers.netname, sizeof( name ) );
	Q_strncpyz( clean, name, sizeof( clean ) );
	Q_CleanStr( clean );
	sep = ( strlen( clean ) > 19 || strlen( name ) > 25 ) ? "\n" : " ";

	switch ( client->sess.sessionTeam ) {
	case TEAM_RED:
	case TEAM_BLUE:
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE "%sjoined the %s.\n\"", name, sep,
			client->sess.sessionTeam == TEAM_RED ? "Red Team" : "Blue Team" ) );
		break;
	case TEAM_SPECTATOR:
		if ( oldTeam != TEAM_SPECTATOR ) {
			trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE "%sjoined the spectators.\n\"", name, sep ) );
		}
		break;
	case TEAM_FREE:
		trap_SendServerCommand( -1, va( "cp \"%s" S_COLOR_WHITE "%sjoined the battle.\n\"", name, sep ) );
		break;
	default:
		break;
	}
}


static qboolean AllowTeamSwitch( int clientNum, team_t newTeam ) {

	if ( (newTeam == TEAM_RED || newTeam == TEAM_BLUE) && level.teamLocked[newTeam] ) {
		trap_SendServerCommand(clientNum, va("print \"The %s team is locked!\n\"", TeamName(newTeam)));
		return qfalse;
	}
	if ( g_teamForceBalance.integer  ) {
		int		counts[TEAM_NUM_TEAMS];

		counts[TEAM_BLUE] = TeamCount( clientNum, TEAM_BLUE );
		counts[TEAM_RED] = TeamCount( clientNum, TEAM_RED );

		// We allow a spread of two
		if ( newTeam == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
			trap_SendServerCommand( clientNum, "cp \"Red team has too many players.\n\"" );
			return qfalse; // ignore the request
		}

		if ( newTeam == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
			trap_SendServerCommand( clientNum, "cp \"Blue team has too many players.\n\"" );
			return qfalse; // ignore the request
		}

		// It's ok, the team we are switching to has less or same number of players
	}

	return qtrue;
}


/*
=================
G_SetSpectateOnly

Retail qagamex86 0x10040759: in a duel, asking to spectate leaves the queue.
=================
*/
static void G_SetSpectateOnly( gclient_t *client ) {
	if ( g_gametype.integer != GT_TOURNAMENT ) {
		return;
	}
	client->sess.spectateOnly = 1;
	client->sess.queuePosition = 0;
	trap_SendServerCommand( client - level.clients, "cp \"You are set to spectate only\n\"" );
}


/*
=================
SetTeam
=================
*/
static int SetTeamInternal( gentity_t *ent, const char *s, qboolean force ) {
	team_t				team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;
	qboolean			checkTeamLeader = qfalse;
	int					i;

	//
	// see what change is requested
	//

	clientNum = ent - g_entities;
	client = level.clients + clientNum;

	// early team override
	if ( client->pers.connected == CON_CONNECTING && g_gametype.integer >= GT_TEAM ) {
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			team = -1;
		}
		if ( team != -1 && (force || AllowTeamSwitch( clientNum, team )) ) {
			client->sess.sessionTeam = team;
			client->pers.teamState.state = TEAM_BEGIN;
			G_WriteClientSessionData( client );
			// count current clients and rank for scoreboard
			CalculateRanks();
		}
		return -1; // bypass flood protection
	}

	// retail (qagamex86 0x10040715) starts a free spectator on client 0, so it
	// hears client 0's deaths like a follower
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		G_SetSpectateOnly( client );
		// retail (qagamex86 0x100407a5) follows only when someone plays
		if ( level.numPlayingClients > 0 ) {
			specState = SPECTATOR_FOLLOW;
			specClient = -1;
		} else {
			specState = SPECTATOR_FREE;
		}
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		G_SetSpectateOnly( client );
		if ( level.numPlayingClients > 1 ) {
			specState = SPECTATOR_FOLLOW;
			specClient = -2;
		} else if ( level.numPlayingClients == 1 ) {
			specState = SPECTATOR_FOLLOW;
			specClient = -1;
		} else {
			specState = SPECTATOR_FREE;
		}
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
		G_SetSpectateOnly( client );
	} else if ( g_gametype.integer == GT_RED_ROVER && !force ) {
		team = g_rrInfected.integer ? G_RoverJoinTeam(clientNum) : PickTeam(clientNum);
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		if ( !force && !AllowTeamSwitch( clientNum, team ) ) {
			return -1;
		}

	} else {
		team = TEAM_FREE;
		if ( g_gametype.integer == GT_TOURNAMENT ) {
			// retail (qagamex86 0x10040bd1): asking to play joins the queue
			client->sess.spectateOnly = 0;
			trap_SendServerCommand( clientNum, "cp \"You are in the queue to play\n\"" );
		}
	}

	// a duel takes only the next in line while it lacks players; everyone
	// else asking waits as a spectator (retail qagamex86 0x10040c83)
	if ( !force && g_gametype.integer == GT_TOURNAMENT ) {
		if ( !( level.numNonSpectatorClients < 2 && G_NextInLine() == client )
			&& ( team != client->sess.sessionTeam || team == TEAM_SPECTATOR ) ) {
			team = TEAM_SPECTATOR;
		}
	} else if ( !force && g_maxGameClients.integer > 0 &&
		level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	// a request for the team already held changes nothing, but a spectator's
	// request always respawns it (retail qagamex86 0x10040cbe)
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return team;
	}

	//
	// execute the team change
	//

	// Retail 0x10040489 tests the viewed player state (a round spectator
	// may be following a living teammate), rather than the old entity health.
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue( ent );
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;

	if ( oldTeam != TEAM_SPECTATOR ) {


		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->s.health = ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SWITCHTEAM);
	}

	// a player leaves the duel queue; a spectator keeps its place, or joins
	// the end of the line (retail qagamex86 0x100404da)
	if ( team != TEAM_SPECTATOR ) {
		client->sess.queuePosition = 0;
		client->sess.spectatorTime = 0;
	} else {
		if ( !client->sess.queuePosition ) {
			client->sess.spectatorTime = trap_RealTime( NULL );
		}
		client->ps.eFlags |= EF_SPECTATOR;
		client->pers.readyToPlay = 0;
	}

	// retail resets the player's match stats on every team change
	// (qagamex86 0x10040543), except when red rover moves a player between
	// the playing teams
	if ( g_gametype.integer != GT_RED_ROVER || team == TEAM_SPECTATOR ) {
		G_ResetClientStats( client );
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;
	// retail (qagamex86 0x1004056f) catches a spectator up on the item timers
	if ( team == TEAM_SPECTATOR ) {
		G_SendItemTimers( clientNum );
	}

	client->sess.teamLeader = qfalse;
	(void)checkTeamLeader;

	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}

	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	G_WriteClientSessionData( client );

	BroadcastTeamChange( client, oldTeam );

	// nobody keeps revenge counts against a player who changes team
	// (retail qagamex86 0x10040644)
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		level.clients[level.sortedClients[i]].killedBy[clientNum] = 0;
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	if ( level.shuffling && g_gametype.integer == GT_RED_ROVER ) G_RespawnForShuffle(ent);
	else ClientBegin( clientNum );

	return team;
}


int SetTeam( gentity_t *ent, const char *team ) { return SetTeamInternal(ent, team, qfalse); }
int G_ForceTeam( gentity_t *ent, const char *team ) { return SetTeamInternal(ent, team, qtrue); }

/*
=================
StopFollowing

Retail qagamex86 0x10040d10: the follower floats free from where it
watched, keeping its team; it stays a spectator.
=================
*/
void StopFollowing( gentity_t *ent ) {
	gclient_t	*client = ent->client;

	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->ps.pm_type = PM_SPECTATOR;
	client->ps.pm_flags &= ~PMF_FOLLOW;
	client->sess.spectatorState = SPECTATOR_FREE;
	client->ps.clientNum = ent - g_entities;
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
}


/*
=================
G_CountAlivePlayers

Retail qagamex86 0x1006b170: the connected players moving normally, by
team.
=================
*/
void G_CountAlivePlayers( int counts[TEAM_NUM_TEAMS] ) {
	const gclient_t	*cl;
	int				i;

	memset( counts, 0, TEAM_NUM_TEAMS * sizeof( counts[0] ) );
	for ( i = 0; i < level.maxclients; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_CONNECTED && cl->ps.pm_type == PM_NORMAL ) {
			counts[cl->sess.sessionTeam]++;
		}
	}
}


/*
=================
G_FollowNextOrStop

When the followed player is gone (retail qagamex86 0x1003c8fd,
0x100355be, 0x10040f91): a player of a team still in the game, with both
teams alive and team spectating not free, follows the next teammate;
everyone else floats free.
=================
*/
void G_FollowNextOrStop( gentity_t *ent ) {
	int		counts[TEAM_NUM_TEAMS];

	G_CountAlivePlayers( counts );
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || g_teamSpecFreeCam.integer
		|| counts[TEAM_RED] <= 0 || counts[TEAM_BLUE] <= 0 ) {
		StopFollowing( ent );
	} else {
		G_FollowCycle( ent, 1 );
	}
}


/*
=================
Cmd_ReadyUp_f

Retail qagamex86 0x10045bd0: a player in the game toggles ready while the
match waits in PRE_GAME, and everyone hears about it.
=================
*/
static void Cmd_ReadyUp_f( gentity_t *ent ) {
	gclient_t	*client = ent->client;

	if ( level.warmupTime >= 0 || level.intermissiontime ) {
		return;
	}
	if ( client->ps.pm_type == PM_SPECTATOR || client->sess.sessionTeam == TEAM_SPECTATOR
		|| ( client->ps.pm_flags & PMF_FOLLOW ) ) {
		return;
	}

	client->pers.readyToPlay = !client->pers.readyToPlay;
	ClientUserinfoChanged( ent - g_entities );
	trap_SendServerCommand( -1, va( "cp \"%s ^7is ^7%s\n\"", client->pers.netname,
		client->pers.readyToPlay ? "Ready" : "Not Ready" ) );
}


/*
=================
Cmd_Team_f

Retail qagamex86 0x10040d90: with no argument it names the player's team;
a change may follow another only g_switchTeamDelay seconds after joining a
team, and a duelist leaving a match in progress takes a loss.
=================
*/
static void Cmd_Team_f( gentity_t *ent ) {
	char		s[MAX_TOKEN_CHARS];
	const char	*name;

	if ( trap_Argc() != 2 ) {
		switch ( ent->client->sess.sessionTeam ) {
		case TEAM_RED:
			name = "Red Team";
			break;
		case TEAM_BLUE:
			name = "Blue Team";
			break;
		case TEAM_SPECTATOR:
			name = "Spectator";
			break;
		default:
			name = "Team Free";
			break;
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", name ) );
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, va( "print \"^1May not switch teams more than once per %i seconds.^7\n\"",
			g_switchTeamDelay.integer ) );
		return;
	}

	if ( !level.warmupTime && g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	trap_Argv( 1, s, sizeof( s ) );

	if ( SetTeam( ent, s ) != -1 && ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		ent->client->switchTeamTime = level.time + g_switchTeamDelay.integer * 1000;
	}
}


/*
=================
Cmd_Follow_f

Retail qagamex86 0x10040f30.  With a name, follow that player unless it is
the follower or a spectator, leaving the game for the spectators first (a
duelist leaving a match books a loss).  "follow <name> pw" keeps following
a flag carrier.  Without a name, a follower moves on or floats free.
=================
*/
static void Cmd_Follow_f( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	gclient_t	*target;
	char		arg[MAX_TOKEN_CHARS];
	int			i;

	if ( trap_Argc() < 2 ) {
		if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			G_FollowNextOrStop( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	if ( client->sess.spectatorState == SPECTATOR_FOLLOW && trap_Argc() >= 3 ) {
		trap_Argv( 2, arg, sizeof( arg ) );
		target = &level.clients[client->sess.spectatorClient];
		if ( !strcmp( arg, "pw" ) && ( target->ps.powerups[PW_REDFLAG] || target->ps.powerups[PW_BLUEFLAG]
			|| target->ps.powerups[PW_NEUTRALFLAG] ) ) {
			return;
		}
	}

	target = &level.clients[i];
	if ( target == client || target->ps.pm_type == PM_SPECTATOR ) {
		return;
	}

	if ( !level.warmupTime && g_gametype.integer == GT_TOURNAMENT && client->sess.sessionTeam == TEAM_FREE ) {
		client->sess.losses++;
	}
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}
	client->sess.spectatorState = SPECTATOR_FOLLOW;
	client->sess.spectatorClient = i;
}


/*
=================
G_FollowCycle

Retail qagamex86 0x10041130: follow the next (or previous) player in the
game that is not following someone itself; in a team mode a team member
follows only its own team.  Starting to follow brings the scoreboard.
Race also sends the current run and upcoming checkpoint entities.
=================
*/
void G_FollowCycle( gentity_t *ent, int dir ) {
	gclient_t	*client = ent->client;
	gclient_t	*cl;
	int			clientnum, original;

	if ( dir != 1 && dir != -1 ) {
		G_Error( "FollowCycle: bad dir %i", dir );
	}

	clientnum = client->sess.spectatorClient;
	original = clientnum;
	if ( clientnum < 0 || clientnum >= level.maxclients ) {
		G_Printf( "FollowCycle: bad input clientnum value: %d, maxclients: %d\n", clientnum, level.maxclients );
		original = 0;
	}

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		} else if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		cl = &level.clients[clientnum];
		if ( cl->pers.connected != CON_CONNECTED || cl->ps.pm_type == PM_SPECTATOR || ( cl->ps.pm_flags & PMF_FOLLOW ) ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR && g_gametype.integer >= GT_TEAM
			&& cl->sess.sessionTeam != client->sess.sessionTeam ) {
			continue;
		}

		client->sess.spectatorClient = clientnum;
		if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
			DeathmatchScoreboardMessage( ent );
		}
		client->sess.spectatorState = SPECTATOR_FOLLOW;
		if ( g_gametype.integer == GT_RACE ) G_RaceInfo(ent, cl);
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
=================
Cmd_FollowCycle_f

Retail qagamex86 0x100412e0: follownext and followprev leave the game for
the spectators first (a duelist leaving a match books a loss).
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	gclient_t	*client = ent->client;

	if ( !level.warmupTime && g_gametype.integer == GT_TOURNAMENT && client->sess.sessionTeam == TEAM_FREE ) {
		client->sess.losses++;
	}
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}
	G_FollowCycle( ent, dir );
}


/*
==================
G_Say
==================
*/
static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	// Retail 10041820: muted tells are suppressed; broadcast chat exposes
	// only a red marker to the sender and higher-privilege recipients.
	if ( ent->client->sess.muted ) {
		if ( mode == SAY_TELL || (other != ent && other->client->pers.privileges <= ent->client->pers.privileges) ) return;
		message = "(muted)";
		color = COLOR_RED;
	}
	trap_SendServerCommand( other-g_entities, va( "%s \"%02i %s%c%c%s\"", mode == SAY_TEAM ? "tchat" : "chat",
		(int)(ent - g_entities), name, Q_COLOR_ESCAPE, color, message ) );
}

#define EC		"\x19"

static void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64 + 64 + 12]; // name + location + formatting
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s^7)"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && target->inuse && target->client && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s^7)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}


/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( (unsigned)targetNum >= (unsigned)level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

#ifdef MISSIONPACK

/* Retail 10041e40/10041e60 consume legacy voice commands without sending
 * Q3 voice packets. The vtell logger and vtaunt bookkeeping still run.
 * tdm_commands locks down all six transports, including muted callers. */
void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	(void)ent; (void)target; (void)mode; (void)id; (void)voiceonly;
}


/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_GAUNTLET) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
#endif


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( (unsigned)player >= MAX_CLIENTS ) {
		return;
	}
	if ( (unsigned) order >= ARRAY_LEN( gc_orders ) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}


/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

/*
==================
Cmd_Forfeit_f

Retail qagamex86 0x100456b0 and 0x10057cd0: a player may give up a match
in progress under g_allowForfeit, though not during a timeout or a round
countdown, and not in free-for-all, race or red rover: in duel only from
behind, in a team game only as the last player of the losing team.
==================
*/
static const char *const gametypeNames[GT_MAX_GAME_TYPE] = {
	"Free For All", "Duel", "Race", "Team Deathmatch", "Clan Arena", "Capture the Flag",
	"One Flag CTF", "Overload", "Harvester", "Freeze Tag", "Domination", "Attack and Defend",
	"Red Rover"
};

const char *G_GametypeName( int mode ) {
	return mode >= 0 && mode < GT_MAX_GAME_TYPE ? gametypeNames[mode] : "Unknown";
}

static void Cmd_Forfeit_f( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	const int	gametype = g_gametype.integer;
	const char	*refusal;
	int			red, blue, rank;

	if ( client->ps.pm_type == PM_SPECTATOR ) {
		return;
	}
	if ( level.timeoutStartTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"Forfeit is not available during a pause or timeout.\n\"" );
		return;
	}
	if ( G_IsRoundBasedGametype( gametype ) && ( level.warmupTime > 0 || G_RoundInProgress() ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"Forfeit is not available during a round countdown.\n\"" );
		return;
	}

	if ( gametype == GT_FFA || gametype == GT_RACE || gametype == GT_RED_ROVER ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"Forfeit is not available in %s.\n\"", gametypeNames[gametype] ) );
		return;
	}

	if ( !g_allowForfeit.integer ) {
		refusal = "Forfeits are not enabled on this server.\n";
	} else if ( level.warmupTime ) {
		refusal = "Forfeit is not available in warmup.\n";
	} else if ( gametype == GT_TOURNAMENT ) {
		rank = client->ps.persistant[PERS_RANK];
		if ( rank && ( rank & ~RANK_TIED_FLAG ) ) {
			client->ps.persistant[PERS_SCORE] = -999;
			G_Forfeit();
			return;
		}
		refusal = "Forfeit is only available to the losing player.\n";
	} else {
		red = level.teamScores[TEAM_RED];
		blue = level.teamScores[TEAM_BLUE];
		if ( client->sess.sessionTeam == TEAM_RED && red < blue ) {
			if ( TeamCount( -1, TEAM_RED ) == 1 ) {
				G_Forfeit();
				return;
			}
			refusal = "Forfeit is only available to the last remaining player on the losing team.\n";
		} else if ( client->sess.sessionTeam == TEAM_BLUE && blue < red ) {
			if ( TeamCount( -1, TEAM_BLUE ) == 1 ) {
				G_Forfeit();
				return;
			}
			refusal = "Forfeit is only available to the last remaining player on the losing team.\n";
		} else {
			refusal = "Forfeit is only available to members of the losing team.\n";
		}
	}
	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", refusal ) );
}


/*
==================
G_MapVote

Retail qagamex86 0x10043ff0: at the intermission "vote <1-3>" picks the
next map, once every two seconds and only while the twenty-second vote
runs; a vote also counts as ready to leave.
==================
*/
static void G_MapVote( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	char		arg[64];
	int			vote;

	if ( g_voteFlags.integer & 0x800 ) {
		return;
	}
	trap_Cvar_VariableStringBuffer( "nextmaps", arg, sizeof( arg ) );
	if ( !arg[0] ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	vote = atoi( arg );

	if ( level.time - level.voteTime >= 20000 ) {
		trap_SendServerCommand( ent - g_entities, "print \"Voting time has expired.\n\"" );
		return;
	}
	if ( level.time - client->pers.mapVoteTime < 2000 ) {
		trap_SendServerCommand( ent - g_entities, "print \"You may only vote once every 2 seconds.\n\"" );
		return;
	}
	if ( vote == client->pers.mapVote ) {
		trap_SendServerCommand( ent - g_entities, "print \"You already voted for this arena.\n\"" );
		return;
	}
	if ( vote < 1 || vote > 3 || !level.voteMaps[vote - 1][0] || !Q_stricmp( "default", level.voteMaps[vote - 1] ) ) {
		return;
	}

	client->pers.mapVoteTime = level.time;
	client->pers.mapVote = vote;
	G_CountMapVotes();
	trap_SendServerCommand( ent - g_entities, "disable_vote_ui" );
	trap_SendServerCommand( -1, va( "print \"%s voted for %s.\n\"", client->pers.netname, level.voteTitles[vote - 1] ) );
	if ( !client->pers.readyToPlay ) {
		client->pers.readyToPlay = 1;
		ClientUserinfoChanged( ent - g_entities );
	}
}


/*
==================
Cmd_Vote_f
==================
*/
static void Cmd_Vote_f( gentity_t *ent ) {
	if ( level.intermissiontime ) {
		G_MapVote( ent );
	} else {
		G_CastVote( ent );
	}
}


/*
=================
Cmd_SetViewpos_f
=================
*/
static void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, "print \"usage: setviewpos x y z yaw\n\"");
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}



/*
=================
Cmd_Stats_f
=================
*/
static void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}


/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client )
		return;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( ent->client->pers.connected != CON_CONNECTED ) {
		if ( ent->client->pers.connected == CON_CONNECTING && g_gametype.integer >= GT_TEAM ) {
			if ( Q_stricmp( cmd, "team" ) == 0 && !level.restarted ) {
				Cmd_Team_f( ent ); // early team override
			}
		}
		return;	// not fully in game yet
	}

	// the match and admin commands come first (retail qagamex86 0x10045e28)
	if ( G_AdminCommand( ent, cmd ) ) {
		return;
	}

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}
	if (Q_stricmp(cmd, "botSay") == 0) {
		Cmd_BotSay_f(ent);
		return;
	}
#ifdef MISSIONPACK
	if (Q_stricmp (cmd, "vsay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		Cmd_VoiceTaunt_f ( ent );
		return;
	}
#endif
	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}
	// acc brings the scoreboard along (retail qagamex86 0x10045fa4)
	if ( Q_stricmp( cmd, "acc" ) == 0 ) {
		Cmd_Score_f( ent );
		Cmd_WeaponAccuracy_f( ent, "acc" );
		return;
	}
	if ( Q_stricmp( cmd, "pstats" ) == 0 ) {
		Cmd_WeaponAccuracy_f( ent, "pstats" );
		return;
	}

	// ready-up, ragequit and votes also work at the intermission; from the
	// moment the match ends every other command is ignored (retail
	// qagamex86 0x10045fe3-0x1004602c)
	if ( Q_stricmp( cmd, "readyup" ) == 0 ) {
		Cmd_ReadyUp_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ragequit" ) == 0 ) {
		// the engine decides what a rage quit does (retail 0x1004600e)
		trap_MarkClientGotCP( ent->s.clientNum );
		return;
	}
	if ( Q_stricmp( cmd, "vote" ) == 0 ) {
		Cmd_Vote_f( ent );
		return;
	}
	if ( level.intermissionQueued || level.intermissiontime ) {
		return;
	}

	if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "dropflag") == 0)
		G_DropItemCommand( ent, DROP_FLAG );
	else if (Q_stricmp (cmd, "droppowerup") == 0)
		G_DropItemCommand( ent, DROP_POWERUP );
	else if (Q_stricmp (cmd, "droprune") == 0)
		G_DropItemCommand( ent, DROP_RUNE );
	else if (Q_stricmp (cmd, "dropweapon") == 0)
		G_DropItemCommand( ent, DROP_WEAPON );
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp(cmd, "spec") == 0)
		Cmd_Spec_f(ent);
	else if (Q_stricmp(cmd, "racepoint") == 0)
		Cmd_RacePoint_f(ent);
	else if (Q_stricmp(cmd, "raceinit") == 0)
		G_ResetRace(ent->client);
	else if (Q_stricmp (cmd, "forfeit") == 0)
		Cmd_Forfeit_f (ent);
	else if (Q_stricmp (cmd, "specresp") == 0)	// the retail cgame asks for the item timers
		G_SendItemTimers( ent - g_entities );
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else if (Q_stricmp (cmd, "stats") == 0)
		Cmd_Stats_f( ent );
	else
		trap_SendServerCommand( clientNum, va( "print \"unknown cmd %s\n\"", cmd ) );
}
