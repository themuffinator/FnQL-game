// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

// Quake Live item table.  Row order is part of the wire contract: an item's
// index is its entity modelindex and its EV_ITEM_PICKUP parm, so this table
// matches the retail bg_itemlist row for row (classname, pickup sound, model
// slots, icon, pickup name, quantity, type and tag).  It is generated from
// docs/contract/retail_tables.json and checked by scripts/check_wire_contract.py.
gitem_t	bg_itemlist[] = 
{
	{ NULL },	// leave index 0 alone

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_shard",
		"sound/misc/ar1_pkup.wav",
		{ "models/powerups/armor/shard.md3", "models/powerups/armor/shard_sphere.md3" },
/* icon */		"icons/iconr_shard",
/* pickup */	"Armor Shard",
		5,
		IT_ARMOR,
		ARMOR_TIER_SHARD,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_combat",
		"sound/misc/ar2_pkup.wav",
		{ "models/powerups/armor/armor_yel.md3" },
/* icon */		"icons/iconr_yellow",
/* pickup */	"Yellow Armor",
		50,
		IT_ARMOR,
		ARMOR_TIER_YELLOW,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_body",
		"sound/misc/ar2_pkup.wav",
		{ "models/powerups/armor/armor_red.md3" },
/* icon */		"icons/iconr_red",
/* pickup */	"Red Armor",
		100,
		IT_ARMOR,
		ARMOR_TIER_RED,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_jacket",
		"sound/misc/ar2_pkup.wav",
		{ "models/powerups/armor/armor_grn.md3" },
/* icon */		"icons/iconr_green",
/* pickup */	"Green Armor",
		25,
		IT_ARMOR,
		ARMOR_TIER_GREEN,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_small",
		"sound/items/s_health.wav",
		{ "models/powerups/health/small_cross.md3", "models/powerups/health/small_sphere.md3" },
/* icon */		"icons/iconh_green",
/* pickup */	"5 Health",
		5,
		IT_HEALTH,
		HEALTH_TIER_SMALL,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health",
		"sound/items/n_health.wav",
		{ "models/powerups/health/medium_cross.md3", "models/powerups/health/medium_sphere.md3" },
/* icon */		"icons/iconh_yellow",
/* pickup */	"25 Health",
		25,
		IT_HEALTH,
		HEALTH_TIER_MEDIUM,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_large",
		"sound/items/l_health.wav",
		{ "models/powerups/health/large_cross.md3", "models/powerups/health/large_sphere.md3" },
/* icon */		"icons/iconh_red",
/* pickup */	"50 Health",
		50,
		IT_HEALTH,
		HEALTH_TIER_LARGE,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_mega",
		"sound/items/m_health.wav",
		{ "models/powerups/health/mega_cross.md3", "models/powerups/health/mega_sphere.md3" },
/* icon */		"icons/iconh_mega",
/* pickup */	"Mega Health",
		100,
		IT_HEALTH,
		HEALTH_TIER_MEGA,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_gauntlet",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/gauntlet/gauntlet.md3" },
/* icon */		"icons/iconw_gauntlet",
/* pickup */	"Gauntlet",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shotgun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/shotgun/shotgun.md3" },
/* icon */		"icons/iconw_shotgun",
/* pickup */	"Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_machinegun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/machinegun/machinegun.md3" },
/* icon */		"icons/iconw_machinegun",
/* pickup */	"Machine Gun",
		100,
		IT_WEAPON,
		WP_MACHINEGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/grenadel/grenadel.md3" },
/* icon */		"icons/iconw_grenade",
/* pickup */	"Grenade Launcher",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/rocketl/rocketl.md3" },
/* icon */		"icons/iconw_rocket",
/* pickup */	"Rocket Launcher",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_lightning",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/lightning/lightning.md3" },
/* icon */		"icons/iconw_lightning",
/* pickup */	"Lightning Gun",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_railgun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/railgun/railgun.md3" },
/* icon */		"icons/iconw_railgun",
/* pickup */	"Railgun",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_plasmagun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/plasma/plasma.md3" },
/* icon */		"icons/iconw_plasma",
/* pickup */	"Plasma Gun",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_bfg",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/bfg/bfg.md3" },
/* icon */		"icons/iconw_bfg",
/* pickup */	"BFG10K",
		10,
		IT_WEAPON,
		WP_BFG,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.wav",
		{ "models/weapons2/grapple/grapple.md3" },
