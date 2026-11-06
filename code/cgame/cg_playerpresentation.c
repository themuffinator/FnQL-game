// Copyright (C) 1999-2000 Id Software, Inc.
//
// Player animation and powerup passes, extracted from cg_players.c.
// FnQL-game frozen-player behavior is independently written from retail evidence.

#include "cg_local.h"
#include <stdint.h>

void CG_UpdatePlayerPresentationSettings(void) {
	const char *p = CG_ConfigString(CS_SETTINGS_MASK);
	uint32_t mask = 0;
	cgs.noPlayerClipPresentation = qfalse;
	cgs.settingsMask = 0;
	if (!p) return;
	while (*p && (unsigned char)*p <= ' ') ++p;
	if (*p == '+') ++p;
	if (*p < '0' || *p > '9') return;
	do {
		unsigned digit = (unsigned)(*p++ - '0');
		if (mask > (UINT32_MAX - digit) / 10) return;
		mask = mask * 10 + digit;
	} while (*p >= '0' && *p <= '9');
	while (*p && (unsigned char)*p <= ' ') ++p;
	if (*p) return;
	// Retail 1001ce72 / 1004a077 keep CS 2c0 in one word (0x10a3ff28);
	// 10041878 tests bit 25, not gametype. EV_POI reads bits 18 and 26.
	cgs.settingsMask = mask;
	cgs.noPlayerClipPresentation = (mask & (1u << 25)) != 0;
}

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}


