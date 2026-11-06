// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#define ITEM_DEG2RAD	0.01745329238474369	// retail's single-precision degrees to radians

// retail respawn delays in seconds (qagamex86 0x1004f13e-0x1004f196)
#define RESPAWN_ARMOR_SECONDS		25
#define RESPAWN_HEALTH_SECONDS		35
#define RESPAWN_HOLDABLE_SECONDS	60

/*
===============
G_IsFlagMode

Retail qagamex86 0x10059e50: the modes with team bases.
===============
*/
static qboolean G_IsFlagMode( void ) {
	switch ( g_gametype.integer ) {
	case GT_CTF:
	case GT_1FCTF:
	case GT_OBELISK:
	case GT_HARVESTER:
	case GT_ATTACK_DEFEND:
		return qtrue;
	default:
		return qfalse;
	}
}

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/

//======================================================================

/*
===============
G_AnnouncePickup

Retail qagamex86 0x1004dc20.  A powerup is announced to everyone; red
armor and mega health only to the taker's side: teammates and spectators
(in the free modes, the spectators), told which half of a flag map it was
on.
===============
*/
static void G_AnnouncePickup( gentity_t *other, gentity_t *ent, qboolean everyone ) {
	const char	*color, *side;
	int			i, enemy;
	gentity_t	*listener;

	// The entire announcement is gated on a map pickup, not just its side
	// label (retail 0x1004dcbc). Transfers must not announce another spawn.
	if ( !other->client || ( ent->flags & FL_DROPPED_ITEM ) ) {
		return;
	}
	side = S_COLOR_WHITE;
	if ( G_IsFlagMode() ) {
		vec3_t toRed, toBlue;

		VectorSubtract( level.redFlagBase, ent->s.origin, toRed );
		VectorSubtract( level.blueFlagBase, ent->s.origin, toBlue );
		side = (float)( (double)toBlue[2] * toBlue[2] + (double)toBlue[1] * toBlue[1] + (double)toBlue[0] * toBlue[0] )
			> (float)( (double)toRed[2] * toRed[2] + (double)toRed[1] * toRed[1] + (double)toRed[0] * toRed[0] )
			? S_COLOR_RED : S_COLOR_BLUE;
	}

	switch ( g_gametype.integer ) {
	case GT_CTF:
	case GT_1FCTF:
	case GT_OBELISK:
	case GT_HARVESTER:
	case GT_DOMINATION:
	case GT_ATTACK_DEFEND:
		color = S_COLOR_WHITE;
		break;
	default:
		color = S_COLOR_YELLOW;
		break;
	}

	if ( everyone ) {
		// the colour follows the name (retail qagamex86 0x1004dd97)
		trap_SendServerCommand( -1, va( "print \"%s%s got the %s!" S_COLOR_WHITE "\n\"",
			other->client->pers.netname, color, ent->item->pickup_name ) );
		return;
	}

	enemy = other->client->sess.sessionTeam == TEAM_RED ? TEAM_BLUE
		: other->client->sess.sessionTeam == TEAM_BLUE ? TEAM_RED : TEAM_FREE;
	for ( i = 0; i < level.maxclients; i++ ) {
		listener = &g_entities[i];
		if ( level.clients[i].pers.connected != CON_CONNECTED || listener == other
			|| level.clients[i].sess.sessionTeam == enemy ) {
			continue;
		}
		trap_SendServerCommand( i, va( "print \"%s " S_COLOR_WHITE "got the %s%s.\n\"",
			other->client->pers.netname, G_IsFlagMode() ? side : S_COLOR_WHITE, ent->item->pickup_name ) );
	}
}


/*
===============
G_CountItemPickup

The item-control statistics of armor and mega health (retail
qagamex86 0x1004e540, 0x1004e810).
===============
*/
static void G_CountItemPickup( gentity_t *ent, gentity_t *other, itemControl_t *control, teamStat_t teamStat ) {
	gclient_t *client = other->client;

	if ( !ent->itemPickups ) {
		control->first++;
	}
	control->pickups++;
	if ( ent->itemPickups > 0 ) {
		control->waited += level.time - ent->itemSpawnTime;
	}
	if ( g_gametype.integer >= GT_TEAM ) {
		level.teamStats[teamStat][client->sess.sessionTeam]++;
	}
}


