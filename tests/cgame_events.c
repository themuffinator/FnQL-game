/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_buzzerSound;
vmCvar_t cg_footsteps;
vmCvar_t cg_announcer;
vmCvar_t cg_announcerLastStandingVO, cg_announcerLeadsVO, cg_announcerTiesVO, cg_followPowerup;
vmCvar_t cg_impactSparks, cg_impactSparksSize, cg_impactSparksLifetime, cg_impactSparksVelocity;
int bg_numItems = 60;
static int registrations, plays, lastEntity, lastChannel, lastSound, localSound, pois;
static char registered[32][MAX_QPATH];
static char sentCommand[64];
static int commandsSent;
void trap_SendClientCommand(const char *s) { snprintf(sentCommand, sizeof(sentCommand), "%s", s); ++commandsSent; }
static snapshot_t snapshot;
static localEntity_t puff;
static int puffCount;
static int bufferedSound, missingAlternate;
static char lastVoice[MAX_QPATH], lastMedal[MAX_QPATH];
static int damagePlums;
static int bleeds, sparks;
static vec3_t bloodPosition;
static char centerMessage[MAX_STRING_CHARS], music[MAX_QPATH];
static const char *winnerList = "", *settingsMask = "0";
const char *CG_ConfigString(int index) {
    CHECK(index == CS_SETTINGS_MASK || index == CS_MATCH_AWARDS);
    return index == CS_SETTINGS_MASK ? settingsMask : winnerList;
}
static int musicStarts, buzzers;
static int thaws, throughImpacts, raceEvents;
void CG_RaceEvent(const entityState_t *event) {
    CHECK((event->event & ~EV_EVENT_BITS) == EV_RACE_START + raceEvents);
    ++raceEvents;
}
void CG_ThawPlayer(const vec3_t origin) {
    CHECK(origin[0] == 32 && origin[1] == 48 && origin[2] == 64); ++thaws;
}
void ByteToDir(int encoded, vec3_t direction) {
    CHECK(encoded == 5); VectorSet(direction,0,0,1);
}
void CG_MissileHitWallDmgThrough(int weapon, vec3_t origin, vec3_t direction) {
    CHECK(weapon == WP_ROCKET_LAUNCHER && origin[0] == 32 && direction[2] == 1); ++throughImpacts;
}
void CG_CenterPrint(const char *text, int y, int width) {
    CHECK(y == 90 && width == BIGCHAR_WIDTH); snprintf(centerMessage,sizeof(centerMessage),"%s",text);
}
void trap_S_StartBackgroundTrack(const char *intro, const char *loop) {
    CHECK(!loop[0]); snprintf(music,sizeof(music),"%s",intro); ++musicStarts;
}
void trap_S_StartLocalSound(sfxHandle_t sound, int channel) {
    CHECK(channel == CHAN_LOCAL_SOUND); localSound = sound;
    if (sound == 12) ++buzzers;
}
void CG_PointOfInterest(const centity_t *cent) { CHECK(cent); ++pois; }
void CG_DamagePlum(const centity_t *cent) { ++damagePlums; }
void CG_Bleed(const vec3_t origin, int entity) {
    CHECK(entity == 7 && origin[0] >= 16 && origin[0] < 48 &&
          origin[1] >= 32 && origin[1] < 64 && origin[2] >= 72 && origin[2] < 96);
    if (bleeds & 1) CHECK(!memcmp(origin, bloodPosition, sizeof(vec3_t)));
    VectorCopy(origin, bloodPosition); ++bleeds;
}

void CG_AddBufferedSound(sfxHandle_t sound) { bufferedSound = sound; }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) {
    strcpy(lastMedal, name);
    return 101;
}

qhandle_t trap_R_RegisterShader(const char *name) {
    if (!strcmp(name, "gfx/misc/tracer")) return 82;
    CHECK(!strcmp(name, "models/weaphits/electric.tga"));
    return 80;
}
localEntity_t *CG_SmokePuff(const vec3_t origin, const vec3_t velocity, float radius,
                          float r, float g, float b, float a, float duration,
                          int start, int fadeIn, int flags, qhandle_t shader) {
    if (shader == 82) {
        CHECK(!memcmp(origin, bloodPosition, sizeof(vec3_t)) && !velocity[0] && !velocity[1] && velocity[2] == 128);
        CHECK(radius == 8 && duration == 250 && start == cg.time && fadeIn == 0 && flags == LEF_PUFF_DONT_SCALE);
        CHECK(r == 1 && g == 1 && b == 1 && a == 1);
        ++sparks; return &puff;
    }
    CHECK(origin[0] == 32 && origin[1] == 48 && origin[2] == 64);
    CHECK(!velocity[0] && !velocity[1] && !velocity[2]);
    CHECK(radius == 96 && duration == 450 && start == 2000);
    CHECK(r == 1 && g == 1 && b == 1 && a == 1 && fadeIn == 0 && flags == 0 && shader == 80);
    ++puffCount;
    return &puff;
}

