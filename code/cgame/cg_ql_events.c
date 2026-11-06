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
#include <stdint.h>

/* Retail cgame 0x1002236c/394, 0x10022e5d and 0x10022ed7.
 * Keep these handles separate from the inherited Team Arena rune sounds. */
static sfxHandle_t surfaceSteps[2][4];
static sfxHandle_t armorRegeneration, thawTick, overtimeKlaxon, matchBuzzer;
/* Retail 0x100214a3: domination capture/neutralize sounds (gametype 10 only);
 * 0x1002138b: the round survivor cue (clan arena, freeze, A&D, Red Rover). */
static sfxHandle_t domCaptureOurs, domCaptureTheirs, domNeutralized, roundSurvivor;
/* Retail team media: 0x1002112b registers the flag-return sounds and team
 * voices from team deathmatch up; 0x100213de the flag-capture sounds for
 * CTF, One Flag, Overload, Harvester, Domination and A&D; 0x10022dba the
 * far kamikaze explosion for every gametype. */
static sfxHandle_t flagCaptureOurs, flagCaptureTheirs, flagReturnOurs, flagReturnTheirs, kamikazeFar;

void CG_PushReward(sfxHandle_t sound, qhandle_t shader, int count) {
    int slot;
    if (cg.rewardStack < 0 || cg.rewardStack >= MAX_REWARDSTACK - 1 || count < 0) return;
    slot = ++cg.rewardStack;
    cg.rewardSound[slot] = sound;
    cg.rewardShader[slot] = shader;
    cg.rewardCount[slot] = count;
}

/* Retail 0x10020dd0 selects the stock, evil or female announcer folders.
 * Renderer/sound registration caches the handles; missing alternate audio
 * falls back to the stock voice without replacing the user's preference. */
sfxHandle_t CG_QLVoice(const char *file) {
    char path[MAX_QPATH];
    sfxHandle_t sound;
    const char *folder = cg_announcer.integer == 2 ? "vo_evil" :
                         cg_announcer.integer == 3 ? "vo_female" : "vo";
    Com_sprintf(path, sizeof(path), "sound/%s/%s.ogg", folder, file);
    sound = trap_S_RegisterSound(path, qtrue);
    if (!sound && cg_announcer.integer >= 2 && cg_announcer.integer <= 3) {
        Com_sprintf(path, sizeof(path), "sound/vo/%s.ogg", file);
        sound = trap_S_RegisterSound(path, qtrue);
    }
    return sound;
}

/* Retail award event 0x1001c410 and media registration 0x10021efa/24692.
 * IDs belong to this event, not the persistent Q3 award-counter indices. */
static void CG_QLAward(const entityState_t *event) {
    static const struct { const char *medal, *voice; int variants; } awards[] = {
        {"combokill", "combokill", 3}, {"rampage", "rampage", 3},
        {"midair", "midair", 3}, {"revenge", "revenge", 3},
        {"perforated", "perforated", 1}, {"headshot", "headshot", 1},
        {"accuracy", "accuracy", 1}, {"quadgod", "quad_god", 1},
        {"firstfrag", "first_frag", 1}, {"perfect", "perfect", 1}
    };
    int id = event->generic1;
    char voice[MAX_QPATH], medal[MAX_QPATH];
    if (!cg.snap || event->clientNum != cg.snap->ps.clientNum ||
        id < 0 || id >= ARRAY_LEN(awards) || event->modelindex2 < 0) return;
    if (awards[id].variants > 1)
        Com_sprintf(voice, sizeof(voice), "%s%d", awards[id].voice, 1 + rand() % awards[id].variants);
    else
        Com_sprintf(voice, sizeof(voice), "%s", awards[id].voice);
    Com_sprintf(medal, sizeof(medal), "medal_%s", awards[id].medal);
    CG_PushReward(CG_QLVoice(voice), trap_R_RegisterShaderNoMip(medal),
                  id == 8 ? 1 : event->modelindex2);
}

