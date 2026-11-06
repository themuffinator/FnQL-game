// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include <limits.h>


/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// Retail 0x100338dd-0x10033923 quantizes pain to four health bands.
	// The damage sequence advances on every hit, even while sound is muted.
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		const int pain = player->health < 25 ? 20 : player->health < 50 ? 40 : player->health < 75 ? 60 : 80;
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, pain );
	}
	client->ps.damageEvent++;


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean	envirosuit;
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
#ifdef MISSIONPACK
	if( ent->s.eFlags & EF_TICKING ) {
		ent->client->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
	else
#endif
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time, g_itemHeight.integer ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	// retail qagamex86 0x10033e5a: spectators are never linked, and one that
	// was following before it respawned follows again, scoreboard and all
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	if ( client->followPending ) {
		client->sess.spectatorClient = client->followPendingClient;
		if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
			DeathmatchScoreboardMessage( ent );
		}
		client->sess.spectatorState = SPECTATOR_FOLLOW;
		client->followPending = qfalse;
	}

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 480;	// retail qagamex86 0x10033ecc

		// set up for pmove
		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		if ( client->noclip )
			pm.tracemask = 0;
		else
			pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;
		pm.settings = &level.pmoveSettings;

		// perform a pmove
		Pmove( &pm );
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack cycles through the players; the 0x800 button frees a
	// spectator, or a team player under g_teamSpecFreeCam 1 (retail
	// qagamex86 0x10033f5c)
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		G_FollowCycle( ent, 1 );
		return;
	}
	if ( ( client->buttons & 0x800 ) && !( client->oldbuttons & 0x800 ) ) {
		if ( client->sess.sessionTeam == TEAM_SPECTATOR
			|| ( g_teamSpecFreeCam.integer == 1 && client->sess.spectatorState == SPECTATOR_FOLLOW ) ) {
			StopFollowing( ent );
		}
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	client = ent->client;
	client->timeResidual += msec;
	// Retail 100343f3 supplies Race's millisecond clock too. Saturate an
	// unrepresentable run so it cannot wrap into a new personal best.
	if ( g_gametype.integer == GT_RACE ) {
		long long elapsed = (long long)client->flagCarryTime + msec;
		client->flagCarryTime = elapsed > INT_MAX ? INT_MAX : elapsed < 0 ? 0 : (int)elapsed;
	} else client->flagCarryTime += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

		// team item control: the seconds each powerup and a flag is held
		// (retail qagamex86 0x10034416)
		if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_CLAN_ARENA ) {
			const int team = client->sess.sessionTeam;

			if ( client->ps.powerups[PW_QUAD] ) {
				level.teamStats[TEAM_STAT_QUAD_TIME][team]++;
			}
			if ( client->ps.powerups[PW_BATTLESUIT] ) {
				level.teamStats[TEAM_STAT_BATTLESUIT_TIME][team]++;
			}
			if ( client->ps.powerups[PW_REGEN] ) {
				level.teamStats[TEAM_STAT_REGEN_TIME][team]++;
			}
			if ( client->ps.powerups[PW_HASTE] ) {
				level.teamStats[TEAM_STAT_HASTE_TIME][team]++;
			}
			if ( client->ps.powerups[PW_INVIS] ) {
				level.teamStats[TEAM_STAT_INVIS_TIME][team]++;
			}
			if ( client->ps.powerups[PW_REDFLAG] || client->ps.powerups[PW_BLUEFLAG] || client->ps.powerups[PW_NEUTRALFLAG] ) {
				level.teamStats[TEAM_STAT_FLAG_TIME][team]++;
			}
		}

		// Retail 0x100344d3: only Regeneration heals, not the Guard rune.
		if ( client->ps.powerups[PW_REGEN] ) {
			if ( ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 15;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2) {
				ent->health += 5;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
		} else {
			// count down health when over max
			if ( ent->health > client->ps.stats[STAT_MAX_HEALTH]
				&& (!G_IsRoundBasedGametype(g_gametype.integer) || (level.roundState == 3 && !level.roundDeadline)) ) {
				ent->health--;
			}
		}

		// QL's Armor Regen restores armor, not Team Arena's ammunition
		// (retail 0x10034601). Tiered armor permits the last increment past max.
		if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_ARMORREGEN
			&& client->ps.stats[STAT_ARMOR] < client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR] += 10;
			if ( !armor_tiered.integer && client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
				client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH];
			}
			ent->s.armor = client->ps.stats[STAT_ARMOR];
			G_AddEvent( ent, EV_POWERUP_ARMORREGEN, 0 );
		}

		// Scout has its own faster decay below. Retail 0x100346b6 also
		// distinguishes tiered armor and Clan Arena's armor countdown.
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH]
			&& ( g_gametype.integer == GT_CLAN_ARENA ? armor_tiered.integer
				: !armor_tiered.integer && client->ps.stats[STAT_PERSISTANT_POWERUP] != PP_SCOUT ) ) {
			client->ps.stats[STAT_ARMOR]--;
			ent->s.armor = client->ps.stats[STAT_ARMOR];
		}
	}
	// Retail 0x100347d6: Scout loses one armor every 500 ms down to 50.
	G_QuadHogTimer( ent, msec );
	G_NeutralFlagTimer( ent, msec );
	// Its residual is retained between rune pickups, and pauses freeze it.
	if ( g_gametype.integer != GT_CLAN_ARENA && client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_SCOUT ) {
		client->scoutArmorResidual += msec;
		while ( client->scoutArmorResidual >= 500 ) {
			client->scoutArmorResidual -= 500;
			if ( client->ps.stats[STAT_ARMOR] > 50 ) {
				client->ps.stats[STAT_ARMOR]--;
			}
		}
	}
	// Retail 0x10034260 / 0x100342f0: independent residuals, armed by
	// damage, with armor optionally waiting for health. Keep the pre-tick
	// cap check (a tick may overshoot); reject nonpositive rates safely.
	if ( g_regenHealth.integer && g_regenHealthRate.integer > 0 ) {
		if ( ent->health >= client->ps.stats[STAT_MAX_HEALTH] ) client->healthRegenActive = qfalse;
		if ( client->healthRegenActive && client->regenDamageTime
			&& (long long)level.time - client->regenDamageTime > g_regenHealth.integer ) client->healthRegenResidual += msec;
		ent->health += client->healthRegenResidual / g_regenHealthRate.integer;
		client->healthRegenResidual %= g_regenHealthRate.integer;
	}
	if ( g_regenArmor.integer && g_regenArmorRate.integer > 0 ) {
		int cap = armor_tiered.integer ? 50 * (client->ps.stats[STAT_ARMOR_TIER] + 1) : client->ps.stats[STAT_MAX_HEALTH];
		if ( client->ps.stats[STAT_ARMOR] >= cap ) client->armorRegenActive = qfalse;
		if ( client->armorRegenActive && (long long)level.time - client->regenDamageTime > g_regenArmor.integer
			&& (!g_regenArmorAfterHealth.integer || ent->health >= client->ps.stats[STAT_MAX_HEALTH]) ) client->armorRegenResidual += msec;
		if ( client->armorRegenResidual >= g_regenArmorRate.integer ) {
			client->ps.stats[STAT_ARMOR] += client->armorRegenResidual / g_regenArmorRate.integer;
			client->armorRegenResidual %= g_regenArmorRate.integer;
			ent->s.armor = client->ps.stats[STAT_ARMOR];
		}
	}
}

