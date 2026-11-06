/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>

/* Private presentation state, populated only from authoritative configstrings.
 * CS_MATCH_STATE and CS_WARMUP share retail's warmup clock but have different
 * round-number semantics (10048c80/10048d10). No shared ABI layout changes. */
static struct {
    int warmup, round, turn, levelStart, timeoutStart, timeoutEnd;
    int limitMinutes, roundSeconds, overtimeSeconds;
    int64_t roundStart;
    int lastFrame;
    qboolean frameSeen;
    int64_t soundTime;
} clocks;

static struct {
    char text[1024];
    int time, y, lines;
    float scale;
    qboolean active;
} notice;

void CG_ClearMatchNotice(void) { memset(&notice,0,sizeof(notice)); }

static void CenterNotice(const char *text,int y,float scale) {
    const char *cursor;
    CG_ClearMatchNotice();
    Q_strncpyz(notice.text,text,sizeof(notice.text));
    notice.time=cg.time;
    notice.y=y+(cgs.gametype == GT_RACE ? 10 : 0);
    notice.scale=scale;
    notice.lines=1;
    for (cursor=notice.text; *cursor; ++cursor) if (*cursor=='\n') ++notice.lines;
    notice.active=qtrue;
    cg.centerPrintTime=0;
}

/* Local retail center prints (0x1000be10 callers) share the painter. */
void CG_CenterPrintNative(const char *text, int y, float scale) {
    if (text) CenterNotice(text, y, scale);
}

/* Shipping dispatcher 1004af3d-1004b035: pcp always uses y=90 and echoes
 * its argument verbatim. cp uses y=144 during warmup, otherwise 90. */
qboolean CG_MatchNoticeCommand(const char *command) {
    qboolean print;
    const char *text;
    if (!command || (strcmp(command,"cp") && strcmp(command,"pcp"))) return qfalse;
    print=!strcmp(command,"pcp");
    text=CG_Argv(1);
    CenterNotice(text,!print && clocks.warmup ? 144 : 90,.4f);
    if (print) CG_Printf("%s",text);
    return qtrue;
}

qboolean CG_DrawMatchNotice(void) {
    const char *cursor;
    double duration=(double)cg_centertime.value*1000;
    int64_t age=(int64_t)cg.time-notice.time;
    int64_t y;
    vec4_t color={1,1,1,1};
    if (!notice.active) return qfalse;
    if (!(duration > 0 && duration <= INT_MAX) || age < 0 || age >= duration) {
        CG_ClearMatchNotice();
        return qfalse;
    }
    if (duration-age < 200) color[3]=(float)((duration-age)/200);
    /* Native center painter 1000beb0: 50 bytes per explicit line, drop the
     * rest of an overlong line, ascent plus six pixels between baselines. */
    y=(int)(notice.y-notice.scale*16*notice.lines);
    CG_SetTextFont(0);
    cursor=notice.text;
    for (;;) {
        char line[51];
        int length=0, width;
        while (length<50 && cursor[length] && cursor[length]!='\n') ++length;
        memcpy(line,cursor,length); line[length]=0;
        width=CG_Text_Width(line,notice.scale,0);
        y+=CG_Text_Height(line,notice.scale,0);
        CG_Text_Paint((640-width)/2,(float)y,notice.scale,color,line,0,0,ITEM_TEXTSTYLE_SHADOWEDMORE);
        while (*cursor && *cursor!='\n') ++cursor;
        if (!*cursor) break;
        ++cursor; y+=6;
    }
    return qtrue;
}

static qboolean RoundMode(void);

void CG_AnnounceMatchStart(void) {
    const char *text="FIGHT!", *voice="fight";
    int team=cg.snap ? cg.snap->ps.persistant[PERS_TEAM] : TEAM_SPECTATOR;
    /* Shipping 1000ea40 is shared by round countdown and map_restart. */
    if (cgs.gametype == GT_ATTACK_DEFEND && (team == TEAM_RED || team == TEAM_BLUE)) {
        qboolean attacking=(team == TEAM_RED) == (clocks.turn == 0);
        text=attacking ? "ATTACK THE FLAG!" : "DEFEND THE FLAG!";
        voice=attacking ? "attack_the_flag" : "defend_the_flag";
    } else if (cgs.gametype == GT_RED_ROVER) {
        if (clocks.round > 0) trap_S_StartLocalSound(
            trap_S_RegisterSound("sound/items/kamikazerespawn.wav",qtrue),CHAN_LOCAL_SOUND);
        if (cg.snap && (cg.snap->ps.pm_flags & PMF_INFECTED)) { text="BITE!"; voice="bite"; }
    }
    CenterNotice(text,120,.5f);
    CG_ClearMatchAnnouncements();
    trap_S_StartLocalSoundVolume(CG_QLVoice(voice),CHAN_ANNOUNCER,cg_announcerVolume.value);
}

