// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"
#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
extern displayContextDef_t cgDC;
#endif


/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like 
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}

	if ( !cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model '%s'.\n", cg.testModelName );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================


/*
===============
CG_OffsetThirdPersonView

===============
*/
#define	FOCUS_DISTANCE	512
/* Retail 0x1004c6f0: the smart camera's trace box, half the diagonal of the
 * near plane: V is the vertical FOV of cg_fov over 640x480, H its horizontal
 * FOV for the r_aspectRatio preset (16:9, 16:10, else 5:4; preset 0 keeps
 * cg_fov itself, as retail's helper returns its input there). */
static float CG_ThirdPersonBoxSize( void ) {
	char text[32];
	double vertical, horizontal, znear;
	int preset;
	vertical = atan2( 480.0, 640.0 / tan( cg_fov.value * M_PI / 360.0 ) ) * 360.0 / M_PI;
	trap_Cvar_VariableStringBuffer( "r_aspectRatio", text, sizeof( text ) );
	preset = atoi( text );
	if ( preset ) {
		const double aspect = preset == 1 ? 16.0 / 9.0 : preset == 2 ? 16.0 / 10.0 : 5.0 / 4.0;
		horizontal = atan( tan( vertical * M_PI / 360.0 ) * aspect ) * 360.0 / M_PI;
	} else {
		horizontal = cg_fov.value;
	}
	trap_Cvar_VariableStringBuffer( "r_znear", text, sizeof( text ) );
	znear = atof( text );
	return (float)sqrt( znear * znear + ( znear * tan( horizontal * M_PI / 360.0 ) ) * ( znear * tan( horizontal * M_PI / 360.0 ) ) );
}

/* Retail 0x1004ca50 (with 0x1004c8f0 for cg_thirdPerson 2):
 * - Mode 2 is a fixed camera: eye height, cg_thirdPersonAngle added to the
 *   yaw, cg_thirdPersonPitch (unless -1) as the pitch, then
 *   cg_thirdPersonRange straight back; no trace.
 * - Otherwise duck smoothing applies first; a spectator gets no offset.
 *   The focus pitch is capped at 45 only while dead, and the dead view no
 *   longer turns to the killer. Landing smoothing follows the eye height.
 * - The range offset applies with cg_thirdPerson, cg_cameraSmartMode or a
 *   dead player. Without com_cameraMode the plain Quake III trace (box 4)
 *   runs unless cg_thirdPerson uses cg_cameraThirdPersonSmartMode, the
 *   smart mode is on, or the player is dead; then the smart trace runs back
 *   from the player along the view (box from CG_ThirdPersonBoxSize) and
 *   keeps the camera r_znear in front of what it hits. */
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist, dt;
	float		forwardScale, sideScale;
	const qboolean spectator = cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR;
	const qboolean dead = cg.predictedPlayerState.stats[STAT_HEALTH] <= 0;

	if ( cg_thirdPerson.integer == 2 ) {
		float pitch, yaw;
		cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
		if ( cg_thirdPersonAngle.value != 0 ) {
			cg.refdefViewAngles[YAW] += cg_thirdPersonAngle.value;
		}
		if ( cg_thirdPersonPitch.value != -1.0f ) {
			cg.refdefViewAngles[PITCH] = cg_thirdPersonPitch.value;
		}
		pitch = cg.refdefViewAngles[PITCH] * 0.017453292f;
		yaw = cg.refdefViewAngles[YAW] * 0.017453292f;
		forward[0] = cos( pitch ) * cos( yaw );
		forward[1] = cos( pitch ) * sin( yaw );
		forward[2] = -sin( pitch );
		VectorMA( cg.refdef.vieworg, -cg_thirdPersonRange.value, forward, cg.refdef.vieworg );
		return;
	}

	dt = (float)( cg.time - cg.duckTime );
	if ( dt < 100 ) {
		cg.refdef.vieworg[2] -= ( 100 - dt ) * cg.duckChange / 100;
	}
	if ( spectator ) {
		return;
	}

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( cg.refdefViewAngles, focusAngles );
	if ( dead && focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}

	dt = (float)( cg.time - cg.landTime );
	if ( dt < 100 ) {
		cg.refdef.vieworg[2] -= cg.landChange * ( 100 - dt ) / 100;
	}

	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	if ( cg_thirdPerson.integer || cg_cameraSmartMode.integer || dead ) {
		forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
		sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
		VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
		VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );
	}

	if (!cg_cameraMode.integer) {
		const qboolean plain = ( !cg_thirdPerson.integer || !cg_cameraThirdPersonSmartMode.integer ) &&
			!cg_cameraSmartMode.integer && !dead;
		if ( plain ) {
			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
			if ( trace.fraction != 1.0 ) {
				VectorCopy( trace.endpos, view );
				view[2] += (1.0 - trace.fraction) * 32;
				// try another trace to this position, because a tunnel may have the ceiling
				// close enough that this is poking out
				CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
				VectorCopy( trace.endpos, view );
			}
		} else {
			vec3_t start, boxMins, boxMaxs, offset;
			char text[32];
			const float size = CG_ThirdPersonBoxSize();
			VectorSet( boxMins, -size, -size, -size );
			VectorSet( boxMaxs, size, size, size );
			VectorMA( view, cg_thirdPersonRange.value, forward, start );
			CG_Trace( &trace, start, boxMins, boxMaxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
			if ( trace.fraction < 1.0f ) {
				const float first = trace.fraction;
				trap_Cvar_VariableStringBuffer( "r_znear", text, sizeof( text ) );
				VectorScale( forward, (float)atof( text ), offset );
				VectorAdd( trace.endpos, offset, view );
				view[2] += ( 1.0f - first ) * 32;
				CG_Trace( &trace, start, boxMins, boxMaxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
				if ( trace.fraction < 1.0f ) {
					VectorAdd( trace.endpos, offset, view );
				}
			}
		}
	}

	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = atan2( focusPoint[2], focusDist ) * -57.2957763671875f;
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	int				timeDelta;
	
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// retail 0x1004d160: cg_trackPlayer turns the view to that client
	if ( cg_trackPlayer.integer != -1 && cg_trackPlayer.integer >= 0 && cg_trackPlayer.integer < MAX_CLIENTS &&
		cgs.clientinfo[cg_trackPlayer.integer].infoValid ) {
		vec3_t toward;
		VectorSubtract( cg_entities[cg_trackPlayer.integer].lerpOrigin, cg.refdef.vieworg, toward );
		vectoangles( toward, cg.refdefViewAngles );
	}

	// if dead, fix the angle and don't add any kick (retail keeps the yaw)
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	VectorAdd (angles, cg.kick_angles, angles);

	// add angles based on damage kick.  Retail (cgame 0x1004d232-0x1004d2d7)
	// has no kick at all with cg_kickScale 0, and scales only the deflection
	// by it: the return phase, which joins after the multiply (0x1004d2c9 to
	// 0x1004d289), starts from the whole angle.
	if ( cg.damageTime && cg_kickScale.value != 0.0f ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio = ratio * cg_kickScale.value / DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else if ( ratio < DAMAGE_DEFLECT_TIME + DAMAGE_RETURN_TIME ) {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// Velocity lean and bob.  Retail (cgame 0x1004d2d9-0x1004d47c) drives all
	// of it from one cvar, cg_bob (table row 10076b80, default 0.25), where
	// Quake III had cg_runpitch, cg_runroll, cg_bobpitch, cg_bobroll and
	// cg_bobup: zero turns the lean off with the bob; otherwise the lean keeps
	// Quake III's constants (0.002 pitch, 0.005 roll, the float constants at
	// 0x10073c40 and 0x10073c38), and the bob angles and height are the one
	// cg_bob-scaled swing times 0.002 and 0.005.  Crouching doubles the swing
	// (Quake III tripled it) and the height stops at 5 units (Quake III: 6).
	if ( cg_bob.value != 0.0f ) {
		// make sure the bob is visible even at low speeds
		speed = cg.xyspeed > 200 ? cg.xyspeed : 200;
		delta = cg.bobfracsin * cg_bob.value * speed;

		VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );
		angles[PITCH] += DotProduct( predictedVelocity, cg.refdef.viewaxis[0] ) * 0.002f;
		angles[ROLL] -= DotProduct( predictedVelocity, cg.refdef.viewaxis[1] ) * 0.005f;

		if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED ) {
			delta += delta;		// crouching
		}
		angles[PITCH] += delta * 0.002f;
		if ( cg.bobcycle & 1 ) {
			angles[ROLL] -= delta * 0.002f;
		} else {
			angles[ROLL] += delta * 0.002f;
		}

		// add bob height
		bob = delta * 0.005f;
		if ( bob < 0 ) {
			bob = 0;
		} else if ( bob > 5 ) {
			bob = 5;
		}
		origin[2] += bob;
	}

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;
 
	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void ) {
	int			t;
	int			maxTime;
	refEntity_t		ent;

	// Retail 0x1004d930 has no blood gate: the flash takes the damage colour
	// (cg.damageColor), twice the damage value as radius, and the gore DLC's
	// viewBloodBlend only when the DLC is installed, else viewDamageBlend.
	if ( !cg.damageValue ) {
		return;
	}

	//if (cg.cameraMode) {
	//	return;
	//}

	// ragePro systems can't fade blends, so don't obscure the screen
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}


	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue + cg.damageValue;
	ent.customShader = cgs.media.dlcBloodSpray[0] ? cgs.media.dlcViewBloodBlend : cgs.media.viewDamageBlend;
	ent.shaderRGBA[0] = ( cg.damageColor >> 24 ) & 0xff;
	ent.shaderRGBA[1] = ( cg.damageColor >> 16 ) & 0xff;
	ent.shaderRGBA[2] = ( cg.damageColor >> 8 ) & 0xff;
	ent.shaderRGBA[3] = (int)( ( 1.0 - (float)t / 500.0 ) * ( cg.damageColor & 0xff ) );
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
/* Retail 0x1004db20 (cg_filter_angles N, from 0x1004dd00): a moving
 * average of the predicted view yaw and pitch over the last N frames
 * (at most 32), unwrapping the stored yaws when the yaw jumps by more than
 * 350 degrees. A negative N resets the cvar to 0; above 32 it is set to 31
 * (the frame still uses N, capped at 32). Changing the cvar clears the
 * history. */
static void CG_FilterViewAngles( vec3_t angles ) {
	static float yaws[32], pitches[32], lastYaw;
	static int count, head, modification = -1;
	int n = cg_filter_angles.integer, i, slot;
	float delta, sumYaw = 0, sumPitch = 0;
	if ( n < 0 ) {
		trap_Cvar_Set( "cg_filter_angles", "0" );
		return;
	}
	if ( n > 32 ) {
		trap_Cvar_Set( "cg_filter_angles", "31" );
		n = 32;
	}
	if ( modification != cg_filter_angles.modificationCount ) {
		memset( yaws, 0, sizeof( yaws ) );
		memset( pitches, 0, sizeof( pitches ) );
		count = head = 0;
		lastYaw = 0;
		modification = cg_filter_angles.modificationCount;
	}
	delta = angles[YAW] - lastYaw;
	if ( fabs( delta ) > 350 && count > 1 ) {
		for ( i = 1; i < count; i++ ) {
			slot = head - i;
			if ( slot < 0 ) slot += 32;
			yaws[slot] += delta >= 0 ? 360.0f : -360.0f;
		}
	}
	lastYaw = angles[YAW];
	yaws[head] = angles[YAW];
	pitches[head] = angles[PITCH];
	if ( ++count > n ) count = n;
	for ( i = 0; i < count; i++ ) {
		slot = head - i;
		if ( slot < 0 ) slot += 32;
		sumYaw += yaws[slot];
		sumPitch += pitches[slot];
	}
	if ( count > 0 ) {
		angles[YAW] = sumYaw / count;
		angles[PITCH] = sumPitch / count;
	}
	head = ( head + 1 ) & 31;
}

static int CG_CalcViewValues( void ) {
	playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap_getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1] );
	CG_SampleSpeed( cg.xyspeed );	// retail 0x1004de3e


	VectorCopy( ps->origin, cg.refdef.vieworg );
	if ( cg_filter_angles.integer ) {
		CG_FilterViewAngles( cg.predictedPlayerState.viewangles );
	}
	VectorCopy( ps->viewangles, cg.refdefViewAngles );

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson ) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye relative to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;

	// clear all buffered sounds
	if ( sfx == -1 ) {
		cg.soundBufferIn = 0;
		cg.soundBufferOut = 0;
		memset( cg.soundBuffer, 0, sizeof( cg.soundBuffer ) );
		return;
	}
	/* Retail 1004e110 stops new calls after the match result, while the queued
	 * result itself remains eligible to play. map_restart reopens the queue. */
	if (cg.matchEnded) return;

	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		//cg.soundBufferOut++;
		cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			cg.soundPlaying = cg.soundBuffer[cg.soundBufferOut];
			trap_S_StartLocalSound( cg.soundPlaying, CHAN_ANNOUNCER );
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		} else {
			cg.soundPlaying = 0;
		}
	}
}

