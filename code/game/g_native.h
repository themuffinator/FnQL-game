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
// g_native.h -- retail Quake Live native qagame ABI.
//
// Retail Quake Live hosts qagamex86.dll through a structured entry point:
//
//     void dllEntry( void **exports, const void *imports, int *apiVersion );
//
// The engine passes a table of function pointers (the imports below, slot
// order fixed by the retail binary) and receives a table of game entry
// points (gameNativeExport_t order).  A module that also exports vmMain is
// treated as a classic Quake III module, so FnQL-game exports dllEntry only.
//
// Evidence: the slot numbers and signatures mirror FnQL's engine-side binding
// table (FnQL code/game/g_public.h gameNativeImport_t and the QL_BIND list in
// code/server/sv_game.cpp), which hosts the retail module.  Slots 187-191 and
// 193-197 are unproven retail gaps and stay unbound here as well.
//
// Only slots 0..206 exist in the retail engine.  FnQL appends a compatibility
// slab after them (see G_NATIVE_COMPAT_BASE); FnQL-game touches that slab only
// after the engine has identified itself through the "//trap_GetValue" cvar,
// so the same binary stays a drop-in module for the retail engine.
//

#ifndef G_NATIVE_H
#define G_NATIVE_H

#define	GAME_NATIVE_API_VERSION		10
#define	GAME_NATIVE_IMPORT_SLOTS	207

// FnQL compatibility slab: legacy Quake III syscall numbers offset by the
// retail slab size.  Valid only on engines that report "//trap_GetValue".
#define	G_NATIVE_COMPAT_BASE		GAME_NATIVE_IMPORT_SLOTS
#define	G_NATIVE_COM_TRAP_GETVALUE	700

typedef enum {
	GAME_NATIVE_EXPORT_SHUTDOWN,
	GAME_NATIVE_EXPORT_RUN_FRAME,
	GAME_NATIVE_EXPORT_REGISTER_CVARS,
	GAME_NATIVE_EXPORT_INIT,
	GAME_NATIVE_EXPORT_CONSOLE_COMMAND,
	GAME_NATIVE_EXPORT_CLIENT_USERINFO_CHANGED,
	GAME_NATIVE_EXPORT_CLIENT_THINK,
	GAME_NATIVE_EXPORT_CLIENT_DISCONNECT,
	GAME_NATIVE_EXPORT_CLIENT_CONNECT,
	GAME_NATIVE_EXPORT_CLIENT_COMMAND,
	GAME_NATIVE_EXPORT_CLIENT_BEGIN,
	GAME_NATIVE_EXPORT_BOTAI_START_FRAME,
	GAME_NATIVE_EXPORT_CAN_CLIENT_SEE_CLIENT,
	GAME_NATIVE_EXPORT_FREEZE_CAN_SEE_THAW_PROGRESS_EVENT,
	GAME_NATIVE_EXPORT_IS_OBJECTIVE_ENTITY,
	GAME_NATIVE_EXPORT_SHOULD_SUPPRESS_VOICE_TO_CLIENT,
	GAME_NATIVE_EXPORT_IS_CLIENT_ADMIN,
	GAME_NATIVE_EXPORT_ARE_ENEMY_CLIENTS,
	GAME_NATIVE_EXPORT_GET_CLIENT_SCORE,
	GAME_NATIVE_EXPORT_COUNT
} gameNativeExport_t;

// Opaque engine types that the game never dereferences.
typedef struct gameNativeCvar_s gameNativeCvar_t;

#define G_NATIVE_SLOT( type, name, args )	type ( QDECL *name ) args;
#define G_NATIVE_GAP( first, last )			void *unbound_##first##_##last[ (last) - (first) + 1 ];