/*
====================
ClientIntermissionThink

Retail qagamex86 0x10034d1f: at the intermission a press of attack or use
toggles the player's ready flag, which the scoreboard shows; the player
state, its command time included, stays as the intermission left it.
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		client->pers.readyToPlay ^= 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	origin, angles;
//	qboolean	fired;
	gitem_t *item;
	gentity_t *drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_FAR ) {
				damage = 10;
			} else {
				damage = 5;
			}
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
			FireWeapon( ent );
			break;

		case EV_USE_ITEM1:		// teleporter
			// drop flags in CTF
			item = NULL;
			j = 0;

			if ( ent->client->ps.powerups[ PW_REDFLAG ] ) {
				item = BG_FindItemForPowerup( PW_REDFLAG );
				j = PW_REDFLAG;
			} else if ( ent->client->ps.powerups[ PW_BLUEFLAG ] ) {
				item = BG_FindItemForPowerup( PW_BLUEFLAG );
				j = PW_BLUEFLAG;
			} else if ( ent->client->ps.powerups[ PW_NEUTRALFLAG ] ) {
				item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
				j = PW_NEUTRALFLAG;
			}

			if ( item ) {
				drop = Drop_Item( ent, item, 0 );
				// decide how many seconds it has left
				drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				// for pickup prediction
				drop->s.time2 = drop->count;

				ent->client->ps.powerups[ j ] = 0;
			}

#ifdef MISSIONPACK
			if ( g_gametype.integer == GT_HARVESTER ) {
				if ( ent->client->ps.generic1 > 0 ) {
					if ( ent->client->sess.sessionTeam == TEAM_RED ) {
						item = BG_FindItem( "Blue Skull" );
					} else {
						item = BG_FindItem( "Red Skull" );
					}
					if ( item ) {
						for ( j = 0; j < ent->client->ps.generic1; j++ ) {
							drop = Drop_Item( ent, item, 0 );
							if ( ent->client->sess.sessionTeam == TEAM_RED ) {
								drop->spawnflags = TEAM_BLUE;
							} else {
								drop->spawnflags = TEAM_RED;
							}
						}
					}
					ent->client->ps.generic1 = 0;
				}
			}
#endif
			SelectSpawnPoint( ent, ent->client->ps.origin, origin, angles );
			TeleportPlayer( ent, origin, angles );
			break;

		case EV_USE_ITEM2:		// medkit
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

			break;

#ifdef MISSIONPACK
		case EV_USE_ITEM3:		// kamikaze
			// make sure the invulnerability is off
			ent->client->invulnerabilityTime = 0;
			// start the kamikze
			G_StartKamikaze( ent );
			break;

		case EV_USE_ITEM4:		// portal
			if( ent->client->portalID ) {
				DropPortalSource( ent );
			}
			else {
				DropPortalDestination( ent );
			}
			break;
		case EV_USE_ITEM5:		// invulnerability
			ent->client->invulnerabilityTime = level.time + 10000;
			break;
#endif

		default:
			break;
		}
	}

}

#ifdef MISSIONPACK
/*
==============
StuckInOtherClient
==============
*/
static int StuckInOtherClient(gentity_t *ent) {
	int i;
	gentity_t	*ent2;

	ent2 = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent2++ ) {
		if ( ent2 == ent ) {
			continue;
		}
		if ( !ent2->inuse ) {
			continue;
		}
		if ( !ent2->client ) {
			continue;
		}
		if ( ent2->health <= 0 ) {
			continue;
		}
		//
		if (ent2->r.absmin[0] > ent->r.absmax[0])
			continue;
		if (ent2->r.absmin[1] > ent->r.absmax[1])
			continue;
		if (ent2->r.absmin[2] > ent->r.absmax[2])
			continue;
		if (ent2->r.absmax[0] < ent->r.absmin[0])
			continue;
		if (ent2->r.absmax[1] < ent->r.absmin[1])
			continue;
		if (ent2->r.absmax[2] < ent->r.absmin[2])
			continue;
		return qtrue;
	}
	return qfalse;
}
#endif

