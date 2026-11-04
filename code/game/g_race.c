/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "g_local.h"
#include <limits.h>

/* Retail 10063cd0/10063a60 supplies
 * the marker shape, model choice and floor placement; 10063ec0 supplies the
 * editor grammar. Keep all editor ownership in level state, not DLL statics. */
#define MAX_RACE_POINTS 64

/* Retail 100634f0 resets personal records on ClientBegin, not on death.
 * CS_RACE_RECORDS is the number of registered checkpoint entities, despite
 * its historical name. No remote leaderboard or disk record is invented. */
void G_BeginRace( gentity_t *ent ) {
	memset(ent->client->raceBestSplits, 0, sizeof(ent->client->raceBestSplits));
	ent->client->ps.persistant[PERS_SCORE] = INT_MAX;
	trap_SetConfigstring(CS_RACE_RECORDS, va("%i", level.racePointCount));
	trap_SendServerCommand(ent - g_entities, "race_init");
}

void G_RaceInfo( gentity_t *recipient, const gclient_t *runner ) {
	trap_SendServerCommand(recipient - g_entities, va("race_info %i %i %i %i %i %i",
		runner->raceActive, runner->raceStartTime, runner->raceLastTime, runner->raceSplitCount,
		runner->raceNext ? runner->raceNext->s.number : -1,
		runner->raceFollowing ? runner->raceFollowing->s.number : -1));
}

static gentity_t *RaceTarget( const gentity_t *point ) {
	gentity_t *next;
	if ( !point || !point->target || !point->target[0] ) return NULL;
	next = G_PickTarget(point->target);
	return next && next->inuse && next->classname && !strcmp(next->classname, "race_point") ? next : NULL;
}

/* Retail 10063580. The event payload uses integer entityState fields:
 * time, otherEntityNum/2, powerups and generic1 (not float aliases). */
static void RaceProgressEvent( gentity_t *runner, int event, int time, int hasBest ) {
	gclient_t *client = runner->client;
	gentity_t *notice = G_TempEntity(client->ps.origin, event);
	notice->r.svFlags = SVF_BROADCAST;
	notice->s.clientNum = runner - g_entities;
	notice->s.time = time;
	notice->s.generic1 = hasBest;
	if ( event != EV_RACE_FINISH ) {
		notice->s.otherEntityNum = client->raceSplitCount;
		notice->s.otherEntityNum2 = client->raceNext ? client->raceNext->s.number : -1;
		notice->s.powerups = client->raceFollowing ? client->raceFollowing->s.number : -1;
	}
}

static void RacePointTouch( gentity_t *point, gentity_t *runner, trace_t *trace ) {
	gclient_t *client = runner->client;
	int elapsed, i, previous;
	qboolean best;
	(void)trace;
	if ( !client || level.warmupTime > 0 ) return;
	if ( !point->targetname ) {
		if ( client->raceActive && client->flagCarryTime < 1000 ) return;
		client->raceNext = RaceTarget(point);
		if ( client->raceNext && client->raceNext->target ) client->raceFollowing = RaceTarget(client->raceNext);
		if ( !client->raceNext ) { G_ResetRace(client); return; }
		client->flagCarryTime = 0;
		memset(client->raceSplits, 0, sizeof(client->raceSplits));
		RaceProgressEvent(runner, EV_RACE_START, level.time, 0);
		client->raceSplitCount = 0;
		client->raceStartTime = level.time;
		client->raceActive = qtrue;
		return;
	}
	if ( !client->raceActive || client->raceNext != point ) return;
	elapsed = client->flagCarryTime;
	if ( elapsed < 0 ) { G_ResetRace(client); return; }
	if ( point->target ) {
		if ( client->raceSplitCount >= MAX_RACE_POINTS ) { G_ResetRace(client); return; }
		i = client->raceSplitCount++;
		previous = client->raceBestSplits[i];
		client->raceSplits[i] = elapsed;
		client->raceNext = RaceTarget(point);
		if ( client->raceNext && client->raceNext->target ) client->raceFollowing = RaceTarget(client->raceNext);
		if ( !client->raceNext ) { G_ResetRace(client); return; }
		RaceProgressEvent(runner, EV_RACE_CHECKPOINT, previous ? elapsed - previous : 0, previous != 0);
		return;
	}
	/* Retail avoids identical scoreboard times by walking the current players. */
	for ( i = 0; i < level.numPlayingClients; ++i ) {
		if ( elapsed == level.clients[level.sortedClients[i]].ps.persistant[PERS_SCORE] && elapsed < INT_MAX ) ++elapsed;
	}
	if ( elapsed == INT_MAX ) { G_ResetRace(client); return; }
	client->flagCarryTime = client->raceLastTime = elapsed;
	if ( level.numPlayingClients && elapsed < level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ) {
		gentity_t *notice = G_TempEntity(client->ps.origin, EV_NEW_HIGH_SCORE);
		notice->r.svFlags |= SVF_BROADCAST;
	}
	best = elapsed < client->ps.persistant[PERS_SCORE];
	if ( best ) {
		client->ps.persistant[PERS_SCORE] = elapsed;
		memcpy(client->raceBestSplits, client->raceSplits, sizeof(client->raceBestSplits));
	}
	RaceProgressEvent(runner, EV_RACE_FINISH, elapsed, 1);
	{
		/* Match retail 1002cc40's float rounding before formatting. */
		float seconds = (float)((double)elapsed / 1000.0);
		int minutes = (int)seconds / 60;
		float remainder = (float)((double)seconds - minutes * 60.0);
		char timeText[48];
		snprintf(timeText, sizeof(timeText), "%d:%s%.03f", minutes, remainder < 10 ? "0" : "", (double)remainder);
		trap_SendServerCommand(-1, va(best
			? "print \"^1Personal best! ^7%s^7 finished the race in in %s\n\""
			: "print \"%s^7 finished the race in %s\n\"", client->pers.netname, timeText));
	}
	client->raceNext = NULL;
	client->raceActive = qfalse;
	CalculateRanks();
}