/* icon */		"icons/iconw_grapple",
/* pickup */	"Grappling Hook",
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_shells",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/shotgunam.md3" },
/* icon */		"icons/icona_shotgun",
/* pickup */	"Shells",
		5,
		IT_AMMO,
		WP_SHOTGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bullets",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/machinegunam.md3" },
/* icon */		"icons/icona_machinegun",
/* pickup */	"Bullets",
		50,
		IT_AMMO,
		WP_MACHINEGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/grenadeam.md3" },
/* icon */		"icons/icona_grenade",
/* pickup */	"Grenades",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_cells",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/plasmaam.md3" },
/* icon */		"icons/icona_plasma",
/* pickup */	"Cells",
		50,
		IT_AMMO,
		WP_PLASMAGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_lightning",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/lightningam.md3" },
/* icon */		"icons/icona_lightning",
/* pickup */	"Lightning",
		50,
		IT_AMMO,
		WP_LIGHTNING,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/rocketam.md3" },
/* icon */		"icons/icona_rocket",
/* pickup */	"Rockets",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_slugs",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/railgunam.md3" },
/* icon */		"icons/icona_railgun",
/* pickup */	"Slugs",
		5,
		IT_AMMO,
		WP_RAILGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bfg",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/bfgam.md3" },
/* icon */		"icons/icona_bfg",
/* pickup */	"BFG Ammo",
		5,
		IT_AMMO,
		WP_BFG,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_teleporter",
		"sound/items/holdable.wav",
		{ "models/powerups/holdable/teleporter.md3" },
/* icon */		"icons/teleporter",
/* pickup */	"Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_medkit",
		"sound/items/holdable.wav",
		{ "models/powerups/holdable/medkit.md3", "models/powerups/holdable/medkit_sphere.md3" },
/* icon */		"icons/medkit",
/* pickup */	"Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/use_medkit.wav"
	},

/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_quad",
		"sound/items/damage3.ogg",
		{ "models/powerups/instant/quad.md3", "models/powerups/instant/quad_ring.md3" },
/* icon */		"icons/quad",
/* pickup */	"Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_enviro",
		"sound/items/protect.wav",
		{ "models/powerups/instant/enviro.md3", "models/powerups/instant/enviro_ring.md3" },
/* icon */		"icons/envirosuit",
/* pickup */	"Battle Suit",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
	},

/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_haste",
		"sound/items/guard.wav",
		{ "models/powerups/instant/haste.md3", "models/powerups/instant/haste_ring.md3" },
/* icon */		"icons/haste",
/* pickup */	"Haste",
		30,
		IT_POWERUP,
		PW_HASTE,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_invis",
		"sound/items/holdable.wav",
		{ "models/powerups/instant/invis.md3", "models/powerups/instant/invis_ring.md3" },
/* icon */		"icons/invis",
/* pickup */	"Invisibility",
		30,
		IT_POWERUP,
		PW_INVIS,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_regen",
		"sound/items/holdable.wav",
		{ "models/powerups/instant/regen.md3", "models/powerups/instant/regen_ring.md3" },
/* icon */		"icons/regen",
/* pickup */	"Regeneration",
		30,
		IT_POWERUP,
		PW_REGEN,
/* timer */	1,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/regen.wav"
	},

/*QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_flight",
		"sound/items/flight.wav",
		{ "models/powerups/instant/flight.md3", "models/powerups/instant/flight_ring.md3" },
/* icon */		"icons/flight",
/* pickup */	"Flight",
		60,
		IT_HOLDABLE,
		HI_FLIGHT,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/flight.wav"
	},

