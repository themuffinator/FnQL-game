/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Team end-of-match statistics: pickups, held times, average ping, player
 * counts and timeouts for both teams. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static char painted[64], redTimeouts[8] = "", serverinfo[64] = "\\g_timeoutCount\\3";
static float paintX;
static vec4_t paintColor;
static int font = -1;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) {
    if (index == CS_RED_TIMEOUTS_LEFT) return redTimeouts;
    if (index == CS_BLUE_TIMEOUTS_LEFT) return "1";
    return index == CS_SERVERINFO ? serverinfo : "";
}
void CG_SetTextFont(int value) { font = value; }
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 6; }
int CG_Text_WidthInFont(const char *text, int f, float scale, int limit) { (void)f; (void)scale; (void)limit; return (int)strlen(text) * 10; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)y; (void)scale; (void)adjust; (void)limit; (void)style;
    paintX = x; memcpy(paintColor, color, sizeof(paintColor)); snprintf(painted, sizeof(painted), "%s", text);
}
static int pics, texts;
static float picX[16], picY[16];
static char picName[16][32];
static const char *lastShader;
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { lastShader = name; return 9; }
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    (void)w; (void)h; (void)shader; picX[pics] = x; picY[pics] = y; snprintf(picName[pics++], 32, "%s", lastShader);
}
static const char *Draw(int ownerDraw, int align) {
    vec4_t color = { .5f, .5f, .5f, 1 };
    painted[0] = 0; font = -1;
    CHECK(CG_DrawTeamStat(ownerDraw, 100, 50, align, .2f, color, 0));
    return painted;
}

