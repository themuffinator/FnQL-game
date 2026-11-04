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

// Retail drop command policy: qagamex86 0x10044910, 0x10044ae0,
// 0x10044f10 and 0x10045250. The g_dropCmds bits are flag=1,
// powerup=2, weapon=4; runes have their own switch.

static qboolean G_ManualDropAllowed( gentity_t *ent, dropKind_t kind ) {
	static const char *const names[] = { "DropFlag", "DropPowerup", "DropRune", "DropWeapon" };
	static const int masks[] = { 1, 2, 0, 4 };
	const int mode = g_gametype.integer;
	const char *reason = NULL;

	if ( kind == DROP_RUNE && !g_runes.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Runes are not enabled on this server.\n\"" );
		return qfalse;
	}
	if ( kind != DROP_RUNE && !( g_dropCmds.integer & masks[kind] ) ) {
		reason = "enabled on this server";
	} else if ( kind == DROP_FLAG && mode != GT_CTF && mode != GT_1FCTF && mode != GT_ATTACK_DEFEND ) {
		reason = "available in non-flag gametypes";
	} else if ( ( kind == DROP_POWERUP || kind == DROP_WEAPON ) && mode < GT_TEAM ) {
		reason = "available in non-team gametypes";
	} else if ( ( kind == DROP_RUNE && mode == GT_RACE )
		|| ( ( kind == DROP_POWERUP || kind == DROP_WEAPON )
			&& ( mode == GT_CLAN_ARENA || mode == GT_DOMINATION || mode == GT_ATTACK_DEFEND || mode == GT_RED_ROVER ) ) ) {
		reason = va( "available in %s", G_GametypeName( mode ) );
	} else if ( level.timeoutStartTime ) {
		reason = "available during a pause or timeout";
	} else if ( ( kind == DROP_POWERUP || kind == DROP_WEAPON ) && g_instaGib.integer ) {
		reason = "available in InstaGib";
	}
	if ( reason ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"%s is not %s.\n\"", names[kind], reason ) );
		return qfalse;
	}
	return ent->client && ent->client->ps.pm_type != PM_SPECTATOR;
}

// A manual drop must fit at the destination and all the way to it. Retail
// traces only solid brushes, with ENTITYNUM_NONE, and places ordinary drops
// 64 units ahead using yaw alone (0x10044d11, 0x100454d1, 0x100513b0).
static qboolean G_ManualDropPosition( gentity_t *ent, qboolean flag, vec3_t origin, vec3_t velocity ) {
	vec3_t angles = { 0, 0, 0 }, forward;
	vec3_t mins = { -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS };
	vec3_t maxs = { ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS };
	trace_t trace;
	int i;

	VectorClear( velocity );
	if ( flag && g_throwFlagVelocity.integer ) {
		PM_AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
		VectorCopy( ent->client->ps.origin, origin );
		for ( i = 0; i < 3; i++ ) {
			const float scaled = (float)( (double)forward[i] * g_throwFlagForwardMult.value );
			velocity[i] = (float)( (double)scaled * g_throwFlagVelocity.value + (double)vec3_origin[i] );
		}
	} else {
		angles[YAW] = ent->client->ps.viewangles[YAW];
		PM_AngleVectors( angles, forward, NULL, NULL );
		for ( i = 0; i < 3; i++ ) {
			origin[i] = (float)( (double)ent->client->ps.origin[i] + (double)forward[i] * 64.0 );
		}
	}
	if ( flag ) {
		maxs[2] = 35;
	}
	trap_Trace( &trace, ent->client->ps.origin, mins, maxs, origin, ENTITYNUM_NONE, CONTENTS_SOLID );
	return trace.fraction == 1.0f;
}

static gentity_t *G_ManualDropItem( gentity_t *owner, gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t *drop = LaunchItem( item, origin, velocity );
	// Manual drops expire after ten seconds; LaunchItem supplies either the
	// flag-return callback or the ordinary free callback (retail 0x10051611).
	drop->nextthink = level.time + 10000;
	drop->s.clientNum = owner->s.clientNum;
	drop->s.time = level.time;
	return drop;
}

