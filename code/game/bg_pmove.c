// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t		*pm;
pml_t		pml;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.50f;
float	pm_wadeScale = 0.70f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

int		c_pmove = 0;

// Quake Live movement constants (retail qagamex86 data, see PM_SetMovementProfile)
#define PM_CROUCH_SLIDE_SCALE	0.75	// wish speed cap while crouch sliding
#define PM_SLICK_CROUCH_ACCEL	1.4f	// ground acceleration crouched on SURF_SLICK
#define PM_BUNNYHOP_ACCEL		0.55f	// pmove_BunnyHop forward boost per move
#define PM_LADDER_FRICTION		6.0
#define PM_FREEZE_FRICTION		0.25	// friction scale while frozen
#define PM_LADDER_SCALE			0.6600000262260437	// (float)0.66: ladder speed fraction
#define PM_NOCLIP_FRICTION		10.0

/*
==============================================================================

Retail arithmetic

The retail modules evaluate pmove expressions in x87 registers at double
precision and round to float only where they store a value.  Expressions in
this file compute in double, in the retail operation order, and round where
retail stores, so x87 and SSE2 builds alike reproduce retail movement bit for
bit (gate G3, tests/lockstep).  The helpers below are the retail forms of the
shared math routines Pmove calls.

==============================================================================
*/

// the degrees-to-radians factor retail AngleVectors multiplies by: (float)( M_PI / 180 )
#define PM_DEG2RAD	0.01745329238474369

// ( a0 * b0 + a1 * b1 ) + a2 * b2, unrounded
double PM_Dot3( const vec3_t a, const vec3_t b ) {
	return (double)a[0] * b[0] + (double)a[1] * b[1] + (double)a[2] * b[2];
}

float PM_VectorNormalize( vec3_t v ) {
	const float lengthSq = (float)PM_Dot3( v, v );
	const float length = (float)sqrt( lengthSq );
	float ilength;

	if ( length != 0.0f ) {
		ilength = (float)( 1.0 / length );
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
}

float PM_VectorNormalize2( const vec3_t v, vec3_t out ) {
	const float lengthSq = (float)PM_Dot3( v, v );
	const float length = (float)sqrt( lengthSq );
	float ilength;

	if ( length != 0.0f ) {
		ilength = (float)( 1.0 / length );
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	} else {
		VectorClear( out );
	}
	return length;
}

void PM_CrossProduct( const vec3_t a, const vec3_t b, vec3_t cross ) {
	cross[0] = (float)( (double)a[1] * b[2] - (double)a[2] * b[1] );
	cross[1] = (float)( (double)a[2] * b[0] - (double)a[0] * b[2] );
	cross[2] = (float)( (double)a[0] * b[1] - (double)a[1] * b[0] );
}

void PM_AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up ) {
	float angle, sy, cy, sp, cp, sr, cr;

	angle = (float)( angles[YAW] * PM_DEG2RAD );
	sy = (float)sin( angle );
	cy = (float)cos( angle );
	angle = (float)( angles[PITCH] * PM_DEG2RAD );
	sp = (float)sin( angle );
	cp = (float)cos( angle );
	angle = (float)( angles[ROLL] * PM_DEG2RAD );
	sr = (float)sin( angle );
	cr = (float)cos( angle );

	// any output may be NULL, as in retail
	if ( forward ) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if ( right ) {
		right[0] = (float)( (double)-sy * -cr - (double)cy * ( (double)sr * sp ) );
		right[1] = (float)( (double)-cr * cy - ( (double)sr * sp ) * sy );
		right[2] = -sr * cp;
	}
	if ( up ) {
		up[0] = (float)( ( (double)cr * sp ) * cy - (double)-sy * sr );
		up[1] = (float)( (double)sy * ( (double)cr * sp ) - (double)cy * sr );
		up[2] = cr * cp;
	}
}

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps, -1 );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}

	if ( pm->numtouch >= MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	int		i;

	backoff = (float)PM_Dot3( in, normal );

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		out[i] = (float)( in[i] - (double)normal[i] * backoff );
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control, friction;
	float	drop;
	const double frametime = pml.frametime;
	const int flags = pm->ps->pm_flags;
	const int dir = pm->ps->movementDir;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = (float)sqrt( PM_Dot3( vec, vec ) );
	if ( speed < 1.0f ) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		return;
	}

	drop = 0;

	// apply ground friction, unless on ice or getting knocked back
	if ( pm->waterlevel <= 1 && pml.walking && !( pml.groundTrace.surfaceFlags & SURF_SLICK )
		&& !( flags & PMF_TIME_KNOCKBACK ) ) {
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		if ( ( flags & PMF_CROUCH_SLIDE ) && pm->cmd.upmove < 0 && pm->ps->crouchSlideTime > 0 ) {
			friction = pm->settings->crouchSlideFriction;
		} else if ( ( flags & PMF_AIR_CONTROL ) && pm->cmd.forwardmove && pm->cmd.rightmove
			&& ( dir == 1 || dir == 3 || dir == 5 || dir == 7 ) ) {
			friction = pml.profile.circleStrafeFriction;	// PQL circle strafing
		} else {
			friction = pm->settings->walkFriction;
		}
		drop = (float)( frametime * control * friction + drop );
	}

	// apply water friction even if just wading
	if ( pm->waterlevel && !pml.ladder ) {
		drop = (float)( frametime * speed * pm->waterlevel + drop );
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		drop = (float)( frametime * speed * pm_spectatorfriction + drop );
	}

	// retail scales friction down while frozen and for pm_flags 0x2000
	if ( pm->ps->pm_type == PM_FREEZE || ( pm->ps->pm_flags & PMF_SCOREBOARD ) ) {
		drop = (float)( drop * PM_FREEZE_FRICTION );
	}

	if ( pml.ladder ) {
		drop = (float)( speed * PM_LADDER_FRICTION * frametime + drop );
	}

	// scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( const vec3_t wishdir, float wishspeed, float accel ) {
	// q2 style
	float		*vel = pm->ps->velocity;
	const float	addspeed = (float)( wishspeed - PM_Dot3( wishdir, vel ) );
	const float	speed = (float)pm->ps->speed;
	const float	hspeed = (float)sqrt( (double)vel[0] * vel[0] + (double)vel[1] * vel[1] );
	float		accelspeed, limit;
	int			i;

	if ( addspeed > 0 ) {
		accelspeed = (float)( (double)pml.frametime * wishspeed * accel );
		if ( accelspeed > addspeed ) {
			accelspeed = addspeed;
		}
		for ( i = 0; i < 3; i++ ) {
			vel[i] = (float)( (double)wishdir[i] * accelspeed + vel[i] );
		}
		return;
	}

	// pmove_BunnyHop: moving straight ahead at or past the wish speed still
	// gains a little each move, tapering to nothing at twice the run speed
	if ( !pm->settings->bunnyHop || pm->ps->movementDir != 0 ) {
		return;
	}
	limit = speed + speed;
	if ( hspeed > limit ) {
		return;
	}
	accelspeed = PM_BUNNYHOP_ACCEL;
	if ( hspeed > speed ) {
		accelspeed = (float)( ( (double)limit - hspeed ) / speed * (double)PM_BUNNYHOP_ACCEL );
	}
	for ( i = 0; i < 3; i++ ) {
		vel[i] = (float)( (double)wishdir[i] * accelspeed + vel[i] );
	}
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		} 
	}
}


