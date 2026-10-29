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
// g_native.c -- retail Quake Live native ABI for qagame.
//
// The engine enters through dllEntry with the retail import table and takes
// back the export table below.  Every export funnels into vmMain, so the
// per-call bookkeeping in g_main.c (deferred exit-rule checks) runs exactly as
// it does for classic dispatch.  The trap_* wrappers keep the game code's
// calling surface unchanged while each call becomes a typed table call.
//

#include "g_local.h"
#include "g_native.h"
#include "fnql_module.h"
#include "../../version/fnql_game_version.h"

static const gameNativeImports_t *gi;

typedef qboolean ( QDECL *gameNativeGetValue_t )( char *value, int valueSize, const char *key );
static gameNativeGetValue_t g_nativeGetValue;
static qboolean g_nativeExtensionsProbed;

/*
================
G_NativeInitExtensions

The retail engine has no "//trap_GetValue" cvar.  FnQL publishes it with the
Quake3e syscall number and backs it with the compatibility slab that follows
the retail import slots, so only then is the slab addressed.
================
*/
static void G_NativeInitExtensions( void ) {
	char value[ MAX_CVAR_VALUE_STRING ];
	void * const *slots;

	if ( g_nativeExtensionsProbed ) {
		return;
	}
	g_nativeExtensionsProbed = qtrue;
	g_nativeGetValue = NULL;

	gi->Cvar_VariableStringBuffer( "//trap_GetValue", value, sizeof( value ) );
	if ( atoi( value ) != G_NATIVE_COM_TRAP_GETVALUE ) {
		return;
	}

	slots = (void * const *)gi;
	g_nativeGetValue = (gameNativeGetValue_t)slots[ G_NATIVE_COMPAT_BASE + G_NATIVE_COM_TRAP_GETVALUE ];
}

qboolean G_NativeHasEngineExtensions( void ) {
	return g_nativeGetValue ? qtrue : qfalse;
}

qboolean G_NativeGetValue( char *value, int valueSize, const char *key ) {
	if ( !g_nativeGetValue || !value || valueSize <= 0 || !key ) {
		return qfalse;
	}
	return g_nativeGetValue( value, valueSize, key ) ? qtrue : qfalse;
}

/*
================
Export thunks

Signatures follow the retail export slots the engine calls through.
================
*/
static void QDECL G_NativeShutdown( qboolean restart ) {
	vmMain( GAME_SHUTDOWN, restart, 0, 0 );
}

static void QDECL G_NativeRunFrame( int levelTime ) {
	vmMain( GAME_RUN_FRAME, levelTime, 0, 0 );
}

static void QDECL G_NativeRegisterCvars( void ) {
	G_NativeInitExtensions();
	G_RegisterCvars();
}

static void QDECL G_NativeInit( int levelTime, int randomSeed, qboolean restart ) {
	G_NativeInitExtensions();
	vmMain( GAME_INIT, levelTime, randomSeed, restart );
}

static qboolean QDECL G_NativeConsoleCommand( void ) {
	return vmMain( GAME_CONSOLE_COMMAND, 0, 0, 0 ) ? qtrue : qfalse;
}

static void QDECL G_NativeClientUserinfoChanged( int clientNum ) {
	vmMain( GAME_CLIENT_USERINFO_CHANGED, clientNum, 0, 0 );
}

static void QDECL G_NativeClientThink( int clientNum ) {
	vmMain( GAME_CLIENT_THINK, clientNum, 0, 0 );
}

static void QDECL G_NativeClientDisconnect( int clientNum ) {
	vmMain( GAME_CLIENT_DISCONNECT, clientNum, 0, 0 );
}

static const char * QDECL G_NativeClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	return (const char *)vmMain( GAME_CLIENT_CONNECT, clientNum, firstTime, isBot );
}

static void QDECL G_NativeClientCommand( int clientNum ) {
	vmMain( GAME_CLIENT_COMMAND, clientNum, 0, 0 );
}

static void QDECL G_NativeClientBegin( int clientNum ) {
	vmMain( GAME_CLIENT_BEGIN, clientNum, 0, 0 );
}

static int QDECL G_NativeBotAIStartFrame( int time ) {
	return (int)vmMain( BOTAI_START_FRAME, time, 0, 0 );
}

