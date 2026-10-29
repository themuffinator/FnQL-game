// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

// Retail qagame publishes "baseqz" in CS_GAME_VERSION (HLIL qagamex86 0x1006648d)
// and the retail cgame refuses a server whose game version differs.
#define	GAME_VERSION		"baseqz"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	0.66

#define	HEALTH_SOFT_LIMIT	100
#define	AMMO_HARD_LIMIT		200

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	LIGHTNING_RANGE		768

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	DROPPED_TIME		30000   // 30 seconds before removing  dropped items

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

#define	PM_STEP_HEIGHT		18

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
//
// Quake Live configstring layout.  Indices are retail facts taken from the
// retail qagame's SetConfigstring/GetConfigstring call sites (import slots 25
// and 26) and the retail cgame dispatcher; they differ from Quake III below
// CS_MODELS.  Payload grammars are documented where they are implemented.
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// not written by retail qagame
#define	CS_WARMUP				5		// retail payload is an infostring: "\time\<ms>"
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define	CS_VOTE_TIME			8
#define	CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11
#define	CS_GAME_VERSION			12		// GAME_VERSION
#define	CS_LEVEL_START_TIME		13		// so the timer only shows the current level
#define	CS_INTERMISSION			14		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define	CS_ITEMS				15		// string of 0's and 1's that tell which items are present
#define	CS_AWARD_LEADERS		16		// infostring of match leaders (keys e, ed, tg, tgd, sg, ...)

#define	CS_MODELS				17
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)		// 273
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)		// 529
#define	CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)	// 593, "unknown" at index 0
#define	CS_FLAGSTATUS			658		// string indicating flag status in CTF

#define	CS_FIRST_PLACE			0x293
#define	CS_SECOND_PLACE			0x294
#define	CS_MATCH_STATE			0x295	// infostring; retail keys: time, round, turn, state
#define	CS_ROUND_START_TIME		0x296
#define	CS_RED_PLAYERS_LEFT		0x297
#define	CS_BLUE_PLAYERS_LEFT	0x298
#define CS_PRACTICE_FLAGS		0x29B	// practiceflags (retail qagamex86 0x10059338)
#define CS_TEAM_SPEC_FREECAM	0x29C	// g_teamSpecFreeCam (retail qagamex86 0x10059359)
#define	CS_TIMEOUT_START_TIME	0x29D
#define	CS_TIMEOUT_END_TIME		0x29E
#define	CS_RED_TIMEOUTS_LEFT	0x29F
#define	CS_BLUE_TIMEOUTS_LEFT	0x2A0
#define	CS_CUSTOM_SETTINGS		0x2A1	// written next to g_customSettings handling
#define	CS_PLAYER_CYLINDERS		0x2A2
#define	CS_FACTORY_FLAGS		0x2A3
#define	CS_ENABLE_BREATH		0x2A4
#define	CS_DOM_CAPTURE_TIME		0x2A5
#define	CS_MAP_AUTHOR			0x2A6
#define	CS_MAP_AUTHOR2			0x2A7
// 0x2A9..0x2AC are published by the engine from cvar flag groups (pmove
// settings, armor tiers, weapon reload times, player appearance); the game
// only registers the cvars.
#define	CS_PMOVE_SETTINGS		0x2A9
#define	CS_ARMOR_SETTINGS		0x2AA
#define	CS_WEAPON_SETTINGS		0x2AB
#define	CS_APPEARANCE_SETTINGS	0x2AC
// duel's two players, lower client number first (retail CalculateRanks,
// qagamex86 0x10056836); the wire-contract names stay as aliases
#define	CS_DUEL_SCORE1			0x2AD
#define	CS_DUEL_SCORE2			0x2AE
#define	CS_DUEL_CLIENT1			0x2AF
#define	CS_DUEL_CLIENT2			0x2B0
#define	CS_DUEL_NAME1			0x2B1
#define	CS_DUEL_NAME2			0x2B2
#define	CS_QL_2AD				CS_DUEL_SCORE1
#define	CS_QL_2AE				CS_DUEL_SCORE2
#define	CS_QL_2AF				CS_DUEL_CLIENT1
#define	CS_QL_2B0				CS_DUEL_CLIENT2
#define	CS_QL_2B1				CS_DUEL_NAME1
#define	CS_QL_2B2				CS_DUEL_NAME2
#define	CS_QL_2B3				0x2B3
#define	CS_AWARD_ITEM_CONTROL	0x2B4
#define	CS_AWARD_ACCURACY		0x2B5
#define	CS_RED_FLAG_ORIGIN		0x2B6	// where each team flag stands: an x, y, z infostring
#define	CS_BLUE_FLAG_ORIGIN		0x2B7
#define	CS_QL_2B6				CS_RED_FLAG_ORIGIN
#define	CS_QL_2B7				CS_BLUE_FLAG_ORIGIN
#define	CS_AWARD_VALUABLE		0x2B8
#define	CS_AWARD_VALUABLE_OFFENSE	0x2B9
#define	CS_AWARD_VALUABLE_DEFENSE	0x2BA
#define	CS_AWARD_DAMAGE			0x2BB
#define	CS_RACE_SCORES			0x2BC
#define	CS_RACE_INFO			0x2BD
#define	CS_MATCH_AWARDS			0x2BF
#define	CS_SETTINGS_MASK		0x2C0
#define	CS_ROTATION_TITLES		0x2C1
#define	CS_ROTATION_VOTES		0x2C2
#define	CS_INTERMISSION_EXIT	0x2C3
#define	CS_READY_UP				0x2C4
#define	CS_RR_PING_RATE			0x2C5
#define	CS_RACE_RECORDS			0x2C6
#define	CS_DISABLED_LOADOUTS	0x2C7
#define	CS_MATCH_GUID			0x2C8
#define	CS_STARTING_WEAPONS		0x2C9

