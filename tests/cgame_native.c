/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Exercise the production adapter without graphics, assets, or user input. */
#include "../code/cgame/cg_local.h"
#include "../code/cgame/cg_native.h"
#include "../code/game/fnql_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
extern void QDECL dllEntry(void **, const void *, int *);
static unsigned mutedLow, mutedHigh;
static int QDECL Muted(unsigned low, unsigned high) {
    mutedLow = low; mutedHigh = high; return low == (unsigned)76561198000000001ULL;
}
extern const fnqlModuleInfo_t *QDECL FnQL_GetModuleInfo(void);
cg_t cg;
cgs_t cgs;
static int calls, weaponValue, primaryValue, fovValue, killAll, frameClear;
static float sensitivityValue;
static snapshot_t supplied;
static qboolean snapshotAvailable;
static const char *playerInfo;
static int textCalls;
static int rangeCalls, registerCalls, updateCalls;
static int volumeCalls, localCalls;
static float soundVolume;
static void QDECL VolumeSound(sfxHandle_t sound,int channel,float volume) {
    CHECK(sound==42 && channel==CHAN_ANNOUNCER); ++volumeCalls; soundVolume=volume;
}
static void QDECL LocalSound(sfxHandle_t sound,int channel) {
    CHECK(sound==42 && channel==CHAN_ANNOUNCER); ++localCalls;
}
static vmCvar_t rangeCvar;
static void QDECL Register(vmCvar_t *cv, const char *name, const char *value, int flags) {
    CHECK(cv == &rangeCvar && !strcmp(name,"cg_fov") && !strcmp(value,"100") && flags == CVAR_ARCHIVE);
    ++registerCalls;
}
static void QDECL RegisterRange(vmCvar_t *cv, const char *name, const char *value,
    const char *minimum, const char *maximum, int flags) {
    Register(cv,name,value,flags);
    CHECK(!strcmp(minimum,"10") && !strcmp(maximum,"130"));
    ++rangeCalls;
}
static void QDECL Update(vmCvar_t *cv) { CHECK(cv == &rangeCvar); ++updateCalls; }
static void QDECL KeyName(int key, char *buffer, int size) {
    CHECK(key == 75 && size == 8);
    memset(buffer,'K',size); /* The adapter must enforce termination. */
}
static void QDECL DrawText(int x, int y, const char *text, int font, float size, int limit, float *clip, int force) {
    CHECK(x == 100 && y == 200 && !strcmp(text, "125") && font == 2);
    if (clip) { CHECK(size==37.25f && limit==-1 && force==0 && *clip==150); *clip=142.5f; return; }
    CHECK(size == 37.25f && !clip && force == 1);
    CHECK(limit == (textCalls++ ? 3 : -1));
}
static unsigned long long QDECL MeasureText(const char *text, const char *end, int font, float size, int limit, float *bounds) {
    unsigned long long packed;
    const float dimensions[2] = {42.5f, 17.25f};
    CHECK(!strcmp(text, "125") && !end && font == 2 && size == 37.25f && limit == (bounds ? 2 : -1));
    if (bounds) {
        int i; for (i=0;i<5;++i) { CHECK(bounds[i]==0); bounds[i]=(float)(i+1); }
    }
    memcpy(&packed, dimensions, sizeof(packed));
    return packed;
}
void CG_Init(int a, int b, int c) { calls = a + b + c; }
void CG_Shutdown(void) { ++calls; }
void CG_RegisterCvars(void) { ++calls; }
qboolean CG_ConsoleCommand(void) { return qtrue; }
void CG_DrawActiveFrame(int time, stereoFrame_t stereo, qboolean demo) { calls = time + stereo + demo; }
int CG_CrosshairPlayer(void) { return 7; }
int CG_LastAttacker(void) { return 9; }
void CG_KeyEvent(int key, qboolean down) { (void)key; (void)down; }
void CG_MouseEvent(int x, int y) { (void)x; (void)y; }
static int mouseX = -12345, mouseY = -12345, mouseMoves;
void CG_MouseMoveTo(int x, int y) { mouseX = x; mouseY = y; ++mouseMoves; }
void CG_EventHandling(cgame_event_t mode) { cgs.eventHandling = mode; }
void CG_CenterPrint(const char *text, int y, int width) { (void)text; (void)y; (void)width; }
static char chatText[64];
static int chatAdds;
void CG_ChatAreaAdd(const char *text, int type, int extraMsec) {
    CHECK(type == 0 && extraMsec == 0);
    snprintf(chatText, sizeof(chatText), "%s", text); ++chatAdds;
}
static void QDECL LastMessage(const char *name, char *buffer, int size) {
    CHECK(!strcmp(name, "cg_lastmsg")); snprintf(buffer, size, "%s", "^3notice text");
}
const char *CG_ConfigString(int index) { (void)index; return playerInfo; }
void QDECL Com_Printf(const char *format, ...) { (void)format; }
void QDECL Com_Error(int code, const char *format, ...) { (void)code; (void)format; abort(); }
static void QDECL UserCmd(int weapon, int primary, float sensitivity, int fov) {
    weaponValue = weapon; primaryValue = primary; sensitivityValue = sensitivity; fovValue = fov;
}
static void QDECL ClearFrame(void) { ++frameClear; }
static void QDECL ClearAll(void) { ++killAll; }
static qboolean QDECL Snapshot(int number, snapshot_t *out) { (void)number; *out = supplied; return snapshotAvailable; }
static void QDECL ModelToClip(const vec3_t point, vec4_t eye, vec4_t clip) {
    CHECK(point[0] == 12 && point[1] == 34);
    memset(eye, 0, sizeof(vec4_t));
    clip[0] = 2; clip[1] = 3; clip[2] = 4; clip[3] = point[2];
}
static void QDECL ClipToWindow(const vec4_t clip, vec4_t normalized, vec4_t window) {
    CHECK(clip[0] == 2 && clip[1] == 3 && clip[2] == 4 && clip[3] == 5);
    memset(normalized, 0, sizeof(vec4_t));
    window[0] = 320; window[1] = 240; window[2] = .5f; window[3] = 1;
}

