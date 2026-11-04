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

// Retail CA controller 0x10038230 and frame dispatch 0x10059a55. State is
// private to qagame; only the retail configstrings cross the module boundary.
enum { ROUND_IDLE, ROUND_COUNTDOWN, ROUND_SHUFFLE, ROUND_PLAY, ROUND_OVER, ROUND_EXIT };

static int RoundDeadline( long long delay ) {
	long long time = (long long)level.time + delay;
	return time > INT_MAX ? INT_MAX : time < 1 ? 1 : (int)time;
}

static void RoundCounts( int *alive, int *health ) {
	int i;
	memset( alive, 0, sizeof(int) * TEAM_NUM_TEAMS );
	memset( health, 0, sizeof(int) * TEAM_NUM_TEAMS );
	for ( i = 0; i < level.maxclients; ++i ) {
		gentity_t *ent = &g_entities[i];
		gclient_t *cl = ent->client;
		if ( !cl || cl->pers.connected != CON_CONNECTED || cl->ps.pm_type != PM_NORMAL ) continue;
		if ( cl->sess.sessionTeam != TEAM_RED && cl->sess.sessionTeam != TEAM_BLUE ) continue;
		alive[cl->sess.sessionTeam]++;
		health[cl->sess.sessionTeam] += ent->health;
	}
}

static void RoundPublishCounts( void ) {
	int alive[TEAM_NUM_TEAMS], health[TEAM_NUM_TEAMS];
	RoundCounts( alive, health );
	trap_SetConfigstring( CS_RED_PLAYERS_LEFT, va("%d", alive[TEAM_RED]) );
	trap_SetConfigstring( CS_BLUE_PLAYERS_LEFT, va("%d", alive[TEAM_BLUE]) );
}

void G_UpdateRoundCounts( void ) {
	if ( !G_IsRoundBasedGametype(g_gametype.integer) ) return;
	// Retail 0x100593e0: strictly more than 250 ms between publications.
	if ( level.time > level.roundCountTime && level.time - level.roundCountTime <= 250 ) return;
	level.roundCountTime = level.time;
	if ( level.roundState ) RoundPublishCounts();
	else {
		trap_SetConfigstring(CS_RED_PLAYERS_LEFT, va("%d", TeamCount(-1, TEAM_RED)));
		trap_SetConfigstring(CS_BLUE_PLAYERS_LEFT, va("%d", TeamCount(-1, TEAM_BLUE)));
	}
}

static const char *RoundExitReason( void ) {
	int red = level.teamScores[TEAM_RED], blue = level.teamScores[TEAM_BLUE];
	if ( g_gametype.integer == GT_ATTACK_DEFEND && level.adTurn != 1 ) return NULL;
	if ( g_gametype.integer == GT_RED_ROVER ) {
		if ( level.numPlayingClients > 1 && level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] == level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] ) return NULL;
		if ( g_timelimit.integer && (long long)level.time - level.startTime >= (long long)g_timelimit.integer * 60000 ) return "Timelimit hit.";
		if ( roundlimit.integer && red + blue >= roundlimit.integer ) return "Roundlimit hit.";
		return NULL;
	}
	if ( red == blue ) return NULL;
	if ( g_timelimit.integer && (long long)level.time - level.startTime >= (long long)g_timelimit.integer * 60000 ) return "Timelimit hit.";
	if ( g_gametype.integer == GT_ATTACK_DEFEND ) {
		if ( scorelimit.integer && (red >= scorelimit.integer || blue >= scorelimit.integer) ) return "Scorelimit hit.";
	} else if ( roundlimit.integer && (red >= roundlimit.integer || blue >= roundlimit.integer) ) return "Roundlimit hit.";
	if ( mercylimit.integer && (long long)level.time - level.startTime >= (long long)g_mercytime.integer * 60000 + level.overtimeAdded
		&& abs(red - blue) >= mercylimit.integer ) return "Mercylimit hit.";
	return NULL;
}

static void InfectedPrepare( void ) {
	int i;
	level.shuffling = qtrue;
	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *cl = &level.clients[i];
		if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam == TEAM_RED ) G_ForceTeam(&g_entities[i], "blue");
	}
	if ( level.infectedLast >= 0 && level.infectedLast < level.maxclients
		&& level.clients[level.infectedLast].pers.connected == CON_CONNECTED && level.clients[level.infectedLast].sess.sessionTeam == TEAM_BLUE ) G_ForceTeam(&g_entities[level.infectedLast], "red");
	if ( !TeamCount(-1, TEAM_RED) && level.numPlayingClients > 0 ) {
		int slot = level.sortedClients[rand() % level.numPlayingClients];
		G_ForceTeam(&g_entities[slot], "red");
		level.infectedFirst = slot;
	}
	level.shuffling = qfalse;
}