typedef struct {
	G_NATIVE_SLOT( void, SendConsoleCommand, ( const char *text ) )										// 0
	G_NATIVE_SLOT( int, RealTime, ( qtime_t *qtime ) )													// 1
	G_NATIVE_SLOT( void, Print, ( const char *text ) )													// 2
	G_NATIVE_SLOT( int, Milliseconds, ( void ) )														// 3
	G_NATIVE_SLOT( void, FS_Write, ( const void *buffer, int len, fileHandle_t f ) )					// 4
	G_NATIVE_SLOT( int, FS_Seek, ( fileHandle_t f, long offset, int origin ) )							// 5
	G_NATIVE_SLOT( void, FS_Read, ( void *buffer, int len, fileHandle_t f ) )							// 6
	G_NATIVE_SLOT( int, FS_GetFileList, ( const char *path, const char *extension, char *listbuf, int bufsize ) )	// 7
	G_NATIVE_SLOT( int, FS_FOpenFile, ( const char *qpath, fileHandle_t *f, fsMode_t mode ) )			// 8
	G_NATIVE_SLOT( void, FS_FCloseFile, ( fileHandle_t f ) )											// 9
	G_NATIVE_SLOT( void, Error, ( const char *text ) )													// 10
	G_NATIVE_SLOT( float, Cvar_VariableValue, ( const char *var_name ) )								// 11
	G_NATIVE_SLOT( void, Cvar_Update, ( vmCvar_t *cvar ) )												// 12
	G_NATIVE_SLOT( void, Cvar_VariableStringBuffer, ( const char *var_name, char *buffer, int bufsize ) )	// 13
	G_NATIVE_SLOT( gameNativeCvar_t *, Cvar_SetValue, ( const char *var_name, float value ) )			// 14
	G_NATIVE_SLOT( void, Cvar_Set, ( const char *var_name, const char *value ) )						// 15
	G_NATIVE_SLOT( gameNativeCvar_t *, Cvar_RegisterBounded, ( vmCvar_t *cvar, const char *var_name, const char *value, const char *minValue, const char *maxValue, int flags ) )	// 16
	G_NATIVE_SLOT( void, Cvar_Register, ( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) )	// 17
	G_NATIVE_SLOT( void, Argv, ( int n, char *buffer, int bufferLength ) )								// 18
	G_NATIVE_SLOT( void, Args, ( char *buffer, int bufferLength ) )										// 19
	G_NATIVE_SLOT( int, Argc, ( void ) )																// 20
	G_NATIVE_SLOT( void, Cmd_TokenizeString, ( const char *text ) )										// 21
	G_NATIVE_SLOT( void, LocateGameData, ( void *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient ) )	// 22
	G_NATIVE_SLOT( void, DropClient, ( int clientNum, const char *reason ) )							// 23
	G_NATIVE_SLOT( void, SendServerCommand, ( int clientNum, const char *text ) )						// 24
	G_NATIVE_SLOT( void, SetConfigstring, ( int num, const char *string ) )								// 25
	G_NATIVE_SLOT( void, GetConfigstring, ( int num, char *buffer, int bufferSize ) )					// 26
	G_NATIVE_SLOT( void, ResendConfigstring, ( int num ) )												// 27
	G_NATIVE_SLOT( void, GetUserinfo, ( int num, char *buffer, int bufferSize ) )						// 28
	G_NATIVE_SLOT( void, SetUserinfo, ( int num, const char *buffer ) )									// 29
	G_NATIVE_SLOT( void, GetServerinfo, ( char *buffer, int bufferSize ) )								// 30
	G_NATIVE_SLOT( void, SetBrushModel, ( void *ent, const char *name ) )								// 31
	G_NATIVE_SLOT( void, Trace, ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) )	// 32
	G_NATIVE_SLOT( void, TraceCapsule, ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) )	// 33
	G_NATIVE_SLOT( int, PointContents, ( const vec3_t point, int passEntityNum ) )						// 34
	G_NATIVE_SLOT( qboolean, InPVS, ( const vec3_t p1, const vec3_t p2 ) )								// 35
	G_NATIVE_SLOT( qboolean, InPVSIgnorePortals, ( const vec3_t p1, const vec3_t p2 ) )					// 36
	G_NATIVE_SLOT( void, AdjustAreaPortalState, ( void *ent, qboolean open ) )							// 37
	G_NATIVE_SLOT( qboolean, AreasConnected, ( int area1, int area2 ) )									// 38
	G_NATIVE_SLOT( void, LinkEntity, ( void *ent ) )													// 39
	G_NATIVE_SLOT( void, UnlinkEntity, ( void *ent ) )													// 40
	G_NATIVE_SLOT( int, EntitiesInBox, ( const vec3_t mins, const vec3_t maxs, int *list, int maxcount ) )	// 41
	G_NATIVE_SLOT( qboolean, EntityContact, ( const vec3_t mins, const vec3_t maxs, const void *ent ) )	// 42
	G_NATIVE_SLOT( int, BotAllocateClient, ( void ) )													// 43
	G_NATIVE_SLOT( void, BotFreeClient, ( int clientNum ) )												// 44
	G_NATIVE_SLOT( void, GetUsercmd, ( int clientNum, usercmd_t *cmd ) )								// 45
	G_NATIVE_SLOT( qboolean, GetEntityToken, ( char *buffer, int bufferSize ) )							// 46
	G_NATIVE_SLOT( int, DebugPolygonCreate, ( int color, int numPoints, vec3_t *points ) )				// 47
	G_NATIVE_SLOT( void, DebugPolygonDelete, ( int id ) )												// 48
	G_NATIVE_SLOT( int, BotLibSetup, ( void ) )															// 49
	G_NATIVE_SLOT( int, BotLibShutdown, ( void ) )														// 50
	G_NATIVE_SLOT( int, BotLibVarSet, ( const char *var_name, const char *value ) )						// 51
	G_NATIVE_SLOT( int, BotLibVarGet, ( const char *var_name, char *value, int size ) )					// 52
	G_NATIVE_SLOT( int, BotLibDefine, ( const char *string ) )											// 53
	G_NATIVE_SLOT( int, BotLibStartFrame, ( float time ) )												// 54
	G_NATIVE_SLOT( int, BotLibLoadMap, ( const char *mapname ) )										// 55
	G_NATIVE_SLOT( int, BotLibUpdateEntity, ( int ent, void *bue ) )									// 56
	G_NATIVE_SLOT( int, BotLibTest, ( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) )			// 57
	G_NATIVE_SLOT( int, BotGetSnapshotEntity, ( int clientNum, int sequence ) )							// 58
	G_NATIVE_SLOT( int, BotGetServerCommand, ( int clientNum, char *message, int size ) )				// 59
	G_NATIVE_SLOT( void, BotUserCommand, ( int clientNum, usercmd_t *ucmd ) )							// 60
	G_NATIVE_SLOT( int, AAS_BBoxAreas, ( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas ) )	// 61
	G_NATIVE_SLOT( int, AAS_AreaInfo, ( int areanum, void *info ) )										// 62
	G_NATIVE_SLOT( void, AAS_EntityInfo, ( int entnum, void *info ) )									// 63
	G_NATIVE_SLOT( int, AAS_Initialized, ( void ) )														// 64
	G_NATIVE_SLOT( void, AAS_PresenceTypeBoundingBox, ( int presencetype, vec3_t mins, vec3_t maxs ) )	// 65
	G_NATIVE_SLOT( float, AAS_Time, ( void ) )															// 66
	G_NATIVE_SLOT( int, AAS_PointAreaNum, ( vec3_t point ) )											// 67
	G_NATIVE_SLOT( int, AAS_PointReachabilityAreaIndex, ( vec3_t point ) )								// 68
	G_NATIVE_SLOT( int, AAS_TraceAreas, ( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas ) )	// 69
	G_NATIVE_SLOT( int, AAS_PointContents, ( vec3_t point ) )											// 70
	G_NATIVE_SLOT( int, AAS_NextBSPEntity, ( int ent ) )												// 71
	G_NATIVE_SLOT( int, AAS_ValueForBSPEpairKey, ( int ent, const char *key, char *value, int size ) )	// 72
	G_NATIVE_SLOT( int, AAS_VectorForBSPEpairKey, ( int ent, const char *key, vec3_t v ) )				// 73
	G_NATIVE_SLOT( int, AAS_FloatForBSPEpairKey, ( int ent, const char *key, float *value ) )			// 74
	G_NATIVE_SLOT( int, AAS_IntForBSPEpairKey, ( int ent, const char *key, int *value ) )				// 75
	G_NATIVE_SLOT( int, AAS_AreaReachability, ( int areanum ) )											// 76
	G_NATIVE_SLOT( int, AAS_AreaTravelTimeToGoalArea, ( int areanum, vec3_t origin, int goalareanum, int travelflags ) )	// 77
	G_NATIVE_SLOT( int, AAS_EnableRoutingArea, ( int areanum, int enable ) )							// 78
	G_NATIVE_SLOT( int, AAS_PredictRoute, ( void *route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum ) )	// 79
	G_NATIVE_SLOT( int, AAS_AlternativeRouteGoals, ( vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void *altroutegoals, int maxaltroutegoals, int type ) )	// 80
	G_NATIVE_SLOT( int, AAS_Swimming, ( vec3_t origin ) )												// 81
	G_NATIVE_SLOT( int, AAS_PredictClientMovement, ( void *move, int entnum, const vec3_t origin, int presencetype, int onground, const vec3_t velocity, const vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) )	// 82
	G_NATIVE_SLOT( void, BotDrawDebugAreas, ( vec3_t origin, int enable, int areanum ) )				// 83
	G_NATIVE_SLOT( void, BotDrawAvoidSpots, ( int movestate ) )											// 84
	G_NATIVE_SLOT( void, EA_Say, ( int client, const char *str ) )										// 85
	G_NATIVE_SLOT( void, EA_SayTeam, ( int client, const char *str ) )									// 86
	G_NATIVE_SLOT( void, EA_Command, ( int client, const char *command ) )								// 87
	G_NATIVE_SLOT( void, EA_Action, ( int client, int action ) )										// 88
	G_NATIVE_SLOT( void, EA_Walk, ( int client ) )														// 89
	G_NATIVE_SLOT( void, EA_Gesture, ( int client ) )													// 90
	G_NATIVE_SLOT( void, EA_Talk, ( int client ) )														// 91
	G_NATIVE_SLOT( void, EA_Attack, ( int client ) )													// 92
	G_NATIVE_SLOT( void, EA_Use, ( int client ) )														// 93
	G_NATIVE_SLOT( void, EA_Respawn, ( int client ) )													// 94
	G_NATIVE_SLOT( void, EA_Crouch, ( int client ) )													// 95
	G_NATIVE_SLOT( void, EA_MoveUp, ( int client ) )													// 96
	G_NATIVE_SLOT( void, EA_MoveDown, ( int client ) )													// 97
	G_NATIVE_SLOT( void, EA_MoveForward, ( int client ) )												// 98
	G_NATIVE_SLOT( void, EA_MoveBack, ( int client ) )													// 99
	G_NATIVE_SLOT( void, EA_MoveLeft, ( int client ) )													// 100
	G_NATIVE_SLOT( void, EA_MoveRight, ( int client ) )													// 101
	G_NATIVE_SLOT( void, EA_SelectWeapon, ( int client, int weapon ) )									// 102
	G_NATIVE_SLOT( void, EA_Jump, ( int client ) )														// 103
	G_NATIVE_SLOT( void, EA_DelayedJump, ( int client ) )												// 104
	G_NATIVE_SLOT( void, EA_Move, ( int client, vec3_t dir, float speed ) )								// 105
	G_NATIVE_SLOT( void, EA_View, ( int client, vec3_t viewangles ) )									// 106
	G_NATIVE_SLOT( void, EA_EndRegular, ( int client, float thinktime ) )								// 107
	G_NATIVE_SLOT( void, EA_GetInput, ( int client, float thinktime, void *input ) )					// 108
	G_NATIVE_SLOT( void, EA_ResetInput, ( int client ) )												// 109
	G_NATIVE_SLOT( int, BotLoadCharacter, ( char *charfile, float skill ) )								// 110
	G_NATIVE_SLOT( void, BotFreeCharacter, ( int character ) )											// 111
	G_NATIVE_SLOT( float, Characteristic_Float, ( int character, int index ) )							// 112
	G_NATIVE_SLOT( float, Characteristic_BFloat, ( int character, int index, float min, float max ) )	// 113
	G_NATIVE_SLOT( int, Characteristic_Integer, ( int character, int index ) )							// 114
	G_NATIVE_SLOT( int, Characteristic_BInteger, ( int character, int index, int min, int max ) )		// 115
	G_NATIVE_SLOT( void, Characteristic_String, ( int character, int index, char *buf, int size ) )		// 116
	G_NATIVE_SLOT( int, BotAllocChatState, ( void ) )													// 117
	G_NATIVE_SLOT( void, BotFreeChatState, ( int handle ) )												// 118
	G_NATIVE_SLOT( void, BotQueueConsoleMessage, ( int chatstate, int type, char *message ) )			// 119
	G_NATIVE_SLOT( void, BotRemoveConsoleMessage, ( int chatstate, int handle ) )						// 120
	G_NATIVE_SLOT( int, BotNextConsoleMessage, ( int chatstate, void *cm ) )							// 121
	G_NATIVE_SLOT( int, BotNumConsoleMessages, ( int chatstate ) )										// 122
	G_NATIVE_SLOT( void, BotInitialChat, ( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) )	// 123
	G_NATIVE_SLOT( int, BotNumInitialChats, ( int chatstate, char *type ) )								// 124
	G_NATIVE_SLOT( int, BotReplyChat, ( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) )	// 125
	G_NATIVE_SLOT( int, BotChatLength, ( int chatstate ) )												// 126
	G_NATIVE_SLOT( void, BotEnterChat, ( int chatstate, int client, int sendto ) )						// 127
	G_NATIVE_SLOT( void, BotGetChatMessage, ( int chatstate, char *buf, int size ) )					// 128
	G_NATIVE_SLOT( int, StringContains, ( char *str1, char *str2, int casesensitive ) )					// 129
	G_NATIVE_SLOT( int, BotFindMatch, ( char *str, void *match, unsigned long context ) )				// 130
	G_NATIVE_SLOT( void, BotMatchVariable, ( void *match, int variable, char *buf, int size ) )			// 131
	G_NATIVE_SLOT( void, UnifyWhiteSpaces, ( char *string ) )											// 132
	G_NATIVE_SLOT( void, BotReplaceSynonyms, ( char *string, unsigned long context ) )					// 133
	G_NATIVE_SLOT( int, BotLoadChatFile, ( int chatstate, char *chatfile, char *chatname ) )			// 134
	G_NATIVE_SLOT( void, BotSetChatGender, ( int chatstate, int gender ) )								// 135
	G_NATIVE_SLOT( void, BotSetChatName, ( int chatstate, char *name, int client ) )					// 136
	G_NATIVE_SLOT( void, BotResetGoalState, ( int goalstate ) )											// 137
	G_NATIVE_SLOT( void, BotRemoveFromAvoidGoals, ( int goalstate, int number ) )						// 138
	G_NATIVE_SLOT( void, BotResetAvoidGoals, ( int goalstate ) )										// 139
	G_NATIVE_SLOT( void, BotPushGoal, ( int goalstate, void *goal ) )									// 140
	G_NATIVE_SLOT( void, BotPopGoal, ( int goalstate ) )												// 141
	G_NATIVE_SLOT( void, BotEmptyGoalStack, ( int goalstate ) )											// 142
	G_NATIVE_SLOT( void, BotDumpAvoidGoals, ( int goalstate ) )											// 143
	G_NATIVE_SLOT( void, BotDumpGoalStack, ( int goalstate ) )											// 144
	G_NATIVE_SLOT( void, BotGoalName, ( int number, char *name, int size ) )							// 145
	G_NATIVE_SLOT( int, BotGetTopGoal, ( int goalstate, void *goal ) )									// 146
	G_NATIVE_SLOT( int, BotGetSecondGoal, ( int goalstate, void *goal ) )								// 147
	G_NATIVE_SLOT( int, BotChooseLTGItem, ( int goalstate, vec3_t origin, int *inventory, int travelflags ) )	// 148
	G_NATIVE_SLOT( int, BotChooseNBGItem, ( int goalstate, vec3_t origin, int *inventory, int travelflags, void *ltg, float maxtime ) )	// 149
	G_NATIVE_SLOT( int, BotTouchingGoal, ( vec3_t origin, void *goal ) )								// 150
	G_NATIVE_SLOT( int, BotItemGoalInVisButNotVisible, ( int viewer, vec3_t eye, vec3_t viewangles, void *goal ) )	// 151
	G_NATIVE_SLOT( int, BotGetNextCampSpotGoal, ( int num, void *goal ) )								// 152
	G_NATIVE_SLOT( int, BotGetMapLocationGoal, ( char *name, void *goal ) )								// 153
	G_NATIVE_SLOT( int, BotGetLevelItemGoal, ( int index, char *classname, void *goal ) )				// 154
	G_NATIVE_SLOT( float, BotAvoidGoalTime, ( int goalstate, int number ) )								// 155
	G_NATIVE_SLOT( void, BotSetAvoidGoalTime, ( int goalstate, int number, float avoidtime ) )			// 156
	G_NATIVE_SLOT( void, BotInitLevelItems, ( void ) )													// 157
	G_NATIVE_SLOT( void, BotUpdateEntityItems, ( void ) )												// 158
	G_NATIVE_SLOT( int, BotLoadItemWeights, ( int goalstate, char *filename ) )							// 159
	G_NATIVE_SLOT( void, BotFreeItemWeights, ( int goalstate ) )										// 160
	G_NATIVE_SLOT( void, BotInterbreedGoalFuzzyLogic, ( int parent1, int parent2, int child ) )			// 161
	G_NATIVE_SLOT( void, BotSaveGoalFuzzyLogic, ( int goalstate, char *filename ) )						// 162
	G_NATIVE_SLOT( void, BotMutateGoalFuzzyLogic, ( int goalstate, float range ) )						// 163
	G_NATIVE_SLOT( int, BotAllocGoalState, ( int state ) )												// 164
	G_NATIVE_SLOT( void, BotFreeGoalState, ( int handle ) )												// 165
	G_NATIVE_SLOT( void, BotResetMoveState, ( int movestate ) )											// 166
	G_NATIVE_SLOT( void, BotMoveToGoal, ( void *result, int movestate, void *goal, int travelflags ) )	// 167
	G_NATIVE_SLOT( int, BotMoveInDirection, ( int movestate, vec3_t dir, float speed, int type ) )		// 168
	G_NATIVE_SLOT( void, BotResetAvoidReach, ( int movestate ) )										// 169
	G_NATIVE_SLOT( void, BotResetLastAvoidReach, ( int movestate ) )									// 170
	G_NATIVE_SLOT( int, BotReachabilityArea, ( vec3_t origin, int testground ) )						// 171
	G_NATIVE_SLOT( int, BotMovementViewTarget, ( int movestate, void *goal, int travelflags, float lookahead, vec3_t target ) )	// 172
	// Retail qagame pushes all five arguments here (QLSRP HLIL 0x1001ae1d..0x1001ae2b).
	G_NATIVE_SLOT( int, BotPredictVisiblePosition, ( vec3_t origin, int areanum, void *goal, int travelflags, vec3_t target ) )	// 173
	G_NATIVE_SLOT( int, BotAllocMoveState, ( void ) )													// 174
	G_NATIVE_SLOT( void, BotFreeMoveState, ( int handle ) )												// 175
	G_NATIVE_SLOT( void, BotInitMoveState, ( int handle, void *initmove ) )								// 176
	G_NATIVE_SLOT( void, BotAddAvoidSpot, ( int movestate, vec3_t origin, float radius, int type ) )	// 177
	G_NATIVE_SLOT( int, BotChooseBestFightWeapon, ( int weaponstate, int *inventory ) )					// 178
	G_NATIVE_SLOT( void, BotGetWeaponInfo, ( int weaponstate, int weapon, void *weaponinfo ) )			// 179
	G_NATIVE_SLOT( int, BotLoadWeaponWeights, ( int weaponstate, char *filename ) )						// 180
	G_NATIVE_SLOT( int, BotAllocWeaponState, ( void ) )													// 181
	G_NATIVE_SLOT( void, BotFreeWeaponState, ( int weaponstate ) )										// 182
	G_NATIVE_SLOT( void, BotResetWeaponState, ( int weaponstate ) )										// 183
	G_NATIVE_SLOT( int, GeneticParentsAndChildSelection, ( int numranks, float *ranks, int *parent1, int *parent2, int *child ) )	// 184
	G_NATIVE_SLOT( void, SubmitMatchReport, ( void *report ) )											// 185
	G_NATIVE_SLOT( void, ReportPlayerEvent, ( unsigned int steamIdLow, unsigned int steamIdHigh, const void *clientStats, const char *eventName, void *payload ) )	// 186
	G_NATIVE_GAP( 187, 191 )
	G_NATIVE_SLOT( void *, MarkClientGotCP, ( int clientNum ) )											// 192
	G_NATIVE_GAP( 193, 197 )
	G_NATIVE_SLOT( int, ReturnZero198, ( void ) )														// 198
	G_NATIVE_SLOT( int, ReturnZero199, ( void ) )														// 199
	// Retail returns the whole 64-bit SteamID in EDX:EAX (retail ClientConnect stores both halves).
	G_NATIVE_SLOT( unsigned long long, GetSteamId64, ( int clientNum ) )								// 200
	G_NATIVE_SLOT( void, AddSteamStat, ( int clientNum, int statIndex, int delta ) )					// 201
	G_NATIVE_SLOT( void, GenerateMatchGuid, ( char *buffer, int bufferSize ) )							// 202
	G_NATIVE_SLOT( void, UnlockSteamAchievement, ( int clientNum, int achievementId ) )					// 203
	G_NATIVE_SLOT( qboolean, HasSteamAchievement, ( int clientNum, int achievementId ) )				// 204
	G_NATIVE_SLOT( qboolean, BeginSteamAuthSession, ( int clientNum ) )									// 205
	G_NATIVE_SLOT( qboolean, FactoryExists, ( const char *factoryName ) )								// 206
} gameNativeImports_t;

