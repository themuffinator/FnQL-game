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

// Retail 0x1004ace0: signed progress, red negative and blue positive.
// Each map point owns its progress and capturer; level only owns the bounded
// point registry and the score batch so simultaneous ticks rank together.
static int DomDeadline( void ) {
	long long deadline = (long long)level.time + (long long)g_domScoreRate.integer * 1000;
	return deadline > INT_MAX ? INT_MAX : deadline < 1 ? 1 : (int)deadline;
}

static int DomStep( double value ) {
	// Server cvars can contain non-finite or out-of-range values. Limit
	// the conversion, not ordinary capture progress or its rounding order.
	if ( value != value ) return 0;
	if ( value > INT_MAX / 2 ) return INT_MAX / 2;
	if ( value < -INT_MAX / 2 ) return -INT_MAX / 2;
	return (int)value;
}

static void DomEvent( gentity_t *point, qboolean neutral ) {
	gentity_t *event = G_TempEntity(neutral ? point->s.pos.trBase : vec3_origin, EV_GLOBAL_TEAM_SOUND);
	event->r.svFlags |= SVF_BROADCAST;
	event->s.eventParm = 23;
	event->s.modelindex2 = point->s.modelindex2;
	if ( !neutral ) {
		event->s.powerups = level.warmupTime ? 0 : point->s.powerups;
		if ( level.warmupTime && point->activator && point->activator->client ) {
			// Retail 0x1004b4ad excludes the warmup capturer from this sound.
			event->r.svFlags |= SVF_NOTSINGLECLIENT;
			event->r.singleClient = point->activator->client->ps.clientNum;
		}
	}
}

static void DomAwards( gentity_t *point, gentity_t **players, int count ) {
	int i;
	for ( i = 0; i < count; ++i ) {
		gentity_t *player = players[i];
		gclient_t *cl = player->client;
		qboolean capture = player == point->activator;
		if ( capture ) {
			cl->pers.teamState.captures++;
			cl->stats.captures++;
			cl->ps.persistant[PERS_CAPTURES]++;
		} else {
			cl->pers.teamState.assists++;
			cl->stats.assists++;
			cl->ps.persistant[PERS_ASSIST_COUNT]++;
		}
		cl->ps.eFlags = (cl->ps.eFlags & ~EF_AWARDS) | (capture ? EF_AWARD_CAP : EF_AWARD_ASSIST);
		cl->rewardTime = level.time + REWARD_SPRITE_TIME;
		AddScore(player, point->r.currentOrigin, capture ? 25 : 15);
	}
}

