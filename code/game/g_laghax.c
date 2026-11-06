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

// Retail 0x10051660..0x10051add: short, server-frame collision history.
// Indexed storage has a fixed, separate budget so operator settings cannot
// overflow an allocation or exhaust the map-entity string pool.
#define MAX_LAGHAX_SAMPLES 256
typedef struct {
	vec3_t origin, mins, maxs;
	int time;
} rewindSample_t;
static rewindSample_t storage[MAX_CLIENTS * MAX_LAGHAX_SAMPLES], *samples;
static rewindSample_t saved[MAX_CLIENTS];
static int heads[MAX_CLIENTS], sampleCount;
static qboolean restore[MAX_CLIENTS];

void G_InitLagHax( void ) {
	int count = g_lagHaxHistory.integer;
	samples = NULL;
	sampleCount = 0;
	memset(heads, 0, sizeof(heads));
	memset(restore, 0, sizeof(restore));
	if ( count <= 0 || g_lagHaxMs.integer <= 0 ) return;
	if ( count > MAX_LAGHAX_SAMPLES ) {
		G_Printf("g_lagHaxHistory %d exceeds safe history capacity; using %d\n", count, MAX_LAGHAX_SAMPLES);
		count = MAX_LAGHAX_SAMPLES;
	}
	samples = storage;
	memset(samples, 0, MAX_CLIENTS * count * sizeof(*samples));
	sampleCount = count;
}

void G_ResetLagHax( gentity_t *ent ) {
	int slot = ent - g_entities;
	if ( slot < 0 || slot >= MAX_CLIENTS ) return;
	if ( samples ) samples[slot * sampleCount + heads[slot]].time = 0;
	restore[slot] = qfalse;
}

void G_StoreLagHax( gentity_t *ent ) {
	int slot = ent - g_entities;
	rewindSample_t *sample;
	if ( !samples || slot < 0 || slot >= MAX_CLIENTS ) return;
	heads[slot] = (heads[slot] + 1) % sampleCount;
	sample = &samples[slot * sampleCount + heads[slot]];
	sample->time = ent->s.eType == ET_PLAYER ? level.time : 0;
	VectorCopy(ent->s.pos.trBase, sample->origin);
	VectorCopy(ent->r.mins, sample->mins);
	VectorCopy(ent->r.maxs, sample->maxs);
}

static void RewindClient( int slot, int time ) {
	gentity_t *ent = &g_entities[slot];
	const rewindSample_t *history = &samples[slot * sampleCount];
	const rewindSample_t *older = &history[heads[slot]], *newer = older;
	int index = heads[slot], visited, axis;
	if ( time > older->time ) return;
	for ( visited = 1; time < older->time && visited < sampleCount; ++visited ) {
		const rewindSample_t *previous;
		index = (index + sampleCount - 1) % sampleCount;
		previous = &history[index];
		if ( previous->time >= older->time ) break;
		newer = older;
		older = previous;
	}
	if ( !older->time ) return; // a spawn/teleport breaks history continuity
	if ( time < older->time ) newer = older; // oldest available sample
	trap_UnlinkEntity(ent);
	for ( axis = 0; axis < 3; ++axis ) {
		if ( older == newer ) ent->r.currentOrigin[axis] = older->origin[axis];
		else {
			float delta = older->origin[axis] - newer->origin[axis];
			float elapsed = (float)((long long)time - older->time);
			ent->r.currentOrigin[axis] = (float)(older->origin[axis]
				+ (double)elapsed / ((long long)older->time - newer->time) * delta);
		}
	}
	// Retail interpolates the origin, but uses the newer stance's bounds.
	VectorCopy(newer->mins, ent->r.mins);
	VectorCopy(newer->maxs, ent->r.maxs);
	trap_LinkEntity(ent);
}

void G_BeginLagHax( gentity_t *shooter ) {
	int slot, time, weapon = shooter->s.weapon;
	long long earliest;
	memset(restore, 0, sizeof(restore));
	if ( g_unlagged.integer != 1 || !samples || g_lagHaxMs.integer <= 0
		|| !shooter->client || (shooter->r.svFlags & SVF_BOT)
		|| weapon < 0 || weapon >= 15 || !( (1u << weapon) & 0x60ccu ) ) return;
	// Retail 0x1006f3ae uses the completed pmove's command time, not the
	// incoming command timestamp or a wall-clock offset within the frame.
	time = shooter->client->ps.commandTime;
	earliest = (long long)level.time - g_lagHaxMs.integer;
	if ( time < earliest ) time = earliest < INT_MIN ? INT_MIN : (int)earliest;
	for ( slot = 0; slot < level.maxclients && slot < MAX_CLIENTS; ++slot ) {
		gentity_t *ent = &g_entities[slot];
		if ( ent == shooter || !ent->inuse || !ent->client
			|| ent->client->pers.connected != CON_CONNECTED || ent->client->ps.pm_type == PM_SPECTATOR ) continue;
		VectorCopy(ent->r.currentOrigin, saved[slot].origin);
		VectorCopy(ent->r.mins, saved[slot].mins);
		VectorCopy(ent->r.maxs, saved[slot].maxs);
		restore[slot] = qtrue;
		RewindClient(slot, time);
	}
}

void G_EndLagHax( void ) {
	int slot;
	for ( slot = 0; slot < MAX_CLIENTS; ++slot ) {
		gentity_t *ent = &g_entities[slot];
		if ( !restore[slot] ) continue;
		restore[slot] = qfalse;
		VectorCopy(saved[slot].origin, ent->r.currentOrigin);
		VectorCopy(saved[slot].mins, ent->r.mins);
		VectorCopy(saved[slot].maxs, ent->r.maxs);
		trap_LinkEntity(ent);
	}
}