static void InfectedBonus( qboolean death ) {
	int i;
	if ( !g_rrInfected.integer || !g_rrInfectedSurvivorScoreMethod.integer ) return;
	if ( !level.infectedScoreTime ) level.infectedScoreTime = RoundDeadline((long long)g_rrInfectedSurvivorScoreRate.integer * 1000);
	if ( (g_rrInfectedSurvivorScoreMethod.integer == 1 && level.time > level.infectedScoreTime)
		|| (g_rrInfectedSurvivorScoreMethod.integer == 2 && death) ) {
		gentity_t *event;
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *cl = &level.clients[i];
			if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != TEAM_BLUE ) continue;
			cl->ps.persistant[PERS_SCORE] += g_rrInfectedSurvivorScoreBonus.integer;
			trap_SendServerCommand(i, va("print \"Survival Bonus! +%i\n\"", g_rrInfectedSurvivorScoreBonus.integer));
		}
		event = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND);
		event->r.svFlags |= SVF_BROADCAST;
		event->s.eventParm = 24; // retail 0x100653bf
		event->s.modelindex2 = TEAM_BLUE;
		if ( g_rrInfectedSurvivorScoreMethod.integer == 1 ) level.infectedScoreTime = RoundDeadline((long long)g_rrInfectedSurvivorScoreRate.integer * 1000);
	}
	CalculateRanks();
}

static void InfectedSpread( void ) {
	long long elapsed = (long long)level.time - level.infectedSpreadTime;
	long long limit = (long long)g_rrInfectedSpreadTime.integer * 1000;
	long long warning = limit - (long long)g_rrInfectedSpreadWarningTime.integer * 1000;
	int i;
	if ( !g_rrInfected.integer || !g_rrInfectedSpreadTime.integer || level.infectedSpreadTime == -1 || TeamCount(-1, TEAM_BLUE) <= 1 ) return;
	if ( g_rrInfectedSpreadWarningTime.integer && elapsed > warning && elapsed - warning <= level.msec ) trap_SendServerCommand(-1, va("cp \" ^3Infection spreads in %i second%s!\"", g_rrInfectedSpreadWarningTime.integer, g_rrInfectedSpreadWarningTime.integer == 1 ? "" : "s"));
	if ( elapsed <= limit ) return;
	for ( i = level.numPlayingClients - 1; i >= 0; --i ) {
		int slot = level.sortedClients[i];
		if ( level.clients[slot].sess.sessionTeam != TEAM_BLUE ) continue;
		level.shuffling = qtrue;
		G_ForceTeam(&g_entities[slot], "red");
		level.shuffling = qfalse;
		level.infectedSpreadTime = level.time;
		break;
	}
}

// Retail 0x100642d0 / 0x10055fa0 sorts the round winner list separately
// from the match scoreboard. Build from client slots so tied round scores
// do not inherit the ordering of a previous match-score sort.
static int QDECL RoundScoreOrder( const void *a, const void *b ) {
	int left = level.clients[*(const int *)a].roundScore;
	int right = level.clients[*(const int *)b].roundScore;
	return (left < right) - (left > right);
}

// Retail 0x100364a0 / 0x100365f0: twenty alternating attack scores,
// -2 for the next turn, -1 for unused cells; retain complete round pairs.
static void AttackScores( qboolean reset ) {
	char command[512] = "scores_ad";
	int i, first;
	if ( reset ) {
		level.adTurns = 0;
		memset(level.adHistory, -1, sizeof(level.adHistory));
	} else {
		int team = level.adTurn + TEAM_RED;
		level.adHistory[level.adTurns % 20] = level.teamScores[team] - level.adLastScore[level.adTurn];
		level.adLastScore[level.adTurn] = level.teamScores[team];
		level.adTurns++;
	}
	first = level.adTurns < 20 ? 0 : (level.adTurns + 1) / 2 * 2 - 20;
	for ( i = first; i < first + 20; ++i ) {
		int score = i < level.adTurns ? level.adHistory[i % 20] : i == level.adTurns && !reset && level.adTurns < 20 ? -2 : -1;
		Q_strcat(command, sizeof(command), va(" %d", score));
	}
	Q_strcat(command, sizeof(command), va(" %d %d", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]));
	trap_SendServerCommand(-1, command);
}