void CG_RegisterQLEventSounds(void) {
    int surface, variant;
    char path[MAX_QPATH];
    const char *names[2] = {"snow", "wood"};
    for (surface = 0; surface < 2; ++surface) {
        for (variant = 0; variant < 4; ++variant) {
            Com_sprintf(path, sizeof(path), "sound/player/footsteps/%s%d.ogg",
                        names[surface], variant + 1);
            surfaceSteps[surface][variant] = trap_S_RegisterSound(path, qfalse);
        }
    }
    armorRegeneration = trap_S_RegisterSound("sound/misc/ar1_pkup.ogg", qfalse);
    thawTick = trap_S_RegisterSound("sound/misc/tim_pump.ogg", qfalse);
    overtimeKlaxon = trap_S_RegisterSound("sound/world/klaxon2.ogg", qfalse);
    matchBuzzer = trap_S_RegisterSound("sound/world/buzzer.ogg", qfalse);
    domCaptureOurs = domCaptureTheirs = domNeutralized = roundSurvivor = 0;
    if (cgs.gametype == GT_DOMINATION) {
        domNeutralized = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.ogg", qfalse);
        domCaptureOurs = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.ogg", qfalse);
        domCaptureTheirs = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.ogg", qfalse);
    }
    if (cgs.gametype == GT_CLAN_ARENA || cgs.gametype == GT_FREEZE ||
        cgs.gametype == GT_ATTACK_DEFEND || cgs.gametype == GT_RED_ROVER)
        roundSurvivor = trap_S_RegisterSound("sound/feedback/survivor_01.ogg", qfalse);
    flagCaptureOurs = flagCaptureTheirs = flagReturnOurs = flagReturnTheirs = 0;
    kamikazeFar = trap_S_RegisterSound("sound/items/kam_explode_far.ogg", qfalse);
    if (cgs.gametype >= GT_TEAM) {
        flagReturnOurs = trap_S_RegisterSound("sound/teamplay/flagreturn_yourteam.ogg", qfalse);
        flagReturnTheirs = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.ogg", qfalse);
    }
    if ((cgs.gametype >= GT_CTF && cgs.gametype <= GT_HARVESTER) ||
        cgs.gametype == GT_DOMINATION || cgs.gametype == GT_ATTACK_DEFEND) {
        flagCaptureOurs = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.ogg", qfalse);
        flagCaptureTheirs = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.ogg", qfalse);
    }
}

/* Voices retail registers only for some gametypes stay silent elsewhere
 * (a zero handle is ignored by the announcement queue, 0x1004e110). */
static void CG_TeamVoice(qboolean registered, const char *voice) {
    if (registered) CG_AddBufferedSound(CG_QLVoice(voice));
}

static void CG_LocalTeamSound(sfxHandle_t sound) {
    if (sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND);
}

static int CG_LocalTeam(void) {
    if (cg.clientNum < 0 || cg.clientNum >= MAX_CLIENTS) return -1;
    return cgs.clientinfo[cg.clientNum].team;
}

/* Retail 0x10019d20: with cg_followPowerup, a free spectator (local team
 * spectator, snapshot pm_type spectator) follows whoever took a flag or a
 * powerup. Mode 2 adds " pw" for powerups so the server keeps following
 * carriers; flags never get the suffix. */
void CG_FollowPowerup(int client, qboolean flag) {
    char command[32];
    if (!cg_followPowerup.integer || !cg.snap || cg.snap->ps.pm_type != PM_SPECTATOR ||
        CG_LocalTeam() != TEAM_SPECTATOR || client < 0 || client >= MAX_CLIENTS) return;
    Com_sprintf(command, sizeof(command), "follow %d%s", client,
                cg_followPowerup.integer == 2 && !flag ? " pw" : "");
    trap_SendClientCommand(command);
}

/* Retail EV_GLOBAL_ITEM_PICKUP 0x1001a5ab, after the pickup sound: unless
 * Quad Hog is on, item tags 4..8 queue the powerup voice (the switch reads
 * the tag only); powerups (type 5) also drive cg_followPowerup. */
void CG_GlobalItemPickupAnnouncement(const entityState_t *event, int giType, int giTag) {
    static const char *const voices[] = {
        "quad_damage", "battlesuit", "haste", "invisibility", "regeneration"
    };
    if (!(cgs.settingsMask & CG_SETTING_QUAD_HOG) && giTag >= PW_QUAD && giTag <= PW_REGEN)
        CG_AddBufferedSound(CG_QLVoice(voices[giTag - PW_QUAD]));
    if (giType == IT_POWERUP) CG_FollowPowerup(event->otherEntityNum, qfalse);
}

/* Retail EV_ITEM_PICKUP 0x1001a557: persistent powerup tags 1..4 queue
 * the scout, guard, damage and armor-regeneration voices. */
