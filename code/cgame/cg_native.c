/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "cg_local.h"
#include "cg_native.h"
#include "../game/fnql_module.h"
#include "../../version/fnql_game_version.h"

static const cgameNativeImports_t *cgi;

void trap_Print(const char *text) {
	cgi->trap_Print(text );
}

void trap_Error(const char *text) {
	cgi->trap_Error(text );
}

int trap_Milliseconds(void) {
	return cgi->trap_Milliseconds();
}

int trap_RealTime(qtime_t *qtime) {
	return cgi->trap_RealTime(qtime );
}

void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags) {
	cgi->trap_Cvar_Register(vmCvar, varName, defaultValue, flags );
}

void trap_Cvar_Update(vmCvar_t *vmCvar) {
	cgi->trap_Cvar_Update(vmCvar );
}

void trap_Cvar_RegisterRange(vmCvar_t *vmCvar, const char *name, const char *value,
		const char *minimum, const char *maximum, int flags) {
	if (cgi->Cvar_RegisterRange) {
		cgi->Cvar_RegisterRange(vmCvar, name, value, minimum, maximum, flags);
	} else {
		cgi->trap_Cvar_Register(vmCvar, name, value, flags);
	}
	// FnQL's range registration can clamp after filling the mirror.
	if (vmCvar) cgi->trap_Cvar_Update(vmCvar);
}

void trap_Cvar_Set(const char *var_name, const char *value) {
	cgi->trap_Cvar_Set(var_name, value );
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	cgi->trap_Cvar_VariableStringBuffer(var_name, buffer, bufsize );
}

int trap_Argc(void) {
	return cgi->trap_Argc();
}

void trap_Argv(int n, char *buffer, int bufferLength) {
	cgi->trap_Argv(n, buffer, bufferLength );
}

void trap_Args(char *buffer, int bufferLength) {
	cgi->trap_Args(buffer, bufferLength );
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
	return cgi->trap_FS_FOpenFile(qpath, f, mode );
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f) {
	cgi->trap_FS_Read(buffer, len, f );
}

void trap_FS_Write(const void *buffer, int len, fileHandle_t f) {
	cgi->trap_FS_Write(buffer, len, f );
}

void trap_FS_FCloseFile(fileHandle_t f) {
	cgi->trap_FS_FCloseFile(f );
}

int trap_FS_Seek(fileHandle_t f, long offset, fsOrigin_t origin) {
	return cgi->trap_FS_Seek(f, offset, origin );
}

void trap_SendConsoleCommand(const char *text) {
	cgi->trap_SendConsoleCommand(text );
}

void trap_AddCommand(const char *cmdName) {
	cgi->trap_AddCommand(cmdName );
}

void trap_RemoveCommand(const char *cmdName) {
	cgi->trap_RemoveCommand(cmdName );
}

void trap_SendClientCommand(const char *s) {
	cgi->trap_SendClientCommand(s );
}

void trap_UpdateScreen(void) {
	cgi->trap_UpdateScreen();
}

void trap_CM_LoadMap(const char *mapname) {
	cgi->trap_CM_LoadMap(mapname );
}

int trap_CM_NumInlineModels(void) {
	return cgi->trap_CM_NumInlineModels();
}

clipHandle_t trap_CM_InlineModel(int index) {
	return cgi->trap_CM_InlineModel(index );
}

clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs) {
	return cgi->trap_CM_TempBoxModel(mins, maxs );
}

clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins, const vec3_t maxs) {
	return cgi->trap_CM_TempCapsuleModel(mins, maxs );
}

int trap_CM_PointContents(const vec3_t p, clipHandle_t model) {
	return cgi->trap_CM_PointContents(p, model );
}

int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles) {
	return cgi->trap_CM_TransformedPointContents(p, model, origin, angles );
}

void trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask) {
	cgi->trap_CM_BoxTrace(results, start, end, mins, maxs, model, brushmask );
}

void trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask) {
	cgi->trap_CM_CapsuleTrace(results, start, end, mins, maxs, model, brushmask );
}

void trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles) {
	cgi->trap_CM_TransformedBoxTrace(results, start, end, mins, maxs, model, brushmask, origin, angles );
}

void trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles) {
	cgi->trap_CM_TransformedCapsuleTrace(results, start, end, mins, maxs, model, brushmask, origin, angles );
}

