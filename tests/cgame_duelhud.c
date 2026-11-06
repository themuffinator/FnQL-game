/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Duel player slot ownerdraws: slot assignment, names, scores, counters,
 * ping tint, ready/leads, per-weapon statistics and medals. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static snapshot_t snapshot;
static char names[2][40] = { "Keel", "AVeryLongDuelPlayerName" }, scores[2][16] = { "7", "-999" };
static char duelClients[2][8] = { "3", "5" };
static char painted[64];
static float paintX;
static vec4_t paintColor;
static int font = -1, picShader = -1;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) {
    if (index == CS_DUEL_NAME1 || index == CS_DUEL_NAME2) return names[index - CS_DUEL_NAME1];
    if (index == CS_DUEL_SCORE1 || index == CS_DUEL_SCORE2) return scores[index - CS_DUEL_SCORE1];
    if (index == CS_DUEL_CLIENT1 || index == CS_DUEL_CLIENT2) return duelClients[index - CS_DUEL_CLIENT1];
    return "";
}
void CG_SetTextFont(int value) { font = value; }
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 7; }
int CG_Text_WidthInFont(const char *text, int f, float scale, int limit) { (void)f; (void)scale; (void)limit; return (int)strlen(text) * 10; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)y; (void)scale; (void)adjust; (void)limit; (void)style;
    paintX = x; memcpy(paintColor, color, sizeof(paintColor)); snprintf(painted, sizeof(painted), "%s", text);
}
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { return !strcmp(name, "ui/assets/flags/none.tga") ? 61 : 63; }
static char banner[64];
qhandle_t trap_R_RegisterShader(const char *name) { snprintf(banner, sizeof(banner), "%s", name); return 62; }
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_specDuelHealthArmor, cg_specDuelHealthColor;
static int fills;
static float fillX[8], fillY[8], fillW[8], fillH[8];
static vec4_t fillColor[8];
void CG_FillRect(float x, float y, float w, float h, const float *c) {
    if (fills < 8) { fillX[fills] = x; fillY[fills] = y; fillW[fills] = w; fillH[fills] = h; memcpy(fillColor[fills++], c, sizeof(vec4_t)); }
}
void trap_R_SetColor(const float *c) { (void)c; }
void CG_OverheadHealthColor(int client, int value, vec4_t c) { (void)client; (void)value; c[0] = 1; c[1] = .5f; c[2] = 0; c[3] = 1; }
static int pics;
static float picX[8], picY[8];
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) { (void)w; (void)h; picShader = shader; if (pics < 8) { picX[pics] = x; picY[pics++] = y; } }

static const char *Draw(int ownerDraw, int align) {
    vec4_t color = { .5f, .5f, .5f, 1 };
    painted[0] = 0; font = -1; picShader = -1;
    CHECK(CG_DrawDuelStat(ownerDraw, 100, 50, 32, 32, align, .25f, color, 0));
    return painted;
}