void CG_PersistentPowerupAnnouncement(int giTag) {
    static const char *const voices[] = { "scout", "guard", "damage", "armor_regen" };
    if (giTag >= PP_SCOUT && giTag <= PP_ARMORREGEN)
        CG_AddBufferedSound(CG_QLVoice(voices[giTag - PP_SCOUT]));
}

/* Retail global team sounds 0..9, 12 and 13 (switch 0x1001b349). Every
 * branch uses the local client's team, as retail reads clientinfo for
 * cg.clientNum rather than the followed snapshot player. */
static qboolean CG_QLTeamSound(const entityState_t *event) {
    const qboolean teamMedia = cgs.gametype >= GT_TEAM;
    const qboolean ctfVoices = cgs.gametype == GT_CTF || cgs.gametype == GT_ATTACK_DEFEND;
    int local = CG_LocalTeam(), team, other;
    switch (event->eventParm) {
    case GTS_RED_CAPTURE:
    case GTS_BLUE_CAPTURE:
        /* 0x1001b362/39e: effect only, on the local-sound channel. */
        team = event->eventParm == GTS_RED_CAPTURE ? TEAM_RED : TEAM_BLUE;
        CG_LocalTeamSound(local == team ? flagCaptureOurs : flagCaptureTheirs);
        return qtrue;
    case GTS_RED_RETURN:
    case GTS_BLUE_RETURN:
        /* 0x1001b3d7/452: RED_RETURN returns the blue flag. Its owners
         * hear the opponent effect and "your flag returned". */
        team = event->eventParm == GTS_RED_RETURN ? TEAM_BLUE : TEAM_RED;
        other = team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
        if (local == team) {
            CG_LocalTeamSound(flagReturnTheirs);
            CG_TeamVoice(ctfVoices, "your_flag_returned");
        } else if (local == other) {
            CG_LocalTeamSound(flagReturnOurs);
            CG_TeamVoice(ctfVoices, "enemy_flag_returned");
        } else {
            CG_TeamVoice(ctfVoices, team == TEAM_BLUE ? "blue_flag_returned" : "red_flag_returned");
        }
        return qtrue;
    case GTS_RED_TAKEN:
    case GTS_BLUE_TAKEN: {
        /* 0x1001b4c1/582: RED_TAKEN means red took the blue flag. The
         * snapshot's own carrier hears nothing here; One Flag players get
         * the return effects instead of voices. */
        int flag = event->eventParm == GTS_RED_TAKEN ? PW_BLUEFLAG : PW_REDFLAG;
        team = event->eventParm == GTS_RED_TAKEN ? TEAM_RED : TEAM_BLUE;
        other = team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
        CG_FollowPowerup(event->otherEntityNum, qtrue);
        if (!cg.snap || cg.snap->ps.powerups[flag] || cg.snap->ps.powerups[PW_NEUTRALFLAG]) return qtrue;
        if (local == other) {
            if (cgs.gametype == GT_1FCTF) CG_LocalTeamSound(flagReturnOurs);
            else CG_TeamVoice(ctfVoices, "your_flag_taken");
        } else if (local == team) {
            if (cgs.gametype == GT_1FCTF) CG_LocalTeamSound(flagReturnTheirs);
            else CG_TeamVoice(ctfVoices, "enemy_flag_taken");
        } else {
            CG_TeamVoice(ctfVoices, team == TEAM_RED ? "blue_flag_taken" : "red_flag_taken");
        }
        return qtrue;
    }
    case GTS_REDOBELISK_ATTACKED:
    case GTS_BLUEOBELISK_ATTACKED:
        /* 0x1001b63a/64f, voice registered for Overload only. */
        team = event->eventParm == GTS_REDOBELISK_ATTACKED ? TEAM_RED : TEAM_BLUE;
        if (local == team) CG_TeamVoice(cgs.gametype == GT_OBELISK, "base_attack");
        return qtrue;
    case GTS_REDTEAM_SCORED:
    case GTS_BLUETEAM_SCORED:
        /* 0x1001b672/6e9: Freeze Tag also centers the scoring team; A&D
         * has no scoring voice. */
        team = event->eventParm == GTS_REDTEAM_SCORED ? TEAM_RED : TEAM_BLUE;
        other = team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
        if (cgs.gametype == GT_FREEZE)
            CG_CenterPrint(team == TEAM_RED ? "Red Scores" : "Blue Scores", 90, BIGCHAR_WIDTH);
        if (cgs.gametype == GT_ATTACK_DEFEND) return qtrue;
        if (local == team) CG_TeamVoice(teamMedia, "your_team_scores");
        else if (local == other) CG_TeamVoice(teamMedia, "enemy_scores");
        else CG_TeamVoice(teamMedia, team == TEAM_RED ? "red_scores" : "blue_scores");
        return qtrue;
    case GTS_TEAMS_ARE_TIED:
        /* 0x1001b7f7 */
        if (cgs.gametype != GT_ATTACK_DEFEND && cg_announcerTiesVO.integer)
            CG_TeamVoice(teamMedia, "teams_tied");
        return qtrue;
    case GTS_KAMIKAZE:
        /* 0x1001bad9 */
        CG_LocalTeamSound(kamikazeFar);
        return qtrue;
    default:
        return qfalse;
    }
}