void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	} else
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	}

	// unlagged
	client->frameOffset = trap_Milliseconds() - level.frameStartTime;
	client->lastCmdTime = ucmd->serverTime;
	client->lastUpdateFrame = level.framenum;

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// a timeout freezes everyone: the command time moves on and nothing
	// else happens (retail qagamex86 0x10034d66)
	if ( level.timeoutStartTime ) {
		client->ps.pm_flags |= PMF_NO_MOVE;
		client->ps.commandTime = ucmd->serverTime;
		return;
	}

	// a deferred spawn tries again once its time has passed (retail
	// qagamex86 0x10034d79); until then the player flies about as a
	// spectator, unlinked
	if ( client->deferredSpawnTime && (g_gametype.integer != GT_ATTACK_DEFEND
		|| client->deferredSpawnTime < level.time || client->ps.pm_type == PM_SPECTATOR) ) {
		if ( client->deferredSpawnTime < level.time ) {
			client->deferredSpawnTime = 0;
			client->ps.pm_type = PM_NORMAL;
			// Retail 0x10034dd0 re-enters AD's round-aware spawn dispatch.
			if ( g_gametype.integer == GT_ATTACK_DEFEND ) G_RoundSpawn(ent);
			else ClientSpawn( ent );
		} else {
			pmove_t pm;

			client->ps.pm_type = PM_SPECTATOR;
			client->ps.speed = 480;
			memset( &pm, 0, sizeof( pm ) );
			pm.ps = &client->ps;
			pm.cmd = *ucmd;
			pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
			pm.trace = trap_Trace;
			pm.pointcontents = trap_PointContents;
			pm.settings = &level.pmoveSettings;
			Pmove( &pm );
			VectorCopy( client->ps.origin, ent->s.origin );
			VectorCopy( client->ps.origin, ent->r.currentOrigin );
			trap_UnlinkEntity( ent );
		}
		return;
	}

	// Retail 0x10034e20 consumes the previous pmove's selected primary
	// weapon before this command is simulated. It becomes the next spawn's
	// loadout, including choices made while dead or spectating.
	if ( client->ps.weaponPrimary ) client->sess.loadoutWeapon = client->ps.weaponPrimary;

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR || client->ps.pm_type == PM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	// clear the rewards if time
	if ( level.time > client->rewardTime ) {
		client->ps.eFlags &= ~EF_AWARDS;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.powerups[PW_FROZEN] ) {
		client->ps.pm_type = PM_FREEZE;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = g_gametype.integer == GT_ATTACK_DEFEND && level.roundState == 1 ? PM_FREEZE : PM_NORMAL;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	// Retail 0x10034ee5: Haste takes precedence over Scout's 1.25 scale.
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	} else if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_SCOUT ) {
		client->ps.speed = (int)( (double)client->ps.speed * 1.25 );
	}

	if ( g_gametype.integer == GT_RED_ROVER && g_rrInfected.integer && client->sess.sessionTeam == TEAM_RED ) client->ps.speed = (int)((float)client->ps.speed * g_rrInfectedZombieSpeed.value);

	// Let go of the hook if we aren't firing
	if ( client->ps.weapon == WP_GRAPPLING_HOOK &&
		client->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(client->hook);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if ( client->ps.weapon == WP_GAUNTLET && !( ucmd->buttons & BUTTON_TALK ) &&
		( ucmd->buttons & BUTTON_ATTACK ) && client->ps.weaponTime <= 0 ) {
		pm.gauntletHit = CheckGauntletAttack( ent );
	}

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

#ifdef MISSIONPACK
	// check for invulnerability expansion before doing the Pmove
	if (client->ps.powerups[PW_INVULNERABILITY] ) {
		if ( !(client->ps.pm_flags & PMF_INVULEXPAND) ) {
			vec3_t mins = { -42, -42, -42 };
			vec3_t maxs = { 42, 42, 42 };
			vec3_t oldmins, oldmaxs;

			VectorCopy (ent->r.mins, oldmins);
			VectorCopy (ent->r.maxs, oldmaxs);
			// expand
			VectorCopy (mins, ent->r.mins);
			VectorCopy (maxs, ent->r.maxs);
			trap_LinkEntity(ent);
			// check if this would get anyone stuck in this player
			if ( !StuckInOtherClient(ent) ) {
				// set flag so the expanded size will be set in PM_CheckDuck
				client->ps.pm_flags |= PMF_INVULEXPAND;
			}
			// set back
			VectorCopy (oldmins, ent->r.mins);
			VectorCopy (oldmaxs, ent->r.maxs);
			trap_LinkEntity(ent);
		}
	}
#endif

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;

	pm.settings = &level.pmoveSettings;
	// Retail 0x10035190: latching a player quarters the owner's pull.
	pm.grappleSlow = client->hook && client->hook->enemy && client->hook->enemy->client;
	// Retail 0x1003517e: dmflags bit 32 suppresses surface footsteps.
	pm.noFootsteps = (g_dmflags.integer & DF_NO_FOOTSTEPS) != 0;

	VectorCopy( client->ps.origin, client->oldOrigin );

#ifdef MISSIONPACK
		if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
			if ( level.time - level.intermissionQueued >= 1000  ) {
				pm.cmd.buttons = 0;
				pm.cmd.forwardmove = 0;
				pm.cmd.rightmove = 0;
				pm.cmd.upmove = 0;
				if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
					trap_SendConsoleCommand( EXEC_APPEND, "centerview\n");
				}
				ent->client->ps.pm_type = PM_INTERMISSION;
			}
		}
		Pmove (&pm);