static void RoundEnter( int state ) {
	int i, winner = 0;
	qboolean awarded[MAX_CLIENTS] = { qfalse };
	qboolean freeze = g_gametype.integer == GT_FREEZE;
	qboolean rover = g_gametype.integer == GT_RED_ROVER;
	qboolean attack = g_gametype.integer == GT_ATTACK_DEFEND;
	int delay = freeze && level.roundNumber > 1 ? g_freezeRoundDelay.integer : g_roundWarmupDelay.integer;
	if ( freeze && !g_freezeRoundDelay.integer ) delay = 4000;
	int alive[TEAM_NUM_TEAMS], health[TEAM_NUM_TEAMS];
	gentity_t *event;
	level.roundState = state;
	level.roundDeadline = 0;
	level.roundStartTime = level.time;
	switch ( state ) {
	case ROUND_IDLE:
		if ( attack ) {
			AttackScores(qtrue);
			trap_SetConfigstring(CS_MATCH_STATE, "\\time\\-1\\round\\0\\turn\\0\\state\\0");
			return;
		}
		trap_SetConfigstring(CS_MATCH_STATE, "\\time\\-1\\round\\0");
		return;
	case ROUND_SHUFFLE:
		if ( rover && g_rrInfected.integer ) InfectedPrepare();
		else G_ShuffleTeams();
		level.roundNextState = ROUND_COUNTDOWN;
		level.roundDeadline = RoundDeadline(1);
		return;
	case ROUND_COUNTDOWN:
		level.roundRespawning = qtrue;
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *cl = &level.clients[i];
			if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR ) continue;
			if ( !freeze || cl->ps.pm_type == PM_DEAD || cl->ps.pm_type == PM_SPECTATOR
				|| (cl->ps.pm_type == PM_FREEZE && (g_freezeThawWinningTeam.integer || cl->sess.sessionTeam != level.roundWinner)) ) {
				if ( cl->ps.pm_type == PM_FREEZE ) G_TempEntity(g_entities[i].r.currentOrigin, EV_THAW_PLAYER);
				cl->ps.pm_type = attack ? PM_FREEZE : PM_NORMAL;
				ClientSpawn(&g_entities[i]);
			}
			if ( freeze ) {
				if ( g_freezeRemovePowerupsOnRound.integer ) {
					int p;
					for ( p = PW_QUAD; p <= PW_REGEN; ++p ) if ( cl->ps.powerups[p] > level.time + 3000 ) cl->ps.powerups[p] = level.time + 3000;
				}
				G_ResetFreezeLoadout(&g_entities[i]);
			}
			if ( rover ) cl->roundScore = 0;
			if ( !freeze || g_freezeRoundDelay.integer ) cl->ps.pm_flags |= PMF_ATTACK_LOCKOUT;
		}
		level.roundRespawning = qfalse;
		// Retail 0x1004c4e9 removes dropped timed powerups at a reset.
		if ( freeze && g_freezeRemovePowerupsOnRound.integer ) {
			for ( i = MAX_CLIENTS; i < level.num_entities; ++i ) {
				gentity_t *drop = &g_entities[i];
				if ( drop->inuse && (drop->flags & FL_DROPPED_ITEM) && drop->item && drop->item->giType == IT_POWERUP ) G_FreeEntity(drop);
			}
		}
		if ( !attack ) level.roundNumber = level.teamScores[TEAM_RED] + level.teamScores[TEAM_BLUE] + 1;
		level.roundNextState = ROUND_PLAY;
		level.roundDeadline = RoundDeadline(delay);
		if ( !delay ) RoundEnter(ROUND_PLAY);
		if ( attack ) trap_SetConfigstring(CS_MATCH_STATE, va("\\time\\%d\\round\\%d\\turn\\%d\\state\\1", RoundDeadline(delay), level.roundNumber, level.adTurn));
		else if ( !freeze || g_freezeRoundDelay.integer ) trap_SetConfigstring(CS_MATCH_STATE, va("\\time\\%d\\round\\%d", RoundDeadline(delay), level.roundNumber));
		return;
	case ROUND_PLAY:
		if ( rover ) { level.infectedLast = -1; level.infectedScoreTime = 0; level.infectedSpreadTime = level.time; }
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *cl = &level.clients[i];
			if ( cl->pers.connected != CON_CONNECTED ) continue;
			cl->ps.pm_flags &= ~PMF_ATTACK_LOCKOUT;
			if ( attack && cl->sess.sessionTeam != TEAM_SPECTATOR ) cl->ps.pm_type = PM_NORMAL;
			cl->pers.roundShotsBase = cl->accuracy_shots;
			cl->pers.roundHitsBase = cl->accuracy_hits;
			cl->pers.roundTakenBase = cl->stats.damageTaken;
			if ( attack ) cl->adDamaged = qfalse;
			if ( g_spawnArmor.integer ) cl->ps.powerups[PW_SPAWNARMOR] = level.time / 1000 * 1000 + g_spawnArmor.integer;
		}
		trap_SetConfigstring(CS_ROUND_START_TIME, va("%d", level.time));
		if ( attack ) {
			level.adTouched = level.adCaptured = qfalse;
			trap_SetConfigstring(CS_MATCH_STATE, va("\\turn\\%d\\state\\2", level.adTurn));
		} else trap_SetConfigstring(CS_MATCH_STATE, va("\\round\\%d", level.roundNumber));
		return;
	case ROUND_OVER:
		RoundCounts(alive, health);
		if ( attack ) {
			winner = level.adCaptured ? level.adTurn + TEAM_RED : alive[TEAM_RED] ? (alive[TEAM_BLUE] ? TEAM_FREE : TEAM_RED) : alive[TEAM_BLUE] ? TEAM_BLUE : TEAM_FREE;
			if ( !level.adCaptured && winner == level.adTurn + TEAM_RED ) level.teamScores[winner] += g_adElimScoreBonus.integer;
			for ( i = 0; i < level.maxclients; ++i ) {
				gclient_t *cl = &level.clients[i];
				int shots = cl->accuracy_shots - cl->pers.roundShotsBase;
				if ( cl->pers.connected != CON_CONNECTED ) continue;
				if ( cl->sess.sessionTeam != TEAM_SPECTATOR ) {
					cl->ps.pm_flags |= PMF_ATTACK_LOCKOUT;
					cl->ps.powerups[PW_REDFLAG] = cl->ps.powerups[PW_BLUEFLAG] = 0;
				}
				if ( shots > 0 && (cl->accuracy_hits - cl->pers.roundHitsBase) * 100 / shots > 50 ) G_AwardMedal(&g_entities[i], MEDAL_ACCURACY);
				if ( !cl->adDamaged ) G_AwardMedal(&g_entities[i], MEDAL_PERFECT);
			}
			CalculateRanks();
			AttackScores(qfalse);
			Team_ResetFlags();
			level.roundWinner = winner;
			level.roundNextState = RoundExitReason() ? ROUND_EXIT : ROUND_COUNTDOWN;
			level.roundDeadline = RoundDeadline(level.roundNextState == ROUND_EXIT ? 1500 : 3500);
			if ( level.roundNextState != ROUND_EXIT ) {
				event = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND);
				event->r.svFlags |= SVF_BROADCAST;
				event->s.eventParm = winner == TEAM_RED ? GTS_REDTEAM_WINS_ROUND : winner == TEAM_BLUE ? GTS_BLUETEAM_WINS_ROUND : GTS_ROUND_DRAW;
				trap_SetConfigstring(CS_ROUND_START_TIME, "-1");
				level.adTurn ^= 1;
				if ( !level.adTurn ) level.roundNumber++;
			}
			return;
		}
		if ( rover ) {
			for ( i = 0; i < level.maxclients; ++i ) {
				gclient_t *cl = &level.clients[i];
				if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
					cl->ps.persistant[PERS_SCORE] += g_rrRoundScoreBonus.integer;
					cl->roundScore += g_rrRoundScoreBonus.integer;
				}
			}
		}
		if ( !alive[TEAM_BLUE] && alive[TEAM_RED] ) winner = TEAM_RED;
		else if ( !alive[TEAM_RED] && alive[TEAM_BLUE] ) winner = TEAM_BLUE;
		else if ( alive[TEAM_RED] && alive[TEAM_BLUE] ) {
			if ( rover ) {
				int team = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] || g_rrInfected.integer ? TEAM_BLUE : TEAM_RED;
				level.teamScores[team]++;
				CalculateRanks();
			}
			else if ( g_roundDrawLivingCount.integer && alive[TEAM_RED] != alive[TEAM_BLUE] ) winner = alive[TEAM_RED] > alive[TEAM_BLUE] ? TEAM_RED : TEAM_BLUE;
			else if ( g_roundDrawHealthCount.integer && health[TEAM_RED] != health[TEAM_BLUE] ) winner = health[TEAM_RED] > health[TEAM_BLUE] ? TEAM_RED : TEAM_BLUE;
		}
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *cl = &level.clients[i];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR && (!freeze || g_freezeRoundDelay.integer) ) cl->ps.pm_flags |= PMF_ATTACK_LOCKOUT;
		}
		if ( winner ) {
			level.teamScores[winner]++;
			CalculateRanks();
			if ( !rover && (!freeze || g_freezeRoundDelay.integer) ) {
				const char *color = winner == TEAM_RED ? "^1" : "^4";
				const char *name = winner == TEAM_RED ? "RED" : "BLUE";
				if ( alive[TEAM_RED] && alive[TEAM_BLUE] && (!g_roundDrawLivingCount.integer || alive[TEAM_RED] == alive[TEAM_BLUE]) )
					trap_SendServerCommand(-1, va("print \"%s%s TEAM^3 WINS the round!^7 (^1%i^7 hp vs ^4%i^7 hp)\n\"", color, name, health[TEAM_RED], health[TEAM_BLUE]));
				else trap_SendServerCommand(-1, va("print \"%s%s TEAM^3 WINS the round!^7 (%i %s remaining)\n\"", color, name,
					alive[winner] > 1 ? alive[winner] : health[winner], alive[winner] > 1 ? "players" : "hp"));
			}
		}
		if ( rover ) {
			char winners[MAX_STRING_CHARS] = "";
			int slots[MAX_CLIENTS], count = 0;
			int best = INT_MIN;
			for ( i = 0; i < level.maxclients; ++i ) {
				gclient_t *cl = &level.clients[i];
				if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR ) continue;
				slots[count++] = i;
				if ( cl->roundScore > best ) best = cl->roundScore;
			}
			qsort(slots, count, sizeof(slots[0]), RoundScoreOrder);
			for ( i = 0; i < count; ++i ) {
				int slot = slots[i];
				if ( g_rrInfected.integer ? (level.infectedLast >= 0 ? slot == level.infectedLast : level.clients[slot].sess.sessionTeam == TEAM_BLUE) : winner && level.clients[slot].roundScore == best ) { awarded[slot] = qtrue; Q_strcat(winners, sizeof(winners), va("%d ", slot)); }
			}
			trap_SetConfigstring(0x2bf, winners); // retail RR winner client list 0x10065014
		}
		// Retail 0x100385e0: per-round accuracy and undamaged winner awards.
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *cl = &level.clients[i];
			int shots = cl->accuracy_shots - cl->pers.roundShotsBase;
			if ( cl->pers.connected != CON_CONNECTED ) continue;
			if ( shots > 0 && (cl->accuracy_hits - cl->pers.roundHitsBase) * 100 / shots > 50 ) G_AwardMedal(&g_entities[i], MEDAL_ACCURACY);
			if ( (rover ? awarded[i] : winner && cl->sess.sessionTeam == winner) && cl->stats.damageTaken == cl->pers.roundTakenBase ) G_AwardMedal(&g_entities[i], MEDAL_PERFECT);
		}
		level.roundWinner = winner;
		if ( !rover ) level.roundNumber++;
		level.roundNextState = RoundExitReason() ? ROUND_EXIT : rover ? ROUND_SHUFFLE : ROUND_COUNTDOWN;
		level.roundDeadline = RoundDeadline(level.roundNextState == ROUND_EXIT ? 1500 : freeze ? (g_freezeRoundDelay.integer < 3500 ? g_freezeRoundDelay.integer : 3500) : 3500);
		if ( rover || level.roundNextState != ROUND_EXIT ) {
			event = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND);
			event->r.svFlags |= SVF_BROADCAST;
			event->s.eventParm = rover ? (winner ? 20 : GTS_ROUND_DRAW) : winner == TEAM_RED ? (freeze ? GTS_REDTEAM_SCORED : GTS_REDTEAM_WINS_ROUND) : winner == TEAM_BLUE ? (freeze ? GTS_BLUETEAM_SCORED : GTS_BLUETEAM_WINS_ROUND) : GTS_ROUND_DRAW;
			if ( level.roundNextState != ROUND_EXIT ) trap_SetConfigstring(CS_ROUND_START_TIME, "-1");
		}
		if ( freeze && !g_freezeRoundDelay.integer && level.roundNextState != ROUND_EXIT ) RoundEnter(ROUND_COUNTDOWN);
		return;
	case ROUND_EXIT: {
		const char *reason = RoundExitReason();
		if ( reason ) {
			if ( !strcmp(reason, "Timelimit hit.") ) trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			else if ( rover ) trap_SendServerCommand(-1, "print \"Game hit the roundlimit.\n\"");
			else trap_SendServerCommand(-1, va("print \"%s hit the %s\n\"", level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ? "Red" : "Blue", !strcmp(reason, "Roundlimit hit.") ? "roundlimit." : !strcmp(reason, "Scorelimit hit.") ? "scorelimit." : "mercylimit."));
			LogExit(reason);
			if ( rover ) G_ShuffleTeams();
		}
		return;
	}
	}
}