/*
===============
Pickup_Powerup

Retail qagamex86 0x1004dfe0: quad hog's quad lasts g_quadHogTime; the
frag counts of the quad and the battle suit restart with a fresh one; the
timer starts on a whole second; nearby enemies who saw it go are denied.
===============
*/
int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	gclient_t	*client = other->client;
	const int	tag = ent->item->giTag;
	int			quantity;
	int			i;

	quantity = ent->count ? ent->count : ent->item->quantity;
	// Retail 0x1004dfe0: only manually dropped, decay-marked powerups lose
	// whole seconds on the ground. Death drops retain their existing rules.
	if ( ent->flags & FL_POWERUP_DECAY ) {
		quantity -= ( level.time - ent->s.time ) / 1000;
		if ( quantity < 0 ) {
			quantity = 0;
		}
	}

	if ( !client->ps.powerups[PW_BATTLESUIT] && tag == PW_BATTLESUIT ) {
		client->ps.stats[STAT_BATTLESUIT_FRAG_COUNT] = 0;
	}
	if ( !client->ps.powerups[PW_QUAD] && tag == PW_QUAD ) {
		client->ps.stats[STAT_QUAD_FRAG_COUNT] = 0;
	}
	if ( g_quadHog.integer && g_gametype.integer == GT_FFA ) {
		quantity = g_quadHogTime.integer;
	}

	if ( !client->ps.powerups[tag] ) {
		// round timing to seconds to make multiple powerup timers count in sync
		client->ps.powerups[tag] = level.time / 1000 * 1000;
	}
	client->ps.powerups[tag] += quantity * 1000;

	client->stats.powerupPickups[tag]++;
	if ( !ent->s.modelindex2 ) {
		switch ( tag ) {
		case PW_QUAD:
			client->stats.quadPickups++;
			if ( g_gametype.integer >= GT_TEAM ) {
				level.teamStats[TEAM_STAT_QUAD][client->sess.sessionTeam]++;
			}
			break;
		case PW_BATTLESUIT:
			client->stats.battleSuitPickups++;
			if ( g_gametype.integer >= GT_TEAM ) {
				level.teamStats[TEAM_STAT_BATTLESUIT][client->sess.sessionTeam]++;
			}
			break;
		case PW_REGEN:
			client->stats.regenPickups++;
			if ( g_gametype.integer >= GT_TEAM ) {
				level.teamStats[TEAM_STAT_REGEN][client->sess.sessionTeam]++;
			}
			break;
		case PW_HASTE:
			client->stats.hastePickups++;
			if ( g_gametype.integer >= GT_TEAM ) {
				level.teamStats[TEAM_STAT_HASTE][client->sess.sessionTeam]++;
			}
			break;
		case PW_INVIS:
			client->stats.invisPickups++;
			if ( g_gametype.integer >= GT_TEAM ) {
				level.teamStats[TEAM_STAT_INVIS][client->sess.sessionTeam]++;
			}
			break;
		default:
			break;
		}
	}

	G_AnnouncePickup( other, ent, qtrue );

	// give any nearby players a "denied" anti-reward
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*witness = &level.clients[i];
		vec3_t		delta, forward;
		trace_t		tr;

		if ( witness->pers.connected == CON_DISCONNECTED || witness == client
			|| witness->ps.stats[STAT_HEALTH] <= 0 ) {
			continue;
		}
		// if same team in team game, no sound
		if ( g_gametype.integer >= GT_TEAM && witness->sess.sessionTeam == client->sess.sessionTeam ) {
			continue;
		}

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, witness->ps.origin, delta );
		if ( PM_VectorNormalize( delta ) > 192 ) {
			continue;
		}

		// if not facing, no sound
		PM_AngleVectors( witness->ps.viewangles, forward, NULL, NULL );
		if ( PM_Dot3( forward, delta ) < 0.4 ) {
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, witness->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction != 1.0f ) {
			continue;
		}

		// anti-reward
		witness->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
		client->stats.denies++;
	}

	return g_powerupRespawn.integer;
}


/*
===============
Pickup_PersistantPowerup

Retail qagamex86 0x1004e4a0: the rune is held until death, shown as its
powerup with no end; it goes back to its spawn only when dropped.
===============
*/
int Pickup_PersistantPowerup( gentity_t *ent, gentity_t *other ) {
	gclient_t *client = other->client;

	client->persistantPowerup = ent;
	client->ps.stats[STAT_PERSISTANT_POWERUP] = ent->item->giTag;
	client->ps.powerups[PW_SCOUT - PP_SCOUT + ent->item->giTag] = 0x7fffffff;
	G_RunePickedUp( ent->item->giTag );

	return -1;
}


/*
===============
Pickup_Holdable

Retail qagamex86 0x1004ded0: the flight holdable fills its thrust and fuel
from the g_flight* settings.
===============
*/
int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {
	gclient_t	*client = other->client;
	const int	tag = ent->item->giTag;
	int			fuel;

	client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	if ( tag == HI_KAMIKAZE ) {
		client->ps.eFlags |= EF_KAMIKAZE;
	}
	if ( tag == HI_FLIGHT ) {
		fuel = g_maxFlightFuel.integer > 32001 ? 32001 : g_maxFlightFuel.integer;
		client->ps.stats[STAT_PLAYER_ITEM_THRUST] = g_flightThrust.integer;
		client->ps.stats[STAT_PLAYER_ITEM_TIME_MAX] = fuel;
		client->ps.stats[STAT_PLAYER_ITEM_TIME] = fuel;
		client->ps.stats[STAT_PLAYER_ITEM_RECHARGE] = g_flightRefuelRate.integer;
	}
	if ( tag == HI_MEDKIT && g_gametype.integer >= GT_TEAM ) {
		client->stats.medkitPickups++;
		level.teamStats[TEAM_STAT_MEDKIT][client->sess.sessionTeam]++;
	}
	if ( tag >= 0 && tag < HI_NUM_HOLDABLE ) {
		client->stats.holdablePickups[tag]++;
	}

	return RESPAWN_HOLDABLE_SECONDS;
}


//======================================================================


//======================================================================