int trap_CM_MarkFragments(int numPoints, const vec3_t *points, 
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer) {
	return cgi->trap_CM_MarkFragments(numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void trap_S_StartSound(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx) {
	cgi->trap_S_StartSound(origin, entityNum, entchannel, sfx );
}

void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum) {
	cgi->trap_S_StartLocalSound(sfx, channelNum );
}

void trap_S_StartLocalSoundVolume(sfxHandle_t sfx, int channelNum, float volume) {
	if (!(volume >= 0)) volume=0;
	if (volume > 2) volume=2;
	if (cgi->StartLocalSoundVolume) cgi->StartLocalSoundVolume(sfx,channelNum,volume);
	else if (volume > 0) cgi->trap_S_StartLocalSound(sfx,channelNum);
}

void trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx) {
	cgi->trap_S_AddLoopingSound(entityNum, origin, velocity, sfx );
}

void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin) {
	cgi->trap_S_UpdateEntityPosition(entityNum, origin );
}

void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater) {
	cgi->trap_S_Respatialize(entityNum, origin, axis, inwater );
}

sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed) {
	return cgi->trap_S_RegisterSound(sample, compressed );
}

void trap_S_StartBackgroundTrack(const char *intro, const char *loop) {
	cgi->trap_S_StartBackgroundTrack(intro, loop );
}

void trap_S_StopBackgroundTrack(void) {
	cgi->trap_S_StopBackgroundTrack();
}

void trap_R_LoadWorldMap(const char *mapname) {
	cgi->trap_R_LoadWorldMap(mapname );
}

qhandle_t trap_R_RegisterModel(const char *name) {
	return cgi->trap_R_RegisterModel(name );
}

qhandle_t trap_R_RegisterSkin(const char *name) {
	return cgi->trap_R_RegisterSkin(name );
}

qhandle_t trap_R_RegisterShader(const char *name) {
	return cgi->trap_R_RegisterShader(name );
}

qhandle_t trap_R_RegisterShaderNoMip(const char *name) {
	return cgi->trap_R_RegisterShaderNoMip(name );
}

void trap_R_ClearScene(void) {
	cgi->trap_R_ClearScene();
}

void trap_R_AddRefEntityToScene(const refEntity_t *re) {
	cgi->trap_R_AddRefEntityToScene(re );
}

void trap_R_AddPolyToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts) {
	cgi->trap_R_AddPolyToScene(hShader, numVerts, verts );
}

void trap_R_AddPolysToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts, int num) {
	cgi->trap_R_AddPolysToScene(hShader, numVerts, verts, num );
}

void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	cgi->trap_R_AddLightToScene(org, intensity, r, g, b );
}

int trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
	return cgi->trap_R_LightForPoint(point, ambientLight, directedLight, lightDir );
}

void trap_R_RenderScene(const refdef_t *fd) {
	cgi->trap_R_RenderScene(fd );
}

void trap_R_SetColor(const float *rgba) {
	cgi->trap_R_SetColor(rgba );
}

void trap_R_DrawStretchPic(float x, float y, float w, float h, 
							   float s1, float t1, float s2, float t2, qhandle_t hShader) {
	cgi->trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader );
}

void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) {
	cgi->trap_R_ModelBounds(model, mins, maxs );
}

int trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName) {
	return cgi->trap_R_LerpTag(tag, mod, startFrame, endFrame, frac, tagName );
}

void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset) {
	cgi->trap_R_RemapShader(oldShader, newShader, timeOffset );
}

void trap_GetGlconfig(glconfig_t *glconfig) {
	cgi->trap_GetGlconfig(glconfig );
}

void trap_GetGameState(gameState_t *gamestate) {
	cgi->trap_GetGameState(gamestate );
}

void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime) {
	cgi->trap_GetCurrentSnapshotNumber(snapshotNumber, serverTime );
}

qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot) {
	int i;
	if (!snapshot || !cgi->trap_GetSnapshot(snapshotNumber, snapshot)) return qfalse;
	if (snapshot->numEntities < 0 || snapshot->numEntities > MAX_ENTITIES_IN_SNAPSHOT
		|| snapshot->ps.clientNum < 0 || snapshot->ps.clientNum >= MAX_CLIENTS) return qfalse;
	for (i = 0; i < snapshot->numEntities; ++i) {
		if ((unsigned)snapshot->entities[i].number >= MAX_GENTITIES) return qfalse;
	}
	return qtrue;
}

qboolean trap_GetServerCommand(int serverCommandNumber) {
	return cgi->trap_GetServerCommand(serverCommandNumber );
}

int trap_GetCurrentCmdNumber(void) {
	return cgi->trap_GetCurrentCmdNumber();
}