#else
		Pmove (&pm);
#endif

	// save results of pmove
	if ( client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	SendPendingPredictableEvents( &client->ps );

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestAAS( ent->r.currentOrigin );

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if ( client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	if ( client->ps.powerups[PW_FROZEN] ) {
		G_FreezeThink(ent, msec);
		return;
	}

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// retail qagamex86 0x10039f20: once the respawn time has come, attack
		// or use respawns, and g_respawn_delay_max later the respawn is
		// forced; during a round mode's countdown the dead come straight
		// back. Clan Arena spectators wait for the next round.
		if ( level.time >= client->respawnTime ) {
			if ( (g_gametype.integer == GT_CLAN_ARENA || g_gametype.integer == GT_ATTACK_DEFEND) && !level.warmupTime && !G_RoundCountingDown() ) {
				CopyToBodyQue(ent);
				client->ps.pm_type = PM_SPECTATOR;
				ClientSpawn(ent);
				G_FollowNextOrStop(ent);
				return;
			}
			qboolean countdown = qfalse;

			switch ( g_gametype.integer ) {
			case GT_CLAN_ARENA:
			case GT_FREEZE:
			case GT_ATTACK_DEFEND:
			case GT_RED_ROVER:
				countdown = level.warmupTime > 0;
				break;
			default:
				break;
			}
			if ( ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) )
				|| level.time - client->respawnTime > g_respawn_delay_max.integer || countdown ) {
				respawn( ent );
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );
}


