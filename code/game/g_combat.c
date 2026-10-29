// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_combat.c

#include "g_local.h"


/*
============
ScorePlum

Retail qagamex86 0x10046510: shown only to the scorer.
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->client->ps.clientNum;
	plum->s.clientNum = ent->client->ps.clientNum;
	plum->s.time = score;
}

/*
============
AddScore

Adds score to both the client and his team.  Retail qagamex86 0x10046620:
nothing scores before the match, during the intermission, in race, or for
humans when bots may only use the gauntlet; the match remembers its first
and latest scorers.  Red rover's own tally arrives with milestone 6.
============
*/
void AddScore( gentity_t *ent, vec3_t origin, int score ) {
	if ( !ent->client || level.warmupTime || level.intermissionQueued || level.intermissiontime ) {
		return;
	}
	if ( bot_gauntletOnly.integer && !( ent->r.svFlags & SVF_BOT ) ) {
		return;
	}
	if ( g_gametype.integer == GT_RACE ) {
		return;
	}
	if ( !level.firstScorer[0] ) {
		Q_strncpyz( level.firstScorer, ent->client->pers.netname, sizeof( level.firstScorer ) );
	}
	Q_strncpyz( level.lastScorer, ent->client->pers.netname, sizeof( level.lastScorer ) );

	// show score plum
	ScorePlum(ent, origin, score);
	//
	ent->client->ps.persistant[PERS_SCORE] += score;
	if ( g_gametype.integer == GT_TEAM ) {
		AddTeamScore( ent->client->ps.persistant[PERS_TEAM], score );
	}
	CalculateRanks();
}