void G_InitRounds( void ) {
	if ( g_gametype.integer == GT_ATTACK_DEFEND ) {
		RoundEnter(level.warmupTime ? ROUND_IDLE : ROUND_COUNTDOWN);
		level.adTurn = level.roundNumber = 0;
		AttackScores(qtrue);
		return;
	}
	if ( g_gametype.integer == GT_RED_ROVER && g_rrInfected.integer ) {
		level.infectedFirst = level.infectedLast = level.infectedSpreadTime = -1;
		level.roundDeadline = RoundDeadline(500);
		level.roundNextState = level.warmupTime ? ROUND_IDLE : ROUND_SHUFFLE;
	} else if ( g_gametype.integer == GT_CLAN_ARENA || g_gametype.integer == GT_FREEZE || g_gametype.integer == GT_RED_ROVER ) RoundEnter(level.warmupTime ? ROUND_IDLE : ROUND_COUNTDOWN);
}

void G_RunRounds( void ) {
	int alive[TEAM_NUM_TEAMS], health[TEAM_NUM_TEAMS];
	if ( !G_IsRoundBasedGametype(g_gametype.integer) || level.intermissionQueued || level.intermissiontime ) return;
	if ( level.timeoutStartTime ) level.roundStartTime += level.msec;
	if ( level.warmupTime && !(g_gametype.integer == GT_RED_ROVER && level.warmupTime < 0) ) return;
	if ( level.roundDeadline && level.time >= level.roundDeadline ) RoundEnter(level.roundNextState);
	if ( level.roundState != ROUND_PLAY ) return;
	RoundCounts(alive, health);
	if ( g_gametype.integer == GT_RED_ROVER ) { InfectedBonus(qfalse); InfectedSpread(); alive[TEAM_RED] = TeamCount(-1, TEAM_RED); alive[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE); }
	if ( !alive[TEAM_RED] || !alive[TEAM_BLUE] || (g_gametype.integer == GT_ATTACK_DEFEND && level.adCaptured)
		|| (roundtimelimit.integer && (long long)level.time - level.roundStartTime >= (long long)roundtimelimit.integer * 1000) ) {
		if ( g_gametype.integer == GT_RED_ROVER && alive[TEAM_BLUE] == 1 ) {
			int i;
			for ( i = 0; i < level.maxclients; ++i ) if ( level.clients[i].pers.connected == CON_CONNECTED && level.clients[i].sess.sessionTeam == TEAM_BLUE ) level.infectedLast = i;
		}
		RoundEnter(ROUND_OVER);
	}
}