/* Retail global team sound 23, 0x1001bce3. modelindex2 is the capturing
 * team (0 when a point turns neutral) and powerups the point number; the
 * server sends point 0 for warmup captures. Any team other than red is
 * named "Blue". The announcer half follows cg_announcerLeadsVO and the
 * local client's team rather than a followed player's. */
static void CG_DominationPoint(const entityState_t *event) {
    static const char *const captured[6] = {
        NULL, "a_captured", "b_captured", "c_captured", "d_captured", "e_captured"
    };
    static const char *const lost[6] = {
        NULL, "a_lost", "b_lost", "c_lost", "d_lost", "e_lost"
    };
    int team = event->modelindex2, point = event->powerups, local;
    if (!team) {
        if (domNeutralized) trap_S_StartLocalSound(domNeutralized, CHAN_LOCAL_SOUND);
        return;
    }
    if (point) {
        /* 0x10001120 names points 1..5 A..E and anything else "?". */
        static const char letters[] = "ABCDE";
        char message[64], name[2] = { '?', 0 };
        if (point >= 1 && point <= 5) name[0] = letters[point - 1];
        Com_sprintf(message, sizeof(message), "%s captured point %s!",
                    team == TEAM_RED ? "Red" : "Blue", name);
        CG_CenterPrint(message, 90, BIGCHAR_WIDTH);
    }
    /* Retail indexes five-entry sound tables directly; other point numbers
     * would read past them, so they stay silent here. */
    if (!cg_announcerLeadsVO.integer || point < 0 || point > 5) return;
    local = CG_LocalTeam();
    if (local < 0) return;
    if (local == team)
        CG_AddBufferedSound(point ? CG_QLVoice(captured[point]) : domCaptureOurs);
    else if (local != TEAM_SPECTATOR)
        CG_AddBufferedSound(point ? CG_QLVoice(lost[point]) : domCaptureTheirs);
}

/* Retail 1004e180 clears reward/voice queues but preserves a future voice
 * deadline so the result does not overlap an announcement already playing. */
void CG_ClearMatchAnnouncements(void) {
    cg.rewardStack = cg.rewardTime = 0;
    memset(cg.rewardSound,0,sizeof(cg.rewardSound));
    memset(cg.rewardShader,0,sizeof(cg.rewardShader));
    memset(cg.rewardCount,0,sizeof(cg.rewardCount));
    cg.soundBufferIn = cg.soundBufferOut = cg.soundPlaying = 0;
    memset(cg.soundBuffer,0,sizeof(cg.soundBuffer));
    if (cg.soundTime <= cg.time) cg.soundTime = 0;
}

static qboolean CG_MatchWinner(void) {
    int team, client;
    if (!cg.snap) return qfalse;
    client = cg.snap->ps.clientNum;
    if (client < 0 || client >= MAX_CLIENTS || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) return qfalse;
    /* Retail 1002a500: RR uses individual rank; tied first is not a win.
     * For team games a tied score favors red, while blue must lead strictly. */
    if (cgs.gametype <= GT_RACE || cgs.gametype == GT_RED_ROVER)
        return cg.snap->ps.persistant[PERS_RANK] == 0;
    team = cgs.clientinfo[client].team;
    return (team == TEAM_RED && cgs.scores1 >= cgs.scores2) ||
           (team == TEAM_BLUE && cgs.scores2 > cgs.scores1);
}

