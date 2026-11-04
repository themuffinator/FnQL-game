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

// Retail 0x1004d650 reserves the chosen spawn entity number until pickup.
// This is map-local state, independent of the hidden item left by a pickup.
static int runeSpots[PP_ARMORREGEN + 1];
static int spawnCounts[TEAM_BLUE + 1];
static const char *const spawnClasses[] = {
	"info_player_deathmatch", "team_CTF_redspawn", "team_CTF_bluespawn"
};

static qboolean G_RuneSpotReserved( int number ) {
	int rune;
	for ( rune = PP_SCOUT; rune <= PP_ARMORREGEN; rune++ ) {
		if ( runeSpots[rune] == number ) {
			return qtrue;
		}
	}
	return qfalse;
}

static gentity_t *G_SpawnRune( int rune, team_t team ) {
	gentity_t *candidates[16], *spot = NULL, *itemEnt;
	gitem_t *item;
	int count = 0;

	if ( rune < PP_SCOUT || rune > PP_ARMORREGEN ) {
		return NULL;
	}
	// Retail falls back to deathmatch spawns when the requested team has
	// fewer than five. Bound the search even on malformed/incomplete maps.
	if ( team < TEAM_FREE || team > TEAM_BLUE || spawnCounts[team] < 5 ) {
		team = TEAM_FREE;
	}
	if ( spawnCounts[team] >= 5 ) {
		while ( count < ARRAY_LEN(candidates)
			&& ( spot = G_Find( spot, FOFS(classname), spawnClasses[team] ) ) != NULL ) {
			if ( !G_RuneSpotReserved( spot->s.number ) ) {
				candidates[count++] = spot;
			}
		}
	}
	if ( !count ) {
		G_Printf( "WARNING: no available spawn point for rune %d\n", rune );
		return NULL;
	}
	item = BG_FindItemForPersistantPowerup( rune );
	if ( !item ) {
		G_Printf( "WARNING: missing item for rune %d\n", rune );
		return NULL;
	}
	spot = candidates[rand() % count];
	runeSpots[rune] = spot->s.number;
	itemEnt = G_Spawn();
	itemEnt->s.eType = ET_ITEM;
	itemEnt->s.modelindex = item - bg_itemlist;
	itemEnt->s.modelindex2 = 1;
	itemEnt->classname = item->classname;
	itemEnt->item = item;
	VectorSet( itemEnt->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( itemEnt->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
	itemEnt->r.contents = CONTENTS_TRIGGER;
	itemEnt->touch = Touch_Item;
	G_SetOrigin( itemEnt, spot->s.origin );
	itemEnt->s.pos.trType = TR_GRAVITY;
	itemEnt->s.pos.trTime = level.time;
	itemEnt->s.eFlags |= EF_BOUNCE_HALF;
	// Initial runes have neither a dropped-item flag nor an expiry callback.
	// A later LaunchItem drop owns its relocation timer (0x1004f6ce).
	trap_LinkEntity( itemEnt );
	return itemEnt;
}

void G_InitRunes( void ) {
	gentity_t *spot;
	team_t team;
	int rune, group;
	qboolean split;
	memset( runeSpots, 0, sizeof(runeSpots) );
	memset( spawnCounts, 0, sizeof(spawnCounts) );
	for ( group = TEAM_FREE; group <= TEAM_BLUE; group++ ) {
		for ( spot = G_Find( NULL, FOFS(classname), spawnClasses[group] ); spot;
			spot = G_Find( spot, FOFS(classname), spawnClasses[group] ) ) {
			spawnCounts[group]++;
		}
	}
	// Retail 0x1004d950: no initial runes in Race or the intermission.
	if ( !g_runes.integer || g_gametype.integer == GT_RACE
		|| level.intermissionQueued || level.intermissiontime ) {
		return;
	}
	split = spawnCounts[TEAM_RED] >= 5 && spawnCounts[TEAM_BLUE] >= 5;
	if ( !split && spawnCounts[TEAM_FREE] < 5 ) {
		G_Printf( "WARNING: not enough available spawn points for spawned persistant powerups\n" );
		return;
	}
	team = split ? ( rand() & 1 ) + TEAM_RED : TEAM_FREE;
	for ( rune = PP_SCOUT; rune <= PP_ARMORREGEN; rune++ ) {
		if ( split ) {
			team = team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
		}
		G_SpawnRune( rune, team );
	}
}

void G_RunePickedUp( int rune ) {
	if ( rune >= PP_SCOUT && rune <= PP_ARMORREGEN ) {
		runeSpots[rune] = 0; // retail 0x1004e4e5
	}
}

void G_RespawnRune( gentity_t *ent ) {
	int rune;
	team_t team = TEAM_FREE;
	if ( !ent || !ent->item || ent->item->giType != IT_PERSISTANT_POWERUP ) {
		return;
	}
	rune = ent->item->giTag;
	// Retail 0x1004da10 chooses a fresh side for each relocation, before
	// freeing the old entity; it need not stay on the side of its first spawn.
	if ( spawnCounts[TEAM_RED] >= 5 && spawnCounts[TEAM_BLUE] >= 5 ) {
		team = ( rand() & 1 ) + TEAM_RED;
	}
	G_FreeEntity( ent );
	G_SpawnRune( rune, team );
}
