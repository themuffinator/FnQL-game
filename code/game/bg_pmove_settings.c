/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
//
// bg_pmove_settings.c -- Quake Live movement settings.
//
// One table maps each pmove_* setting to its field and its retail default
// string (the retail qagame cvar defaults, docs/contract/retail_tables.json).
// The defaults and the infostring parser share it, and parsing a value goes
// through atof/atoi exactly as the engine does when it fills a vmCvar, so a
// server reading its cvars and a client reading CS_PMOVE_SETTINGS end up with
// identical values.
//

#include "q_shared.h"
#include "bg_public.h"

typedef enum {
	PMS_FLOAT,
	PMS_FLOAT_MIN,		// a float the retail filler raises to at least PMS_MINIMUM
	PMS_INT,
	PMS_BOOL			// stored as 0 or 1, as the retail filler stores ( value != 0 )
} pmoveSettingType_t;

#define PMS_MINIMUM		0.001f

typedef struct {
	const char			*key;		// cvar and infostring key
	size_t				offset;
	pmoveSettingType_t	type;
	const char			*defaultValue;
} pmoveSettingField_t;

#define PMS( name, field, type, def )	{ name, offsetof( pmoveSettings_t, field ), type, def }

static const pmoveSettingField_t pmoveSettingFields[] = {
	PMS( "pmove_AirAccel", airAccel, PMS_FLOAT, "1.0f" ),
	PMS( "pmove_AirStepFriction", airStepFriction, PMS_FLOAT, "0.03f" ),
	PMS( "pmove_AirSteps", airSteps, PMS_BOOL, "1" ),
	PMS( "pmove_AirStopAccel", airStopAccel, PMS_FLOAT, "1.0f" ),
	PMS( "pmove_AutoHop", autoHop, PMS_BOOL, "1" ),
	PMS( "pmove_BunnyHop", bunnyHop, PMS_BOOL, "1" ),
	PMS( "pmove_ChainJump", chainJump, PMS_INT, "1" ),
	PMS( "pmove_ChainJumpVelocity", chainJumpVelocity, PMS_FLOAT, "110.0f" ),
	PMS( "pmove_CircleStrafeFriction", circleStrafeFriction, PMS_FLOAT, "6.0f" ),
	PMS( "pmove_CrouchSlideFriction", crouchSlideFriction, PMS_FLOAT, "0.5" ),
	PMS( "pmove_CrouchSlideTime", crouchSlideTime, PMS_INT, "2000" ),
	PMS( "pmove_CrouchStepJump", crouchStepJump, PMS_BOOL, "1" ),
	PMS( "pmove_JumpTimeDeltaMin", jumpTimeDeltaMin, PMS_FLOAT, "100.0f" ),
	PMS( "pmove_JumpVelocity", jumpVelocity, PMS_FLOAT, "275.0f" ),
	PMS( "pmove_JumpVelocityMax", jumpVelocityMax, PMS_FLOAT, "700.0f" ),
	PMS( "pmove_JumpVelocityScaleAdd", jumpVelocityScaleAdd, PMS_FLOAT, "0.4f" ),
	PMS( "pmove_JumpVelocityTimeThreshold", jumpVelocityTimeThreshold, PMS_FLOAT_MIN, "500.0f" ),
	PMS( "pmove_JumpVelocityTimeThresholdOffset", jumpVelocityTimeThresholdOffset, PMS_FLOAT_MIN, "0.6f" ),
	PMS( "pmove_noPlayerClip", noPlayerClip, PMS_BOOL, "0" ),
	PMS( "pmove_RampJump", rampJump, PMS_BOOL, "0" ),
	PMS( "pmove_RampJumpScale", rampJumpScale, PMS_FLOAT, "1.0f" ),
	PMS( "pmove_StepHeight", stepHeight, PMS_FLOAT, "22.0f" ),
	PMS( "pmove_StepJump", stepJump, PMS_BOOL, "1" ),
	PMS( "pmove_StepJumpVelocity", stepJumpVelocity, PMS_FLOAT, "48.0f" ),
	PMS( "pmove_StrafeAccel", strafeAccel, PMS_FLOAT, "1.0f" ),
	PMS( "pmove_velocity_gh", velocityGH, PMS_FLOAT_MIN, "800" ),
	PMS( "pmove_WalkAccel", walkAccel, PMS_FLOAT, "10.0f" ),
	PMS( "pmove_WalkFriction", walkFriction, PMS_FLOAT, "6.0f" ),
	PMS( "pmove_WaterSwimScale", waterSwimScale, PMS_FLOAT, "0.6f" ),
	PMS( "pmove_WaterWadeScale", waterWadeScale, PMS_FLOAT, "0.8f" ),
	PMS( "pmove_WeaponDropTime", weaponDropTime, PMS_INT, "200" ),
	PMS( "pmove_WeaponRaiseTime", weaponRaiseTime, PMS_INT, "200" ),
	PMS( "pmove_WishSpeed", wishSpeed, PMS_FLOAT, "400.0f" ),
};