static void CG_MatchResult(int winningTeam) {
    qboolean winner;
    int team;
    if (!cg.snap) return;
    CG_ClearMatchAnnouncements();
    if (cg_buzzerSound.integer && matchBuzzer) trap_S_StartLocalSound(matchBuzzer,CHAN_LOCAL_SOUND);
    if (winningTeam) {
        /* Team result events use the local client's team, including while
         * following another player (1001b840/1001b8ca). Spectators get no music. */
        team = cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS ? cgs.clientinfo[cg.clientNum].team : TEAM_SPECTATOR;
        if (team == TEAM_RED || team == TEAM_BLUE)
            trap_S_StartBackgroundTrack(team == winningTeam ? "music/win" : "music/loss", "");
        CG_AddBufferedSound(CG_QLVoice(winningTeam == TEAM_RED ? "red_wins" : "blue_wins"));
    } else {
        winner = CG_MatchWinner();
        team = cg.snap->ps.persistant[PERS_TEAM];
        if (team != TEAM_SPECTATOR && (cgs.gametype <= GT_RACE || cgs.gametype == GT_RED_ROVER))
            CG_AddBufferedSound(CG_QLVoice(winner ? "you_win" : "you_lose"));
        trap_S_StartBackgroundTrack(winner || team == TEAM_SPECTATOR ? "music/win" : "music/loss", "");
    }
    cg.matchEnded = qtrue;
}

static void CG_Overtime(void) {
    char message[96];
    int64_t elapsed;
    if (!cg.snap) return;
    trap_S_StartSound(NULL,cg.snap->ps.clientNum,CHAN_AUTO,overtimeKlaxon);
    Com_sprintf(message,sizeof(message),"Overtime! %d seconds added",cgs.overtimeSeconds);
    CG_CenterPrint(message,90,BIGCHAR_WIDTH);
    /* 1002a5f0 returns zero until the first whole second past the time limit.
     * Only that zero/nonzero result matters here. Avoid its iterative count
     * and modulo by an untrusted overtime duration. Use wide arithmetic. */
    elapsed = (int64_t)cg.time - cgs.levelStartTime - (int64_t)cgs.timelimit * 60000;
    if (elapsed < 1000) CG_AddBufferedSound(CG_QLVoice("overtime"));
}

/* These configstrings are read at dispatch, so initial game state and later
 * updates share one path. Retail 1004a05a/077: winner list 0x2bf, settings
 * mask 0x2c0; bit 26 selects infected RR. Reject malformed native strings
 * before indexing client info or displaying a partial winner list. */
static qboolean CG_RoundNumber(const char **cursor, uint32_t limit, uint32_t *value) {
    const char *p = *cursor;
    uint32_t number = 0;
    while (*p && (unsigned char)*p <= ' ') ++p;
    if (*p < '0' || *p > '9') return qfalse;
    do {
        uint32_t digit = (uint32_t)(*p++ - '0');
        if (digit > limit || number > (limit - digit) / 10) return qfalse;
        number = number * 10 + digit;
    } while (*p >= '0' && *p <= '9');
    if (*p && (unsigned char)*p > ' ') return qfalse;
    while (*p && (unsigned char)*p <= ' ') ++p;
    *cursor = p;
    *value = number;
    return qtrue;
}