static void TimeoutCount(int64_t seconds) {
    const char *voice=NULL;
    switch (seconds) {
    case 5: voice="prepare_to_fight"; break;
    case 3: voice="three"; break;
    case 2: voice="two"; break;
    case 1: voice="one"; break;
    case 0:
        if (RoundMode()) CG_AnnounceMatchStart();
        return;
    default: break;
    }
    if (voice) trap_S_StartLocalSoundVolume(CG_QLVoice(voice),CHAN_ANNOUNCER,cg_announcerVolume.value);
}

void CG_DrawMatchPause(void) {
    char text[96];
    int64_t remaining=-1;
    vec4_t color={1,1,1,1};
    /* 10011173/1000ef30: visible even while following or showing scores.
     * No banner on the final start-cleared update. Timed pauses show the
     * countdown at every nonzero second, not just the final five. */
    if (!clocks.timeoutStart) return;
    if (clocks.soundTime < clocks.timeoutStart) {
        clocks.soundTime=clocks.timeoutStart;
        trap_S_StartLocalSound(trap_S_RegisterSound("sound/world/klaxon1.ogg",qtrue),CHAN_LOCAL_SOUND);
    }
    if (clocks.timeoutEnd) {
        remaining=((int64_t)clocks.timeoutEnd-cg.time)/1000;
        snprintf(text,sizeof(text),"Match resuming in ^5%lld^7 seconds",(long long)remaining);
    } else Q_strncpyz(text,"Match Paused",sizeof(text));
    if (remaining) {
        CG_SetTextFont(0);
        CG_Text_Paint(320-CG_Text_Width(text,.5f,0)*.5f,128,.5f,color,text,0,0,ITEM_TEXTSTYLE_SHADOWED);
    }
    if ((int64_t)cg.time-clocks.soundTime >= 1000) {
        clocks.soundTime=cg.time;
        TimeoutCount(remaining);
    } else if (cg.time < clocks.soundTime && cg.time >= clocks.timeoutStart) clocks.soundTime=cg.time;
}

static qboolean RoundMode(void) {
    /* Shipping lookup tables 1002980c, 10033bdc, 10030dd8. */
    return cgs.gametype == GT_CLAN_ARENA || cgs.gametype == GT_FREEZE ||
        cgs.gametype == GT_ATTACK_DEFEND || cgs.gametype == GT_RED_ROVER;
}

static qboolean Integer(const char *text, int *value) {
    char *end;
    long parsed;
    if (!text || !*text) { *value = 0; return qtrue; }
    errno = 0;
    parsed = strtol(text,&end,10);
    if (end == text || errno || parsed < INT_MIN || parsed > INT_MAX) return qfalse;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (*end) return qfalse;
    *value = (int)parsed;
    return qtrue;
}

static qboolean InfoInteger(const char *info, const char *key, int *value) {
    return Integer(Info_ValueForKey(info,key),value);
}