/*
=============
PM_JumpVelocity

Quake Live jump strength (retail PM_DoJump, qagamex86 0x1002e2c0).  A jump
within pmove_JumpVelocityTimeThreshold of the previous one is boosted by the
profile's chain jump mode: 1 scales the base velocity up by up to
pmove_JumpVelocityScaleAdd, 2 adds a constant (the step jump velocity for
step jumps), and 3 adds the constant inside the offset window and a
proportional amount after it.
=============
*/
static float PM_JumpVelocity( void ) {
	const pmoveSettings_t *s = pm->settings;
	const float base = s->jumpVelocity;
	const float window = s->jumpVelocityTimeThreshold;
	const float since = (float)( pm->cmd.serverTime - pm->ps->jumpTime );
	double scaledWindow;

	if ( !( since > 0.0f ) || !( window > since ) ) {
		return base;
	}

	switch ( pml.profile.chainJump ) {
	case 1:
		return (float)( ( ( (double)window - since ) / window * s->jumpVelocityScaleAdd + 1.0 ) * base );
	case 2:
		break;
	case 3:
		scaledWindow = (double)window * s->jumpVelocityTimeThresholdOffset;
		if ( !( scaledWindow > since ) ) {
			return (float)( ( scaledWindow - since ) / scaledWindow * s->chainJumpVelocity
				+ s->chainJumpVelocity + base );
		}
		break;
	default:
		return base;
	}
	return pml.stepJumping ? s->stepJumpVelocity + base : s->chainJumpVelocity + base;
}


/*
=============
PM_DoJump

Also the step jump and crouch step jump of PM_StepSlideMove, which set
pml.stepJumping and pml.noRampJump around the call.
=============
*/
void PM_DoJump( void ) {
	const pmoveSettings_t *s = pm->settings;
	const float velocity = PM_JumpVelocity();
	float *vel = pm->ps->velocity;

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	if ( s->rampJump && !pml.noRampJump ) {
		// ramp jump: keep part of the upward speed the player already had
		vel[2] *= s->rampJumpScale;
		vel[2] += velocity;
		if ( velocity > vel[2] ) {
			vel[2] = velocity;
		}
		if ( vel[2] > s->jumpVelocityMax ) {
			vel[2] = s->jumpVelocityMax;
		}
	} else {
		vel[2] = velocity;
	}

	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	pm->ps->jumpTime = pm->cmd.serverTime;
}


/*
=============
PM_CanJump

The Quake Live jump gate (retail qagamex86 0x1002e510): holding jump
re-jumps under pmove_AutoHop unless the player asked for release-to-jump,
and jumps are at least pmove_JumpTimeDeltaMin apart.  A held jump that may
not re-jump clears upmove so PM_CmdScale doesn't lower the running speed.
=============
*/
qboolean PM_CanJump( void ) {
	const pmoveSettings_t *s = pm->settings;
	const int flags = pm->ps->pm_flags;
	const float since = (float)( pm->cmd.serverTime - pm->ps->jumpTime );

	if ( ( !s->autoHop || ( flags & PMF_REQUIRE_JUMP_RELEASE ) ) && ( flags & PMF_RESPAWNED ) ) {
		return qfalse;		// don't allow jump until all buttons are up
	}
	if ( pm->cmd.upmove < 10 ) {
		return qfalse;		// not holding jump
	}
	if ( s->jumpTimeDeltaMin > since ) {
		return qfalse;
	}
	if ( ( !s->autoHop && ( flags & PMF_JUMP_HELD ) )
		|| ( ( flags & PMF_REQUIRE_JUMP_RELEASE ) && ( flags & PMF_JUMP_HELD ) ) ) {
		pm->cmd.upmove = 0;	// must wait for jump to be released
		return qfalse;
	}
	return qtrue;
}