#define	CS_MAX					(CS_STARTING_WEAPONS+1)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

// g_gametype values; order matches the retail gametype name tables in both
// qagamex86 and cgamex86 (docs/contract/retail_tables.json "gametypes").
typedef enum {
	GT_FFA,				// free for all
	GT_TOURNAMENT,		// duel
	GT_RACE,			// race reuses the Quake III single player slot

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_CLAN_ARENA,
	GT_CTF,				// capture the flag
	GT_1FCTF,			// one flag ctf
	GT_OBELISK,			// overload
	GT_HARVESTER,
	GT_FREEZE,			// freeze tag
	GT_DOMINATION,
	GT_ATTACK_DEFEND,
	GT_RED_ROVER,
	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_TUTORIAL			// retail pmove type 6 (docs/contract/retail_tables.json "pmTypes")
} pmtype_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define PMF_ATTACK_LOCKOUT	4		// firing gated (round start, controller)
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	128		// pm_time is waterjump (retail PM_CheckWaterJump ORs 0x80, HLIL 0x1002f814)
#define PMF_NO_MOVE			256		// commandTime advances but the player does not move
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_INVULEXPAND		16384	// invulnerability sphere set to full size
#define PMF_WEAPON_RESET	32768
#define PMF_AIR_CONTROL		65536	// per-player movement profile bits (milestone 3)
#define PMF_DOUBLE_JUMP		131072
#define PMF_REQUIRE_JUMP_RELEASE	262144
#define PMF_INFECTED		524288	// red rover infection: no weapon pickups (retail 0x100645fc)
#define PMF_CROUCH_SLIDE	1048576

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

