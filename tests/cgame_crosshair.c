/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Quake Live crosshair (retail 0x1000c2a0, 0x1000c0a0, 0x10043c2c). */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_drawCrosshair, cg_crosshairSize, cg_crosshairHealth, cg_crosshairX, cg_crosshairY, cg_crosshairColor,
    cg_crosshairBrightness, cg_crosshairPulse, cg_crosshairHitColor, cg_crosshairHitStyle, cg_crosshairHitTime;
static snapshot_t snap;
static float color[4], pic[5];
static int draws, health, armor;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void CG_GetColorForHealth(int h, int a, vec4_t c) { health = h; armor = a; c[0] = 0.5f; c[1] = 1; c[2] = 0; c[3] = 1; }
void trap_R_SetColor(const float *rgba) { if (rgba) memcpy(color, rgba, sizeof(color)); }
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {
    CHECK(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1);
    pic[0] = x; pic[1] = y; pic[2] = w; pic[3] = h; pic[4] = (float)shader; ++draws;
}
static qboolean Near(float a, float b) { return a > b - 0.01f && a < b + 0.01f; }

int main(void) {
    playerState_t ps = {0}, ops = {0};
    int i;
    for (i = 0; i < CG_NUM_CROSSHAIRS; ++i) cgs.media.crosshairShader[i] = 100 + i;
    cg.snap = &snap; cgs.glconfig.vidWidth = 960;
    cg.refdef.x = 0; cg.refdef.y = 0; cg.refdef.width = 960; cg.refdef.height = 600;
    cg_drawCrosshair.integer = 2; cg_crosshairSize.value = 32; cg_crosshairColor.integer = 25;
    cg_crosshairBrightness.value = 1; cg_crosshairPulse.integer = 1; cg_crosshairHitStyle.integer = 1;
    cg_crosshairHitTime.value = 200; cg_crosshairHitColor.integer = 1;
    /* Default: white crosshair 2, 48 px at 960 wide, centred. */
    cg.time = 1000;
    CG_DrawQLCrosshair();
    CHECK(draws == 1 && pic[4] == 101 && pic[2] == 48 && pic[0] == 456 && pic[1] == 276);
    CHECK(color[0] == 1 && color[1] == 1 && color[2] == 1 && color[3] == 1);
    /* Colour table and brightness; out-of-range entries use red. */
    cg_crosshairColor.integer = 15; cg_crosshairBrightness.value = 0.5f; CG_DrawQLCrosshair();
    CHECK(color[0] == 0 && Near(color[1], 0.25f) && Near(color[2], 0.5f) && color[3] == 1);
    cg_crosshairColor.integer = 40; CG_DrawQLCrosshair(); CHECK(Near(color[0], 0.5f) && color[1] == 0);
    cg_crosshairBrightness.value = 1;
    /* Health colour from the snapshot stats. */
    snap.ps.stats[STAT_HEALTH] = 80; snap.ps.stats[STAT_ARMOR] = 20; cg_crosshairHealth.integer = 1;
    CG_DrawQLCrosshair(); CHECK(health == 80 && armor == 20 && color[0] == 0.5f && color[1] == 1);
    cg_crosshairHealth.integer = 0; cg_crosshairColor.integer = 25;
    /* Indices: 0 and multiples of 30 draw nothing; 31 is crosshair 1. */
    draws = 0; cg_drawCrosshair.integer = 30; CG_DrawQLCrosshair(); cg_drawCrosshair.integer = 0; CG_DrawQLCrosshair();
    CHECK(draws == 0);
    cg_drawCrosshair.integer = 31; CG_DrawQLCrosshair(); CHECK(draws == 1 && pic[4] == 100);
    cg_drawCrosshair.integer = 2;
    /* Offsets scale with the width. */
    cg_crosshairX.integer = 10; cg_crosshairY.integer = -20; CG_DrawQLCrosshair();
    CHECK(pic[0] == 471 && pic[1] == 246); cg_crosshairX.integer = cg_crosshairY.integer = 0;
    /* Pickup pulse, newer than the last hit. */
    cg.itemPickupBlendTime = 950; CG_DrawQLCrosshair(); CHECK(Near(pic[2], 32 * 1.25f * 1.5f));
    cg_crosshairPulse.integer = 0; CG_DrawQLCrosshair(); CHECK(pic[2] == 48); cg_crosshairPulse.integer = 1;
    cg.time = 1300; CG_DrawQLCrosshair(); CHECK(pic[2] == 48);
    /* Hits: recorded from PERS_HITS with generic1 / 64 + 1. */
    ps.persistant[PERS_HITS] = 1; ps.generic1 = 130; CG_RecordCrosshairHit(&ps, &ops);
    CHECK(cg.crosshairHitTime == 1300 && cg.crosshairHitLevel == 3);
    ops = ps; ps.generic1 = 0; CG_RecordCrosshairHit(&ps, &ops); CHECK(cg.crosshairHitLevel == 3);
    /* Style 1: level colour only (3 = orange), no growth. */
    cg.time = 1400; CG_DrawQLCrosshair(); CHECK(pic[2] == 48 && color[0] == 1 && color[1] == 0.5f && color[2] == 0);
    /* Style 4: colour plus growth by level: 1 + 3 * 0.5. */
    cg_crosshairHitStyle.integer = 4; CG_DrawQLCrosshair(); CHECK(Near(pic[2], 32 * 2.5f * 1.5f));
    /* Style 8: custom colour, fixed growth. */
    cg_crosshairHitStyle.integer = 8; cg_crosshairHitColor.integer = 9; CG_DrawQLCrosshair();
    CHECK(Near(pic[2], 32 * 1.5f * 1.5f) && color[0] == 0 && color[1] == 1 && color[2] == 0);
    /* Level 1 is light blue; outside the hit time nothing changes. */
    cg_crosshairHitStyle.integer = 1; cg.crosshairHitLevel = 1; CG_DrawQLCrosshair();
    CHECK(color[0] == 0.25f && color[1] == 0.5f && color[2] == 1);
    cg.time = 1600; CG_DrawQLCrosshair(); CHECK(color[0] == 1 && color[1] == 1 && pic[2] == 48);
    /* Third person draws nothing. */
    draws = 0; cg.renderingThirdPerson = qtrue; CG_DrawQLCrosshair(); CHECK(!draws);
    puts("crosshair: colours, indices, pulse, hit styles and placement OK");
    return 0;
}