/*
=============
PM_CanCrouchStepJump

A crouch step jump (pmove_CrouchStepJump) needs a crouched player rising
through the air at least pmove_JumpVelocityTimeThreshold after the last jump
(retail qagamex86 0x1002e4c0).
=============
*/
qboolean PM_CanCrouchStepJump( void ) {
	if ( !( pm->ps->pm_flags & PMF_DUCKED ) || pml.groundPlane || 0.0f > pm->ps->velocity[2] ) {
		return qfalse;
	}
	return pm->settings->jumpVelocityTimeThreshold > (double)( pm->cmd.serverTime - pm->ps->jumpTime )
		? qfalse : qtrue;
}


/*
=============
PM_CheckJump

Jumps from the ground, or with PMF_DOUBLE_JUMP once more in the air
(retail qagamex86 0x1002e590).  Jumping ends a crouch slide.
=============
*/
static qboolean PM_CheckJump( qboolean inAir ) {
	int flags;

	if ( !PM_CanJump() ) {
		return qfalse;
	}
	flags = pm->ps->pm_flags;
	if ( inAir && ( flags & PMF_DOUBLE_JUMP ) ) {
		if ( flags & PMF_JUMP_HELD ) {
			pm->cmd.upmove = 0;
			return qfalse;
		}
		if ( pm->ps->doubleJumped ) {
			return qfalse;
		}
	}

	if ( flags & PMF_CROUCH_SLIDE ) {
		pm->ps->crouchSlideTime = 0;
	}
	PM_DoJump();
	if ( inAir && ( pm->ps->pm_flags & PMF_DOUBLE_JUMP ) ) {
		pm->ps->doubleJumped = 1;
	}
	return qtrue;
}

/*
===================
PM_CalcWish

The wish direction and speed of the swimming, flying and noclip moves
(retail qagamex86 0x1002e110): the view axes and the jump and crouch keys,
with the command scale applied to the speed only.  Returns qfalse with a
zero wish when there is no input.
===================
*/
static qboolean PM_CalcWish( vec3_t wishdir, float *wishspeed ) {
	const float scale = PM_CmdScale( &pm->cmd );
	const float fmove = pm->cmd.forwardmove;
	const float smove = pm->cmd.rightmove;
	int i;

	if ( scale == 0.0f ) {
		VectorClear( wishdir );
		*wishspeed = 0;
		return qfalse;
	}
	for ( i = 0; i < 3; i++ ) {
		wishdir[i] = (float)( (double)pml.forward[i] * fmove + (double)pml.right[i] * smove );
	}
	wishdir[2] = (float)( (double)pm->cmd.upmove + wishdir[2] );
	*wishspeed = PM_VectorNormalize( wishdir ) * scale;
	return qtrue;
}


/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	int		i;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	PM_VectorNormalize (flatforward);

	for ( i = 0; i < 3; i++ ) {
		spot[i] = (float)( (double)flatforward[i] * 30.0 + pm->ps->origin[i] );
	}
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	// there must be nothing at all above the ledge
	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( cont ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] = (float)( pm->ps->velocity[2] - (double)pm->ps->gravity * pml.frametime );
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	float	wishspeed;
	vec3_t	wishdir;
	double	limit;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}

	PM_Friction ();

	//
	// user intentions
	//
	if ( !PM_CalcWish( wishdir, &wishspeed ) ) {
		VectorSet( wishdir, 0, 0, -60 );	// sink towards bottom
		wishspeed = PM_VectorNormalize( wishdir );
	}

	limit = (double)pm->ps->speed
		* ( pm->waterlevel == 1 ? pm->settings->waterWadeScale : pm->settings->waterSwimScale );
	if ( wishspeed > limit ) {
		wishspeed = (float)limit;
	}

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && 0.0 > PM_Dot3( pm->ps->velocity, pml.groundTrace.plane.normal ) ) {
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove( qfalse );
}

#ifdef MISSIONPACK
/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear(pm->ps->velocity);
}
#endif

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	float	wishspeed;
	vec3_t	wishdir;

	// normal slowdown
	PM_Friction ();

	PM_CalcWish( wishdir, &wishspeed );
	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_UsingFlight

The flight holdable thrusts while the use button is held and fuel remains.
===================
*/
static qboolean PM_UsingFlight( void ) {
	const playerState_t *ps = pm->ps;

	return bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLIGHT
		&& ( ps->pm_flags & PMF_USE_ITEM_HELD ) && ps->pm_type != PM_DEAD
		&& ( !ps->stats[STAT_PLAYER_ITEM_RECHARGE] || ps->stats[STAT_PLAYER_ITEM_TIME] )
		? qtrue : qfalse;
}


