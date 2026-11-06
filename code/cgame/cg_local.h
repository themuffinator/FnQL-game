// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "../game/q_shared.h"
#include "tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"


// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#ifdef MISSIONPACK
#define CG_FONT_THRESHOLD 0.1
#endif

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
#define	ZOOM_TIME			150
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4

#define PICKUP_ICON_SIZE	32
#define PICKUP_TEXT_SIZE	12

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define	CG_NUM_CROSSHAIRS		29

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define	DEFAULT_MODEL			"sarge"
#ifdef MISSIONPACK
#define	DEFAULT_TEAM_MODEL		"james"
#define	DEFAULT_TEAM_HEAD		"*james"
#else
#define	DEFAULT_TEAM_MODEL		"sarge"
#define	DEFAULT_TEAM_HEAD		"sarge"
#endif

#define DEFAULT_REDTEAM_NAME		"Stroggs"
#define DEFAULT_BLUETEAM_NAME		"Pagans"

// dynamic lights definitions
#define GL_EXPLOSION_RADIUS		300
#define RL_EXPLOSION_RADIUS		300
#define BFG_EXPLOSION_RADIUS	300

// holdable powerups
#define POWERUP_GLOW_RADIUS		200
#define POWERUP_GLOW_RADIUS_MOD	31

// running projectiles
#define HOOK_GLOW_RADIUS		200
#define MISSILE_GLOW_RADIUS		200

// on-hand firing weapons
#define MG_FLASH_RADIUS			200 // original: 300
#define WEAPON_FLASH_RADIUS		300
#define WEAPON_FLASH_RADIUS_MOD	31

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;


typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	qboolean		painIgnore;
	int				lightningFiring;

	vec3_t			muzzleOrigin;

	// railgun trail spawning
	vec3_t			railgunImpact;
	qboolean		railgunFlash;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;
} playerEntity_t;

//=================================================



// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;
	int				delaySpawn;
	qboolean		delaySpawnPlayed;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;
	
	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
#ifdef MISSIONPACK
	LE_KAMIKAZE,
	LE_INVULIMPACT,
	LE_INVULJUICED,
#endif
	LE_SHOWREFENTITY,
	LE_QL_GIB_FLASH	// retail type 15 light (0x1001f730)
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008			// sound 2 for kamikaze
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD,
	LEMT_ICE,
	LEMT_ELECTRO	// retail mark type 3 (0x1001e316)
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS,
	LEBS_ICE,
	LEBS_ELECTRO	// retail bounce type 3 (0x1001ec7a)
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	refEntity_t		refEntity;		
} localEntity_t;

//======================================================================


typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				gauntletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean		perfect;
	team_t			team;
	int alive, kills, deaths, bestWeapon, bestAccuracy, damageGiven, damageTaken;
	int roundScore, suicides, teamKills, teamKilled;
	int itemPickups[4]; /* red/yellow/green armor, mega */
	float itemWait[4];
	int powerupPickups[5]; /* quad, suit, regen, haste, invisibility */
	struct { int hits, shots, accuracy, damage, kills; } weaponStats[16];

	int				minx, maxx;
	int				miny, maxy;

} score_t;

qboolean CG_ParseScoreCommand(const char *command);
void CG_UpdateTeamInfoFromSnapshot(const snapshot_t *snapshot);
void CG_SetAccuracyOverlay(qboolean show);
void CG_SetPlayerStatsRequest(qboolean down);
void CG_DrawCrosshairNameText(float *color);
qboolean CG_ClientMuted(int client);
qboolean CG_AccuracyOverlayVisible(void);
void CG_DrawAccuracyColumn(qboolean icons, float x, float y, float width,
    float step, float scale, vec4_t color);
int CG_ScoreRow(float feeder, int index);
const char *CG_ScoreCell(float feeder, int index, int column, qhandle_t *handle);
const char *CG_SelectedPlayerAccuracy(void);
const char *CG_ScoreMode(void);
const char *CG_ScoreMenuName(qboolean end);
qboolean CG_ScoreVisible2(int flags);
qboolean CG_OwnerDrawVisibleEx(int flags, int flags2);

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	32

