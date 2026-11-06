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

/* Local player-state sounds, retail cgame 0x100435b0 (called from the
 * player-state transition 0x10043b60 for non-spectators outside
 * intermission). Handles registered by retail 0x10021bec..0x10021cf1,
 * 0x10021141 and 0x10021403; voices resolve through CG_QLVoice. */
static struct {
    sfxHandle_t hit[4], killBeep[8], hitTeammate, capture;
} localSounds;

void CG_RegisterLocalSounds(void) {
    static const char *const killBeeps[8] = {
        "sound/feedback/impact1.ogg", "sound/feedback/impact2.ogg", "sound/feedback/impact3.ogg",
        "sound/feedback/impact4.ogg", "sound/feedback/impact5.ogg", "sound/feedback/impact6.ogg",
        "sound/world/bell_01.ogg", "sound/misc/chaching.ogg"
    };
    char path[MAX_QPATH];
    int i;
    memset(&localSounds, 0, sizeof(localSounds));
    for (i = 0; i < 4; ++i) {
        Com_sprintf(path, sizeof(path), "sound/feedback/hit%d.ogg", i);
        localSounds.hit[i] = trap_S_RegisterSound(path, qfalse);
    }
    for (i = 0; i < 8; ++i) localSounds.killBeep[i] = trap_S_RegisterSound(killBeeps[i], qfalse);
    if (cgs.gametype >= GT_TEAM)
        localSounds.hitTeammate = trap_S_RegisterSound("sound/feedback/hit_teammate.ogg", qfalse);
    /* The capture medal's sound is the flag-capture effect (0x10021403),
     * registered for CTF, One Flag, Overload, Harvester, Domination, A&D. */
    if ((cgs.gametype >= GT_CTF && cgs.gametype <= GT_HARVESTER) ||
        cgs.gametype == GT_DOMINATION || cgs.gametype == GT_ATTACK_DEFEND)
        localSounds.capture = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.ogg", qfalse);
}

/* 0x100435c5..0x100436de: hits within one life. A kill (kill counter up)
 * with cg_killBeep 1..8 plays that beep at s_killBeepVolume; otherwise
 * cg_hitBeep 1 is the single tone, 2 picks hit0..3 from the hit tier the
 * server keeps in generic1 bits 6-7, and 3 inverts that tier. A drop in
 * hits means a teammate was hit (any nonzero cg_hitBeep). */
static void CG_HitSounds(const playerState_t *ps, const playerState_t *ops) {
    int mode = cg_hitBeep.integer;
    if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT]) return;
    if (ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS]) {
        int beep = cg_killBeep.integer;
        if (beep >= 1 && ps->persistant[PERS_KILL_COUNT] > ops->persistant[PERS_KILL_COUNT]) {
            sfxHandle_t sound = beep >= 2 && beep <= 8 ? localSounds.killBeep[beep - 1] : localSounds.killBeep[0];
            if (sound) trap_S_StartLocalSoundVolume(sound, CHAN_LOCAL_SOUND, cg_killBeepVolume.value);
        } else if (mode >= 2) {
            int tier = ps->generic1 >> 6;
            if (mode == 3) tier = 3 - tier;
            tier = tier >= 1 && tier <= 3 ? tier : 0;
            if (localSounds.hit[tier]) trap_S_StartLocalSound(localSounds.hit[tier], CHAN_LOCAL_SOUND);
        } else if (mode == 1 && localSounds.hit[1]) {
            trap_S_StartLocalSound(localSounds.hit[1], CHAN_LOCAL_SOUND);
        }
    } else if (ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS] && mode && localSounds.hitTeammate) {
        trap_S_StartLocalSound(localSounds.hitTeammate, CHAN_LOCAL_SOUND);
    }
}

static sfxHandle_t CG_VariantVoice(int count, const char *first, const char *name) {
    char voice[MAX_QPATH];
    if (count == 1 && first) return CG_QLVoice(first);
    Com_sprintf(voice, sizeof(voice), "%s%d", name, 1 + rand() % 3);
    return CG_QLVoice(voice);
}

/* A medal counter that changed to a nonzero value (0x10043700..0x1004383e).
 * The capture medal uses the flag-capture effect, assist and defense one
 * voice each; the others have a first-award voice and then one of three
 * variants at random. Voices resolve only when a medal is awarded. */
static void CG_Medal(const playerState_t *ps, const playerState_t *ops, int counter, qhandle_t shader,
                     sfxHandle_t effect, const char *voice, const char *first, const char *variants) {
    int count = ps->persistant[counter];
    sfxHandle_t sound = effect;
    if (!count || count == ops->persistant[counter]) return;
    if (voice) sound = CG_QLVoice(voice);
    else if (variants) sound = CG_VariantVoice(count, first, variants);
    CG_PushReward(sound, shader, count);
}