/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
#if 0 // unlagged
	ent->client->lastCmdTime = level.time;
#endif

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

Retail qagamex86 0x10035470: a follower takes the followed player's state
but stays a spectator (pm_type PM_SPECTATOR) and keeps its own ping; a
spectator-team follower is marked EF_SPECTATOR, while a team player
watching a teammate keeps its own persistant counters but the followed
player's hit count.  When a chosen player can no longer be followed, the
follower moves on or floats free.  (Quake III's scoreboard spectating is
gone.)
==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	gclient_t	*cl;
	int			clientNum, ping;
	int			persistant[MAX_PERSISTANT];

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}

	// team follow1 and team follow2 go to whatever clients are playing
	clientNum = client->sess.spectatorClient;
	if ( clientNum == -1 ) {
		clientNum = level.follow1;
	} else if ( clientNum == -2 ) {
		clientNum = level.follow2;
	}

	if ( clientNum >= 0 ) {
		cl = &level.clients[clientNum];
		if ( cl->pers.connected == CON_CONNECTED && cl->ps.pm_type != PM_SPECTATOR ) {
			ping = client->ps.ping;
			memcpy( persistant, client->ps.persistant, sizeof( persistant ) );
			client->ps = cl->ps;
			client->ps.pm_type = PM_SPECTATOR;
			client->ps.pm_flags |= PMF_FOLLOW;
			client->ps.ping = ping;
			if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				client->ps.eFlags |= EF_SPECTATOR;
				return;
			}
			memcpy( client->ps.persistant, persistant, sizeof( persistant ) );
			client->ps.persistant[PERS_HITS] = cl->ps.persistant[PERS_HITS];
			return;
		}
	}

	if ( client->sess.spectatorClient >= 0 ) {
		G_FollowNextOrStop( ent );
	}
}