typedef struct {
	int speaking, speakingTime;
	qboolean		infoValid;

	char			name[MAX_NAME_LENGTH];
	team_t			team;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	vec3_t			color1;
	vec3_t			color2;

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode
	int				spectatorOnly;	// "so" (retail clientInfo +0x714)
	int				playerQueue;	// "pq" (retail clientInfo +0x718)

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;
	int				invulnerabilityStartTime;
	int				invulnerabilityStopTime;

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
	char			headModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
	qboolean		deferred;

	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model

	qhandle_t		legsModel;
	qhandle_t		legsSkin;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;

	qhandle_t		headModel;
	qhandle_t		headSkin;

	qhandle_t		modelIcon;
	float			modelHeightScale;	// retail clientInfo +0x2d8 (0x1003d260)

	animation_t		animations[MAX_TOTALANIMATIONS];

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	qboolean		coloredSkin;
	vec3_t			headColor;
	vec3_t			bodyColor;
	vec3_t			legsColor;

} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	qhandle_t		attachedAmmoModel;	// grapple "_ammo.md3" on tag_ammo (retail weaponInfo +0x18)

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	void			(*ejectBrassFunc)( centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
	qboolean		loopFireSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
	qhandle_t		icon_df;
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		10

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK		16 /* Retail queue: 0x10043570. */
#define MAX_SOUNDBUFFER		20

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16

#define PICKUP_PREDICTION_DELAY 200

#define NUM_SAVED_STATES ( CMD_BACKUP + 2 )

#define CG_MAX_TIMED_ITEMS 10
typedef struct {
	int entity, side, item, remaining, distance, group;	// remaining ms, counted down per frame
	vec3_t origin;
} cgTimedItem_t;

#define CG_MAX_DAMAGE_PLUMS 32
typedef struct {
	qboolean active;
	int startedAt;
	vec3_t origin;
	vec4_t color;
	float horizontalSpeed, verticalSpeed;
	char text[16];
} cgDamagePlum_t;

void CG_DamagePlum(const centity_t *cent);
void CG_DrawDamagePlums(void);

void CG_ClearTimedItems(void);
void CG_UpdateTimedItems(int frametime);
qboolean CG_TimedItemShown(int mask, const gitem_t *item);
void CG_DrawItemTimers(void);

void CG_VitalsCleanName(char *name);
int CG_VitalsFitName(char *name, int size, float maxWidth);
void CG_DrawTeamVitals(void);
void CG_ParseAppearanceSettings(void);
void CG_CrosshairColorEntry(int index, vec4_t color);
void CG_RecordCrosshairHit(const playerState_t *ps, const playerState_t *ops);
qboolean CG_CrosshairHitStyle(float *w, float *h, vec4_t color);
void CG_DrawQLCrosshair(void);
const char *CG_DemoHudTime(int msec);
void CG_DrawDemoHud(void);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
void CG_RegisterGoreDLC(void);
qboolean CG_GoreGibPlayer(const vec3_t origin);
qboolean CG_GoreBleed(const vec3_t origin);
qboolean CG_GoreBloodTrail(localEntity_t *le);
sfxHandle_t CG_GoreImpactSound(void);
qboolean CG_GoreBloodMark(const trace_t *trace);
void CG_CloseInitialScoreboard(void);
void CG_CheckInitialScoreboard(void);
void CG_ComputeModelHeightScale(clientInfo_t *ci, const char *modelName);
void CG_RefreshModelHeightScales(void);
qboolean CG_LeaderModelView(float x, float y, float w, float h, float heightScale, refdef_t *refdef, vec3_t origin);
void CG_DrawLeaderModel(float x, float y, float w, float h, int torsoAnim, int weapon, const vec3_t angles);
void CG_DrawLeaderModelOwnerDraw(int ownerDraw, float x, float y, float w, float h);

/* Retail world-marker projection (0x10011680), shared by race navigation
 * icons and points of interest. */
void CG_WorldMarkerWidths(float scale, float *worldWidth, float *minimum);
qboolean CG_ProjectWorldMarker(const vec3_t point, float worldWidth, float minimum,
                               float *x, float *y, float *size);

/* EV_POI pings (retail 0x1001c228): Quad Hog carriers, neutral flag
 * carriers and Red Rover infection. Retail's marker queue holds 32. */
#define CG_MAX_POINTS_OF_INTEREST 32
#define CG_SETTING_QUAD_HOG (1u << 18)
#define CG_SETTING_RR_INFECTED (1u << 26)
typedef struct {
	qboolean active;
	int startedAt, lifetime, fadeDelay;
	int client, team;
	vec3_t origin;
	vec4_t color;
	float worldWidth, minimum;
	qhandle_t shader;
} cgPointOfInterest_t;
typedef struct {
	qhandle_t quadHog, infected, neutralFlag, harvesterCapture, flagCapture;
} cgPoiMedia_t;
void CG_RegisterPoiMedia(void);
void CG_RegisterFlagStatusIcons(void);
qhandle_t CG_FlagStatusIcon(int team, int state);
qhandle_t CG_FlagTrackPointer(void);
void CG_DrawFlagTrack(float x, float y, float w, float h);
qboolean CG_DrawTeamFlagStatus(int ownerDraw, float x, float y, float w, float h);
void CG_DrawOneFlagStatus(float x, float y, float w, float h);
void CG_PointOfInterest(const centity_t *cent);
void CG_DrawPointsOfInterest(void);
void CG_DrawOverheadNames(void);
const float *CG_OverheadTeamColor(int team);
void CG_OverheadHealthColor(int client, int value, vec4_t color);
const gitem_t *CG_PowerupItem(int powerup);
/* Per-frame teammate markers (retail marker type 4, 0x10040dcd). */
#define CG_MAX_TEAMMATE_MARKERS 32
typedef struct { vec3_t origin; vec4_t color; float worldWidth, minimum; qhandle_t shader; } cgTeammateMarker_t;
void CG_AddTeammateMarker(const vec3_t origin, const vec4_t color, qhandle_t shader);
void CG_RecordObjectiveBase(int team, const vec3_t origin);
void CG_ObjectiveMarkers(const centity_t *cent);
typedef struct { qboolean active; int startedAt; float lifetime, worldWidth, minimum; vec3_t origin; } cgDeadFriend_t;
void CG_DeadTeammateMarker(const entityState_t *obituary);
void CG_RetailObituary(const entityState_t *ent);
void CG_AddObituary(const char *target, const char *attacker, int targetTeam, int attackerTeam,
                    qboolean hasAttacker, qhandle_t icon);
void CG_ClearObituaries(void);
void CG_ClearChatArea(void);
void CG_ChatAreaAdd(const char *text, int type, int extraMsec);
void CG_ChatHistoryShow(qboolean show);
void CG_ChatHistoryToggle(void);
qboolean CG_ChatHistoryShown(void);
int CG_ObituaryRows(void);
qhandle_t CG_LightningBoltShader(void);

void CG_RegisterQLEventSounds(void);
qboolean CG_QL_EntityEvent(centity_t *cent);
void CG_PushReward(sfxHandle_t sound, qhandle_t shader, int count);
void CG_FollowPowerup(int client, qboolean flag);
void CG_GlobalItemPickupAnnouncement(const entityState_t *event, int giType, int giTag);
void CG_PersistentPowerupAnnouncement(int giTag);

typedef struct {
	qboolean active, hasSplit;
	int startedAt, lastTime, bestTime, reportedSplits;
	int nextPoint, followingPoint, splitDelta, pointsPassed;
} cgRaceState_t;
typedef struct {
	qhandle_t start, checkpoint, finish, inactive;
	qhandle_t base, redSkin, blueSkin, neutralSkin;
	qhandle_t icons[3];
} cgRaceMedia_t;
void CG_ResetRace(qboolean clearRecords, qboolean notifyServer);
qboolean CG_ParseRaceCommand(const char *command);
void CG_RaceEvent(const entityState_t *event);
void CG_RaceLevelTimeChanged(int oldTime, int newTime);
void CG_DrawRaceHud(int ownerDraw, float x, float y, float scale, vec4_t color, int align);
void CG_RaceRespawnCommand(void);
void CG_RegisterRaceMedia(void);
qboolean CG_DrawRacePoint(const centity_t *cent);
void CG_DrawRaceMarkers(void);

typedef struct {
	qboolean chatExpanded;
	cgRaceState_t race;
	qboolean raceRespawnUsed;
	cgTimedItem_t timedItems[CG_MAX_TIMED_ITEMS];
	int timedItemCount;			// records filled (retail 0x10ab994c)
	int messageTime, noticeTime;	// last message/notice expiry (retail 0x10a5f2f8/0x10a5f2fc)
	int crosshairHitTime, crosshairHitLevel;	// retail 0x10ab8fb0/0x10ab8fb4
	qboolean initialScoreboard;	// scoreboard raised by CG_Init (retail 0x10a9c9ac)
	cgDamagePlum_t damagePlums[CG_MAX_DAMAGE_PLUMS];
	unsigned damagePlumRandom;
	cgPointOfInterest_t pointsOfInterest[CG_MAX_POINTS_OF_INTEREST];
	cgTeammateMarker_t teammateMarkers[CG_MAX_TEAMMATE_MARKERS];
	int teammateMarkerCount;
	cgDeadFriend_t deadFriends[CG_MAX_TEAMMATE_MARKERS];
	vec3_t objectiveBases[3];		// neutral, red, blue (retail 0x10aba978)
	qboolean objectiveBaseSeen[3];
	int			clientFrame;		// incremented each frame

	int			clientNum;
	
	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t	activeSnapshots[2];

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;
	int			teamJoinTime;	// last spectator-to-team transition (retail 0x10a9c9b0)

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;
	int             weaponPrimary; /* queued loadout slot, native user command */
	int             weaponSelectPrevious, weaponSelectObserved; /* per-frame history */

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;
	int			userCmdFov; // retail 4:3 command value, before projection/water warp

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
	int scoreTeamStats[2][17]; /* CTF order; TDM omits flag/medkit fields. */
	int weaponAccuracy[WP_NUM_WEAPONS]; /* acc/pstats, including WP_NONE */
	qboolean showAccuracy;
	int accuracyRequestTime;
	qboolean playerStatsHeld;
	int scoreRoundStats[20]; /* attack/defend supplementary scores_ad */
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	char		killerName[MAX_NAME_LENGTH+32];
	qhandle_t	killerIcon;	// retail 0x10aa69b0, the killing weapon icon
	int			killerTime;
	char			spectatorList[MAX_STRING_CHARS];		// list of names
	int				spectatorLen;												// length of list
	float			spectatorWidth;											// width in device units
	int				spectatorTime;											// next time to offset
	int				spectatorPaintX;										// current paint x
	int				spectatorPaintX2;										// current paint x
	int				spectatorOffset;										// current offset from start
	int				spectatorPaintLen; 									// current offset from start

#ifdef MISSIONPACK
	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];
#endif

	// centerprinting
	int			centerPrintTime;
	int			centerPrintCharWidth;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// kill timers for carnage reward
	int			lastKillTime;

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	char		attackerName[MAX_NAME_LENGTH];
	int			attackerClientNum;
	int			attackerTime;

	int			voiceTime;

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qboolean matchEnded; /* retail suppresses new announcements after the result */
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];
	qhandle_t	soundPlaying;

	// for voice chat buffer