qboolean trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd) {
	return cgi->trap_GetUserCmd(cmdNumber, ucmd );
}

int trap_MemoryRemaining(void) {
	return cgi->trap_MemoryRemaining();
}

void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) {
	cgi->trap_R_RegisterFont(fontName, pointSize, font );
}

qboolean trap_Key_IsDown(int keynum) {
	return cgi->trap_Key_IsDown(keynum );
}

int trap_Key_GetCatcher(void) {
	return cgi->trap_Key_GetCatcher();
}

void trap_Key_SetCatcher(int catcher) {
	cgi->trap_Key_SetCatcher(catcher );
}

int trap_Key_GetKey(const char *binding) {
	return cgi->trap_Key_GetKey(binding );
}

void trap_Key_KeynumToStringBuf(int keynum, char *buffer, int size) {
    if (!buffer || size <= 0) return;
    buffer[0] = '\0';
    if (cgi->KeynumToString) cgi->KeynumToString(keynum,buffer,size);
    buffer[size-1] = '\0';
}

int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits) {
	return cgi->trap_CIN_PlayCinematic(arg0, xpos, ypos, width, height, bits);
}

e_status trap_CIN_StopCinematic(int handle) {
	return cgi->trap_CIN_StopCinematic(handle);
}

e_status trap_CIN_RunCinematic(int handle) {
	return cgi->trap_CIN_RunCinematic(handle);
}

void trap_CIN_DrawCinematic(int handle) {
	cgi->trap_CIN_DrawCinematic(handle);
}

void trap_CIN_SetExtents(int handle, int x, int y, int w, int h) {
	cgi->trap_CIN_SetExtents(handle, x, y, w, h);
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize) {
	return cgi->trap_GetEntityToken(buffer, bufferSize );
}

int trap_PC_AddGlobalDefine(char *define) {
	return cgi->trap_PC_AddGlobalDefine(define );
}

int trap_PC_LoadSource(const char *filename) {
	return cgi->trap_PC_LoadSource(filename );
}

/* Retail 0x10022ffc asks import 19 whether dlc_gibs exists. */
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize) {
	return cgi->trap_FS_GetFileList ? cgi->trap_FS_GetFileList(path, extension, listbuf, bufsize) : 0;
}

int trap_PC_FreeSource(int handle) {
	return cgi->trap_PC_FreeSource(handle );
}

int trap_PC_ReadToken(int handle, pc_token_t *pc_token) {
	return cgi->trap_PC_ReadToken(handle, pc_token );
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line) {
	return cgi->trap_PC_SourceFileAndLine(handle, filename, line );
}


/* Retail has separate frame-clear and kill-all imports (slots 41/42). */
void trap_S_ClearLoopingSounds(qboolean killall) {
    if (killall) cgi->ClearLoopingAll();
    else cgi->ClearLoopingFrame();
}
void trap_SetUserCmdValue(int weapon, float sensitivity) {
	cgi->SetUserCmd(weapon, cg.weaponPrimary > WP_NONE && cg.weaponPrimary < WP_NUM_WEAPONS ?
        cg.weaponPrimary : WP_NONE, sensitivity, cg.userCmdFov);
}
/* Retail looping sounds are submitted each frame. */
void trap_S_AddRealLoopingSound(int entity, const vec3_t origin, const vec3_t velocity, sfxHandle_t sound) {
    trap_S_AddLoopingSound(entity, origin, velocity, sound);
}
void trap_R_AddAdditiveLightToScene(const vec3_t origin, float intensity, float r, float g, float b) {
    trap_R_AddLightToScene(origin, intensity, r, g, b);
}
/* These legacy syscall extensions have no entry in retail's 128-slot table. */
qboolean trap_GetValue(char *value, int size, const char *key) {
    if (value && size > 0) value[0] = '\0';
    (void)key; return qfalse;
}
void trap_R_AddRefEntityToScene2(const refEntity_t *entity) { trap_R_AddRefEntityToScene(entity); }
void trap_R_AddLinearLightToScene(const vec3_t start, const vec3_t end, float intensity, float r, float g, float b) {
    (void)end; trap_R_AddLightToScene(start, intensity, r, g, b);
}
void trap_Cvar_SetDescription(const char *name, const char *description) { (void)name; (void)description; }