/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n");
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	/* Retail 1003f057: frozen takes precedence over haste. */
	if ( cent->currentState.powerups & ( 1 << PW_FROZEN ) ) {
		speedScale = 0;
	} else if ( cent->currentState.powerups & ( 1 << PW_HASTE ) ) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team ) {

	// Retail 10041878: the local client retains normal body rendering;
	// other owners use a single no-clip pass ahead of all powerup materials.
	if (cgs.noPlayerClipPresentation && state->clientNum != cg.clientNum && cgs.media.noPlayerClipShader) {
		ent->customShader = cgs.media.noPlayerClipShader;
		trap_R_AddRefEntityToScene(ent);
	} else if ( state->powerups & ( 1 << PW_INVIS ) ) {
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
	} else {
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
			if (team == TEAM_BLUE)
				ent->customShader = cgs.media.blueKamikazeShader;
			else
				ent->customShader = cgs.media.redKamikazeShader;
			trap_R_AddRefEntityToScene( ent );
		}
		else {*/
			trap_R_AddRefEntityToScene( ent );
		//}

		if ( state->powerups & ( 1 << PW_QUAD ) )
		{
			if (team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene( ent );
		}
		if ( state->powerups & ( 1 << PW_REGEN ) ) {
			if ( ( ( cg.time / 100 ) % 10 ) == 1 ) {
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene( ent );
			}
		}
		if ( state->powerups & ( 1 << PW_BATTLESUIT ) ) {
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene( ent );
		}
		/* Retail 1004198c: generic1 bit 1 selects ice1, then bit 0
		 * selects ice2; neither selects ice3. Preserve other powerup passes. */
		if ( state->powerups & ( 1 << PW_FROZEN ) ) {
			int stage = state->generic1 & 2 ? 0 : state->generic1 & 1 ? 1 : 2;
			if (cgs.media.icePlayerShader[stage]) {
				ent->customShader = cgs.media.icePlayerShader[stage];
				trap_R_AddRefEntityToScene(ent);
			}
		}
	}
}

/* Retail 10052350 treats the snapshot's viewed client as the local weapon,
 * unlike the body helper's persistent local-client comparison. */
void CG_AddWeaponWithPowerups(refEntity_t *gun, const entityState_t *state) {
	int viewedClient = cg.snap ? cg.snap->ps.clientNum : cg.clientNum;
	qhandle_t replacement = 0;
	if (cgs.noPlayerClipPresentation && state->clientNum != viewedClient) {
		replacement = cgs.media.noPlayerClipShader;
	}
	if (!replacement && (state->powerups & (1 << PW_INVIS))) replacement = cgs.media.invisShader;
	if (!replacement && state->clientNum == viewedClient && cg_drawGun.integer == 3) {
		replacement = cgs.media.ghostWeaponShader;
	}
	if (replacement) {
		gun->customShader = replacement;
		trap_R_AddRefEntityToScene(gun);
		return;
	}
	trap_R_AddRefEntityToScene(gun);
	if (state->powerups & (1 << PW_BATTLESUIT)) {
		gun->customShader = cgs.media.battleWeaponShader;
		trap_R_AddRefEntityToScene(gun);
	}
	if (state->powerups & (1 << PW_QUAD)) {
		gun->customShader = cgs.media.quadWeaponShader;
		trap_R_AddRefEntityToScene(gun);
	}
}


/*
===============
CG_PlayerFloatSprite

Retail 0x10040cd0. A teammate's sprite becomes a screen-space marker
drawn through walls, tinted by health and sized by the cg_teammatePOIs
widths; Red Rover infection shows infected players to survivors with an
alpha that falls with their speed. Everyone else gets the inherited
48-unit, radius-10 world sprite.
===============
*/
static void CG_PlayerFloatSprite( const centity_t *cent, qhandle_t shader, qboolean teammate ) {
	qboolean self = cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson;
	int				rf;
	refEntity_t		ent;

	if ( teammate ) {
		vec4_t color = { 1, 1, 1, 1 };
		int health = cent->currentState.health, maximum = cg.snap->ps.stats[STAT_MAX_HEALTH];
		if ( self ) return;
		// 0x10040d2f: a spectating or dead local player (pm_type & 2) sees
		// plain yellow; otherwise 0x10040d36 tints red to yellow below the
		// maximum and yellow to white above it
		if ( cg.predictedPlayerState.pm_type & 2 ) {
			color[2] = 0;
		} else if ( health < maximum ) {
			float fraction = maximum > 0 ? (float)health / maximum : 0;
			color[1] = fraction > 0 ? fraction : 0;
			color[2] = 0;
		} else {
			float fraction = maximum > 0 ? (float)health / ( 2.0f * maximum ) : 1;
			color[2] = fraction > 1 || !( fraction == fraction ) ? 1 : fraction;
		}
		CG_AddTeammateMarker( cent->lerpOrigin, color, shader );
		return;
	}
	if ( cgs.gametype == GT_RED_ROVER && ( cgs.settingsMask & CG_SETTING_RR_INFECTED ) ) {
		// 0x10040e40: survivors only; speed is the trajectory delta
		vec4_t color = { 1, 1, 1, 1 };
		float minimum = atof( CG_ConfigString( CS_RR_PING_RATE ) );
		if ( cg.clientNum < 0 || cg.clientNum >= MAX_CLIENTS ||
			cgs.clientinfo[cg.clientNum].team != TEAM_RED || self ) return;
		if ( minimum > 0 ) {
			float alpha = ( minimum - VectorLength( cent->currentState.pos.trDelta ) ) / minimum;
			color[3] = alpha > 0 ? alpha : 0;
		}
		CG_AddTeammateMarker( cent->lerpOrigin, color, shader );
		return;
	}

	rf = self ? RF_THIRD_PERSON : 0;	// only show in mirrors

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerSprites

Retail 0x100411a0 (called when cg_drawSprites is set): connection, One
Flag and flag carriers, frozen teammates, talk, medals, Red Rover
infection and finally teammates, first match wins. Infected enemies hide
their talk balloon and the first three medals in Red Rover infection.
===============
*/
void CG_PlayerSprites( centity_t *cent ) {
	const entityState_t *s = &cent->currentState;
	int team, flags = s->eFlags, carriers;
	qboolean teammate, infectedEnemy, hit;

	if ( !cg_drawSprites.integer ) return;
	/* Retail 10041205: self sprites are opt-in, including third person. */
	if ( !cg_drawSpriteSelf.integer && s->clientNum == cg.clientNum ) return;
	if ( s->clientNum < 0 || s->clientNum >= MAX_CLIENTS ) return;

	team = cgs.clientinfo[ s->clientNum ].team;
	teammate = cgs.gametype >= GT_TEAM && cg.snap->ps.persistant[PERS_TEAM] == team;
	hit = cg.time - cent->pe.painTime < 1500;

	if ( flags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader, teammate );
		return;
	}
	carriers = ( 1 << PW_REDFLAG ) | ( 1 << PW_BLUEFLAG ) | ( 1 << PW_NEUTRALFLAG );
	if ( cgs.gametype == GT_1FCTF && teammate && ( s->powerups & ( 1 << PW_NEUTRALFLAG ) ) ) {
		CG_PlayerFloatSprite( cent, cgs.poiMedia.neutralFlag, teammate );
		return;
	}
	if ( cgs.gametype >= GT_TEAM && cg_teammatePOIs.integer && !cg_teammateNames.integer &&
		teammate && ( s->powerups & carriers ) ) {
		CG_PlayerFloatSprite( cent, hit ? cgs.media.flagCarrierHitShader : cgs.media.flagCarrierShader, teammate );
		return;
	}
	if ( cgs.gametype == GT_FREEZE && teammate && ( s->powerups & ( 1 << PW_FROZEN ) ) ) {
		CG_PlayerFloatSprite( cent, cgs.media.frozenSpriteShader, teammate );
		return;
	}

	infectedEnemy = cgs.gametype == GT_RED_ROVER && ( cgs.settingsMask & CG_SETTING_RR_INFECTED ) &&
		team == TEAM_BLUE && !teammate;
	if ( ( flags & EF_TALK ) && !infectedEnemy && !( flags & EF_DEAD ) ) {
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader, teammate );
		return;
	}
	if ( ( flags & EF_AWARD_IMPRESSIVE ) && !infectedEnemy ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive, teammate );
		return;
	}
	if ( ( flags & EF_AWARD_EXCELLENT ) && !infectedEnemy ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent, teammate );
		return;
	}
	if ( ( flags & EF_AWARD_GAUNTLET ) && !infectedEnemy ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet, teammate );
		return;
	}
	if ( flags & EF_AWARD_DEFEND ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend, teammate );
		return;
	}
	if ( flags & EF_AWARD_ASSIST ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist, teammate );
		return;
	}
	if ( flags & EF_AWARD_CAP ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture, teammate );
		return;
	}
	if ( cgs.gametype == GT_RED_ROVER && ( cgs.settingsMask & CG_SETTING_RR_INFECTED ) && team == TEAM_BLUE &&
		cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS && cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
		CG_PlayerFloatSprite( cent, cgs.poiMedia.infected, teammate );
		return;
	}
	if ( cgs.gametype >= GT_TEAM && cg_teammatePOIs.integer && !cg_teammateNames.integer &&
		teammate && !( flags & EF_DEAD ) ) {
		CG_PlayerFloatSprite( cent, cg.time - cent->pe.painTime >= cg_drawHitFriendTime.integer ?
			cgs.media.friendShader : cgs.media.friendHitShader, teammate );
	}
}