/*
================
Quake Live engine queries

Retail qagame answers these for snapshot culling, Freeze Tag thaw events,
objective markers, voice routing, admin checks, team relations and score
lookups.  Until the Quake Live rules land (docs/PLAN.md milestones 4-6) the
answers are the neutral Quake III ones: everything visible, no objectives,
no voice suppression, no admins.
================
*/
static gclient_t *G_NativeClient( int clientNum ) {
	if ( clientNum < 0 || clientNum >= level.maxclients || !level.clients ) {
		return NULL;
	}
	return &level.clients[ clientNum ];
}

static qboolean QDECL G_NativeCanClientSeeClient( int viewerClientNum, int targetClientNum ) {
	(void)viewerClientNum;
	(void)targetClientNum;
	return qtrue;
}

static qboolean QDECL G_NativeFreezeCanSeeThawProgressEvent( int clientNum, int entNum ) {
	(void)clientNum;
	(void)entNum;
	return qtrue;
}

static qboolean QDECL G_NativeIsObjectiveEntity( int entNum ) {
	(void)entNum;
	return qfalse;
}

static qboolean QDECL G_NativeShouldSuppressVoiceToClient( int senderClientNum, int recipientClientNum ) {
	(void)senderClientNum;
	(void)recipientClientNum;
	return qfalse;
}

static qboolean QDECL G_NativeIsClientAdmin( int clientNum ) {
	(void)clientNum;
	return qfalse;
}

static qboolean QDECL G_NativeAreEnemyClients( int clientNumA, int clientNumB ) {
	gclient_t *a = G_NativeClient( clientNumA );
	gclient_t *b = G_NativeClient( clientNumB );

	if ( !a || !b || clientNumA == clientNumB ) {
		return qfalse;
	}
	if ( a->pers.connected != CON_CONNECTED || b->pers.connected != CON_CONNECTED ) {
		return qfalse;
	}
	if ( a->sess.sessionTeam == TEAM_SPECTATOR || b->sess.sessionTeam == TEAM_SPECTATOR ) {
		return qfalse;
	}
	return OnSameTeam( &g_entities[ clientNumA ], &g_entities[ clientNumB ] ) ? qfalse : qtrue;
}

static int QDECL G_NativeGetClientScore( int clientNum ) {
	gclient_t *client = G_NativeClient( clientNum );

	return client ? client->ps.persistant[ PERS_SCORE ] : 0;
}

static void *g_nativeExports[ GAME_NATIVE_EXPORT_COUNT ];

/*
================
dllEntry

Publishes the export table in retail slot order and records the engine's
import table.  No engine call is made here: the retail engine only promises
a usable table once it starts calling exports.
================
*/
DLLEXPORT void QDECL dllEntry( void **exports, const void *imports, int *apiVersion ) {
	gi = (const gameNativeImports_t *)imports;
	g_nativeExtensionsProbed = qfalse;
	g_nativeGetValue = NULL;

	g_nativeExports[ GAME_NATIVE_EXPORT_SHUTDOWN ] = (void *)G_NativeShutdown;
	g_nativeExports[ GAME_NATIVE_EXPORT_RUN_FRAME ] = (void *)G_NativeRunFrame;
	g_nativeExports[ GAME_NATIVE_EXPORT_REGISTER_CVARS ] = (void *)G_NativeRegisterCvars;
	g_nativeExports[ GAME_NATIVE_EXPORT_INIT ] = (void *)G_NativeInit;
	g_nativeExports[ GAME_NATIVE_EXPORT_CONSOLE_COMMAND ] = (void *)G_NativeConsoleCommand;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_USERINFO_CHANGED ] = (void *)G_NativeClientUserinfoChanged;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_THINK ] = (void *)G_NativeClientThink;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_DISCONNECT ] = (void *)G_NativeClientDisconnect;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_CONNECT ] = (void *)G_NativeClientConnect;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_COMMAND ] = (void *)G_NativeClientCommand;
	g_nativeExports[ GAME_NATIVE_EXPORT_CLIENT_BEGIN ] = (void *)G_NativeClientBegin;
	g_nativeExports[ GAME_NATIVE_EXPORT_BOTAI_START_FRAME ] = (void *)G_NativeBotAIStartFrame;
	g_nativeExports[ GAME_NATIVE_EXPORT_CAN_CLIENT_SEE_CLIENT ] = (void *)G_NativeCanClientSeeClient;
	g_nativeExports[ GAME_NATIVE_EXPORT_FREEZE_CAN_SEE_THAW_PROGRESS_EVENT ] = (void *)G_NativeFreezeCanSeeThawProgressEvent;
	g_nativeExports[ GAME_NATIVE_EXPORT_IS_OBJECTIVE_ENTITY ] = (void *)G_NativeIsObjectiveEntity;
	g_nativeExports[ GAME_NATIVE_EXPORT_SHOULD_SUPPRESS_VOICE_TO_CLIENT ] = (void *)G_NativeShouldSuppressVoiceToClient;
	g_nativeExports[ GAME_NATIVE_EXPORT_IS_CLIENT_ADMIN ] = (void *)G_NativeIsClientAdmin;
	g_nativeExports[ GAME_NATIVE_EXPORT_ARE_ENEMY_CLIENTS ] = (void *)G_NativeAreEnemyClients;
	g_nativeExports[ GAME_NATIVE_EXPORT_GET_CLIENT_SCORE ] = (void *)G_NativeGetClientScore;

	if ( exports ) {
		*exports = g_nativeExports;
	}
	if ( apiVersion ) {
		*apiVersion = GAME_NATIVE_API_VERSION;
	}
}