/*
Retail Add_Ammo (qagamex86 0x1004dab0): an ammo pack adds its share to every
weapon owned, and afterwards every weapon is held to the most it can carry
(to its starting ammo in Domination). Infinite ammo adds nothing. Retail's
second switch at 0x1004db61 uses normal caps in CA and Red Rover as well.
*/
static void Add_Ammo( gentity_t *ent, int weapon, int count )
{
	playerState_t	*ps = &ent->client->ps;
	int				w, limit;
	qboolean		startingLimit;

	if ( g_infiniteAmmo.integer ) {
		// Retail 0x1004dac1: an LG pickup replenishes infinite ammo after
		// a discharge emptied it. Other weapons still add nothing.
		if ( g_lightningDischarge.integer && weapon == WP_LIGHTNING ) ps->ammo[weapon] = -1;
		return;
	}
	if ( weapon < 0 || weapon > WP_NUM_WEAPONS || ps->ammo[weapon] == -1 ) {
		return;
	}

	if ( weapon != WP_NUM_WEAPONS ) {
		ps->ammo[weapon] += count;
	} else {
		for ( w = WP_MACHINEGUN; w < WP_NUM_WEAPONS; w++ ) {
			if ( ps->stats[STAT_WEAPONS] & ( 1 << w ) ) {
				ps->ammo[w] += bg_weaponPackAmmo[w];
			}
		}
	}

	startingLimit = g_gametype.integer == GT_DOMINATION;
	for ( w = WP_MACHINEGUN; w < WP_NUM_WEAPONS; w++ ) {
		limit = startingLimit ? G_StartingAmmo( w ) : bg_weaponMaxAmmo[w];
		if ( ps->ammo[w] > limit ) {
			ps->ammo[w] = limit;
		}
	}
}


/*
===============
Pickup_Ammo

Retail qagamex86 0x1004e500.
===============
*/
static int Pickup_Ammo( gentity_t *ent, gentity_t *other )
{
	Add_Ammo( other, ent->item->giTag, ent->count ? ent->count : ent->item->quantity );
	other->client->stats.ammoPickups++;

	return g_ammoRespawn.integer;
}


/*
===============
Pickup_Weapon

Retail qagamex86 0x1004e950: the weapon's full ammo is added.  With weapons
staying (g_weaponRespawn 0) a placed weapon tops its ammo up to the
weapon's quantity and stays put, so the pickup event is raised here.
===============
*/
static int Pickup_Weapon( gentity_t *ent, gentity_t *other ) {
	gclient_t	*client = other->client;
	const int	weapon = ent->item->giTag;

	if ( !g_weaponRespawn.integer && ( client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) )
		&& client->ps.ammo[weapon] && !ent->s.modelindex2 ) {
		return 0;
	}
	if ( ent->count < 0 ) {
		G_Error( "Pickup_Weapon: item has infinite count" );
	}

	client->ps.stats[STAT_WEAPONS] |= ( 1 << weapon );
	Add_Ammo( other, weapon, ent->count ? ent->count : ent->item->quantity );

	if ( !g_weaponRespawn.integer && !ent->s.modelindex2 ) {
		client->ps.ammo[weapon] = ent->item->quantity;
	}
	if ( weapon == WP_GRAPPLING_HOOK ) {
		client->ps.ammo[weapon] = -1; // unlimited ammo
	}
	client->stats.weaponPickups[weapon]++;

	if ( !g_weaponRespawn.integer ) {
		if ( client->pers.predictItemPickup ) {
			G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
		} else {
			G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
		}
	}
	return g_weaponRespawn.integer;
}


/*
===============
Pickup_Health

Retail qagamex86 0x1004e810: small and mega health go to twice the maximum.
===============
*/
static int Pickup_Health( gentity_t *ent, gentity_t *other ) {
	gclient_t	*client = other->client;
	const int	tag = ent->item->giTag;
	int			max;

	max = client->ps.stats[STAT_MAX_HEALTH];
	if ( tag == HEALTH_TIER_SMALL || tag == HEALTH_TIER_MEGA ) {
		max *= 2;
	}

	other->health += ent->item->quantity;
	if ( other->health > max ) {
		other->health = max;
	}
	other->s.health = other->health;
	client->ps.stats[STAT_HEALTH] = other->health;

	if ( tag == HEALTH_TIER_MEGA ) {
		G_CountItemPickup( ent, other, &client->stats.mega, TEAM_STAT_MEGA_HEALTH );
		G_AnnouncePickup( other, ent, qfalse );
	} else {
		client->stats.healthPickups++;
	}

	return RESPAWN_HEALTH_SECONDS;
}