int G_AttackingTeam( void ) {
	if ( level.roundDeadline && level.time >= level.roundDeadline ) RoundEnter(level.roundNextState);
	return level.roundState == ROUND_PLAY ? TEAM_RED + level.adTurn : TEAM_FREE;
}

qboolean G_RoundDamage( gentity_t *attacker, gentity_t *target, int *health, int *armor ) {
	// Retail 0x10037fd0: separate self/team health and armor masks, followed
	// by the active-round gate and capped damage-score accumulation.
	int flags = g_dmflags.integer;
	if ( attacker == target ) {
		if ( flags & 4 ) *health = 0;
		if ( flags & 8 ) *armor = 0;
	} else if ( OnSameTeam(attacker, target) ) {
		if ( flags & 1 ) *health = 0;
		if ( flags & 2 ) *armor = 0;
	}
	if ( level.warmupTime ) return qtrue;
	if ( level.roundDeadline && level.time >= level.roundDeadline ) RoundEnter(level.roundNextState);
	if ( level.roundState != ROUND_PLAY || level.roundDeadline ) return qfalse;
	if ( (g_gametype.integer != GT_RED_ROVER || g_rrDamageScoreBonus.integer) && attacker->client && target->client && target->health > 0 && !OnSameTeam(attacker, target) ) {
		if ( *health > target->health ) *health = target->health;
		if ( *armor > target->client->ps.stats[STAT_ARMOR] ) *armor = target->client->ps.stats[STAT_ARMOR];
		attacker->client->pers.roundDamage += *health + *armor;
		if ( attacker->client->pers.roundDamage >= 100 ) {
			attacker->client->pers.roundDamage -= 100;
			int bonus = g_gametype.integer == GT_RED_ROVER ? g_rrDamageScoreBonus.integer : 1;
			attacker->client->ps.persistant[PERS_SCORE] += bonus;
			if ( g_gametype.integer == GT_RED_ROVER ) attacker->client->roundScore += bonus;
			CalculateRanks();
		}
	}
	return qtrue;
}

