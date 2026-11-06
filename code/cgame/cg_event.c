// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

// for the voice chats
#ifdef MISSIONPACK // bk001205
#include "../../docs/contract/retail_menudef.h"
#endif
//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char	*CG_PlaceString( int rank ) {
	static char	str[64];
	char	*s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if ( rank == 1 ) {
		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;		// draw in blue
	} else if ( rank == 2 ) {
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;		// draw in red
	} else if ( rank == 3 ) {
		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;		// draw in yellow
	} else if ( rank == 11 ) {
		s = "11th";
	} else if ( rank == 12 ) {
		s = "12th";
	} else if ( rank == 13 ) {
		s = "13th";
	} else if ( rank % 10 == 1 ) {
		s = va("%ist", rank);
	} else if ( rank % 10 == 2 ) {
		s = va("%ind", rank);
	} else if ( rank % 10 == 3 ) {
		s = va("%ird", rank);
	} else {
		s = va("%ith", rank);
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}


//==========================================================================


/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent ) {
	clientInfo_t *ci;
	int			itemNum, clientNum;
	gitem_t		*item;
	entityState_t *es;

	es = &cent->currentState;
	
	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}

	// print a message if the local player
	if ( es->number == cg.snap->ps.clientNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		} else {
			item = BG_FindItemForHoldable( itemNum );
			CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		}
	}

	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		clientNum = cent->currentState.clientNum;
		if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
			ci = &cgs.clientinfo[ clientNum ];
			ci->medkitUsageTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		break;

#ifdef MISSIONPACK
	case HI_KAMIKAZE:
		break;

	case HI_PORTAL:
		break;
	case HI_INVULNERABILITY:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useInvulnerabilitySound );
		break;
#endif
	}
}


/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum ) {
	static int oldItem = -1;
	
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;

	if ( oldItem != itemNum )
		cg.itemPickupCount = 1;
	else
		cg.itemPickupCount++;

	oldItem = itemNum;
	
	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {
		CG_SelectPickedUpWeapon(bg_itemlist[itemNum].giTag);
	}

}


/*
================
CG_WaterLevel

Returns waterlevel for entity origin
================
*/
int CG_WaterLevel(centity_t *cent) {
	vec3_t point;
	int contents, sample1, sample2, anim, waterlevel;
	int viewheight;

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		viewheight = CROUCH_VIEWHEIGHT;
	} else {
		viewheight = DEFAULT_VIEWHEIGHT;
	}

	//
	// get waterlevel, accounting for ducking
	//
	waterlevel = 0;

	point[0] = cent->lerpOrigin[0];
	point[1] = cent->lerpOrigin[1];
	point[2] = cent->lerpOrigin[2] + MINS_Z + 1;
	contents = CG_PointContents(point, -1);

	if ( contents & MASK_WATER ) {
		sample2 = viewheight - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerpOrigin[2] + MINS_Z + sample1;
		contents = CG_PointContents(point, -1);

		if (contents & MASK_WATER) {
			waterlevel = 2;
			point[2] = cent->lerpOrigin[2] + MINS_Z + sample2;
			contents = CG_PointContents(point, -1);

			if (contents & MASK_WATER) {
				waterlevel = 3;
			}
		}
	}

	return waterlevel;
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health ) {
	const char *snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		cent->pe.painIgnore = qfalse;
		return;
	}

	if ( cent->pe.painIgnore ) {
		cent->pe.painIgnore = qfalse;
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25_1.wav";
	} else if ( health < 50 ) {
		snd = "*pain50_1.wav";
	} else if ( health < 75 ) {
		snd = "*pain75_1.wav";
	} else {
		snd = "*pain100_1.wav";
	}

	// play a gurp sound instead of a normal pain sound
	if (CG_WaterLevel(cent) == 3) {
		if (rand()&1) {
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp1.wav"));
		} else {
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, "sound/player/gurp2.wav"));
		}
	} else {
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound(cent->currentState.number, snd));
	}

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}