/*
===============
Pickup_Armor

Retail qagamex86 0x1004e540.  Classic armor adds its quantity up to twice
the maximum health; armor_tiered sets the tier being worn instead: red 150
more up to 200, yellow 100 up to 150, green 50 up to 100, a shard 2.
===============
*/
int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	gclient_t	*client = other->client;
	int			*armor = &client->ps.stats[STAT_ARMOR];
	const int	limit = client->ps.stats[STAT_MAX_HEALTH] * 2;

	switch ( ent->item->giTag ) {
	case ARMOR_TIER_RED:
		G_CountItemPickup( ent, other, &client->stats.redArmor, TEAM_STAT_RED_ARMOR );
		break;
	case ARMOR_TIER_YELLOW:
		G_CountItemPickup( ent, other, &client->stats.yellowArmor, TEAM_STAT_YELLOW_ARMOR );
		break;
	case ARMOR_TIER_GREEN:
		G_CountItemPickup( ent, other, &client->stats.greenArmor, TEAM_STAT_GREEN_ARMOR );
		break;
	default:
		client->stats.shardPickups++;
		break;
	}

	if ( !armor_tiered.integer ) {
		*armor += ent->item->quantity;
	} else {
		switch ( ent->item->giTag ) {
		case ARMOR_TIER_RED:
			*armor += 150;
			if ( *armor > 200 ) {
				*armor = 200;
			}
			client->ps.stats[STAT_ARMOR_TIER] = 2;
			break;
		case ARMOR_TIER_YELLOW:
			*armor += 100;
			if ( *armor > 150 ) {
				*armor = 150;
			}
			client->ps.stats[STAT_ARMOR_TIER] = 1;
			break;
		case ARMOR_TIER_GREEN:
			*armor += 50;
			if ( *armor > 100 ) {
				*armor = 100;
			}
			client->ps.stats[STAT_ARMOR_TIER] = 0;
			break;
		default:
			if ( *armor <= 0 ) {
				client->ps.stats[STAT_ARMOR_TIER] = 0;
			}
			*armor += 2;
			break;
		}
	}
	if ( *armor > limit ) {
		*armor = limit;
	}

	if ( ent->item->giTag == ARMOR_TIER_RED ) {
		G_AnnouncePickup( other, ent, qfalse );
	}

	// retail mirrors the armor for observers (qagamex86 0x1004e803)
	other->s.armor = *armor;

	return RESPAWN_ARMOR_SECONDS;
}

//======================================================================

/*
===============
G_ItemHasTimer

The items whose timers spectators see: the timed ones except green armor
and the holdables (retail qagamex86 0x1004eb53).
===============
*/
static qboolean G_ItemHasTimer( const gitem_t *item ) {
	if ( !item || !item->timer || item->giType == IT_HOLDABLE ) {
		return qfalse;
	}
	return !( item->giType == IT_ARMOR && item->giTag == ARMOR_TIER_GREEN );
}


/*
===============
G_SendItemTimer

Retail qagamex86 0x1004eac0: tell one spectator when a timed item comes
back, which item and, on flag maps, which half it is on (3 for the only
one of its kind). Both updates and spectator catch-up pass zero for the
event's groundEntityNum marker (0x1004f4a3, 0x10050b9f, 0x1004ede7).
===============
*/
static void G_SendItemTimer( int clientNum, gentity_t *ent ) {
	gclient_t	*client = &level.clients[clientNum];
	gentity_t	*te;
	vec3_t		toRed, toBlue;
	float		red, blue;

	if ( client->pers.connected != CON_CONNECTED || !( client->ps.eFlags & EF_SPECTATOR )
		|| ( ent->flags & FL_DROPPED_ITEM ) ) {
		return;
	}
	if ( !G_ItemHasTimer( ent->item ) || ( ent->spawnflags & 0x100 ) ) {
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_ITEM_PICKUP_SPEC );
	te->s.time = ent->nextthink;
	te->s.otherEntityNum = ent->s.number;
	te->s.modelindex = ent->s.modelindex;
	te->s.modelindex2 = ent->team != NULL;
	te->r.svFlags |= SVF_SINGLECLIENT | SVF_BROADCAST;
	te->r.singleClient = g_entities[clientNum].s.number;
	te->s.groundEntityNum = 0;

	if ( g_gametype.integer != GT_CTF ) {
		return;
	}
	if ( level.timedItemCount[ent->item - bg_itemlist] < 2 ) {
		te->s.otherEntityNum2 = 3;
		return;
	}
	VectorSubtract( level.redFlagBase, ent->s.origin, toRed );
	VectorSubtract( level.blueFlagBase, ent->s.origin, toBlue );
	red = (float)( (double)toRed[1] * toRed[1] + (double)toRed[0] * toRed[0] + (double)toRed[2] * toRed[2] );
	blue = (float)( (double)toBlue[1] * toBlue[1] + (double)toBlue[0] * toBlue[0] + (double)toBlue[2] * toBlue[2] );
	if ( blue > red ) {
		te->s.otherEntityNum2 = 1;
		te->s.time2 = (int)red;
	} else {
		te->s.otherEntityNum2 = 2;
		te->s.time2 = (int)blue;
	}
}


/*
===============
G_BroadcastItemTimer

Retail qagamex86 0x1004ed10: every spectator hears about a timed item.
===============
*/
static void G_BroadcastItemTimer( gentity_t *ent ) {
	int i;

	if ( !g_specItemTimers.integer || !G_ItemHasTimer( ent->item ) ) {
		return;
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		G_SendItemTimer( i, ent );
	}
}


/*
===============
G_SendItemTimers

Retail qagamex86 0x1004ed70: a player who starts spectating learns every
timed item's state.
===============
*/
void G_SendItemTimers( int clientNum ) {
	gentity_t	*ent;
	int			i;

	if ( !g_specItemTimers.integer || !( level.clients[clientNum].ps.eFlags & EF_SPECTATOR ) ) {
		return;
	}
	for ( i = level.maxclients; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( ent->inuse && G_ItemHasTimer( ent->item ) ) {
			G_SendItemTimer( clientNum, ent );
		}
	}
}