// Retail 0x1004bc80 / 0x1004cd40: a completed thaw shatters the frozen
// shell, announces an obituary/return sound, and respawns the same player.
static void FreezeThaw( gentity_t *ent, gentity_t *helper ) {
	gentity_t *event;
	gclient_t *cl = ent->client;
	if ( helper ) {
		AddScore(helper, ent->r.currentOrigin, 2);
		helper->client->pers.teamState.assists++;
		helper->client->stats.assists++;
		helper->client->ps.persistant[PERS_ASSIST_COUNT]++;
		helper->client->ps.eFlags = (helper->client->ps.eFlags & ~EF_AWARDS) | EF_AWARD_ASSIST;
		helper->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	}
	event = G_TempEntity(ent->r.currentOrigin, EV_OBITUARY);
	event->s.eventParm = MOD_THAW;
	event->s.otherEntityNum = cl->ps.clientNum;
	event->s.otherEntityNum2 = helper ? helper->s.number : ENTITYNUM_WORLD;
	event->r.svFlags = SVF_BROADCAST;
	event = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	event->s.eventParm = cl->sess.sessionTeam == TEAM_BLUE ? GTS_RED_RETURN : GTS_BLUE_RETURN;
	event->r.svFlags |= SVF_BROADCAST;
	G_AddEvent(ent, EV_GIB_PLAYER, DirToByte(cl->damage_from));
	G_TempEntity(ent->r.currentOrigin, EV_THAW_PLAYER);
	cl->ps.pm_type = PM_NORMAL;
	ClientSpawn(ent);
}

