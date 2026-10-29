// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_local.h -- local definitions for the bg (both games) files

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes

#define	JUMP_VELOCITY	270

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001f

// Movement profile for one PmoveSingle.  Quake Live overwrites these
// settings every frame (retail qagamex86 0x1002d9f0): the VQL constants, or
// the PQL set when the player has PMF_AIR_CONTROL, so the matching pmove_*
// cvars do not affect movement.
typedef struct {
	float		airAccel;
	float		airStopAccel;
	float		strafeAccel;
	float		strafeWishSpeed;
	float		walkAccel;
	float		circleStrafeFriction;
	float		airControl;
	int			chainJump;
} pmoveProfile_t;

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct {
	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	qboolean	walking;
	qboolean	groundPlane;
	trace_t		groundTrace;

	qboolean	noRampJump;		// PM_DoJump: a crouch step jump, never a ramp jump
	qboolean	ladder;			// facing a SURF_LADDER wall
	qboolean	stepJumping;	// PM_DoJump: a step jump
	float		impactSpeed;

	vec3_t		previous_origin;
	vec3_t		previous_velocity;
	int			previous_waterlevel;

	pmoveProfile_t	profile;
} pml_t;

extern	pmove_t		*pm;
extern	pml_t		pml;

// movement parameters
extern	float	pm_stopspeed;
extern	float	pm_duckScale;
extern	float	pm_swimScale;
extern	float	pm_wadeScale;

extern	float	pm_accelerate;
extern	float	pm_airaccelerate;
extern	float	pm_wateraccelerate;
extern	float	pm_flyaccelerate;

extern	float	pm_friction;
extern	float	pm_waterfriction;
extern	float	pm_flightfriction;

extern	int		c_pmove;

void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out, float overbounce );
void PM_AddTouchEnt( int entityNum );
void PM_AddEvent( int newEvent );

qboolean	PM_CanJump( void );
qboolean	PM_CanCrouchStepJump( void );
void		PM_DoJump( void );

qboolean	PM_SlideMove( qboolean gravity );
void		PM_StepSlideMove( qboolean gravity );
