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

qboolean G_IsQuadHog( void ) {
	return g_gametype.integer == GT_FFA && g_quadHog.integer != 0;
}

static void QuadHogTouch( gentity_t *quad, gentity_t *player, trace_t *trace ) {
	// Retail 0x1004d290 announces before Touch_Item performs pickup policy.
	if ( !player || !player->client ) return;
	trap_SendServerCommand( -1, va( "cp \"%s is the ^5Quad Hog!\"", player->client->pers.netname ) );
	Touch_Item( quad, player, trace );
}

gentity_t *G_LaunchQuadHog( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t *quad = LaunchItem( item, origin, velocity );
	long long deadline = (long long)level.time + (long long)g_quadHogIdle.integer * 1000;
	// 0x1004d2d0: ordinary dropped-item state, but custom touch and an idle
	// lifetime controlled by g_quadHogIdle (not a carrier inactivity limit).
	quad->touch = QuadHogTouch;
	quad->nextthink = deadline > INT_MAX ? INT_MAX : deadline < 1 ? 1 : (int)deadline;
	return quad;
}

void G_CheckQuadHog( void ) {
	gentity_t *spot = NULL, *item;
	int i, draws;
	if ( !G_IsQuadHog() || level.intermissionQueued || level.intermissiontime ) return;
	// 0x1004d500: either a carrier or a linked world quad owns the objective.
	for ( i = 0; i < level.maxclients; ++i ) {
		if ( g_entities[i].client && g_entities[i].client->ps.powerups[PW_QUAD] ) return;
	}
	for ( item = NULL; ( item = G_Find( item, FOFS(classname), "item_quad" ) ) != NULL; ) {
		if ( item->r.linked ) return;
	}
	// 0x1004d420 draws 0..16, walks deathmatch spawns, and restarts after
	// reaching the end. Preserve the random draw even on malformed maps,
	// but avoid retail's endless retry when no deathmatch spawn exists.
	draws = (int)( G_Random() * 16.0 );
	for ( i = 0; i < draws; ++i ) spot = G_Find( spot, FOFS(classname), "info_player_deathmatch" );
	if ( !spot ) spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch" );
	if ( !spot ) return;
	G_LaunchQuadHog( BG_FindItemForPowerup( PW_QUAD ), spot->s.origin, vec3_origin );
	item = G_TempEntity( spot->s.origin, EV_GLOBAL_SOUND );
	item->r.svFlags |= SVF_BROADCAST;
	item->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
	trap_SendServerCommand( -1, "cp \" ^5Quad ^7respawned!\"" );
}

void G_QuadHogPing( gentity_t *player ) {
	gentity_t *event = G_TempEntity( player->client->ps.origin, EV_POI );
	event->r.svFlags |= SVF_BROADCAST;
	event->s.powerups = g_quadHogPingRate.integer;
	event->s.generic1 = player->client->ps.clientNum;
	player->client->quadHogPingResidual = 0;
}

void G_QuadHogTimer( gentity_t *player, int msec ) {
	long long elapsed;
	if ( !G_IsQuadHog() || !player->client->ps.powerups[PW_QUAD] ) return;
	// 0x10034725: elapsed user-command time, paused along with timer actions.
	elapsed = (long long)player->client->quadHogPingResidual + msec;
	player->client->quadHogPingResidual = elapsed > INT_MAX ? INT_MAX : elapsed < 0 ? 0 : (int)elapsed;
	if ( player->client->quadHogPingResidual >= g_quadHogPingRate.integer ) G_QuadHogPing( player );
}

// Retail 0x1004f293 / 0x10034777: neutral carriers ping their team number
// immediately on pickup and then on command time, sharing the Quad Hog clock.
void G_NeutralFlagPing( gentity_t *player ) {
	gentity_t *event = G_TempEntity( player->client->ps.origin, EV_POI );
	event->r.svFlags |= SVF_BROADCAST;
	event->s.powerups = g_neutralFlagPingRate.integer;
	event->s.generic1 = player->client->ps.persistant[PERS_TEAM];
	player->client->quadHogPingResidual = 0;
}

void G_NeutralFlagTimer( gentity_t *player, int msec ) {
	long long elapsed;
	if ( !player->client->ps.powerups[PW_NEUTRALFLAG] ) return;
	elapsed = (long long)player->client->quadHogPingResidual + msec;
	player->client->quadHogPingResidual = elapsed > INT_MAX ? INT_MAX : elapsed < 0 ? 0 : (int)elapsed;
	if ( player->client->quadHogPingResidual >= g_neutralFlagPingRate.integer ) G_NeutralFlagPing( player );
}
