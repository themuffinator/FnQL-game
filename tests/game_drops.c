/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/


// Exercise the production command policy and state transitions with a small
// fake world. G3 covers real geometry, item pickups, events and expiration.
#include "../code/game/g_local.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vec3_t vec3_origin;
vmCvar_t g_gametype, g_dropCmds, g_runes, g_instaGib, g_droppedPowerupsDecay;
vmCvar_t g_throwFlagVelocity, g_throwFlagForwardMult, g_flagBounce;
vmCvar_t g_flagPhysics, g_tackleFlag;
static gclient_t client;
static gentity_t drops[16];
static gitem_t items[4] = {
	{ NULL, NULL, { NULL }, NULL, NULL, 10, IT_WEAPON, WP_ROCKET_LAUNCHER },
	{ NULL, NULL, { NULL }, NULL, NULL, 30, IT_POWERUP, PW_QUAD },
	{ NULL, NULL, { NULL }, NULL, NULL, 0, IT_TEAM, PW_BLUEFLAG },
	{ NULL, NULL, { NULL }, NULL, NULL, 0, IT_PERSISTANT_POWERUP, PP_SCOUT }
};
static int dropCount, traces, event, eventParm;
static qboolean blocked;
static char message[256];

void G_RunFlagPhysics( gentity_t *flag ) { CHECK(0); }

char *QDECL va( const char *format, ... ) {
	static char buffers[4][512];
	static unsigned index;
	char *result = buffers[index++ % 4];
	va_list args;
	va_start( args, format );
	vsnprintf( result, 512, format, args );
	va_end( args );
	return result;
}

const char *G_GametypeName( int mode ) {
	switch ( mode ) {
	case GT_RACE: return "Race";
	case GT_CLAN_ARENA: return "Clan Arena";
	case GT_DOMINATION: return "Domination";
	case GT_ATTACK_DEFEND: return "Attack and Defend";
	case GT_RED_ROVER: return "Red Rover";
	default: CHECK(0); return "";
	}
}

void trap_SendServerCommand( int slot, const char *text ) {
	CHECK( slot == 0 );
	snprintf( message, sizeof(message), "%s", text );
}

void PM_AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up ) {
	CHECK( right == NULL && up == NULL );
	VectorSet( forward, 1, 0, 0 );
}

void trap_Trace( trace_t *trace, const vec3_t start, const vec3_t mins,
	const vec3_t maxs, const vec3_t end, int pass, int mask ) {
	CHECK( pass == ENTITYNUM_NONE && mask == CONTENTS_SOLID );
	CHECK( mins[0] == -15 && mins[2] == -15 && maxs[0] == 15 );
	CHECK( maxs[2] == 15 || maxs[2] == 35 );
	CHECK( end[0] == start[0] + 64 || end[0] == start[0] );
	memset( trace, 0, sizeof(*trace) );
	trace->fraction = blocked ? 0.5f : 1.0f;
	traces++;
}

gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t *drop;
	CHECK( dropCount < 16 );
	drop = &drops[dropCount++];
	drop->item = item;
	drop->flags = FL_DROPPED_ITEM;
	VectorCopy( origin, drop->s.pos.trBase );
	VectorCopy( velocity, drop->s.pos.trDelta );
	return drop;
}

gitem_t *BG_FindItemForWeapon( weapon_t weapon ) {
	CHECK( weapon == WP_ROCKET_LAUNCHER );
	return &items[0];
}

gitem_t *BG_FindItemForPowerup( powerup_t powerup ) {
	if ( powerup == PW_QUAD ) return &items[1];
	if ( powerup == PW_BLUEFLAG ) return &items[2];
	return NULL;
}

gitem_t *BG_FindItemForPersistantPowerup( persistantPowerup_t powerup ) {
	CHECK( powerup == PP_SCOUT );
	return &items[3];
}

void BG_AddPredictableEventToPlayerstate( entity_event_t value, int parm, playerState_t *ps, int number ) {
	CHECK( ps == &client.ps );
	event = value;
	eventParm = parm;
}