void G_FreezeDeath( gentity_t *ent, int mod ) {
	gclient_t *cl = ent->client;
	cl->ps.powerups[PW_FROZEN] = INT_MAX;
	ent->s.powerups = 1 << PW_FROZEN;
	cl->freezeTime = level.time;
	ent->die = player_die;
	if ( mod == MOD_FALLING || mod == MOD_TELEFRAG || mod == MOD_CRUSH ) G_FreezeRemove(ent);
}

void G_FreezeRemove( gentity_t *ent ) {
	gclient_t *cl = ent->client;
	int alive[TEAM_NUM_TEAMS], health[TEAM_NUM_TEAMS];
	gentity_t *event;
	cl->ps.powerups[PW_FROZEN] = 0;
	ent->s.powerups &= ~(1 << PW_FROZEN);
	cl->ps.pm_type = PM_DEAD;
	// The environmental delay controls damage to a frozen shell. Once
	// removed, retail 0x1004bce1 uses the ordinary minimum respawn delay.
	cl->respawnTime = RoundDeadline(g_respawn_delay_min.integer);
	ent->health = GIB_HEALTH;
	RoundCounts(alive, health);
	if ( !alive[cl->sess.sessionTeam] ) return;
	cl->thawing = qfalse;
	event = G_TempEntity(ent->r.currentOrigin, EV_OBITUARY);
	event->s.eventParm = MOD_THAW;
	event->s.otherEntityNum = cl->ps.clientNum;
	event->s.otherEntityNum2 = ENTITYNUM_WORLD;
	event->r.svFlags = SVF_BROADCAST;
	event = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	event->s.eventParm = cl->sess.sessionTeam == TEAM_BLUE ? GTS_RED_RETURN : GTS_BLUE_RETURN;
	event->r.svFlags |= SVF_BROADCAST;
	GibEntity(ent);
	cl->freezeTime = 0;
}