/*
================
FnQL_GetModuleInfo
================
*/
static const fnqlModuleInfo_t g_moduleInfo = {
	sizeof( fnqlModuleInfo_t ),
	FNQL_MODULE_INFO_VERSION,
	FNQL_MODULE_PRODUCT,
	"qagame",
	FNQL_GAME_VERSION_STRING,
	__DATE__ " " __TIME__,
	0u,	// no retail wire parity claimed yet; see docs/PLAN.md milestone 2+
	GAME_NATIVE_API_VERSION
};

DLLEXPORT const fnqlModuleInfo_t * QDECL FnQL_GetModuleInfo( void ) {
	return &g_moduleInfo;
}

/*
================
Traps without a one-to-one retail slot
================
*/

// The retail table has a single append-only console command slot.
void trap_SendConsoleCommand( int exec_when, const char *text ) {
	(void)exec_when;
	gi->SendConsoleCommand( text );
}

// Retail qagame reads numeric cvars through the float slot.
int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return (int)gi->Cvar_VariableValue( var_name );
}

void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
		playerState_t *gameClients, int sizeofGameClient ) {
	gi->LocateGameData( gEnts, numGEntities, sizeofGEntity_t, gameClients, sizeofGameClient );
}

// No engine slot exists; round to nearest like the engine helper it replaces.
// Prediction parity for this rounding is a milestone 3 gate.
void trap_SnapVector( float *v ) {
	v[0] = rintf( v[0] );
	v[1] = rintf( v[1] );
	v[2] = rintf( v[2] );
}

// Quake Live online-service imports (slots 200, 203 and 204); the engine
// decides whether a Steam provider backs them.
unsigned long long trap_GetSteamId64( int clientNum ) {
	return gi->GetSteamId64( clientNum );
}

qboolean trap_HasSteamAchievement( int clientNum, int achievementId ) {
	return gi->HasSteamAchievement( clientNum, achievementId );
}

void trap_UnlockSteamAchievement( int clientNum, int achievementId ) {
	gi->UnlockSteamAchievement( clientNum, achievementId );
}

qboolean trap_GetValue( char *value, int valueSize, const char *key ) {
	return G_NativeGetValue( value, valueSize, key );
}

/*
================
Direct slot wrappers
================
*/
void trap_Print( const char *text ) {
	gi->Print( text );
}

void trap_Error( const char *text ) {
	gi->Error( text );
}

int trap_Milliseconds( void ) {
	return gi->Milliseconds();
}

int trap_Argc( void ) {
	return gi->Argc();
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	gi->Argv( n, buffer, bufferLength );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return gi->FS_FOpenFile( qpath, f, mode );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	gi->FS_Read( buffer, len, f );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	gi->FS_Write( buffer, len, f );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	gi->FS_FCloseFile( f );
}

int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return gi->FS_GetFileList( path, extension, listbuf, bufsize );
}

int trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin ) {
	return gi->FS_Seek( f, offset, origin );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	gi->Cvar_Register( cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	gi->Cvar_Update( cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	gi->Cvar_Set( var_name, value );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	gi->Cvar_VariableStringBuffer( var_name, buffer, bufsize );
}

void trap_DropClient( int clientNum, const char *reason ) {
	gi->DropClient( clientNum, reason );
}

void trap_SendServerCommand( int clientNum, const char *text ) {
	gi->SendServerCommand( clientNum, text );
}

void trap_SetConfigstring( int num, const char *string ) {
	gi->SetConfigstring( num, string );
}

void trap_GetConfigstring( int num, char *buffer, int bufferSize ) {
	gi->GetConfigstring( num, buffer, bufferSize );
}

void trap_GetUserinfo( int num, char *buffer, int bufferSize ) {
	gi->GetUserinfo( num, buffer, bufferSize );
}

void trap_SetUserinfo( int num, const char *buffer ) {
	gi->SetUserinfo( num, buffer );
}

void trap_GetServerinfo( char *buffer, int bufferSize ) {
	gi->GetServerinfo( buffer, bufferSize );
}

void trap_SetBrushModel( gentity_t *ent, const char *name ) {
	gi->SetBrushModel( ent, name );
}

void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	gi->Trace( results, start, mins, maxs, end, passEntityNum, contentmask );
}

void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	gi->TraceCapsule( results, start, mins, maxs, end, passEntityNum, contentmask );
}

int trap_PointContents( const vec3_t point, int passEntityNum ) {
	return gi->PointContents( point, passEntityNum );
}

qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 ) {
	return gi->InPVS( p1, p2 );
}

qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return gi->InPVSIgnorePortals( p1, p2 );
}

void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	gi->AdjustAreaPortalState( ent, open );
}

qboolean trap_AreasConnected( int area1, int area2 ) {
	return gi->AreasConnected( area1, area2 );
}

void trap_LinkEntity( gentity_t *ent ) {
	gi->LinkEntity( ent );
}

void trap_UnlinkEntity( gentity_t *ent ) {
	gi->UnlinkEntity( ent );
}

int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount ) {
	return gi->EntitiesInBox( mins, maxs, list, maxcount );
}

qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return gi->EntityContact( mins, maxs, ent );
}

int trap_BotAllocateClient( void ) {
	return gi->BotAllocateClient();
}

void trap_BotFreeClient( int clientNum ) {
	gi->BotFreeClient( clientNum );
}

void trap_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	gi->GetUsercmd( clientNum, cmd );
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return gi->GetEntityToken( buffer, bufferSize );
}

int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	return gi->DebugPolygonCreate( color, numPoints, points );
}

void trap_DebugPolygonDelete( int id ) {
	gi->DebugPolygonDelete( id );
}

int trap_RealTime( qtime_t *qtime ) {
	return gi->RealTime( qtime );
}

int trap_BotLibSetup( void ) {
	return gi->BotLibSetup();
}

int trap_BotLibShutdown( void ) {
	return gi->BotLibShutdown();
}

int trap_BotLibVarSet( const char *var_name, const char *value ) {
	return gi->BotLibVarSet( var_name, value );
}

int trap_BotLibVarGet( const char *var_name, char *value, int size ) {
	return gi->BotLibVarGet( var_name, value, size );
}

int trap_BotLibDefine( char *string ) {
	return gi->BotLibDefine( string );
}

int trap_BotLibStartFrame( float time ) {
	return gi->BotLibStartFrame( time );
}

int trap_BotLibLoadMap( const char *mapname ) {
	return gi->BotLibLoadMap( mapname );
}

int trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue ) {
	return gi->BotLibUpdateEntity( ent, bue );
}

int trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) {
	return gi->BotLibTest( parm0, parm1, parm2, parm3 );
}

int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return gi->BotGetSnapshotEntity( clientNum, sequence );
}

int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return gi->BotGetServerCommand( clientNum, message, size );
}

void trap_BotUserCommand( int clientNum, usercmd_t *ucmd ) {
	gi->BotUserCommand( clientNum, ucmd );
}

void trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info ) {
	gi->AAS_EntityInfo( entnum, info );
}

int trap_AAS_Initialized( void ) {
	return gi->AAS_Initialized();
}