void CG_MatchClockConfigChanged(int index) {
    const char *text;
    int first, second, third;
    switch (index) {
    case CS_SERVERINFO:
        text = CG_ConfigString(index);
        if (InfoInteger(text,"timelimit",&first) && first >= 0 &&
            InfoInteger(text,"roundtimelimit",&second) && second >= 0 &&
            InfoInteger(text,"g_overtime",&third) && third >= 0) {
            clocks.limitMinutes=first; clocks.roundSeconds=second; clocks.overtimeSeconds=third;
        }
        break;
    case CS_WARMUP:
        if (InfoInteger(CG_ConfigString(index),"time",&first)) {
            clocks.warmup=first; clocks.round=0;
        }
        break;
    case CS_MATCH_STATE:
        /* 10048cb6: every round-state update re-arms the 30-second warning. */
        cg.timelimitWarnings &= ~8;
        text = CG_ConfigString(index);
        if (InfoInteger(text,"time",&first) && InfoInteger(text,"round",&second) &&
            InfoInteger(text,"turn",&third)) {
            /* AD publishes zero-based rounds; retail 10048cf4 adds one. */
            if (cgs.gametype == GT_ATTACK_DEFEND) {
                if (second == INT_MAX) break;
                ++second;
            }
            clocks.warmup=first; clocks.round=second; clocks.turn=third;
        }
        break;
    case CS_LEVEL_START_TIME:
        if (Integer(CG_ConfigString(index),&first)) clocks.levelStart=first;
        break;
    case CS_ROUND_START_TIME:
        if (Integer(CG_ConfigString(index),&first)) clocks.roundStart=first;
        break;
    case CS_TIMEOUT_START_TIME:
        if (Integer(CG_ConfigString(index),&first)) clocks.timeoutStart=first;
        break;
    case CS_TIMEOUT_END_TIME:
        if (Integer(CG_ConfigString(index),&first)) clocks.timeoutEnd=first;
        break;
    default: break;
    }
}

/* Retail 0x1004395d: time warnings outside warmup and timeouts. Each
 * moment is announced only if the local player joined before it (retail
 * compares the time its join menu last closed, 0x1000e880; FnQL-game uses
 * cg.teamJoinTime). Sudden death plays unless overtime is configured in a
 * non-round mode; the round 30-second warning is checked only when no match
 * warning fired. Bits: 1 five minutes, 2 one minute, 4 sudden death, 8 round. */
void CG_MatchClockWarnings(void) {
    int64_t limit, elapsed, joined = cg.teamJoinTime;
    int *flags = &cg.timelimitWarnings;
    if (clocks.limitMinutes <= 0 || clocks.warmup || clocks.timeoutStart) return;
    limit = (int64_t)clocks.limitMinutes * 60000;
    elapsed = (int64_t)cg.time - clocks.levelStart;
    if (!(*flags & 4) && elapsed > limit + 2000 && joined < clocks.levelStart + limit + 2000) {
        *flags |= 7;
        if (!clocks.overtimeSeconds || RoundMode()) CG_AddBufferedSound(CG_QLVoice("sudden_death"));
    } else if (!(*flags & 2) && elapsed > limit - 60000 && joined < clocks.levelStart + limit - 60000) {
        *flags |= 3;
        CG_AddBufferedSound(CG_QLVoice("1_minute"));
    } else if (clocks.limitMinutes > 5 && !(*flags & 1) && elapsed > limit - 300000 &&
               joined < clocks.levelStart + limit - 300000) {
        *flags |= 1;
        CG_AddBufferedSound(CG_QLVoice("5_minute"));
    } else if (!(*flags & 8) && RoundMode()) {
        int64_t remaining = (int64_t)clocks.roundSeconds * 1000 - cg.time + clocks.roundStart;
        if (clocks.roundStart > 0) --remaining;
        if (remaining >= 0 && remaining <= 30000) {
            *flags |= 8;
            CG_AddBufferedSound(CG_QLVoice("30_second_warning"));
        }
    }
}

void CG_InitMatchClocks(void) {
    memset(&clocks,0,sizeof(clocks));
    CG_ClearMatchNotice();
    CG_MatchClockConfigChanged(CS_SERVERINFO);
    CG_MatchClockConfigChanged(CS_LEVEL_START_TIME);
    CG_MatchClockConfigChanged(CS_ROUND_START_TIME);
    CG_MatchClockConfigChanged(CS_TIMEOUT_START_TIME);
    CG_MatchClockConfigChanged(CS_TIMEOUT_END_TIME);
    CG_MatchClockConfigChanged(CS_WARMUP);
    /* Retail initialization 100497df-100497ff applies round state last. */
    if (RoundMode()) CG_MatchClockConfigChanged(CS_MATCH_STATE);
}