static void CG_Rewards(const playerState_t *ps, const playerState_t *ops) {
    int changed;
    CG_Medal(ps, ops, PERS_CAPTURES, cgs.media.medalCapture, localSounds.capture, NULL, NULL, NULL);
    CG_Medal(ps, ops, PERS_ASSIST_COUNT, cgs.media.medalAssist, 0, "assist", NULL, NULL);
    CG_Medal(ps, ops, PERS_DEFEND_COUNT, cgs.media.medalDefend, 0, "defense", NULL, NULL);
    CG_Medal(ps, ops, PERS_IMPRESSIVE_COUNT, cgs.media.medalImpressive, 0, NULL, "first_impressive", "impressive");
    CG_Medal(ps, ops, PERS_EXCELLENT_COUNT, cgs.media.medalExcellent, 0, NULL, "first_excellent", "excellent");
    CG_Medal(ps, ops, PERS_GAUNTLET_FRAG_COUNT, cgs.media.medalGauntlet, 0, NULL, "first_gauntlet", "humiliation");
    /* 0x10043846: player-event bits, first changed bit wins. */
    changed = ps->persistant[PERS_PLAYEREVENTS] ^ ops->persistant[PERS_PLAYEREVENTS];
    if (changed & PLAYEREVENT_DENIEDREWARD) CG_AddBufferedSound(CG_QLVoice("denied"));
    else if (changed & PLAYEREVENT_GAUNTLETREWARD) CG_AddBufferedSound(CG_VariantVoice(0, NULL, "humiliation"));
    else if (changed & PLAYEREVENT_HOLYSHIT) CG_AddBufferedSound(CG_QLVoice("holy_shit"));
}

/* 0x10043893: "you have the flag" when the snapshot player gains any flag,
 * checked in CTF, One Flag, Overload, Harvester and A&D but registered
 * (0x10021790) only for CTF, One Flag and A&D. */
static void CG_FlagPickup(const playerState_t *ps, const playerState_t *ops) {
    int flag;
    if (cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF && cgs.gametype != GT_ATTACK_DEFEND) return;
    for (flag = PW_REDFLAG; flag <= PW_NEUTRALFLAG; ++flag) {
        if (ps->powerups[flag] && ps->powerups[flag] != ops->powerups[flag]) {
            CG_AddBufferedSound(CG_QLVoice("you_have_flag"));
            return;
        }
    }
}

/* 0x100438ed: individual-mode lead changes outside warmup, skipped on the
 * frame a first-frag event toggles. */
static void CG_LeadChanges(const playerState_t *ps, const playerState_t *ops) {
    int rank = ps->persistant[PERS_RANK], old = ops->persistant[PERS_RANK];
    if (cg.warmup || ((ps->persistant[PERS_PLAYEREVENTS] ^ ops->persistant[PERS_PLAYEREVENTS]) &
                      PLAYEREVENT_FIRSTFRAG) || rank == old || cgs.gametype >= GT_TEAM) return;
    if (!rank) {
        if (cg_announcerLeadsVO.integer) CG_AddBufferedSound(CG_QLVoice("lead_taken"));
    } else if (rank == RANK_TIED_FLAG) {
        if (cg_announcerTiesVO.integer) CG_AddBufferedSound(CG_QLVoice("lead_tied"));
    } else if (!(old & ~RANK_TIED_FLAG) && cg_announcerLeadsVO.integer) {
        CG_AddBufferedSound(CG_QLVoice("lead_lost"));
    }
}

/* 0x10043aa2: frag warnings read only CS_SCORES1 and stop at team
 * deathmatch; the first matching warning plays. */
static void CG_FragWarnings(void) {
    int limit = cgs.fraglimit, high = cgs.scores1;
    if (limit <= 0 || cgs.gametype > GT_TEAM) return;
    if (!(cg.fraglimitWarnings & 4) && high == limit - 1) {
        cg.fraglimitWarnings |= 7;
        CG_AddBufferedSound(CG_QLVoice("1_frag"));
    } else if (limit > 2 && !(cg.fraglimitWarnings & 2) && high == limit - 2) {
        cg.fraglimitWarnings |= 3;
        CG_AddBufferedSound(CG_QLVoice("2_frags"));
    } else if (limit > 3 && !(cg.fraglimitWarnings & 1) && high == limit - 3) {
        cg.fraglimitWarnings |= 1;
        CG_AddBufferedSound(CG_QLVoice("3_frags"));
    }
}

void CG_QLCheckLocalSounds(const playerState_t *ps, const playerState_t *ops) {
    CG_HitSounds(ps, ops);
    if (cg.intermissionStarted) return;
    CG_Rewards(ps, ops);
    CG_FlagPickup(ps, ops);
    CG_LeadChanges(ps, ops);
    CG_MatchClockWarnings();
    CG_FragWarnings();
}