/*
===============
G_InitItemSides

Retail qagamex86 0x10069190, at init: where the team flags spawn, and how
many timed items of each kind the map has.
===============
*/
void G_InitItemSides( void ) {
	gentity_t	*ent;
	int			i;

	VectorClear( level.redFlagBase );
	VectorClear( level.blueFlagBase );
	for ( ent = G_Find( NULL, FOFS( classname ), "team_CTF_redflag" ); ent; ent = G_Find( ent, FOFS( classname ), "team_CTF_redflag" ) ) {
		if ( !( ent->flags & FL_DROPPED_ITEM ) ) {
			VectorCopy( ent->s.origin, level.redFlagBase );
			break;
		}
	}
	for ( ent = G_Find( NULL, FOFS( classname ), "team_CTF_blueflag" ); ent; ent = G_Find( ent, FOFS( classname ), "team_CTF_blueflag" ) ) {
		if ( !( ent->flags & FL_DROPPED_ITEM ) ) {
			VectorCopy( ent->s.origin, level.blueFlagBase );
			break;
		}
	}

	memset( level.timedItemCount, 0, sizeof( level.timedItemCount ) );
	for ( i = 0; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( ent->inuse && G_ItemHasTimer( ent->item ) ) {
			level.timedItemCount[ent->item - bg_itemlist]++;
		}
	}
}


/*
===============
RespawnItem

Retail qagamex86 0x1004ee20: one of a team of items comes back at random;
spectators hear about timed ones.
===============
*/
void RespawnItem( gentity_t *ent ) {

	if ( !ent ) {
		return;
	}

	// randomly select from teamed entities
	if ( ent->team ) {
		gentity_t *master;
		int	count;
		int choice;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster" );
		}

		master = ent->teammaster;

		for ( count = 0, ent = master; ent; ent = ent->teamchain ) {
			// Skip disabled items
			if ( ent->tag == TAG_DONTSPAWN ) {
				continue;
			}
			count++;
		}

		// No valid items to spawn
		if ( count == 0 ) {
			return;
		}

		choice = rand() % count;

		for ( count = 0, ent = master; ent && count < choice; ent = ent->teamchain ) {
			// Skip disabled items
			if ( ent->tag == TAG_DONTSPAWN ) {
				continue;
			}
			count++;
		}
	}

	if ( !ent || ent->tag == TAG_DONTSPAWN ) {
		return;
	}

	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	ent->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( ent );

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if ( ent->speed ) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		} else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	if ( ent->item->giType == IT_HOLDABLE && ent->item->giTag == HI_KAMIKAZE ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/kamikazerespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	ent->itemSpawnTime = level.time;

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;

	G_BroadcastItemTimer( ent );
}


