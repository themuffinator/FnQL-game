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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_damagePlum, cg_damagePlumColorStyle;
static snapshot_t snapshot;
static int draws, lastX, lastY, resets;
static qboolean visible = qtrue;
static vec4_t lastColor;
static char lastText[16];
static float lastSize;
void QDECL Com_Printf(const char *format, ...) { (void)format; }
void QDECL Com_Error(int code, const char *format, ...) { (void)code; (void)format; abort(); }
qboolean trap_R_ProjectPoint(const vec3_t point, vec4_t window) {
    (void)point; window[0] = 320; window[1] = 240; window[2] = 0.5f; window[3] = 1;
    return visible;
}
void trap_R_MeasureText(const char *text, int font, float size, int limit, float *width, float *height) {
    CHECK(font == 0 && limit == 0 && text[0]); *width = 20; *height = 10;
}
void trap_R_SetColor(const float *color) {
    if (color) memcpy(lastColor, color, sizeof(lastColor)); else ++resets;
}
void trap_R_DrawText(int x, int y, const char *text, int font, float size, int limit, int forceColor) {
    CHECK(font == 0 && limit == -1 && forceColor == 0);
    ++draws; lastX = x; lastY = y; lastSize = size; strcpy(lastText, text);
}
static void Reset(void) {
    memset(&cg, 0, sizeof(cg)); memset(&cgs, 0, sizeof(cgs)); memset(&snapshot, 0, sizeof(snapshot));
    cg.snap = &snapshot; snapshot.ps.clientNum = 7; cg.clientNum = 3;
    cg.time = 1000; cg.damagePlumRandom = 0x92u;
    cgs.screenXScale = cgs.screenYScale = 1;
    cg.refdef.width = cgs.glconfig.vidWidth = 640;
    cg.refdef.height = cgs.glconfig.vidHeight = 480;
    strcpy(cg_damagePlum.string, "g mg sg gl rl lg rg pg bfg gh cg ng pl hmg");
    cg_damagePlumColorStyle.integer = 1; draws = resets = 0; visible = qtrue;
}
static int Count(void) {
    int i, count = 0;
    for (i = 0; i < CG_MAX_DAMAGE_PLUMS; ++i) count += cg.damagePlums[i].active != 0;
    return count;
}
int main(void) {
    centity_t cent = {0};
    int i;
    static const int damages[] = {25, 26, 50, 51, 75, 76};
    static const float colors[][3] = {{.25f,.5f,1}, {1,1,0}, {1,1,0}, {1,.5f,0}, {1,.5f,0}, {1,0,0}};
    cent.currentState.clientNum = 7; cent.currentState.generic1 = WP_ROCKET_LAUNCHER; cent.currentState.time = 50;
    Reset(); CG_DamagePlum(&cent);
    CHECK(Count() == 1 && !strcmp(cg.damagePlums[0].text, "50"));
    CHECK(cg.damagePlums[0].color[0] == 1 && cg.damagePlums[0].color[1] == .5f);
    for (i = 0; i < 3; ++i) CHECK(cg.damagePlums[0].origin[i] >= -10 && cg.damagePlums[0].origin[i] < 10);
    CHECK(cg.damagePlums[0].horizontalSpeed >= -50 && cg.damagePlums[0].horizontalSpeed < 50);
    CHECK(cg.damagePlums[0].verticalSpeed <= -120 && cg.damagePlums[0].verticalSpeed > -140);
    /* Retail: left-anchored at the projected point, scale 0.15 (9 px at 480). */
    CG_DrawDamagePlums(); CHECK(draws == 1 && resets == 1 && lastX == 320 && lastY == 240 && lastSize == 9);
    cg.time = 1500; CG_DrawDamagePlums();
    CHECK(lastX == (int)(320 + cg.damagePlums[0].horizontalSpeed * 0.5f));
    CHECK(lastY == (int)(240 + cg.damagePlums[0].verticalSpeed * 0.5f + 150 * 0.25f));
    cgs.glconfig.vidWidth = 1280; cgs.glconfig.vidHeight = 960; cg.refdef.width = 1280; cg.refdef.height = 960;
    CG_DrawDamagePlums(); CHECK(lastSize == 18 && lastX == (int)(320 + cg.damagePlums[0].horizontalSpeed * 0.5f * 2));
    cg.refdef.width = cgs.glconfig.vidWidth = 640; cg.refdef.height = cgs.glconfig.vidHeight = 480;
    cg.time = 1600; CG_DrawDamagePlums(); CHECK(lastColor[3] == 1 && lastY < 240);
    cg.time = 1800; CG_DrawDamagePlums(); CHECK(lastColor[3] == .5f);
    cg.time = 2000; CG_DrawDamagePlums(); CHECK(draws == 5 && Count() == 0);
    for (i = 0; i < ARRAY_LEN(damages); ++i) {
        Reset(); cg_damagePlumColorStyle.integer = 2; cent.currentState.time = damages[i];
        CG_DamagePlum(&cent); CHECK(!memcmp(cg.damagePlums[0].color, colors[i], sizeof(colors[i])));
    }
    Reset(); cg_damagePlumColorStyle.integer = 3; cent.currentState.generic1 = WP_RAILGUN;
    CG_DamagePlum(&cent); CHECK(cg.damagePlums[0].color[0] == 0 && cg.damagePlums[0].color[1] == 1);
    Reset(); strcpy(cg_damagePlum.string, "RG"); CG_DamagePlum(&cent); CHECK(Count() == 1);
    strcpy(cg_damagePlum.string, "all 1 off rocket"); CG_DamagePlum(&cent); CHECK(Count() == 1);
    strcpy(cg_damagePlum.string, "x x x x x x x x x x x x x x x x rg"); CG_DamagePlum(&cent); CHECK(Count() == 1);
    Reset(); cent.currentState.generic1 = -1; CG_DamagePlum(&cent);
    cent.currentState.generic1 = 15; CG_DamagePlum(&cent); CHECK(Count() == 0);
    cent.currentState.generic1 = WP_RAILGUN; cent.currentState.clientNum = cg.clientNum; CG_DamagePlum(&cent); CHECK(Count() == 0);
    cent.currentState.clientNum = 7; cg.snap = NULL; CG_DamagePlum(&cent); CHECK(Count() == 0);
    cg.snap = &snapshot; cgs.gametype = GT_RACE; snapshot.ps.eFlags = EF_SPECTATOR;
    CG_DamagePlum(&cent); CHECK(Count() == 0);
    snapshot.ps.pm_flags = PMF_FOLLOW; CG_DamagePlum(&cent); CHECK(Count() == 1);
    Reset(); cent.currentState.time = INT_MIN; CG_DamagePlum(&cent);
    CHECK(!strcmp(cg.damagePlums[0].text, "-2147483648") && cg.damagePlums[0].color[1] == 1);
    cent.currentState.time = INT_MAX; CG_DamagePlum(&cent); CHECK(cg.damagePlums[1].color[1] == 0);
    Reset(); for (i = 0; i < 40; ++i) CG_DamagePlum(&cent); CHECK(Count() == 32);
    visible = qfalse; CG_DrawDamagePlums(); CHECK(draws == 0);
    cg.time += 1000; CG_DamagePlum(&cent); CHECK(cg.damagePlums[0].startedAt == cg.time);
    CG_DrawDamagePlums(); CHECK(Count() == 1);
    cg.time -= 1; CG_DrawDamagePlums(); CHECK(Count() == 0); /* demo seek discards stale text */
    Reset(); cg.time = INT_MAX - 500; CG_DamagePlum(&cent);
    cg.time = (int)((unsigned)cg.time + 800u); CG_DrawDamagePlums(); CHECK(draws == 1 && lastColor[3] == .5f);
    puts("damage plums: recipient, filters, colors, bounded queue, motion/fade and clock wrap OK");
    return 0;
}