void G_ResetRace( gclient_t *client ) {
	/* Retail 10063e50 preserves personal bests and resets only the active run
	 * and the two upcoming checkpoint references. */
	client->raceActive = qfalse;
	client->raceNext = NULL;
	client->raceFollowing = NULL;
}

static void RacePointSettle( gentity_t *point ) {
	vec3_t end, start;
	trace_t trace;
	if ( !(point->spawnflags & 1) ) {
		VectorCopy(point->s.origin, start);
		VectorCopy(start, end);
		end[2] -= 256;
		trap_Trace(&trace, start, point->r.mins, point->r.maxs, end, ENTITYNUM_NONE, CONTENTS_SOLID);
		if ( trace.startsolid ) {
			G_Printf("FinishSpawningItem: race_point startsolid at %s\n", vtos(start));
			start[2] += 128;
			trap_Trace(&trace, start, point->r.mins, point->r.maxs, end, ENTITYNUM_NONE, CONTENTS_SOLID);
		}
		if ( !trace.startsolid && trace.fraction < 1 ) {
			VectorCopy(trace.endpos, point->s.origin);
			point->s.groundEntityNum = trace.entityNum;
		}
	}
	point->s.origin[2] += 1;
	SnapVector(point->s.origin);
	G_SetOrigin(point, point->s.origin);
}

void SP_race_point( gentity_t *point ) {
	gentity_t *other = NULL;
	int count = 0;
	if ( g_gametype.integer != GT_RACE || (!point->target && !point->targetname) ) {
		G_FreeEntity(point);
		return;
	}
	while ( (other = G_Find(other, FOFS(classname), "race_point")) != NULL ) ++count;
	if ( count > MAX_RACE_POINTS || level.racePointCount >= MAX_RACE_POINTS ) { G_FreeEntity(point); return; }
	++level.racePointCount;
	point->r.svFlags |= SVF_BROADCAST;
	VectorSet(point->r.mins, -40, -40, -15);
	VectorSet(point->r.maxs, 40, 40, 128);
	point->r.contents = CONTENTS_TRIGGER;
	point->s.modelindex2 = point->targetname && !point->target;
	point->s.modelindex = G_ModelIndex(!point->targetname ? "models/flag3/g_flag3.md3"
		: point->target ? "models/flag3/d_flag3.md3" : "models/flag3/f_flag3.md3");
	point->s.generic1 = point->spawnflags;
	point->touch = RacePointTouch;
	point->think = RacePointSettle;
	point->nextthink = level.time + 1;
	trap_LinkEntity(point);
}

void Cmd_RacePoint_f( gentity_t *ent ) {
	char action[MAX_TOKEN_CHARS];
	gentity_t *point = NULL;
	int count = 0, i;
	vec3_t origin;
	if ( g_gametype.integer != GT_RACE ) {
		trap_SendServerCommand(ent - g_entities, "print \"RacePoint is only permitted in the Race gametype.\n\"");
		return;
	}
	if ( !g_cheats.integer ) {
		trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be alive to use this command.\n\"");
		return;
	}
	trap_Argv(1, action, sizeof(action));
	if ( !Q_stricmp(action, "clear") ) {
		trap_SendServerCommand(-1, "print \"clearing race points\n\"");
		while ( (point = G_Find(point, FOFS(classname), "race_point")) != NULL ) G_FreeEntity(point);
		level.raceEditorPrevious = NULL;
		level.raceEditorCount = 0;
		for ( i = 0; i < level.maxclients; ++i ) G_ResetRace(&level.clients[i]);
		return;
	}
	if ( !Q_stricmp(action, "dump") ) {
		while ( (point = G_Find(point, FOFS(classname), "race_point")) != NULL )
			trap_SendServerCommand(-1, va("print \"%f %f %f\n\"", point->r.currentOrigin[0], point->r.currentOrigin[1], point->r.currentOrigin[2]));
		return;
	}
	while ( (point = G_Find(point, FOFS(classname), "race_point")) != NULL ) ++count;
	if ( count >= MAX_RACE_POINTS || level.raceEditorCount >= MAX_RACE_POINTS || level.racePointCount >= MAX_RACE_POINTS ) {
		trap_SendServerCommand(-1, "print \"too many race points\n\"");
		return;
	}
	point = G_Spawn();
	point->classname = "race_point";
	point->s.eType = ET_GENERAL;
	VectorCopy(ent->client->ps.origin, origin);
	origin[2] -= 8;
	G_SetOrigin(point, origin);
	point->targetname = level.raceEditorNames[level.raceEditorCount];
	Com_sprintf(level.raceEditorNames[level.raceEditorCount], sizeof(level.raceEditorNames[0]), "arp%i", level.raceEditorCount);
	++level.raceEditorCount;
	if ( level.raceEditorPrevious && level.raceEditorPrevious->inuse
		&& !Q_stricmp(level.raceEditorPrevious->classname, "race_point") )
		level.raceEditorPrevious->target = point->targetname;
	SP_race_point(point);
	level.raceEditorPrevious = point;
	trap_SendServerCommand(-1, "print \"spawning a race point\n\"");
}
