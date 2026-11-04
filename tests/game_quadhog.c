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
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype, g_quadHog, g_quadHogIdle, g_quadHogPingRate;
vec3_t vec3_origin;
static gclient_t clients[2];
static gitem_t quadItem = { "item_quad" };
static gentity_t launched, event;
static int launches, events, announcements, touches, randomCalls, randomValue;

int rand( void ) { randomCalls++; return randomValue; }
char *QDECL va( const char *format, ... ) {
	static char buffer[1024];
	va_list ap;
	va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap);
	return buffer;
}
gentity_t *G_Find( gentity_t *from, int field, const char *name ) {
	int i = from ? (int)(from - g_entities) + 1 : 0;
	CHECK(field == FOFS(classname));
	for ( ; i < level.num_entities; i++ )
		if ( g_entities[i].inuse && g_entities[i].classname && !strcmp(g_entities[i].classname, name) ) return &g_entities[i];
	return NULL;
}
gitem_t *BG_FindItemForPowerup( powerup_t powerup ) { CHECK(powerup == PW_QUAD); return &quadItem; }
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	memset(&launched, 0, sizeof(launched));
	CHECK(item == &quadItem);
	VectorCopy(origin, launched.s.origin);
	VectorCopy(velocity, launched.s.pos.trDelta);
	launches++;
	return &launched;
}
gentity_t *G_TempEntity( vec3_t origin, int type ) {
	memset(&event, 0, sizeof(event));
	event.s.event = type;
	VectorCopy(origin, event.s.origin);
	events++;
	return &event;
}
int G_SoundIndex( const char *name ) { CHECK(!strcmp(name, "sound/items/poweruprespawn.wav")); return 7; }
void trap_SendServerCommand( int client, const char *text ) {
	CHECK(client == -1);
	CHECK(!strcmp(text, "cp \" ^5Quad ^7respawned!\"") || !strcmp(text, "cp \"Carrier is the ^5Quad Hog!\""));
	announcements++;
}
void Touch_Item( gentity_t *item, gentity_t *player, trace_t *trace ) {
	CHECK(item == &launched && player == &g_entities[0]);
	CHECK(announcements > 0);
	touches++;
}

static void Reset( void ) {
	memset(&level, 0, sizeof(level));
	memset(g_entities, 0, sizeof(g_entities));
	memset(clients, 0, sizeof(clients));
	g_gametype.integer = GT_FFA;
	g_quadHog.integer = 1;
	g_quadHogIdle.integer = 30;
	g_quadHogPingRate.integer = 1500;
	level.time = 1250;
	level.maxclients = 2;
	level.num_entities = 6;
	g_entities[0].client = &clients[0];
	g_entities[1].client = &clients[1];
	strcpy(clients[0].pers.netname, "Carrier");
	launches = events = announcements = touches = randomCalls = randomValue = 0;
}

int main( void ) {
	int i;
	Reset();
	g_quadHog.integer = 0; G_CheckQuadHog();
	g_quadHog.integer = 1; g_gametype.integer = GT_TEAM; G_CheckQuadHog();
	g_gametype.integer = GT_FFA; level.intermissionQueued = 1; G_CheckQuadHog();
	level.intermissionQueued = 0; level.intermissiontime = 1; G_CheckQuadHog();
	CHECK(!launches && !randomCalls);
	Reset();
	// A malformed map without a spawn must terminate without allocating.
	G_CheckQuadHog(); CHECK(!launches && randomCalls == 1);
	for ( i = 2; i < 5; i++ ) {
		g_entities[i].inuse = qtrue;
		g_entities[i].classname = "info_player_deathmatch";
		g_entities[i].s.origin[0] = (float)i;
	}
	// Both ends of the retail random range, including walking past NULL.
	G_CheckQuadHog(); CHECK(launches == 1 && launched.s.origin[0] == 2);
	randomValue = 32767; G_CheckQuadHog();
	CHECK(launches == 2 && launched.s.origin[0] == 2 && launched.nextthink == 31250);
	CHECK(event.s.event == EV_GLOBAL_SOUND && event.s.eventParm == 7 && (event.r.svFlags & SVF_BROADCAST));
	clients[1].ps.powerups[PW_QUAD] = 1; G_CheckQuadHog(); CHECK(launches == 2);
	clients[1].ps.powerups[PW_QUAD] = 0;
	g_entities[5].inuse = qtrue; g_entities[5].classname = "item_quad";
	g_entities[5].r.linked = qtrue; G_CheckQuadHog(); CHECK(launches == 2);
	g_entities[5].r.linked = qfalse; G_CheckQuadHog(); CHECK(launches == 3);
	launched.touch(&launched, &g_entities[5], NULL); CHECK(!touches);
	launched.touch(&launched, &g_entities[0], NULL); CHECK(touches == 1);
	g_quadHogIdle.integer = INT_MAX; G_LaunchQuadHog(&quadItem, vec3_origin, vec3_origin); CHECK(launched.nextthink == INT_MAX);
	g_quadHogIdle.integer = INT_MIN; G_LaunchQuadHog(&quadItem, vec3_origin, vec3_origin); CHECK(launched.nextthink == 1);
	Reset();
	G_QuadHogTimer(&g_entities[0], 1500); CHECK(!events);
	clients[0].ps.powerups[PW_QUAD] = 20000;
	G_QuadHogTimer(&g_entities[0], 1499); CHECK(!events);
	G_QuadHogTimer(&g_entities[0], 1);
	CHECK(events == 1 && event.s.event == EV_POI && event.s.powerups == 1500);
	CHECK(event.s.generic1 == 0 && (event.r.svFlags & SVF_BROADCAST));
	CHECK(clients[0].quadHogPingResidual == 0);
	clients[0].quadHogPingResidual = INT_MAX - 1;
	g_quadHogPingRate.integer = INT_MAX;
	G_QuadHogTimer(&g_entities[0], 50); CHECK(events == 2);
	puts("Quad Hog ownership, malformed-map bounds, lifetime and locator checks passed");
	return 0;
}