void CG_AdvanceMatchClocks(void) {
    /* The timeout HUD adjusts its private round start after painting clocks
     * (10011173/1000ef30). A server update replaces that adjusted value.
     * Advance only once at a given time, including stereo rendering. */
    if (clocks.frameSeen && clocks.lastFrame == cg.time) return;
    if ((!clocks.frameSeen || cg.time > clocks.lastFrame) &&
        (clocks.timeoutStart || clocks.timeoutEnd)) {
        if (!clocks.timeoutStart && clocks.timeoutEnd) {
            clocks.roundStart += ((int64_t)clocks.timeoutEnd - cg.time - 1000) / 1000;
            clocks.timeoutEnd=0;
        } else if (!clocks.timeoutEnd && cg.frametime > 0) {
            clocks.roundStart += cg.frametime;
        }
    }
    clocks.lastFrame=cg.time; clocks.frameSeen=qtrue;
}

static int64_t MatchNow(void) { return clocks.timeoutStart ? clocks.timeoutStart : cg.time; }

static void LevelText(char *text,int size) {
    int64_t elapsed=0, limit=(int64_t)clocks.limitMinutes*60000, seconds;
    /* Retail 10029770: round breaks retain match time after round zero. */
    if (!clocks.warmup || (RoundMode() && clocks.round != 0)) {
        elapsed=MatchNow()-clocks.levelStart;
        if (limit) {
            if (elapsed > limit) elapsed-=limit;
            else if (cg_levelTimerDirection.integer == 1) elapsed=limit-elapsed;
        }
    }
    if (elapsed < 0) elapsed=0;
    seconds=elapsed/1000;
    /* 64-bit intermediates avoid overflow for extreme server times/limits. */
    snprintf(text,size,"%lld:%02d",(long long)(seconds/60),(int)(seconds%60));
}

qboolean CG_DrawMatchClock(int owner,float x,float y,int align,float scale,vec4_t color,int style) {
    char text[96];
    int64_t remaining, count;
    int width;
    switch (owner) {
    case CG_LEVELTIMER:
        LevelText(text,sizeof(text));
        break;
    case CG_ROUND:
        if (!RoundMode()) return qtrue;
        if (clocks.round <= 0) Q_strncpyz(text,"Warmup",sizeof(text));
        else snprintf(text,sizeof(text),"Round %d",clocks.round);
        break;
    case CG_ROUNDTIMER:
        /* 10033b62: 1..29999 ms, round modes only, no round warmup. */
        remaining=(int64_t)clocks.roundSeconds*1000-cg.time+clocks.roundStart;
        if (!RoundMode() || clocks.roundStart <= 0 || clocks.warmup ||
            remaining <= 0 || remaining >= 30000) return qtrue;
        snprintf(text,sizeof(text),"^1%d",(int)(remaining/1000));
        break;
    case CG_OVERTIME:
        if (RoundMode() || clocks.warmup || !clocks.limitMinutes || !clocks.overtimeSeconds) return qtrue;
        /* 1002a5f0 counts complete overtime seconds, then one period for
         * each exact multiple. Division replaces retail's per-second loop. */
        remaining=(MatchNow()-clocks.levelStart-(int64_t)clocks.limitMinutes*60000)/1000;
        if (remaining <= 0) return qtrue;
        count=1+remaining/clocks.overtimeSeconds;
        if (count == 1) Q_strncpyz(text,"Overtime",sizeof(text));
        else snprintf(text,sizeof(text),"Overtime x%lld",(long long)count);
        break;
    case CG_LOCALTIME: {
        static const char *const months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        qtime_t now;
        memset(&now,0,sizeof(now));
        trap_RealTime(&now);
        if (now.tm_mon < 0 || now.tm_mon >= 12 || now.tm_hour < 0 || now.tm_hour > 23 ||
            now.tm_min < 0 || now.tm_min > 59 || now.tm_mday < 1 || now.tm_mday > 31 ||
            now.tm_year < -1900 || now.tm_year > INT_MAX-1900) return qtrue;
        snprintf(text,sizeof(text),"%02d:%02d (%s %02d, %d)",now.tm_hour,now.tm_min,
            months[now.tm_mon],now.tm_mday,now.tm_year+1900);
        break;
    }
    default: return qfalse;
    }
    /* 10030c00/100336e0/1002e050/10033d80 align about the item origin,
     * not its rectangle width. y is already the text baseline. */
    if (owner != CG_LOCALTIME) CG_SetTextFont(0);
    width=CG_Text_Width(text,scale,0);
    if (align == ITEM_ALIGN_CENTER) x-=width*.5f;
    else if (align == ITEM_ALIGN_RIGHT) x-=width;
    CG_Text_Paint(x,y,scale,color,text,0,0,style);
    return qtrue;
}