extern void CG_Init(int, int, int);
extern void CG_Shutdown(void);
extern void CG_RegisterCvars(void);
static void CG_ChatDown(void) { cg.chatExpanded = qtrue; }
static void CG_ChatUp(void) { cg.chatExpanded = qfalse; }
static int CG_PhysicsTime(void) { return cg.physicsTime; }
/* Retail 0x100209e0..0x10020a32: the command-overlay layout is event mode 5. */
static float CG_ChatY(void) { return cgs.eventHandling == 5 ? 455.0f : 413.0f; }
static float CG_ChatWidth(void) { return cgs.eventHandling == 5 ? 300.0f : 640.0f; }
static int CG_ChatChars(void) { return cgs.eventHandling == 5 ? 30 : 73; }
static int CG_Speaking(int client, int speaking) {
    if (client < 0 || client >= MAX_CLIENTS) return 0;
    cgs.clientinfo[client].speaking = speaking;
    cgs.clientinfo[client].speakingTime = cg.time;
    return 0;
}
/* Retail export slots 10 and 11 (0x10029ff0, 0x1002a060): the engine's
 * new message and new notice. Each arms its own three-second flag timer
 * (CG_SHOW_IF_MSG_PRESENT / _NOTICE_PRESENT) and feeds cg_lastmsg to the
 * HUD chat area (0x10006910) with no extra lifetime. */
static void CG_LastMessageArrived(int *timer) {
    char text[MAX_STRING_CHARS];
    *timer = cg.time + 3000;
    trap_Cvar_VariableStringBuffer("cg_lastmsg", text, sizeof(text));
    CG_ChatAreaAdd(text, 0, 0);
}
static void CG_NewMessage(void) { CG_LastMessageArrived(&cg.messageTime); }
static void CG_NewNotice(void) { CG_LastMessageArrived(&cg.noticeTime); }
static qboolean CG_CopyIdentity(int client, cgameClientIdentity_t *out) {
    const char *info, *value;
    unsigned long long identity = 0;
    if (!out || client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid) return qfalse;
    memset(out, 0, sizeof(*out));
    out->clientNum = client;
    info = CG_ConfigString(CS_PLAYERS + client);
    value = Info_ValueForKey(info, "st");
    while (*value >= '0' && *value <= '9') {
        unsigned digit = (unsigned)(*value++ - '0');
        if (identity > (~0ULL - digit) / 10) { identity = 0; break; }
        identity = identity * 10 + digit;
    }
    if (*value) identity = 0;
    out->identityLow = (unsigned)identity;
    out->identityHigh = (unsigned)(identity >> 32);
    Q_strncpyz(out->displayName, cgs.clientinfo[client].name, sizeof(out->displayName));
    Q_strncpyz(out->cleanName, out->displayName, sizeof(out->cleanName));
    Q_CleanStr(out->cleanName);
    return qtrue;
}
/* Retail scoreboard feeders (0x10026294 and kin) ask import 125 whether
 * the player's Steam identity is muted. A missing import or an invalid
 * client is "not muted". */
qboolean CG_ClientMuted(int client) {
    cgameClientIdentity_t identity;
    if (!cgi || !cgi->IsClientMuted || !CG_CopyIdentity(client, &identity)) return qfalse;
    return cgi->IsClientMuted(identity.identityLow, identity.identityHigh) != 0;
}

/* Retail export slot 8 (0x100208f0, tail call to 0x1003c4f0) receives
 * absolute drawable pixels. With a positive widescreen bias (0x10a3ff04,
 * cgs.screenXBias) x becomes (x - bias) / (vidHeight / 3 * 4 / 640), the
 * divisor rounded to single precision; otherwise x * 640 / vidWidth. y is
 * y * 480 / vidHeight. Both truncate toward zero (_ftol2) and are not
 * clamped. A nonpositive size, which retail would divide by, is rejected. */
qboolean CG_NativeCursorFromPixels(int x, int y, int vidWidth, int vidHeight, float bias,
                                   int *cursorX, int *cursorY) {
    double fx, fy;
    if (vidWidth <= 0 || vidHeight <= 0) return qfalse;
    if (bias > 0) fx = ((double)x - bias) / (double)(float)(vidHeight / 3.0 * 4.0 / 640.0);
    else fx = (double)x * 640.0 / vidWidth;
    fy = (double)y * 480.0 / vidHeight;
    *cursorX = (int)fx;
    *cursorY = (int)fy;
    return qtrue;
}

static void CG_NativeMouseEvent(int x, int y) {
    int cursorX, cursorY;
    if (CG_NativeCursorFromPixels(x, y, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight,
                                  cgs.screenXBias, &cursorX, &cursorY))
        CG_MouseMoveTo(cursorX, cursorY);
}

