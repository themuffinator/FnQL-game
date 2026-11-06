/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail end-of-match vote ownerdraws: choice text, shots, counts, timer. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static const char *titles = "\\map_0\\campgrounds\\title_0\\Campgrounds\\gt_0\\Clan Arena"
                            "\\map_1\\\\title_1\\Sorrow\\gt_1\\Free For All";
static const char *votes = "\\0\\3\\1\\0";
static int font = -1;
static char painted[128], registered[128];
static float paintX, paintY, picW;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) {
    return index == CS_ROTATION_TITLES ? titles : index == CS_ROTATION_VOTES ? votes : "";
}
void CG_SetTextFont(int value) { font = value; }
int CG_Text_WidthInFont(const char *text, int f, float scale, int limit) {
    CHECK(f == 0 && !limit); (void)scale; return (int)strlen(text) * 10;
}
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 4; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)scale; (void)color; (void)adjust; (void)limit; (void)style;
    paintX = x; paintY = y; snprintf(painted, sizeof(painted), "%s", text);
}
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { snprintf(registered, sizeof(registered), "%s", name); return 7; }
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) { (void)x; (void)y; (void)h; CHECK(shader == 7); picW = w; }

static void Draw(int ownerDraw, int align) {
    vec4_t color = { 1, 1, 1, 1 };
    painted[0] = 0; font = -1;
    CHECK(CG_DrawEndVote(ownerDraw, 100, 50, 120, 90, align, .2f, color, 0));
}

int main(void) {
    CHECK(!CG_DrawEndVote(CG_PLAYER_HEALTH, 0, 0, 0, 0, 0, 1, NULL, 0));
    Draw(CG_VOTEGAMETYPE1, ITEM_ALIGN_LEFT);
    CHECK(!strcmp(painted, "Clan Arena") && paintX == 100 && paintY == 42 && font == 0);
    Draw(CG_VOTEGAMETYPE2, ITEM_ALIGN_RIGHT); CHECK(!strcmp(painted, "Free For All") && paintX == 100 - 120);
    Draw(CG_VOTEGAMETYPE3, ITEM_ALIGN_CENTER); CHECK(!painted[0] && paintX == 100);
    Draw(CG_VOTEMAP1, ITEM_ALIGN_CENTER); CHECK(!strcmp(painted, "campgrounds") && paintX == 100 - 55 && paintY == 50);
    Draw(CG_VOTENAME2, ITEM_ALIGN_LEFT); CHECK(!strcmp(painted, "Sorrow"));
    /* Shots: the map's preview, else the default preview. */
    Draw(CG_VOTESHOT1, 0); CHECK(!strcmp(registered, "levelshots/preview/campgrounds") && picW == 120);
    Draw(CG_VOTESHOT2, 0); CHECK(!strcmp(registered, "levelshots/preview/default"));
    Draw(CG_VOTESHOT3, 0); CHECK(!strcmp(registered, "levelshots/preview/default"));
    /* Counts use the item font. */
    Draw(CG_VOTECOUNT1, ITEM_ALIGN_RIGHT); CHECK(!strcmp(painted, "Votes: 3") && font == -1 && paintX == 100 - 32);
    Draw(CG_VOTECOUNT3, ITEM_ALIGN_LEFT); CHECK(!strcmp(painted, "Votes: "));
    /* Timer: 20 seconds after the vote time. */
    cgs.voteTime = 10000; cg.time = 10000; CHECK(!strcmp(CG_VoteTimerText(), "Voting ends in 20 seconds."));
    cg.time = 28500; CHECK(!strcmp(CG_VoteTimerText(), "Voting ends in 1 second."));
    cg.time = 29001; CHECK(!strcmp(CG_VoteTimerText(), "Voting has ended."));
    cg.time = 40000; Draw(CG_VOTETIMER, ITEM_ALIGN_LEFT); CHECK(!strcmp(painted, "Voting has ended.") && font == 0);
    CHECK(!CG_VoteChoice("map", 3)[0] && !CG_VoteChoice("map", -1)[0]);
    puts("end vote: choices, shots, counts and timer OK");
    return 0;
}