/*
=================
TossClientItems

Toss the powerups and weapon for the killed player.  Retail qagamex86
0x10046760: the timed powerups go first, fanned out from 45 degrees (all of
them under g_dropPowerups, otherwise only flags and invisibility), then the
held weapon, which instagib and the round-based modes keep.
=================
*/
void TossClientItems( gentity_t *self ) {
	gclient_t	*client = self->client;
	gitem_t		*item;
	gentity_t	*drop;
	int			weapon;
	float		angle;
	int			i;

	angle = 45;
	for ( i = PW_REDFLAG; i < PW_NUM_POWERUPS; i++ ) {
		if ( client->ps.powerups[i] <= level.time || i == PW_FLIGHT || i == PW_FROZEN ) {
			continue;
		}
		item = BG_FindItemForPowerup( i );
		if ( !item ) {
			continue;
		}
		if ( !g_dropPowerups.integer && item->giTag != PW_INVIS
			&& !( item->giType == IT_TEAM && item->giTag >= PW_REDFLAG && item->giTag <= PW_NEUTRALFLAG ) ) {
			continue;
		}
		drop = Drop_Item( self, item, angle );
		if ( item->giType == IT_TEAM && item->giTag >= PW_REDFLAG && item->giTag <= PW_NEUTRALFLAG ) {
			drop->physicsBounce = g_flagBounce.value;
			drop->physicsObject = qtrue;
			drop->activator = self;
		}
		if ( client->persistantPowerup ) {
			client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
			client->persistantPowerup = NULL;
		}
		// decide how many seconds it has left
		drop->count = ( client->ps.powerups[i] - level.time ) / 1000;
		if ( drop->count < 1 ) {
			drop->count = 1;
		}
		angle += 45;
	}

	if ( g_instaGib.integer ) {
		return;
	}
	switch ( g_gametype.integer ) {
	case GT_RACE:
	case GT_CLAN_ARENA:
	case GT_ATTACK_DEFEND:
	case GT_RED_ROVER:
		return;
	default:
		break;
	}

	// a player who picked up a weapon but is still lowering the machinegun
	// or the grapple drops the one being raised
	weapon = self->s.weapon;
	if ( weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK ) {
		if ( client->ps.weaponstate == WEAPON_DROPPING ) {
			weapon = client->pers.cmd.weapon;
		}
		if ( !( client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

	if ( weapon > WP_MACHINEGUN && weapon < WP_NUM_WEAPONS && weapon != WP_GRAPPLING_HOOK
		&& client->ps.ammo[weapon] > 0 ) {
		Drop_Item( self, BG_FindItemForWeapon( weapon ), 0 );
	}
}


#ifdef MISSIONPACK
/*
=================
TossClientCubes
=================
*/
extern gentity_t	*neutralObelisk;

void TossClientCubes( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	vec3_t		origin;

	self->client->ps.generic1 = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if (!G_EntitiesFree()) {
		return;
	}

	if( self->client->sess.sessionTeam == TEAM_RED ) {
		item = BG_FindItem( "Red Skull" );
	}
	else {
		item = BG_FindItem( "Blue Skull" );
	}

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if( neutralObelisk ) {
		VectorCopy( neutralObelisk->s.pos.trBase, origin );
		origin[2] += 44;
	} else {
		VectorClear( origin ) ;
	}

	drop = LaunchItem( item, origin, velocity );

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = G_FreeEntity;
	drop->spawnflags = self->client->sess.sessionTeam;
}


/*
=================
TossClientPersistantPowerups
=================
*/
void TossClientPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->client ) {
		return;
	}

	if( !ent->client->persistantPowerup ) {
		return;
	}

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( powerup );

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = NULL;
}
#endif



/*
==================
GibEntity

Retail qagamex86 0x10046d80: the gib flies away from the last damage, and
a gibbed frozen player thaws.
==================
*/
void GibEntity( gentity_t *self ) {
#ifdef MISSIONPACK
	gentity_t *ent;
	int i;

	//if this entity still has kamikaze
	if (self->s.eFlags & EF_KAMIKAZE) {
		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < level.num_entities; i++) {
			ent = &g_entities[i];
			if (!ent->inuse)
				continue;
			if (ent->activator != self)
				continue;
			if (strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	}
#endif

	if ( self->client ) {
		G_AddEvent( self, EV_GIB_PLAYER, DirToByte( self->client->damage_from ) );
	} else {
		vec3_t down = { 0, 0, -1 };

		G_AddEvent( self, EV_GIB_PLAYER, DirToByte( down ) );
	}
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;

	// freeze tag's thaw of a gibbed ice block arrives with milestone 6
}

/*
==================
body_die

Retail qagamex86 0x10046f00: a corpse gibs once it is hurt past
GIB_HEALTH, whatever g_blood says.
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health <= GIB_HEALTH ) {
		GibEntity( self );
	}
}


// these are just for logging, the client prints its own messages
char	*modNames[MOD_NUM_MAX] = {
#define MOD_STRINGS
	#include "bg_mods.h"
#undef MOD_STRINGS
	"NULL"	// avoid -Wpedantic warnings
};

#ifdef MISSIONPACK
/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}

#endif

/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if this player was carrying a flag
	if ( self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( g_gametype.integer == GT_CTF ) {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void CheckAlmostScored( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if the player was carrying cubes
	if ( self->client->ps.generic1 ) {
		if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			classname = "team_redobelisk";
		}
		else {
			classname = "team_blueobelisk";
		}
		ent = G_Find(NULL, FOFS(classname), classname);
		// if we found the destination obelisk
		if ( ent ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
G_AwardMedal

Retail qagamex86 0x10046550: count a Quake Live medal and announce it to
everyone from the player's position.
==================
*/
static void G_AwardMedal( gentity_t *ent, medal_t medal ) {
	gclient_t	*client = ent->client;
	gentity_t	*te;

	te = G_TempEntity( client->ps.origin, EV_AWARD );
	client->medals[medal]++;
	te->r.svFlags |= SVF_BROADCAST;
	te->s.clientNum = client->ps.clientNum;
	te->s.generic1 = medal;
	te->s.modelindex2 = client->medals[medal];
	client->rewardTime = level.time + REWARD_SPRITE_TIME;
}


/*
==================
G_AwardSprite

Retail qagamex86 0x100465d0: the Quake III rewards, counted in a
persistant slot and drawn as a sprite over the player.
==================
*/
static void G_AwardSprite( gentity_t *ent, int persistant, int eFlag ) {
	gclient_t	*client = ent->client;

	client->ps.persistant[persistant]++;
	client->ps.eFlags &= ~EF_AWARDS;
	client->ps.eFlags |= eFlag;
	client->rewardTime = level.time + REWARD_SPRITE_TIME;
}


/*
==================
G_IsWorldDeath

The means of death that retail counts as killed by the world when no
player is to blame.
==================
*/
static qboolean G_IsWorldDeath( int meansOfDeath ) {
	switch ( meansOfDeath ) {
	case MOD_WATER:
	case MOD_SLIME:
	case MOD_LAVA:
	case MOD_CRUSH:
	case MOD_TELEFRAG:
	case MOD_FALLING:
	case MOD_TARGET_LASER:
	case MOD_TRIGGER_HURT:
		return qtrue;
	default:
		return qfalse;
	}
}


/*
==================
G_KillRewards

Retail qagamex86 0x1004768f-0x10047ce9: an enemy kill's score, medals and
rewards.
==================
*/
static void G_KillRewards( gentity_t *self, gentity_t *attacker, int meansOfDeath ) {
	gclient_t	*client = self->client;
	gclient_t	*killer = attacker->client;
	trace_t		tr;
	vec3_t		end;

	if ( !g_quadHog.integer ) {
		// red rover's infected bonus and death penalty arrive with milestone 6
		AddScore( attacker, self->r.currentOrigin, 1 );
	} else if ( g_quadHog.integer == 1 && g_gametype.integer == GT_FFA ) {
		// quad hog: only kills by or of the quad holder score
		if ( killer->ps.powerups[PW_QUAD] || client->ps.powerups[PW_QUAD] ) {
			AddScore( attacker, self->r.currentOrigin, 1 );
		}
	}

	if ( killer->ps.powerups[PW_BATTLESUIT] ) {
		killer->ps.stats[STAT_BATTLESUIT_FRAG_COUNT]++;
	}
	if ( killer->ps.powerups[PW_QUAD] ) {
		killer->ps.stats[STAT_QUAD_FRAG_COUNT]++;
		killer->stats.quadKills++;
	}

	// the first frag of the match
	if ( !level.firstFragger[0] && !level.warmupTime ) {
		Q_strncpyz( level.firstFragger, killer->pers.netname, sizeof( level.firstFragger ) );
		killer->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_FIRSTFRAG;
		G_AwardMedal( attacker, MEDAL_FIRSTFRAG );
	}

	if ( meansOfDeath == MOD_GAUNTLET ) {
		// play humiliation on the player and the target
		G_AwardSprite( attacker, PERS_GAUNTLET_FRAG_COUNT, EF_AWARD_GAUNTLET );
		client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;
	}

	// two kills in a short amount of time
	if ( level.time - killer->lastKillTime < CARNAGE_REWARD_TIME ) {
		G_AwardSprite( attacker, PERS_EXCELLENT_COUNT, EF_AWARD_EXCELLENT );
	}

	// killing a player who has killed you three times since you last got him
	client->killedBy[attacker->s.number]++;
	if ( killer->killedBy[self->s.number] >= 3 ) {
		G_AwardMedal( attacker, MEDAL_REVENGE );
	}
	killer->killedBy[self->s.number] = 0;

	// a rail kill right after another weapon hit
	if ( meansOfDeath == MOD_RAILGUN || meansOfDeath == MOD_RAILGUN_HEADSHOT ) {
		const int weapon = G_WeaponForMod( client->prevhurt_mod );

		if ( weapon != WP_NONE && weapon != WP_RAILGUN && client->prevhurt_client == attacker->s.number
			&& client->prevhurt_time + 1500 > client->lasthurt_time ) {
			G_AwardMedal( attacker, MEDAL_COMBOKILL );
		}
	}

	if ( meansOfDeath == MOD_SHOTGUN ) {
		if ( client->ps.groundEntityNum == ENTITYNUM_NONE && !self->waterlevel ) {
			killer->stats.midairShotgunKills++;
		}
	} else {
		// a projectile kill of a player well clear of the ground
		if ( ( meansOfDeath == MOD_GRENADE || meansOfDeath == MOD_ROCKET || meansOfDeath == MOD_PLASMA
				|| meansOfDeath == MOD_BFG || meansOfDeath == MOD_NAIL )
			&& client->ps.groundEntityNum == ENTITYNUM_NONE && !self->waterlevel ) {
			end[0] = client->ps.origin[0];
			end[1] = client->ps.origin[1];
			end[2] = (float)( (double)client->ps.origin[2] - (double)g_midAirMinHeight.value );
			trap_Trace( &tr, client->ps.origin, NULL, NULL, end, ENTITYNUM_NONE, CONTENTS_SOLID );
			if ( tr.fraction == 1.0f ) {
				G_AwardMedal( attacker, MEDAL_MIDAIR );
			}
		}
		if ( meansOfDeath == MOD_RAILGUN_HEADSHOT ) {
			G_AwardMedal( attacker, MEDAL_HEADSHOT );
		} else if ( meansOfDeath == MOD_TELEFRAG ) {
			G_AwardMedal( attacker, MEDAL_PERFORATED );
		}
	}

	// every tenth kill under the quad
	if ( killer->ps.powerups[PW_QUAD] && killer->ps.stats[STAT_QUAD_FRAG_COUNT] > 9
		&& killer->ps.stats[STAT_QUAD_FRAG_COUNT] % 10 == 0 ) {
		G_AwardMedal( attacker, MEDAL_QUADGOD );
	}

	killer->lastKillTime = level.time;
}


/*
==================
G_ReturnCarriedFlag

A dying carrier's flag goes home instead of dropping.
==================
*/
static void G_ReturnCarriedFlag( gclient_t *client ) {
	if ( client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
		Team_ReturnFlag( TEAM_FREE );
		client->ps.powerups[PW_NEUTRALFLAG] = 0;
	} else if ( client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
		Team_ReturnFlag( TEAM_RED );
		client->ps.powerups[PW_REDFLAG] = 0;
	} else if ( client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
		Team_ReturnFlag( TEAM_BLUE );
		client->ps.powerups[PW_BLUEFLAG] = 0;
	}
}


/*
==================
player_die

Retail qagamex86 0x10047200.  Freeze tag's freezing, red rover's
infection, team-kill complaints and the round checks of the round-based
modes arrive with milestone 6.
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gclient_t	*client = self->client;
	gentity_t	*ent;
	int			anim;
	int			killer;
	int			i;
	char		*killerName, *obit;
	static int	deathAnim;

	if ( client->ps.pm_type == PM_DEAD || client->ps.pm_type == PM_FREEZE
		|| level.intermissionQueued || level.intermissiontime ) {
		return;
	}

	// a training bot owns up to its own death
	if ( ( self->r.svFlags & SVF_BOT ) && ( bot_training.integer == 1 || bot_training.integer == 2 ) ) {
		trap_EA_Command( client->ps.clientNum, "say Oops! Um... I meant to do that! Just don't you do it, ok?" );
	}

	//unlag the client
	G_UnTimeShiftClient( self );

	// check for an almost capture
	CheckAlmostCapture( self, attacker );
	// check for a player that almost brought in cubes
	CheckAlmostScored( self, attacker );

	// free this player's grapple, and every grapple holding this player
	if ( client->hook ) {
		Weapon_HookFree( client->hook );
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t *other = &level.clients[i];

		if ( other->pers.connected == CON_CONNECTED && other->sess.sessionTeam != TEAM_SPECTATOR
			&& other->hook && other->hook->enemy == self ) {
			Weapon_HookFree( other->hook );
		}
	}
#ifdef MISSIONPACK
	if ((client->ps.eFlags & EF_TICKING) && self->activator) {
		client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}
#endif
	client->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( (unsigned)meansOfDeath >= ARRAY_LEN( modNames ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[ meansOfDeath ];
	}

	// a shotgun kill shows the blast's damage at the victim
	if ( attacker && attacker->client && meansOfDeath == MOD_SHOTGUN ) {
		ent = G_TempEntity( client->ps.origin, EV_SHOTGUN_KILL );
		ent->s.generic1 = attacker->client->shotgunDamage[self->s.number];
		ent->s.otherEntityNum = attacker->s.number;
		ent->s.otherEntityNum2 = self->s.number;
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n",
		killer, self->s.number, meansOfDeath, killerName,
		client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST;	// send to everyone

	self->enemy = attacker;

	client->ps.persistant[PERS_KILLED]++;
	G_StatsKill( self, attacker, meansOfDeath );

	if ( !attacker || !attacker->client ) {
		AddScore( self, self->r.currentOrigin, -1 );
		if ( G_IsWorldDeath( meansOfDeath ) ) {
			client->stats.suicides++;
		}
	} else {
		attacker->client->lastkilled_client = self->s.number;
		client->lastKiller = attacker->s.number;

		if ( attacker != self && OnSameTeam( self, attacker ) ) {
			AddScore( attacker, self->r.currentOrigin, -1 );
			attacker->client->stats.teamKills++;
			client->stats.teamKilled++;
		} else if ( attacker != self ) {
			G_KillRewards( self, attacker, meansOfDeath );
		} else {
			AddScore( self, self->r.currentOrigin, -1 );
			if ( meansOfDeath == MOD_SUICIDE ) {
				client->stats.suicides++;
			}
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// a team switch always returns a carried flag; a suicide does when
	// g_returnFlagOnSuicide says so
	if ( meansOfDeath == MOD_SWITCHTEAM || ( meansOfDeath == MOD_SUICIDE && g_returnFlagOnSuicide.integer ) ) {
		G_ReturnCarriedFlag( client );
	}

	// carried keys go back to their spawns
	if ( client->ps.stats[STAT_KEY_MASK] ) {
		for ( i = 0; i < 3; i++ ) {
			if ( client->ps.stats[STAT_KEY_MASK] & ( 1 << i ) ) {
				G_ResetKey( 1 << i );
			}
		}
		client->ps.stats[STAT_KEY_MASK] = 0;
	}

	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	if ( trap_PointContents( self->r.currentOrigin, -1 ) & CONTENTS_NODROP ) {
		G_ReturnCarriedFlag( client );
	} else {
		TossClientItems( self );
	}
#ifdef MISSIONPACK
	if ( g_gametype.integer == GT_HARVESTER && meansOfDeath != MOD_SWITCHTEAM ) {
		TossClientCubes( self );
	}
#endif

	// remove powerups
	memset( client->ps.powerups, 0, sizeof(client->ps.powerups) );

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*follower;

		follower = &level.clients[i];
		if ( follower->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( follower->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( follower->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->s.loopSound = 0;
	self->r.contents = CONTENTS_CORPSE;
	self->r.maxs[2] = -8;

	// the body and the view face the killer
	self->s.angles[PITCH] = 0;
	self->s.angles[ROLL] = 0;
	if ( attacker && attacker != self ) {
		vec3_t dir;

		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
		self->s.angles[YAW] = vectoyaw( dir );
	}
	client->ps.viewangles[PITCH] = 0;
	client->ps.viewangles[YAW] = self->s.angles[YAW];
	client->ps.viewangles[ROLL] = self->s.angles[ROLL];

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	client->respawnTime = level.time + g_respawn_delay_min.integer;
	if ( level.warmupTime > 0 ) {
		switch ( g_gametype.integer ) {
		case GT_CLAN_ARENA:
		case GT_FREEZE:
		case GT_ATTACK_DEFEND:
		case GT_RED_ROVER:
			// a round about to start brings everyone straight back
			client->respawnTime = level.time + 500;
			break;
		default:
			break;
		}
	}

	if ( meansOfDeath == MOD_SUICIDE ) {
		// a suicide waits g_allowKill longer, and always gibs
		client->respawnTime += g_allowKill.integer;
		GibEntity( self );
	} else if ( self->health <= GIB_HEALTH ) {
		GibEntity( self );
		// three gibs, each within three seconds of the last
		if ( attacker && attacker->client ) {
			gclient_t *gibber = attacker->client;

			if ( attacker != self && !OnSameTeam( self, attacker ) ) {
				if ( gibber->gibStreak == 0 || ( gibber->gibStreak == 1 && gibber->lastGibTime + 3000 >= level.time ) ) {
					gibber->gibStreak++;
				} else if ( gibber->gibStreak >= 2 && gibber->lastGibTime + 3000 > level.time ) {
					G_AwardMedal( attacker, MEDAL_RAMPAGE );
					gibber->gibStreak = 0;
				}
			}
			gibber->lastGibTime = level.time;
		}
	} else {
		// normal death; the death animations cycle globally
		switch ( deathAnim ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		default:
			anim = BOTH_DEATH3;
			break;
		}
		G_AddEvent( self, meansOfDeath == MOD_WATER ? EV_DROWN : EV_DEATH1 + deathAnim, 0 );
		deathAnim = ( deathAnim + 1 ) % 3;

		// the body can be gibbed later
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH+1;
		}

		client->ps.legsAnim =
			( ( client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		client->ps.torsoAnim =
			( ( client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		// the body can still be gibbed
		self->die = body_die;

#ifdef MISSIONPACK
		if (self->s.eFlags & EF_KAMIKAZE) {
			Kamikaze_DeathTimer( self );
		}
#endif
	}

	trap_LinkEntity (self);

}


/*
================
G_ArmorSave

How much of the damage the target's armor absorbs (retail qagamex86
0x10048b50).  Tiered armor protects by tier, the armor regen powerup always
75%; G_Damage takes the armor off.
================
*/
static int G_ArmorSave( gentity_t *ent, int damage, int dflags ) {
	gclient_t	*client = ent->client;
	double		protection;
	int			save;

	if ( !damage || !client || ( dflags & DAMAGE_NO_ARMOR ) ) {
		return 0;
	}

	if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_ARMORREGEN ) {
		protection = 0.75;
	} else if ( !armor_tiered.integer || g_gametype.integer == GT_CLAN_ARENA ) {
		protection = 0.66;
	} else {
		switch ( client->ps.stats[STAT_ARMOR_TIER] ) {
		case 0:
			protection = 0.5;
			break;
		case 2:
			protection = 0.75;
			break;
		default:
			protection = 0.66;
			break;
		}
	}

	save = (int)ceil( protection * damage );
	if ( save > client->ps.stats[STAT_ARMOR] ) {
		save = client->ps.stats[STAT_ARMOR];
	}
	return save;
}


/*
================
G_HitTier

The hit sound tier the attacker's generic1 carries in bits 6-7.
================
*/
int G_HitTier( int damage ) {
	if ( damage >= 75 ) {
		return 3;
	}
	if ( damage >= 50 ) {
		return 2;
	}
	return damage >= 25 ? 1 : 0;
}


/*
==================
G_StatsHit

A scored hit counts toward both the overall and the weapon's accuracy.
==================
*/
void G_StatsHit( gclient_t *client, int weapon ) {
	client->accuracy_hits++;
	client->stats.weaponHits[weapon]++;
}


/*
==================
G_StatsDamage

Retail qagamex86 0x10071580, before the damage lands: an enemy's damage
counts as taken, capped at what the target had left, and as given while
the target is alive.
==================
*/
void G_StatsDamage( gentity_t *targ, gentity_t *attacker, int mod, int damage ) {
	gclient_t	*victim = targ->client;
	gclient_t	*killer = attacker->client;
	int			weapon, taken;

	if ( attacker == targ ) {
		return;
	}
	weapon = G_WeaponForMod( mod );

	if ( victim && ( !killer || ( g_gametype.integer < GT_TEAM && victim->sess.sessionTeam != TEAM_SPECTATOR )
		|| victim->sess.sessionTeam != killer->sess.sessionTeam ) ) {
		taken = victim->ps.stats[STAT_ARMOR] + victim->ps.stats[STAT_HEALTH];
		if ( damage <= taken ) {
			taken = damage;
		}
		victim->stats.damageTaken += taken;
		if ( weapon != WP_NONE ) {
			victim->stats.weaponDamageTaken[weapon] += taken;
		}
	}

	if ( weapon != WP_NONE && killer && victim && ( targ->health > 0 || g_instaGib.integer )
		&& !OnSameTeam( targ, attacker ) ) {
		killer->stats.damageGiven += damage;
		killer->stats.weaponDamageGiven[weapon] += damage;
	}
}


/*
==================
G_StatsKill

Retail qagamex86 0x100726e0: an enemy kill counts for the killer and every
death but a team switch for the victim, whose streak ends.
==================
*/
void G_StatsKill( gentity_t *victim, gentity_t *attacker, int mod ) {
	const int	weapon = G_WeaponForMod( mod );
	gclient_t	*client = victim->client;

	if ( attacker && attacker->client && attacker != victim && !OnSameTeam( attacker, victim ) ) {
		attacker->client->stats.kills++;
		attacker->client->stats.weaponKills[weapon]++;
		attacker->client->stats.killStreak++;
	}

	if ( client ) {
		if ( mod != MOD_SWITCHTEAM ) {
			client->stats.deaths++;
			if ( weapon != WP_NONE ) {
				client->stats.weaponDeaths[weapon]++;
			}
		}
		if ( client->stats.killStreak > client->stats.maxKillStreak ) {
			client->stats.maxKillStreak = client->stats.killStreak;
		}
		client->stats.killStreak = 0;
	}
}


/*
================
G_WeaponForMod

The weapon a means of death belongs to (retail qagamex86 0x10071320).
================
*/
int G_WeaponForMod( int mod ) {
	switch ( mod ) {
	case MOD_SHOTGUN:				return WP_SHOTGUN;
	case MOD_GAUNTLET:				return WP_GAUNTLET;
	case MOD_MACHINEGUN:			return WP_MACHINEGUN;
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:		return WP_GRENADE_LAUNCHER;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:			return WP_ROCKET_LAUNCHER;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:			return WP_PLASMAGUN;
	case MOD_RAILGUN:
	case MOD_RAILGUN_HEADSHOT:		return WP_RAILGUN;
	case MOD_LIGHTNING:
	case MOD_LIGHTNING_DISCHARGE:	return WP_LIGHTNING;
	case MOD_BFG:
	case MOD_BFG_SPLASH:			return WP_BFG;
	case MOD_NAIL:					return WP_NAILGUN;
	case MOD_CHAINGUN:				return WP_CHAINGUN;
	case MOD_PROXIMITY_MINE:		return WP_PROX_LAUNCHER;
	case MOD_HMG:					return WP_HEAVY_MACHINEGUN;
	default:						return WP_NONE;
	}
}


/*
================
G_KnockbackScale

g_knockback_<weapon> for the means of death (retail qagamex86 0x10048a10);
an unset (zero) scale and means of death without a weapon count as 1.
================
*/
static float G_KnockbackScale( int mod ) {
	float scale;

	switch ( mod ) {
	case MOD_SHOTGUN:				scale = g_knockback_sg.value; break;
	case MOD_GAUNTLET:				scale = g_knockback_g.value; break;
	case MOD_MACHINEGUN:			scale = g_knockback_mg.value; break;
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:		scale = g_knockback_gl.value; break;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:			scale = g_knockback_rl.value; break;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:			scale = g_knockback_pg.value; break;
	case MOD_RAILGUN:
	case MOD_RAILGUN_HEADSHOT:		scale = g_knockback_rg.value; break;
	case MOD_LIGHTNING:
	case MOD_LIGHTNING_DISCHARGE:	scale = g_knockback_lg.value; break;
	case MOD_BFG:
	case MOD_BFG_SPLASH:			scale = g_knockback_bfg.value; break;
	case MOD_NAIL:					scale = g_knockback_ng.value; break;
	case MOD_CHAINGUN:				scale = g_knockback_cg.value; break;
	case MOD_PROXIMITY_MINE:		scale = g_knockback_pl.value; break;
	case MOD_GRAPPLE:				scale = g_knockback_gh.value; break;
	case MOD_HMG:					scale = g_knockback_hmg.value; break;
	default:						return 1.0f;
	}
	return scale != 0.0f ? scale : 1.0f;
}


/*
================
G_DamagePlum

A floating damage number (EV_DAMAGEPLUM) for the attacker, or for everyone
with g_damagePlums 2 (retail qagamex86 0x100464a0).
================
*/
void G_DamagePlum( gentity_t *attacker, const vec3_t origin, int damage, int weapon ) {
	gentity_t *plum = G_TempEntity( (float *)origin, EV_DAMAGEPLUM );

	plum->r.svFlags |= g_damagePlums.integer == 2 ? SVF_BROADCAST : SVF_SINGLECLIENT;
	plum->r.singleClient = attacker->client->ps.clientNum;
	plum->s.clientNum = attacker->client->ps.clientNum;
	plum->s.time = damage;
	plum->s.generic1 = weapon;
}


/*
================
G_BoxDistance

Distance from a point to an entity's bounding box.
================
*/
static float G_BoxDistance( const gentity_t *ent, const vec3_t origin ) {
	vec3_t	v;
	int		i;

	for ( i = 0; i < 3; i++ ) {
		if ( origin[i] < ent->r.absmin[i] ) {
			v[i] = ent->r.absmin[i] - origin[i];
		} else if ( origin[i] > ent->r.absmax[i] ) {
			v[i] = origin[i] - ent->r.absmax[i];
		} else {
			v[i] = 0;
		}
	}
	return VectorLength( v );
}


/*
================
G_Handicap

The attacker's userinfo handicap, 1..100 (100 when unset or out of range).
================
*/
static int G_Handicap( const gentity_t *ent ) {
	char	userinfo[MAX_INFO_STRING];
	int		handicap;

	trap_GetUserinfo( ent->client->ps.clientNum, userinfo, sizeof( userinfo ) );
	handicap = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	return ( handicap < 1 || handicap > 100 ) ? 100 : handicap;
}


/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

#ifdef MISSIONPACK
/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->client ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->client->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->client->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}
#endif
/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything

Follows retail qagamex86 0x10048c30.  The gametype hooks (Clan Arena,
Attack and Defend, Red Rover and Freeze Tag damage rules), the team damage
complaints, the stats records and regeneration interrupts arrive with match
flow and the special modes (docs/PLAN.md M5, M6).
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
	int			asave;
	int			knockback;
	int			i;
	vec3_t		bouncedir, impactpoint;

	if ( !targ->takedamage ) {
		return;
	}

	client = targ->client;
	if ( client && client->noclip ) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued || level.intermissiontime ) {
		return;
	}

	if ( client ) {
		if ( mod != MOD_JUICED && client->invulnerabilityTime > level.time ) {
			if ( dir && point ) {
				G_InvulnerabilityEffect( targ, dir, point, impactpoint, bouncedir );
			}
			return;
		}
		if ( client->ps.pm_type == PM_SPECTATOR ) {
			return;
		}
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// pmove_noPlayerClip: players pass through and cannot hurt each other
	if ( pmove_noPlayerClip.integer && attacker->client && client && targ != attacker ) {
		return;
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( g_gametype.integer == GT_OBELISK && CheckObeliskAttack( targ, attacker ) ) {
		return;
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	// knockback scales by weapon; rocket and plasma jumps have their own
	if ( client && targ == attacker && ( mod == MOD_ROCKET || mod == MOD_ROCKET_SPLASH ) ) {
		knockback = (int)( g_knockback_rl_self.value * damage );
	} else if ( client && targ == attacker && ( mod == MOD_PLASMA || mod == MOD_PLASMA_SPLASH ) ) {
		knockback = (int)( g_knockback_pg_self.value * damage );
	} else {
		// retail also cuts MOD_GRAPPLE knockback to 0.2 when a hook field
		// (+0x370) is set; that field is not identified yet
		knockback = (int)( G_KnockbackScale( mod ) * damage );
	}
	if ( knockback > g_max_knockback.integer ) {
		knockback = g_max_knockback.integer;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// rocket splash rebuilds its damage from g_splashdamage_rl and the
	// quad factor the rocket carries
	if ( mod == MOD_ROCKET_SPLASH ) {
		damage = (int)( ( 1.0 - G_BoxDistance( targ, inflictor->r.currentOrigin )
			/ inflictor->splashRadius ) * g_splashdamage_rl.integer ) * inflictor->damageFactor;
	}

	// reduce damage by the attacker's handicap value
	if ( attacker->client && attacker != targ ) {
		damage = G_Handicap( attacker ) * damage / 100;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && client ) {
		double	push;
		float	sign = 1.0f;
		float	t;

		if ( knockback < 0 ) {
			// a negative knockback pulls
			knockback = -knockback;
			sign = -1.0f;
		}
		push = (double)(float)knockback * g_knockback.value / 200.0;
		for ( i = 0; i < 3; i++ ) {
			client->ps.velocity[i] = (float)( push * ( sign * dir[i] ) + client->ps.velocity[i] );
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !client->ps.pm_time ) {
			t = (float)( knockback * 2 );
			if ( g_knockback_cripple.value > t ) {
				t = g_knockback_cripple.value;
			} else if ( t > 200.0f ) {
				t = 200.0f;
			}
			client->ps.pm_time = (int)t;
			client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( mod != MOD_JUICED && targ != attacker && !( dflags & DAMAGE_NO_TEAM_PROTECTION )
			&& OnSameTeam( targ, attacker ) && !g_friendlyFire.integer ) {
			return;
		}

		// g_quadHog: in FFA only the quad carrier hurts or gets hurt
		if ( client && attacker->client && g_quadHog.integer && g_gametype.integer == GT_FFA
			&& attacker != targ && !attacker->client->ps.powerups[PW_QUAD] && !client->ps.powerups[PW_QUAD] ) {
			return;
		}

		if ( mod == MOD_PROXIMITY_MINE ) {
			if ( inflictor && inflictor->parent && OnSameTeam( targ, inflictor->parent ) ) {
				return;
			}
			if ( targ == attacker ) {
				return;
			}
		}

		// bot_gauntletOnly: bots do not hurt each other
		if ( bot_gauntletOnly.integer && ( attacker->r.svFlags & SVF_BOT ) && ( targ->r.svFlags & SVF_BOT ) ) {
			return;
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}
	}

	if ( client ) {
		// battlesuit dampens all damage but falling
		if ( client->ps.powerups[PW_BATTLESUIT] ) {
			G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
			if ( mod != MOD_FALLING ) {
				damage = (int)( (float)damage * g_battleSuitDampen.value );
			}
		}
		// spawn protection
		if ( client->ps.powerups[PW_SPAWNARMOR] ) {
			damage = (int)( (float)damage * g_spawnArmorDmgScale.value );
		}
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->client && targ != attacker && targ->health > 0
		&& targ->s.eType != ET_MISSILE && targ->s.eType != ET_GENERAL ) {
		if ( !OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS]++;
			attacker->client->ps.generic1 = ( attacker->client->ps.generic1 & 0x3f ) | ( G_HitTier( damage ) << 6 );
		} else {
			attacker->client->ps.persistant[PERS_HITS]--;
			damage = (int)( (float)damage * g_friendlyFireDampen.value );
		}
	}

	// always give half damage if hurting self, except in instagib
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker && !g_instaGib.integer ) {
		damage = (int)( damage * 0.5 );
	}

	if ( damage < 1 ) {
		damage = 1;
	}

	if ( client ) {
		if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_GUARD ) {
			damage = (int)( (float)damage * 0.5f );
		} else if ( client->ps.stats[STAT_PERSISTANT_POWERUP] == PP_SCOUT && targ == attacker
			&& mod != MOD_KAMIKAZE ) {
			damage = 0;
		}
	}

	// save some from armor
	asave = G_ArmorSave( targ, damage, dflags );
	take = damage - asave;

	// dmflags can spare a player their own health or armor
	if ( g_gametype.integer != GT_CLAN_ARENA && g_gametype.integer != GT_ATTACK_DEFEND && g_dmflags.integer && targ == attacker ) {
		if ( ( g_dmflags.integer & DF_NO_SELF_HEALTH_DAMAGE ) && mod != MOD_KAMIKAZE ) {
			take = 0;
		}
		if ( g_dmflags.integer & DF_NO_SELF_ARMOR_DAMAGE ) {
			asave = 0;
		}
	}

	// a frozen player takes no damage
	if ( client && client->ps.powerups[PW_FROZEN] ) {
		take = 0;
	}

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		if ( dir ) {
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorClear( client->damage_from );
			client->damage_fromWorld = qtrue;
		}
		// set the last client who damaged the target, keeping the hit
		// before it for combo kills
		client->prevhurt_client = client->lasthurt_client;
		client->lasthurt_client = attacker->s.number;
		client->prevhurt_mod = client->lasthurt_mod;
		client->lasthurt_mod = mod;
		client->prevhurt_time = client->lasthurt_time;
		client->lasthurt_time = level.time;
	}

	// See if it's the player hurting the emeny flag carrier
	if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF || g_gametype.integer == GT_ATTACK_DEFEND ) {
		Team_CheckHurtCarrier( targ, attacker );
	}

	// instagib: weapon hits kill outright, splash does nothing, and spawn
	// protection stops everything
	if ( g_instaGib.integer && client ) {
		if ( client->ps.powerups[PW_SPAWNARMOR] ) {
			if ( attacker->client && attacker != targ ) {
				attacker->client->ps.persistant[PERS_HITS]--;
			}
			return;
		}
		switch ( mod ) {
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET_SPLASH:
		case MOD_PLASMA_SPLASH:
		case MOD_BFG_SPLASH:
			if ( attacker->client && attacker != targ ) {
				attacker->client->ps.persistant[PERS_HITS]--;
			}
			return;
		case MOD_SHOTGUN: case MOD_GAUNTLET: case MOD_MACHINEGUN: case MOD_GRENADE: case MOD_ROCKET:
		case MOD_PLASMA: case MOD_RAILGUN: case MOD_LIGHTNING: case MOD_BFG: case MOD_NAIL:
		case MOD_CHAINGUN: case MOD_PROXIMITY_MINE: case MOD_GRAPPLE: case MOD_HMG:
		case MOD_RAILGUN_HEADSHOT:
			targ->health = GIB_HEALTH + 1;
			if ( attacker->client && attacker != targ ) {
				attacker->client->ps.persistant[PERS_KILL_COUNT]++;
			}
			break;
		default:
			break;
		}
	}

	// do the damage
	if ( take || asave ) {
		if ( g_damagePlums.integer && client && attacker->client && attacker != targ && targ->health > 0 ) {
			if ( mod == MOD_SHOTGUN ) {
				// one plum per blast, sent by the shotgun pattern
				attacker->client->shotgunDamage[client->ps.clientNum] += damage;
			} else {
				vec3_t origin;

				VectorCopy( client->ps.origin, origin );
				origin[2] += 32;
				G_DamagePlum( attacker, origin, damage, G_WeaponForMod( mod ) );
			}
		}

		G_StatsDamage( targ, attacker, mod, damage );

		// the attacker's killing blows
		if ( targ->health > 0 && targ->health - take <= 0 && client && attacker->client && attacker != targ ) {
			attacker->client->ps.persistant[PERS_KILL_COUNT]++;
		}

		targ->health = targ->health - take;
		if ( client ) {
			targ->s.health = targ->health;
			client->ps.stats[STAT_HEALTH] = targ->health;
			client->ps.stats[STAT_ARMOR] -= asave;
			targ->s.armor = client->ps.stats[STAT_ARMOR];
		}

		// vampiric damage heals the attacker, up to twice their max health
		if ( g_vampiricDamage.value != 0.0f && attacker->client && client && attacker != targ
			&& !OnSameTeam( targ, attacker ) && attacker->health > 0 && client->ps.pm_type != PM_DEAD
			&& take ) {
			const int limit = attacker->client->ps.stats[STAT_MAX_HEALTH] * 2;

			if ( attacker->health < limit ) {
				const int overkill = targ->health < 0 ? -targ->health : 0;

				attacker->health += (int)( (float)( take - overkill ) * g_vampiricDamage.value );
				if ( attacker->health > limit ) {
					attacker->health = limit;
				}
				attacker->client->ps.stats[STAT_HEALTH] = attacker->health;
				attacker->s.health = attacker->health;
			}
		}

		if ( client && armor_tiered.integer && client->ps.stats[STAT_ARMOR] <= 0 ) {
			client->ps.stats[STAT_ARMOR_TIER] = 0;
		}

		if ( targ->health > 0 ) {
			if ( targ->pain ) {
				targ->pain( targ, attacker, take );
			}
		} else {
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
			}
			if ( targ->health < -999 ) {
				targ->health = -999;
			}
			targ->enemy = attacker;
			targ->die( targ, inflictor, attacker, take, mod );
		}
	}
}