/*
===============
Touch_Item

Retail qagamex86 0x1004f020.  Nothing is taken during the countdown.  The
pickup functions return the respawn delay in seconds (0 to leave the item
in place, as weapons that stay are).  Under g_itemTimers a taken timed
item stays visible with its respawn time, for clients to count down.
===============
*/
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
	gclient_t	*client = other->client;
	int			respawn;
	qboolean	predict;

	if ( !client || other->health < 1 ) {
		return;		// dead people can't pickup
	}

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &client->ps, level.time, level.warmupTime,
			armor_tiered.integer, g_weaponRespawn.integer ) ) {
		return;
	}
	// The pre-match countdown suppresses map pickups.
	if ( level.warmupTime > 0 ) {
		return;
	}

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	predict = client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch( ent->item->giType ) {
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	case IT_PERSISTANT_POWERUP:
		respawn = Pickup_PersistantPowerup(ent, other);
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_KEY:
		client->ps.stats[STAT_KEY_MASK] |= ent->item->giTag;
		respawn = -1;
		break;
	default:
		return;
	}

	ent->itemPickups++;
	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}
	if ( !respawn ) {
		return;
	}

	// play the normal pickup sound
	if ( predict ) {
		G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	} else {
		G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	// Retail 0x1004f21a emits a carrier locator even for a subsequent item.
	if ( G_IsQuadHog() && client->ps.powerups[PW_QUAD] ) G_QuadHogPing( other );
	if ( g_gametype.integer == GT_1FCTF && client->ps.powerups[PW_NEUTRALFLAG] ) G_NeutralFlagPing( other );
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM ) {
		gentity_t	*te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
		te->s.eventParm = ent->s.modelindex;
		te->s.otherEntityNum = other->s.number;
		// if we want the global sound to play
		if ( !ent->speed ) {
			te->r.svFlags |= SVF_BROADCAST;
		} else {
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	// fire item targets
	G_UseTargets (ent, other);

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if ( ent->wait != 0 ) {
		respawn = (int)ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random != 0 ) {
		respawn = (int)( G_CRandom() * ent->random + respawn );
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}
	if ( bot_gauntletOnly.integer ) {
		respawn += bot_itemDelayTime.integer;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if ( respawn > 0 ) {
		if ( g_itemTimers.integer && ent->item->timer ) {
			ent->s.time2 = respawn * 1000;
			ent->s.time = level.time + respawn * 1000;
			ent->flags |= FL_ITEM_TIMER;
			ent->r.svFlags &= ~SVF_NOCLIENT;
			ent->s.generic1 = ent->team != NULL;
		}
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	} else {
		ent->nextthink = 0;
		ent->think = 0;
	}

	G_BroadcastItemTimer( ent );
	trap_LinkEntity( ent );
}


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward.  Retail qagamex86 0x1004f4f0: a
dropped flag is as tall as a placed one, and a dropped item disappears
after 30 seconds unless it is a flag.
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t	*dropped;
	const qboolean flag = item->giType == IT_TEAM && item->giTag >= PW_REDFLAG && item->giTag <= PW_NEUTRALFLAG;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, flag ? 35 : ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->flags = FL_DROPPED_ITEM;

	if ( ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF ) && item->giType == IT_TEAM ) {
		// Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem( dropped );
	} else if ( g_gametype.integer == GT_ATTACK_DEFEND && item->giType == IT_TEAM ) {
		// Retail 0x1004f6b7: keep the dropped flag until the turn resets.
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = 0;
		Team_CheckDroppedItem( dropped );
	} else if ( item->giType == IT_PERSISTANT_POWERUP ) {
		dropped->think = G_RespawnRune; // retail 0x1004f6ce
		dropped->nextthink = level.time + 30000;
	} else { // Retail 0x1004f6ec: the health-drop rule shortens all ordinary drops.
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + ( g_dropDamagedHealth.integer ? 10000 : 30000 );
	}

	trap_LinkEntity (dropped);

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward.  Retail qagamex86 0x1004f740 throws
it level along the yaw plus angle, evaluating the toss in x87 double
precision: 150 units a second forward and 200 +- 50 up.
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	float	yaw;

	yaw = (float)( ( (double)ent->s.apos.trBase[YAW] + angle ) * ITEM_DEG2RAD );
	velocity[0] = (float)( (double)(float)cos( yaw ) * 150.0 );
	velocity[1] = (float)( (double)(float)sin( yaw ) * 150.0 );
	velocity[2] = (float)( G_CRandom() * 50.0 + 200.0 );

	if ( G_IsQuadHog() && item->giType == IT_POWERUP && item->giTag == PW_QUAD ) {
		return G_LaunchQuadHog( item, ent->s.pos.trBase, velocity );
	}
	return LaunchItem( item, ent->s.pos.trBase, velocity );
}

// Retail 0x1004f8c0: convert actual health lost into rings of 25s, then 5s.
// The angle deliberately uses the victim's position Y, not their view yaw.
// Both the position offset and the 200/225 toss are visible in retail drops.
void G_DropDamagedHealth( gentity_t *victim, gentity_t *attacker, int damage ) {
	const int amounts[2] = { 25, 5 };
	const char *names[2] = { "25 Health", "5 Health" };
	int group, count, n, axis;
	float angle, yaw;
	vec3_t velocity, origin;
	gitem_t *item;

	if ( !g_dropDamagedHealth.integer || !victim || !victim->client || damage <= 0
		|| ( victim == attacker && ( g_dmflags.integer & DF_NO_SELF_HEALTH_DAMAGE ) ) ) return;

	for ( group = 0; group < 2 && damage > 0; group++ ) {
		count = damage / amounts[group];
		damage %= amounts[group];
		// Retail consumes the small-ring seed even when the remainder is <5.
		if ( !count && group == 0 ) continue;
		angle = (float)( rand() % 360 );
		item = BG_FindItem( names[group] );
		if ( !item ) continue;
		for ( n = 0; n < count; n++ ) {
			yaw = (float)( (double)(float)( victim->s.pos.trBase[1] + (double)angle ) * ITEM_DEG2RAD );
			velocity[0] = (float)( (double)(float)cos( yaw ) * 200.0 );
			velocity[1] = (float)( (double)(float)sin( yaw ) * 200.0 );
			velocity[2] = 225.0f;
			for ( axis = 0; axis < 3; axis++ ) {
				origin[axis] = (float)( victim->s.pos.trBase[axis] + (double)velocity[axis] * 0.2 );
			}
			// Avoid exhausting the entity pool on malformed health settings.
			if ( level.num_entities >= ENTITYNUM_MAX_NORMAL && !G_EntitiesFree() ) return;
			LaunchItem( item, origin, velocity );
			angle = (float)( (double)angle + 360 / count );
		}
	}
}