void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	gi->AAS_PresenceTypeBoundingBox( presencetype, mins, maxs );
}

float trap_AAS_Time( void ) {
	return gi->AAS_Time();
}

int trap_AAS_PointAreaNum( vec3_t point ) {
	return gi->AAS_PointAreaNum( point );
}

int trap_AAS_PointReachabilityAreaIndex( vec3_t point ) {
	return gi->AAS_PointReachabilityAreaIndex( point );
}

int trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas ) {
	return gi->AAS_TraceAreas( start, end, areas, points, maxareas );
}

int trap_AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas ) {
	return gi->AAS_BBoxAreas( absmins, absmaxs, areas, maxareas );
}

int trap_AAS_AreaInfo( int areanum, void /* struct aas_areainfo_s */ *info ) {
	return gi->AAS_AreaInfo( areanum, info );
}

int trap_AAS_PointContents( vec3_t point ) {
	return gi->AAS_PointContents( point );
}

int trap_AAS_NextBSPEntity( int ent ) {
	return gi->AAS_NextBSPEntity( ent );
}

int trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size ) {
	return gi->AAS_ValueForBSPEpairKey( ent, key, value, size );
}

int trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v ) {
	return gi->AAS_VectorForBSPEpairKey( ent, key, v );
}

int trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value ) {
	return gi->AAS_FloatForBSPEpairKey( ent, key, value );
}

int trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value ) {
	return gi->AAS_IntForBSPEpairKey( ent, key, value );
}

int trap_AAS_AreaReachability( int areanum ) {
	return gi->AAS_AreaReachability( areanum );
}

int trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	return gi->AAS_AreaTravelTimeToGoalArea( areanum, origin, goalareanum, travelflags );
}

int trap_AAS_EnableRoutingArea( int areanum, int enable ) {
	return gi->AAS_EnableRoutingArea( areanum, enable );
}

int trap_AAS_PredictRoute( void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum ) {
	return gi->AAS_PredictRoute( route, areanum, origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum );
}

int trap_AAS_AlternativeRouteGoals( vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals, int type ) {
	return gi->AAS_AlternativeRouteGoals( start, startareanum, goal, goalareanum, travelflags, altroutegoals, maxaltroutegoals, type );
}

int trap_AAS_Swimming( vec3_t origin ) {
	return gi->AAS_Swimming( origin );
}

int trap_AAS_PredictClientMovement( void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return gi->AAS_PredictClientMovement( move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
}

void trap_EA_Say( int client, char *str ) {
	gi->EA_Say( client, str );
}

void trap_EA_SayTeam( int client, char *str ) {
	gi->EA_SayTeam( client, str );
}

void trap_EA_Command( int client, char *command ) {
	gi->EA_Command( client, command );
}

void trap_EA_Action( int client, int action ) {
	gi->EA_Action( client, action );
}

void trap_EA_Gesture( int client ) {
	gi->EA_Gesture( client );
}

void trap_EA_Talk( int client ) {
	gi->EA_Talk( client );
}

void trap_EA_Attack( int client ) {
	gi->EA_Attack( client );
}

void trap_EA_Use( int client ) {
	gi->EA_Use( client );
}

void trap_EA_Respawn( int client ) {
	gi->EA_Respawn( client );
}

void trap_EA_Crouch( int client ) {
	gi->EA_Crouch( client );
}

void trap_EA_MoveUp( int client ) {
	gi->EA_MoveUp( client );
}

void trap_EA_MoveDown( int client ) {
	gi->EA_MoveDown( client );
}

void trap_EA_MoveForward( int client ) {
	gi->EA_MoveForward( client );
}

void trap_EA_MoveBack( int client ) {
	gi->EA_MoveBack( client );
}

void trap_EA_MoveLeft( int client ) {
	gi->EA_MoveLeft( client );
}

void trap_EA_MoveRight( int client ) {
	gi->EA_MoveRight( client );
}

void trap_EA_SelectWeapon( int client, int weapon ) {
	gi->EA_SelectWeapon( client, weapon );
}

void trap_EA_Jump( int client ) {
	gi->EA_Jump( client );
}

void trap_EA_DelayedJump( int client ) {
	gi->EA_DelayedJump( client );
}

void trap_EA_Move( int client, vec3_t dir, float speed ) {
	gi->EA_Move( client, dir, speed );
}

void trap_EA_View( int client, vec3_t viewangles ) {
	gi->EA_View( client, viewangles );
}

void trap_EA_EndRegular( int client, float thinktime ) {
	gi->EA_EndRegular( client, thinktime );
}

void trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input ) {
	gi->EA_GetInput( client, thinktime, input );
}