//=========================================================================


/*
=================
CG_FirstFrame

Called once on first rendered frame
=================
*/
static void CG_FirstFrame( void )
{
	CG_SetConfigValues();

	cgs.voteTime = atoi( CG_ConfigString( CS_VOTE_TIME ) );
	cgs.voteYes = atoi( CG_ConfigString( CS_VOTE_YES ) );
	cgs.voteNo = atoi( CG_ConfigString( CS_VOTE_NO ) );
	Q_strncpyz( cgs.voteString, CG_ConfigString( CS_VOTE_STRING ), sizeof( cgs.voteString ) );

	if ( cgs.voteTime )
		cgs.voteModified = qtrue;
	else
		cgs.voteModified = qfalse;
}


/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	if ( cg.clientFrame == 0 )
		CG_FirstFrame();

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	CG_TrackClientTeamChange();

	// follow killer
	if ( cg.followTime && cg.followTime < cg.time ) {
		cg.followTime = 0;
		if ( !cg.demoPlayback ) {
			trap_SendConsoleCommand( va( "follow %i\n", cg.followClient ) );
		}
	}

	// build cg.refdef
	inwater = CG_CalcViewValues();

	// Retail 1004e597-1004e5c6 publishes this frame's unadjusted command FOV.
	trap_SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity );

	// Retail 1004e5d1: cg_spectating (read by the engine and the about
	// menu) follows the snapshot player's free-spectator state.
	if ( cg_spectating.integer && cg.snap->ps.pm_type != PM_SPECTATOR ) {
		trap_Cvar_Set( "cg_spectating", "0" );
		trap_Cvar_Update( &cg_spectating );
	} else if ( !cg_spectating.integer && cg.snap->ps.pm_type == PM_SPECTATOR ) {
		trap_Cvar_Set( "cg_spectating", "1" );
		trap_Cvar_Update( &cg_spectating );
	}

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities();	// alter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddParticles ();
		CG_AddLocalEntities();
	}
	CG_UpdateWeaponSelection();
	CG_AddViewWeapon( &cg.predictedPlayerState );

	// add buffered sounds
	CG_PlayBufferedSounds();

#ifdef MISSIONPACK
	// play buffered voice chats
	CG_PlayBufferedVoiceChats();
#endif

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// update audio positions
	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
		CG_UpdateTimedItems( cg.frametime );	// retail 0x1004e80b
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	// actually issue the rendering calls
#ifdef MISSIONPACK
	cgDC.realTime = cg.time;
	cgDC.frameTime = cg.frametime;
#endif
	CG_DrawActive( stereoView );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}
}