void G_FreezeThink( gentity_t *ent, int msec ) {
	gclient_t *cl = ent->client;
	gentity_t *helper = NULL;
	int i, oldSeconds;
	vec3_t mins, maxs;
	int nearby[MAX_GENTITIES], count;
	if ( !cl->ps.powerups[PW_FROZEN] ) return;
	if ( cl->thawRemaining <= g_freezeThawTime.integer / 3 ) cl->ps.generic1 |= 2;
	else if ( cl->thawRemaining <= g_freezeThawTime.integer / 3 * 2 ) cl->ps.generic1 |= 1;
	else cl->ps.generic1 &= ~3;
	if ( level.timeoutStartTime ) return;
	if ( !level.warmupTime && g_freezeRoundDelay.integer && level.roundState != ROUND_PLAY ) return;
	if ( !level.warmupTime && g_freezeAutoThawTime.integer > 0 && level.time - cl->freezeTime >= g_freezeAutoThawTime.integer ) {
		FreezeThaw(ent, NULL);
		return;
	}
	for ( i = 0; i < 3; ++i ) { mins[i] = ent->r.currentOrigin[i] - g_freezeThawRadius.integer; maxs[i] = ent->r.currentOrigin[i] + g_freezeThawRadius.integer; }
	count = trap_EntitiesInBox(mins, maxs, nearby, MAX_GENTITIES);
	if ( !level.warmupTime ) for ( i = 0; i < count; ++i ) {
		gentity_t *other = &g_entities[nearby[i]];
		trace_t trace;
		if ( other == ent || !other->client || other->health <= 0 || other->client->ps.powerups[PW_FROZEN] || !OnSameTeam(ent, other) ) continue;
		if ( !g_freezeThawThroughSurface.integer ) {
			trap_Trace(&trace, cl->ps.origin, NULL, NULL, other->client->ps.origin, ENTITYNUM_NONE, CONTENTS_SOLID);
			if ( trace.fraction != 1.0f ) continue;
		}
		helper = other;
		break;
	}
	if ( helper || level.warmupTime ) {
		// Retail 0x1004d18f retains the initial helper while that client is
		// still in the thaw box, even when the entity query order changes.
		if ( helper && cl->thawing && cl->thawClient >= 0 && cl->thawClient < level.maxclients ) {
			for ( i = 0; i < count; ++i ) if ( nearby[i] == cl->thawClient ) {
				helper = &g_entities[cl->thawClient];
				break;
			}
		}
		oldSeconds = cl->thawRemaining / 1000;
		cl->thawRemaining -= msec;
		if ( helper ) {
			if ( !cl->thawing ) cl->thawClient = helper->s.number;
			cl->thawing = qtrue;
			if ( oldSeconds > 0 && oldSeconds != cl->thawRemaining / 1000 && g_freezeThawTick.integer ) {
				gentity_t *event = G_TempEntity(ent->r.currentOrigin, EV_THAW_TICK);
				event->r.ownerNum = ent->s.number;
			}
		}
	} else {
		cl->thawing = qfalse;
		cl->thawRemaining += msec;
		if ( cl->thawRemaining > g_freezeThawTime.integer ) cl->thawRemaining = g_freezeThawTime.integer;
	}
	if ( cl->thawRemaining < 1 ) {
		if ( level.warmupTime ) {
			G_TempEntity(ent->r.currentOrigin, EV_THAW_PLAYER);
			cl->ps.pm_type = PM_NORMAL;
			ClientSpawn(ent);
		} else FreezeThaw(ent, helper);
	}
}


// Retail death dispatch 0x10065700 / 0x100389f0. Conversion is a session
// team change, without a second death or a spawn before the respawn delay.
void G_RoundDeath( gentity_t *ent ) {
	int alive[TEAM_NUM_TEAMS], health[TEAM_NUM_TEAMS], i;
	int team = ent->client->sess.sessionTeam;
	if ( level.warmupTime || level.roundState != ROUND_PLAY || level.intermissiontime || level.intermissionQueued ) return;
	if ( g_gametype.integer == GT_RED_ROVER && (team == TEAM_BLUE || (team == TEAM_RED && !g_rrInfected.integer)) ) {
		ent->client->sess.sessionTeam = OtherTeam(team);
		ClientUserinfoChanged(ent->s.number);
		if ( g_rrInfected.integer ) {
			gentity_t *event = G_TempEntity(ent->client->ps.origin, EV_INFECTED);
			event->r.svFlags |= SVF_SINGLECLIENT;
			event->r.singleClient = event->r.ownerNum = ent->s.number;
		}
	}
	RoundCounts(alive, health);
	if ( g_gametype.integer == GT_RED_ROVER && g_rrInfected.integer && team == TEAM_BLUE ) {
		level.infectedSpreadTime = level.time;
		InfectedBonus(qtrue);
		if ( !alive[TEAM_BLUE] ) level.infectedLast = ent->s.number;
	}
	if ( g_gametype.integer == GT_RED_ROVER && g_rrInfected.integer && team == TEAM_RED ) return;
	if ( !g_lastManStandingWarning.integer || !alive[TEAM_RED] || !alive[TEAM_BLUE] || alive[team] != 1 ) return;
	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *cl = &level.clients[i];
		if ( cl->pers.connected == CON_CONNECTED && cl->ps.pm_type == PM_NORMAL && cl->sess.sessionTeam == team ) {
			gentity_t *event = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND);
			event->r.svFlags |= SVF_BROADCAST;
			event->s.eventParm = GTS_LAST_STANDING;
			event->s.modelindex2 = team;
			trap_SendServerCommand(i, va("cp \"%s\n\"", g_lastManStandingMessage.string));
			break;
		}
	}
}

team_t G_RoverJoinTeam( int clientNum ) {
	if ( level.roundDeadline && level.time >= level.roundDeadline ) RoundEnter(level.roundNextState);
	return (level.roundState == ROUND_PLAY && !level.roundDeadline) || clientNum == level.infectedFirst || clientNum == level.infectedLast ? TEAM_RED : TEAM_BLUE;
}