static void CG_RoundResult(qboolean draw) {
    const char *settings = CG_ConfigString(CS_SETTINGS_MASK);
    const char *list = CG_ConfigString(CS_MATCH_AWARDS);
    uint32_t mask = 0, client;
    int winners[MAX_CLIENTS], count = 0, i;
    qboolean infected, localWinner = qfalse, valid = qtrue;
    char names[MAX_STRING_CHARS] = "", message[MAX_STRING_CHARS];
    const char *voice = draw ? "round_draw" : "round_over";

    if (draw) CG_ClearMatchAnnouncements(); /* 1001b977, no match-end guard. */
    if (!CG_RoundNumber(&settings,UINT32_MAX,&mask) || *settings) mask = 0;
    infected = (mask & (1u << 26)) != 0;
    if (cgs.gametype != GT_RED_ROVER || (draw && !infected)) {
        CG_CenterPrint(draw ? "Round Draw" : "Round Over",90,BIGCHAR_WIDTH);
        CG_AddBufferedSound(CG_QLVoice(voice));
        return;
    }

    while (*list && (unsigned char)*list <= ' ') ++list;
    while (*list) {
        if (count == MAX_CLIENTS || !CG_RoundNumber(&list,MAX_CLIENTS-1,&client)) {
            valid = qfalse;
            break;
        }
        for (i = 0; i < count; ++i) if (winners[i] == (int)client) break;
        if (i != count) { valid = qfalse; break; }
        winners[count++] = (int)client;
    }
    if (!valid || !count) {
        CG_CenterPrint(draw ? "Round Draw" : "Round Over",90,BIGCHAR_WIDTH);
        CG_AddBufferedSound(CG_QLVoice(voice));
        return;
    }
    for (i = 0; i < count; ++i) {
        const clientInfo_t *info = &cgs.clientinfo[winners[i]];
        size_t used = strlen(names);
        if (winners[i] == cg.clientNum) localWinner = qtrue;
        if (info->infoValid && used < sizeof(names)-1)
            Com_sprintf(names+used,(int)(sizeof(names)-used),"%s\n",info->name);
    }
    /* Retail 1001ba38/1001bc17: draws recognize any local survivor;
     * infected round wins compare the final listed client, even for a tie. */
    if (infected && !draw && winners[count-1] == cg.clientNum) {
        Com_sprintf(message,sizeof(message),"^7%s Wins!^7\n",names);
        voice = "you_win";
    } else {
        Com_sprintf(message,sizeof(message),infected ? "^1Last Survivor%s:^7\n%s" : "Round Winner%s:\n%s",
                    count == 1 ? "" : "s",names);
        if (infected) voice = draw && localWinner ? "you_win" : "round_over";
    }
    CG_CenterPrint(message,90,BIGCHAR_WIDTH);
    CG_AddBufferedSound(CG_QLVoice(voice));
}

/* Retail 0x10019d90 maintains ten timed-item records, keyed by item entity.
 * Updates replace an existing record; a full table does not evict one, and
 * timedItemCount counts the records added (capped at ten).
 * groundEntityNum modifies the grouped-item flag only on an update. */
static void CG_RecordSpectatorPickup(const entityState_t *event) {
    int i, slot = -1;
    qboolean updating = qfalse;
    cgTimedItem_t *item;
    if (event->otherEntityNum <= 0 || event->otherEntityNum >= MAX_GENTITIES ||
        event->modelindex <= 0 || event->modelindex >= bg_numItems ||
        event->modelindex2 < 0 || event->modelindex2 > 1) return;
    for (i = 0; i < CG_MAX_TIMED_ITEMS; ++i) {
        if (cg.timedItems[i].entity == event->otherEntityNum) {
            slot = i;
            updating = qtrue;
            break;
        }
        if (slot < 0 && !cg.timedItems[i].entity) slot = i;
    }
    if (slot < 0) return;
    if (!updating) {
        if (cg.timedItemCount >= CG_MAX_TIMED_ITEMS) return;
        ++cg.timedItemCount;
    }
    item = &cg.timedItems[slot];
    item->entity = event->otherEntityNum;
    item->side = event->otherEntityNum2;
    item->item = event->modelindex;
    item->remaining = (int)((unsigned)event->time - (unsigned)cg.time);
    item->distance = event->time2;
    item->group = event->modelindex2 * (updating && event->groundEntityNum > 0 ? 2 : 1);
    VectorCopy(event->pos.trBase, item->origin);
}

/* Retail 0x10053d90: choose the highest numbered owned, nonempty weapon,
 * excluding the dropped weapon and reserved slot 15, including infinite ammo.
 * The event addresses the local client, not a spectator's followed player. */
static void CG_SelectAfterDrop(const entityState_t *event) {
    int weapon;
    if (!cg.snap || event->clientNum != cg.clientNum ||
        event->eventParm <= WP_NONE || event->eventParm >= WP_NUM_WEAPONS) return;
    cg.weaponSelectTime = cg.time;
    for (weapon = WP_NUM_WEAPONS - 1; weapon > WP_NONE; --weapon) {
        if (weapon != event->eventParm &&
            (cg.snap->ps.stats[STAT_WEAPONS] & (1 << weapon)) && cg.snap->ps.ammo[weapon]) {
            cg.weaponSelect = weapon;
            return;
        }
    }
}

/* Retail 0x10055aa0: generic1 is total shotgun damage (an eight-bit wire
 * field), not a pellet count or eventParm. Each group of five produces two
 * blood spurts at one scattered position and an optional impact spark. */
