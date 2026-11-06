/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "ui_local.h"
#include "ui_native.h"
#include "../game/fnql_module.h"
#include "../../version/fnql_game_version.h"
static const uiNativeImports_t *uiImports;
static int textFont;
void UI_SetTextFont(int font) { textFont = font >= 0 && font <= 2 ? font : 0; }
void trap_Print(const char *string) { uiImports->trap_Print(string); }
void trap_Error(const char *string) { uiImports->trap_Error(string); }
int trap_Milliseconds(void) { return uiImports->trap_Milliseconds(); }
void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags) { uiImports->trap_Cvar_Register(cvar, var_name, value, flags); }
void trap_Cvar_Update(vmCvar_t *cvar) { uiImports->trap_Cvar_Update(cvar); }
void trap_Cvar_Set(const char *var_name, const char *value) { uiImports->trap_Cvar_Set(var_name, value); }
float trap_Cvar_VariableValue(const char *var_name) { return uiImports->trap_Cvar_VariableValue(var_name); }
void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize) { uiImports->trap_Cvar_VariableStringBuffer(var_name, buffer, bufsize); }
void trap_Cvar_SetValue(const char *var_name, float value) { uiImports->trap_Cvar_SetValue(var_name, value); }
void trap_Cvar_Create(const char *var_name, const char *var_value, int flags) { uiImports->trap_Cvar_Create(var_name, var_value, flags); }
int trap_Argc(void) { return uiImports->trap_Argc(); }
void trap_Argv(int n, char *buffer, int bufferLength) { uiImports->trap_Argv(n, buffer, bufferLength); }
void trap_Cmd_ExecuteText(int exec_when, const char *text) {
    /* Retail has only the append lane; never reinterpret EXEC_NOW as a pointer. */
    (void)exec_when;
    uiImports->AppendCommand(text);
}
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) { return uiImports->trap_FS_FOpenFile(qpath, f, mode); }
void trap_FS_Read(void *buffer, int len, fileHandle_t f) { uiImports->trap_FS_Read(buffer, len, f); }
void trap_FS_Write(const void *buffer, int len, fileHandle_t f) { uiImports->trap_FS_Write(buffer, len, f); }
void trap_FS_FCloseFile(fileHandle_t f) { uiImports->trap_FS_FCloseFile(f); }
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize) { return uiImports->trap_FS_GetFileList(path, extension, listbuf, bufsize); }
int trap_FS_Seek(fileHandle_t f, long offset, int origin) { return uiImports->trap_FS_Seek(f, offset, origin); }
qhandle_t trap_R_RegisterModel(const char *name) { return uiImports->trap_R_RegisterModel(name); }
qhandle_t trap_R_RegisterSkin(const char *name) { return uiImports->trap_R_RegisterSkin(name); }
void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) { uiImports->trap_R_RegisterFont(fontName, pointSize, font); }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { return uiImports->trap_R_RegisterShaderNoMip(name); }
void trap_R_ClearScene(void) { uiImports->trap_R_ClearScene(); }
void trap_R_AddRefEntityToScene(const refEntity_t *re) { uiImports->trap_R_AddRefEntityToScene(re); }
void trap_R_AddPolyToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts) { uiImports->trap_R_AddPolyToScene(hShader, numVerts, verts); }
void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) { uiImports->trap_R_AddLightToScene(org, intensity, r, g, b); }
void trap_R_RenderScene(const refdef_t *fd) { uiImports->trap_R_RenderScene(fd); }
void trap_R_SetColor(const float *rgba) { uiImports->trap_R_SetColor(rgba); }
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) { uiImports->trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader); }
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) { uiImports->trap_R_ModelBounds(model, mins, maxs); }
void trap_UpdateScreen(void) { uiImports->trap_UpdateScreen(); }
int trap_CM_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName) { return uiImports->trap_CM_LerpTag(tag, mod, startFrame, endFrame, frac, tagName); }
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum) { uiImports->trap_S_StartLocalSound(sfx, channelNum); }
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed) { return uiImports->trap_S_RegisterSound(sample, compressed); }
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen) { uiImports->trap_Key_KeynumToStringBuf(keynum, buf, buflen); }
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen) { uiImports->trap_Key_GetBindingBuf(keynum, buf, buflen); }
void trap_Key_SetBinding(int keynum, const char *binding) { uiImports->trap_Key_SetBinding(keynum, binding); }
qboolean trap_Key_IsDown(int keynum) { return uiImports->trap_Key_IsDown(keynum); }
qboolean trap_Key_GetOverstrikeMode(void) { return uiImports->trap_Key_GetOverstrikeMode(); }
void trap_Key_SetOverstrikeMode(qboolean state) { uiImports->trap_Key_SetOverstrikeMode(state); }
void trap_Key_ClearStates(void) { uiImports->trap_Key_ClearStates(); }
int trap_Key_GetCatcher(void) { return uiImports->trap_Key_GetCatcher(); }
void trap_Key_SetCatcher(int catcher) { uiImports->trap_Key_SetCatcher(catcher); }
void trap_GetClipboardData(char *buf, int bufsize) { uiImports->trap_GetClipboardData(buf, bufsize); }
void trap_GetClientState(uiClientState_t *state) { uiImports->trap_GetClientState(state); }
void trap_GetGlconfig(glconfig_t *glconfig) { uiImports->trap_GetGlconfig(glconfig); }
int trap_GetConfigString(int index, char* buff, int buffsize) { return uiImports->trap_GetConfigString(index, buff, buffsize); }
int trap_LAN_GetServerCount(int source) { return uiImports->trap_LAN_GetServerCount(source); }
void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen) { uiImports->trap_LAN_GetServerAddressString(source, n, buf, buflen); }
void trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen) { uiImports->trap_LAN_GetServerInfo(source, n, buf, buflen); }
int trap_LAN_GetServerPing(int source, int n) { return uiImports->trap_LAN_GetServerPing(source, n); }
int trap_LAN_GetPingQueueCount(void) { return uiImports->trap_LAN_GetPingQueueCount(); }
int trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen) { return uiImports->trap_LAN_ServerStatus(serverAddress, serverStatus, maxLen); }
void trap_LAN_SaveCachedServers(void) { uiImports->trap_LAN_SaveCachedServers(); }
void trap_LAN_LoadCachedServers(void) { uiImports->trap_LAN_LoadCachedServers(); }
void trap_LAN_ResetPings(int n) { uiImports->trap_LAN_ResetPings(n); }
void trap_LAN_ClearPing(int n) { uiImports->trap_LAN_ClearPing(n); }
void trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime) { uiImports->trap_LAN_GetPing(n, buf, buflen, pingtime); }
void trap_LAN_GetPingInfo(int n, char *buf, int buflen) { uiImports->trap_LAN_GetPingInfo(n, buf, buflen); }
void trap_LAN_MarkServerVisible(int source, int n, qboolean visible) { uiImports->trap_LAN_MarkServerVisible(source, n, visible); }
int trap_LAN_ServerIsVisible(int source, int n) { return uiImports->trap_LAN_ServerIsVisible(source, n); }
qboolean trap_LAN_UpdateVisiblePings(int source) { return uiImports->trap_LAN_UpdateVisiblePings(source); }
int trap_LAN_AddServer(int source, const char *name, const char *addr) { return uiImports->trap_LAN_AddServer(source, name, addr); }
void trap_LAN_RemoveServer(int source, const char *addr) { uiImports->trap_LAN_RemoveServer(source, addr); }
int trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2) { return uiImports->trap_LAN_CompareServers(source, sortKey, sortDir, s1, s2); }
int trap_MemoryRemaining(void) { return uiImports->trap_MemoryRemaining(); }
void trap_GetCDKey(char *buf, int buflen) { uiImports->trap_GetCDKey(buf, buflen); }
void trap_SetCDKey(char *buf) { uiImports->trap_SetCDKey(buf); }
int trap_PC_AddGlobalDefine(char *define) { return uiImports->trap_PC_AddGlobalDefine(define); }
int trap_PC_LoadSource(const char *filename) { return uiImports->trap_PC_LoadSource(filename); }
int trap_PC_FreeSource(int handle) { return uiImports->trap_PC_FreeSource(handle); }
int trap_PC_ReadToken(int handle, pc_token_t *pc_token) { return uiImports->trap_PC_ReadToken(handle, pc_token); }
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line) { return uiImports->trap_PC_SourceFileAndLine(handle, filename, line); }
void trap_S_StopBackgroundTrack(void) { uiImports->trap_S_StopBackgroundTrack(); }
void trap_S_StartBackgroundTrack(const char *intro, const char *loop) { uiImports->trap_S_StartBackgroundTrack(intro, loop); }
int trap_RealTime(qtime_t *qtime) { return uiImports->trap_RealTime(qtime); }
int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits) { return uiImports->trap_CIN_PlayCinematic(arg0, xpos, ypos, width, height, bits); }
e_status trap_CIN_StopCinematic(int handle) { return uiImports->trap_CIN_StopCinematic(handle); }
e_status trap_CIN_RunCinematic(int handle) { return uiImports->trap_CIN_RunCinematic(handle); }
void trap_CIN_DrawCinematic(int handle) { uiImports->trap_CIN_DrawCinematic(handle); }
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h) { uiImports->trap_CIN_SetExtents(handle, x, y, w, h); }
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset) { uiImports->trap_R_RemapShader(oldShader, newShader, timeOffset); }
qboolean trap_VerifyCDKey(const char *key, const char *chksum) { return uiImports->trap_VerifyCDKey(key, chksum); }
/* A host without the slot reports "not subscribed", the retail fallback. */
qboolean trap_IsSubscribedApp(int appId) { return uiImports->IsSubscribedApp && uiImports->IsSubscribedApp(appId) ? qtrue : qfalse; }
void trap_DownloadInfo(unsigned long long item, unsigned long long *downloaded, unsigned long long *total) {
    *downloaded = *total = 0;
    if (uiImports->DownloadInfo)
        uiImports->DownloadInfo((unsigned)(item & 0xffffffffu), (unsigned)(item >> 32), downloaded, total);
}
/* No access beyond the retail slab. These legacy calls have no retail slot. */
void trap_Cvar_Reset(const char *name) {
    /* A quoted cvar name cannot escape into another console command. */
    const char *p;
    for (p = name; *p; ++p) if (!(isalnum((unsigned char)*p) || *p == '_')) return;
    trap_Cmd_ExecuteText(EXEC_APPEND, va("reset %s\n", name));
}
void trap_Cvar_InfoStringBuffer(int bit, char *buffer, int size) {
    if (size > 0) buffer[0] = 0;
    if (bit == CVAR_SERVERINFO) trap_GetConfigString(CS_SERVERINFO, buffer, size);
}
void trap_SetPbClStatus(int status) { (void)status; }
void UI_NativeDrawText(int x, int y, const char *text, float size, int limit, int forceColor) {
    uiImports->DrawText(x, y, text, textFont, size, limit > 0 ? limit : -1, NULL, forceColor);
}
void UI_NativeMeasureText(const char *text, float size, int limit, float *width, float *height) {
    float bounds[5] = {0};
    /* Retail Text_GetDimensions (0x10003e03) reads the output bounds;
     * do not depend on the host's optional packed-register return. */
    uiImports->MeasureText(text, NULL, textFont, size, limit > 0 ? limit : -1, bounds);
    *width = bounds[2] - bounds[0]; *height = bounds[4];
}
/* Pixel bounds (left, top, right, bottom, height) of text[0..end), as the
 * retail cursor painter measures a prefix through import 95. */