void G_DropItemCommand( gentity_t *ent, dropKind_t kind ) {
	playerState_t *ps;
	gitem_t *item;
	gentity_t *drop;
	vec3_t origin, velocity;
	int tag, weapon, replacement, amount;

	if ( kind < DROP_FLAG || kind > DROP_WEAPON || !G_ManualDropAllowed( ent, kind ) ) {
		return;
	}
	ps = &ent->client->ps;
	if ( kind == DROP_FLAG && ( ps->pm_flags & PMF_NO_MOVE ) ) {
		return;
	}
	if ( !G_ManualDropPosition( ent, kind == DROP_FLAG, origin, velocity ) ) {
		return;
	}
	if ( kind == DROP_WEAPON ) {
		weapon = ent->s.weapon;
		if ( weapon <= WP_MACHINEGUN || weapon >= WP_NUM_WEAPONS
			|| ps->ammo[weapon] <= 0 || !( ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			return;
		}
		item = BG_FindItemForWeapon( weapon );
		if ( !item ) {
			return;
		}
		amount = ps->ammo[weapon] < item->quantity ? ps->ammo[weapon] : item->quantity;
		for ( replacement = weapon - 1; replacement > WP_NONE; replacement-- ) {
			if ( ps->stats[STAT_WEAPONS] & ( 1 << replacement ) ) {
				break;
			}
		}
		ps->ammo[weapon] -= amount;
		ps->stats[STAT_WEAPONS] &= ~( 1 << weapon );
		ps->weapon = replacement;
		drop = G_ManualDropItem( ent, item, origin, velocity );
		drop->count = amount;
		BG_AddPredictableEventToPlayerstate( EV_DROP_WEAPON, weapon, ps, -1 );
		return;
	}
	if ( kind == DROP_FLAG ) {
		tag = ps->powerups[PW_NEUTRALFLAG] ? PW_NEUTRALFLAG
			: ps->powerups[PW_REDFLAG] ? PW_REDFLAG : PW_BLUEFLAG;
		if ( !ps->powerups[tag] || !( item = BG_FindItemForPowerup( tag ) ) ) {
			return;
		}
		drop = G_ManualDropItem( ent, item, origin, velocity );
		drop->physicsObject = qtrue;
		drop->physicsBounce = g_flagBounce.value;
		drop->target_ent = ent;
		if ( g_flagPhysics.integer ) {
			drop->frameThink = G_RunFlagPhysics;
		}
		ps->powerups[tag] = 0;
		return;
	}
	for ( tag = PW_REDFLAG; tag < PW_NUM_POWERUPS; tag++ ) {
		if ( ps->powerups[tag] <= level.time || tag == PW_FLIGHT || tag == PW_FROZEN ) {
			continue;
		}
		// Retail BG_FindItemForPowerup (0x1002cb60) maps PW_SCOUT..PW_ARMORREGEN
		// onto the separate PP_* tags. Keep the conversion explicit here.
		item = tag >= PW_SCOUT && tag <= PW_ARMORREGEN
			? BG_FindItemForPersistantPowerup( tag - PW_SCOUT + PP_SCOUT )
			: BG_FindItemForPowerup( tag );
		if ( !item || ( item->giType == IT_TEAM && tag <= PW_NEUTRALFLAG )
			|| ( ( item->giType == IT_PERSISTANT_POWERUP ) != ( kind == DROP_RUNE ) ) ) {
			continue;
		}
		drop = G_ManualDropItem( ent, item, origin, velocity );
		amount = ( ps->powerups[tag] - level.time ) / 1000;
		drop->count = amount > 0 ? amount : 1;
		if ( kind == DROP_POWERUP && g_droppedPowerupsDecay.integer ) {
			drop->flags |= FL_POWERUP_DECAY;
			if ( ps->powerups[tag] < drop->nextthink ) {
				drop->nextthink = ps->powerups[tag];
			}
		}
		ps->powerups[tag] = 0;
		if ( kind == DROP_RUNE ) {
			ent->client->persistantPowerup = NULL;
			ps->stats[STAT_PERSISTANT_POWERUP] = 0;
		}
	}
}