int QDECL Com_sprintf(char *dest, int size, const char *format, ...) {
    int length;
    va_list args;
    va_start(args, format); length = vsnprintf(dest, size, format, args); va_end(args);
    return length;
}
sfxHandle_t trap_S_RegisterSound(const char *name, qboolean compressed) {
    if (compressed) {
        strcpy(lastVoice, name);
        return missingAlternate && strncmp(name, "sound/vo/", 9) ? 0 : 100;
    }
    CHECK(!compressed && registrations < 32);
    strcpy(registered[registrations], name);
    return ++registrations;
}
void trap_S_StartSound(const vec3_t origin, int entity, int channel, sfxHandle_t sound) {
    CHECK(!origin);
    ++plays; lastEntity = entity; lastChannel = channel; lastSound = sound;
}
sfxHandle_t CG_CustomSound(int client, const char *name) {
    CHECK(client == 7 && !strcmp(name, "*drown.wav"));
    return 77;
}

int main(void) {
    centity_t cent = {0};
    entityState_t *event = &cent.currentState;
    cgTimedItem_t before[CG_MAX_TIMED_ITEMS];
    int i;
    CG_RegisterQLEventSounds();
    CHECK(registrations == 13 && !strcmp(registered[12], "sound/items/kam_explode_far.ogg"));
    CHECK(!strcmp(registered[0], "sound/player/footsteps/snow1.ogg"));
    CHECK(!strcmp(registered[7], "sound/player/footsteps/wood4.ogg"));
    CHECK(!strcmp(registered[8], "sound/misc/ar1_pkup.ogg"));
    CHECK(!strcmp(registered[9], "sound/misc/tim_pump.ogg"));
    CHECK(!strcmp(registered[10], "sound/world/klaxon2.ogg"));
    CHECK(!strcmp(registered[11], "sound/world/buzzer.ogg"));
    cg.snap = &snapshot; cg.clientNum = 7; snapshot.ps.clientNum = 7;
    event->number = 7; event->event = EV_FOOTSTEP_SNOW | EV_EVENT_BIT1;
    CHECK(CG_QL_EntityEvent(&cent) && plays == 0);
    cg_footsteps.integer = 1;
    CHECK(CG_QL_EntityEvent(&cent) && plays == 1);
    CHECK(lastEntity == 7 && lastChannel == CHAN_BODY && lastSound >= 1 && lastSound <= 4);
    event->event = EV_FOOTSTEP_WOOD;
    CHECK(CG_QL_EntityEvent(&cent) && lastSound >= 5 && lastSound <= 8);
    event->event = EV_DROWN;
    CHECK(CG_QL_EntityEvent(&cent) && lastChannel == CHAN_VOICE && lastSound == 77);
    event->event = EV_POWERUP_ARMORREGEN; cg.time = 1000;
    CHECK(CG_QL_EntityEvent(&cent) && lastChannel == CHAN_ITEM && lastSound == 9);
    CHECK(cg.powerupActive == PW_ARMORREGEN && cg.powerupTime == 1000);
    event->number = 8; cg.time = 1100;
    CHECK(CG_QL_EntityEvent(&cent) && cg.powerupTime == 1000 && lastEntity == 8);
    event->event = EV_THAW_TICK;
    CHECK(CG_QL_EntityEvent(&cent) && lastChannel == CHAN_BODY && lastSound == 10);

    event->event = EV_DROP_WEAPON; event->clientNum = 7; event->eventParm = WP_HEAVY_MACHINEGUN;
    snapshot.ps.stats[STAT_WEAPONS] = (1 << WP_HEAVY_MACHINEGUN) | (1 << WP_RAILGUN) | (1 << WP_GRAPPLING_HOOK);
    snapshot.ps.ammo[WP_HEAVY_MACHINEGUN] = 100;
    snapshot.ps.ammo[WP_RAILGUN] = 10;
    snapshot.ps.ammo[WP_GRAPPLING_HOOK] = -1;
    CHECK(CG_QL_EntityEvent(&cent) && cg.weaponSelect == WP_GRAPPLING_HOOK && cg.weaponSelectTime == 1100);
    snapshot.ps.ammo[WP_GRAPPLING_HOOK] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && cg.weaponSelect == WP_RAILGUN);
    snapshot.ps.ammo[WP_RAILGUN] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && cg.weaponSelect == WP_RAILGUN); /* No replacement. */
    snapshot.ps.ammo[WP_RAILGUN] = 10;
    event->clientNum = 8; snapshot.ps.clientNum = 8; cg.weaponSelect = WP_MACHINEGUN;
    CHECK(CG_QL_EntityEvent(&cent) && cg.weaponSelect == WP_MACHINEGUN); /* Followed, not local. */

    event->event = EV_ITEM_PICKUP_SPEC; event->otherEntityNum = 42; event->otherEntityNum2 = 2;
    event->modelindex = 3; event->modelindex2 = 1; event->groundEntityNum = 1;
    event->time = 31000; event->time2 = 456; event->pos.trBase[0] = 12.5f;
    CHECK(CG_QL_EntityEvent(&cent));
    CHECK(cg.timedItems[0].entity == 42 && cg.timedItems[0].side == 2 && cg.timedItems[0].item == 3);
    CHECK(cg.timedItems[0].remaining == 29900 && cg.timedItemCount == 1 && cg.timedItems[0].group == 1);
    CHECK(cg.timedItems[0].origin[0] == 12.5f && cg.timedItems[0].distance == 456);
    cg.time = 2000; event->time = 33000;
    CHECK(CG_QL_EntityEvent(&cent) && cg.timedItems[0].remaining == 31000 && cg.timedItemCount == 1 && cg.timedItems[0].group == 2);
    for (i = 1; i < CG_MAX_TIMED_ITEMS; ++i) {
        event->otherEntityNum = 42 + i;
        CHECK(CG_QL_EntityEvent(&cent));
    }
    CHECK(cg.timedItemCount == CG_MAX_TIMED_ITEMS);
    memcpy(before, cg.timedItems, sizeof(before));
    event->otherEntityNum = 100;
    CHECK(CG_QL_EntityEvent(&cent) && !memcmp(before, cg.timedItems, sizeof(before)));
    event->otherEntityNum = -1;
    CHECK(CG_QL_EntityEvent(&cent) && !memcmp(before, cg.timedItems, sizeof(before)));
    event->otherEntityNum = 42; event->modelindex = bg_numItems;
    CHECK(CG_QL_EntityEvent(&cent) && !memcmp(before, cg.timedItems, sizeof(before)));
    event->event = EV_FIRE_WEAPON;
    CHECK(!CG_QL_EntityEvent(&cent)); /* Existing dispatcher remains authoritative. */
    event->event = EV_MAX + 1;
    CHECK(!CG_QL_EntityEvent(&cent));
    event->event = EV_LIGHTNING_DISCHARGE; event->eventParm = 150;
    cent.lerpOrigin[0] = 32; cent.lerpOrigin[1] = 48; cent.lerpOrigin[2] = 64;
    CHECK(CG_QL_EntityEvent(&cent) && puffCount == 1 && puff.leType == LE_SCALE_FADE);
    event->eventParm = -1;
    CHECK(CG_QL_EntityEvent(&cent) && puffCount == 1);
    event->eventParm = 256;
    CHECK(CG_QL_EntityEvent(&cent) && puffCount == 1);
    event->event = EV_AWARD; event->clientNum = snapshot.ps.clientNum;
    event->generic1 = 5; event->modelindex2 = 2;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardStack == 1 && cg.rewardCount[1] == 2);
    CHECK(cg.rewardSound[1] == 100 && cg.rewardShader[1] == 101);
    CHECK(!strcmp(lastVoice, "sound/vo/headshot.ogg") && !strcmp(lastMedal, "medal_headshot"));
    event->generic1 = 8; event->modelindex2 = 17; cg_announcer.integer = 2;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardCount[2] == 1);
    CHECK(!strcmp(lastVoice, "sound/vo_evil/first_frag.ogg") && !strcmp(lastMedal, "medal_firstfrag"));
    event->generic1 = 0; cg_announcer.integer = 3;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardCount[3] == 17);
    CHECK(!strncmp(lastVoice, "sound/vo_female/combokill", 23));
    CHECK(!strcmp(lastMedal, "medal_combokill"));
    missingAlternate = 1; event->generic1 = 7;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/quad_god.ogg"));
    CHECK(cg.rewardStack == 4);
    event->clientNum = 6;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardStack == 4);
    event->clientNum = snapshot.ps.clientNum; event->generic1 = 10;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardStack == 4);
    event->generic1 = -1;
    CHECK(CG_QL_EntityEvent(&cent) && cg.rewardStack == 4);
    for (i = 0; i < 32; ++i) CG_PushReward(80, 81, 2);
    CHECK(cg.rewardStack == MAX_REWARDSTACK - 1 && cg.rewardSound[MAX_REWARDSTACK - 1] == 80);
    CG_PushReward(90, 91, 5);
    CHECK(cg.rewardSound[MAX_REWARDSTACK - 1] == 80);
    event->event = EV_INFECTED;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/infected.ogg"));
    event->event = EV_NEW_HIGH_SCORE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/new_high_score.ogg"));
    event->event = EV_DAMAGEPLUM | EV_EVENT_BIT1;
    CHECK(CG_QL_EntityEvent(&cent) && damagePlums == 1);
    event->event = EV_SHOTGUN_KILL;
    event->generic1 = 14; event->otherEntityNum2 = 7;
    VectorSet(event->pos.trBase, 32, 48, 64);
    cg_impactSparks.integer = 1; cg_impactSparksSize.integer = 8;
    cg_impactSparksLifetime.integer = 250; cg_impactSparksVelocity.value = 128;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 4 && sparks == 2);
    cg_impactSparks.integer = 0;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 8 && sparks == 2);
    event->generic1 = 4;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 8);
    event->generic1 = -1;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 8);
    event->generic1 = 256;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 8);
    event->generic1 = 255;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 110);
    event->otherEntityNum2 = MAX_GENTITIES;
    CHECK(CG_QL_EntityEvent(&cent) && bleeds == 110);
    cg_announcer.integer = 1; missingAlternate = 0;
    snapshot.ps.clientNum = 7;
    cg.time = 61050; cgs.levelStartTime = 1000; cgs.timelimit = 1; cgs.overtimeSeconds = 120;
    event->event = EV_OVERTIME;
    CHECK(CG_QL_EntityEvent(&cent) && lastEntity == 7 && lastSound == 11 && lastChannel == CHAN_AUTO);
    CHECK(!strcmp(centerMessage,"Overtime! 120 seconds added") && !strcmp(lastVoice,"sound/vo/overtime.ogg"));
    bufferedSound = 0; cg.time = 181050;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound); /* later overtime, klaxon only */
    cgs.overtimeSeconds = 0; CHECK(CG_QL_EntityEvent(&cent));
    CHECK(!strcmp(centerMessage,"Overtime! 0 seconds added")); /* no modulo by zero */
    cg.snap = NULL; i = plays; CHECK(CG_QL_EntityEvent(&cent) && plays == i); cg.snap = &snapshot;
    event->event = EV_GAMEOVER; cg_buzzerSound.integer = 1;
    cg.rewardStack = 4; cg.rewardTime = 123; cg.rewardCount[3] = 5;
    cg.soundBufferIn = 4; cg.soundBuffer[1] = 77; cg.soundTime = cg.time + 500;
    cgs.gametype = GT_FFA; snapshot.ps.persistant[PERS_TEAM] = TEAM_FREE;
    snapshot.ps.persistant[PERS_RANK] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && cg.matchEnded && buzzers == 1);
    CHECK(!cg.rewardStack && !cg.rewardTime && !cg.rewardCount[3] && !cg.soundBuffer[1] && !cg.soundBufferIn);
    CHECK(cg.soundTime == cg.time + 500 && !strcmp(music,"music/win") && !strcmp(lastVoice,"sound/vo/you_win.ogg"));
    cg.matchEnded = qfalse; snapshot.ps.persistant[PERS_RANK] = RANK_TIED_FLAG;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/loss") && !strcmp(lastVoice,"sound/vo/you_lose.ogg"));
    cg.matchEnded = qfalse; snapshot.ps.persistant[PERS_RANK] = 1; cg_buzzerSound.integer = 0;
    cg.soundTime = cg.time - 1;
    CHECK(CG_QL_EntityEvent(&cent) && buzzers == 2 && !cg.soundTime && !strcmp(music,"music/loss"));
    cg.matchEnded = qfalse; snapshot.ps.persistant[PERS_TEAM] = TEAM_SPECTATOR; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound && !strcmp(music,"music/win"));
    cg.matchEnded = qfalse; cgs.gametype = GT_TEAM;
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; cgs.clientinfo[7].team = TEAM_RED;
    cgs.scores1 = cgs.scores2 = 10; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound && !strcmp(music,"music/win"));
    cg.matchEnded = qfalse; cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/loss"));
    cg.matchEnded = qfalse; cgs.scores2 = 11;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/win"));
    cg.matchEnded = qfalse; cgs.gametype = GT_RED_ROVER; snapshot.ps.persistant[PERS_RANK] = 1;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/loss") && !strcmp(lastVoice,"sound/vo/you_lose.ogg"));
    event->event = EV_GLOBAL_TEAM_SOUND; event->eventParm = GTS_REDTEAM_WINS;
    cg.matchEnded = qfalse; cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/win") && !strcmp(lastVoice,"sound/vo/red_wins.ogg"));
    cg.matchEnded = qfalse; event->eventParm = GTS_BLUETEAM_WINS;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(music,"music/loss") && !strcmp(lastVoice,"sound/vo/blue_wins.ogg"));
    cg.matchEnded = qfalse; cgs.clientinfo[7].team = TEAM_SPECTATOR; i = musicStarts;
    CHECK(CG_QL_EntityEvent(&cent) && musicStarts == i); /* local spectator, even if following */
    cg.matchEnded = qfalse; event->eventParm = GTS_REDTEAM_WINS_ROUND;
    CHECK(CG_QL_EntityEvent(&cent) && !cg.matchEnded && !strcmp(centerMessage,"Red wins the round"));
    CHECK(!strcmp(lastVoice,"sound/vo/red_wins_round.ogg"));
    event->eventParm = GTS_BLUETEAM_WINS_ROUND;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Blue wins the round"));
    CHECK(!strcmp(lastVoice,"sound/vo/blue_wins_round.ogg"));
    event->eventParm = GTS_LAST_STANDING; event->modelindex2 = TEAM_RED;
    cg_announcerLastStandingVO.integer = 1; cgs.clientinfo[7].team = TEAM_RED;
    snapshot.ps.clientNum = 8; cgs.clientinfo[8].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice,"sound/vo/last_standing.ogg"));
    bufferedSound = 0; event->modelindex2 = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound); /* Local, not followed team. */
    event->modelindex2 = TEAM_RED; cg_announcerLastStandingVO.integer = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cg_announcerLastStandingVO.integer = 1; cg.clientNum = -1;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound); cg.clientNum = 7;
    cgs.clientinfo[7].team = TEAM_SPECTATOR; event->modelindex2 = TEAM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    event->eventParm = GTS_ROUND_DRAW; cgs.gametype = GT_CLAN_ARENA;
    cg.rewardStack = 2; cg.soundBuffer[0] = 42; cg.soundTime = cg.time+200;
    i = musicStarts;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Draw"));
    CHECK(!strcmp(lastVoice,"sound/vo/round_draw.ogg") && !cg.matchEnded);
    CHECK(!cg.rewardStack && !cg.soundBuffer[0] && cg.soundTime == cg.time+200 && musicStarts == i);
    event->eventParm = 20;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Over"));
    CHECK(!strcmp(lastVoice,"sound/vo/round_over.ogg"));
    cgs.gametype = GT_RED_ROVER; winnerList = "7";
    cgs.clientinfo[7].infoValid = cgs.clientinfo[8].infoValid = qtrue;
    strcpy(cgs.clientinfo[7].name,"^2Local"); strcpy(cgs.clientinfo[8].name,"Other");
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Winner:\n^2Local\n"));
    winnerList = " 7\t8 \n";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Winners:\n^2Local\nOther\n"));
    settingsMask = "67108864";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^1Last Survivors:^7\n^2Local\nOther\n"));
    CHECK(!strcmp(lastVoice,"sound/vo/round_over.ogg"));
    winnerList = "8 7";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^7Other\n^2Local\n Wins!^7\n"));
    CHECK(!strcmp(lastVoice,"sound/vo/you_win.ogg"));
    event->eventParm = GTS_ROUND_DRAW; winnerList = "7 8";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^1Last Survivors:^7\n^2Local\nOther\n"));
    CHECK(!strcmp(lastVoice,"sound/vo/you_win.ogg"));
    winnerList = "8";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^1Last Survivor:^7\nOther\n"));
    CHECK(!strcmp(lastVoice,"sound/vo/round_over.ogg"));
    { /* Invalid lists do not index client memory or display partial winners. */
        const char *bad[] = {"", "7 7", "-1", "64", "999999999999999999999", "7 nope", "7x", "7 8.5"};
        for (i = 0; i < ARRAY_LEN(bad); ++i) {
            winnerList = bad[i];
            CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Draw"));
            CHECK(!strcmp(lastVoice,"sound/vo/round_draw.ogg"));
        }
    }
    winnerList = "7"; settingsMask = "4294967296"; event->eventParm = 20;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Winner:\n^2Local\n"));
    settingsMask = "67108864 garbage";
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"Round Winner:\n^2Local\n"));
    settingsMask = "67108864"; winnerList = "63"; cgs.clientinfo[63].infoValid = qtrue;
    strcpy(cgs.clientinfo[63].name,"Last slot");
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^1Last Survivor:^7\nLast slot\n"));
    cgs.clientinfo[63].infoValid = qfalse;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage,"^1Last Survivor:^7\n"));
    /* Domination point announcements (retail global team sound 23). */
    cgs.gametype = GT_DOMINATION; registrations = 0; CG_RegisterQLEventSounds();
    CHECK(registrations == 20 && !strcmp(registered[12], "sound/teamplay/flagreturn_opponent.ogg"));
    CHECK(!strcmp(registered[16], "sound/teamplay/flagreturn_yourteam.ogg"));
    CHECK(!strcmp(registered[19], "sound/teamplay/flagcapture_opponent.ogg"));
    CHECK(!strcmp(registered[13], "sound/teamplay/flagcapture_yourteam.ogg"));
    CHECK(!strcmp(registered[14], "sound/teamplay/flagcapture_opponent.ogg"));
    event->eventParm = 23; event->modelindex2 = TEAM_RED; event->powerups = 2;
    cg.clientNum = 7; cgs.clientinfo[7].team = TEAM_RED; cg_announcerLeadsVO.integer = 1;
    bufferedSound = 0; centerMessage[0] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Red captured point B!"));
    CHECK(bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/b_captured.ogg"));
    event->modelindex2 = TEAM_BLUE; event->powerups = 5; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Blue captured point E!"));
    CHECK(bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/e_lost.ogg"));
    cg_announcerLeadsVO.integer = 0; bufferedSound = 0; centerMessage[0] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Blue captured point E!") && !bufferedSound);
    cg_announcerLeadsVO.integer = 1; event->powerups = 0; centerMessage[0] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !centerMessage[0] && bufferedSound == 15); /* warmup capture */
    cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 14);
    cgs.clientinfo[7].team = TEAM_SPECTATOR; event->powerups = 1; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Blue captured point A!") && !bufferedSound);
    cgs.clientinfo[7].team = TEAM_RED; event->modelindex2 = TEAM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Blue captured point A!")); /* not red */
    event->modelindex2 = TEAM_RED; event->powerups = 6; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Red captured point ?!") && !bufferedSound);
    event->powerups = -1;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Red captured point ?!") && !bufferedSound);
    event->powerups = 3; cg.clientNum = -1;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Red captured point C!") && !bufferedSound);
    cg.clientNum = 7; event->modelindex2 = TEAM_FREE; centerMessage[0] = 0; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 13 && !centerMessage[0] && !bufferedSound);
    /* Survivor cue (24): registered for round modes, sent to one team. */
    cgs.gametype = GT_CLAN_ARENA; registrations = 0; CG_RegisterQLEventSounds();
    CHECK(registrations == 16 && !strcmp(registered[12], "sound/feedback/survivor_01.ogg"));
    event->eventParm = 24; event->modelindex2 = TEAM_BLUE; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !localSound);
    cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 13);
    event->eventParm = 23; event->modelindex2 = TEAM_FREE; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !localSound); /* outside domination: unregistered */
    cgs.gametype = GT_FFA; registrations = 0; CG_RegisterQLEventSounds();
    event->eventParm = 24; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && registrations == 13 && !localSound);
    /* Lead changes (10/11) use the QL lead voices, not Quake III's. */
    event->eventParm = GTS_REDTEAM_TOOK_LEAD; cgs.gametype = GT_DOMINATION;
    cgs.clientinfo[7].team = TEAM_RED; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/lead_taken.ogg"));
    cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/lead_lost.ogg"));
    cgs.clientinfo[7].team = TEAM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/red_leads.ogg"));
    event->eventParm = GTS_BLUETEAM_TOOK_LEAD;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/blue_leads.ogg"));
    cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/lead_taken.ogg"));
    cg_announcerLeadsVO.integer = 0; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cg_announcerLeadsVO.integer = 1; cgs.gametype = GT_ATTACK_DEFEND;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cgs.gametype = GT_FFA;
    /* Retail team sounds 0..9, 12, 13 (0x1001b349), local team = client 7. */
    cgs.gametype = GT_CTF; registrations = 0; CG_RegisterQLEventSounds();
    CHECK(registrations == 17 && !strcmp(registered[13], "sound/teamplay/flagreturn_yourteam.ogg"));
    CHECK(!strcmp(registered[15], "sound/teamplay/flagcapture_yourteam.ogg"));
    cg.clientNum = 7; snapshot.ps.clientNum = 8; memset(snapshot.ps.powerups, 0, sizeof(snapshot.ps.powerups));
    event->eventParm = GTS_RED_CAPTURE; cgs.clientinfo[7].team = TEAM_RED; localSound = bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 16 && !bufferedSound);
    cgs.clientinfo[7].team = TEAM_BLUE; CHECK(CG_QL_EntityEvent(&cent) && localSound == 17);
    cgs.clientinfo[7].team = TEAM_SPECTATOR; CHECK(CG_QL_EntityEvent(&cent) && localSound == 17);
    event->eventParm = GTS_BLUE_CAPTURE; CHECK(CG_QL_EntityEvent(&cent) && localSound == 17);
    cgs.clientinfo[7].team = TEAM_BLUE; CHECK(CG_QL_EntityEvent(&cent) && localSound == 16);
    /* RED_RETURN returns the blue flag. */
    event->eventParm = GTS_RED_RETURN; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 15 && !strcmp(lastVoice, "sound/vo/your_flag_returned.ogg"));
    cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 14 && !strcmp(lastVoice, "sound/vo/enemy_flag_returned.ogg"));
    cgs.clientinfo[7].team = TEAM_SPECTATOR; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !localSound && !strcmp(lastVoice, "sound/vo/blue_flag_returned.ogg"));
    event->eventParm = GTS_BLUE_RETURN;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/red_flag_returned.ogg"));
    cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 15 && !strcmp(lastVoice, "sound/vo/your_flag_returned.ogg"));
    /* RED_TAKEN: red took the blue flag. */
    event->eventParm = GTS_RED_TAKEN; event->otherEntityNum = 5; cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/your_flag_taken.ogg"));
    cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/enemy_flag_taken.ogg"));
    cgs.clientinfo[7].team = TEAM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/blue_flag_taken.ogg"));
    event->eventParm = GTS_BLUE_TAKEN;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/red_flag_taken.ogg"));
    snapshot.ps.powerups[PW_REDFLAG] = 1; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound); /* the snapshot carrier */
    event->eventParm = GTS_RED_TAKEN; cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 100); /* checks the blue flag */
    snapshot.ps.powerups[PW_REDFLAG] = 0; snapshot.ps.powerups[PW_NEUTRALFLAG] = 1; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    snapshot.ps.powerups[PW_NEUTRALFLAG] = 0;
    /* cg_followPowerup: free spectators only; flags never add " pw". */
    commandsSent = 0; cg_followPowerup.integer = 1; cgs.clientinfo[7].team = TEAM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && !commandsSent); /* pm_type is not spectator */
    snapshot.ps.pm_type = PM_SPECTATOR;
    CHECK(CG_QL_EntityEvent(&cent) && commandsSent == 1 && !strcmp(sentCommand, "follow 5"));
    cg_followPowerup.integer = 2;
    CHECK(CG_QL_EntityEvent(&cent) && commandsSent == 2 && !strcmp(sentCommand, "follow 5"));
    event->otherEntityNum = MAX_CLIENTS; CHECK(CG_QL_EntityEvent(&cent) && commandsSent == 2);
    event->otherEntityNum = 5; cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && commandsSent == 2); /* a player, not a spectator */
    /* Powerup pickups (EV_GLOBAL_ITEM_PICKUP helper). */
    cgs.clientinfo[7].team = TEAM_SPECTATOR; cgs.settingsMask = 0; bufferedSound = 0;
    CG_GlobalItemPickupAnnouncement(event, IT_POWERUP, PW_QUAD);
    CHECK(bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/quad_damage.ogg"));
    CHECK(commandsSent == 3 && !strcmp(sentCommand, "follow 5 pw"));
    CG_GlobalItemPickupAnnouncement(event, IT_POWERUP, PW_REGEN);
    CHECK(!strcmp(lastVoice, "sound/vo/regeneration.ogg"));
    CG_GlobalItemPickupAnnouncement(event, IT_POWERUP, PW_BATTLESUIT);
    CHECK(!strcmp(lastVoice, "sound/vo/battlesuit.ogg"));
    cg_followPowerup.integer = 0; bufferedSound = 0;
    CG_GlobalItemPickupAnnouncement(event, IT_TEAM, PW_REDFLAG);
    CHECK(!bufferedSound && commandsSent == 5);
    cgs.settingsMask = CG_SETTING_QUAD_HOG;
    CG_GlobalItemPickupAnnouncement(event, IT_POWERUP, PW_QUAD);
    CHECK(!bufferedSound); /* Quad Hog announces through its own notice */
    cgs.settingsMask = 0; snapshot.ps.pm_type = PM_NORMAL;
    /* Rune pickups (EV_ITEM_PICKUP, 0x1001a557). */
    bufferedSound = 0; CG_PersistentPowerupAnnouncement(PP_SCOUT);
    CHECK(bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/scout.ogg"));
    CG_PersistentPowerupAnnouncement(PP_DOUBLER); CHECK(!strcmp(lastVoice, "sound/vo/damage.ogg"));
    CG_PersistentPowerupAnnouncement(PP_ARMORREGEN); CHECK(!strcmp(lastVoice, "sound/vo/armor_regen.ogg"));
    bufferedSound = 0; CG_PersistentPowerupAnnouncement(PP_NONE); CG_PersistentPowerupAnnouncement(5);
    CHECK(!bufferedSound);
    /* One Flag: players get return effects, spectators no voice. */
    cgs.gametype = GT_1FCTF; registrations = 0; CG_RegisterQLEventSounds();
    event->eventParm = GTS_RED_TAKEN; cgs.clientinfo[7].team = TEAM_BLUE; localSound = bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 14 && !bufferedSound);
    cgs.clientinfo[7].team = TEAM_RED; CHECK(CG_QL_EntityEvent(&cent) && localSound == 15 && !bufferedSound);
    cgs.clientinfo[7].team = TEAM_SPECTATOR; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !localSound && !bufferedSound);
    event->eventParm = GTS_RED_RETURN; cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 15 && !bufferedSound); /* no CTF voice */
    /* Overload base attack: own obelisk, Overload only. */
    event->eventParm = GTS_REDOBELISK_ATTACKED; cgs.clientinfo[7].team = TEAM_RED;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cgs.gametype = GT_OBELISK;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/base_attack.ogg"));
    event->eventParm = GTS_BLUEOBELISK_ATTACKED; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    /* Scores: Freeze centers the team; A&D is silent; FFA has no voices. */
    cgs.gametype = GT_FREEZE; event->eventParm = GTS_REDTEAM_SCORED; centerMessage[0] = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Red Scores"));
    CHECK(!strcmp(lastVoice, "sound/vo/your_team_scores.ogg"));
    cgs.clientinfo[7].team = TEAM_BLUE;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(lastVoice, "sound/vo/enemy_scores.ogg"));
    cgs.clientinfo[7].team = TEAM_SPECTATOR; event->eventParm = GTS_BLUETEAM_SCORED;
    CHECK(CG_QL_EntityEvent(&cent) && !strcmp(centerMessage, "Blue Scores"));
    CHECK(!strcmp(lastVoice, "sound/vo/blue_scores.ogg"));
    cgs.gametype = GT_TEAM; centerMessage[0] = 0; event->eventParm = GTS_REDTEAM_SCORED;
    CHECK(CG_QL_EntityEvent(&cent) && !centerMessage[0] && !strcmp(lastVoice, "sound/vo/red_scores.ogg"));
    cgs.gametype = GT_ATTACK_DEFEND; bufferedSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cgs.gametype = GT_FFA; CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    /* Ties: cg_announcerTiesVO, not in A&D. */
    event->eventParm = GTS_TEAMS_ARE_TIED; cgs.gametype = GT_TEAM; cg_announcerTiesVO.integer = 1;
    CHECK(CG_QL_EntityEvent(&cent) && bufferedSound == 100 && !strcmp(lastVoice, "sound/vo/teams_tied.ogg"));
    cg_announcerTiesVO.integer = 0; bufferedSound = 0; CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    cg_announcerTiesVO.integer = 1; cgs.gametype = GT_ATTACK_DEFEND;
    CHECK(CG_QL_EntityEvent(&cent) && !bufferedSound);
    /* Kamikaze: far explosion on the local-sound channel. */
    cgs.gametype = GT_FFA; registrations = 0; CG_RegisterQLEventSounds();
    event->eventParm = GTS_KAMIKAZE; localSound = 0;
    CHECK(CG_QL_EntityEvent(&cent) && localSound == 13);
    /* Retail has no case 21 or 22: the inherited dispatcher keeps them. */
    event->eventParm = 21; CHECK(!CG_QL_EntityEvent(&cent));
    event->event = EV_POI | EV_EVENT_BIT2;
    CHECK(CG_QL_EntityEvent(&cent) && pois == 1);
    event->event = EV_THAW_PLAYER;
    CHECK(CG_QL_EntityEvent(&cent) && thaws == 1);
    event->event = EV_MISSILE_MISS_DMGTHROUGH; event->weapon = WP_ROCKET_LAUNCHER; event->eventParm = 5;
    CHECK(CG_QL_EntityEvent(&cent) && throughImpacts == 1);
    event->eventParm = -1; CHECK(CG_QL_EntityEvent(&cent) && throughImpacts == 1);
    event->eventParm = NUMVERTEXNORMALS; CHECK(CG_QL_EntityEvent(&cent) && throughImpacts == 1);
    for (i = EV_RACE_START; i <= EV_RACE_FINISH; ++i) {
        event->event = i | EV_EVENT_BIT1;
        CHECK(CG_QL_EntityEvent(&cent));
    }
    CHECK(raceEvents == 3);
    puts("QL events: audio, local/drop selection, timers, damage and match/round/race results, team/domination/survivor sounds, powerup follow and POI dispatch OK");
    return 0;
}
