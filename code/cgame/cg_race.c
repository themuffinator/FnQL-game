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
#include "../../docs/contract/retail_menudef.h"
#include <stdint.h>
#include <limits.h>

static qboolean RaceInteger(const char *text, int *value) {
    uint32_t magnitude = 0, limit;
    qboolean negative = *text == '-';
    if (*text == '-' || *text == '+') ++text;
    if (!*text) return qfalse;
    limit = negative ? (uint32_t)INT_MAX+1u : INT_MAX;
    while (*text) {
        unsigned digit = (unsigned)(*text++-'0');
        if (digit > 9 || magnitude > (limit-digit)/10u) return qfalse;
        magnitude = magnitude*10u+digit;
    }
    *value = negative && magnitude == (uint32_t)INT_MAX+1u ? INT_MIN :
             negative ? -(int)magnitude : (int)magnitude;
    return qtrue;
}

/* Retail 10001320: signed m:ss.mmm. Integer decomposition preserves exact
 * milliseconds at long durations and handles INT_MIN without signed abs. */
static void RaceTime(char *text, int size, int64_t milliseconds) {
    uint64_t magnitude = milliseconds < 0 ? (uint64_t)(-milliseconds) : (uint64_t)milliseconds;
    /* Q_vsprintf supports signed decimal only. Each decomposed component
     * fits int even when the input is a difference of two int timestamps. */
    Com_sprintf(text,size,"%s%i:%02i.%03i",milliseconds < 0 ? "-" : "",
                (int)(magnitude/60000), (int)(magnitude/1000%60), (int)(magnitude%1000));
}

void CG_ResetRace(qboolean clearRecords, qboolean notifyServer) {
    int last = cg.race.lastTime, best = cg.race.bestTime;
    memset(&cg.race,0,sizeof(cg.race));
    cg.race.nextPoint = cg.race.followingPoint = -1;
    if (!clearRecords) { cg.race.lastTime = last; cg.race.bestTime = best; }
    /* Retail 1002a4a0 asks the server to clear its active run as well. Demo
     * playback must stay local; no request is fabricated for a non-race map. */
    if (notifyServer && cgs.gametype == GT_RACE && !cg.demoPlayback)
        trap_SendClientCommand("raceinit");
}

void CG_RaceLevelTimeChanged(int oldTime, int newTime) {
    if (cgs.gametype == GT_RACE && cg.race.active) {
        /* Retail 10049b91: timeout compensation shifts the run's start. */
        int64_t shifted = (int64_t)cg.race.startedAt+newTime-oldTime;
        if (shifted < INT_MIN || shifted > INT_MAX) CG_ResetRace(qfalse,qfalse);
        else cg.race.startedAt = (int)shifted;
    }
}

qboolean CG_ParseRaceCommand(const char *command) {
    int values[6], i;
    char text[MAX_STRING_CHARS];
    if (!strcmp(command,"race_init")) {
        if (trap_Argc() == 1 && cgs.gametype == GT_RACE) CG_ResetRace(qtrue,qtrue);
        return qtrue;
    }
    if (strcmp(command,"race_info")) return qfalse;
    if (cgs.gametype != GT_RACE) return qtrue;
    if (trap_Argc() != 7) goto malformed;
    for (i = 0; i < 6; ++i) {
        trap_Argv(i+1,text,sizeof(text));
        if (!RaceInteger(text,&values[i])) goto malformed;
    }
    if (values[2] < 0 || values[3] < 0 || values[3] > 64 ||
        values[4] < -1 || values[4] >= MAX_GENTITIES ||
        values[5] < -1 || values[5] >= MAX_GENTITIES) goto malformed;
    /* 10048820 updates these six fields only. Personal best, split delta
     * and event checkpoint count survive follow/reliable-info updates. */
    cg.race.active = values[0] != 0;
    cg.race.startedAt = values[1]; cg.race.lastTime = values[2];
    cg.race.reportedSplits = values[3];
    cg.race.nextPoint = values[4]; cg.race.followingPoint = values[5];
    return qtrue;
malformed:
    CG_Printf("Ignored malformed race_info command\n");
    return qtrue;
}

static void RaceBeep(void) {
    static const char *sounds[] = {
        "sound/feedback/impact1.ogg", "sound/feedback/impact2.ogg",
        "sound/feedback/impact3.ogg", "sound/feedback/impact4.ogg",
        "sound/feedback/impact5.ogg", "sound/feedback/impact6.ogg",
        "sound/world/bell_01.ogg", "sound/misc/chaching.ogg"
    };
    int choice = cg_raceBeep.integer;
    if (choice >= 1 && choice <= ARRAY_LEN(sounds))
        trap_S_StartLocalSound(trap_S_RegisterSound(sounds[choice-1],qfalse),CHAN_LOCAL_SOUND);
}

static void RaceSplitMessage(int64_t delta, const char *remaining) {
    char time[40], message[96];
    RaceTime(time,sizeof(time),delta);
    Com_sprintf(message,sizeof(message),"%s%s\n%s",delta < 0 ? "^2" : delta > 0 ? "^1" : "^7",time,remaining);
    CG_CenterPrint(message,320,BIGCHAR_WIDTH*3/5);
}

