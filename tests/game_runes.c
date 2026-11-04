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
vmCvar_t g_runes, g_gametype;
gentity_t g_entities[MAX_GENTITIES];
gitem_t bg_itemlist[5] = {
	{ NULL },
	{ "test_scout", NULL, { NULL }, NULL, NULL, 0, IT_PERSISTANT_POWERUP, PP_SCOUT },
	{ "test_guard", NULL, { NULL }, NULL, NULL, 0, IT_PERSISTANT_POWERUP, PP_GUARD },
	{ "test_damage", NULL, { NULL }, NULL, NULL, 0, IT_PERSISTANT_POWERUP, PP_DOUBLER },
	{ "test_regen", NULL, { NULL }, NULL, NULL, 0, IT_PERSISTANT_POWERUP, PP_ARMORREGEN }
};
static int spotCount, itemCount, links, frees, warnings, randomValue, randomCalls;
static int missingRune;

int rand( void ) { randomCalls++; return randomValue; }

gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match ) {
	int i = from ? (int)(from - g_entities) + 1 : 1;
	CHECK( fieldofs == FOFS(classname) );
	for ( ; i <= spotCount; i++ ) {
		if ( g_entities[i].inuse && !strcmp(g_entities[i].classname, match) ) {
			return &g_entities[i];
		}
	}
	return NULL;
}

gentity_t *G_Spawn( void ) {
	gentity_t *ent;
	CHECK( itemCount < 100 );
	ent = &g_entities[200 + itemCount++];
	ent->inuse = qtrue;
	return ent;
}

void G_FreeEntity( gentity_t *ent ) { CHECK(ent->inuse); ent->inuse = qfalse; frees++; }
void G_SetOrigin( gentity_t *ent, vec3_t origin ) {
	VectorCopy(origin, ent->s.pos.trBase);
	VectorCopy(origin, ent->s.origin);
	VectorCopy(origin, ent->r.currentOrigin);
}
void trap_LinkEntity( gentity_t *ent ) { CHECK(ent->inuse); links++; }
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace ) { CHECK(0); }
void QDECL G_Printf( const char *format, ... ) { CHECK(strstr(format, "WARNING:")); warnings++; }
gitem_t *BG_FindItemForPersistantPowerup( persistantPowerup_t rune ) {
	CHECK(rune >= PP_SCOUT && rune <= PP_ARMORREGEN);
	return rune == missingRune ? NULL : &bg_itemlist[rune];
}

static void Reset( void ) {
	memset(&level, 0, sizeof(level));
	memset(g_entities, 0, sizeof(g_entities));
	spotCount = itemCount = links = frees = warnings = randomCalls = missingRune = 0;
	randomValue = 0;
	g_runes.integer = 1;
	g_gametype.integer = GT_TEAM;
	level.time = 1250;
}

static void Spots( const char *classname, int count ) {
	while ( count-- ) {
		gentity_t *ent = &g_entities[++spotCount];
		CHECK(spotCount < 100);
		ent->inuse = qtrue;
		ent->classname = (char *)classname;
		ent->s.number = spotCount;
		VectorSet(ent->s.origin, spotCount, 0, 80);
	}
}

static void CheckSpawn( int index, int rune, int spot ) {
	gentity_t *ent = &g_entities[200 + index];
	CHECK(ent->inuse && ent->item == &bg_itemlist[rune]);
	CHECK(ent->s.eType == ET_ITEM && ent->s.modelindex == rune && ent->s.modelindex2 == 1);
	CHECK(ent->s.pos.trType == TR_GRAVITY && ent->s.pos.trTime == level.time);
	CHECK(ent->s.pos.trBase[0] == spot && ent->s.pos.trBase[2] == 80);
	CHECK(ent->s.pos.trDelta[0] == 0 && ent->s.pos.trDelta[2] == 0);
	CHECK(ent->r.mins[0] == -ITEM_RADIUS && ent->r.maxs[2] == ITEM_RADIUS);
	CHECK(ent->r.contents == CONTENTS_TRIGGER && ent->touch == Touch_Item);
	CHECK(ent->s.eFlags & EF_BOUNCE_HALF);
	CHECK(!(ent->flags & FL_DROPPED_ITEM) && !ent->think && !ent->nextthink);
}

int main( void ) {
	int i;
	// Disabled states must not allocate entities or consume the RNG stream.
	for ( i = 0; i < 4; i++ ) {
		Reset(); Spots("info_player_deathmatch", 5);
		if (i == 0) g_runes.integer = 0;
		if (i == 1) g_gametype.integer = GT_RACE;
		if (i == 2) level.intermissionQueued = 1;
		if (i == 3) level.intermissiontime = 1;
		G_InitRunes(); CHECK(!itemCount && !randomCalls && !warnings);
	}
	Reset(); Spots("info_player_deathmatch", 4);
	G_InitRunes(); CHECK(!itemCount && !randomCalls && warnings == 1);
	Reset(); Spots("info_player_deathmatch", 5);
	G_InitRunes(); CHECK(itemCount == 4 && links == 4 && randomCalls == 4);
	for (i = 0; i < 4; i++) CheckSpawn(i, i + 1, i + 1);
	// Unpicked runes reserve their spots. Relocation must not overlap them.
	G_RespawnRune(&g_entities[200]);
	CHECK(frees == 1 && !g_entities[200].inuse);
	CheckSpawn(4, PP_SCOUT, 5);
	// Pickup releases the reservation, even while other runes remain out.
	G_RunePickedUp(PP_SCOUT);
	G_RunePickedUp(-1); G_RunePickedUp(99);
	G_RespawnRune(&g_entities[204]); CheckSpawn(5, PP_SCOUT, 1);
	G_RespawnRune(NULL); G_RespawnRune(&g_entities[1]); CHECK(frees == 2);
	// Split maps distribute two runes per side, with no deathmatch spots.
	Reset(); Spots("team_CTF_redspawn", 5); Spots("team_CTF_bluespawn", 5);
	G_InitRunes(); CHECK(itemCount == 4 && randomCalls == 5);
	CheckSpawn(0, PP_SCOUT, 6); CheckSpawn(1, PP_GUARD, 1);
	CheckSpawn(2, PP_DOUBLER, 7); CheckSpawn(3, PP_ARMORREGEN, 2);
	G_RunePickedUp(PP_SCOUT); G_RespawnRune(&g_entities[200]);
	CheckSpawn(4, PP_SCOUT, 3); // relocation chooses a new side
	// A short team group falls back to deathmatch spawns for all four.
	Reset(); Spots("team_CTF_redspawn", 5); Spots("team_CTF_bluespawn", 4);
	Spots("info_player_deathmatch", 5); G_InitRunes();
	for (i = 0; i < 4; i++) CheckSpawn(i, i + 1, i + 10);
	// Candidate selection is capped at sixteen available spots per rune.
	Reset(); Spots("info_player_deathmatch", 24); randomValue = 15;
	G_InitRunes();
	for (i = 0; i < 4; i++) CheckSpawn(i, i + 1, i + 16);
	// Missing item definitions and vanished map entities fail without a loop.
	Reset(); Spots("info_player_deathmatch", 5); missingRune = PP_SCOUT;
	G_InitRunes(); CHECK(itemCount == 3 && warnings == 1);
	for (i = 1; i <= spotCount; i++) g_entities[i].inuse = qfalse;
	G_RespawnRune(&g_entities[200]); CHECK(itemCount == 3 && warnings == 2 && frees == 1);
	puts("game runes OK: disabled modes, spawn limits, reservations, team distribution, relocation, malformed maps");
	return 0;
}