#ifdef MISSIONPACK
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;
#endif

	// warmup countdown
	int			warmup;
	int			warmupCount;
	int			warmupFightSound;	// last time "Fight!" sound was played

	//==========================

	int			itemPickup;
	int			itemPickupCount;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	int			damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int			nextOrbitTime;

	//qboolean cameraMode;		// if rendering from a loaded camera

	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	// optimized prediction
	int				lastPredictedCommand;
	int				lastServerTime;
	playerState_t	savedPmoveStates[ NUM_SAVED_STATES ];
	int				stateHead, stateTail;

	int				meanPing;
	int				timeResidual;
	int				allowPickupPrediction;

	int				followTime;
	int				followClient;

	qboolean		skipDFshaders;
} cg_t;


// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t sizeCursor, selectCursor;
	qhandle_t	charsetShader;
	qhandle_t	charsetProp;
	qhandle_t	charsetPropGlow;
	qhandle_t	charsetPropB;
	qhandle_t	whiteShader;

	qhandle_t	redCubeModel;
	qhandle_t	blueCubeModel;
	qhandle_t	redCubeIcon;
	qhandle_t	blueCubeIcon;
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

#ifdef MISSIONPACK
	qhandle_t	overloadBaseModel;
	qhandle_t	overloadTargetModel;
	qhandle_t	overloadLightsModel;
	qhandle_t	overloadEnergyModel;

	qhandle_t	harvesterModel;
	qhandle_t	harvesterRedSkin;
	qhandle_t	harvesterBlueSkin;
	qhandle_t	harvesterNeutralModel;