static void DomCapture( gentity_t *point ) {
	gentity_t *players[TEAM_NUM_TEAMS][MAX_CLIENTS];
	int counts[TEAM_NUM_TEAMS] = { 0 }, i, team, base, delta, old = point->health;
	qboolean contestedNeutral = qfalse;
	for ( i = 0; i < level.maxclients; ++i ) {
		gentity_t *player = &g_entities[i];
		vec3_t distance;
		float squared;
		if ( !player->inuse || !player->client || player->client->pers.connected != CON_CONNECTED || player->health <= 0 ) continue;
		team = player->client->sess.sessionTeam;
		if ( team != TEAM_RED && team != TEAM_BLUE ) continue;
		VectorSubtract(point->r.currentOrigin, player->r.currentOrigin, distance);
		squared = (float)((double)distance[0]*distance[0] + (double)distance[1]*distance[1] + (double)distance[2]*distance[2]);
		if ( squared <= (double)point->count * point->count ) players[team][counts[team]++] = player;
	}
	team = counts[TEAM_RED] ? (counts[TEAM_BLUE] ? TEAM_FREE : TEAM_RED) : counts[TEAM_BLUE] ? TEAM_BLUE : TEAM_FREE;
	if ( team ) {
		qboolean found = qfalse;
		for ( i = 0; i < counts[team]; ++i ) if ( players[team][i] == point->activator ) found = qtrue;
		if ( !found ) point->activator = players[team][0];
	} else if ( counts[TEAM_RED] && counts[TEAM_BLUE] ) point->activator = NULL;
	point->s.otherEntityNum2 = team ? counts[team] : 0;
	base = g_domCapTime.integer > 0 ? level.msec / g_domCapTime.integer : 0;
	if ( g_domNeutralFlag.integer && g_domEnableContention.integer && counts[TEAM_RED] && counts[TEAM_BLUE] ) {
		delta = old ? DomStep((double)base * (1.0 + g_domTeammateCapScale.value) * (old > 0 ? -1.0 : 1.0)) : 0;
		contestedNeutral = delta && old == -delta;
	} else {
		delta = counts[TEAM_RED] ? DomStep(-(double)base * (1.0 + (double)g_domTeammateCapScale.value * (counts[TEAM_RED] - 1))) : 0;
		if ( counts[TEAM_BLUE] ) delta = DomStep((double)delta + base * (1.0 + (double)g_domTeammateCapScale.value * (counts[TEAM_BLUE] - 1)));
	}
	point->health = (int)Com_Clamp(-1000, 1000, (float)((double)old + delta));
	point->s.generic1 = abs(point->health) / 10;
	point->s.eventParm = point->health > 0 ? TEAM_BLUE : TEAM_RED;
	if ( counts[TEAM_RED] && counts[TEAM_RED] == counts[TEAM_BLUE] ) point->s.weapon = 2;
	else if ( counts[TEAM_RED] && counts[TEAM_BLUE] ) point->s.weapon = 1;
	else point->s.weapon = ((!point->s.modelindex2 && point->s.generic1 > 0 && point->s.generic1 < 100)
		|| (point->s.generic1 && point->s.generic1 < g_domDistressThreshold.value)) ? 1 : 0;
	if ( !delta ) return;
	if ( contestedNeutral || (point->s.modelindex2 && abs(old) < abs(base)) ) {
		if ( !g_domNeutralFlag.integer ) point->health = point->s.modelindex2 == TEAM_RED ? 1000 : -1000;
		point->s.modelindex2 = TEAM_FREE;
		if ( g_domNeutralFlag.integer ) DomEvent(point, qtrue);
	}
	if ( abs(point->health) == 1000 && !point->s.modelindex2 ) {
		team = point->health < 0 ? TEAM_RED : TEAM_BLUE;
		point->s.modelindex2 = team;
		DomEvent(point, qfalse);
		DomAwards(point, players[team], counts[team]);
	}
}

static void DomScore( gentity_t *point ) {
	int red, blue, oldRed, oldBlue;
	point->nextthink = DomDeadline();
	if ( level.warmupTime || level.intermissiontime || level.intermissionQueued ) return;
	if ( point->s.modelindex2 ) level.domPending[point->s.modelindex2]++;
	if ( ++level.domTickCount < level.domPointCount ) return;
	oldRed = level.teamScores[TEAM_RED]; oldBlue = level.teamScores[TEAM_BLUE];
	red = oldRed + level.domPending[TEAM_RED]; blue = oldBlue + level.domPending[TEAM_BLUE];
	if ( (oldRed <= oldBlue && red > blue) || (oldBlue <= oldRed && blue > red) ) {
		gentity_t *event = G_TempEntity(vec3_origin, EV_GLOBAL_TEAM_SOUND);
		event->r.svFlags |= SVF_BROADCAST;
		event->s.eventParm = red > blue ? GTS_REDTEAM_TOOK_LEAD : GTS_BLUETEAM_TOOK_LEAD;
	}
	level.teamScores[TEAM_RED] = red; level.teamScores[TEAM_BLUE] = blue;
	level.domTickCount = 0;
	memset(level.domPending, 0, sizeof(level.domPending));
	CalculateRanks();
}

static void DomPlace( gentity_t *point ) {
	vec3_t end, start, mins = { -15, -15, -15 }, maxs = { 15, 15, 35 };
	trace_t trace;
	VectorCopy(point->r.currentOrigin, start);
	VectorCopy(start, end); end[2] -= 256;
	trap_Trace(&trace, start, mins, maxs, end, ENTITYNUM_NONE, CONTENTS_SOLID);
	if ( trace.startsolid ) {
		start[2] += 128;
		trap_Trace(&trace, start, mins, maxs, end, ENTITYNUM_NONE, CONTENTS_SOLID);
	}
	if ( !trace.startsolid && trace.fraction < 1 ) G_SetOrigin(point, trace.endpos);
	point->nextthink = DomDeadline();
	point->think = DomScore;
	point->frameThink = DomCapture;
}