/*
===================
PM_FlightThrust

Quake Live's flight is a holdable (retail qagamex86 0x1002fa80): while in
use it pushes the player up by STAT_PLAYER_ITEM_THRUST per second, burns
fuel (32001 and more is unlimited) and flags PW_FLIGHT for the cgame.  A
non-recharging flight item is used up when its fuel runs out.
===================
*/
static void PM_FlightThrust( void ) {
	playerState_t *ps = pm->ps;

	if ( !PM_UsingFlight() ) {
		ps->powerups[PW_FLIGHT] = 0;
		return;
	}
	ps->powerups[PW_FLIGHT] = INT_MAX;
	ps->velocity[2] = (float)( (int)( (double)ps->stats[STAT_PLAYER_ITEM_THRUST] * pml.frametime )
		+ (double)ps->velocity[2] );
	if ( ps->stats[STAT_PLAYER_ITEM_TIME] <= 32000 ) {
		ps->stats[STAT_PLAYER_ITEM_TIME] -= pml.msec;
		if ( ps->stats[STAT_PLAYER_ITEM_TIME] <= 0 ) {
			ps->stats[STAT_PLAYER_ITEM_TIME] = 0;
			if ( !ps->stats[STAT_PLAYER_ITEM_RECHARGE] ) {
				ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
		}
	}
	if ( ps->pm_flags & PMF_DUCKED ) {
		PM_ForceLegsAnim( LEGS_IDLECR );
	} else if ( ( ps->legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_JUMP ) {
		PM_ForceLegsAnim( LEGS_JUMP );
	}
}


/*
===================
PM_AirControl

PQL air control (retail qagamex86 0x1002fba0): moving straight forward or
back turns the horizontal velocity toward the wish direction, more strongly
the closer the two already are, without changing its speed.
===================
*/
static void PM_AirControl( const vec3_t wishdir, float wishspeed ) {
	float	*vel = pm->ps->velocity;
	float	zspeed, speed, dot, k;

	if ( ( pm->ps->movementDir != 0 && pm->ps->movementDir != 4 ) || wishspeed == 0.0f ) {
		return;
	}

	zspeed = vel[2];
	vel[2] = 0;
	speed = PM_VectorNormalize( vel );

	dot = (float)PM_Dot3( vel, wishdir );
	k = (float)( (double)pml.profile.airControl * dot * dot * pml.frametime * 32.0 );

	if ( dot > 0 ) {
		vel[0] = (float)( (double)wishdir[0] * k + (double)vel[0] * speed );
		vel[1] = (float)( (double)vel[1] * speed + (double)wishdir[1] * k );
		PM_VectorNormalize( vel );
	}

	vel[0] *= speed;
	vel[1] *= speed;
	vel[2] = zspeed;
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed, controlSpeed;
	float		scale, accel;

	// in the air without crouching, the crouch slide recharges five times as fast as it runs
	if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->cmd.upmove >= 0 ) {
		pm->ps->crouchSlideTime += pml.msec * 5;
		if ( pm->ps->crouchSlideTime > pm->settings->crouchSlideTime ) {
			pm->ps->crouchSlideTime = pm->settings->crouchSlideTime;
		}
	}

	PM_Friction();

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	PM_VectorNormalize( pml.forward );
	PM_VectorNormalize( pml.right );

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = (float)( (double)pml.forward[i] * fmove + (double)pml.right[i] * smove );
	}
	wishvel[2] = 0;

	scale = PM_CmdScale( &pm->cmd );
	VectorCopy( wishvel, wishdir );
	wishspeed = PM_VectorNormalize( wishdir );
	wishspeed *= scale;
	controlSpeed = wishspeed;

	// not on ground, so little effect on velocity; PQL brakes harder and
	// gives pure strafes their own wish speed and acceleration
	accel = pml.profile.airAccel;
	if ( pm->ps->pm_flags & PMF_AIR_CONTROL ) {
		if ( 0.0 > PM_Dot3( pm->ps->velocity, wishdir ) ) {
			accel = pml.profile.airStopAccel;
		}
		if ( pm->ps->movementDir == 2 || pm->ps->movementDir == 6 ) {
			if ( wishspeed > pml.profile.strafeWishSpeed ) {
				wishspeed = pml.profile.strafeWishSpeed;
			}
			accel = pml.profile.strafeAccel;
		}
	}
	PM_Accelerate( wishdir, wishspeed, accel );
	if ( pm->ps->pm_flags & PMF_AIR_CONTROL ) {
		PM_AirControl( wishdir, controlSpeed );
	}

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove( qtrue );

	// crouching in the air shows crouched legs
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		PM_ForceLegsAnim( LEGS_IDLECR );
	} else if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLECR ) {
		PM_ForceLegsAnim( LEGS_IDLE );
	}

	PM_FlightThrust();

	if ( pm->ps->pm_flags & PMF_DOUBLE_JUMP ) {
		PM_CheckJump( qtrue );
	}
}