static void CG_ShotgunKill(const entityState_t *event) {
    int i, bursts;
    qhandle_t shader = 0;
    if (event->generic1 < 0 || event->generic1 > 255 ||
        event->otherEntityNum2 < 0 || event->otherEntityNum2 >= MAX_GENTITIES) return;
    bursts = event->generic1 / 5;
    if (bursts && cg_impactSparks.integer && cg_impactSparksSize.integer > 0 &&
        cg_impactSparksSize.integer <= 1024 && cg_impactSparksLifetime.integer > 0 &&
        cg_impactSparksLifetime.integer <= 10000)
        shader = trap_R_RegisterShader("gfx/misc/tracer");
    for (i = 0; i < bursts; ++i) {
        vec3_t position, velocity = {0, 0, 0};
        VectorCopy(event->pos.trBase, position);
        position[0] += rand() % 32 - 16;
        position[1] += rand() % 32 - 16;
        position[2] += rand() % 24 + 8;
        CG_Bleed(position, event->otherEntityNum2);
        CG_Bleed(position, event->otherEntityNum2);
        if (shader) {
            velocity[2] = cg_impactSparksVelocity.value;
            CG_SmokePuff(position, velocity, cg_impactSparksSize.integer, 1, 1, 1, 1,
                         cg_impactSparksLifetime.integer, cg.time, 0, LEF_PUFF_DONT_SCALE, shader);
        }
    }
}