void UI_NativeMeasureRange(const char *text, const char *end, float size, int limit, float *bounds) {
    int i;
    for (i = 0; i < 5; ++i) bounds[i] = 0;
    if (text) uiImports->MeasureText(text, end, textFont, size, limit > 0 ? limit : -1, bounds);
}
extern void _UI_Init(qboolean);
extern void _UI_Shutdown(void);
extern void _UI_KeyEvent(int, qboolean);
extern void _UI_MouseEvent(int, int);
extern void _UI_Refresh(int);
extern qboolean _UI_IsFullscreen(void);
extern void _UI_SetActiveMenu(uiMenuCommand_t);
static void UI_NativeKey(int key, int character, int down) {
    _UI_KeyEvent(character ? key | K_CHAR_FLAG : key, down ? qtrue : qfalse);
}
/* Retail export 9 (0x10003910) always reports a unique CD key. */
static qboolean UI_UniqueKey(void) { return qtrue; }
static void *exports[] = {
    (void *)_UI_Init, (void *)_UI_Shutdown, (void *)UI_NativeKey,
    (void *)_UI_MouseEvent, (void *)_UI_Refresh, (void *)_UI_IsFullscreen,
    (void *)_UI_SetActiveMenu, (void *)UI_ConsoleCommand, (void *)UI_DrawConnectScreen,
    (void *)UI_UniqueKey, (void *)UI_RefreshDisplayContext, (void *)UI_MenusVisible,
    (void *)UI_ForEachArenaName, (void *)UI_DrawAdvertisementWaitScreen
};
DLLEXPORT void QDECL dllEntry(void **table, const void *imports, int *version) {
    uiImports = (const uiNativeImports_t *)imports;
    if (table) *table = imports ? exports : NULL;
    if (version) *version = 8;
}
static const fnqlModuleInfo_t moduleInfo = {
    sizeof(fnqlModuleInfo_t), FNQL_MODULE_INFO_VERSION, FNQL_MODULE_PRODUCT,
    "ui", FNQL_GAME_VERSION_STRING, __DATE__ " " __TIME__, 0u, 8
};
DLLEXPORT const fnqlModuleInfo_t *QDECL FnQL_GetModuleInfo(void) { return &moduleInfo; }
typedef char import_size[(sizeof(uiNativeImports_t) == 97 * sizeof(void (*)(void))) ? 1 : -1];
typedef char export_count[(sizeof(exports) / sizeof(exports[0]) == 14) ? 1 : -1];
typedef char glconfig_size[(sizeof(glconfig_t) == 0x2c44) ? 1 : -1];