#endif

	qhandle_t	armorModel;
	qhandle_t	armorIcon;
    qhandle_t infiniteAmmoShader;
    qhandle_t weaponBarLitShader;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;

	qhandle_t	smoke2;

	qhandle_t	machinegunBrassModel;
	qhandle_t	shotgunBrassModel;

	qhandle_t	railRingsShader;
	qhandle_t	railCoreShader;

	qhandle_t	lightningShader;

	qhandle_t	lightningBolts[5];
	qhandle_t	electroGibModel;
	sfxHandle_t	electroGibSounds[2];
	sfxHandle_t	electroBounceSounds[2];
	qhandle_t	friendShader;
	qhandle_t	healthSegmentShader;	// retail 0x10a5f4a8, overhead health
	qhandle_t	obituaryIcon;	// retail 0x10a5f680 icons/icon_frag
	qhandle_t	thawIcon;	// retail 0x10a5f684 icons/thaw, Freeze Tag up
	qhandle_t	speakingIcon;
	qhandle_t	mutedIcon;
	qhandle_t	friendHitShader;
	qhandle_t	friendDeadShader;
	qhandle_t	voiceWindowShader;	// ui/assets/voiceWindow (retail 0x10a5f4d4)
	qhandle_t	frozenSpriteShader;
	qhandle_t	flagCarrierShader;
	qhandle_t	flagCarrierHitShader;

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	selectShader;
	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[CG_NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	smokePuffShader;
	qhandle_t	iceTrailShader, iceMarkShader, surfacePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	shotgunSmokePuffShader;
	qhandle_t	plasmaBallShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;
#ifdef MISSIONPACK
	qhandle_t	nailPuffShader;
	qhandle_t	blueProxMine;
#endif

	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[5];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
	qhandle_t	noPlayerClipShader;
	qhandle_t	ghostWeaponShader;
	qhandle_t	icePlayerShader[3];
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	hastePuffShader;
#ifdef MISSIONPACK
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;
#endif

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
	qhandle_t	bloodExplosionShader;

	// special effects models
	qhandle_t	teleportEffectModel;
	qhandle_t	teleportEffectShader;
#ifdef MISSIONPACK
	qhandle_t	kamikazeEffectModel;
	qhandle_t	kamikazeShockWave;
	qhandle_t	kamikazeHeadModel;
	qhandle_t	kamikazeHeadTrail;
	qhandle_t	guardPowerupModel;
	qhandle_t	scoutPowerupModel;
	qhandle_t	doublerPowerupModel;
	qhandle_t	ammoRegenPowerupModel;
	qhandle_t	invulnerabilityImpactModel;
	qhandle_t	invulnerabilityJuicedModel;
	// gore DLC (retail 0x10022ffc/0x10020ea8: registered only when dlc_gibs exists)
	qboolean	goreDLC;
	qhandle_t	dlcGibModels[15];		// skull, brain, chest, abdomen, intestine, arm x2, forearm x2, fist x2, leg x2, foot x2
	qhandle_t	dlcBloodSpray[4];		// bloodSpray1..4
	qhandle_t	dlcViewBloodBlend;
	sfxHandle_t	dlcGibSplatSound;
	sfxHandle_t	dlcGibImpactSounds[3];
	qhandle_t	medkitUsageModel;
	qhandle_t	dustPuffShader;
	qhandle_t	heartShader;
	qhandle_t	invulnerabilityPowerupModel;
#endif

	// scoreboard headers
	qhandle_t	scoreboardName;
	qhandle_t	scoreboardPing;
	qhandle_t	scoreboardScore;
	qhandle_t	scoreboardTime;

	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;

	// sounds
	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	sfxHandle_t	sfx_railg;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;
#ifdef MISSIONPACK
	sfxHandle_t	sfx_proxexp;
	sfxHandle_t	sfx_nghit;
	sfxHandle_t	sfx_nghitflesh;
	sfxHandle_t	sfx_nghitmetal;
	sfxHandle_t	sfx_chghit;
	sfxHandle_t	sfx_chghitflesh;
	sfxHandle_t	sfx_chghitmetal;
	sfxHandle_t kamikazeExplodeSound;
	sfxHandle_t kamikazeImplodeSound;
	sfxHandle_t kamikazeFarSound;
	sfxHandle_t useInvulnerabilitySound;
	sfxHandle_t invulnerabilityImpactSound1;
	sfxHandle_t invulnerabilityImpactSound2;
	sfxHandle_t invulnerabilityImpactSound3;
	sfxHandle_t invulnerabilityJuicedSound;
	sfxHandle_t obeliskHitSound1;
	sfxHandle_t obeliskHitSound2;
	sfxHandle_t obeliskHitSound3;
	sfxHandle_t	obeliskRespawnSound;
	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;
#endif
	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	lowAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t jumpPadSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
	sfxHandle_t hitSounds[4];
#ifdef MISSIONPACK
	sfxHandle_t hitSoundHighArmor;
	sfxHandle_t hitSoundLowArmor;
#endif
	sfxHandle_t hitTeamSound;
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
	sfxHandle_t assistSound;
	sfxHandle_t defendSound;
#ifdef MISSIONPACK
	sfxHandle_t firstImpressiveSound;
	sfxHandle_t firstExcellentSound;
	sfxHandle_t firstHumiliationSound;
#endif
	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;
#ifdef MISSIONPACK
	sfxHandle_t voteNow;
	sfxHandle_t votePassed;
	sfxHandle_t voteFailed;
#endif
	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t flightSound;
	sfxHandle_t medkitSound;

#ifdef MISSIONPACK
	sfxHandle_t weaponHoverSound;
#endif

	// teamplay sounds
	sfxHandle_t captureAwardSound;
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;
	sfxHandle_t	takenYourTeamSound;
	sfxHandle_t	takenOpponentSound;

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
#ifdef MISSIONPACK
	sfxHandle_t neutralFlagReturnedSound;
#endif
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t	enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t	youHaveFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;
	sfxHandle_t holyShitSound;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

#ifdef MISSIONPACK
	// new stuff
	qhandle_t patrolShader;
	qhandle_t assaultShader;
	qhandle_t campShader;
	qhandle_t followShader;
	qhandle_t defendShader;
	qhandle_t teamLeaderShader;
	qhandle_t retrieveShader;
	qhandle_t escortShader;
	qhandle_t flagShaders[3];
	sfxHandle_t	countPrepareTeamSound;

	sfxHandle_t ammoregenSound;
	sfxHandle_t doublerSound;
	sfxHandle_t guardSound;
	sfxHandle_t scoutSound;
#endif
	qhandle_t cursor;

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
	sfxHandle_t	n_healthSound;
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;

} cgMedia_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	
	float			cursorScaleR;		// clamped 1/screenXScale

	float			screenXBias;
	float			screenYBias;

	float			screenXmin;
	float			screenXmax;

	float			screenYmin;
	float			screenYmax;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t		gametype;
	int				dmflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];
	char			redTeam[MAX_QPATH];
	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;
	int overtimeSeconds; /* g_overtime from serverinfo */

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