int main(void) {
    cgameNativeImports_t imports = {0};
    void **exports = NULL;
    int api = 0, i;
    snapshot_t result;
    cgameClientIdentity_t identity;
    const fnqlModuleInfo_t *info;
    float width, height;
    vec3_t point = {12, 34, 5};
    vec4_t window;
    imports.DrawScaledText = DrawText;
    imports.MeasureText = MeasureText;
    imports.SetUserCmd = UserCmd;
    imports.trap_Cvar_Register = Register;
    imports.Cvar_RegisterRange = RegisterRange;
    imports.trap_Cvar_Update = Update;
    imports.StartLocalSoundVolume = VolumeSound;
    imports.trap_S_StartLocalSound = LocalSound;
    imports.ClearLoopingFrame = ClearFrame;
    imports.ClearLoopingAll = ClearAll;
    imports.trap_GetSnapshot = Snapshot;
    imports.trap_Cvar_VariableStringBuffer = LastMessage;
    dllEntry((void **)&exports, &imports, &api);
    CHECK(api == 8 && exports != NULL);
    {
        char key[8] = "before";
        CHECK(offsetof(cgameNativeImports_t,KeynumToString) == 98*sizeof(void (*)(void)));
        trap_Key_KeynumToStringBuf(75,key,sizeof(key)); CHECK(!key[0]);
        imports.KeynumToString = KeyName;
        trap_Key_KeynumToStringBuf(75,key,sizeof(key)); CHECK(!strcmp(key,"KKKKKKK"));
        trap_Key_KeynumToStringBuf(75,NULL,8);
        trap_Key_KeynumToStringBuf(75,key,0); CHECK(!strcmp(key,"KKKKKKK"));
    }
    CHECK(!trap_R_ProjectPoint(point, window));
    imports.TransformModelToClip = ModelToClip;
    imports.TransformClipToWindow = ClipToWindow;
    CHECK(offsetof(cgameNativeImports_t, TransformModelToClip) == 120 * sizeof(void (*)(void)));
    CHECK(offsetof(cgameNativeImports_t, TransformClipToWindow) == 121 * sizeof(void (*)(void)));
    CHECK(trap_R_ProjectPoint(point, window) && window[0] == 320 && window[1] == 240);
    point[2] = 0; CHECK(!trap_R_ProjectPoint(point, window));
    point[2] = -5; CHECK(!trap_R_ProjectPoint(point, window));
    trap_R_DrawText(100, 200, "125", 2, 37.25f, 0, 1);
    trap_R_DrawText(100, 200, "125", 2, 37.25f, 3, 1);
    { float edge=150; trap_R_DrawTextClipped(100,200,"125",2,37.25f,&edge); CHECK(edge==142.5f); }
    trap_R_MeasureText("125", 2, 37.25f, 0, &width, &height);
    CHECK(textCalls == 2 && width == 42.5f && height == 17.25f);
    {
        struct { float before, bounds[5], after; } packet = {77, {9,9,9,9,9}, 88};
        trap_R_MeasureTextBounds("125",2,37.25f,2,packet.bounds);
        CHECK(packet.before==77 && packet.after==88);
        for (i=0;i<5;++i) CHECK(packet.bounds[i]==i+1);
    }
    for (i = 0; i < 21; ++i) CHECK((exports[i] != NULL) == (i != 16));
    ((void (*)(int,int,int))exports[0])(1,2,3); CHECK(calls == 6);
    ((void (*)(void))exports[1])(); CHECK(calls == 7);
    ((void (*)(void))exports[2])(); CHECK(calls == 8);
    CHECK(((qboolean (*)(void))exports[3])() == qtrue);
    ((void (*)(int,stereoFrame_t,qboolean))exports[4])(100,STEREO_RIGHT,qtrue); CHECK(calls == 103);
    CHECK(((int (*)(void))exports[5])() == 7);
    CHECK(((int (*)(void))exports[6])() == 9);
    /* Slots 10/11 (0x10029ff0, 0x1002a060): message and notice timers plus the chat area. */
    cg.time = 500; ((void (*)(void))exports[10])();
    CHECK(cg.messageTime == 3500 && !cg.noticeTime && chatAdds == 1 && !strcmp(chatText, "^3notice text"));
    cg.time = 700; ((void (*)(void))exports[11])();
    CHECK(cg.noticeTime == 3700 && cg.messageTime == 3500 && chatAdds == 2);
    { /* Slot 8: retail 0x1003c4f0 absolute mapping, not deltas. */
        int cx, cy;
        void (*mouse)(int,int) = (void (*)(int,int))exports[8];
        CHECK(exports[8] != (void *)CG_MouseEvent);
        cgs.glconfig.vidWidth = 1280; cgs.glconfig.vidHeight = 960; cgs.screenXBias = 0;
        mouse(640, 480); CHECK(mouseMoves == 1 && mouseX == 320 && mouseY == 240);
        mouse(1279, 959); CHECK(mouseX == 639 && mouseY == 479);
        mouse(3, 3); CHECK(mouseX == 1 && mouseY == 1); /* truncation */
        /* 16:9 with retail's pillarbox bias: x is measured from the 4:3 edge. */
        cgs.glconfig.vidWidth = 1920; cgs.glconfig.vidHeight = 1080; cgs.screenXBias = 240;
        mouse(240, 0); CHECK(mouseX == 0 && mouseY == 0);
        mouse(1679, 1079); CHECK(mouseX == 639 && mouseY == 479);
        mouse(100, 540); CHECK(mouseX == -62 && mouseY == 240); /* unclamped, toward zero */
        mouse(1900, 540); CHECK(mouseX == 737);
        cgs.glconfig.vidHeight = 0; mouse(5, 5); CHECK(mouseMoves == 7);
        CHECK(!CG_NativeCursorFromPixels(1, 1, 0, 480, 0, &cx, &cy));
        CHECK(!CG_NativeCursorFromPixels(1, 1, 640, -1, 0, &cx, &cy));
        CHECK(CG_NativeCursorFromPixels(-3, -3, 640, 480, 0, &cx, &cy) && cx == -3 && cy == -3);
        CHECK(CG_NativeCursorFromPixels(INT_MAX, INT_MIN, 640, 480, 0, &cx, &cy) &&
              cx == INT_MAX && cy == INT_MIN);
        cgs.glconfig.vidWidth = cgs.glconfig.vidHeight = 0; cgs.screenXBias = 0;
    }
    ((void (*)(void))exports[12])(); CHECK(cg.chatExpanded);
    ((void (*)(void))exports[13])(); CHECK(!cg.chatExpanded);
    cg.physicsTime = 1234; CHECK(((int (*)(void))exports[14])() == 1234);
    for (i = 0; i < 16; ++i) {
        CHECK(((float (*)(void))exports[17])() == 413.0f);
        CHECK(((float (*)(void))exports[18])() == 640.0f);
    }
    CHECK(((int (*)(void))exports[19])() == 73);
    ((void (*)(int))exports[9])(5);
    CHECK(((float (*)(void))exports[17])() == 455.0f);
    CHECK(((float (*)(void))exports[18])() == 300.0f);
    CHECK(((int (*)(void))exports[19])() == 30);
    cg.time = 777;
    ((int (*)(int,int))exports[20])(63,1); CHECK(cgs.clientinfo[63].speaking == 1 && cgs.clientinfo[63].speakingTime == 777);
    ((int (*)(int,int))exports[20])(-1,1);
    ((int (*)(int,int))exports[20])(MAX_CLIENTS,1);
    CHECK(!((qboolean (*)(int,void *))exports[15])(0,&identity));
    cgs.clientinfo[0].infoValid = qtrue;
    strcpy(cgs.clientinfo[0].name,"^1Player");
    playerInfo = "\\st\\76561198000000000";
    CHECK(((qboolean (*)(int,void *))exports[15])(0,&identity));
    CHECK(((unsigned long long)identity.identityHigh << 32 | identity.identityLow) == 76561198000000000ULL);
    CHECK(!strcmp(identity.displayName,"^1Player") && !strcmp(identity.cleanName,"Player"));
    playerInfo = "\\st\\18446744073709551616";
    CHECK(((qboolean (*)(int,void *))exports[15])(0,&identity)); CHECK(identity.identityLow == 0 && identity.identityHigh == 0);
    CHECK(!((qboolean (*)(int,void *))exports[15])(-1,&identity));
    CHECK(!((qboolean (*)(int,void *))exports[15])(64,&identity));
    CHECK(!((qboolean (*)(int,void *))exports[15])(0,NULL));
    /* Import 125 (retail scoreboard feeders): mute state by Steam identity. */
    playerInfo = "\\st\\76561198000000001";
    CHECK(!CG_ClientMuted(0)); /* no import */
    imports.IsClientMuted = Muted;
    CHECK(CG_ClientMuted(0) && mutedLow == (unsigned)76561198000000001ULL);
    CHECK(mutedHigh == (unsigned)(76561198000000001ULL >> 32));
    playerInfo = "\\st\\5"; CHECK(!CG_ClientMuted(0) && mutedLow == 5 && mutedHigh == 0);
    CHECK(!CG_ClientMuted(-1) && !CG_ClientMuted(MAX_CLIENTS));
    cgs.clientinfo[0].infoValid = qfalse; CHECK(!CG_ClientMuted(0)); cgs.clientinfo[0].infoValid = qtrue;
    imports.IsClientMuted = NULL;
    trap_Cvar_RegisterRange(&rangeCvar,"cg_fov","100","10","130",CVAR_ARCHIVE);
    CHECK(rangeCalls == 1 && registerCalls == 1 && updateCalls == 1);
    imports.Cvar_RegisterRange = NULL;
    trap_Cvar_RegisterRange(&rangeCvar,"cg_fov","100","10","130",CVAR_ARCHIVE);
    CHECK(rangeCalls == 1 && registerCalls == 2 && updateCalls == 2);
    CHECK(offsetof(cgameNativeImports_t,StartLocalSoundVolume)==40*sizeof(void (*)(void)));
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,.625f); CHECK(volumeCalls==1 && soundVolume==.625f);
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,-1); CHECK(volumeCalls==2 && soundVolume==0);
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,3); CHECK(volumeCalls==3 && soundVolume==2);
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,NAN); CHECK(volumeCalls==4 && soundVolume==0);
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,INFINITY); CHECK(volumeCalls==5 && soundVolume==2);
    imports.StartLocalSoundVolume=NULL;
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,0); CHECK(!localCalls);
    trap_S_StartLocalSoundVolume(42,CHAN_ANNOUNCER,.5f); CHECK(localCalls==1);
    cg.refdef.fov_x = 120;
    cg.userCmdFov = 103;
    trap_SetUserCmdValue(5,0.625f); CHECK(weaponValue == 5 && primaryValue == 0 && sensitivityValue == 0.625f && fovValue == 103);
    cg.weaponPrimary = WP_RAILGUN;
    trap_SetUserCmdValue(5,0.625f); CHECK(primaryValue == WP_RAILGUN);
    cg.weaponPrimary = MAX_WEAPONS;
    trap_SetUserCmdValue(5,0.625f); CHECK(primaryValue == WP_NONE);
    cg.weaponPrimary = -1;
    trap_SetUserCmdValue(5,0.625f); CHECK(primaryValue == WP_NONE);
    trap_S_ClearLoopingSounds(qfalse); trap_S_ClearLoopingSounds(qtrue); CHECK(frameClear == 1 && killAll == 1);
    CHECK(!trap_GetSnapshot(0,&result));
    snapshotAvailable = qtrue; supplied.numEntities = 384; supplied.ps.clientNum = 63;
    supplied.entities[383].number = MAX_GENTITIES-1;
    CHECK(trap_GetSnapshot(0,&result)); CHECK(result.entities[383].number == MAX_GENTITIES-1);
    supplied.numEntities = 385; CHECK(!trap_GetSnapshot(0,&result));
    supplied.numEntities = -1; CHECK(!trap_GetSnapshot(0,&result));
    supplied.numEntities = 1; supplied.entities[0].number = MAX_GENTITIES; CHECK(!trap_GetSnapshot(0,&result));
    supplied.entities[0].number = 0; supplied.ps.clientNum = MAX_CLIENTS; CHECK(!trap_GetSnapshot(0,&result));
    CHECK(!trap_GetSnapshot(0,NULL));
    info = FnQL_GetModuleInfo(); CHECK(!strcmp(info->module,"cgame") && info->nativeApi == 8 && info->capabilities == 0);
    puts("cgame native ABI: exports, float calls, snapshots, identity, mute import and bounds OK");
    return 0;
}