static void BG_SetPmoveSettingField( pmoveSettings_t *settings, const pmoveSettingField_t *field,
	const char *value ) {
	byte *base = (byte *)settings + field->offset;

	float f;

	switch ( field->type ) {
	case PMS_FLOAT:
		*(float *)base = (float)atof( value );
		break;
	case PMS_FLOAT_MIN:
		f = (float)atof( value );
		*(float *)base = f >= PMS_MINIMUM ? f : PMS_MINIMUM;
		break;
	case PMS_INT:
		*(int *)base = atoi( value );
		break;
	case PMS_BOOL:
		*(int *)base = atoi( value ) != 0;
		break;
	}
}

// weapon_reload_* (CS_WEAPON_RELOAD) in the order retail qagamex86 0x100549e0
// copies them into the per-weapon table PM_Weapon reads
typedef struct {
	const char	*key;
	weapon_t	weapon;
	const char	*defaultValue;
} weaponReloadField_t;

static const weaponReloadField_t weaponReloadFields[] = {
	{ "weapon_reload_gauntlet", WP_GAUNTLET, "400" },
	{ "weapon_reload_mg", WP_MACHINEGUN, "100" },
	{ "weapon_reload_sg", WP_SHOTGUN, "1000" },
	{ "weapon_reload_gl", WP_GRENADE_LAUNCHER, "800" },
	{ "weapon_reload_rl", WP_ROCKET_LAUNCHER, "800" },
	{ "weapon_reload_lg", WP_LIGHTNING, "50" },
	{ "weapon_reload_rg", WP_RAILGUN, "1500" },
	{ "weapon_reload_pg", WP_PLASMAGUN, "100" },
	{ "weapon_reload_bfg", WP_BFG, "300" },
	{ "weapon_reload_gh", WP_GRAPPLING_HOOK, "100" },
	{ "weapon_reload_ng", WP_NAILGUN, "1000" },
	{ "weapon_reload_prox", WP_PROX_LAUNCHER, "800" },
	{ "weapon_reload_cg", WP_CHAINGUN, "50" },
	{ "weapon_reload_hmg", WP_HEAVY_MACHINEGUN, "75" },
};

/*
================
BG_DefaultPmoveSettings
================
*/
void BG_DefaultPmoveSettings( pmoveSettings_t *settings ) {
	int i;

	for ( i = 0; i < ARRAY_LEN( pmoveSettingFields ); i++ ) {
		BG_SetPmoveSettingField( settings, &pmoveSettingFields[i], pmoveSettingFields[i].defaultValue );
	}
	memset( settings->weaponReload, 0, sizeof( settings->weaponReload ) );
	for ( i = 0; i < ARRAY_LEN( weaponReloadFields ); i++ ) {
		settings->weaponReload[weaponReloadFields[i].weapon] = atoi( weaponReloadFields[i].defaultValue );
	}
}

/*
================
BG_WeaponReloadFromInfo

Fills the reload times from an infostring keyed by cvar name
(CS_WEAPON_RELOAD on the client, or one built from the server's cvars).
Missing keys keep their current value.
================
*/
void BG_WeaponReloadFromInfo( const char *info, pmoveSettings_t *settings ) {
	const char *value;
	int i;

	if ( !info || !info[0] ) {
		return;
	}
	for ( i = 0; i < ARRAY_LEN( weaponReloadFields ); i++ ) {
		value = Info_ValueForKey( info, weaponReloadFields[i].key );
		if ( value[0] ) {
			settings->weaponReload[weaponReloadFields[i].weapon] = atoi( value );
		}
	}
}

/*
================
BG_PmoveSettingsFromInfo

Fills the settings from an infostring keyed by cvar name (CS_PMOVE_SETTINGS
on the client, or one built from the server's cvars).  Missing keys keep the
retail default.
================
*/
void BG_PmoveSettingsFromInfo( const char *info, pmoveSettings_t *settings ) {
	const char *value;
	int i;

	BG_DefaultPmoveSettings( settings );
	if ( !info || !info[0] ) {
		return;
	}
	for ( i = 0; i < ARRAY_LEN( pmoveSettingFields ); i++ ) {
		value = Info_ValueForKey( info, pmoveSettingFields[i].key );
		if ( value[0] ) {
			BG_SetPmoveSettingField( settings, &pmoveSettingFields[i], value );
		}
	}
}
