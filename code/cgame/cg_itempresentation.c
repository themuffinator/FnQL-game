/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* How a world item is placed and animated. Observed in retail cgame's item
 * drawer (0x10015f30..0x100175e4):
 *
 * - An item whose row mask (gitem_t +0x40) has the gametype's bit is not
 *   drawn (0x10016c8d); the red and blue flags carry One Flag's bit.
 * - cg_itemFx is a bit set. Bit 1 bobs the model: 4 + 6 * sin of a 1024 ms
 *   cycle that every item shares (0x10016e5e). Bit 2 rotates it, health at
 *   the fast rate (0x10016eb1); without it the model keeps the fixed axis.
 *   Bit 4 scales a model up over the second after it respawns (0x10017047).
 * - cg_simpleItems draws every item but the team items as a sprite
 *   (0x10016cbe): cg_simpleItemsHeightOffset above the item's origin (10
 *   for a persistant powerup); cg_simpleItemsBob 1, or 2 for an item with a
 *   timer (armor, mega health, the powerups), replaces that with 10 plus a
 *   6-unit sine on the same cycle; then cg_simpleItemsRadius less 15 is
 *   added, and the radius is the cvar's integer.
 * - cg_flagStyle 2 draws the three flags with the row's fifth model
 *   (0x10016fa9).
 * - The railgun pickup is green (0x1001712b); the other models keep the
 *   entity's color. */

#include "cg_local.h"

#define ITEM_BOB_CYCLE		1024
#define ITEM_BOB_HEIGHT		6.0f
#define ITEM_BOB_BASE		4.0f

static float CG_ItemBobSine( int time ) {
	return ITEM_BOB_HEIGHT * (float)sin( ( time & ( ITEM_BOB_CYCLE - 1 ) ) * ( 2.0 * M_PI / ITEM_BOB_CYCLE ) );
}

qboolean CG_ItemHiddenInGametype( const gitem_t *item, int gametype ) {
	if ( !item || gametype < 0 || gametype > 30 ) {
		return qfalse;
	}
	return ( item->retail[0] & ( 1 << gametype ) ) ? qtrue : qfalse;
}

/* What the bob adds to a model item's height. */
float CG_ItemBobOffset( int itemFx, int time ) {
	if ( !( itemFx & 1 ) ) {
		return 0.0f;
	}
	return ITEM_BOB_BASE + CG_ItemBobSine( time );
}

/* 0: the fixed axis, 1: the slow rotation, 2: the fast one. */
int CG_ItemRotation( int itemFx, const gitem_t *item ) {
	if ( !( itemFx & 2 ) ) {
		return 0;
	}
	return ( item && item->giType == IT_HEALTH ) ? 2 : 1;
}

/* The scale of a model sinceSpawn milliseconds after it respawned. */
float CG_ItemSpawnScale( int itemFx, int sinceSpawn ) {
	if ( !( itemFx & 4 ) || sinceSpawn < 0 || sinceSpawn >= ITEM_SCALEUP_TIME ) {
		return 1.0f;
	}
	return (float)sinceSpawn / ITEM_SCALEUP_TIME;
}

/* The height of a simple item's sprite over its origin, without the radius
 * term; *bobs says whether the bob replaced the height offset. */
float CG_SimpleItemLift( const gitem_t *item, int bobMode, int heightOffset, int time, qboolean *bobs ) {
	const qboolean bobbing = ( bobMode == 1 || ( bobMode == 2 && item && item->timer ) ) ? qtrue : qfalse;

	if ( bobs ) {
		*bobs = bobbing;
	}
	if ( bobbing ) {
		return 10.0f + CG_ItemBobSine( time );
	}
	return ( item && item->giType == IT_PERSISTANT_POWERUP ) ? 10.0f : (float)heightOffset;
}

/* Whether cg_flagStyle picks the row's fifth model for this item. */
const char *CG_ItemFlagStyleModel( const gitem_t *item, int flagStyle ) {
	if ( flagStyle != 2 || !item || item->giType != IT_TEAM ) {
		return NULL;
	}
	if ( item->giTag != PW_REDFLAG && item->giTag != PW_BLUEFLAG && item->giTag != PW_NEUTRALFLAG ) {
		return NULL;
	}
	return item->world_model[4];
}
