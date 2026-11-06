/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail local player-state sounds (0x100435b0): hit and kill beeps,
 * medals, player-event voices, the flag voice, lead changes and frag
 * warnings. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_hitBeep, cg_killBeep, cg_killBeepVolume, cg_announcerLeadsVO, cg_announcerTiesVO;
static char registered[16][MAX_QPATH], voice[MAX_QPATH];
static int registrations, localSound, localChannel, volumeSound, buffered, bufferedCount;
static int rewardSound[8], rewardShader[8], rewardCount[8], rewards, clockChecks;
static char rewardVoice[8][MAX_QPATH];
static float volume;

sfxHandle_t trap_S_RegisterSound(const char *name, qboolean compressed) {
    CHECK(!compressed && registrations < 16);
    snprintf(registered[registrations], MAX_QPATH, "%s", name);
    return ++registrations;
}
void trap_S_StartLocalSound(sfxHandle_t sound, int channel) { localSound = sound; localChannel = channel; }
void trap_S_StartLocalSoundVolume(sfxHandle_t sound, int channel, float level) {
    CHECK(channel == CHAN_LOCAL_SOUND); volumeSound = sound; volume = level;
}
sfxHandle_t CG_QLVoice(const char *file) { snprintf(voice, sizeof(voice), "%s", file); return 100; }
void CG_AddBufferedSound(sfxHandle_t sound) { buffered = sound; ++bufferedCount; }
void CG_PushReward(sfxHandle_t sound, qhandle_t shader, int count) {
    CHECK(rewards < 8);
    snprintf(rewardVoice[rewards], MAX_QPATH, "%s", sound == 100 ? voice : "");
    rewardSound[rewards] = sound; rewardShader[rewards] = shader; rewardCount[rewards++] = count;
}
void CG_MatchClockWarnings(void) { ++clockChecks; }
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }

static playerState_t ps, ops;

static void Run(void) { CG_QLCheckLocalSounds(&ps, &ops); }
static void Clear(void) {
    localSound = localChannel = volumeSound = buffered = bufferedCount = rewards = 0;
    voice[0] = 0;
}
static void Media(int gametype) {
    registrations = 0; cgs.gametype = gametype;
    CG_RegisterLocalSounds();
}
static void Sync(void) { ops = ps; }