static void *exports[] = {
    (void *)CG_Init, (void *)CG_RegisterCvars, (void *)CG_Shutdown,
    (void *)CG_ConsoleCommand, (void *)CG_DrawActiveFrame, (void *)CG_CrosshairPlayer,
    (void *)CG_LastAttacker, (void *)CG_KeyEvent, (void *)CG_NativeMouseEvent,
    (void *)CG_EventHandling, (void *)CG_NewMessage, (void *)CG_NewNotice,
    (void *)CG_ChatDown, (void *)CG_ChatUp, (void *)CG_PhysicsTime,
    (void *)CG_CopyIdentity, NULL, (void *)CG_ChatY, (void *)CG_ChatWidth,
    (void *)CG_ChatChars, (void *)CG_Speaking
};
DLLEXPORT void QDECL dllEntry(void **table, const void *imports, int *version) {
    cgi = (const cgameNativeImports_t *)imports;
    if (table) *table = exports;
    if (version) *version = CGAME_NATIVE_API_VERSION;
}
static const fnqlModuleInfo_t moduleInfo = {
    sizeof(fnqlModuleInfo_t), FNQL_MODULE_INFO_VERSION, FNQL_MODULE_PRODUCT,
    "cgame", FNQL_GAME_VERSION_STRING, __DATE__ " " __TIME__,
    0u, CGAME_NATIVE_API_VERSION /* G4/visual parity not yet established. */
};
DLLEXPORT const fnqlModuleInfo_t *QDECL FnQL_GetModuleInfo(void) { return &moduleInfo; }

typedef char native_import_size[(sizeof(cgameNativeImports_t) == 128 * sizeof(void (*)(void))) ? 1 : -1];
typedef char native_snapshot_size[(sizeof(snapshot_t) == 0x16488) ? 1 : -1];
typedef char native_snapshot_entities[(offsetof(snapshot_t, entities) == 0x280) ? 1 : -1];
typedef char native_glconfig_size[(sizeof(glconfig_t) == 0x2c44) ? 1 : -1];

qboolean trap_R_ProjectPoint(const vec3_t point, vec4_t window) {
    vec4_t eye, clip, normalized;
    if (!cgi->TransformModelToClip || !cgi->TransformClipToWindow) return qfalse;
    cgi->TransformModelToClip(point, eye, clip);
    if (!(clip[3] > 0.0f)) return qfalse;
    cgi->TransformClipToWindow(clip, normalized, window);
    return qtrue;
}

/* Retail engine 0x4514c0 also returns the point's screen fractions in
 * window[2] (across) and window[3] (up); derive them from the normalized
 * coordinates so engines that keep the depth term there agree. */
qboolean trap_R_ProjectScreenFraction(const vec3_t point, float *across, float *up) {
    vec4_t eye, clip, normalized, window;
    if (!cgi->TransformModelToClip || !cgi->TransformClipToWindow) return qfalse;
    cgi->TransformModelToClip(point, eye, clip);
    if (!(clip[3] > 0.0f)) return qfalse;
    cgi->TransformClipToWindow(clip, normalized, window);
    *across = 0.5f * (1.0f + normalized[0]);
    *up = 0.5f * (1.0f + normalized[1]);
    return *across == *across && *up == *up;
}

/* Retail imports 123/124: text uses pixel size; width/height return as two
 * IEEE floats packed into EDX:EAX. Bounds writes five floats when supplied. */
void trap_R_DrawText(int x, int y, const char *text, int font, float size, int limit, int forceColor) {
    /* The legacy menu API uses zero for unlimited; the native font API
     * uses a negative limit and treats zero as an empty run. */
    cgi->DrawScaledText(x, y, text, font, size, limit > 0 ? limit : -1, NULL, forceColor);
}
void trap_R_DrawTextClipped(int x, int y, const char *text, int font, float size, float *maxX) {
    cgi->DrawScaledText(x, y, text, font, size, -1, maxX, 0);
}
void trap_R_MeasureText(const char *text, int font, float size, int limit, float *width, float *height) {
    unsigned long long packed = cgi->MeasureText(text, NULL, font, size, limit > 0 ? limit : -1, NULL);
    float dimensions[2];
    memcpy(dimensions, &packed, sizeof(dimensions));
    *width = dimensions[0];
    *height = dimensions[1];
}

void trap_R_MeasureTextBounds(const char *text, int font, float size, int limit, float bounds[5]) {
    memset(bounds, 0, 5 * sizeof(*bounds));
    cgi->MeasureText(text, NULL, font, size, limit > 0 ? limit : -1, bounds);
}
