// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_slidemove.c -- part of bg_pmove functionality

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

Arithmetic follows the retail x87 evaluation; see "Retail arithmetic" in
bg_pmove.c.

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean	PM_SlideMove( qboolean gravity ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	float		*vel = pm->ps->velocity;

	numbumps = 4;

	VectorCopy (vel, primal_velocity);

	if ( gravity ) {
		VectorCopy( vel, endVelocity );
		endVelocity[2] = (float)( vel[2] - (double)pm->ps->gravity * pml.frametime );
		vel[2] = (float)( ( (double)vel[2] + endVelocity[2] ) * 0.5 );
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			// slide along the ground plane
			PM_ClipVelocity (vel, pml.groundTrace.plane.normal, vel, OVERCLIP );
		}
	} else {
		VectorClear( endVelocity );
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	PM_VectorNormalize2( vel, planes[numplanes] );
	numplanes++;

	for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		for ( i = 0; i < 3; i++ ) {
			end[i] = (float)( (double)vel[i] * time_left + pm->ps->origin[i] );
		}

		// see if we can make it there
		pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			vel[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy (trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1) {
			 break;		// moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left = (float)( time_left - (double)trace.fraction * time_left );

		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			VectorClear( vel );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0 ; i < numplanes ; i++ ) {
			if ( PM_Dot3( trace.plane.normal, planes[i] ) > 0.99 ) {
				VectorAdd( trace.plane.normal, vel, vel );
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = (float)PM_Dot3( vel, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity (vel, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( PM_Dot3( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( PM_Dot3( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				PM_CrossProduct (planes[i], planes[j], dir);
				PM_VectorNormalize( dir );
				d = (float)PM_Dot3( dir, vel );
				VectorScale( dir, d, clipVelocity );

				PM_CrossProduct (planes[i], planes[j], dir);
				PM_VectorNormalize( dir );
				d = (float)PM_Dot3( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( PM_Dot3( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( vel );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, vel );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity ) {
		VectorCopy( endVelocity, vel );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, vel );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_StepJump

pmove_StepJump: running up a step with jump held jumps off it, and with
pmove_CrouchStepJump a crouched player rising onto a step jumps again
(retail qagamex86 0x1002f455).  end is where the move was headed.
==================
*/
static void PM_StepJump( const vec3_t end ) {
	const pmoveSettings_t *s = pm->settings;
	vec3_t		probeStart, probeEnd, mins, maxs, below;
	trace_t		trace;

	if ( !PM_CanJump() && !( s->crouchStepJump && PM_CanCrouchStepJump() ) ) {
		return;
	}

	// there must be walkable ground within a step of where the move was headed
	VectorCopy( end, probeStart );
	probeStart[2] = s->stepHeight + end[2];
	VectorCopy( end, probeEnd );
	probeEnd[2] = end[2] - s->stepHeight;
	pm->trace( &trace, probeStart, pm->mins, pm->maxs, probeEnd, pm->ps->clientNum, pm->tracemask );
	if ( trace.allsolid || trace.startsolid || trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		return;
	}

	if ( PM_CanJump() ) {
		pml.stepJumping = qtrue;
		PM_DoJump();
		pml.stepJumping = qfalse;
		return;
	}

	if ( s->crouchStepJump && PM_CanCrouchStepJump() ) {
		// a slightly narrower box must find ground within 64 units below
		VectorSet( mins, pm->mins[0] + 1.0f, pm->mins[1] + 1.0f, pm->mins[2] );
		VectorSet( maxs, pm->maxs[0] - 1.0f, pm->maxs[1] - 1.0f, pm->maxs[2] );
		VectorCopy( pm->ps->origin, below );
		below[2] = (float)( pm->ps->origin[2] - 64.0 );
		pm->trace( &trace, pm->ps->origin, mins, maxs, below, pm->ps->clientNum, pm->tracemask );
		if ( trace.fraction != 1.0f ) {
			pml.noRampJump = qtrue;
			PM_DoJump();
			pml.noRampJump = qfalse;
		}
	}
}

/*
==================
PM_StepSlideMove

Follows retail qagamex86 0x1002efe0.  Quake Live has no step events: the
height of a step up is handed back in pmove_t.stepUp and stepTime for cgame
step smoothing.
==================
*/
void PM_StepSlideMove( qboolean gravity ) {
	const pmoveSettings_t *s = pm->settings;
	vec3_t		start_o, start_v;
	vec3_t		end, up, down;
	trace_t		trace;
	float		stepSize, delta, into;
	int			i;

	VectorCopy (pm->ps->origin, start_o);
	VectorCopy (pm->ps->velocity, start_v);

	if ( PM_SlideMove( gravity ) == 0 ) {
		return;		// we got exactly where we wanted to go first try
	}

	// where the move was headed
	for ( i = 0; i < 3; i++ ) {
		end[i] = (float)( (double)pml.frametime * start_v[i] + start_o[i] );
	}

	if ( !s->airSteps ) {
		// never step up when you still have up velocity, unless the move
		// was headed for ground within a step
		pm->trace( &trace, start_o, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask );
		VectorCopy( trace.endpos, end );
		VectorCopy( end, down );
		down[2] = end[2] - s->stepHeight;
		pm->trace( &trace, end, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );
		if ( start_v[2] > 0 && ( trace.fraction == 1.0f || trace.plane.normal[2] < MIN_WALK_NORMAL ) ) {
			return;
		}
	}

	VectorCopy (start_o, up);
	up[2] = s->stepHeight + start_o[2];

	// test the player position if they were a stepheight higher
	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if ( trace.allsolid ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy (trace.endpos, pm->ps->origin);
	VectorCopy (start_v, pm->ps->velocity);

	PM_SlideMove( gravity );

	// push down the final amount
	VectorCopy (pm->ps->origin, down);
	down[2] = pm->ps->origin[2] - stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if ( !trace.allsolid ) {
		VectorCopy (trace.endpos, pm->ps->origin);
	}
	// clip only a velocity running into the surface or along it
	into = (float)PM_Dot3( pm->ps->velocity, trace.plane.normal );
	if ( trace.fraction < 1.0f && ( 0.0f > into || 0.001f > (float)fabs( into ) ) ) {
		PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
	}

	// it was a step only if the direct path is blocked
	pm->trace( &trace, start_o, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
	if ( !( trace.fraction < 1.0f ) ) {
		return;
	}

	delta = pm->ps->origin[2] - start_o[2];
	if ( delta > 2 ) {
		pm->stepUp = delta;
		pm->stepTime = pm->cmd.serverTime;
	}

	// stepping up in the air (pmove_AirSteps) costs some speed
	if ( !pml.groundPlane && delta > 0 && start_v[2] > 0 ) {
		const float keep = 1.0f - s->airStepFriction;

		pm->ps->velocity[0] *= keep;
		pm->ps->velocity[1] *= keep;
	}

	if ( s->stepJump && pm->ps->pm_type == PM_NORMAL && delta > 0 && pm->waterlevel < 2 ) {
		PM_StepJump( end );
	}

	if ( pm->debugLevel ) {
		Com_Printf("%i:stepped %f\n", c_pmove, delta);
	}
}