static void Reset( void ) {
	memset( &client, 0, sizeof(client) );
	memset( g_entities, 0, sizeof(g_entities) );
	memset( drops, 0, sizeof(drops) );
	memset( &level, 0, sizeof(level) );
	g_entities[0].client = &client;
	level.time = 2500;
	g_gametype.integer = GT_TEAM;
	g_dropCmds.integer = 7;
	g_runes.integer = 0;
	g_instaGib.integer = 0;
	g_droppedPowerupsDecay.integer = 1;
	g_throwFlagVelocity.integer = 0;
	g_flagBounce.value = 0.25f;
	g_flagPhysics.integer = 0;
	dropCount = traces = event = eventParm = 0;
	blocked = qfalse;
	message[0] = '\0';
}

static void Drop( dropKind_t kind ) { G_DropItemCommand( &g_entities[0], kind ); }

int main( void ) {
	int i, ammo;
	const int ammoCounts[] = { -1, 0, 1, 9, 10, 11 };
	const int forbidden[] = { GT_CLAN_ARENA, GT_DOMINATION, GT_ATTACK_DEFEND, GT_RED_ROVER };
	for ( i = 0; i < 4; i++ ) {
		Reset(); g_gametype.integer = forbidden[i]; Drop( DROP_WEAPON );
		CHECK( strstr( message, G_GametypeName(forbidden[i]) ) && !traces );
	}
	Reset(); g_dropCmds.integer = 1; Drop( DROP_WEAPON );
	CHECK( !strcmp(message, "print \"DropWeapon is not enabled on this server.\n\"") && !traces );
	Reset(); g_gametype.integer = GT_FFA; Drop( DROP_POWERUP );
	CHECK( strstr(message, "non-team gametypes") && !traces );
	Reset(); Drop( DROP_FLAG ); CHECK( strstr(message, "non-flag gametypes") && !traces );
	Reset(); Drop( DROP_RUNE ); CHECK( strstr(message, "Runes are not enabled") && !traces );
	Reset(); g_runes.integer = 1; g_gametype.integer = GT_RACE; Drop( DROP_RUNE );
	CHECK( strstr(message, "not available in Race") && !traces );
	for ( i = DROP_FLAG; i <= DROP_WEAPON; i++ ) {
		Reset(); g_gametype.integer = GT_CTF; g_runes.integer = 1;
		level.timeoutStartTime = 1; Drop( i );
		CHECK( strstr(message, "pause or timeout") && !traces );
		Reset(); client.ps.pm_type = PM_SPECTATOR; g_gametype.integer = GT_CTF;
		g_runes.integer = 1; Drop( i ); CHECK( !message[0] && !traces );
	}
	Reset(); g_instaGib.integer = 1; Drop( DROP_WEAPON );
	CHECK( strstr(message, "InstaGib") && !traces );

	// A blocked drop leaves ownership, ammo and events untouched. With room,
	// transfer at most the normal pickup quantity, even with surplus ammo.
	for ( i = 0; i < sizeof(ammoCounts) / sizeof(ammoCounts[0]); i++ ) {
		ammo = ammoCounts[i];
		Reset(); g_entities[0].s.weapon = WP_ROCKET_LAUNCHER;
		client.ps.stats[STAT_WEAPONS] = (1 << WP_ROCKET_LAUNCHER) | (1 << WP_SHOTGUN);
		client.ps.ammo[WP_ROCKET_LAUNCHER] = ammo;
		blocked = qtrue; Drop( DROP_WEAPON );
		CHECK( !dropCount && !event && client.ps.ammo[WP_ROCKET_LAUNCHER] == ammo );
		blocked = qfalse; Drop( DROP_WEAPON );
		if ( ammo <= 0 ) { CHECK( !dropCount && !event ); continue; }
		CHECK( dropCount == 1 && drops[0].count == (ammo < 10 ? ammo : 10) );
		CHECK( client.ps.ammo[WP_ROCKET_LAUNCHER] == ammo - drops[0].count );
		CHECK( client.ps.stats[STAT_WEAPONS] == (1 << WP_SHOTGUN) && client.ps.weapon == WP_SHOTGUN );
		CHECK( event == EV_DROP_WEAPON && eventParm == WP_ROCKET_LAUNCHER );
	}

	Reset(); client.ps.powerups[PW_QUAD] = level.time + 500;
	Drop( DROP_POWERUP );
	CHECK( dropCount == 1 && drops[0].count == 1 && (drops[0].flags & FL_POWERUP_DECAY) );
	CHECK( drops[0].nextthink == level.time + 500 && drops[0].s.time == level.time );
	CHECK( !client.ps.powerups[PW_QUAD] );
	Reset(); g_droppedPowerupsDecay.integer = 0; client.ps.powerups[PW_QUAD] = level.time + 500;
	Drop( DROP_POWERUP ); CHECK( !(drops[0].flags & FL_POWERUP_DECAY) && drops[0].nextthink == level.time + 10000 );

	// Rune slots use PW_* in playerState and PP_* in the item table. Dropping
	// ordinary powerups must preserve the rune and both persistent references.
	Reset(); g_runes.integer = 1;
	client.ps.powerups[PW_SCOUT] = 0x7fffffff;
	client.ps.stats[STAT_PERSISTANT_POWERUP] = PP_SCOUT;
	client.persistantPowerup = &g_entities[10];
	Drop( DROP_POWERUP ); CHECK( !dropCount && client.persistantPowerup );
	Drop( DROP_RUNE );
	CHECK( dropCount == 1 && drops[0].item == &items[3] && drops[0].nextthink == level.time + 10000 );
	CHECK( !client.ps.powerups[PW_SCOUT] && !client.ps.stats[STAT_PERSISTANT_POWERUP] && !client.persistantPowerup );

	Reset(); g_gametype.integer = GT_CTF; client.ps.powerups[PW_BLUEFLAG] = 0x7fffffff;
	client.ps.pm_flags = PMF_NO_MOVE; Drop( DROP_FLAG ); CHECK( !traces && !dropCount );
	client.ps.pm_flags = 0; Drop( DROP_FLAG );
	CHECK( dropCount == 1 && drops[0].physicsObject && drops[0].physicsBounce == 0.25f );
	CHECK( drops[0].target_ent == &g_entities[0] && !client.ps.powerups[PW_BLUEFLAG] );
	CHECK( !drops[0].frameThink );
	Reset(); g_gametype.integer = GT_CTF; g_flagPhysics.integer = 1;
	client.ps.powerups[PW_BLUEFLAG] = 0x7fffffff; Drop( DROP_FLAG );
	CHECK( dropCount == 1 && drops[0].frameThink == G_RunFlagPhysics );

	// g_tackleFlag (retail 0x1004983a / 0x100513b0): the flag goes 64 units
	// up with a straight-up toss, needs room and a movable, playing carrier,
	// and leaves nothing behind without a flag.
	Reset(); g_tackleFlag.integer = 600; VectorSet( client.ps.origin, 10, 20, 30 );
	client.ps.powerups[PW_BLUEFLAG] = 0x7fffffff; G_TackleFlag( &g_entities[0] );
	CHECK( traces == 1 && dropCount == 1 && drops[0].item == &items[2] && !client.ps.powerups[PW_BLUEFLAG] );
	CHECK( drops[0].s.pos.trBase[0] == 10 && drops[0].s.pos.trBase[1] == 20 && drops[0].s.pos.trBase[2] == 94 );
	CHECK( drops[0].s.pos.trDelta[0] == 0 && drops[0].s.pos.trDelta[1] == 0 && drops[0].s.pos.trDelta[2] == 600 );
	CHECK( drops[0].physicsObject && drops[0].nextthink == level.time + 10000 && drops[0].target_ent == &g_entities[0] );
	Reset(); g_tackleFlag.integer = 600; G_TackleFlag( &g_entities[0] ); CHECK( traces == 1 && !dropCount );
	Reset(); g_tackleFlag.integer = 600; client.ps.powerups[PW_BLUEFLAG] = 0x7fffffff;
	blocked = qtrue; G_TackleFlag( &g_entities[0] ); CHECK( !dropCount && client.ps.powerups[PW_BLUEFLAG] );
	blocked = qfalse; traces = 0; client.ps.pm_flags = PMF_NO_MOVE; G_TackleFlag( &g_entities[0] ); CHECK( !traces && !dropCount );
	client.ps.pm_flags = 0; client.ps.pm_type = PM_SPECTATOR; G_TackleFlag( &g_entities[0] ); CHECK( !traces && !dropCount );
	g_entities[0].client = NULL; G_TackleFlag( &g_entities[0] ); CHECK( !traces );
	puts("manual drop command policy and state transitions: OK");
	return 0;
}