void SP_team_dom_point( gentity_t *point ) {
	if ( g_gametype.integer != GT_DOMINATION ) { G_FreeEntity(point); return; }
	if ( level.domPointCount >= ARRAY_LEN(level.domPoints) ) {
		G_Printf("Ignoring Domination point beyond the five-point limit\n");
		G_FreeEntity(point); return;
	}
	// Retail spawn 0x1004bb10: radius in groundEntityNum, identifier in
	// powerups, owner in modelindex2, and signed capture progress privately.
	point->r.svFlags |= SVF_BROADCAST;
	point->health = 0;
	point->s.groundEntityNum = point->count;
	point->s.loopSound = G_SoundIndex("sound/ambient/loop_elec_hum_01_short.wav");
	point->s.modelindex = G_ModelIndex("models/flag3/d_flag3.md3");
	point->s.modelindex2 = TEAM_FREE;
	G_SpawnInt("identifier", "0", &point->s.powerups);
	point->think = DomPlace;
	point->nextthink = level.time < INT_MAX ? level.time + 1 : INT_MAX;
	level.domPoints[level.domPointCount++] = point;
	trap_LinkEntity(point);
}

void G_RunDomination( void ) {
	int counts[TEAM_NUM_TEAMS] = { 0 }, i;
	if ( g_gametype.integer != GT_DOMINATION ) return;
	for ( i = 0; i < level.domPointCount; ++i ) {
		gentity_t *point = level.domPoints[i];
		if ( point->inuse && point->s.modelindex2 >= TEAM_RED && point->s.modelindex2 <= TEAM_BLUE ) counts[point->s.modelindex2]++;
	}
	trap_SetConfigstring(0x2bc, va("%d", counts[TEAM_RED]));
	trap_SetConfigstring(0x2bd, va("%d", counts[TEAM_BLUE]));
}

void G_DominationDefense( gentity_t *attacker, gentity_t *victim ) {
	int i;
	for ( i = 0; i < level.domPointCount; ++i ) {
		gentity_t *point = level.domPoints[i];
		vec3_t distance;
		if ( !point->inuse || point->s.modelindex2 != attacker->client->sess.sessionTeam ) continue;
		VectorSubtract(victim->r.currentOrigin, point->r.currentOrigin, distance);
		if ( VectorLength(distance) >= 500 || !trap_InPVS(point->r.currentOrigin, victim->r.currentOrigin) ) continue;
		attacker->client->pers.teamState.basedefense++;
		attacker->client->stats.defends++;
		AddScore(attacker, victim->r.currentOrigin, 10);
		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		attacker->client->ps.eFlags = (attacker->client->ps.eFlags & ~EF_AWARDS) | EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
}

// Retail 0x10038b60 associates point.target with deathmatch targetnames.
// The candidate pool is bounded to 25, including repeated map associations.
gentity_t *G_DominationSpawn( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t *spots[25], *point = NULL, *spot;
	int count = 0, eligible = 0, i, j;
	double minimum = (double)g_spawnMinDistance.integer * g_spawnMinDistance.integer;
	while ( (point = G_Find(point, FOFS(classname), "team_dom_point")) != NULL ) {
		if ( point->s.modelindex2 != player->client->sess.sessionTeam || !point->target ) continue;
		spot = NULL;
		while ( (spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL ) {
			if ( spot->targetname && !Q_stricmp(point->target, spot->targetname) && count < ARRAY_LEN(spots) ) spots[count++] = spot;
		}
	}
	for ( i = 0; i < count; ++i ) {
		float nearest = 100000000.0f;
		for ( j = 0; j < level.maxclients; ++j ) {
			gentity_t *other = &g_entities[j];
			vec3_t d;
			float distance;
			if ( !other->client || other->client->pers.connected == CON_DISCONNECTED || other->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || other->client->ps.stats[STAT_HEALTH] <= 0 ) continue;
			VectorSubtract(spots[i]->r.currentOrigin, other->r.currentOrigin, d);
			distance = (float)((double)d[1]*d[1] + (double)d[0]*d[0] + (double)d[2]*d[2]);
			if ( distance < nearest ) nearest = distance;
		}
		if ( nearest > minimum ) spots[eligible++] = spots[i];
	}
	if ( eligible <= 1 ) return NULL;
	i = (int)((double)(rand() & 0x7fff) / 32767.0 * eligible);
	if ( i >= eligible ) return NULL;
	spot = spots[i];
	VectorCopy(spot->s.origin, origin); origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
	return spot;
}