void trap_EA_ResetInput( int client ) {
	gi->EA_ResetInput( client );
}

int trap_BotLoadCharacter( char *charfile, float skill ) {
	return gi->BotLoadCharacter( charfile, skill );
}

void trap_BotFreeCharacter( int character ) {
	gi->BotFreeCharacter( character );
}

float trap_Characteristic_Float( int character, int index ) {
	return gi->Characteristic_Float( character, index );
}

float trap_Characteristic_BFloat( int character, int index, float min, float max ) {
	return gi->Characteristic_BFloat( character, index, min, max );
}

int trap_Characteristic_Integer( int character, int index ) {
	return gi->Characteristic_Integer( character, index );
}

int trap_Characteristic_BInteger( int character, int index, int min, int max ) {
	return gi->Characteristic_BInteger( character, index, min, max );
}

void trap_Characteristic_String( int character, int index, char *buf, int size ) {
	gi->Characteristic_String( character, index, buf, size );
}

int trap_BotAllocChatState( void ) {
	return gi->BotAllocChatState();
}

void trap_BotFreeChatState( int handle ) {
	gi->BotFreeChatState( handle );
}

void trap_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	gi->BotQueueConsoleMessage( chatstate, type, message );
}

void trap_BotRemoveConsoleMessage( int chatstate, int handle ) {
	gi->BotRemoveConsoleMessage( chatstate, handle );
}

int trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm ) {
	return gi->BotNextConsoleMessage( chatstate, cm );
}

int trap_BotNumConsoleMessages( int chatstate ) {
	return gi->BotNumConsoleMessages( chatstate );
}

void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	gi->BotInitialChat( chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotNumInitialChats( int chatstate, char *type ) {
	return gi->BotNumInitialChats( chatstate, type );
}

int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return gi->BotReplyChat( chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotChatLength( int chatstate ) {
	return gi->BotChatLength( chatstate );
}

void trap_BotEnterChat( int chatstate, int client, int sendto ) {
	gi->BotEnterChat( chatstate, client, sendto );
}

void trap_BotGetChatMessage( int chatstate, char *buf, int size ) {
	gi->BotGetChatMessage( chatstate, buf, size );
}

int trap_StringContains( char *str1, char *str2, int casesensitive ) {
	return gi->StringContains( str1, str2, casesensitive );
}

int trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, unsigned long int context ) {
	return gi->BotFindMatch( str, match, context );
}

void trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size ) {
	gi->BotMatchVariable( match, variable, buf, size );
}

void trap_UnifyWhiteSpaces( char *string ) {
	gi->UnifyWhiteSpaces( string );
}

void trap_BotReplaceSynonyms( char *string, unsigned long int context ) {
	gi->BotReplaceSynonyms( string, context );
}

int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return gi->BotLoadChatFile( chatstate, chatfile, chatname );
}

void trap_BotSetChatGender( int chatstate, int gender ) {
	gi->BotSetChatGender( chatstate, gender );
}

void trap_BotSetChatName( int chatstate, char *name, int client ) {
	gi->BotSetChatName( chatstate, name, client );
}

void trap_BotResetGoalState( int goalstate ) {
	gi->BotResetGoalState( goalstate );
}

void trap_BotResetAvoidGoals( int goalstate ) {
	gi->BotResetAvoidGoals( goalstate );
}

void trap_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	gi->BotRemoveFromAvoidGoals( goalstate, number );
}

void trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	gi->BotPushGoal( goalstate, goal );
}

void trap_BotPopGoal( int goalstate ) {
	gi->BotPopGoal( goalstate );
}

void trap_BotEmptyGoalStack( int goalstate ) {
	gi->BotEmptyGoalStack( goalstate );
}

void trap_BotDumpAvoidGoals( int goalstate ) {
	gi->BotDumpAvoidGoals( goalstate );
}

void trap_BotDumpGoalStack( int goalstate ) {
	gi->BotDumpGoalStack( goalstate );
}