#undef G_NATIVE_SLOT
#undef G_NATIVE_GAP

// Every slot is one pointer wide, so a slot's byte offset is its index times
// the pointer size.  The checks below pin the table to the retail layout.
#define G_NATIVE_SLOT_OFFSET( slot )	( (slot) * sizeof( void * ) )
typedef char g_native_slot0_check[ ( offsetof( gameNativeImports_t, SendConsoleCommand ) == G_NATIVE_SLOT_OFFSET( 0 ) ) ? 1 : -1 ];
typedef char g_native_slot22_check[ ( offsetof( gameNativeImports_t, LocateGameData ) == G_NATIVE_SLOT_OFFSET( 22 ) ) ? 1 : -1 ];
typedef char g_native_slot49_check[ ( offsetof( gameNativeImports_t, BotLibSetup ) == G_NATIVE_SLOT_OFFSET( 49 ) ) ? 1 : -1 ];
typedef char g_native_slot110_check[ ( offsetof( gameNativeImports_t, BotLoadCharacter ) == G_NATIVE_SLOT_OFFSET( 110 ) ) ? 1 : -1 ];
typedef char g_native_slot184_check[ ( offsetof( gameNativeImports_t, GeneticParentsAndChildSelection ) == G_NATIVE_SLOT_OFFSET( 184 ) ) ? 1 : -1 ];
typedef char g_native_slot192_check[ ( offsetof( gameNativeImports_t, MarkClientGotCP ) == G_NATIVE_SLOT_OFFSET( 192 ) ) ? 1 : -1 ];
typedef char g_native_slot200_check[ ( offsetof( gameNativeImports_t, GetSteamId64 ) == G_NATIVE_SLOT_OFFSET( 200 ) ) ? 1 : -1 ];
typedef char g_native_size_check[ ( sizeof( gameNativeImports_t ) == G_NATIVE_SLOT_OFFSET( GAME_NATIVE_IMPORT_SLOTS ) ) ? 1 : -1 ];

// FnQL engine extension query, available once G_NativeInitExtensions found
// an FnQL-family engine.  Returns qfalse on the retail engine.
qboolean G_NativeGetValue( char *value, int valueSize, const char *key );
qboolean G_NativeHasEngineExtensions( void );

#endif // G_NATIVE_H