/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
void CG_EntityEvent( centity_t *cent, vec3_t position, int entityNum ) {
	entityState_t	*es;
	entity_event_t	event;
	vec3_t			dir;
	const char		*s;
	int				clientNum;
	clientInfo_t	*ci;
	vec3_t			vec;
	float			fovOffset;
	centity_t		*ce;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( (unsigned) event >= EV_MAX ) {
		CG_Error( "Unknown event: %i", event );
		return;
	}

	if ( cg_debugEvents.integer ) {
		CG_Printf( "ent:%3i  event:%3i %s", es->number, event, eventnames[ event ] );
	}

	if ( !event ) {
		return;
	}

	clientNum = es->clientNum;
	if ( (unsigned) clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	if (CG_QL_EntityEvent(cent)) return;

	switch ( event ) {
	//
	// movement generated events
	//
	case EV_FOOTSTEP:
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ ci->footsteps ][rand()&3] );
		}
		break;

	case EV_FOOTSTEP_METAL:
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3] );
		}
		break;

	case EV_FOOTSPLASH:
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;

	case EV_FOOTWADE:
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;

	case EV_SWIM:
		if (cg_footsteps.integer) {
			trap_S_StartSound (NULL, es->number, CHAN_BODY, 
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3] );
		}
		break;

	case EV_FALL_SHORT:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		break;

	case EV_FALL_MEDIUM:
		// use normal pain sound
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		cent->pe.painIgnore = qtrue;
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;

	case EV_FALL_FAR:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painIgnore = qtrue;
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;

	/* QL step smoothing comes from pmove.stepUp, not Q3 step events. */
	case EV_JUMP_PAD:
//		CG_Printf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		{
			vec3_t			up = {0, 0, 1};


			CG_SmokePuff( cent->lerpOrigin, up, 
						  32, 
						  1, 1, 1, 0.33f,
						  1000, 
						  cg.time, 0,
						  LEF_PUFF_DONT_SCALE, 
						  cgs.media.smokePuffShader );
		}

		// boing sound at origin, jump sound on player
		trap_S_StartSound ( cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_JUMP:
		// pain event with fast sequential jump just creates sound distortion
		if ( cg.time - cent->pe.painTime > 50 )
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_TAUNT:
		trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
		break;

#ifdef MISSIONPACK
	case EV_TAUNT_YES:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_YES);
		break;

	case EV_TAUNT_NO:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_NO);
		break;

	case EV_TAUNT_FOLLOWME:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_FOLLOWME);
		break;

	case EV_TAUNT_GETFLAG:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONGETFLAG);
		break;

	case EV_TAUNT_GUARDBASE:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONDEFENSE);
		break;

	case EV_TAUNT_PATROL:
		CG_VoiceChatLocal(SAY_TEAM, qfalse, es->number, COLOR_CYAN, VOICECHAT_ONPATROL);
		break;