/*
================
G_ResetKey

Retail qagamex86 0x1004d570: a key whose carrier died goes back to its
spawn, and any dropped copy of it disappears.
================
*/
void G_ResetKey( int key ) {
	const char	*classname;
	gentity_t	*ent;

	switch ( key ) {
	case KEY_SILVER:
		classname = "item_key_silver";
		break;
	case KEY_GOLD:
		classname = "item_key_gold";
		break;
	case KEY_MASTER:
		classname = "item_key_master";
		break;
	default:
		return;
	}

	for ( ent = G_Find( NULL, FOFS( classname ), classname ); ent; ent = G_Find( ent, FOFS( classname ), classname ) ) {
		if ( ent->flags & FL_DROPPED_ITEM ) {
			trap_UnlinkEntity( ent );
			G_FreeEntity( ent );
		} else {
			RespawnItem( ent );
		}
	}
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;

	// retail (qagamex86 0x100503f0): flags stand 35 units tall
	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS,
		( ent->item->giType == IT_TEAM && ent->item->giTag >= PW_REDFLAG && ent->item->giTag <= PW_NEUTRALFLAG )
		? 35 : ITEM_RADIUS );

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// using an item causes it to respawn
	ent->use = Use_Item;

	if ( !( ent->spawnflags & 1 ) ) {
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_Printf( "FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos( ent->s.origin ) );
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		VectorCopy( tr.endpos, ent->s.origin );
	}

	// retail rests every item, suspended or dropped, on whole units, a unit up
	ent->s.origin[0] = (int)ent->s.origin[0];
	ent->s.origin[1] = (int)ent->s.origin[1];
	ent->s.origin[2] = (int)(float)( ent->s.origin[2] + 1.0 );
	G_SetOrigin( ent, ent->s.origin );

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// powerups and keys first appear after the delays drawn at worldspawn,
	// except during warmup on a training map
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_KEY ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		if ( !level.trainingMap || !level.warmupTime ) {
			ent->nextthink = level.time + 1000 *
				( ent->item->giType == IT_POWERUP ? level.powerupSpawnDelay : level.keySpawnDelay );
			ent->think = RespawnItem;
		}
		return;
	}

	ent->itemSpawnTime = level.time;
	trap_LinkEntity( ent );
}


qboolean	itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

	// Set up team stuff
	Team_InitGame();

	if( g_gametype.integer == GT_CTF ) {
		gitem_t	*item;

		// check for the two flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n" );
		}
	}
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_1FCTF ) {
		gitem_t	*item;

		// check for all three flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n" );
		}
		item = BG_FindItem( "Neutral Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_neutralflag in map\n" );
		}
	}

	if( g_gametype.integer == GT_OBELISK ) {
		gentity_t	*ent;

		// check for the two obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n" );
		}
	}

	if( g_gametype.integer == GT_HARVESTER ) {
		gentity_t	*ent;

		// check for all three obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_neutralobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_neutralobelisk in map\n" );
		}
	}
#endif
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	int rune;
	memset( itemRegistered, 0, sizeof( itemRegistered ) );
	// Retail ClearRegisteredItems 0x1004ffd0 precaches dynamic health drops.
	if ( g_dropDamagedHealth.integer ) {
		RegisterItem( BG_FindItem( "25 Health" ) );
		RegisterItem( BG_FindItem( "5 Health" ) );
	}
	// Retail 0x100500ba registers the dynamic objective before CS_ITEMS.
	if ( G_IsQuadHog() ) RegisterItem( BG_FindItemForPowerup( PW_QUAD ) );
	// Retail 0x1005011f precaches all four runes before dynamic spawning.
	if ( g_runes.integer || trap_Cvar_VariableValue( "com_build" ) != 0.0f ) {
		for ( rune = PP_SCOUT; rune <= PP_ARMORREGEN; rune++ ) {
			RegisterItem( BG_FindItemForPersistantPowerup( rune ) );
		}
	}

	// retail registers the base weapons when a player spawns, after
	// CS_ITEMS is written (G_GiveSpawnLoadout)
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_HARVESTER ) {
		RegisterItem( BG_FindItem( "Red Skull" ) );
		RegisterItem( BG_FindItem( "Blue Skull" ) );
	}