#ifdef MISSIONPACK
	int eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// orders
	int currentOrder;
	qboolean orderPending;
	int orderTime;
	int currentVoiceClient;
	int acceptOrderTime;
	int acceptTask;
	int acceptLeader;
	char acceptVoice[MAX_NAME_LENGTH];
#endif

	// media
	cgMedia_t		media;
	cgRaceMedia_t raceMedia;
	cgPoiMedia_t poiMedia;

	float			fov;		// clamped cg_fov value
	float			zoomFov;	// clamped cg_zoomFov value

	pmoveSettings_t pmoveSettings;
	qboolean noPlayerClipPresentation; // CS_SETTINGS_MASK bit 25, separate from movement settings
	unsigned settingsMask; // CS_SETTINGS_MASK (retail 0x10a3ff28); 0 when malformed
	int itemHeight, weaponRespawn;
	int startingHealth; // serverinfo g_startingHealth (retail 0x10a601e0)
	int timeoutCount;   // serverinfo g_timeoutCount (retail 0x10a3ff60)
	// CS_APPEARANCE_SETTINGS (retail 0x10048e40): head scale/offset and model scale
	float playerHeadScale, playerHeadScaleOffset, playerModelScale;
	char playerModelOverride[64], playerHeadModelOverride[64];	// g_playermodelOverride, g_playerheadmodelOverride
	int allowCustomHeadmodels;		// g_allowCustomHeadmodels
	int teamSize; // serverinfo teamsize (retail 0x10a3ff18)
	qboolean armorTiered;

	qboolean		synchronousClients;

	int				ospEnc;
	qboolean		defrag;

	qboolean		filterKeyUpEvent;
	qboolean		score_catched;
	int				score_key;

	float			cursorX;
	float			cursorY;
} cgs_t;