#endif
	case EV_WATER_TOUCH:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;

	case EV_WATER_LEAVE:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;

	case EV_WATER_UNDER:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
		break;

	case EV_WATER_CLEAR:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
		break;

	case EV_ITEM_PICKUP:
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}

			if ( entityNum >= 0 ) {
				// our predicted entity
				ce = cg_entities + entityNum;
				if ( ce->delaySpawn > cg.time && ce->delaySpawnPlayed ) {
					break; // delay item pickup
				}
			} else {
				ce = NULL;
			}

			item = &bg_itemlist[ index ];

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if ( item->giType == IT_POWERUP || item->giType == IT_TEAM) {
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.n_healthSound );
			} else if (item->giType == IT_PERSISTANT_POWERUP) {
				// retail 0x1001a50b..0x1001a583: the item's own pickup sound,
				// then the rune voice by tag
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	trap_S_RegisterSound( item->pickup_sound, qfalse ) );
				CG_PersistentPowerupAnnouncement( item->giTag );
			} else {
				trap_S_StartSound (NULL, es->number, CHAN_AUTO,	trap_S_RegisterSound( item->pickup_sound, qfalse ) );
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}

			if ( ce ) {
				ce->delaySpawnPlayed = qtrue;
			}
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		{
			gitem_t	*item;
			int		index;

			index = es->eventParm;		// player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}

			if ( entityNum >= 0 ) {
				// our predicted entity
				ce = cg_entities + entityNum;
				if ( ce->delaySpawn > cg.time && ce->delaySpawnPlayed ) {
					break;
				}
			} else {
				ce = NULL;
			}

			item = &bg_itemlist[ index ];
			// powerup pickups are global
			if( item->pickup_sound ) {
				trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound, qfalse ) );
			}
			CG_GlobalItemPickupAnnouncement( es, item->giType, item->giTag );

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}

			if ( ce ) {
				ce->delaySpawnPlayed = qtrue;
			}
		}
		break;

	//
	// weapon events
	//
	case EV_NOAMMO:
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;

	case EV_CHANGE_WEAPON:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
		break;

	case EV_FIRE_WEAPON:
		CG_FireWeapon( cent );
		break;

	case EV_USE_ITEM0:
	case EV_USE_ITEM1:
	case EV_USE_ITEM2:
	case EV_USE_ITEM3:
	case EV_USE_ITEM4:
	case EV_USE_ITEM5:
	case EV_USE_ITEM6:
	case EV_USE_ITEM7:
	case EV_USE_ITEM8:
	case EV_USE_ITEM9:
	case EV_USE_ITEM10:
	case EV_USE_ITEM11:
	case EV_USE_ITEM12:
	case EV_USE_ITEM13:
	case EV_USE_ITEM14:
		CG_UseItem( cent );
		break;

	//=================================================================

	//
	// other events
	//
	case EV_PLAYER_TELEPORT_IN:
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
		CG_SpawnEffect( position, cg.snap->ps.clientNum == clientNum && cg_thirdPerson.integer == 0 );
		break;

	case EV_PLAYER_TELEPORT_OUT:
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
		CG_SpawnEffect( position, qfalse );
		break;

	case EV_ITEM_POP:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_ITEM_RESPAWN:
		cent->miscTime = cg.time;	// scale up from this
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_GRENADE_BOUNCE:
		if ( rand() & 1 ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound );
		} else {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound );
		}
		break;

#ifdef MISSIONPACK
	case EV_PROXIMITY_MINE_STICK:
		if( es->eventParm & SURF_FLESH ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimplSound );
		} else 	if( es->eventParm & SURF_METALSTEPS ) {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpmSound );
		} else {
			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbimpdSound );
		}
		break;

	case EV_PROXIMITY_MINE_TRIGGER:
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.wstbactvSound );
		break;

	case EV_KAMIKAZE:
		CG_KamikazeEffect( cent->lerpOrigin );
		break;

	case EV_OBELISKEXPLODE:
		CG_ObeliskExplode( cent->lerpOrigin, es->eventParm );
		break;

	case EV_OBELISKPAIN:
		CG_ObeliskPain( cent->lerpOrigin );
		break;

	case EV_INVUL_IMPACT:
		CG_InvulnerabilityImpact( cent->lerpOrigin, cent->currentState.angles );
		break;

	case EV_JUICED:
		// retail 0x1001ab40: the DLC juicer, else electro gibs
		if ( !cgs.media.goreDLC ) {
			CG_ElectroGibs( cent->lerpOrigin, qtrue );
			break;
		}
		CG_InvulnerabilityJuiced( cent->lerpOrigin );
		break;

	case EV_LIGHTNINGBOLT:
		CG_LightningBoltBeam(es->origin2, es->pos.trBase);
		break;