// Quake Live movement settings.  The server fills them from the pmove_*
// cvars (g_ql_cvars.h); the engine publishes the same cvars as the
// CS_PMOVE_SETTINGS infostring, from which the cgame fills its copy, so
// both sides parse identical strings.  Defaults are the retail cvar defaults.
typedef struct {
	float	airAccel;
	float	airStepFriction;
	int		airSteps;
	float	airStopAccel;
	int		autoHop;
	int		bunnyHop;
	int		chainJump;				// 0 none, 1 scale, 2 additive, 3 scale then additive
	float	chainJumpVelocity;
	float	circleStrafeFriction;
	float	crouchSlideFriction;
	int		crouchSlideTime;
	int		crouchStepJump;
	float	jumpTimeDeltaMin;
	float	jumpVelocity;
	float	jumpVelocityMax;
	float	jumpVelocityScaleAdd;
	float	jumpVelocityTimeThreshold;
	float	jumpVelocityTimeThresholdOffset;
	int		noPlayerClip;
	int		rampJump;
	float	rampJumpScale;
	float	stepHeight;
	int		stepJump;
	float	stepJumpVelocity;
	float	strafeAccel;
	float	velocityGH;				// grappling hook pull speed
	float	walkAccel;
	float	walkFriction;
	float	waterSwimScale;
	float	waterWadeScale;
	int		weaponDropTime;
	int		weaponRaiseTime;
	float	wishSpeed;
	int		weaponReload[MAX_WEAPONS];	// msec between shots, from the weapon_reload_* cvars
} pmoveSettings_t;

// retail forms of the shared math routines (bg_pmove.c "Retail arithmetic")
double	PM_Dot3( const vec3_t a, const vec3_t b );
float	PM_VectorNormalize( vec3_t v );
float	PM_VectorNormalize2( const vec3_t v, vec3_t out );
void	PM_CrossProduct( const vec3_t a, const vec3_t b, vec3_t cross );
void	PM_AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

void	BG_DefaultPmoveSettings( pmoveSettings_t *settings );
void	BG_PmoveSettingsFromInfo( const char *info, pmoveSettings_t *settings );
void	BG_WeaponReloadFromInfo( const char *info, pmoveSettings_t *settings );

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;			// horizontal speed, set by PM_Footsteps
	float		stepUp;				// height of the last step up, for cgame step smoothing
	int			stepTime;			// cmd.serverTime of that step

	const pmoveSettings_t	*settings;	// Quake Live movement settings; NULL = retail defaults

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );

	// retail pmove_t +0xe8, set by the game: quarters the hook pull and keeps
	// the ground plane (what the game sets it for is not yet identified)
	qboolean	grappleSlow;
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// Quake Live layout.  Retail evidence: STAT_HOLDABLE_ITEM (stats[1]) indexes
// bg_itemlist in the flight thrust code, STAT_PERSISTANT_POWERUP (stats[2])
// holds the persistant powerup tag in the damage multiplier, and stats[9],
// [11] and [12] carry the flight holdable's thrust, fuel and recharge
// (HLIL qagamex86 0x1002fa80, 0x1006f290).
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_PERSISTANT_POWERUP,		// a persistantPowerup_t tag
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,
	STAT_BATTLESUIT_FRAG_COUNT,		// kills while wearing the battle suit (retail player_die
									// 0x10047710); Quake III's dead yaw slot
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission
	STAT_MAX_HEALTH,				// health / armor limit, changable by handicap
	STAT_CHAINGUN_SPINUP,			// chaingun spin accumulator
	STAT_PLAYER_ITEM_THRUST,		// flight holdable thrust
	STAT_PLAYER_ITEM_TIME_MAX,		// holdable fuel capacity
	STAT_PLAYER_ITEM_TIME,			// holdable fuel remaining
	STAT_PLAYER_ITEM_RECHARGE,		// holdable fuel recharge
	STAT_QUAD_FRAG_COUNT,
	STAT_ARMOR_TIER,				// armorTier_t of the armor being worn
	STAT_KEY_MASK					// keyMask_t bits of carried keys
} statIndex_t;


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	// Quake Live has no PERS_ATTACKEE_ARMOR: the award counters below sit one
	// slot lower than in Team Arena (docs/contract/retail_enums.json)
	PERS_KILLED,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
	PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
	PERS_DEFEND_COUNT,				// defend awards
	PERS_ASSIST_COUNT,				// assist awards
	PERS_GAUNTLET_FRAG_COUNT,		// kills with the gauntlet
	PERS_CAPTURES,					// captures
	PERS_KILL_COUNT					// kill beep counter
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#ifdef MISSIONPACK
#define EF_TICKING			0x00000002		// used to make players play the prox mine ticking sound
#endif
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#ifdef MISSIONPACK
#define	EF_KAMIKAZE			0x00000200
#endif
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_SPECTATOR		0x00004000		// Quake Live: on the spectator team (retail ClientSpawn 0x1003bda2)
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
#define EF_NAIL_BOUNCE		0x00100000		// Quake Live: a nail that bounces off walls (g_nailbounce)