qboolean CG_QL_EntityEvent(centity_t *cent) {
    entityState_t *event = &cent->currentState;
    int kind = event->event & ~EV_EVENT_BITS;
    switch (kind) {
    case EV_RACE_START:
    case EV_RACE_CHECKPOINT:
    case EV_RACE_FINISH:
        CG_RaceEvent(event);
        return qtrue;
    case EV_THAW_PLAYER:
        CG_ThawPlayer(cent->lerpOrigin);
        return qtrue;
    case EV_MISSILE_MISS_DMGTHROUGH: {
        vec3_t direction;
        if (event->eventParm < 0 || event->eventParm >= NUMVERTEXNORMALS) return qtrue;
        ByteToDir(event->eventParm,direction);
        CG_MissileHitWallDmgThrough(event->weapon,cent->lerpOrigin,direction);
        return qtrue;
    }
    case EV_OVERTIME:
        CG_Overtime();
        return qtrue;
    case EV_GAMEOVER:
        CG_MatchResult(0);
        return qtrue;
    case EV_GLOBAL_TEAM_SOUND:
        if (event->eventParm == GTS_ROUND_DRAW || event->eventParm == 20) {
            /* Retail global-team-sound cases 18 and 20, 1001b971/1001bb6e. */
            CG_RoundResult(event->eventParm == GTS_ROUND_DRAW);
            return qtrue;
        }
        if (event->eventParm == GTS_LAST_STANDING) {
            /* 1001bafb: local team, not followed client; cvar default 1. */
            if (cg_announcerLastStandingVO.integer && cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS &&
                (event->modelindex2 == TEAM_RED || event->modelindex2 == TEAM_BLUE) &&
                cgs.clientinfo[cg.clientNum].team == event->modelindex2)
                CG_AddBufferedSound(CG_QLVoice("last_standing"));
            return qtrue;
        }
        if (event->eventParm == GTS_REDTEAM_WINS || event->eventParm == GTS_BLUETEAM_WINS) {
            CG_MatchResult(event->eventParm == GTS_REDTEAM_WINS ? TEAM_RED : TEAM_BLUE);
            return qtrue;
        }
        if (event->eventParm == GTS_REDTEAM_TOOK_LEAD || event->eventParm == GTS_BLUETEAM_TOOK_LEAD) {
            /* 0x1001b750/7b3: no lead voices in attack & defend or with
             * cg_announcerLeadsVO 0; players hear taken/lost by their own
             * team, everyone else hears which team leads. */
            int leader = event->eventParm == GTS_REDTEAM_TOOK_LEAD ? TEAM_RED : TEAM_BLUE;
            int local = CG_LocalTeam();
            if (cgs.gametype == GT_ATTACK_DEFEND || !cg_announcerLeadsVO.integer) return qtrue;
            if (local == leader) CG_AddBufferedSound(CG_QLVoice("lead_taken"));
            else if (local == TEAM_RED || local == TEAM_BLUE) CG_AddBufferedSound(CG_QLVoice("lead_lost"));
            else CG_TeamVoice(cgs.gametype >= GT_TEAM, leader == TEAM_RED ? "red_leads" : "blue_leads");
            return qtrue;
        }
        if (CG_QLTeamSound(event)) return qtrue;
        if (event->eventParm == 23) {
            CG_DominationPoint(event);
            return qtrue;
        }
        if (event->eventParm == 24) {
            /* 0x1001bb46: the survivor cue goes only to modelindex2's team. */
            if (roundSurvivor && CG_LocalTeam() == event->modelindex2)
                trap_S_StartLocalSound(roundSurvivor, CHAN_LOCAL_SOUND);
            return qtrue;
        }
        if (event->eventParm == GTS_REDTEAM_WINS_ROUND || event->eventParm == GTS_BLUETEAM_WINS_ROUND) {
            qboolean red = event->eventParm == GTS_REDTEAM_WINS_ROUND;
            CG_CenterPrint(red ? "Red wins the round" : "Blue wins the round",90,BIGCHAR_WIDTH);
            CG_AddBufferedSound(CG_QLVoice(red ? "red_wins_round" : "blue_wins_round"));
            return qtrue;
        }
        return qfalse;
    case EV_SHOTGUN_KILL:
        CG_ShotgunKill(event);
        return qtrue;
    case EV_DAMAGEPLUM:
        CG_DamagePlum(cent);
        return qtrue;
    case EV_POI:
        CG_PointOfInterest(cent);
        return qtrue;
    case EV_FOOTSTEP_SNOW:
    case EV_FOOTSTEP_WOOD:
        /* Retail 0x1001c007/054: four variants, body channel, cg_footsteps. */
        if (cg_footsteps.integer) {
            int surface = kind == EV_FOOTSTEP_WOOD;
            trap_S_StartSound(NULL, event->number, CHAN_BODY, surfaceSteps[surface][rand() & 3]);
        }
        return qtrue;
    case EV_DROWN:
        /* Retail 0x1001be15 -> 0x1001b213. */
        trap_S_StartSound(NULL, event->number, CHAN_VOICE, CG_CustomSound(event->number, "*drown.wav"));
        return qtrue;
    case EV_POWERUP_ARMORREGEN:
        /* Retail 0x1001bf1b: PW_ARMORREGEN (14), not the old Q3 rune slot. */
        if (cg.snap && event->number == cg.snap->ps.clientNum) {
            cg.powerupActive = PW_ARMORREGEN;
            cg.powerupTime = cg.time;
        }
        trap_S_StartSound(NULL, event->number, CHAN_ITEM, armorRegeneration);
        return qtrue;
    case EV_THAW_TICK:
        /* Retail 0x1001c209. */
        trap_S_StartSound(NULL, event->number, CHAN_BODY, thawTick);
        return qtrue;
    case EV_DROP_WEAPON:
        CG_SelectAfterDrop(event);
        return qtrue;
    case EV_ITEM_PICKUP_SPEC:
        CG_RecordSpectatorPickup(event);
        return qtrue;
    case EV_AWARD:
        CG_QLAward(event);
        return qtrue;
    case EV_INFECTED:
    case EV_NEW_HIGH_SCORE:
        /* Retail 0x1001c5a8/5c5 queues these globally, without a client filter. */
        CG_AddBufferedSound(CG_QLVoice(kind == EV_INFECTED ? "infected" : "new_high_score"));
        return qtrue;
    case EV_LIGHTNING_DISCHARGE:
        /* Retail 0x10013310: electric puff radius is integer (cells*10+48)/16,
         * duration cells+300 ms; local entity type 8 uses scale/fade drawing.
         * eventParm is an eight-bit wire field. Reject malformed native input. */
        if (event->eventParm >= 0 && event->eventParm <= 255) {
            const vec3_t stationary = {0, 0, 0};
            qhandle_t shader = trap_R_RegisterShader("models/weaphits/electric.tga");
            CG_SmokePuff(cent->lerpOrigin, stationary, (event->eventParm * 10 + 48) / 16,
                         1, 1, 1, 1, event->eventParm + 300, cg.time, 0, 0, shader)->leType = LE_SCALE_FADE;
        }
        return qtrue;
    default:
        return qfalse; /* Continue through the existing event dispatcher. */
    }
}