void trap_BotGoalName( int number, char *name, int size ) {
	gi->BotGoalName( number, name, size );
}

int trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return gi->BotGetTopGoal( goalstate, goal );
}

int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return gi->BotGetSecondGoal( goalstate, goal );
}

int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return gi->BotChooseLTGItem( goalstate, origin, inventory, travelflags );
}

int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime ) {
	return gi->BotChooseNBGItem( goalstate, origin, inventory, travelflags, ltg, maxtime );
}

int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal ) {
	return gi->BotTouchingGoal( origin, goal );
}

int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal ) {
	return gi->BotItemGoalInVisButNotVisible( viewer, eye, viewangles, goal );
}

int trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal ) {
	return gi->BotGetLevelItemGoal( index, classname, goal );
}

int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal ) {
	return gi->BotGetNextCampSpotGoal( num, goal );
}

int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal ) {
	return gi->BotGetMapLocationGoal( name, goal );
}

float trap_BotAvoidGoalTime( int goalstate, int number ) {
	return gi->BotAvoidGoalTime( goalstate, number );
}

void trap_BotSetAvoidGoalTime( int goalstate, int number, float avoidtime ) {
	gi->BotSetAvoidGoalTime( goalstate, number, avoidtime );
}

void trap_BotInitLevelItems( void ) {
	gi->BotInitLevelItems();
}

void trap_BotUpdateEntityItems( void ) {
	gi->BotUpdateEntityItems();
}

int trap_BotLoadItemWeights( int goalstate, char *filename ) {
	return gi->BotLoadItemWeights( goalstate, filename );
}

void trap_BotFreeItemWeights( int goalstate ) {
	gi->BotFreeItemWeights( goalstate );
}

void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	gi->BotInterbreedGoalFuzzyLogic( parent1, parent2, child );
}

void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	gi->BotSaveGoalFuzzyLogic( goalstate, filename );
}

void trap_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	gi->BotMutateGoalFuzzyLogic( goalstate, range );
}

int trap_BotAllocGoalState( int state ) {
	return gi->BotAllocGoalState( state );
}

void trap_BotFreeGoalState( int handle ) {
	gi->BotFreeGoalState( handle );
}

void trap_BotResetMoveState( int movestate ) {
	gi->BotResetMoveState( movestate );
}

void trap_BotAddAvoidSpot( int movestate, vec3_t origin, float radius, int type ) {
	gi->BotAddAvoidSpot( movestate, origin, radius, type );
}

void trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags ) {
	gi->BotMoveToGoal( result, movestate, goal, travelflags );
}

int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type ) {
	return gi->BotMoveInDirection( movestate, dir, speed, type );
}

void trap_BotResetAvoidReach( int movestate ) {
	gi->BotResetAvoidReach( movestate );
}

void trap_BotResetLastAvoidReach( int movestate ) {
	gi->BotResetLastAvoidReach( movestate );
}

int trap_BotReachabilityArea( vec3_t origin, int testground ) {
	return gi->BotReachabilityArea( origin, testground );
}

int trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target ) {
	return gi->BotMovementViewTarget( movestate, goal, travelflags, lookahead, target );
}

int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target ) {
	return gi->BotPredictVisiblePosition( origin, areanum, goal, travelflags, target );
}

int trap_BotAllocMoveState( void ) {
	return gi->BotAllocMoveState();
}

void trap_BotFreeMoveState( int handle ) {
	gi->BotFreeMoveState( handle );
}

void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove ) {
	gi->BotInitMoveState( handle, initmove );
}

int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return gi->BotChooseBestFightWeapon( weaponstate, inventory );
}

void trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo ) {
	gi->BotGetWeaponInfo( weaponstate, weapon, weaponinfo );
}

int trap_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return gi->BotLoadWeaponWeights( weaponstate, filename );
}

int trap_BotAllocWeaponState( void ) {
	return gi->BotAllocWeaponState();
}

void trap_BotFreeWeaponState( int weaponstate ) {
	gi->BotFreeWeaponState( weaponstate );
}

void trap_BotResetWeaponState( int weaponstate ) {
	gi->BotResetWeaponState( weaponstate );
}

int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return gi->GeneticParentsAndChildSelection( numranks, ranks, parent1, parent2, child );
}

