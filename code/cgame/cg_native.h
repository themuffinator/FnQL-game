/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
// Retail cgame import ABI: FnQL cg_public.h and retail dllEntry 0x10020a70.
#ifndef CG_NATIVE_H
#define CG_NATIVE_H
#define CGAME_NATIVE_API_VERSION 8
#define CGAME_NATIVE_IMPORT_COUNT 128
typedef struct {
	void (QDECL *trap_Print)(const char *text); // 0
	void (QDECL *trap_Error)(const char *text); // 1
	int (QDECL *trap_Milliseconds)(void); // 2
	int (QDECL *trap_RealTime)(qtime_t *qtime); // 3
	void (QDECL *trap_Cvar_Register)(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags); // 4
	void (QDECL *Cvar_RegisterRange)(vmCvar_t *vmCvar, const char *name, const char *value,
		const char *minimum, const char *maximum, int flags); // 5
	void (QDECL *trap_Cvar_Update)(vmCvar_t *vmCvar); // 6
	void (QDECL *trap_Cvar_Set)(const char *var_name, const char *value); // 7
	void (QDECL *reserved8)(void); // 8, unused by this module
	void (QDECL *trap_Cvar_VariableStringBuffer)(const char *var_name, char *buffer, int bufsize); // 9
	void (QDECL *reserved10)(void); // 10, unused by this module
	int (QDECL *trap_Argc)(void); // 11
	void (QDECL *trap_Argv)(int n, char *buffer, int bufferLength); // 12
	void (QDECL *trap_Args)(char *buffer, int bufferLength); // 13
	int (QDECL *trap_FS_FOpenFile)(const char *qpath, fileHandle_t *f, fsMode_t mode); // 14
	void (QDECL *trap_FS_Read)(void *buffer, int len, fileHandle_t f); // 15
	void (QDECL *trap_FS_Write)(const void *buffer, int len, fileHandle_t f); // 16
	void (QDECL *trap_FS_FCloseFile)(fileHandle_t f); // 17
	int (QDECL *trap_FS_Seek)(fileHandle_t f, long offset, fsOrigin_t origin); // 18
	int (QDECL *trap_FS_GetFileList)(const char *path, const char *extension, char *listbuf, int bufsize); // 19
	void (QDECL *trap_SendConsoleCommand)(const char *text); // 20
	void (QDECL *trap_AddCommand)(const char *cmdName); // 21
	void (QDECL *trap_RemoveCommand)(const char *cmdName); // 22
	void (QDECL *trap_SendClientCommand)(const char *s); // 23
	void (QDECL *trap_UpdateScreen)(void); // 24
	void (QDECL *trap_CM_LoadMap)(const char *mapname); // 25
	int (QDECL *trap_CM_NumInlineModels)(void); // 26
	clipHandle_t (QDECL *trap_CM_InlineModel)(int index); // 27
	clipHandle_t (QDECL *trap_CM_TempBoxModel)(const vec3_t mins, const vec3_t maxs); // 28
	clipHandle_t (QDECL *trap_CM_TempCapsuleModel)(const vec3_t mins, const vec3_t maxs); // 29
	int (QDECL *trap_CM_PointContents)(const vec3_t p, clipHandle_t model); // 30
	int (QDECL *trap_CM_TransformedPointContents)(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles); // 31
	void (QDECL *trap_CM_BoxTrace)(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask); // 32
	void (QDECL *trap_CM_CapsuleTrace)(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask); // 33
	void (QDECL *trap_CM_TransformedBoxTrace)(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles); // 34
	void (QDECL *trap_CM_TransformedCapsuleTrace)(trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles); // 35
	int (QDECL *trap_CM_MarkFragments)(int numPoints, const vec3_t *points, 
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer); // 36
	void (QDECL *trap_S_StartSound)(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx); // 37
	void (QDECL *reserved38)(void); // 38, unused by this module
	void (QDECL *trap_S_StartLocalSound)(sfxHandle_t sfx, int channelNum); // 39
	void (QDECL *StartLocalSoundVolume)(sfxHandle_t sfx, int channelNum, float volume); // 40
	void (QDECL *ClearLoopingFrame)(void); // 41
	void (QDECL *ClearLoopingAll)(void); // 42
	void (QDECL *trap_S_AddLoopingSound)(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx); // 43
	void (QDECL *trap_S_UpdateEntityPosition)(int entityNum, const vec3_t origin); // 44
	void (QDECL *trap_S_Respatialize)(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater); // 45
	sfxHandle_t (QDECL *trap_S_RegisterSound)(const char *sample, qboolean compressed); // 46
	void (QDECL *trap_S_StartBackgroundTrack)(const char *intro, const char *loop); // 47
	void (QDECL *trap_S_StopBackgroundTrack)(void); // 48
	void (QDECL *trap_R_LoadWorldMap)(const char *mapname); // 49
	qhandle_t (QDECL *trap_R_RegisterModel)(const char *name); // 50
	qhandle_t (QDECL *trap_R_RegisterSkin)(const char *name); // 51
	qhandle_t (QDECL *trap_R_RegisterShader)(const char *name); // 52
	qhandle_t (QDECL *trap_R_RegisterShaderNoMip)(const char *name); // 53
	void (QDECL *reserved54)(void); // 54, unused by this module
	void (QDECL *reserved55)(void); // 55, unused by this module
	void (QDECL *reserved56)(void); // 56, unused by this module
	void (QDECL *reserved57)(void); // 57, unused by this module
	void (QDECL *reserved58)(void); // 58, unused by this module
	void (QDECL *reserved59)(void); // 59, unused by this module
	void (QDECL *reserved60)(void); // 60, unused by this module
	void (QDECL *reserved61)(void); // 61, unused by this module
	void (QDECL *reserved62)(void); // 62, unused by this module
	void (QDECL *reserved63)(void); // 63, unused by this module
	void (QDECL *reserved64)(void); // 64, unused by this module
	void (QDECL *reserved65)(void); // 65, unused by this module
	void (QDECL *reserved66)(void); // 66, unused by this module
	void (QDECL *reserved67)(void); // 67, unused by this module
	void (QDECL *reserved68)(void); // 68, unused by this module
	void (QDECL *reserved69)(void); // 69, unused by this module
	void (QDECL *trap_R_ClearScene)(void); // 70
	void (QDECL *trap_R_AddRefEntityToScene)(const refEntity_t *re); // 71
	void (QDECL *trap_R_AddPolyToScene)(qhandle_t hShader , int numVerts, const polyVert_t *verts); // 72
	void (QDECL *trap_R_AddPolysToScene)(qhandle_t hShader , int numVerts, const polyVert_t *verts, int num); // 73
	void (QDECL *trap_R_AddLightToScene)(const vec3_t org, float intensity, float r, float g, float b); // 74
	int (QDECL *trap_R_LightForPoint)(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir); // 75
	void (QDECL *trap_R_RenderScene)(const refdef_t *fd); // 76
	void (QDECL *reserved77)(void); // 77, unused by this module
	void (QDECL *trap_R_SetColor)(const float *rgba); // 78
	void (QDECL *trap_R_DrawStretchPic)(float x, float y, float w, float h, 
							   float s1, float t1, float s2, float t2, qhandle_t hShader); // 79
	void (QDECL *reserved80)(void); // 80, unused by this module
	void (QDECL *trap_R_ModelBounds)(clipHandle_t model, vec3_t mins, vec3_t maxs); // 81
	int (QDECL *trap_R_LerpTag)(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName); // 82
	void (QDECL *trap_R_RemapShader)(const char *oldShader, const char *newShader, const char *timeOffset); // 83
	void (QDECL *trap_GetGlconfig)(glconfig_t *glconfig); // 84
	void (QDECL *trap_GetGameState)(gameState_t *gamestate); // 85
	void (QDECL *trap_GetCurrentSnapshotNumber)(int *snapshotNumber, int *serverTime); // 86
	qboolean (QDECL *trap_GetSnapshot)(int snapshotNumber, snapshot_t *snapshot); // 87
	qboolean (QDECL *trap_GetServerCommand)(int serverCommandNumber); // 88
	int (QDECL *trap_GetCurrentCmdNumber)(void); // 89
	qboolean (QDECL *trap_GetUserCmd)(int cmdNumber, usercmd_t *ucmd); // 90
	void (QDECL *SetUserCmd)(int weapon, int primary, float sensitivity, int fov); // 91
	int (QDECL *trap_MemoryRemaining)(void); // 92
	void (QDECL *trap_R_RegisterFont)(const char *fontName, int pointSize, fontInfo_t *font); // 93
	qboolean (QDECL *trap_Key_IsDown)(int keynum); // 94
	int (QDECL *trap_Key_GetCatcher)(void); // 95
	void (QDECL *trap_Key_SetCatcher)(int catcher); // 96
	int (QDECL *trap_Key_GetKey)(const char *binding); // 97
	void (QDECL *KeynumToString)(int keynum, char *buffer, int size); // 98
	void (QDECL *reserved99)(void); // 99, unused by this module
	void (QDECL *reserved100)(void); // 100, unused by this module
	int (QDECL *trap_CIN_PlayCinematic)(const char *arg0, int xpos, int ypos, int width, int height, int bits); // 101
	e_status (QDECL *trap_CIN_StopCinematic)(int handle); // 102
	e_status (QDECL *trap_CIN_RunCinematic)(int handle); // 103
	void (QDECL *trap_CIN_DrawCinematic)(int handle); // 104
	void (QDECL *trap_CIN_SetExtents)(int handle, int x, int y, int w, int h); // 105
	qboolean (QDECL *trap_GetEntityToken)(char *buffer, int bufferSize); // 106
	int (QDECL *trap_PC_AddGlobalDefine)(char *define); // 107
	int (QDECL *trap_PC_LoadSource)(const char *filename); // 108
	int (QDECL *trap_PC_FreeSource)(int handle); // 109
	int (QDECL *trap_PC_ReadToken)(int handle, pc_token_t *pc_token); // 110
	int (QDECL *trap_PC_SourceFileAndLine)(int handle, char *filename, int *line); // 111
	void (QDECL *reserved112)(void); // 112, unused by this module
	void (QDECL *reserved113)(void); // 113, unused by this module
	void (QDECL *reserved114)(void); // 114, unused by this module
	void (QDECL *reserved115)(void); // 115, unused by this module
	void (QDECL *reserved116)(void); // 116, unused by this module
	void (QDECL *reserved117)(void); // 117, unused by this module
	void (QDECL *reserved118)(void); // 118, unused by this module
	void (QDECL *reserved119)(void); // 119, unused by this module
	void (QDECL *TransformModelToClip)(const vec3_t point, vec4_t eye, vec4_t clip); // 120
	void (QDECL *TransformClipToWindow)(const vec4_t clip, vec4_t normalized, vec4_t window); // 121
	void (QDECL *reserved122)(void); // 122, unused by this module
	void (QDECL *DrawScaledText)(int x, int y, const char *text, int font, float scale, int limit, float *maxX, int forceColor); // 123
	unsigned long long (QDECL *MeasureText)(const char *text, const char *end, int font, float scale, int limit, float *bounds); // 124
	int (QDECL *IsClientMuted)(unsigned identityLow, unsigned identityHigh); // 125
	void (QDECL *reserved126)(void); // 126, unused by this module
	void (QDECL *reserved127)(void); // 127, unused by this module
} cgameNativeImports_t;

qboolean CG_ClientMuted(int client);
qboolean CG_NativeCursorFromPixels(int x, int y, int vidWidth, int vidHeight, float bias,
                                   int *cursorX, int *cursorY);
#endif