void CG_RaceEvent(const entityState_t *event) {
    int kind = event->event & ~EV_EVENT_BITS;
    char remaining[32] = "", message[64];
    int count;
    /* 1001abc4/1001ad18/1001aeda select the snapshot client: a spectator
     * receives the followed runner's events, not their own client slot. */
    if (!cg.snap || cgs.gametype != GT_RACE || event->clientNum < 0 || event->clientNum >= MAX_CLIENTS ||
        event->clientNum != cg.snap->ps.clientNum) return;
    if (kind == EV_RACE_FINISH) {
        if (event->generic1 && event->time < 0) return;
        CG_ResetRace(qfalse,qtrue);
        if (event->generic1) {
            cg.race.lastTime = event->time;
            if (cg.race.bestTime) RaceSplitMessage((int64_t)event->time-cg.race.bestTime,"");
            if (!cg.race.bestTime || event->time < cg.race.bestTime) cg.race.bestTime = event->time;
        }
        trap_S_StartLocalSound(trap_S_RegisterSound("sound/world/klaxon1.ogg",qfalse),CHAN_LOCAL_SOUND);
        return;
    }
    if (kind != EV_RACE_START && kind != EV_RACE_CHECKPOINT) return;
    /* Network -1 sentinels may arrive as the all-ones unsigned field value. */
    if (event->otherEntityNum2 < -1 || event->otherEntityNum2 >= MAX_GENTITIES ||
        event->powerups < -1 || (event->powerups >= MAX_GENTITIES && event->powerups != 65535)) return;
    cg.race.nextPoint = event->otherEntityNum2;
    cg.race.followingPoint = event->powerups == 65535 ? -1 : event->powerups;
    if (kind == EV_RACE_START) {
        cg.race.active = qtrue; cg.race.startedAt = event->time;
        cg.race.hasSplit = qfalse; cg.race.splitDelta = 0; cg.race.pointsPassed = 1;
    } else {
        if (cg.race.pointsPassed < 64) ++cg.race.pointsPassed;
        cg.race.hasSplit = event->generic1 != 0;
        cg.race.splitDelta = cg.race.hasSplit ? event->time : 0;
        if (cg_drawCheckpointRemaining.integer && RaceInteger(CG_ConfigString(CS_RACE_RECORDS),&count) &&
            count >= cg.race.pointsPassed && count-cg.race.pointsPassed <= 3)
            Com_sprintf(remaining,sizeof(remaining),"^7%i remaining",count-cg.race.pointsPassed);
        if (cg.race.hasSplit) RaceSplitMessage(cg.race.splitDelta,remaining);
        else {
            Com_sprintf(message,sizeof(message),"Checkpoint\n%s",remaining);
            CG_CenterPrint(message,320,BIGCHAR_WIDTH*3/5);
        }
    }
    RaceBeep();
}

void CG_RaceRespawnCommand(void) {
    /* Retail 10007e70 forwards kill unchanged and retires its binding hint
     * for this cgame session. Race initialization must not restore it. */
    cg.raceRespawnUsed = qtrue;
    trap_SendClientCommand("kill");
}

static float RaceTextX(const char *text, float x, float scale, int align) {
    if (align == 1) x -= CG_Text_Width(text,scale,0)*0.5f;
    else if (align == 2) x -= CG_Text_Width(text,scale,0);
    return x;
}

static void RaceRespawnHint(float x, float y, float scale, int align) {
    char key[32], hint[96];
    vec4_t color = {1,1,1,.5f};
    if (cg.demoPlayback || (cg.snap->ps.eFlags & EF_SPECTATOR)) return;
    trap_Key_KeynumToStringBuf(trap_Key_GetKey("kill"),key,sizeof(key));
    if (!key[0] || !Q_stricmp(key,"<KEY NOT FOUND>"))
        Q_strncpyz(hint,"Bind 'kill' to respawn",sizeof(hint));
    else {
        if (cg.raceRespawnUsed) return;
        Com_sprintf(hint,sizeof(hint),"Press %s to respawn.",key);
    }
    /* 1002f329/1002f3b0 aligns with the menu scale, then 1002f400 paints
     * at scale .18, offset (19,27), with half-opacity white. */
    CG_Text_Paint(RaceTextX(hint,x,scale,align)+19,y+27,.18f,color,hint,0,0,3);
}

void CG_DrawRaceHud(int ownerDraw, float x, float y, float scale, vec4_t color, int align) {
    char time[40];
    const char *text;
    vec4_t ink = {1,1,1,1};
    int pause = 0;
    int64_t elapsed;
    if (cgs.gametype != GT_RACE || !cg.snap) return;
    (void)color;
    /* Retail 1002f1f0 uses yellow labels and white times even when the menu
     * leaves forecolor unset (100761e4/10076214). */
    if (ownerDraw == CG_RACE_STATUS) {
        RaceRespawnHint(x,y,scale,align);
        ink[2] = 0;
        text = cg.race.active ? "CURRENT RUN" : "LAST TIME";
    }
    else if (ownerDraw == CG_RACE_TIMES) {
        RaceInteger(CG_ConfigString(CS_TIMEOUT_START_TIME),&pause);
        elapsed = cg.race.active ? (int64_t)(pause ? pause : cg.time)-cg.race.startedAt : cg.race.lastTime;
        RaceTime(time,sizeof(time),elapsed < 0 ? 0 : elapsed); text = time;
    } else return;
    CG_Text_Paint(RaceTextX(text,x,scale,align),y,scale,ink,text,0,0,3);
}