#define EF_PERSISTANT ( EF_CONNECTION | EF_VOTED | EF_TEAMVOTED )
#define EF_AWARDS ( EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP )

#define EF_NOPREDICT ( EF_AWARDS | EF_PERSISTANT | EF_TALK )

// ps->powerups[] and entityState_t->powerups bit indexes.  Quake Live order,
// established from retail code rather than inherited: the quad multiplier
// reads powerups[4] (HLIL qagamex86 0x1006f2a9), the flight holdable drives
// powerups[9] (0x1002fa80), and the retail cgame draws the spawn armor,
// quad, battlesuit, invisibility, regen, invulnerability, scout, guard,
// doubler, armor regen and frozen effects from bits 0, 4, 5, 7, 8, 10..15
// (HLIL cgamex86 0x10041860, 0x1004259f..0x10042776).  Item giTags of
// IT_POWERUP and IT_TEAM items index this table.
// NOTE: may not have more than 16
typedef enum {
	PW_SPAWNARMOR,		// spawn protection shell; Quake III's PW_NONE slot
	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,
	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,			// active while the flight holdable thrusts
	PW_INVULNERABILITY,
	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	PW_ARMORREGEN,
	PW_FROZEN,			// freeze tag

	PW_NUM_POWERUPS
} powerup_t;

// stats[STAT_HOLDABLE_ITEM] holds an item index; holdable item giTags
// (retail bg_itemlist) use these values.  Flight is a fuelled holdable.
typedef enum {
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,
	HI_FLIGHT,

	HI_NUM_HOLDABLE
} holdable_t;

// IT_PERSISTANT_POWERUP giTags, stored in stats[STAT_PERSISTANT_POWERUP].
typedef enum {
	PP_NONE,
	PP_SCOUT,
	PP_GUARD,
	PP_DOUBLER,
	PP_ARMORREGEN
} persistantPowerup_t;

// IT_ARMOR giTags and stats[STAT_ARMOR_TIER] values.
typedef enum {
	ARMOR_TIER_NONE,
	ARMOR_TIER_RED,
	ARMOR_TIER_YELLOW,
	ARMOR_TIER_GREEN,
	ARMOR_TIER_SHARD
} armorTier_t;

// IT_HEALTH giTags.
typedef enum {
	HEALTH_TIER_NONE,
	HEALTH_TIER_MEGA,
	HEALTH_TIER_LARGE,
	HEALTH_TIER_MEDIUM,
	HEALTH_TIER_SMALL
} healthTier_t;

// IT_KEY giTags; bits of stats[STAT_KEY_MASK].
typedef enum {
	KEY_SILVER = 1,
	KEY_GOLD = 2,
	KEY_MASTER = 4
} keyMask_t;


// Weapon numbers equal the weapon item giTags in the retail bg_itemlist.
typedef enum {
	WP_NONE,

	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,
	WP_HEAVY_MACHINEGUN,

	WP_NUM_WEAPONS,
	WP_PENDING = WP_NUM_WEAPONS, // used in ui_players.c
	WP_MAX_WEAPONS = 16
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004
#define PLAYEREVENT_FIRSTFRAG			0x0008

// Quake Live medals: EV_AWARD carries the medal in generic1, the awarded
// client in clientNum and that client's count of it in modelindex2
// (retail qagamex86 0x10046550)
typedef enum {
	MEDAL_COMBOKILL,
	MEDAL_RAMPAGE,
	MEDAL_MIDAIR,
	MEDAL_REVENGE,
	MEDAL_PERFORATED,
	MEDAL_HEADSHOT,
	MEDAL_ACCURACY,
	MEDAL_QUADGOD,
	MEDAL_FIRSTFRAG,
	MEDAL_PERFECT,
	MEDAL_NUM_MEDALS
} medal_t;

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
#define EVENT_ENUMS
	#include "bg_events.h"
#undef EVENT_ENUMS
} entity_event_t;


typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE,
	GTS_REDTEAM_WINS,			// Quake Live: the match result (retail LogExit, 0x10057751)
	GTS_BLUETEAM_WINS,
	GTS_REDTEAM_WINS_ROUND,
	GTS_BLUETEAM_WINS_ROUND,
	GTS_ROUND_DRAW,
	GTS_LAST_STANDING
} global_team_sound_t;

// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

typedef enum {
	TAG_NONE = 0,
	TAG_DONTSPAWN,
} tag_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE, 
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
#define MOD_ENUMS
	#include "bg_mods.h"
#undef MOD_ENUMS
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM,
	IT_KEY					// Quake Live keys; giTag is a keyMask_t bit
} itemType_t;

// Retail rows carry eight model slots; slot 4 holds the alternate (flag3)
// model of the team flags.
#define MAX_ITEM_MODELS 8

typedef struct gitem_s {
	const char	*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model[MAX_ITEM_MODELS];

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			const giTag;	// weapon_t, powerup_t, holdable_t, armorTier_t, ... by giType

	int			timer;			// retail row flag: armor, mega health, medkit and powerups
	int			retail[2];		// retail row trailing values (set on the red and blue
								// flags only); meaning not yet identified

	char		const *precaches;		// string of all models and images this item will use
	char		const *sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
// the retail weapon table (qagamex86 0x1008ff10 + w * 0x30): the most ammo a
// weapon holds (also its warmup ammo) and what an ammo pack adds to it
extern	const int	bg_weaponMaxAmmo[WP_NUM_WEAPONS];
extern	const int	bg_weaponPackAmmo[WP_NUM_WEAPONS];
extern	int		bg_numItems;

gitem_t	*BG_FindItem( const char *pickupName );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForPersistantPowerup( persistantPowerup_t pp );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps,
								 int time, int warmupTime, qboolean armorTiered, int weaponRespawn );


// g_dmflags->integer flags
// Quake Live dmflags (retail ClientEvents 0x10034860, ClientThink 0x1003517e,
// G_Damage 0x10049604)
#define DF_NO_SELF_HEALTH_DAMAGE	4
#define DF_NO_SELF_ARMOR_DAMAGE	8
#define	DF_NO_FALLING			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;

// calculated by modulus.c for appropriate dividers:
#define TMOD_004  4272943
#define TMOD_075  2292106
#define TMOD_1000 5730265
#define TMOD_2000 5730265

void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( entity_event_t newEvent, int eventParm, playerState_t *ps, int entityNum );

void	BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime, int itemHeight );


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		65536	// retail scripts/arenas.txt is about 13 KB

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		65536


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704

// custom functions

int BG_sprintf( char *buf, const char *format, ... );
int Q_vsprintf( char *buffer, const char *fmt, va_list argptr );

char *Q_stristr( const char * str1, const char * str2 );

char *strtok( char *strToken, const char *strDelimit );
char *EncodedString( const char *str );
char *DecodedString( const char *str );

void BG_CleanName( const char *in, char *out, int outSize, const char *blankString );
char *BG_StripColor( char *string );

void Q_strcpy( char *dst, const char *src );
char *Q_stradd( char *dst, const char *src );
int Q_sscanf( const char *buffer, const char *fmt, ... );

qboolean replace1( const char match, const char replace, char *str );

qboolean  BigEndian( void );

//#endif // __BG_PUBLIC_H