#endif

	case EV_SCOREPLUM:
		CG_ScorePlum( cent->currentState.otherEntityNum, cent->lerpOrigin, cent->currentState.time );
		break;

	//
	// missile impacts
	//
	case EV_MISSILE_HIT:
		ByteToDir( es->eventParm, dir );
		CG_MissileHitPlayer( es->weapon, position, dir, es->otherEntityNum );
		break;

	case EV_MISSILE_MISS:
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT );
		break;

	case EV_MISSILE_MISS_METAL:
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, 0, position, dir, IMPACTSOUND_METAL );
		break;

	case EV_RAILTRAIL:
		cent->currentState.weapon = WP_RAILGUN;

		if ( cent->currentState.clientNum == cg.snap->ps.clientNum && !cg_thirdPerson.integer ) 
		{
			VectorCopy( cg.refdef.vieworg, vec );
			fovOffset = -0.2f * ( cgs.fov - 90.0f );

			// 13.5, -5.5, -6.0
			VectorMA( vec, cg_gun_x.value + 13.5f, cg.refdef.viewaxis[0], vec );
			VectorMA( vec, cg_gun_y.value - 5.5f, cg.refdef.viewaxis[1], vec );
			VectorMA( vec, cg_gun_z.value + fovOffset - 6.0f, cg.refdef.viewaxis[2], vec );
		}
		else
			VectorCopy( es->origin2, vec );

		// if the end was on a nomark surface, don't make an explosion
		CG_RailTrail( ci, vec, es->pos.trBase );

		if ( es->eventParm != 255 ) {
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( es->weapon, es->clientNum, position, dir, IMPACTSOUND_DEFAULT );
		}
		break;

	case EV_BULLET_HIT_WALL:
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD );
		break;

	case EV_BULLET_HIT_FLESH:
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm );
		break;

	case EV_SHOTGUN:
		CG_ShotgunFire( es );
		break;

	case EV_GENERAL_SOUND:
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound (NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_TEAM_SOUND:
		// every retail subcase is handled by CG_QL_EntityEvent (cg_ql_events.c,
		// retail switch 0x1001b349); retail has no case 21 or 22
		break;

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		if ( CG_WaterLevel(cent) == 3 ) {
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, "*drown.wav") );
		} else {
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound(es->number, va("*death%i.wav", event - EV_DEATH1 + 1)) );
		}

		break;

	case EV_OBITUARY:
		CG_RetailObituary( es );
		break;

	//
	// powerup events
	//
	case EV_POWERUP_QUAD:
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_QUAD;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.quadSound );
		break;

	case EV_POWERUP_BATTLESUIT:
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.protectSound );
		break;

	case EV_POWERUP_REGEN:
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound (NULL, es->number, CHAN_ITEM, cgs.media.regenSound );
		break;

	case EV_GIB_PLAYER:
		// don't play gib sound when using the kamikaze because it interferes
		// with the kamikaze sound, downside is that the gib sound will also
		// not be played when someone is gibbed while just carrying the kamikaze
		// retail 0x1001bf6e: electro gib sound unless eFlags 0x200
		if ( !(es->eFlags & EF_KAMIKAZE) ) {
			// retail 0x1001bf87: the gore DLC splat replaces the electro sound
			sfxHandle_t gibSound = cgs.media.goreDLC ? cgs.media.dlcGibSplatSound : CG_ElectroGibSound();
			if ( gibSound ) trap_S_StartSound( NULL, es->number, CHAN_BODY, gibSound );
		}
		CG_GibPlayer( cent->lerpOrigin );
		break;

	case EV_DEBUG_LINE:
		CG_Beam( cent );
		break;


	default:
		CG_Error( "Unknown event: %i", event );
		break;
	}
}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			return;	// already fired
		}
		// if this is a player event set the entity number of the client entity number
		if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
			cent->currentState.number = cent->currentState.otherEntityNum;
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, cent->lerpOrigin, -1 );
}
