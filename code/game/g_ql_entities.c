/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
//
// g_ql_entities.c -- map entities that exist only in Quake Live.
//
// These are the eight classnames of the retail qagame spawn table that Quake
// III lacks (retail table at qagamex86 0x10080560).  Each spawn function
// follows the retail lifecycle: entities that belong to one gametype free
// themselves in every other gametype, exactly as the retail ones do, so no
// stray entity ever reaches a client.  Behavior that belongs to a later
// milestone is called out where it applies.
//

#include "g_local.h"

/*QUAKED advertisement (0 .5 .8) ?
Quake Live billboard.  The engine renders and rotates the ad surfaces from
the BSP; the game entity only validates its cell and goes away.
"cellId"	advertisement cell number
*/
void SP_advertisement( gentity_t *ent ) {
	int cellId;

	G_SpawnInt( "cellId", "0", &cellId );
	if ( !cellId && !trap_Cvar_VariableIntegerValue( "com_build" ) ) {
		G_Printf( "advertisement entity with no cellId at %s\n", vtos( ent->s.origin ) );
	}
	G_FreeEntity( ent );
}

/*QUAKED info_tour_point (1 0 1) (-8 -8 -8) (8 8 8)
Training map tour stop.  Only the training map uses it; elsewhere it is
removed as retail does.
*/
void SP_info_tour_point( gentity_t *ent ) {
	// The guided tour (worldspawn "trainingMap") is not implemented; retail
	// removes these entities outside the tour as well.
	G_FreeEntity( ent );
}

/*
================
Use_Target_Cvar

Sets the named cvar to the target's value.
================
*/
static void Use_Target_Cvar( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	(void)other;
	(void)activator;

	if ( ent->cvarName && ent->cvarName[0] ) {
		trap_Cvar_Set( ent->cvarName, va( "%f", ent->cvarValue ) );
	}
}

/*QUAKED target_cvar (1 0 0) (-8 -8 -8) (8 8 8)
Sets a cvar when used.
"cvar"		cvar name
"cvarValue"	value to set (default -1)
*/
void SP_target_cvar( gentity_t *ent ) {
	char *name;

	G_SpawnString( "cvar", "", &name );
	ent->cvarName = G_NewString( name );
	G_SpawnFloat( "cvarValue", "-1.0f", &ent->cvarValue );
	ent->use = Use_Target_Cvar;
}

/*QUAKED team_dom_point (0 .5 .8) (-16 -16 -16) (16 16 16)
Domination control point.  Removed in every other gametype.
"identifier"	point number
*/
void SP_team_dom_point( gentity_t *ent ) {
	if ( g_gametype.integer != GT_DOMINATION ) {
		G_FreeEntity( ent );
		return;
	}
	// Domination control points arrive with the round-based and special
	// modes (docs/PLAN.md milestone 6).
	G_Printf( "team_dom_point: Domination is not implemented yet\n" );
	G_FreeEntity( ent );
}

/*QUAKED race_point (0 .5 .8) (-16 -16 -16) (16 16 16) initial
Race checkpoint.  Removed in every other gametype.
*/
void SP_race_point( gentity_t *ent ) {
	if ( g_gametype.integer != GT_RACE ) {
		G_FreeEntity( ent );
		return;
	}
	// Race checkpoints arrive with milestone 6.
	G_Printf( "race_point: Race is not implemented yet\n" );
	G_FreeEntity( ent );
}

// Steam achievement ids for target_achievement "id" 1..7 (retail table at
// qagamex86 0x1008d7cc).
static const int targetAchievementIds[] = { 0, 2, 3, 4, 5, 6, 7, 8 };

/*
================
Use_Target_Achievement

Unlocks the target's achievement for the activating player once.
================
*/
static void Use_Target_Achievement( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int achievement;
	int clientNum;

	(void)other;
	if ( !activator || !activator->client ) {
		return;
	}
	if ( ent->count <= 0 || ent->count >= ARRAY_LEN( targetAchievementIds ) ) {
		return;
	}

	achievement = targetAchievementIds[ent->count];
	clientNum = activator->client->ps.clientNum;
	if ( !trap_HasSteamAchievement( clientNum, achievement ) ) {
		trap_UnlockSteamAchievement( clientNum, achievement );
	}
}

/*QUAKED target_achievement (1 0 0) (-8 -8 -8) (8 8 8)
Unlocks a Steam achievement for the activator.
"id"	achievement number (1..7)
*/
void SP_target_achievement( gentity_t *ent ) {
	G_SpawnInt( "id", "", &ent->count );
	ent->use = Use_Target_Achievement;
}

/*
================
Use_Target_Remove_Keys
================
*/
static void Use_Target_Remove_Keys( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	(void)ent;
	(void)other;
	if ( !activator || !activator->client ) {
		return;
	}
	// Returning the carried keys to their spawn points comes with the key
	// doors in milestone 4; the mask itself is cleared now.
	activator->client->ps.stats[STAT_KEY_MASK] = 0;
}

/*QUAKED target_remove_keys (1 0 0) (-8 -8 -8) (8 8 8)
Takes every key from the activator.
*/
void SP_target_remove_keys( gentity_t *ent ) {
	ent->use = Use_Target_Remove_Keys;
}

/*QUAKED trigger_capturezone (.5 .5 .5) ?
Flag capture area for maps without flag stands.
*/
void SP_trigger_capturezone( gentity_t *ent ) {
	InitTrigger( ent );
	// The capture touch handler arrives with the flag modes in milestone 5;
	// no stock map uses this entity.
	trap_LinkEntity( ent );
}
