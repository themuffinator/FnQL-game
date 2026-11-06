/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* CG_FLAG_STATUS (retail 0x10030240): the spectator flag track. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
static int fills, pics;
static float fill[4][5], pic[8][5];

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) {
    if (index == CS_RED_FLAG_ORIGIN) return "\\x\\0\\y\\0\\z\\0";
    if (index == CS_BLUE_FLAG_ORIGIN) return "\\x\\1000\\y\\0\\z\\0";
    return "";
}
qhandle_t CG_FlagTrackPointer(void) { return 99; }
qhandle_t CG_FlagStatusIcon(int team, int state) { return team * 10 + state; }
void CG_FillRect(float x, float y, float w, float h, const float *color) {
    fill[fills][0] = x; fill[fills][1] = y; fill[fills][2] = w; fill[fills][3] = h; fill[fills++][4] = color[0];
}
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    pic[pics][0] = x; pic[pics][1] = y; pic[pics][2] = w; pic[pics][3] = h; pic[pics++][4] = (float)shader;
}
static qboolean Near(float a, float b) { return a > b - 0.01f && a < b + 0.01f; }

int main(void) {
    cgs.gametype = GT_CTF; cgs.maxclients = 4; cg.predictedPlayerState.eFlags = EF_SPECTATOR;
    VectorSet(cg.objectiveBases[1], 0, 0, 0); VectorSet(cg.objectiveBases[2], 1000, 0, 0);
    /* Both flags home: nothing. */
    cgs.redflag = cgs.blueflag = FLAG_ATBASE;
    CG_DrawFlagTrack(320, 100, 200, 32); CHECK(!fills && !pics);
    /* Red flag carried a quarter of the way: track bar plus its icons. */
    cgs.redflag = FLAG_TAKEN;
    cgs.clientinfo[2].infoValid = qtrue; cg_entities[1].currentState.clientNum = 2;
    cg_entities[1].currentState.powerups = 1 << PW_REDFLAG; VectorSet(cg_entities[1].lerpOrigin, 250, 0, 0);
    CG_DrawFlagTrack(320, 100, 200, 32);
    CHECK(fills == 2 && fill[0][0] == 220 && fill[0][2] == 100 && fill[0][3] == 4 && fill[0][4] == 1);
    CHECK(fill[1][0] == 320 && fill[1][4] == 0);
    /* d = 750 of D = 1000: x = 320 + 100 - 16 - 150. */
    CHECK(pics == 2 && Near(pic[0][0], 262) && pic[0][1] == 100 && pic[0][2] == 16 && pic[0][4] == 99);
    CHECK(Near(pic[1][0], 254) && pic[1][1] == 112 && pic[1][2] == 32 && pic[1][4] == 13);
    /* Dropped blue flag at its last seen position, beyond the red base. */
    fills = pics = 0; cgs.redflag = FLAG_ATBASE; cgs.blueflag = 2; VectorSet(cg.objectiveBases[2], -500, 0, 0);
    CG_DrawFlagTrack(320, 100, 200, 32);
    /* d = 500 of 1000: x = 320 - 100 - 16 + 100. */
    CHECK(pics == 2 && Near(pic[1][0], 304) && pic[1][4] == 22);
    /* Players, One Flag and Freeze Tag see nothing. */
    fills = pics = 0; cg.predictedPlayerState.eFlags = 0; CG_DrawFlagTrack(320, 100, 200, 32); CHECK(!pics);
    cg.predictedPlayerState.eFlags = EF_SPECTATOR;
    cgs.gametype = GT_1FCTF; CG_DrawFlagTrack(320, 100, 200, 32); CHECK(!pics);
    cgs.gametype = GT_FREEZE; CG_DrawFlagTrack(320, 100, 200, 32); CHECK(!pics);
    puts("flag track: bar, carrier and dropped positions OK");
    return 0;
}