//==============================================================================

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

#define EXTERN_CG_CVAR
	#include "cg_cvar.h"
#undef EXTERN_CG_CVAR

extern const char		*eventnames[EV_MAX];


//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... );
void QDECL CG_Error( const char *msg, ... );

void CG_StartMusic( void );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_LoadMenus(const char *menuFile);
void CG_KeyEvent( int key, qboolean down );
void CG_MouseEvent( int x, int y );
void CG_MouseMoveTo( int x, int y );
void CG_EventHandling( cgame_event_t type );
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
#ifdef MISSIONPACK
score_t *CG_GetSelectedScore( void );
#endif
void CG_BuildSpectatorString( void );

void CG_SetScoreCatcher( qboolean enable );

//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
int CG_CalcFov( void );
void CG_UpdateZoomState( void );
void CG_AddBufferedSound( sfxHandle_t sfx);

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );


//
// cg_drawtools.c
//
// Horizontal placement of the 4:3 frame on wide screens (retail 0x10aa6938).
enum { CG_PLACE_DEFAULT, CG_PLACE_LEFT, CG_PLACE_CENTER, CG_PLACE_RIGHT };
int CG_SetScreenPlacement( int placement );	// returns the previous mode
float CG_ScreenXBias( void );
void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_FillScreen( const float *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );

void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );

int CG_DrawStrlen( const char *str );

float *CG_FadeColor( int startMsec, int totalMsec );
float *CG_FadeColorTime( int startMsec, int totalMsec, int fadeMsec );

const float *CG_TeamColor( team_t team );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);

#define USE_NEW_FONT_RENDERER

// flags for CG_DrawString
enum {
	DS_SHADOW      = 0x1,
	DS_FORCE_COLOR = 0x2,
	DS_PROPORTIONAL = 0x4,
	DS_CENTER = 0x8,	// alignment
	DS_RIGHT  = 0x10	// alignment
};
void CG_DrawString( float x, float y, const char *s, const vec4_t color, float charWidth, float charHeight, int maxChars, int flags );
#ifdef USE_NEW_FONT_RENDERER
void CG_LoadFonts( void );
void CG_SelectFont( int index );
#endif

//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
void CG_Text_PaintClipped(float *maxX, float x, float y, float scale, vec4_t color, const char *text);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height(const char *text, float scale, int limit);
int CG_Text_WidthInFont(const char *text, int font, float scale, int limit);
int CG_Text_HeightInFont(const char *text, int font, float scale, int limit);
void CG_SelectPrevPlayer( void );
void CG_SelectNextPlayer( void );
int CG_StatusAmmo(void);
qboolean CG_DrawStatusHud(int owner, float x, float y, float w, float h,
    int align, float scale, vec4_t color, qhandle_t shader, int style);