int main(void) {
    score_t rows[2];
    cg.snap = &snapshot;
    CG_ClearDuelSlots();
    CHECK(CG_DuelSlot(0)->client == -1 && !CG_DuelSlot(2));
    CHECK(!CG_DrawDuelStat(CG_PLAYER_HEALTH, 0, 0, 0, 0, 0, 1, NULL, 0));
    memset(rows, 0, sizeof(rows));
    rows[0].client = 3; rows[0].kills = 12; rows[0].deaths = 4; rows[0].damageGiven = 2500; rows[0].accuracy = 41;
    rows[0].ping = 35; rows[0].excellentCount = 2; rows[0].impressiveCount = 5; rows[0].gauntletCount = 1;
    rows[0].weaponStats[WP_ROCKET_LAUNCHER].kills = 6; rows[0].weaponStats[WP_HEAVY_MACHINEGUN].hits = 33;
    rows[0].weaponStats[WP_CHAINGUN].shots = 90; rows[0].weaponStats[WP_GAUNTLET].damage = 300;
    rows[0].weaponStats[WP_RAILGUN].accuracy = 60;
    rows[1].client = 5; rows[1].ping = 81; rows[1].weaponStats[WP_RAILGUN].accuracy = 50;
    cgs.clientinfo[3].infoValid = qtrue; cgs.clientinfo[5].infoValid = qtrue;
    cgs.clientinfo[3].wins = 4; cgs.clientinfo[3].losses = 2; cgs.clientinfo[3].modelIcon = 77;
    CG_DuelScoresParsed(rows, 2);
    CHECK(CG_DuelSlot(0)->client == 3 && CG_DuelSlot(1)->client == 5);

    /* Names, cut past 140 units (7 per character here), and scores. */
    CHECK(!strcmp(Draw(CG_1ST_PLYR, ITEM_ALIGN_LEFT), "Keel"));
    CHECK(!strcmp(Draw(CG_2ND_PLYR, ITEM_ALIGN_LEFT), "AVeryLongDuelPlaye..."));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_SCORE, ITEM_ALIGN_RIGHT), "7") && paintX == 90 && font == 0);
    CHECK(!strcmp(Draw(CG_2ND_PLYR_SCORE, 0), "-"));
    /* Counters in font 0. */
    CHECK(!strcmp(Draw(CG_1ST_PLYR_FRAGS, 0), "12") && font == 0);
    CHECK(!strcmp(Draw(CG_1ST_PLYR_DEATHS, 0), "4") && !strcmp(Draw(CG_1ST_PLYR_DMG, 0), "2500"));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_ACC, 0), "41%") && !strcmp(Draw(CG_1ST_PLYR_WINS, 0), "4/2"));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_PING, 0), "35") && paintColor[1] == 1 && paintColor[0] == 0 && paintColor[3] == .8f);
    Draw(CG_2ND_PLYR_PING, 0); CHECK(paintColor[0] == 1 && paintColor[1] == 0);
    CHECK(!strcmp(Draw(CG_1ST_PLYR_EXCELLENT, 0), "2") && !strcmp(Draw(CG_1ST_PLYR_IMPRESSIVE, 0), "5"));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_HUMILIATION, 0), "1"));
    /* Per-weapon statistics in retail order (G MG SG GL RL LG RG PG BFG CG NG PL HMG). */
    CHECK(!strcmp(Draw(CG_1ST_PLYR_FRAGS_RL, 0), "6") && font == -1);
    CHECK(!strcmp(Draw(CG_1ST_PLYR_HITS_HMG, 0), "33") && !strcmp(Draw(CG_1ST_PLYR_SHOTS_CG, 0), "90"));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_DMG_G, 0), "300"));
    CHECK(!strcmp(Draw(CG_1ST_PLYR_ACC_RG, 0), "60%") && paintColor[0] == 1 && paintColor[3] == .8f);
    CHECK(!strcmp(Draw(CG_2ND_PLYR_ACC_RG, 0), "50%") && paintColor[0] == .5f);
    /* Ready state in warmup, standing otherwise. */
    cg.warmup = 5; snapshot.ps.stats[STAT_CLIENTS_READY] = 1 << 3;
    CHECK(!strcmp(Draw(CG_1ST_PLYR_READY, 0), "READY") && paintX == 116 && picShader == 62);
    CHECK(!strcmp(banner, "ui/assets/score/1st_plyr_ready.tga"));
    CHECK(!strcmp(Draw(CG_2ND_PLYR_READY, 0), "NOT READY") && paintX == 100 + 164 - 90);
    CHECK(!strcmp(banner, "ui/assets/score/2nd_plyr_notready.tga"));
    cg.warmup = 0; strcpy(scores[1], "3");
    CHECK(!strcmp(Draw(CG_1ST_PLYR_READY, 0), "LEADS") && !strcmp(Draw(CG_2ND_PLYR_READY, 0), "TRAILS"));
    CHECK(!strcmp(banner, "ui/assets/score/2nd_plyr_trails.tga"));
    strcpy(scores[1], "7"); CHECK(!strcmp(Draw(CG_1ST_PLYR_READY, 0), "TIED"));
    /* Pickups: rows, counts and average waits (CRT rounding). */
    { score_t *one = (score_t *)CG_DuelSlot(0);
      one->itemPickups[0] = 3; one->itemWait[0] = 12.345f; one->itemPickups[2] = 1; one->itemWait[2] = 4.5f;
      CHECK(!strcmp(Draw(CG_1ST_PLYR_PICKUPS_RA, 0), "3") && !strcmp(Draw(CG_1ST_PLYR_PICKUPS_YA, 0), "0"));
      CHECK(!strcmp(Draw(CG_1ST_PLYR_AVG_PICKUP_TIME_RA, 0), "12.35") && !strcmp(Draw(CG_1ST_PLYR_AVG_PICKUP_TIME_YA, 0), "-"));
      pics = 0; Draw(CG_1ST_PLYR_PICKUPS, 0);
      CHECK(pics == 2 && picY[0] == 50 && picY[1] == 65 && picX[1] == 100 && !strcmp(painted, "4.50") && paintX == 130); }
    /* Health/armor bars (0x10033040). */
    cgs.startingHealth = 100; cgs.clientinfo[3].handicap = 100; cg_specDuelHealthArmor.integer = 1;
    cg_entities[3].currentState.eType = ET_PLAYER; cg_entities[3].currentState.clientNum = 3;
    cg_entities[3].currentState.health = 100; cg_entities[3].currentState.armor = 50; snapshot.ps.clientNum = 0;
    cg.time = 100000; cg_entities[3].pe.painTime = 0; fills = 0;
    Draw(CG_1ST_PLYR_HEALTH_ARMOR, 0);
    CHECK(fills == 2 && fillW[0] == 16 && fillX[0] == 116 && fillY[0] == 52 && fillH[0] == 8 && fillColor[0][1] == .8f);
    CHECK(fillW[1] == 8 && fillX[1] == 124 && fillY[1] == 60 && fillH[1] == 2 && fillColor[1][2] == 1);
    /* A hit within two seconds trails the lost health in red. */
    cg_entities[3].currentState.health = 50; cg_entities[3].pe.painTime = 99000; fills = 0;
    Draw(CG_1ST_PLYR_HEALTH_ARMOR, 0);
    CHECK(fills == 3 && fillW[0] == 8 && fillW[1] == 8 && fillX[1] == 116 && fillColor[1][0] == 1 && fillColor[1][1] == 0);
    cg_specDuelHealthColor.integer = 1; fills = 0; Draw(CG_1ST_PLYR_HEALTH_ARMOR, 0);
    CHECK(fillColor[0][1] == .5f && fillColor[1][3] == .5f);
    cg_specDuelHealthArmor.integer = 0; fills = 0; Draw(CG_1ST_PLYR_HEALTH_ARMOR, 0); CHECK(!fills);
    cg_specDuelHealthArmor.integer = 1; cg_entities[3].currentState.eFlags = EF_DEAD; fills = 0;
    Draw(CG_1ST_PLYR_HEALTH_ARMOR, 0); CHECK(!fills); cg_entities[3].currentState.eFlags = 0;
    /* Pictures. */
    Draw(CG_1ST_PLYR_FLAG, 0); CHECK(picShader == 61);
    Draw(CG_1ST_PLYR_AVATAR, 0); CHECK(picShader == 77);
    /* A slot without client info stays empty outside intermission. */
    cgs.clientinfo[5].infoValid = qfalse; CHECK(!Draw(CG_2ND_PLYR_FRAGS, 0)[0]);
    cg.intermissionStarted = qtrue; CHECK(!strcmp(Draw(CG_2ND_PLYR_FRAGS, 0), "0"));
    /* Intermission with one remaining player keeps the departed one first. */
    rows[0].client = 5; CG_DuelScoresParsed(rows, 1);
    CHECK(CG_DuelSlot(0)->client == 3 && CG_DuelSlot(1)->client == 5);
    names[0][0] = 0; CHECK(!strcmp(Draw(CG_1ST_PLYR, 0), "-"));
    /* Warmup clears the slots. */
    cg.intermissionStarted = qfalse; cg.warmup = 1; rows[0].client = 9; CG_DuelScoresParsed(rows, 1);
    CHECK(CG_DuelSlot(0)->client == 9 && CG_DuelSlot(1)->client == -1);
    puts("duel HUD: slots, names, scores, counters, weapons, ready and medals OK");
    return 0;
}
