/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
// Retail dllEntry 0x10003970: 97 imports and 14 exports, API 8.
#ifndef UI_NATIVE_H
#define UI_NATIVE_H
typedef struct {
    void (QDECL *trap_Print)(const char *string); // 0
    void (QDECL *trap_Error)(const char *string); // 1
    int (QDECL *trap_Milliseconds)(void); // 2
    int (QDECL *trap_RealTime)(qtime_t *qtime); // 3
    void (QDECL *trap_Cvar_Register)(vmCvar_t *cvar, const char *var_name, const char *value, int flags); // 4
    void (QDECL *trap_Cvar_Create)(const char *var_name, const char *var_value, int flags); // 5
    void (QDECL *trap_Cvar_Update)(vmCvar_t *cvar); // 6
    void (QDECL *trap_Cvar_Set)(const char *var_name, const char *value); // 7
    void (QDECL *trap_Cvar_SetValue)(const char *var_name, float value); // 8
    void (QDECL *trap_Cvar_VariableStringBuffer)(const char *var_name, char *buffer, int bufsize); // 9
    float (QDECL *trap_Cvar_VariableValue)(const char *var_name); // 10
    int (QDECL *trap_Argc)(void); // 11
    void (QDECL *trap_Argv)(int n, char *buffer, int bufferLength); // 12
    void (QDECL *reserved13)(void); // 13
    int (QDECL *trap_FS_FOpenFile)(const char *qpath, fileHandle_t *f, fsMode_t mode); // 14
    void (QDECL *trap_FS_Read)(void *buffer, int len, fileHandle_t f); // 15
    void (QDECL *trap_FS_Write)(const void *buffer, int len, fileHandle_t f); // 16
    void (QDECL *trap_FS_FCloseFile)(fileHandle_t f); // 17
    int (QDECL *trap_FS_Seek)(fileHandle_t f, long offset, int origin); // 18
    int (QDECL *trap_FS_GetFileList)(const char *path, const char *extension, char *listbuf, int bufsize); // 19
    void (QDECL *AppendCommand)(const char *text); // 20 (retail 0x1000c35b)
    qhandle_t (QDECL *trap_R_RegisterModel)(const char *name); // 21
    qhandle_t (QDECL *trap_R_RegisterSkin)(const char *name); // 22
    qhandle_t (QDECL *trap_R_RegisterShaderNoMip)(const char *name); // 23
    void (QDECL *trap_R_ClearScene)(void); // 24
    void (QDECL *trap_R_AddRefEntityToScene)(const refEntity_t *re); // 25
    void (QDECL *trap_R_AddPolyToScene)(qhandle_t hShader , int numVerts, const polyVert_t *verts); // 26
    void (QDECL *trap_R_AddLightToScene)(const vec3_t org, float intensity, float r, float g, float b); // 27
    void (QDECL *trap_R_RenderScene)(const refdef_t *fd); // 28
    void (QDECL *trap_R_SetColor)(const float *rgba); // 29
    void (QDECL *trap_R_DrawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader); // 30
    void (QDECL *trap_R_ModelBounds)(clipHandle_t model, vec3_t mins, vec3_t maxs); // 31
    void (QDECL *trap_UpdateScreen)(void); // 32
    int (QDECL *trap_CM_LerpTag)(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName); // 33
    void (QDECL *trap_S_StartLocalSound)(sfxHandle_t sfx, int channelNum); // 34
    sfxHandle_t (QDECL *trap_S_RegisterSound)(const char *sample, qboolean compressed); // 35
    void (QDECL *trap_Key_KeynumToStringBuf)(int keynum, char *buf, int buflen); // 36
    void (QDECL *trap_Key_GetBindingBuf)(int keynum, char *buf, int buflen); // 37
    void (QDECL *trap_Key_SetBinding)(int keynum, const char *binding); // 38
    qboolean (QDECL *trap_Key_IsDown)(int keynum); // 39
    qboolean (QDECL *trap_Key_GetOverstrikeMode)(void); // 40
    void (QDECL *trap_Key_SetOverstrikeMode)(qboolean state); // 41
    void (QDECL *trap_Key_ClearStates)(void); // 42
    int (QDECL *trap_Key_GetCatcher)(void); // 43
    void (QDECL *trap_Key_SetCatcher)(int catcher); // 44
    void (QDECL *trap_GetClipboardData)(char *buf, int bufsize); // 45
    void (QDECL *trap_GetClientState)(uiClientState_t *state); // 46
    void (QDECL *trap_GetGlconfig)(glconfig_t *glconfig); // 47
    int (QDECL *trap_GetConfigString)(int index, char* buff, int buffsize); // 48
    int (QDECL *trap_LAN_GetServerCount)(int source); // 49
    void (QDECL *trap_LAN_GetServerAddressString)(int source, int n, char *buf, int buflen); // 50
    void (QDECL *trap_LAN_GetServerInfo)(int source, int n, char *buf, int buflen); // 51
    int (QDECL *trap_LAN_GetServerPing)(int source, int n); // 52
    int (QDECL *trap_LAN_GetPingQueueCount)(void); // 53
    void (QDECL *trap_LAN_ClearPing)(int n); // 54
    void (QDECL *trap_LAN_GetPing)(int n, char *buf, int buflen, int *pingtime); // 55
    void (QDECL *trap_LAN_GetPingInfo)(int n, char *buf, int buflen); // 56
    void (QDECL *trap_LAN_LoadCachedServers)(void); // 57
    void (QDECL *trap_LAN_SaveCachedServers)(void); // 58
    void (QDECL *trap_LAN_MarkServerVisible)(int source, int n, qboolean visible); // 59
    int (QDECL *trap_LAN_ServerIsVisible)(int source, int n); // 60
    qboolean (QDECL *trap_LAN_UpdateVisiblePings)(int source); // 61
    int (QDECL *trap_LAN_AddServer)(int source, const char *name, const char *addr); // 62
    void (QDECL *trap_LAN_RemoveServer)(int source, const char *addr); // 63
    void (QDECL *trap_LAN_ResetPings)(int n); // 64
    int (QDECL *trap_LAN_ServerStatus)(const char *serverAddress, char *serverStatus, int maxLen); // 65
    int (QDECL *trap_LAN_CompareServers)(int source, int sortKey, int sortDir, int s1, int s2); // 66
    int (QDECL *trap_MemoryRemaining)(void); // 67
    void (QDECL *trap_GetCDKey)(char *buf, int buflen); // 68
    void (QDECL *trap_SetCDKey)(char *buf); // 69
    void (QDECL *trap_R_RegisterFont)(const char *fontName, int pointSize, fontInfo_t *font); // 70
    void (QDECL *trap_S_StopBackgroundTrack)(void); // 71
    void (QDECL *trap_S_StartBackgroundTrack)(const char *intro, const char *loop); // 72
    int (QDECL *trap_CIN_PlayCinematic)(const char *arg0, int xpos, int ypos, int width, int height, int bits); // 73
    e_status (QDECL *trap_CIN_StopCinematic)(int handle); // 74
    void (QDECL *trap_CIN_DrawCinematic)(int handle); // 75
    e_status (QDECL *trap_CIN_RunCinematic)(int handle); // 76
    void (QDECL *trap_CIN_SetExtents)(int handle, int x, int y, int w, int h); // 77
    void (QDECL *trap_R_RemapShader)(const char *oldShader, const char *newShader, const char *timeOffset); // 78
    qboolean (QDECL *trap_VerifyCDKey)(const char *key, const char *chksum); // 79
    void (QDECL *reserved80)(void); // 80
    void (QDECL *reserved81)(void); // 81
    void (QDECL *reserved82)(void); // 82
    void (QDECL *reserved83)(void); // 83
    void (QDECL *reserved84)(void); // 84
    void (QDECL *reserved85)(void); // 85
    void (QDECL *reserved86)(void); // 86
    void (QDECL *reserved87)(void); // 87
    int (QDECL *trap_PC_AddGlobalDefine)(char *define); // 88
    int (QDECL *trap_PC_LoadSource)(const char *filename); // 89
    int (QDECL *trap_PC_FreeSource)(int handle); // 90
    int (QDECL *trap_PC_ReadToken)(int handle, pc_token_t *pc_token); // 91
    int (QDECL *trap_PC_SourceFileAndLine)(int handle, char *filename, int *line); // 92
    int (QDECL *IsSubscribedApp)(int appId); // 93 (retail init 0x1000feed)
    void (QDECL *DrawText)(int x, int y, const char *text, int font, float size, int limit, float *maxX, int forceColor); // 94
    unsigned long long (QDECL *MeasureText)(const char *text, const char *end, int font, float size, int limit, float *bounds); // 95
    void (QDECL *DownloadInfo)(unsigned low, unsigned high, unsigned long long *downloaded, unsigned long long *total); // 96
} uiNativeImports_t;
void UI_SetTextFont(int font);
qboolean trap_IsSubscribedApp(int appId);
void trap_DownloadInfo(unsigned long long item, unsigned long long *downloaded, unsigned long long *total);
void UI_NativeDrawText(int x, int y, const char *text, float size, int limit, int forceColor);
void UI_NativeMeasureText(const char *text, float size, int limit, float *width, float *height);
void UI_NativeMeasureRange(const char *text, const char *end, float size, int limit, float *bounds);
int UI_RefreshDisplayContext(void);
void UI_ForEachArenaName(void (QDECL *callback)(const char *));
qboolean UI_MenusVisible(void);
void UI_DrawAdvertisementWaitScreen(const char *menu);
#endif
