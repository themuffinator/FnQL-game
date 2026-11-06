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
#include "../docs/contract/retail_menudef.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
weaponInfo_t cg_weapons[MAX_WEAPONS];
int sortedTeamPlayers[TEAM_MAXOVERLAY], numSortedTeamPlayers;
static char storage[32768], *args[2048];
static int argc, warnings, selections;
static int requests, pictures, texts, colorCalls;
static float drawY[16];
static char drawText[16][24];
static int statRequests;
static int mutedClient = -1;
qboolean CG_ClientMuted(int client) { return client == mutedClient; }
void trap_SendClientCommand(const char *command) {
    if (!strcmp(command,"pstats")) { ++statRequests; return; }
    CHECK(!strcmp(command,"acc")); ++requests;
}
void trap_R_SetColor(const float *rgba) { (void)rgba; ++colorCalls; }
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    CHECK(x == 10 && w == 16 && h == 16 && shader > 0); drawY[pictures++] = y;
}
static int duelParses;
static gitem_t quadItem = { "item_quad", NULL, { NULL }, "icons/quad", "Quad Damage", 30, IT_POWERUP, PW_QUAD };
const gitem_t *CG_PowerupItem(int powerup) { return powerup == PW_QUAD ? &quadItem : NULL; }
static char lastRegistered[64];
qhandle_t trap_R_RegisterShader(const char *name) { snprintf(lastRegistered, sizeof(lastRegistered), "%s", name); return 90; }
static score_t testDuel[2] = { { 4 }, { 6 } };
const score_t *CG_DuelSlot(int slot) { return slot >= 0 && slot < 2 ? &testDuel[slot] : NULL; }
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_loadout;
void CG_DuelScoresParsed(const score_t *rows, int count) { (void)rows; (void)count; ++duelParses; }
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 10; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)color; CHECK(x == 10 && scale == .2f && !adjust && !limit && !style);
    drawY[texts] = y; snprintf(drawText[texts++],24,"%s",text);
}
int trap_Argc(void) { return argc; }
void trap_Argv(int index, char *out, int size) { snprintf(out,size,"%s",index >= 0 && index < argc ? args[index] : ""); }
void QDECL CG_Printf(const char *fmt, ...) { (void)fmt; ++warnings; }
void CG_SetScoreSelection(void *menu) { CHECK(!menu); ++selections; }
static void Command(const char *text) {
    char *p;
    snprintf(storage,sizeof(storage),"%s",text); argc = 0;
    for (p = strtok(storage," \t"); p; p = strtok(NULL," \t")) { CHECK(argc < 2048); args[argc++] = p; }
    CHECK(CG_ParseScoreCommand(args[0]));
}
static void Reject(const char *text) {
    cg_t *old = malloc(sizeof(cg));
    cgs_t *oldStatic = malloc(sizeof(cgs));
    int oldTeam[TEAM_MAXOVERLAY], oldCount = numSortedTeamPlayers;
    int before = warnings;
    CHECK(old && oldStatic); memcpy(old,&cg,sizeof(cg)); memcpy(oldStatic,&cgs,sizeof(cgs));
    memcpy(oldTeam,sortedTeamPlayers,sizeof(oldTeam));
    Command(text);
    CHECK(warnings == before+1 && !memcmp(old,&cg,sizeof(cg)) && !memcmp(oldStatic,&cgs,sizeof(cgs)));
    CHECK(oldCount == numSortedTeamPlayers && !memcmp(oldTeam,sortedTeamPlayers,sizeof(oldTeam)));
    free(old); free(oldStatic);
}
static void Append(char *text, int value) {
    size_t n = strlen(text); snprintf(text+n,32768-n," %d",value);
}
int main(void) {
    char command[32768]; int i,w; qhandle_t icon;
    cgs.clientinfo[7].team = TEAM_RED; cgs.clientinfo[63].team = TEAM_BLUE;
    /* Distinct values make shifted columns visible. */
    Command("scores_ffa 2 11 12 7 42 -1 3 55 6 7 8 9 10 1 12 1 14 15 7 17 1800 63 -3 45 8 44 2 3 4 5 6 0 8 0 10 11 3 13 1400");
    CHECK(!warnings && cg.numScores == 2 && cg.teamScores[1] == 12);
    CHECK(cg.scores[0].client == 7 && cg.scores[0].ping == -1 && cg.scores[0].team == TEAM_RED);
    CHECK(cg.scores[0].accuracy == 55 && cg.scores[0].damageGiven == 1800 && cg.scores[0].bestWeapon == 7);
    CHECK(cg.scores[1].score == -3 && cg.scores[1].kills == 10 && cgs.clientinfo[63].score == -3);
    Reject("scores_ffa -1 0 0"); Reject("scores_ffa 65 0 0"); Reject("scores_ffa 1 0 0 7");
    Reject("scores_ffa 0 0 0 extra"); Reject("scores_ffa 0 2147483648 0");
    Reject("scores_ffa 0 -2147483649 0"); Reject("scores_ffa 0 x 0"); Reject("scores_ffa 0 + 0");
    Reject("scores_race 1 64 2 3 4"); Reject("scores_race 2 7 2 3 4 7 5 6 7");
    Command("scores 1 1 2 7 9 10 11 12 13 14 15 16 17 18 19 1 21");
    CHECK(cg.scores[0].captures == 21 && cg.scores[0].powerUps == 13 && cgs.clientinfo[7].powerups == 13);
    Command("scores 1 1 2 7 9 10 11 12 13 14 15 16 17 18 19 1 21 1 23 24 5");
    CHECK(cg.scores[0].kills == 23 && cg.scores[0].deaths == 24 && cg.scores[0].bestWeapon == 5);
    Command("smscores 1 1 2 7 9 10 11 12 1 14 15");
    CHECK(cg.scores[0].captures == 12 && cg.scores[0].kills == 14 && !cg.scores[0].damageGiven);
    Command("scores_race 1 7 12345 3 4 63 -1 5 6");
    CHECK(cg.numScores == 1 && cg.scores[0].score == 12345);
    Command("scores_ca 1 20 30 7 2 101 102 103 104 105 106 7 108 109 110 111 112 1 0");
    CHECK(cg.scores[0].team == TEAM_BLUE && cg.scores[0].score == 101 && cg.scores[0].damageGiven == 109);
    CHECK(cg.scores[0].bestAccuracy == 108 && cg.scores[0].gauntletCount == 112);
    strcpy(command,"scores_tdm"); for(i=1;i<=28;++i) Append(command,i);
    strcat(command," 1 31 32 7 1 101 102 103 104 105 106 7 108 109 110 111 112 113");
    Command(command); CHECK(cg.scores[0].damageGiven == 113 && cg.scores[0].teamKills == 111);
    CHECK(cg.scoreTeamStats[0][8] == 9 && cg.scoreTeamStats[0][9] == 0 && cg.scoreTeamStats[0][11] == 10);
    CHECK(cg.scoreTeamStats[1][15] == 28 && cg.teamScores[1] == 32);
    strcpy(command,"scores_ft"); for(i=1;i<=28;++i) Append(command,i);
    strcat(command," 1 31 32 7 1 101 102 103 104 105 106 7 108 109 110 111 112 113 114 1");
    Command(command); CHECK(cg.scores[0].assistCount == 111 && cg.scores[0].teamKills == 112 && cg.scores[0].damageGiven == 114);
    strcpy(command,"scores_ctf"); for(i=1;i<=34;++i) Append(command,i);
    strcat(command," 1 31 32 7 1 101 102 103 104 105 106 7 108 109 110 111 112 113 1 1");
    Command(command); CHECK(cg.scores[0].defendCount == 111 && cg.scores[0].captures == 113 && cg.scoreTeamStats[1][16] == 34);
    Command("scores_rr 1 1 2 7 100 101 102 103 104 105 106 7 108 109 110 111 112 113 114 1 116 1");
    CHECK(cg.scores[0].roundScore == 101 && cg.scores[0].defendCount == 112 && cg.scores[0].gauntletCount == 113);
    strcpy(command,"scores_duel 1 7 100 101 102 103 104 105 7 107 108 109 110 1 2 1.25 3 2.50 4 3.75 5 4.00");
    for(w=1;w<=14;++w) for(i=1;i<=5;++i) Append(command,w*10+i);
    Command(command); CHECK(cg.scores[0].itemPickups[2] == 4 && cg.scores[0].itemWait[2] == 3.75f);
    CHECK(cg.scores[0].weaponStats[14].hits == 141 && cg.scores[0].weaponStats[14].kills == 145);
    CHECK(cg.scores[0].damageGiven == 107 && cg.scores[0].accuracy == 105);
    CHECK(duelParses == 1); /* valid duel updates feed the duel slots */
    { int before = warnings; score_t saved = cg.scores[0];
      args[16] = "nan"; CHECK(CG_ParseScoreCommand("scores_duel"));
      CHECK(warnings == before+1 && !memcmp(&saved,&cg.scores[0],sizeof(saved)));
    }
    Command("tdmstats 0 1 2 3 4 5 6 7 8 9 10 11");
    CHECK(cg.scores[0].suicides == 1 && cg.scores[0].damageTaken == 5 && cg.scores[0].itemPickups[0] == 6);
    Reject("tdmstats 7 1 2 3 4 5 6 7 8 9 10 11");
    Command("ctfstats 0 21 22 23 24 25 26 27 28 29 30 31 32");
    CHECK(cg.scores[0].damageGiven == 22 && cg.scores[0].powerupPickups[4] == 32);
    strcpy(command,"castats 0 11 12"); for(i=1;i<=30;++i) Append(command,i);
    Command(command); CHECK(cg.scores[0].weaponStats[15].kills == 30 && cg.scores[0].damageTaken == 12);
    strcpy(command,"scores_ad"); for(i=1;i<=22;++i) Append(command,i);
    Command(command); CHECK(cg.scoreRoundStats[19] == 20 && cg.teamScores[0] == 21 && cg.numScores == 1);
    strcpy(command,"scores_race 64"); for(i=0;i<64;++i) { Append(command,i); Append(command,i*1000); Append(command,0); Append(command,0); }
    Command(command); CHECK(cg.numScores == 64 && cg.scores[63].client == 63);
    Command("scores_ffa 0 -2147483648 +2147483647"); CHECK(cg.numScores == 0 && cg.teamScores[0] == (-2147483647-1));
    CHECK(!CG_ParseScoreCommand("unrelated"));
    cgs.gametype = GT_FFA; cgs.clientinfo[7].infoValid = qtrue; cgs.clientinfo[7].team = TEAM_FREE;
    strcpy(cgs.clientinfo[7].name,"Player Seven"); cgs.clientinfo[7].modelIcon = 81;
    cg_weapons[7].weaponIcon = 82;
    Command("scores_ffa 1 0 0 7 42 39 3 55 6 7 8 9 10 1 12 1 14 15 7 17 1800");
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,5,&icon),"Player Seven"));
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,7,&icon),"42"));
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,9,&icon),"1.8k"));
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,11,&icon),"17%"));
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,13,&icon),"39"));
    /* Column 1: muted icon, else speaking icon (retail feeders, case 1). */
    cgs.media.speakingIcon = 71; cgs.media.mutedIcon = 72;
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,1,&icon),"") && icon == -1);
    cgs.clientinfo[cg.scores[0].client].speaking = 1;
    CG_ScoreCell(FEEDER_SCOREBOARD,0,1,&icon); CHECK(icon == 71);
    mutedClient = cg.scores[0].client; CG_ScoreCell(FEEDER_SCOREBOARD,0,1,&icon); CHECK(icon == 72);
    cgs.clientinfo[cg.scores[0].client].speaking = 0; CG_ScoreCell(FEEDER_SCOREBOARD,0,1,&icon); CHECK(icon == 72);
    mutedClient = -1; CG_ScoreCell(FEEDER_SCOREBOARD,0,1,&icon); CHECK(icon == -1);
    /* Column 3: carried powerup in play, ready arrows otherwise. */
    { int client = cg.scores[0].client; snapshot_t columnSnap; snapshot_t *savedSnap = cg.snap;
      memset(&columnSnap, 0, sizeof(columnSnap)); cg.snap = &columnSnap;
      cg_entities[client].currentState.powerups = (1 << PW_FLIGHT) | (1 << PW_QUAD); cg.warmup = 0;
      CG_ScoreCell(FEEDER_SCOREBOARD,0,3,&icon); CHECK(!strcmp(lastRegistered, "icons/quad"));
      cg.warmup = 5; cg.snap->ps.stats[STAT_CLIENTS_READY] = 1 << client;
      CG_ScoreCell(FEEDER_SCOREBOARD,0,3,&icon); CHECK(!strcmp(lastRegistered, "ui/assets/score/arrowg.tga"));
      cg.snap->ps.stats[STAT_CLIENTS_READY] = 0;
      CG_ScoreCell(FEEDER_SCOREBOARD,0,3,&icon); CHECK(!strcmp(lastRegistered, "ui/assets/score/arrowr.tga"));
      cg.warmup = 0; cg_entities[client].currentState.powerups = 0; cg.snap = savedSnap; }
    CG_ScoreCell(FEEDER_SCOREBOARD,0,10,&icon); CHECK(icon == 82);
    /* Column 2: handicap below serverinfo g_startingHealth (retail 0x100262d9). */
    cgs.startingHealth = 100; cgs.clientinfo[7].handicap = 75;
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,2,&icon),"75"));
    cgs.clientinfo[7].handicap = 100; CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,2,&icon),""));
    cgs.startingHealth = 200; CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,2,&icon),"100"));
    cgs.clientinfo[7].botSkill = 3; CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,2,&icon),""));
    cgs.clientinfo[7].botSkill = 0; cgs.startingHealth = 0; CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,2,&icon),""));
    CHECK(!CG_ScoreCell(FEEDER_SCOREBOARD,-1,0,&icon)[0] && icon == -1);
    CHECK(CG_ScoreRow(FEEDER_REDTEAM_LIST,0) == -1);
    CHECK(!strcmp(CG_ScoreMenuName(qfalse),"score_menu_ffa"));
    cgs.gametype = GT_CLAN_ARENA;
    Command("scores_ca 1 20 30 7 2 101 102 103 104 105 106 7 108 109 110 111 112 1 0");
    CHECK(CG_ScoreRow(FEEDER_BLUETEAM_LIST,0) == 0);
    CHECK(!strcmp(CG_ScoreCell(FEEDER_BLUETEAM_LIST,0,10,&icon),"108%"));
    CHECK(!strcmp(CG_ScoreCell(FEEDER_BLUETEAM_LIST,0,11,&icon),"102"));
    CHECK(!strcmp(CG_ScoreMenuName(qtrue),"endteamscore_menu_ca"));
    cgs.gametype = GT_RED_ROVER; CHECK(!strcmp(CG_ScoreMenuName(qtrue),"endscore_menu_rr"));
    cgs.gametype = GT_RACE; cg.scores[0].score = 123456;
    CHECK(!strcmp(CG_ScoreCell(FEEDER_SCOREBOARD,0,4,&icon),"2:03.456"));
    { snapshot_t snap = {0}; cg.snap = &snap;
      snap.ps.persistant[PERS_TEAM] = TEAM_RED;
      CHECK(CG_ScoreVisible2(CG_SHOW_IF_PLYR_IS_ON_RED_NO_SPEC));
      CHECK(!CG_ScoreVisible2(CG_SHOW_IF_PLYR_IS_ON_BLUE_OR_SPEC));
      snap.ps.pm_type = PM_SPECTATOR;
      CHECK(!CG_ScoreVisible2(CG_SHOW_IF_PLYR_IS_ON_RED_NO_SPEC));
      /* Duel, loadout, follow and weapon-fired flags (10031a40..10031cba). */
      { int saved = cg.snap->ps.clientNum;
        cg.snap->ps.clientNum = 4; CHECK(CG_ScoreVisible2(CG_SHOW_IF_PLYR1) && !CG_ScoreVisible2(CG_SHOW_IF_PLYR2));
        cg.predictedPlayerState.pm_type = PM_SPECTATOR; CHECK(CG_ScoreVisible2(CG_SHOW_IF_PLYR2));
        cg.predictedPlayerState.pm_type = PM_NORMAL;
        CHECK(CG_ScoreVisible2(CG_SHOW_IF_LOADOUT_DISABLED) && !CG_ScoreVisible2(CG_SHOW_IF_LOADOUT_ENABLED));
        cg_loadout.integer = 1; CHECK(CG_ScoreVisible2(CG_SHOW_IF_LOADOUT_ENABLED)); cg_loadout.integer = 0;
        cg_entities[6].currentState.clientNum = 4; CHECK(CG_ScoreVisible2(CG_SHOW_IF_2ND_PLYR_FOLLOWED));
        cg_entities[4].currentState.clientNum = 9; CHECK(!CG_ScoreVisible2(CG_SHOW_IF_1ST_PLYR_FOLLOWED));
        CHECK(!CG_ScoreVisible2(CG_SHOW_IF_RL_FIRED));
        testDuel[1].weaponStats[WP_ROCKET_LAUNCHER].shots = 3; CHECK(CG_ScoreVisible2(CG_SHOW_IF_RL_FIRED));
        testDuel[0].weaponStats[WP_HEAVY_MACHINEGUN].shots = 1; CHECK(CG_ScoreVisible2(CG_SHOW_IF_HMG_FIRED));
        CHECK(!CG_ScoreVisible2(CG_SHOW_IF_CG_FIRED));
        cg.snap->ps.clientNum = saved; }
      CHECK(CG_ScoreVisible2(CG_SHOW_IF_PLYR_IS_ON_BLUE_OR_SPEC));
      cg.snap = NULL; CHECK(!CG_ScoreVisible2(CG_SHOW_IF_PLYR_IS_ON_RED_OR_SPEC));
    }
    cgs.clientinfo[7].health = 73; cgs.clientinfo[63].armor = 125;
    Command("tinfo 2 7 63");
    CHECK(numSortedTeamPlayers == 2 && sortedTeamPlayers[0] == 7 && sortedTeamPlayers[1] == 63);
    CHECK(cgs.clientinfo[7].health == 73 && cgs.clientinfo[63].armor == 125);
    Reject("tinfo -1"); Reject("tinfo 33"); Reject("tinfo 2 7"); Reject("tinfo 1 64");
    Reject("tinfo 2 7 7"); Reject("tinfo 1 2147483648"); Reject("tinfo 1 word");
    Reject("tinfo 1 7 1 2 3 15 5"); Reject("tinfo 0 extra");
    Reject("tinfo 1 7 -1 2 3 7 5"); Reject("tinfo 1 7 2147483647 2 3 7 5");
    Command("tinfo 1 7 4 73 125 7 16");
    CHECK(cgs.clientinfo[7].location == 4 && cgs.clientinfo[7].curWeapon == 7);
    CHECK(cgs.clientinfo[7].powerups == 16 && numSortedTeamPlayers == 1 && !sortedTeamPlayers[1]);
    strcpy(command,"tinfo 32"); for(i=0;i<32;++i) Append(command,i);
    Command(command); CHECK(numSortedTeamPlayers == 32 && sortedTeamPlayers[31] == 31);
    Command("tinfo 0"); CHECK(!numSortedTeamPlayers && !sortedTeamPlayers[31]);
    Command("acc 0 1 2 3 4 5 6 7 8 9 10 11 12 13 144");
    CHECK(cg.weaponAccuracy[0] == 0 && cg.weaponAccuracy[14] == 144);
    Reject("acc 0 1 2"); Reject("pstats 0 1 2 3 4 5 6 7 8 9 10 11 12 13 2147483648");
    Reject("pstats 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15");
    Command("pstats 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0");
    CHECK(cg.weaponAccuracy[0] == 14 && cg.weaponAccuracy[14] == 0);
    { snapshot_t snapshot = {0}; entityState_t *state = &snapshot.entities[0];
      snapshot.ps.clientNum = 7; snapshot.ps.stats[STAT_HEALTH] = 90;
      snapshot.ps.stats[STAT_ARMOR] = 60; snapshot.ps.weapon = WP_ROCKET_LAUNCHER;
      snapshot.ps.powerups[PW_QUAD] = 1000; snapshot.numEntities = 1;
      state->number = state->clientNum = 63; state->eType = ET_PLAYER;
      state->health = 89; state->armor = 42; state->location = 5; state->weapon = WP_RAILGUN;
      state->powerups = 1 << PW_REDFLAG;
      CG_UpdateTeamInfoFromSnapshot(&snapshot);
      CHECK(cgs.clientinfo[63].health == 89 && cgs.clientinfo[63].armor == 42 && cgs.clientinfo[63].location == 5);
      CHECK(cgs.clientinfo[63].curWeapon == WP_RAILGUN && cgs.clientinfo[63].powerups == (1 << PW_REDFLAG));
      CHECK(cgs.clientinfo[7].health == 90 && cgs.clientinfo[7].armor == 60 && cgs.clientinfo[7].powerups == (1 << PW_QUAD));
      state->number = 127; state->health = 1; CG_UpdateTeamInfoFromSnapshot(&snapshot);
      CHECK(cgs.clientinfo[63].health == 89); /* corpse cannot replace live player */
      state->number = 63; state->eType = ET_MISSILE; CG_UpdateTeamInfoFromSnapshot(&snapshot);
      CHECK(cgs.clientinfo[63].health == 89);
      state->eType = ET_INVISIBLE; state->weapon = 99; state->location = MAX_LOCATIONS;
      CG_UpdateTeamInfoFromSnapshot(&snapshot); CHECK(!cgs.clientinfo[63].location);
      CHECK(cgs.clientinfo[63].health == 1 && cgs.clientinfo[63].curWeapon == WP_NONE);
      state->clientNum = 7; state->health = 3; CG_UpdateTeamInfoFromSnapshot(&snapshot);
      CHECK(cgs.clientinfo[63].health == 1);
      state->number = 7; CG_UpdateTeamInfoFromSnapshot(&snapshot);
      CHECK(cgs.clientinfo[7].health == 90); /* own playerstate takes precedence */
      snapshot.numEntities = MAX_ENTITIES_IN_SNAPSHOT + 1; snapshot.ps.stats[STAT_HEALTH] = 6;
      CG_UpdateTeamInfoFromSnapshot(&snapshot); CG_UpdateTeamInfoFromSnapshot(NULL);
      CHECK(cgs.clientinfo[7].health == 90);
    }
    { snapshot_t snapshot = {0}; vec4_t color = {1,1,1,1};
      cg.snap = &snapshot; cg.time = 2000;
      CG_SetAccuracyOverlay(qtrue); CHECK(requests == 1 && CG_AccuracyOverlayVisible());
      cg.time = 3000; CG_SetAccuracyOverlay(qtrue); CHECK(requests == 1);
      cg.time = 3001; CG_SetAccuracyOverlay(qtrue); CHECK(requests == 2);
      CG_SetAccuracyOverlay(qfalse); CHECK(!CG_AccuracyOverlayVisible());
      snapshot.ps.pm_type = PM_SPECTATOR; cg.time = 5000;
      CG_SetAccuracyOverlay(qtrue); CHECK(!cg.showAccuracy && requests == 2);
      snapshot.ps.pm_flags = PMF_FOLLOW; CG_SetAccuracyOverlay(qtrue);
      CHECK(CG_AccuracyOverlayVisible() && requests == 3);
      cg.demoPlayback = qtrue; cg.time = 9000; CG_SetAccuracyOverlay(qtrue); CHECK(requests == 3);
      cg.demoPlayback = qfalse; cg.time = 100; CG_SetAccuracyOverlay(qtrue); CHECK(requests == 4);
      cg.snap = NULL; CHECK(!CG_AccuracyOverlayVisible());
      /* +pstats: shared one-second timer, predicted spectator check. */
      cg.time = 20000; CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 1 && cg.playerStatsHeld);
      cg.time = 20500; CG_SetAccuracyOverlay(qfalse); CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 1);
      cg.snap = &snapshot; snapshot.ps.pm_type = PM_NORMAL; snapshot.ps.pm_flags = 0;
      CG_SetAccuracyOverlay(qtrue); CHECK(requests == 4); /* +acc shares the timer */
      cg.time = 21001; CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 2);
      CG_SetPlayerStatsRequest(qfalse); CHECK(!cg.playerStatsHeld);
      cg.predictedPlayerState.pm_type = PM_SPECTATOR; cg.time = 30000;
      CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 2 && cg.playerStatsHeld);
      cg.predictedPlayerState.pm_flags = PMF_FOLLOW; CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 3);
      cg.demoPlayback = qtrue; cg.time = 40000; CG_SetPlayerStatsRequest(qtrue); CHECK(statRequests == 3);
      cg.demoPlayback = qfalse; cg.snap = NULL;
      memset(&cg.predictedPlayerState, 0, sizeof(cg.predictedPlayerState));
      memset(cg_weapons,0,sizeof(cg_weapons));
      cg_weapons[WP_GAUNTLET].weaponIcon = 1; cg_weapons[WP_MACHINEGUN].weaponIcon = 2;
      cg_weapons[WP_GRAPPLING_HOOK].weaponIcon = 10; cg_weapons[WP_HEAVY_MACHINEGUN].weaponIcon = 14;
      cg.weaponAccuracy[WP_MACHINEGUN] = 37; cg.weaponAccuracy[WP_HEAVY_MACHINEGUN] = 101;
      CG_DrawAccuracyColumn(qtrue,10,20,16,18,.2f,color);
      CHECK(pictures == 2 && colorCalls == 2 && drawY[0] == 20 && drawY[1] == 38);
      CG_DrawAccuracyColumn(qfalse,10,20,16,18,.2f,color);
      CHECK(texts == 2 && !strcmp(drawText[0],"37%") && !strcmp(drawText[1],"101%"));
      CHECK(drawY[0] == 20 && drawY[1] == 38);
      cg.selectedScore = 1; cg.numScores = 2; cg.scores[1].accuracy = 43;
      CHECK(!strcmp(CG_SelectedPlayerAccuracy(),"43%"));
      cg.selectedScore = -1; CHECK(!strcmp(CG_SelectedPlayerAccuracy(),"0%"));
      cg.selectedScore = MAX_CLIENTS; CHECK(!strcmp(CG_SelectedPlayerAccuracy(),"0%"));
    }
    { /* End-of-match stats tables (feeders 17/18, retail 0x10028830). */
      score_t *row = &cg.scores[0]; qhandle_t icon;
#define CELL(f, c) CG_ScoreCell(f, 0, c, &icon)
#define RED(c) CELL(FEEDER_REDTEAM_STATS, c)
      memset(cg.scores, 0, sizeof(cg.scores)); cg.numScores = 1;
      row->client = 9; row->team = TEAM_RED; row->score = 31; row->kills = 20; row->deaths = 12;
      row->teamKilled = 2; row->suicides = 3; row->teamKills = 1; row->damageGiven = 4500; row->damageTaken = 3900;
      row->accuracy = 47; row->time = 9;
      for (i = 0; i < 4; ++i) row->itemPickups[i] = 5 + i;
      for (i = 0; i < 5; ++i) row->powerupPickups[i] = 1 + i;
      row->weaponStats[WP_GAUNTLET].kills = 2;
      row->weaponStats[WP_RAILGUN].kills = 4; row->weaponStats[WP_RAILGUN].accuracy = 55;
      row->weaponStats[WP_HEAVY_MACHINEGUN].kills = 6; row->weaponStats[WP_HEAVY_MACHINEGUN].accuracy = 33;
      cgs.clientinfo[9].infoValid = qtrue; cgs.clientinfo[9].team = TEAM_RED; strcpy(cgs.clientinfo[9].name, "Short");
      /* TDM and Freeze Tag (0x10028300). */
      cgs.gametype = GT_TEAM;
      CHECK(!strcmp(RED(0), "Short") && !strcmp(RED(1), "^331") && !strcmp(RED(2), "20"));
      CHECK(!strcmp(RED(3), "7") && !strcmp(RED(4), "3") && !strcmp(RED(5), "1") && !strcmp(RED(6), "2"));
      CHECK(!strcmp(RED(7), "9") && !strcmp(RED(8), "4500") && !strcmp(RED(9), "3900") && !strcmp(RED(10), "47%"));
      CHECK(!strcmp(RED(11), "5") && !strcmp(RED(14), "8") && !strcmp(RED(15), "1") && !strcmp(RED(16), "2"));
      CHECK(!strcmp(RED(17), "   9") && !strcmp(RED(18), "") && icon == -1);
      CHECK(!strcmp(CELL(FEEDER_BLUETEAM_STATS, 2), "")); /* no blue row */
      cgs.gametype = GT_FREEZE; CHECK(!strcmp(RED(7), "9"));
      row->score = SCORE_NOT_PRESENT; CHECK(!strcmp(RED(1), "^3-"));
      row->score = -999; CHECK(!strcmp(RED(1), "^3-")); row->score = 31;
      /* Long names are cut under 92 units and given "...". */
      strcpy(cgs.clientinfo[9].name, "AVeryLongPlayerName"); CHECK(!strcmp(RED(0), "AVeryLong..."));
      strcpy(cgs.clientinfo[9].name, "NineChars"); CHECK(!strcmp(RED(0), "NineChars"));
      /* Clan Arena (0x10027720): per-weapon kills and accuracy. */
      cgs.gametype = GT_CLAN_ARENA;
      CHECK(!strcmp(RED(4), "4500") && !strcmp(RED(5), "3900") && !strcmp(RED(6), "47%") && !strcmp(RED(7), "2"));
      CHECK(!strcmp(RED(13), "^34 ^755%") && !strcmp(RED(15), "^36 ^733%") && !strcmp(RED(8), "^30 ^70%"));
      CHECK(!strcmp(RED(16), "   9") && !strcmp(RED(17), ""));
      /* Flag and objective modes (0x10026cf0). */
      cgs.gametype = GT_CTF;
      CHECK(!strcmp(RED(4), "3") && !strcmp(RED(5), "8") && !strcmp(RED(6), "4500") && !strcmp(RED(7), "3900"));
      CHECK(!strcmp(RED(8), "47%") && !strcmp(RED(9), "5") && !strcmp(RED(12), "8"));
      CHECK(!strcmp(RED(13), "1") && !strcmp(RED(17), "5") && !strcmp(RED(18), "   9") && !strcmp(RED(19), ""));
      cgs.gametype = GT_DOMINATION; CHECK(!strcmp(RED(18), "   9"));
      /* Overload and Red Rover have no stats table; spectators never list. */
      cgs.gametype = GT_OBELISK; CHECK(!strcmp(RED(2), ""));
      cgs.gametype = GT_RED_ROVER; CHECK(!strcmp(RED(2), ""));
      cgs.gametype = GT_TEAM; cgs.clientinfo[9].team = TEAM_SPECTATOR; CHECK(!strcmp(RED(2), ""));
#undef RED
#undef CELL
    }
    puts("score/team/accuracy commands, accuracy panel and snapshot HUD vitals: layouts, rejection and bounds OK");
    return 0;
}
