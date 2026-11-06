/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../docs/contract/retail_menudef.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_raceBeep, cg_drawCheckpointRemaining;
static snapshot_t snapshot;
static const char *arguments[9], *points = "4", *pauseTime = "0";
static int argumentCount, plays, requests, centers, rejected, draws, hints, kills;
static char sound[MAX_QPATH], message[128], painted[64];
static const char *keyName = "k";
static char hintText[96];
static float hintX, hintY;
static float drawX, drawY;
void QDECL Com_Printf(const char *format, ...) { (void)format; }
void QDECL Com_Error(int code, const char *format, ...) { (void)code; (void)format; abort(); }
void QDECL CG_Printf(const char *format, ...) { CHECK(strstr(format,"malformed race_info")); ++rejected; }
int trap_Argc(void) { return argumentCount; }
void trap_Argv(int number, char *out, int size) {
    CHECK(number >= 0 && number < argumentCount); snprintf(out,size,"%s",arguments[number]);
}
const char *CG_ConfigString(int index) {
    CHECK(index == CS_RACE_RECORDS || index == CS_TIMEOUT_START_TIME);
    return index == CS_RACE_RECORDS ? points : pauseTime;
}
sfxHandle_t trap_S_RegisterSound(const char *name, qboolean compressed) {
    CHECK(!compressed); snprintf(sound,sizeof(sound),"%s",name); return 1;
}
void trap_S_StartLocalSound(sfxHandle_t handle, int channel) { CHECK(handle == 1 && channel == CHAN_LOCAL_SOUND); ++plays; }
void trap_SendClientCommand(const char *text) {
    if (!strcmp(text,"kill")) { ++kills; return; }
    CHECK(!strcmp(text,"raceinit")); ++requests;
}
int trap_Key_GetKey(const char *binding) { CHECK(!strcmp(binding,"kill")); return 75; }
void trap_Key_KeynumToStringBuf(int key, char *out, int size) {
    CHECK(key == 75); snprintf(out,size,"%s",keyName);
}
void CG_CenterPrint(const char *text, int y, int width) {
    CHECK(y == 320 && width == BIGCHAR_WIDTH*3/5);
    snprintf(message,sizeof(message),"%s",text); ++centers;
}
int CG_Text_Width(const char *text, float scale, int limit) { CHECK(scale == .5f && !limit); return (int)strlen(text)*5; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    if (scale == .18f) {
        CHECK(!adjust && !limit && style == 3 && color[0] == 1 && color[1] == 1 && color[2] == 1 && color[3] == .5f);
        snprintf(hintText,sizeof(hintText),"%s",text); hintX = x; hintY = y; ++hints; return;
    }
    CHECK(scale == .5f && !adjust && !limit && style == 3 && color[3] == 1);
    CHECK(color[0] == 1 && color[1] == 1);
    CHECK(color[2] == (!strcmp(text,"CURRENT RUN") || !strcmp(text,"LAST TIME") ? 0 : 1));
    snprintf(painted,sizeof(painted),"%s",text); drawX = x; drawY = y; ++draws;
}
static void Info(const char *active, const char *start, const char *last, const char *splits, const char *next, const char *following) {
    argumentCount = 7; arguments[0] = "race_info"; arguments[1] = active; arguments[2] = start;
    arguments[3] = last; arguments[4] = splits; arguments[5] = next; arguments[6] = following;
    CHECK(CG_ParseRaceCommand("race_info"));
}
int main(void) {
    entityState_t event = {0};
    cgRaceState_t saved;
    vec4_t color = {1,1,1,1};
    int i, before;
    cg.snap = &snapshot; cg.clientNum = 2; snapshot.ps.clientNum = 3; cgs.gametype = GT_RACE;
    cg_raceBeep.integer = 7; cg_drawCheckpointRemaining.integer = 1;
    cg.race.bestTime = 100; argumentCount = 1;
    CHECK(CG_ParseRaceCommand("race_init") && !cg.race.bestTime && requests == 1);
    CHECK(cg.race.nextPoint == -1 && cg.race.followingPoint == -1);
    CHECK(!CG_ParseRaceCommand("unrelated"));
    event.clientNum = 2; event.event = EV_RACE_START; event.time = 5000;
    event.otherEntityNum2 = 100; event.powerups = 101;
    CG_RaceEvent(&event); CHECK(!cg.race.active && !plays); /* Followed client, not local. */
    event.clientNum = 3; CG_RaceEvent(&event);
    CHECK(cg.race.active && cg.race.startedAt == 5000 && cg.race.pointsPassed == 1);
    CHECK(cg.race.nextPoint == 100 && cg.race.followingPoint == 101 && plays == 1);
    CHECK(!strcmp(sound,"sound/world/bell_01.ogg"));
    for (i = 0; i <= 9; ++i) {
        before = plays; cg_raceBeep.integer = i; CG_RaceEvent(&event);
        CHECK(plays == before+(i >= 1 && i <= 8));
    }
    cg_raceBeep.integer = 7; event.event = EV_RACE_CHECKPOINT; event.generic1 = 0;
    CG_RaceEvent(&event); CHECK(cg.race.pointsPassed == 2 && !cg.race.hasSplit);
    CHECK(!strcmp(message,"Checkpoint\n^72 remaining"));
    event.generic1 = 1; event.time = -1234; CG_RaceEvent(&event);
    CHECK(cg.race.hasSplit && cg.race.splitDelta == -1234 && !strcmp(message,"^2-0:01.234\n^71 remaining"));
    event.time = 0; CG_RaceEvent(&event); CHECK(!strcmp(message,"^70:00.000\n^70 remaining"));
    cg_drawCheckpointRemaining.integer = 0; event.time = 62001; CG_RaceEvent(&event);
    CHECK(!strcmp(message,"^11:02.001\n"));
    event.time = INT_MIN; CG_RaceEvent(&event); CHECK(!strcmp(message,"^2-35791:23.648\n"));
    saved = cg.race; before = plays; event.otherEntityNum2 = MAX_GENTITIES;
    CG_RaceEvent(&event); CHECK(!memcmp(&saved,&cg.race,sizeof(saved)) && plays == before);
    event.otherEntityNum2 = ENTITYNUM_NONE; event.powerups = 65535; CG_RaceEvent(&event);
    CHECK(cg.race.followingPoint == -1);
    event.event = EV_RACE_FINISH; event.time = 12345; before = centers;
    CG_RaceEvent(&event);
    CHECK(!cg.race.active && cg.race.lastTime == 12345 && cg.race.bestTime == 12345 && centers == before);
    CHECK(!strcmp(sound,"sound/world/klaxon1.ogg") && requests == 2);
    event.time = 10000; CG_RaceEvent(&event);
    CHECK(cg.race.bestTime == 10000 && !strcmp(message,"^2-0:02.345\n"));
    event.time = 11000; CG_RaceEvent(&event);
    CHECK(cg.race.bestTime == 10000 && cg.race.lastTime == 11000 && !strcmp(message,"^10:01.000\n"));
    event.generic1 = 0; event.time = -1; CG_RaceEvent(&event);
    CHECK(cg.race.lastTime == 11000 && cg.race.bestTime == 10000 && !cg.race.hasSplit);
    cg.demoPlayback = qtrue; before = requests; CG_RaceEvent(&event); CHECK(requests == before);
    cg.demoPlayback = qfalse;

    Info("2","5000","11000","3","100","-1");
    CHECK(cg.race.active && cg.race.startedAt == 5000 && cg.race.reportedSplits == 3 && cg.race.bestTime == 10000);
    saved = cg.race;
    Info("1","overflow","0","0","1","2");
    Info("1","2147483648","0","0","1","2");
    Info("1","0","-1","0","1","2");
    Info("1","0","0","65","1","2");
    Info("1","0","0","0","1024","2");
    Info("1","0","0","0","1","-2");
    argumentCount = 6; CHECK(CG_ParseRaceCommand("race_info"));
    argumentCount = 8; CHECK(CG_ParseRaceCommand("race_info"));
    CHECK(rejected == 8 && !memcmp(&saved,&cg.race,sizeof(saved)));
    cg.time = 6234;
    CG_DrawRaceHud(CG_RACE_TIMES,100,50,.5f,color,0); CHECK(!strcmp(painted,"0:01.234") && drawX == 100 && drawY == 50);
    CG_DrawRaceHud(CG_RACE_STATUS,100,50,.5f,color,1); CHECK(!strcmp(painted,"CURRENT RUN") && drawX == 72.5f);
    CHECK(hints == 1 && !strcmp(hintText,"Press k to respawn.") && hintX == 71.5f && hintY == 77);
    CG_RaceRespawnCommand(); CHECK(kills == 1 && cg.raceRespawnUsed);
    CG_ResetRace(qfalse,qfalse); CHECK(cg.raceRespawnUsed);
    Info("2","5000","11000","3","100","-1");
    CG_DrawRaceHud(CG_RACE_STATUS,100,50,.5f,color,1); CHECK(hints == 1);
    keyName = "<KEY NOT FOUND>";
    CG_DrawRaceHud(CG_RACE_STATUS,100,50,.5f,color,0);
    CHECK(hints == 2 && !strcmp(hintText,"Bind 'kill' to respawn") && hintX == 119);
    snapshot.ps.eFlags = EF_SPECTATOR; snapshot.ps.pm_flags = PMF_FOLLOW;
    CG_DrawRaceHud(CG_RACE_STATUS,100,50,.5f,color,0); CHECK(hints == 2);
    snapshot.ps.eFlags = snapshot.ps.pm_flags = 0; cg.demoPlayback = qtrue;
    CG_DrawRaceHud(CG_RACE_STATUS,100,50,.5f,color,0); CHECK(hints == 2);
    cg.demoPlayback = qfalse;
    pauseTime = "6000"; CG_DrawRaceHud(CG_RACE_TIMES,100,50,.5f,color,2);
    CHECK(!strcmp(painted,"0:01.000") && drawX == 60);
    CG_RaceLevelTimeChanged(1000,3000); CHECK(cg.race.startedAt == 7000);
    pauseTime = "0"; cg.time = 8000; CG_DrawRaceHud(CG_RACE_TIMES,0,0,.5f,color,0); CHECK(!strcmp(painted,"0:01.000"));
    cg.time = 6000; CG_DrawRaceHud(CG_RACE_TIMES,0,0,.5f,color,0); CHECK(!strcmp(painted,"0:00.000"));
    cg.race.active = qfalse; CG_DrawRaceHud(CG_RACE_TIMES,0,0,.5f,color,0); CHECK(!strcmp(painted,"0:11.000"));
    CG_DrawRaceHud(CG_RACE_STATUS,0,0,.5f,color,0); CHECK(!strcmp(painted,"LAST TIME"));
    cgs.gametype = GT_FFA; before = draws; CG_DrawRaceHud(CG_RACE_TIMES,0,0,.5f,color,0); CHECK(draws == before);
    CG_RaceRespawnCommand(); CHECK(kills == 2); /* Non-race kill still reaches the server. */
    saved = cg.race; CG_RaceEvent(&event); CHECK(!memcmp(&saved,&cg.race,sizeof(saved)));
    puts("Race: recipient, sounds, splits, records, transactional commands, reset and HUD timing OK");
    return 0;
}
