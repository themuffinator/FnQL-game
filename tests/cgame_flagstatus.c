/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail flag-status ownerdraws 283/288/310/315 (0x10030a80) and 54
 * (0x10030ff0) with the gfx/2d/flag_status icon set. */
#include "../code/cgame/cg_local.h"
#include "../docs/contract/retail_menudef.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static char registered[13][MAX_QPATH];
static int registrations, draws, colorSets, colorClears;
static float drawnY;
static qhandle_t drawn;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) {
    CHECK(registrations < 13);
    snprintf(registered[registrations], MAX_QPATH, "%s", name);
    return 50 + registrations++;
}
void trap_R_SetColor(const float *color) {
    if (color) { CHECK(color[0] == 1 && color[1] == 1 && color[2] == 1 && color[3] == 1); ++colorSets; }
    else ++colorClears;
}
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    CHECK(x == 10 && w == 30 && h == 40); drawnY = y; drawn = shader; ++draws;
}

/* handle = 50 + team * 4 + state; team 0 neutral, 1 red, 2 blue;
 * state 0 at base, 1 taken, 2 dropped, 3 stolen. */
static int Team(int ownerDraw) {
    int before = draws;
    drawn = 0;
    CHECK(CG_DrawTeamFlagStatus(ownerDraw, 10, 20, 30, 40));
    return draws == before ? -1 : drawn;
}

int main(void) {
    static const int noFlags[] = { GT_FFA, GT_TOURNAMENT, GT_RACE, GT_TEAM, GT_CLAN_ARENA, GT_FREEZE, GT_RED_ROVER };
    int i;
    CG_RegisterFlagStatusIcons();
    CHECK(registrations == 13 && !strcmp(registered[12], "gfx/2d/flag_status/track_pointer.tga"));
    CHECK(CG_FlagTrackPointer() == 62 && CG_FlagStatusIcon(1, 3) == 57 && CG_FlagStatusIcon(3, 0) == 0);
    CHECK(!strcmp(registered[0], "gfx/2d/flag_status/flag_at_base.tga"));
    CHECK(!strcmp(registered[3], "gfx/2d/flag_status/flag_stolen.tga"));
    CHECK(!strcmp(registered[5], "gfx/2d/flag_status/red_flag_taken.tga"));
    CHECK(!strcmp(registered[10], "gfx/2d/flag_status/blue_flag_dropped.tga"));

    CHECK(!CG_DrawTeamFlagStatus(CG_ONEFLAG_STATUS, 10, 20, 30, 40)); /* not ours */

    /* CTF: flag status shows at base / taken / dropped, base status only
     * at base / taken. */
    cgs.gametype = GT_CTF;
    cgs.redflag = FLAG_ATBASE; cgs.blueflag = FLAG_TAKEN;
    CHECK(Team(CG_RED_FLAGSTATUS) == 54 && Team(CG_BLUE_FLAGSTATUS) == 59);
    CHECK(Team(CG_RED_BASESTATUS) == 54 && Team(CG_BLUE_BASESTATUS) == 59);
    cgs.redflag = FLAG_DROPPED;
    CHECK(Team(CG_RED_FLAGSTATUS) == 56 && Team(CG_RED_BASESTATUS) == 55);
    CHECK(drawnY == 20 && colorSets == colorClears);
    cgs.redflag = 7; /* malformed status */
    CHECK(Team(CG_RED_FLAGSTATUS) == 54 && Team(CG_RED_BASESTATUS) == 55);
    /* Harvester and Domination draw without setting a color. */
    i = colorSets; cgs.gametype = GT_HARVESTER; cgs.blueflag = FLAG_ATBASE;
    CHECK(Team(CG_BLUE_FLAGSTATUS) == 58 && colorSets == i);
    cgs.gametype = GT_DOMINATION; CHECK(Team(CG_BLUE_FLAGSTATUS) == 58 && colorSets == i);
    cgs.gametype = GT_OBELISK; CHECK(Team(CG_BLUE_FLAGSTATUS) == 58 && colorSets == i + 1);
    cgs.gametype = GT_ATTACK_DEFEND; CHECK(Team(CG_BLUE_FLAGSTATUS) == 58);
    for (i = 0; i < ARRAY_LEN(noFlags); ++i) {
        cgs.gametype = noFlags[i];
        CHECK(Team(CG_RED_FLAGSTATUS) == -1 && Team(CG_BLUE_BASESTATUS) == -1);
    }
    /* One Flag: the stolen icon, neutral while that team carries it. */
    cgs.gametype = GT_1FCTF; cgs.flagStatus = FLAG_ATBASE;
    CHECK(Team(CG_RED_FLAGSTATUS) == 57 && Team(CG_BLUE_BASESTATUS) == 61);
    cgs.flagStatus = FLAG_TAKEN_RED;
    CHECK(Team(CG_RED_FLAGSTATUS) == 53 && Team(CG_RED_BASESTATUS) == 53 && Team(CG_BLUE_FLAGSTATUS) == 61);
    cgs.flagStatus = FLAG_TAKEN_BLUE;
    CHECK(Team(CG_RED_FLAGSTATUS) == 57 && Team(CG_BLUE_FLAGSTATUS) == 53);

    /* Ownerdraw 54: the neutral flag, carried icons nudged by the scores. */
    draws = 0; cgs.gametype = GT_CTF; cgs.flagStatus = FLAG_ATBASE;
    CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(!draws);
    cgs.gametype = GT_1FCTF;
    for (i = FLAG_ATBASE; i <= FLAG_DROPPED; ++i) {
        cgs.flagStatus = i; CG_DrawOneFlagStatus(10, 20, 30, 40);
        CHECK(drawn == 50 + i && drawnY == 20);
    }
    cgs.flagStatus = FLAG_TAKEN_RED; cgs.scores1 = 3; cgs.scores2 = 5;
    CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawn == 53 && drawnY == 29);
    cgs.scores1 = 5; CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawnY == 11); /* tied: red ahead-or-even */
    cgs.scores1 = SCORE_NOT_PRESENT; CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawnY == 29);
    cgs.flagStatus = FLAG_TAKEN_BLUE; cgs.scores1 = 5; cgs.scores2 = 5;
    CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawn == 53 && drawnY == 29); /* tied: blue down */
    cgs.scores2 = 6; CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawnY == 11);
    cgs.scores2 = SCORE_NOT_PRESENT; CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(drawnY == 29);
    i = draws; cgs.flagStatus = 5; CG_DrawOneFlagStatus(10, 20, 30, 40);
    cgs.flagStatus = -1; CG_DrawOneFlagStatus(10, 20, 30, 40); CHECK(draws == i);

    puts("Flag status: icon set, team flag/base status, One Flag status and score offset OK");
    return 0;
}