#endif
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

	G_Printf( "%i items registered\n", count );
	trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled( gitem_t *item ) {

	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue( name );
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
static qboolean G_ItemAllowed( const gitem_t *item ) {
	// Retail 0x1005084d keeps only flag objectives under instagib. Arena,
	// race, domination and red rover suppress ordinary map pickups too
	// (0x100508ad). Dynamic drops have their own creation path.
	if ( g_instaGib.integer ) {
		return item->giType == IT_TEAM && item->giTag >= PW_REDFLAG && item->giTag <= PW_NEUTRALFLAG;
	}
	if ( g_gametype.integer == GT_RACE || g_gametype.integer == GT_CLAN_ARENA
		|| g_gametype.integer == GT_DOMINATION || g_gametype.integer == GT_RED_ROVER ) {
		return qfalse;
	}
	switch ( item->giType ) {
	case IT_WEAPON:
		return g_spawnItemWeapons.integer != 0;
	case IT_AMMO:
		if ( !g_spawnItemAmmo.integer || g_infiniteAmmo.integer ) {
			return qfalse;
		}
		if ( g_ammoPackHack.integer ) {
			return qtrue;	// every ammo box becomes an ammo pack below
		}
		// g_ammoPack (FnQL's engine, like retail's, defaults it to 1)
		// chooses between the map's ammo packs and its ammo boxes
		return g_ammoPack.integer ? item->giTag == WP_NUM_WEAPONS : item->giTag != WP_NUM_WEAPONS;
	case IT_ARMOR:
		return g_spawnItemArmor.integer != 0;
	case IT_HEALTH:
		return g_spawnItemHealth.integer != 0;
	case IT_POWERUP:
		return g_spawnItemPowerup.integer && ( g_gametype.integer != GT_FFA || !g_quadHog.integer );
	case IT_HOLDABLE:
		return g_spawnItemHoldable.integer != 0;
	default:
		return qtrue;
	}
}

/*
Retail G_SpawnItem (qagamex86 0x100507e0): the g_spawnItem* switches, the
ammo pack choice and disable_<classname> decide whether the item spawns;
only spawned items are registered.
*/
static void G_SpawnItemInternal( gentity_t *ent, gitem_t *item, qboolean force ) {
	char *gametypes;
	qboolean registerItem = qtrue;
	// Retail 0x10050801: an explicit Race inclusion overrides the normal
	// pickup suppression, including disable_<item> and the factory switches.
	if ( !force && g_gametype.integer == GT_RACE && G_SpawnString( "gametype", "", &gametypes )
		&& ( G_SpawnGametypeMask( gametypes ) & ( 1 << ( GT_RACE + 1 ) ) ) ) force = qtrue;

	if ( !force && ( !G_ItemAllowed( item ) || G_ItemDisabled( item ) ) ) {
		ent->tag = TAG_DONTSPAWN;
		return;
	}

	// retail (qagamex86 0x100509b7) publishes where each team flag stands
	if ( !force && item->giType == IT_TEAM && !ent->s.modelindex2 && ( item->giTag == PW_REDFLAG || item->giTag == PW_BLUEFLAG ) ) {
		trap_SetConfigstring( item->giTag == PW_REDFLAG ? CS_RED_FLAG_ORIGIN : CS_BLUE_FLAG_ORIGIN,
			va( "\\x\\%f\\y\\%f\\z\\%f", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] ) );
	}

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	if ( g_ammoPackHack.integer && item->giType == IT_AMMO ) {
		if ( item->giTag == WP_NUM_WEAPONS ) {
			if ( !force ) {
				ent->tag = TAG_DONTSPAWN;
				return;
			}
			// Retail 0x10050ab7 still assigns the item for forced giving,
			// without registering an already-pack-shaped ammo item again.
			registerItem = qfalse;
		}
		item = BG_FindItem( "Ammo Pack" );
	}

	if ( registerItem ) RegisterItem( item );

	ent->item = item;
	// some movers spawn on the second frame, so delay item spawns two
	// server frames so they can ride trains (retail qagamex86 0x10050b0a
	// counts sv_fps frames, not baseq3a's two FRAMETIMEs)
	ent->nextthink = level.time + 2 * (int)( 1000.0 / ( sv_fps.integer > 0 ? sv_fps.integer : 40 ) );
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;		// items are bouncy

	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );
		G_SpawnFloat( "noglobalsound", "0", &ent->speed );
	}

#ifdef MISSIONPACK
	if ( item->giType == IT_PERSISTANT_POWERUP ) {
		ent->s.generic1 = ent->spawnflags;
	}
#endif
	// Retail 0x10050ba1 also reports dynamically spawned timed items before
	// FinishSpawningItem; spectators must see both this and the later pickup.
	G_BroadcastItemTimer( ent );
}

void G_SpawnItem( gentity_t *ent, gitem_t *item ) {
	G_SpawnItemInternal( ent, item, qfalse );
}

void G_SpawnItemForced( gentity_t *ent, gitem_t *item ) {
	// Retail 0x1003bbf3 passes force=1 to 0x100507e0 for the give cheat.
	G_SpawnItemInternal( ent, item, qtrue );
}


/*
================
G_BounceItem

Retail qagamex86 0x10050c00: the velocity is taken at level.time less the
trace's fraction of a frame (Quake III counted the fraction from
previousTime instead), and the reflection runs at x87 precision, here in
double.  An item that lands on a floor slowly enough stops one unit up,
its origin truncated to whole units.
================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	double	reflect;
	int		hitTime, i;

	// reflect the velocity on the trace plane
	hitTime = (int)( (double)level.time - (double)trace->fraction * (double)level.msec );
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	reflect = (double)(float)( (double)trace->plane.normal[0] * velocity[0]
		+ (double)trace->plane.normal[1] * velocity[1]
		+ (double)trace->plane.normal[2] * velocity[2] ) * -2.0;
	for ( i = 0; i < 3; i++ ) {
		ent->s.pos.trDelta[i] = (float)( (double)velocity[i] + (double)trace->plane.normal[i] * reflect );
	}

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_RunItem

Retail qagamex86 0x10050de0: the item thinks first, then falls or slides
and bounces.  An item that comes to rest in a nodrop volume is removed; a
team flag goes home. A persistent rune relocates to another eligible spawn
point instead (retail qagamex86 0x1004da10).
================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if its groundentity has been set to ENTITYNUM_NONE, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	// check think function
	G_RunThink( ent );

	if ( !ent->inuse || ent->s.pos.trType == TR_STATIONARY ) {
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, 
		ent->r.ownerNum, mask );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );	// FIXME: avoid this for stationary?

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if (ent->item && ent->item->giType == IT_TEAM) {
			Team_FreeEntity(ent);
		} else if ( ent->item && ent->item->giType == IT_PERSISTANT_POWERUP ) {
			G_RespawnRune( ent ); // retail 0x10050f27
		} else {
			G_FreeEntity( ent );
		}
		return;
	}

	G_BounceItem( ent, &tr );
}