/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	static gentity_t sent;
	int			i;
	gclient_t	*client;
	// unlagged
	int			frames;

	if ( !ent->client )
		return;

	ent->r.svFlags &= ~svf_self_portal2;

	// spectators, and players waiting out a deferred spawn, as in retail
	// (qagamex86 0x1003560b, which tests the spectator pm_type)
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->ps.pm_type == PM_SPECTATOR || ent->client->deferredSpawnTime ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	client = ent->client;

	// turn off any expired powerups by the level clock, unless the player is
	// held in place (retail qagamex86 0x10035640); runes never expire, they
	// are held at 0x7fffffff until death
	if ( !( client->ps.pm_flags & PMF_NO_MOVE ) ) {
		for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
			if ( client->ps.powerups[ i ] < level.time ) {
				client->ps.powerups[ i ] = 0;
			}
		}
	}

#ifdef MISSIONPACK
	if ( ent->client->invulnerabilityTime > level.time ) {
		ent->client->ps.powerups[PW_INVULNERABILITY] = level.time;
	}
#endif

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// team modes keep each player's location current (retail qagamex86
	// 0x100356de)
	if ( g_gametype.integer >= GT_TEAM ) {
		gentity_t *loc = Team_GetLocation( ent );

		client->ps.location = ent->s.location = loc ? loc->health : 0;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...
	ent->s.health = ent->health;	// retail ClientEndFrame (qagamex86 0x10035723)

	G_SetClientSound( ent );

	// set the latest info
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	SendPendingPredictableEvents( &client->ps );

	client->ps.eFlags &= ~EF_CONNECTION;
	ent->s.eFlags &= ~EF_CONNECTION;

	frames = level.framenum - client->lastUpdateFrame - 1;

	// PVS prediction
	if ( g_predictPVS.integer && svf_self_portal2 ) {
		int lag;
		sent.s = ent->s;
		sent.r = ent->r;
		sent.clipmask = ent->clipmask;
		//VectorCopy( client->ps.origin, sent.s.pos.trBase );
		//VectorCopy( client->ps.velocity, sent.s.pos.trDelta );
		lag = level.time - client->ps.commandTime + 50;
		if ( lag > 500 )
			lag = 500;
		G_PredictPlayerMove( &sent, (float)lag * 0.001f );
		VectorCopy( sent.s.pos.trBase, ent->r.s.origin2 );
		ent->r.s.origin2[2] += client->ps.viewheight;
		ent->r.svFlags |= svf_self_portal2;
	}

	if ( frames > 2 ) {
		// limit lagged player prediction to 2 server frames
		frames = 2;
		// and add the EF_CONNECTION flag if we haven't gotten commands recently
		if ( !( ent->r.svFlags & SVF_BOT ) ) {
			client->ps.eFlags |= EF_CONNECTION;
			ent->s.eFlags |= EF_CONNECTION;
		}
	}

	if ( frames > 0 && g_smoothClients.integer ) {
		G_PredictPlayerMove( ent, (float)frames / sv_fps.value );
		SnapVector( ent->s.pos.trBase );
	}

	// unlagged
	G_StoreHistory( ent );

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}