/*
===================
PM_GrappleMove

The hook pulls at pmove_velocity_gh, slowing down within a tenth of that
distance of the hook point (retail qagamex86 0x1002ff20).
===================
*/
static void PM_GrappleMove( void ) {
	const float pull = pm->settings->velocityGH;
	vec3_t vel, v;
	float vlen;
	int i;

	if ( pull == 0.0f ) {
		return;
	}

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = (float)sqrt( PM_Dot3( vel, vel ) );
	PM_VectorNormalize( vel );

	VectorScale( vel, pull, vel );
	if ( !( pull * 0.1 < vlen ) ) {
		const double scale = vlen * 10.0 / pull;

		for ( i = 0; i < 3; i++ ) {
			vel[i] = (float)( vel[i] * scale );
		}
	}

	if ( pm->grappleSlow ) {
		VectorScale( vel, 0.25f, vel );
	} else {
		pml.groundPlane = qfalse;
	}
	if ( pm->waterlevel > 1 ) {
		VectorScale( vel, 0.75f, vel );
	}

	VectorCopy(vel, pm->ps->velocity);
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	float		accelerate;
	int			flags;
	qboolean	slick;

	accelerate = pml.profile.walkAccel;

	if ( pm->waterlevel > 2 && PM_Dot3( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}

	if ( PM_UsingFlight() ) {
		PM_StepSlideMove( qfalse );
		PM_FlightThrust();
		return;
	}

	if ( PM_CheckJump( qfalse ) ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction();

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	PM_VectorNormalize( pml.forward );
	PM_VectorNormalize( pml.right );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = (float)( (double)pml.forward[i] * fmove + (double)pml.right[i] * smove );
	}
	// when going up or down slopes the wish velocity should Not be zero

	scale = PM_CmdScale( &pm->cmd );
	VectorCopy( wishvel, wishdir );
	wishspeed = PM_VectorNormalize( wishdir );
	wishspeed *= scale;

	// clamp the speed lower if ducking; a crouch slide keeps more of it
	flags = pm->ps->pm_flags;
	if ( flags & PMF_DUCKED ) {
		const double limit = (double)pm->ps->speed
			* ( ( flags & PMF_CROUCH_SLIDE ) ? PM_CROUCH_SLIDE_SCALE : (double)pm_duckScale );

		if ( wishspeed > limit ) {
			wishspeed = (float)limit;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		const float depth = (float)( pm->waterlevel / 3.0 );
		const float swimScale = pm->waterlevel == 1
			? pm->settings->waterWadeScale : pm->settings->waterSwimScale;
		const float waterScale = (float)( 1.0 - ( 1.0 - swimScale ) * depth );
		const double limit = (double)pm->ps->speed * waterScale;

		if ( wishspeed > limit ) {
			wishspeed = (float)limit;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	slick = ( pml.groundTrace.surfaceFlags & SURF_SLICK ) ? qtrue : qfalse;
	if ( slick || ( flags & PMF_TIME_KNOCKBACK ) ) {
		accelerate = pm_airaccelerate;
	}
	if ( ( flags & PMF_DUCKED ) && slick ) {
		accelerate = PM_SLICK_CROUCH_ACCEL;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	if ( slick || ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
		pm->ps->velocity[2] = (float)( pm->ps->velocity[2] - (double)pm->ps->gravity * pml.frametime );
	}

	// slide along the ground plane; Quake Live does not restore the speed
	// lost to the slope
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
		pm->ps->velocity, OVERCLIP );

	// don't do anything if standing still
	if ( !pm->ps->velocity[0] && !pm->ps->velocity[1] ) {
		return;
	}

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_LadderMove

Climbing a SURF_LADDER wall (retail qagamex86 0x10030120): the view
direction and the jump and crouch keys move the player at up to 0.66 of the
run speed, without gravity.  Crouching on a ladder stands the player up.
===================
*/
static void PM_LadderMove( void ) {
	vec3_t	wishvel, wishdir;
	float	scale, wishspeed, fmove, smove, upmove, length;
	int		i;

	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );
	if ( scale != 0.0f ) {
		fmove = pm->cmd.forwardmove;
		smove = pm->cmd.rightmove;
		upmove = pm->cmd.upmove;
		for ( i = 0; i < 3; i++ ) {
			wishvel[i] = (float)( (double)pml.forward[i] * scale * fmove
				+ (double)pml.right[i] * scale * smove );
		}
		wishvel[2] = (float)( (double)scale * upmove + wishvel[2] );
	} else {
		VectorClear( wishvel );
	}

	if ( pm->cmd.upmove > 0 ) {
		wishvel[2] = (float)( PM_LADDER_SCALE * pm->ps->speed );
	} else if ( pm->cmd.upmove < 0 ) {
		pm->ps->pm_flags &= ~PMF_DUCKED;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
		wishvel[2] = (float)( PM_LADDER_SCALE * pm->ps->speed * -1.0 );
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = PM_VectorNormalize( wishdir );
	if ( wishspeed > (double)pm->ps->speed * PM_LADDER_SCALE ) {
		wishspeed = (float)( (double)pm->ps->speed * PM_LADDER_SCALE );
	}

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// keep the speed when pushed into the ground at the foot of the ladder
	if ( pml.groundPlane && 0.0 > PM_Dot3( pm->ps->velocity, pml.groundTrace.plane.normal ) ) {
		length = (float)sqrt( PM_Dot3( pm->ps->velocity, pm->ps->velocity ) );
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
		PM_VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, length, pm->ps->velocity );
	}

	PM_SlideMove( qfalse );
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = (float)sqrt( PM_Dot3( pm->ps->velocity, pm->ps->velocity ) );
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		PM_VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, control, newspeed;
	int			i;
	vec3_t		wishdir;
	float		wishspeed;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = (float)sqrt( PM_Dot3( pm->ps->velocity, pm->ps->velocity ) );
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		// Quake Live uses a noclip friction of 10
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop = (float)( (double)pml.frametime * PM_NOCLIP_FRICTION * control );

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	PM_CalcWish( wishdir, &wishspeed );
	PM_Accelerate( wishdir, wishspeed, pml.profile.walkAccel );

	// move
	for ( i = 0; i < 3; i++ ) {
		pm->ps->origin[i] = (float)( (double)pm->ps->velocity[i] * pml.frametime + pm->ps->origin[i] );
	}
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	const int surfaceFlags = pml.groundTrace.surfaceFlags;

	if ( pm->noFootsteps || ( surfaceFlags & SURF_NOSTEPS ) ) {
		return 0;
	}
	if ( surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	if ( surfaceFlags & SURF_SNOWSTEPS ) {
		return EV_FOOTSTEP_SNOW;
	}
	if ( surfaceFlags & SURF_WOODSTEPS ) {
		return EV_FOOTSTEP_WOOD;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events (retail qagamex86
0x100309f0).  Quake Live does not double the fall for a ducked player, plays
nothing for a gibbed one or a scout's soft landing, and a landing restores
the double jump.
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, c, den, negVel;
	int			health;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	acc = (float)-pm->ps->gravity;
	a = acc * 0.5f;
	vel = pml.previous_velocity[2];
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	c = -dist;

	den = (float)( (double)vel * vel - (double)c * ( (double)a * 4.0 ) );
	if ( den < 0 ) {
		return;
	}
	negVel = -vel;
	t = (float)( ( negVel - sqrt( den ) ) / ( (double)a + a ) );

	delta = (float)( (double)t * acc + vel );
	delta = (float)( (double)delta * delta * 0.0001 );

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25f;
	} else if ( pm->waterlevel == 1 ) {
		delta *= 0.5f;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	health = pm->ps->stats[STAT_HEALTH];
	if ( !( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) && health > GIB_HEALTH ) {
		if ( delta > 62 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 42 ) {
			// this is a pain grunt, so don't play it if dead
			if ( health > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else if ( pm->ps->stats[STAT_PERSISTANT_POWERUP] != PP_SCOUT ) {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;

	if ( pm->ps->pm_flags & PMF_DOUBLE_JUMP ) {
		pm->ps->doubleJumped = 0;
	}
}

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && PM_Dot3( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}
		
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving

Quake Live samples at the feet, the origin and the eyes (retail qagamex86
0x10031140) and keeps the feet contents as the water type.
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;

	pm->waterlevel = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = (float)( pm->ps->origin[2] - 24.0 + 1.0 );
	cont = pm->pointcontents( point, pm->ps->clientNum );
	pm->watertype = cont;

	if ( cont & MASK_WATER ) {
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2];
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = (float)pm->ps->viewheight + pm->ps->origin[2];
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		if ( pm->ps->pm_flags & PMF_INVULEXPAND ) {
			// invulnerability sphere has a 42 units radius
			VectorSet( pm->mins, -42, -42, -42 );
			VectorSet( pm->maxs, 42, 42, 42 );
		}
		else {
			VectorSet( pm->mins, -15, -15, MINS_Z );
			VectorSet( pm->maxs, 15, 15, 16 );
		}
		pm->ps->pm_flags |= PMF_DUCKED;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
		if ( !pm->ps->crouchTime ) {
			pm->ps->crouchTime = pm->cmd.serverTime;
		}
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		// retail raises the speed of a crouching player whose slide has run out to 400
		if ( !pm->ps->crouchSlideTime && ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && 400.0 > pm->ps->speed ) {
			pm->ps->speed = 400;
		}
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
		// standing up on the ground ends a crouch slide
		if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->ps->crouchSlideTime && pml.groundPlane ) {
			pm->ps->crouchSlideTime = 0;
		}
	}
}



//===================================================================


/*
===============
PM_Footsteps

Follows retail qagamex86 0x100314d0: a crouch slide counts as moving, a
running player slower than 32 units per second on both axes walks, and
scouts make no footstep sounds.
===============
*/
static void PM_Footsteps( void ) {
	playerState_t	*ps = pm->ps;
	float		bobmove;
	int			old;
	qboolean	footstep;
	const qboolean backwards = ( ps->pm_flags & PMF_BACKWARDS_RUN ) ? qtrue : qfalse;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = (float)sqrt( (double)ps->velocity[0] * ps->velocity[0]
		+ (double)ps->velocity[1] * ps->velocity[1] );

	if ( ps->groundEntityNum == ENTITYNUM_NONE ) {

		if ( ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove
		&& !( ( ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->cmd.upmove < 0 && ps->crouchSlideTime > 0 ) ) {
		if ( pm->xyspeed < 5.0f ) {
			ps->bobCycle = 0;	// start at beginning of cycle again
			if ( ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}

	footstep = ps->stats[STAT_PERSISTANT_POWERUP] != PP_SCOUT ? qtrue : qfalse;

	if ( ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5f;	// ducked characters bob much faster
		PM_ContinueLegsAnim( backwards ? LEGS_BACKCR : LEGS_WALKCR );
		// ducked characters never play footsteps
		footstep = qfalse;
	} else if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
		if ( ps->velocity[0] >= 32.0f || ps->velocity[0] <= -32.0f
			|| ps->velocity[1] >= 32.0f || ps->velocity[1] <= -32.0f ) {
			bobmove = 0.4f;	// faster speeds bob faster
			PM_ContinueLegsAnim( backwards ? LEGS_BACK : LEGS_RUN );
		} else {
			bobmove = 0.35f;	// too slow to run
			PM_ContinueLegsAnim( backwards ? LEGS_BACKWALK : LEGS_WALK );
			footstep = qfalse;
		}
	} else {
		bobmove = 0.3f;	// walking bobs slow
		PM_ContinueLegsAnim( backwards ? LEGS_BACKWALK : LEGS_WALK );
		footstep = qfalse;
	}

	// check for footstep / splash sounds
	old = ps->bobCycle;
	ps->bobCycle = (int)( (double)pml.msec * bobmove + old ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange

Weapon WP_NUM_WEAPONS (15) is raised at once without a drop (retail
qagamex86 0x10031880).
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon > WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	if ( weapon == WP_NUM_WEAPONS ) {
		PM_StartTorsoAnim( TORSO_STAND2 );
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->weapon = WP_NUM_WEAPONS;
		return;
	}
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += pm->settings->weaponDropTime;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_MAX_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += pm->settings->weaponRaiseTime;
	pm->ps->stats[STAT_CHAINGUN_SPINUP] = 0;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
===============
PM_TorsoAnimation

===============
*/
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate == WEAPON_READY ) {
		// retail also stands weapon 15 like the gauntlet
		if ( pm->ps->weapon == WP_GAUNTLET || pm->ps->weapon == WP_NUM_WEAPONS ) {
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		return;
	}
}


/*
===============
PM_ChaingunSpin

The chaingun spins up while firing and down while idle, within 0..1000.
===============
*/
static void PM_ChaingunSpin( int change ) {
	float spin = (float)( pm->ps->stats[STAT_CHAINGUN_SPINUP] + change );

	if ( !( spin >= 0.0f ) ) {
		spin = 0.0f;
	} else if ( spin > 1000.0f ) {
		spin = 1000.0f;
	}
	pm->ps->stats[STAT_CHAINGUN_SPINUP] = (int)spin;
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter.  Follows retail
qagamex86 0x10031990: reload times come from the weapon_reload_* settings,
the chaingun fires at half rate until spun up, haste and the scout speed
firing up, PMF_ATTACK_LOCKOUT holds fire and PMF_WEAPON_RESET cancels it.
==============
*/
static void PM_Weapon( void ) {
	playerState_t	*ps = pm->ps;
	const int		flags = ps->pm_flags;
	int		addTime, tag;

	// don't allow attack until all buttons are up
	if ( flags & PMF_RESPAWNED ) {
		return;
	}

	// check for dead player
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		ps->weapon = WP_NONE;
		return;
	}

	// set the firing flag for continuous beam weapons
	if ( ( pm->cmd.buttons & BUTTON_ATTACK ) && !( flags & PMF_ATTACK_LOCKOUT )
		&& ( ps->ammo[ps->weapon] || ps->weapon == WP_GRAPPLING_HOOK ) ) {
		ps->eFlags |= EF_FIRING;
	}

	// check for item using; flight is used by holding the button (PM_FlightThrust)
	if ( !( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) ) {
		ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	} else if ( !( ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
		tag = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
		if ( tag != HI_FLIGHT ) {
			ps->pm_flags |= PMF_USE_ITEM_HELD;
			// don't use medkit if at max health
			if ( tag != HI_MEDKIT || ps->stats[STAT_HEALTH] < ps->stats[STAT_MAX_HEALTH] + 25 ) {
				PM_AddEvent( EV_USE_ITEM0 + tag );
				ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	}

	// make weapon function
	if ( ps->weaponTime > 0 ) {
		ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( ps->weaponTime <= 0 || ps->weaponstate != WEAPON_FIRING ) {
		if ( ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( ps->weaponstate == WEAPON_RAISING ) {
		ps->weaponstate = WEAPON_READY;
		if ( ps->weapon == WP_GAUNTLET || ps->weapon == WP_NUM_WEAPONS ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
		return;
	}

	if ( ps->weapon == WP_NUM_WEAPONS ) {
		return;
	}

	// check for fire
	if ( !( pm->cmd.buttons & BUTTON_ATTACK ) || ( ps->pm_flags & PMF_ATTACK_LOCKOUT ) ) {
		ps->weaponTime = 0;
		ps->weaponstate = WEAPON_READY;
		PM_ChaingunSpin( -pml.msec );
		return;
	}

	if ( ps->pm_flags & PMF_WEAPON_RESET ) {
		ps->weaponTime = 0;
		ps->weaponstate = WEAPON_READY;
		ps->eFlags &= ~EF_FIRING;
		ps->pm_flags &= ~PMF_WEAPON_RESET;
		return;
	}

	// start the animation even if out of ammo
	if ( ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
			ps->weaponTime = 0;
			ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}

	ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( ! ps->ammo[ ps->weapon ] ) {
		PM_AddEvent( EV_NOAMMO );
		ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if ( ps->ammo[ ps->weapon ] != -1 ) {
		ps->ammo[ ps->weapon ]--;
	}

	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );

	addTime = pm->settings->weaponReload[ps->weapon];
	if ( ps->weapon == WP_CHAINGUN ) {
		if ( ps->stats[STAT_CHAINGUN_SPINUP] < 1000 ) {
			addTime *= 2;
		}
		PM_ChaingunSpin( addTime );
	}

	if ( ps->powerups[PW_HASTE] ) {
		ps->weaponTime += (int)( addTime * ( 1.0 / 1.3 ) );
	} else if ( ps->stats[STAT_PERSISTANT_POWERUP] == PP_SCOUT ) {
		ps->weaponTime += (int)( addTime / 1.25 );
	} else {
		ps->weaponTime += addTime;
	}
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
#ifdef MISSIONPACK
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
#endif
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}

	// a crouch slide runs out while on the ground
	if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pml.groundPlane && pm->ps->crouchSlideTime ) {
		if ( pml.msec >= pm->ps->crouchSlideTime ) {
			pm->ps->crouchSlideTime = 0;
		} else {
			pm->ps->crouchSlideTime -= pml.msec;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_TUTORIAL ) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->pm_type != PM_FREEZE && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}


/*
================
PM_SetMovementProfile

Quake Live chooses the movement profile every frame (retail qagamex86
0x1002d9f0): PQL when the player has PMF_AIR_CONTROL, VQL otherwise.  Both
override the pmove_* cvars of the same names.
================
*/
static void PM_SetMovementProfile( pmoveProfile_t *profile ) {
	if ( pm->ps->pm_flags & PMF_AIR_CONTROL ) {
		profile->airControl = 150.0f;
		profile->airAccel = 1.1f;
		profile->airStopAccel = 2.5f;
		profile->circleStrafeFriction = 5.5f;
		profile->strafeAccel = 70.0f;
		profile->walkAccel = 15.0f;
		profile->chainJump = 3;
		profile->strafeWishSpeed = 35.0f;
	} else {
		profile->airControl = 0.0f;
		profile->airAccel = 1.0f;
		profile->airStopAccel = 1.0f;
		profile->strafeAccel = 1.0f;
		profile->walkAccel = 10.0f;
		profile->chainJump = 1;
		profile->circleStrafeFriction = 6.0f;
		profile->strafeWishSpeed = 400.0f;
	}
}


/*
================
PM_CheckLadder

A SURF_LADDER wall one unit ahead puts the player on a ladder (retail
qagamex86 0x10030360).
================
*/
static void PM_CheckLadder( void ) {
	vec3_t		flatforward, spot;
	trace_t		trace;

	pml.ladder = qfalse;

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	PM_VectorNormalize( flatforward );
	VectorAdd( pm->ps->origin, flatforward, spot );

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, MASK_PLAYERSOLID );
	if ( trace.fraction < 1.0f && ( trace.surfaceFlags & SURF_LADDER ) ) {
		pml.ladder = qtrue;
	}
}


/*
================
PmoveSingle

Follows retail qagamex86 0x10031FA0.
================
*/
void PmoveSingle (pmove_t *pmove) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 && !pm->ps->powerups[PW_FROZEN] ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}
	if ( pm->settings->noPlayerClip == 1 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// pmove_noPlayerClip: players pass through each other
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// PM_Weapon sets the firing flag again
	pm->ps->eFlags &= ~EF_FIRING;

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 &&
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));
	PM_SetMovementProfile( &pml.profile );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	if ( !( pm->ps->pm_flags & PMF_NO_MOVE ) ) {
		// save old org in case we get stuck
		VectorCopy (pm->ps->origin, pml.previous_origin);

		// save old velocity for crashlanding
		VectorCopy (pm->ps->velocity, pml.previous_velocity);
	}

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	// mirror the command for retail follow/demo HUD widgets and prediction
	if ( pm->cmd.weaponPrimary >= 1 && pm->cmd.weaponPrimary < WP_NUM_WEAPONS ) {
		pm->ps->weaponPrimary = pm->cmd.weaponPrimary;
	}
	if ( pm->cmd.fov >= 10 && pm->cmd.fov <= 130 ) {
		pm->ps->fov = pm->cmd.fov;
	}
	pm->ps->forwardmove = pm->cmd.forwardmove;
	pm->ps->rightmove = pm->cmd.rightmove;
	pm->ps->upmove = pm->cmd.upmove;

	PM_AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	// frozen players still run the move, with their input cleared above
	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_TUTORIAL ) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();
	PM_CheckLadder();

	// Quake Live's flight is a holdable; PM_FlightThrust raises this again
	pm->ps->powerups[PW_FLIGHT] = 0;

	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		PM_InvulnerabilityMove();
	} else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
	} else if ( pml.ladder ) {
		PM_LadderMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	// holdable fuel recharges while the item is not in use
	if ( pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] && !( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
		int fuel = (int)( (double)pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] * pml.frametime )
			+ pm->ps->stats[STAT_PLAYER_ITEM_TIME];

		if ( fuel > pm->ps->stats[STAT_PLAYER_ITEM_TIME_MAX] ) {
			fuel = pm->ps->stats[STAT_PLAYER_ITEM_TIME_MAX];
		}
		pm->ps->stats[STAT_PLAYER_ITEM_TIME] = fuel;
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// Quake Live does not snap the velocity: retail PmoveSingle ends after
	// the water events, and the retail cgame predicts with the unsnapped value.
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	static pmoveSettings_t	defaultSettings;
	static qboolean			defaultSettingsReady;
	int			finalTime;

	if ( !pmove->settings ) {
		if ( !defaultSettingsReady ) {
			BG_DefaultPmoveSettings( &defaultSettings );
			defaultSettingsReady = qtrue;
		}
		pmove->settings = &defaultSettings;
	}

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior; Quake Live has no fixed-msec mode
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;
		if ( msec > 66 ) {
			msec = 66;
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}
}