void CG_InitMatchClocks(void);
void CG_MatchClockConfigChanged(int index);
void CG_AdvanceMatchClocks(void);
void CG_MatchClockWarnings(void);
void CG_RegisterLocalSounds(void);
void CG_QLCheckLocalSounds(const playerState_t *ps, const playerState_t *ops);
void CG_ClearMatchNotice(void);
qboolean CG_MatchNoticeCommand(const char *command);
qboolean CG_DrawMatchNotice(void);
void CG_CenterPrintNative(const char *text, int y, float scale);
void CG_DrawMatchPause(void);
void CG_AnnounceMatchStart(void);
void CG_InitPregame(void);
void CG_PregameRestart(void);
void CG_PregameConfigChanged(int index);
void CG_DrawPregame(void);
void CG_InitStandings(void);
void CG_StandingsConfigChanged(int index);
qboolean CG_StandingsVisible(int flags);
qboolean CG_DrawStandings(int owner,float x,float y,float w,float h,int align,float scale,vec4_t color,int style);
const char *CG_StandingsName(int slot);
void CG_DrawArmorTierColorized(float x, float y, float w, float h, const vec4_t color, qhandle_t shader);
void CG_DrawHealthColorized(float x, float y, float w, float h, const vec4_t color, qhandle_t shader);
void CG_DrawHeldKeys(float x, float y, float w, float h);
qboolean CG_SpectatorHudActive(void);
void CG_ClearDuelSlots(void);
qboolean CG_DrawTeamStat(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style);
const char *CG_TeamHeldTime(int seconds);
int CG_TeamAveragePing(int team);
const char *CG_TeamPlayerCountText(int team);
int CG_TeamTimeoutsLeft(int team);
void CG_DrawTeamPickupStrip(int team, float x, float y, float w, float h, float scale, vec4_t color, int style);
void CG_DuelScoresParsed(const score_t *rows, int count);
const score_t *CG_DuelSlot(int slot);
const char *CG_DuelName(int slot, float scale);
qboolean CG_DrawDuelStat(int ownerDraw, float x, float y, float w, float h, int align, float scale, vec4_t color, int style);
qboolean CG_DrawEndVote(int ownerDraw, float x, float y, float w, float h, int align, float scale, vec4_t color, int style);
const char *CG_VoteChoice(const char *key, int choice);
const char *CG_VoteTimerText(void);
void CG_DrawSpectatorHud(void);
void CG_DrawFollowPlayerName(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style);
qboolean CG_DrawEndSummary(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style);
const char *CG_MatchStateText(void);
const char *CG_GametypeTag(int gametype);
const char *CG_MatchEndCondition(void);
const char *CG_EndGameScoreText(void);
const char *CG_BestWeaponName(void);
const char *CG_PlayerCountText(void);
int CG_AwardClient(int ownerDraw);
qboolean CG_DrawAwardPicture(int ownerDraw, float x, float y, float w, float h);
const char *CG_MatchWinnerText(float scale);
sfxHandle_t CG_QLVoice(const char *file);
void CG_ClearMatchAnnouncements(void);
void trap_S_StartLocalSoundVolume(sfxHandle_t sfx, int channelNum, float volume);
qboolean CG_DrawMatchClock(int owner, float x, float y, int align, float scale, vec4_t color, int style);
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_ShowResponseHead( void );
void CG_SetPrintString(int type, const char *p);
void CG_InitTeamChat( void );
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText( void );
const char *CG_GetKillerText( void );
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles );
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_CheckOrderPending( void );
const char *CG_GameTypeString( void );
qboolean CG_YourTeamHasFlag( void );
qboolean CG_OtherTeamHasFlag( void );
qhandle_t CG_StatusHandle(int task);

void CG_ForceModelChange( void );
void CG_TrackClientTeamChange( void );

//
// cg_player.c
//
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team );
void CG_AddWeaponWithPowerups(refEntity_t *gun, const entityState_t *state);
void CG_UpdatePlayerPresentationSettings(void);
void CG_CheckAmmo(void);
void CG_DrawAmmoWarning(void);
void CG_PlayerSprites(centity_t *cent);
void CG_RunLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animation, float speedScale);
void CG_ClearLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animation);
void CG_PlayerAnimation(centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
                        int *torsoOld, int *torso, float *torsoBackLerp);
void CG_NewClientInfo( int clientNum );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask );
void CG_PredictPlayerState( void );
qboolean CG_IsConnectionInterrupted( void );
void CG_LoadDeferredPlayers( void );

void CG_PlayDroppedEvents( playerState_t *ps, playerState_t *ops );

//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char *CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position, int entityNum );
void CG_PainEvent( centity_t *cent, int health );


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( const centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( const centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, const vec3_t angles_in, vec3_t angles_out );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, const char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, const char *tagName );



//
// cg_weapons.c
//
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );
void CG_SelectPickedUpWeapon(int weapon);
void CG_SelectSpawnWeapon(void);
void CG_UpdateWeaponSelection(void);

void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent );
void CG_MissileHitWall( weapon_t weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum );
void CG_ShotgunFire( entityState_t *es );
void CG_Bullet( vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum );

void CG_RailTrail( const clientInfo_t *ci, const vec3_t start, const vec3_t end );
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team );
void CG_DrawWeaponSelect( void );

