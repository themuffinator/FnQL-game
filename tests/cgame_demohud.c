/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Demo playback controls (retail 0x10010450, 0x10001320). */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_drawDemoHUD, cg_timescale;
static char freeze[8] = "0", painted[16][24];
static float px[16], py[16];
static int paints, fills, font;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void trap_Cvar_VariableStringBuffer(const char *name, char *buf, int size) { CHECK(!strcmp(name, "cl_freezeDemo")); Q_strncpyz(buf, freeze, size); }
int CG_SetScreenPlacement(int p) { (void)p; return 0; }
void CG_SetTextFont(int f) { font = f; }
int CG_Text_WidthInFont(const char *text, int f, float scale, int limit) { (void)f; (void)scale; (void)limit; return (int)strlen(text) * 5; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)scale; (void)color; (void)adjust; (void)limit; CHECK(style == ITEM_TEXTSTYLE_SHADOWED);
    px[paints] = x; py[paints] = y; snprintf(painted[paints++], 24, "%s", text);
}
void CG_FillRect(float x, float y, float w, float h, const float *c) { CHECK(x == 210 && y == 355 && w == 220 && h == 60 && c[3] == 0.5f); ++fills; }

int main(void) {
    CHECK(!strcmp(CG_DemoHudTime(4891), "0:04.891") && !strcmp(CG_DemoHudTime(75500), "1:15.500"));
    CHECK(!strcmp(CG_DemoHudTime(-2000), "-0:02.000"));
    CG_DrawDemoHud(); CHECK(!paints);
    cg.demoPlayback = qtrue; cg_drawDemoHUD.integer = 1; cg_timescale.value = 1; cg.time = 5000; cg.scoreFadeTime = 109;
    CG_DrawDemoHud();
    /* First frames: Play / Advance. */
    CHECK(fills == 1 && !strcmp(painted[0], "0:04.891") && px[0] == 288 && py[0] == 375);
    CHECK(!strcmp(painted[1], "Play") && px[1] == 225 && !strcmp(painted[2], "[SPACE]") && py[2] == 405);
    CHECK(!strcmp(painted[3], "Advance") && px[3] == 303 && !strcmp(painted[5], "Hide") && px[5] == 400);
    paints = 0; CG_DrawDemoHud(); paints = 0; CG_DrawDemoHud(); CHECK(!strcmp(painted[1], "Play"));
    /* Fourth running frame: Pause and the speed controls. */
    paints = 0; CG_DrawDemoHud();
    CHECK(!strcmp(painted[1], "Pause") && !strcmp(painted[3], "-") && px[3] == 273);
    CHECK(!strcmp(painted[5], "1.0x") && px[5] == 290 && !strcmp(painted[6], "+") && !strcmp(painted[8], "1.0x"));
    CHECK(!strcmp(painted[10], "Hide"));
    /* Freezing returns to Play. */
    strcpy(freeze, "1"); paints = 0; CG_DrawDemoHud(); CHECK(!strcmp(painted[1], "Play") && !strcmp(painted[3], "Advance"));
    puts("demo HUD: time format, controls and freeze states OK");
    return 0;
}