int main(void) {
    int i;
    CHECK(!CG_DrawTeamStat(CG_PLAYER_HEALTH, 0, 0, 0, 1, NULL, 0));
    CHECK(!CG_DrawTeamStat(CG_RED_BASESTATUS, 0, 0, 0, 1, NULL, 0) && !CG_DrawTeamStat(CG_BLUE_TEAM_MAP_PICKUPS, 0, 0, 0, 1, NULL, 0));
    for (i = 0; i < 17; ++i) { cg.scoreTeamStats[0][i] = 10 + i; cg.scoreTeamStats[1][i] = 50 + i; }
    cg.scoreTeamStats[0][11] = 125; cg.scoreTeamStats[1][16] = 0;
    /* Pickup counts in the item font. */
    CHECK(!strcmp(Draw(CG_RED_TEAM_PICKUPS_RA, ITEM_ALIGN_RIGHT), "10") && paintX == 88 && font == -1);
    CHECK(!strcmp(Draw(CG_RED_TEAM_PICKUPS_BS, 0), "15") && !strcmp(Draw(CG_RED_TEAM_PICKUPS_FLAG, 0), "19"));
    CHECK(!strcmp(Draw(CG_RED_TEAM_PICKUPS_MEDKIT, 0), "20") && !strcmp(Draw(CG_RED_TEAM_PICKUPS_INVIS, 0), "18"));
    CHECK(!strcmp(Draw(CG_BLUE_TEAM_PICKUPS_YA, 0), "51") && !strcmp(Draw(CG_BLUE_TEAM_PICKUPS_REGEN, 0), "56"));
    /* Held times in font 0, "-" for none. */
    CHECK(!strcmp(Draw(CG_RED_TEAM_TIMEHELD_QUAD, 0), "2:05") && font == 0);
    CHECK(!strcmp(Draw(CG_RED_TEAM_TIMEHELD_FLAG, 0), "0:26") && !strcmp(Draw(CG_BLUE_TEAM_TIMEHELD_FLAG, 0), "-"));
    CHECK(!strcmp(Draw(CG_BLUE_TEAM_TIMEHELD_BS, 0), "1:02") && !strcmp(Draw(CG_BLUE_TEAM_TIMEHELD_INVIS, 0), "1:05"));
    CHECK(!strcmp(CG_TeamHeldTime(9), "0:09") && !strcmp(CG_TeamHeldTime(600), "10:00"));
    /* Average ping from the team's score rows. */
    cg.numScores = 3; cg.scores[0].team = TEAM_RED; cg.scores[0].ping = 30; cg.scores[1].team = TEAM_RED; cg.scores[1].ping = 60;
    cg.scores[2].team = TEAM_BLUE; cg.scores[2].ping = 120;
    CHECK(!strcmp(Draw(CG_RED_AVG_PING, 0), "(45)") && paintColor[0] == 1 && paintColor[1] == 1 && paintColor[3] == .8f);
    Draw(CG_BLUE_AVG_PING, 0); CHECK(paintColor[0] == 1 && paintColor[1] == 0);
    cg.scores[0].ping = 10; cg.scores[1].ping = 20; Draw(CG_RED_AVG_PING, 0); CHECK(paintColor[0] == 0 && paintColor[1] == 1);
    cg.numScores = 0; CHECK(!strcmp(Draw(CG_RED_AVG_PING, 0), "(0)"));
    /* Player counts. */
    cgs.maxclients = 8; cgs.clientinfo[1].infoValid = qtrue; cgs.clientinfo[1].team = TEAM_RED;
    cgs.clientinfo[4].infoValid = qtrue; cgs.clientinfo[4].team = TEAM_RED;
    cgs.gametype = GT_CTF; cgs.teamSize = 0; CHECK(!strcmp(Draw(CG_RED_PLAYER_COUNT, 0), "(2)"));
    cgs.teamSize = 4; CHECK(!strcmp(CG_TeamPlayerCountText(TEAM_RED), "(2/4)"));
    cgs.gametype = GT_FREEZE; CHECK(!strcmp(CG_TeamPlayerCountText(TEAM_RED), "2/4 Players"));
    cgs.teamSize = 5; CHECK(!strcmp(CG_TeamPlayerCountText(TEAM_RED), "2 Players"));
    CHECK(!strcmp(CG_TeamPlayerCountText(TEAM_BLUE), "0 Players"));
    cgs.clientinfo[4].team = TEAM_BLUE; CHECK(!strcmp(CG_TeamPlayerCountText(TEAM_RED), "1 Player"));
    /* Timeouts, defaulting to g_timeoutCount; hidden when the server allows none. */
    cgs.timeoutCount = 0; CHECK(!strcmp(Draw(CG_RED_TIMEOUT_COUNT, 0), ""));
    cgs.timeoutCount = 3;
    CHECK(!strcmp(Draw(CG_RED_TIMEOUT_COUNT, 0), "TO: 3") && !strcmp(Draw(CG_BLUE_TIMEOUT_COUNT, 0), "TO: 1"));
    strcpy(redTimeouts, "0"); CHECK(CG_TeamTimeoutsLeft(TEAM_RED) == 0);
    /* Pickup strips (0x10037f80 cases 0/12). */
    memset(cg.scoreTeamStats, 0, sizeof(cg.scoreTeamStats));
    cg.scoreTeamStats[0][0] = 2; cg.scoreTeamStats[0][1] = 1; cg.scoreTeamStats[0][10] = 3;
    cg.scoreTeamStats[0][9] = 1; cg.scoreTeamStats[0][16] = 70; cg.scoreTeamStats[0][4] = 1;
    cgs.gametype = GT_CTF; pics = 0;
    { vec4_t c = { 1, 1, 1, 1 }; CG_DrawTeamPickupStrip(TEAM_RED, 10.6f, 20.4f, 12, 12, .2f, c, 0); }
    CHECK(pics == 5 && !strcmp(picName[0], "pickup_RA") && picX[0] == 10 && picY[0] == 20);
    CHECK(!strcmp(picName[1], "pickup_YA") && picX[1] == 35 && !strcmp(picName[2], "pickup_MK") && picX[2] == 60);
    CHECK(!strcmp(picName[3], "pickup_BLUEFLAG") && picX[3] == 85 && picY[3] == 13);
    CHECK(!strcmp(picName[4], "pickup_QUAD") && picX[4] == 113 && !strcmp(painted, "-"));
    cgs.gametype = GT_1FCTF; cg.scoreTeamStats[1][9] = 2; pics = 0;
    { vec4_t c = { 1, 1, 1, 1 }; CG_DrawTeamPickupStrip(TEAM_BLUE, 0, 0, 12, 12, .2f, c, 0); }
    CHECK(pics == 1 && !strcmp(picName[0], "pickup_NTRLFLAG"));
    cgs.gametype = GT_CTF; pics = 0;
    { vec4_t c = { 1, 1, 1, 1 }; CG_DrawTeamPickupStrip(TEAM_BLUE, 0, 0, 12, 12, .2f, c, 0); }
    CHECK(!strcmp(picName[0], "pickup_REDFLAG"));
    puts("team statistics: pickups, held times, ping, player counts and timeouts OK");
    return 0;
}
