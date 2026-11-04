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

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
static gentity_t events[8], *destination;
static int nearby[MAX_GENTITIES], nearbyCount, eventCount, soundCount, warnings, captures;
static vec3_t forward, queryOrigin;
static const char *sounds[8];

int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int capacity ) {
	int i;
	CHECK( capacity == MAX_GENTITIES );
	for ( i = 0; i < 3; i++ ) {
		CHECK( mins[i] == queryOrigin[i] - 15 && maxs[i] == queryOrigin[i] + 15 );
	}
	memcpy( list, nearby, nearbyCount * sizeof(*list) );
	return nearbyCount;
}

gentity_t *G_TempEntity( vec3_t origin, int event ) {
	gentity_t *result;
	CHECK( event == EV_GENERAL_SOUND && eventCount < ARRAY_LEN(events) );
	result = &events[eventCount++];
	VectorCopy( origin, result->s.origin );
	return result;
}

int G_SoundIndex( const char *sound ) {
	CHECK( soundCount < ARRAY_LEN(sounds) );
	sounds[soundCount++] = sound;
	return soundCount;
}

gentity_t *G_PickTarget( const char *target ) {
	CHECK( target && !strcmp(target, "exit") );
	return destination;
}

void QDECL G_Printf( const char *format, ... ) {
	CHECK( !strcmp(format, "Couldn't find teleporter destination\n") );
	warnings++;
}

void PM_AngleVectors( const vec3_t angles, vec3_t result, vec3_t right, vec3_t up ) {
	CHECK( destination && !memcmp(angles, destination->s.angles, sizeof(vec3_t)) );
	CHECK( !right && !up );
	VectorCopy( forward, result );
}

static void Capture( gentity_t *zone, gentity_t *flag, trace_t *trace ) {
	CHECK( zone == &g_entities[4] && flag == &g_entities[0] && !trace );
	captures++;
	flag->inuse = qfalse;
}

static void Reset( void ) {
	memset( &level, 0, sizeof(level) );
	memset( g_entities, 0, sizeof(g_entities) );
	memset( events, 0, sizeof(events) );
	memset( nearby, 0, sizeof(nearby) );
	nearbyCount = eventCount = soundCount = warnings = captures = 0;
	level.time = 2500;
	level.msec = 25;
	g_entities[0].inuse = qtrue;
	VectorSet( g_entities[0].r.currentOrigin, 100, -50, 25 );
	VectorCopy( g_entities[0].r.currentOrigin, queryOrigin );
	g_entities[1].inuse = qtrue;
	g_entities[1].classname = "trigger_push";
	VectorSet( g_entities[1].s.origin2, 15, -35, 450 );
	g_entities[2].inuse = qtrue;
	g_entities[2].classname = "trigger_teleport";
	g_entities[2].target = "exit";
	destination = &g_entities[3];
	VectorSet( destination->s.origin, -500, 120, 70 );
	VectorSet( destination->s.angles, 17, 123, 0 );
	VectorSet( forward, -0.3333f, 0.6666f, -0.1234f );
	g_entities[4].inuse = qtrue;
	g_entities[4].classname = "trigger_capturezone";
	g_entities[4].touch = Capture;
}

int main( void ) {
	gentity_t *flag = &g_entities[0];
	Reset(); nearby[nearbyCount++] = 1; G_RunFlagPhysics( flag );
	CHECK( !memcmp(flag->s.pos.trBase, queryOrigin, sizeof(vec3_t)) );
	CHECK( !memcmp(flag->s.pos.trDelta, g_entities[1].s.origin2, sizeof(vec3_t)) );
	CHECK( flag->s.pos.trTime == 2475 && flag->fly_sound_debounce_time == 4000 );
	CHECK( eventCount == 1 && events[0].s.eventParm == 1 && !strcmp(sounds[0], "sound/world/jumppad.wav") );
	level.time = 4000; g_entities[1].s.origin2[2] = 600; G_RunFlagPhysics( flag );
	CHECK( eventCount == 1 && flag->s.pos.trTime == 3975 && flag->s.pos.trDelta[2] == 600 );
	level.time++; G_RunFlagPhysics( flag ); CHECK( eventCount == 2 );

	Reset(); nearby[nearbyCount++] = 2; G_RunFlagPhysics( flag );
	CHECK( !memcmp(flag->s.pos.trBase, destination->s.origin, sizeof(vec3_t)) );
	CHECK( !memcmp(flag->r.currentOrigin, destination->s.origin, sizeof(vec3_t)) );
	CHECK( flag->s.pos.trDelta[0] == -133 && flag->s.pos.trDelta[1] == 266 && flag->s.pos.trDelta[2] == 46 );
	CHECK( flag->s.pos.trTime == 2475 && eventCount == 2 && !warnings );
	CHECK( !memcmp(events[0].s.origin, queryOrigin, sizeof(vec3_t)) );
	CHECK( !memcmp(events[1].s.origin, destination->s.origin, sizeof(vec3_t)) );
	CHECK( !strcmp(sounds[0], "sound/world/teleout.ogg") && !strcmp(sounds[1], "sound/world/telein.ogg") );

	// Missing destinations stop this frame, without modifying the flag or
	// applying a later overlapping pad. A capture may free the flag likewise.
	Reset(); destination = NULL; nearby[nearbyCount++] = 2; nearby[nearbyCount++] = 1;
	G_RunFlagPhysics( flag ); CHECK( warnings == 1 && !eventCount && !flag->s.pos.trTime );
	Reset(); nearby[nearbyCount++] = 4; nearby[nearbyCount++] = 1;
	G_RunFlagPhysics( flag ); CHECK( captures == 1 && !eventCount && !flag->inuse );

	// Bad indices, unrelated, unnamed and unlinked entities, self touches,
	// and a capture zone without a callback are harmless.
	Reset(); nearby[nearbyCount++] = -1; nearby[nearbyCount++] = MAX_GENTITIES;
	nearby[nearbyCount++] = 0; nearby[nearbyCount++] = 1; nearby[nearbyCount++] = 2;
	nearby[nearbyCount++] = 3; nearby[nearbyCount++] = 4; nearby[nearbyCount++] = 5;
	g_entities[1].inuse = qfalse; g_entities[2].classname = "Trigger_Teleport";
	g_entities[3].inuse = qtrue; g_entities[4].touch = NULL;
	g_entities[5].inuse = qtrue; g_entities[5].classname = "trigger_multiple";
	G_RunFlagPhysics( flag ); CHECK( !eventCount && !captures && !warnings );
	puts( "flag physics: passed" );
	return 0;
}