int main(void) {
    int tier;
    /* Registration: hit0..3 and eight kill beeps everywhere, the teammate
     * hit from TDM up and the capture medal sound in flag/objective modes. */
    Media(GT_FFA);
    CHECK(registrations == 12 && !strcmp(registered[0], "sound/feedback/hit0.ogg"));
    CHECK(!strcmp(registered[3], "sound/feedback/hit3.ogg"));
    CHECK(!strcmp(registered[4], "sound/feedback/impact1.ogg") && !strcmp(registered[9], "sound/feedback/impact6.ogg"));
    CHECK(!strcmp(registered[10], "sound/world/bell_01.ogg") && !strcmp(registered[11], "sound/misc/chaching.ogg"));
    Media(GT_TEAM); CHECK(registrations == 13 && !strcmp(registered[12], "sound/feedback/hit_teammate.ogg"));
    Media(GT_CLAN_ARENA); CHECK(registrations == 13);
    Media(GT_CTF);
    CHECK(registrations == 14 && !strcmp(registered[13], "sound/teamplay/flagcapture_yourteam.ogg"));
    Media(GT_DOMINATION); CHECK(registrations == 14);
    Media(GT_FREEZE); CHECK(registrations == 13);

    /* Hit beeps within one life (TDM handles: hit0..3 = 1..4,
     * impact1..6 = 5..10, bell = 11, chaching = 12, teammate = 13). */
    Media(GT_TEAM); memset(&ps, 0, sizeof(ps)); Sync(); Clear();
    cg_hitBeep.integer = 1; cg_killBeep.integer = 0;
    ps.persistant[PERS_HITS] = 1; Run();
    CHECK(localSound == 2 && localChannel == CHAN_LOCAL_SOUND); /* hit1 */
    cg_hitBeep.integer = 2;
    for (tier = 0; tier < 4; ++tier) {
        Sync(); ps.persistant[PERS_HITS]++; ps.generic1 = (tier << 6) | 0x3f; Clear(); Run();
        CHECK(localSound == 1 + tier);
    }
    cg_hitBeep.integer = 3;
    for (tier = 0; tier < 4; ++tier) {
        Sync(); ps.persistant[PERS_HITS]++; ps.generic1 = tier << 6; Clear(); Run();
        CHECK(localSound == 4 - tier); /* inverted */
    }
    cg_hitBeep.integer = 2; Sync(); ps.persistant[PERS_HITS]++; ps.generic1 = -64; Clear(); Run();
    CHECK(localSound == 1); /* malformed tier falls back to hit0 */
    cg_hitBeep.integer = 0; Sync(); ps.persistant[PERS_HITS]++; Clear(); Run(); CHECK(!localSound);
    /* Fewer hits: a teammate was hit (any nonzero mode). */
    Sync(); ps.persistant[PERS_HITS] -= 2; Clear(); Run(); CHECK(!localSound);
    cg_hitBeep.integer = 1; Sync(); ps.persistant[PERS_HITS] -= 2; Clear(); Run(); CHECK(localSound == 13);
    /* Kill beeps replace the hit beep, at s_killBeepVolume. */
    cg_killBeep.integer = 7; cg_killBeepVolume.value = 2.5f;
    Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(volumeSound == 11 && volume == 2.5f && !localSound);
    cg_killBeep.integer = 8; Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(volumeSound == 12);
    cg_killBeep.integer = 2; Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(volumeSound == 6);
    cg_killBeep.integer = 1; Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(volumeSound == 5);
    cg_killBeep.integer = 9; Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(volumeSound == 5); /* out of range: impact1, as retail's default */
    cg_killBeep.integer = 0; Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_KILL_COUNT]++; Clear(); Run();
    CHECK(!volumeSound && localSound == 2); /* hit beep instead */
    cg_killBeep.integer = 7; Sync(); ps.persistant[PERS_HITS]++; Clear(); Run();
    CHECK(!volumeSound && localSound == 2); /* no kill this frame */
    /* A new life (or team change) never beeps. */
    Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_SPAWN_COUNT]++; Clear(); Run();
    CHECK(!localSound && !volumeSound);
    cg_hitBeep.integer = 1; Media(GT_FFA);
    Sync(); ps.persistant[PERS_HITS] -= 1; Clear(); Run(); CHECK(!localSound); /* no teammate sound in FFA */

    /* Medals: nonzero changed counters, retail order, first-award voices
     * and three random variants afterwards. */
    Media(GT_CTF); memset(&ps, 0, sizeof(ps)); Sync(); Clear();
    cgs.media.medalCapture = 21; cgs.media.medalAssist = 22; cgs.media.medalDefend = 23;
    cgs.media.medalImpressive = 24; cgs.media.medalExcellent = 25; cgs.media.medalGauntlet = 26;
    ps.persistant[PERS_GAUNTLET_FRAG_COUNT] = 1; ps.persistant[PERS_IMPRESSIVE_COUNT] = 1;
    ps.persistant[PERS_EXCELLENT_COUNT] = 1; ps.persistant[PERS_DEFEND_COUNT] = 2;
    ps.persistant[PERS_ASSIST_COUNT] = 3; ps.persistant[PERS_CAPTURES] = 4;
    Run();
    CHECK(rewards == 6);
    CHECK(rewardSound[0] == 14 && rewardShader[0] == 21 && rewardCount[0] == 4);
    CHECK(rewardShader[1] == 22 && rewardCount[1] == 3 && !strcmp(rewardVoice[1], "assist"));
    CHECK(rewardShader[2] == 23 && rewardCount[2] == 2 && !strcmp(rewardVoice[2], "defense"));
    CHECK(rewardShader[3] == 24 && !strcmp(rewardVoice[3], "first_impressive"));
    CHECK(rewardShader[4] == 25 && !strcmp(rewardVoice[4], "first_excellent"));
    CHECK(rewardShader[5] == 26 && !strcmp(rewardVoice[5], "first_gauntlet"));
    for (tier = 0; tier < 30; ++tier) {
        Sync(); ps.persistant[PERS_IMPRESSIVE_COUNT]++; ps.persistant[PERS_EXCELLENT_COUNT]++;
        ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++; Clear(); Run();
        CHECK(rewards == 3);
        CHECK(!strncmp(rewardVoice[0], "impressive", 10) && rewardVoice[0][10] >= '1' && rewardVoice[0][10] <= '3' && !rewardVoice[0][11]);
        CHECK(!strncmp(rewardVoice[1], "excellent", 9) && rewardVoice[1][9] >= '1' && rewardVoice[1][9] <= '3');
        CHECK(!strncmp(rewardVoice[2], "humiliation", 11) && rewardVoice[2][11] >= '1' && rewardVoice[2][11] <= '3');
    }
    Sync(); ps.persistant[PERS_CAPTURES] = 0; ps.persistant[PERS_ASSIST_COUNT] = 0; Clear(); Run();
    CHECK(!rewards); /* resets are silent */
    Media(GT_FFA); Sync(); ps.persistant[PERS_CAPTURES] = 1; Clear(); Run();
    CHECK(rewards == 1 && !rewardSound[0]); /* medal without the CTF effect */

    /* Player-event voices: the first changed bit wins. */
    Sync(); ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD | PLAYEREVENT_HOLYSHIT; Clear(); Run();
    CHECK(bufferedCount == 1 && !strcmp(voice, "denied"));
    Sync(); ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD; Clear(); Run();
    CHECK(!strncmp(voice, "humiliation", 11));
    Sync(); ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT; Clear(); Run();
    CHECK(!strcmp(voice, "holy_shit"));

    /* Flag voice: CTF, One Flag and A&D only. */
    Media(GT_CTF); Sync(); ps.powerups[PW_REDFLAG] = 1; Clear(); Run();
    CHECK(bufferedCount == 1 && !strcmp(voice, "you_have_flag"));
    Sync(); Clear(); Run(); CHECK(!bufferedCount); /* still carrying */
    cgs.gametype = GT_1FCTF; Sync(); ps.powerups[PW_NEUTRALFLAG] = 5; Clear(); Run();
    CHECK(!strcmp(voice, "you_have_flag"));
    cgs.gametype = GT_OBELISK; Sync(); ps.powerups[PW_BLUEFLAG] = 1; Clear(); Run(); CHECK(!bufferedCount);
    memset(ps.powerups, 0, sizeof(ps.powerups));

    /* FFA lead changes. */
    cgs.gametype = GT_FFA; cg_announcerLeadsVO.integer = cg_announcerTiesVO.integer = 1;
    ps.persistant[PERS_RANK] = 1; Sync(); ps.persistant[PERS_RANK] = 0; Clear(); Run();
    CHECK(!strcmp(voice, "lead_taken"));
    Sync(); ps.persistant[PERS_RANK] = RANK_TIED_FLAG; Clear(); Run(); CHECK(!strcmp(voice, "lead_tied"));
    Sync(); ps.persistant[PERS_RANK] = 2; Clear(); Run(); CHECK(!strcmp(voice, "lead_lost"));
    Sync(); ps.persistant[PERS_RANK] = 3; Clear(); Run(); CHECK(!bufferedCount); /* was not leading */
    Sync(); ps.persistant[PERS_RANK] = 0; cg_announcerLeadsVO.integer = 0; Clear(); Run(); CHECK(!bufferedCount);
    Sync(); ps.persistant[PERS_RANK] = RANK_TIED_FLAG; cg_announcerTiesVO.integer = 0; Clear(); Run(); CHECK(!bufferedCount);
    cg_announcerLeadsVO.integer = cg_announcerTiesVO.integer = 1;
    Sync(); ps.persistant[PERS_RANK] = 0; cg.warmup = 5000; Clear(); Run(); CHECK(!bufferedCount);
    cg.warmup = 0;
    Sync(); ps.persistant[PERS_RANK] = 4; ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_FIRSTFRAG; Clear(); Run();
    CHECK(!bufferedCount);
    cgs.gametype = GT_TEAM; Sync(); ps.persistant[PERS_RANK] = 0; Clear(); Run(); CHECK(!bufferedCount);

    /* Frag warnings read CS_SCORES1 only, up to team deathmatch. */
    cgs.gametype = GT_FFA; cgs.fraglimit = 10; cg.fraglimitWarnings = 0; Sync();
    cgs.scores1 = 7; Clear(); Run(); CHECK(!strcmp(voice, "3_frags") && cg.fraglimitWarnings == 1);
    Clear(); Run(); CHECK(!bufferedCount);
    cgs.scores1 = 8; Clear(); Run(); CHECK(!strcmp(voice, "2_frags") && cg.fraglimitWarnings == 3);
    cgs.scores1 = 9; Clear(); Run(); CHECK(!strcmp(voice, "1_frag") && cg.fraglimitWarnings == 7);
    Clear(); Run(); CHECK(!bufferedCount);
    cg.fraglimitWarnings = 0; cgs.scores1 = 9; cgs.fraglimit = 0; Clear(); Run(); CHECK(!bufferedCount);
    cgs.gametype = GT_TEAM; cgs.fraglimit = 10; cgs.scores1 = 0; cgs.scores2 = 9; Clear(); Run();
    CHECK(!bufferedCount); /* blue's score is not consulted */
    cgs.gametype = GT_CLAN_ARENA; cgs.scores1 = 9; Clear(); Run(); CHECK(!bufferedCount);
    cgs.gametype = GT_FFA; cgs.fraglimit = 2; cgs.scores1 = 0; Clear(); Run(); CHECK(!bufferedCount);

    /* Intermission keeps hit beeps and skips the rest (0x100436ea). */
    tier = clockChecks; cg.intermissionStarted = qtrue; cgs.scores1 = 1; cg.fraglimitWarnings = 0;
    Sync(); ps.persistant[PERS_HITS]++; ps.persistant[PERS_CAPTURES]++; Clear(); Run();
    CHECK(localSound == 2 && !rewards && !bufferedCount && clockChecks == tier);
    cg.intermissionStarted = qfalse; Sync(); Clear(); Run(); CHECK(clockChecks == tier + 1);

    puts("Local sounds: hit/kill beeps, medals, player events, flag voice, lead changes, frag warnings OK");
    return 0;
}