/*QUAKED team_CTF_redflag (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"team_CTF_redflag",
		NULL,
		{ "models/flags/r_flag.md3", NULL, NULL, NULL, "models/flag3/r_flag3.md3" },
/* icon */		"gfx/2d/flag_status/red_flag_at_base",
/* pickup */	"Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* timer */	0,
/* retail */	{ 64, 2048 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_blueflag (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"team_CTF_blueflag",
		NULL,
		{ "models/flags/b_flag.md3", NULL, NULL, NULL, "models/flag3/b_flag3.md3" },
/* icon */		"gfx/2d/flag_status/blue_flag_at_base",
/* pickup */	"Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* timer */	0,
/* retail */	{ 64, 2048 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_kamikaze (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_kamikaze",
		"sound/items/holdable.wav",
		{ "models/powerups/kamikazi.md3" },
/* icon */		"icons/kamikaze",
/* pickup */	"Kamikaze",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/items/kamikazerespawn.wav"
	},

/*QUAKED holdable_portal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_portal",
		"sound/items/holdable.wav",
		{ "models/powerups/holdable/porter.md3" },
/* icon */		"icons/portal",
/* pickup */	"Portal",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_invulnerability",
		"sound/items/holdable.wav",
		{ "models/powerups/holdable/invulnerability.md3" },
/* icon */		"icons/invulnerability",
/* pickup */	"Invulnerability",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_nails (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_nails",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/nailgunam.md3" },
/* icon */		"icons/icona_nailgun",
/* pickup */	"Nails",
		5,
		IT_AMMO,
		WP_NAILGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_mines",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/proxmineam.md3" },
/* icon */		"icons/icona_proxlauncher",
/* pickup */	"Proximity Mines",
		5,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_belt",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/chaingunam.md3" },
/* icon */		"icons/icona_chaingun",
/* pickup */	"Chaingun Belt",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_scout (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_scout",
		"sound/items/scout.ogg",
		{ "models/powerups/scout.md3" },
/* icon */		"icons/scout",
/* pickup */	"Scout",
		0,
		IT_PERSISTANT_POWERUP,
		PP_SCOUT,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_guard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_guard",
		"sound/items/guard.ogg",
		{ "models/powerups/guard.md3" },
/* icon */		"icons/guard",
/* pickup */	"Guard",
		0,
		IT_PERSISTANT_POWERUP,
		PP_GUARD,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_doubler",
		"sound/items/damage.ogg",
		{ "models/powerups/doubler.md3" },
/* icon */		"icons/doubler",
/* pickup */	"Damage",
		0,
		IT_PERSISTANT_POWERUP,
		PP_DOUBLER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armorregen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armorregen",
		"sound/items/armorregen.ogg",
		{ "models/powerups/ammo.md3" },
/* icon */		"icons/armor_regen",
/* pickup */	"Armor Regen",
		0,
		IT_PERSISTANT_POWERUP,
		PP_ARMORREGEN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_neutralflag (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"team_CTF_neutralflag",
		NULL,
		{ "models/flags/n_flag.md3", NULL, NULL, NULL, "models/flag3/n_flag3.md3" },
/* icon */		"gfx/2d/flag_status/flag_at_base",
/* pickup */	"Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_redcube (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_redcube",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/orb/r_orb.md3" },
/* icon */		"icons/iconh_rorb",
/* pickup */	"Red Skull",
		0,
		IT_TEAM,
		0,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_bluecube (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_bluecube",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/orb/b_orb.md3" },
/* icon */		"icons/iconh_borb",
/* pickup */	"Blue Skull",
		0,
		IT_TEAM,
		0,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_nailgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_nailgun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons/nailgun/nailgun.md3" },
/* icon */		"icons/iconw_nailgun",
/* pickup */	"Nailgun",
		10,
		IT_WEAPON,
		WP_NAILGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_prox_launcher",
		"sound/misc/w_pkup.wav",
		{ "models/weapons/proxmine/proxmine.md3" },
/* icon */		"icons/iconw_proxlauncher",
/* pickup */	"Prox Launcher",
		5,
		IT_WEAPON,
		WP_PROX_LAUNCHER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/weapons/proxmine/wstbtick.wav "
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_chaingun",
		"sound/misc/w_pkup.wav",
		{ "models/weapons/vulcan/vulcan.md3" },
/* icon */		"icons/iconw_chaingun",
/* pickup */	"Chaingun",
		100,
		IT_WEAPON,
		WP_CHAINGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ "sound/weapons/vulcan/wvulwind.wav"
	},

/*QUAKED item_spawnarmor (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_spawnarmor",
		NULL,
		{ NULL },
/* icon */		"icons/spawnarmor",
/* pickup */	"Spawn Armor",
		0,
		IT_POWERUP,
		PW_SPAWNARMOR,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_hmg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_hmg",
		"sound/misc/w_pkup.wav",
		{ "models/weapons3/hmg/hmg.md3" },
/* icon */		"icons/weap_hmg",
/* pickup */	"Heavy Machinegun",
		100,
		IT_WEAPON,
		WP_HEAVY_MACHINEGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_hmg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_hmg",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/hmgam.md3" },
/* icon */		"icons/ammo_hmg",
/* pickup */	"Heavy Bullets",
		50,
		IT_AMMO,
		WP_HEAVY_MACHINEGUN,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_pack (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_pack",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/ammopack.md3" },
/* icon */		"icons/ammo_pack",
/* pickup */	"Ammo Pack",
		1,
		IT_AMMO,
		WP_NUM_WEAPONS,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_key_silver (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_key_silver",
		"sound/items/key_silver.wav",
		{ "models/powerups/keys/key_silver.md3" },
/* icon */		"icons/key_silver",
/* pickup */	"Silver Key",
		1,
		IT_KEY,
		KEY_SILVER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_key_gold (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_key_gold",
		"sound/items/key_gold.wav",
		{ "models/powerups/keys/key_gold.md3" },
/* icon */		"icons/key_gold",
/* pickup */	"Gold Key",
		1,
		IT_KEY,
		KEY_GOLD,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_key_master (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_key_master",
		"sound/items/key_gold.wav",
		{ "models/powerups/keys/key_master.md3" },
/* icon */		"icons/key_master",
/* pickup */	"Master Key",
		1,
		IT_KEY,
		KEY_MASTER,
/* timer */	0,
/* retail */	{ 0, 0 },
/* precache */ "",
/* sounds */ ""
	},

	// end of list marker
	{ NULL }
};

int		bg_numItems = ARRAY_LEN( bg_itemlist ) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	int		i;

	// Persistant powerups carry persistantPowerup_t tags, which overlap the
	// powerup_t range in Quake Live, so only powerup and flag items qualify.
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( (bg_itemlist[i].giType == IT_POWERUP || bg_itemlist[i].giType == IT_TEAM) &&
			bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForPersistantPowerup
==============
*/
gitem_t	*BG_FindItemForPersistantPowerup( persistantPowerup_t pp ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_PERSISTANT_POWERUP && bg_itemlist[i].giTag == pp ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *pickupName ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;
#ifdef MISSIONPACK
	int		upperBound;
#endif

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
#ifdef MISSIONPACK
		if( ps->stats[STAT_PERSISTANT_POWERUP] == PP_SCOUT ) {
			return qfalse;
		}

		// we also clamp armor to the maxhealth for handicapping
		if( ps->stats[STAT_PERSISTANT_POWERUP] == PP_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else {
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;
		}

		if ( ps->stats[STAT_ARMOR] >= upperBound ) {
			return qfalse;
		}
#else
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
#endif
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
#ifdef MISSIONPACK
		if( ps->stats[STAT_PERSISTANT_POWERUP] == PP_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else
#endif
		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		return qtrue;	// powerups are always picked up

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if ( ps->stats[STAT_PERSISTANT_POWERUP] ) {
			return qfalse;
		}

		// check team only
		if( ( ent->generic1 & 2 ) && ( ps->persistant[PERS_TEAM] != TEAM_RED ) ) {
			return qfalse;
		}
		if( ( ent->generic1 & 4 ) && ( ps->persistant[PERS_TEAM] != TEAM_BLUE ) ) {
			return qfalse;
		}

		return qtrue;
#endif

	case IT_TEAM: // team items, such as flags
#ifdef MISSIONPACK		
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG ) {
				return qtrue;
			}
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			}
		}
#endif
		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

#ifdef MISSIONPACK
		if( gametype == GT_HARVESTER ) {
			return qtrue;
		}
#endif
		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;

	case IT_KEY:
		// Quake Live keys (stats[STAT_KEY_MASK]) arrive with the key doors in
		// milestone 4; until then they stay in place.
		return qfalse;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef Q3_VM
#ifndef NDEBUG
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( ( atTime - tr->trTime ) % tr->trDuration ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	case TR_QL_ACCEL:
		// Quake Live: TR_GRAVITY with the trajectory's own acceleration
		// (retail qagamex86 BG_EvaluateTrajectory case 6, 0x1002d364)
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * tr->gravity * deltaTime * deltaTime;
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	case TR_QL_ACCEL:
		// retail BG_EvaluateTrajectoryDelta case 6 (0x1002d4a4)
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= tr->gravity * deltaTime;
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType );
		break;
	}
}


const char *eventnames[EV_MAX] = {
#define EVENT_STRINGS
	#include "bg_events.h"
#undef EVENT_STRINGS
	"NULL"	// avoid -Wpedantic warnings
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/
#ifdef CGAME
void CG_StoreEvent( entity_event_t ev, int eventParm, int entityNum );
#endif

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( entity_event_t newEvent, int eventParm, playerState_t *ps, int entityNum ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif

#ifdef CGAME
	CG_StoreEvent( newEvent, eventParm, entityNum );
#endif
	
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}


/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps, -1 );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