void CG_OutOfAmmoChange( void );	// should this be in pmove?

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader, 
				    const vec3_t origin, const vec3_t dir, 
					float orientation, 
				    float r, float g, float b, float a, 
					qboolean alphaFade, 
					float radius, qboolean temporary );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff( const vec3_t p, 
				   const vec3_t vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_BubbleTrail( const vec3_t start, const vec3_t end, float spacing );
refEntity_t *CG_SpawnEffect( const vec3_t origin, qboolean firstPerson );

#ifdef MISSIONPACK
void CG_KamikazeEffect( vec3_t org );
void CG_ObeliskExplode( vec3_t org, int entityNum );
void CG_ObeliskPain( vec3_t org );
void CG_InvulnerabilityImpact( vec3_t org, vec3_t angles );
void CG_InvulnerabilityJuiced( vec3_t org );
void CG_LightningBoltBeam( vec3_t start, vec3_t end );
#endif
void CG_ScorePlum( int client, const vec3_t origin, int score );

void CG_GibPlayer( const vec3_t playerOrigin );
void CG_ThawPlayer(const vec3_t origin);
void CG_ElectroGibs(const vec3_t origin, qboolean event);
sfxHandle_t CG_ElectroGibSound(void);
void CG_ElectroFragmentBounce(localEntity_t *le, const trace_t *trace);
void CG_ElectroFragmentMark(localEntity_t *le, const trace_t *trace);
void CG_AddGibFlash(localEntity_t *le);
void CG_ThawTrail(const localEntity_t *fragment);
void CG_IceFragmentMark(const trace_t *trace);
void CG_MissileHitWallDmgThrough(int weapon, vec3_t origin, vec3_t direction);
void CG_BigExplode( vec3_t playerOrigin );

void CG_Bleed( const vec3_t origin, int entityNum );

localEntity_t *CG_MakeExplosion( const vec3_t origin, const vec3_t dir,
								qhandle_t hModel, qhandle_t shader, int msec,
								qboolean isSprite );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );
void CG_ScoreboardClick( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );
#ifdef MISSIONPACK
void Menu_Reset( void );
#endif

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_ParseSysteminfo( void );
void CG_ParsePmoveSettings(void);
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);
#ifdef MISSIONPACK
void CG_LoadVoiceChats( void );
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );
#endif

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );


//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *text );

// abort the game
void		trap_Error( const char *text );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void trap_Cvar_RegisterRange(vmCvar_t *vmCvar, const char *name, const char *value,
	const char *minimum, const char *maximum, int flags);
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );
int			trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );

int			trap_RealTime(qtime_t *qtime);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );
void		trap_RemoveCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void		trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points, 
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void	trap_S_StopBackgroundTrack( void );


void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void		trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h, 
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean	trap_R_inPVS( const vec3_t p1, const vec3_t p2 );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );	

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale );

// aids for VM testing
void		testPrintInt( char *string, int i );
void		testPrintFloat( char *string, float f );

int			trap_MemoryRemaining( void );
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );
void trap_Key_KeynumToStringBuf(int keynum, char *buffer, int size);


typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t;


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

void		trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion( const char *animStr, const vec3_t origin, const vec3_t vel, int duration, int sizeStart, int sizeEnd );
extern qboolean		initparticles;
int CG_NewParticleArea ( int num );

// extension interface
extern  qboolean intShaderTime;
extern  qboolean linearLight;
extern  qboolean can_trap_Cvar_SetDescription;

#ifdef Q3_VM
extern void (*trap_R_AddRefEntityToScene2)( const refEntity_t *re );
extern void	(*trap_R_AddLinearLightToScene)( const vec3_t start, const vec3_t end, float intensity, float r, float g, float b );
extern void	(*trap_Cvar_SetDescription)( const char *var_name, const char *var_description );
#else
qboolean trap_GetValue( char *value, int valueSize, const char *key );
void trap_R_AddRefEntityToScene2( const refEntity_t *re );
void trap_R_AddLinearLightToScene( const vec3_t start, const vec3_t end, float intensity, float r, float g, float b );
void trap_Cvar_SetDescription( const char *var_name, const char *var_description );
extern int dll_com_trapGetValue;
extern int dll_trap_R_AddRefEntityToScene2;
extern int dll_trap_R_AddLinearLightToScene;
extern int dll_trap_Cvar_SetDescription;
#endif

void trap_R_DrawText(int x, int y, const char *text, int font, float size, int limit, int forceColor);
void trap_R_DrawTextClipped(int x, int y, const char *text, int font, float size, float *maxX);
qboolean trap_R_ProjectPoint(const vec3_t point, vec4_t window);
qboolean trap_R_ProjectScreenFraction(const vec3_t point, float *across, float *up);
void trap_R_MeasureTextBounds(const char *text, int font, float size, int limit, float bounds[5]);
void trap_R_MeasureText(const char *text, int font, float size, int limit, float *width, float *height);
void CG_SetTextFont(int font);