/*
============
G_FreezeTagActive

Retail qagamex86 0x10067e20: freeze tag outside warmup and intermission
(the two level times it tests are written by LogExit and BeginIntermission;
inferred from those writers).
============
*/
qboolean G_FreezeTagActive( void ) {
	return g_gametype.integer == GT_FREEZE && !level.intermissionQueued && !level.intermissiontime
		&& !level.warmupTime;
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.  Retail (qagamex86 0x10049d80) probes the
centre of the target's bounds and four points 15 units off it horizontally.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin ) {
	static const float	probes[4][2] = { { 15, 15 }, { 15, -15 }, { -15, 15 }, { -15, -15 } };
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;
	int		i;

	// use the midpoint of the bounds instead of the origin, because bmodels may have their origin 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	trap_Trace( &tr, origin, vec3_origin, vec3_origin, midpoint, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 || tr.entityNum == targ->s.number ) {
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	for ( i = 0; i < 4; i++ ) {
		VectorCopy( midpoint, dest );
		dest[0] += probes[i][0];
		dest[1] += probes[i][1];
		trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
		if ( tr.fraction == 1.0 ) {
			return qtrue;
		}
	}

	return qfalse;
}


/*
============
G_RadiusDamage

Retail qagamex86 0x1004a090.  The inflictor supplies the damage, radius and
means of death.  With an impact trace the line-of-sight test starts
g_splashdamageOffset off the surface, and a rocket must also see the target
from g_rocketsplashOffset along its flight.  Knockback aims g_knockback_z
(g_knockback_z_self for the attacker) above the target's origin, and with
g_playerCylinders a missile that hit with no travel at all deals its direct
damage to everyone in reach but the attacker.  Damage falls off from the
edge of the target's box, and an entity exactly at the radius is still hit.
============
*/
qboolean G_RadiusDamage( const vec3_t origin, gentity_t *inflictor, gentity_t *ignore,
						 const trace_t *trace, const vec3_t dir ) {
	gentity_t	*attacker = inflictor->parent;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		splashPoint, rocketPoint;
	vec3_t		kick;
	float		radius, dist;
	int			i, e, kickZ;
	qboolean	hitClient = qfalse;

	radius = inflictor->splashRadius;
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	VectorCopy( origin, splashPoint );
	VectorCopy( origin, rocketPoint );
	if ( trace ) {
		for ( i = 0; i < 3; i++ ) {
			splashPoint[i] = (float)( (double)trace->plane.normal[i] * g_splashdamageOffset.value + origin[i] );
		}
		if ( inflictor->s.weapon == WP_ROCKET_LAUNCHER && dir ) {
			for ( i = 0; i < 3; i++ ) {
				rocketPoint[i] = (float)( (double)dir[i] * g_rocketsplashOffset.value + splashPoint[i] );
			}
		}
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore || !ent->takedamage ) {
			continue;
		}

		dist = G_BoxDistance( ent, origin );
		if ( dist > inflictor->splashRadius ) {
			continue;
		}
		if ( !CanDamage( ent, splashPoint ) ) {
			continue;
		}
		if ( inflictor->s.weapon == WP_ROCKET_LAUNCHER && !CanDamage( ent, rocketPoint ) ) {
			continue;
		}

		if ( attacker && LogAccuracyHit( ent, attacker ) ) {
			hitClient = qtrue;
		}

		// push the center of mass higher than the origin so players
		// get knocked into the air more
		VectorSubtract( ent->r.currentOrigin, origin, kick );
		kickZ = ( ent == attacker ) ? g_knockback_z_self.integer : g_knockback_z.integer;
		kick[2] = (float)( (double)kickZ + kick[2] );

		if ( g_playerCylinders.integer && trace && trace->fraction == 0.0f && ent != attacker ) {
			G_Damage( ent, inflictor, attacker, kick, (float *)origin, inflictor->damage, 0, inflictor->methodOfDeath );
		} else {
			G_Damage( ent, inflictor, attacker, kick, (float *)origin,
				(int)( ( 1.0 - (double)dist / inflictor->splashRadius ) * inflictor->splashDamage ),
				DAMAGE_RADIUS, inflictor->splashMethodOfDeath );
		}
	}

	return hitClient;
}
