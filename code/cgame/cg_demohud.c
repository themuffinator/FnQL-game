/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Demo playback controls, retail 0x10010450, drawn during demo playback
 * with cg_drawDemoHUD (default 1) in the centre placement (0x10011418).
 * Observed:
 *
 * - A half-black box (210, 355, 220, 60) holds the time since the
 *   scoreboard was last closed, "[-]m:[0]s.mmm" (0x10001320), at (288, 375)
 *   in font 2, 0.3, white.
 * - Each control is a centred label (font 1, 0.25, white, y 395) over a
 *   centred yellow key hint (font 1, 0.15, y 405), all shadowed: Play or
 *   Pause [SPACE] at 235. While cl_freezeDemo is set, and for three frames
 *   after it clears, the label is Play and Advance [RIGHT] sits at 320;
 *   otherwise Pause, then - [LEFT] at 275, the timescale ("%0.1fx") from
 *   x 290, + [RIGHT] at 330 and 1.0x [DOWN] at 370. Hide [DEL] is at 410.
 * Centring halves the measured width with integer division. */

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <stdio.h>

static int demoHudRunFrames;

static float CG_DemoHudFreeze(void) {
    char value[32];
    trap_Cvar_VariableStringBuffer("cl_freezeDemo", value, sizeof(value));
    return (float)atof(value);
}

const char *CG_DemoHudTime(int msec) {
    static char text[32];
    const float seconds = (float)(abs(msec) / 1000.0);
    const int minutes = (int)seconds / 60;
    const float rest = (float)(seconds - minutes * 60.0);
    /* CRT formatting: bg_lib's Com_sprintf truncates floats. */
    snprintf(text, sizeof(text), "%s%d:%s%.03f", msec < 0 ? "-" : "", minutes, rest < 10.0f ? "0" : "", rest);
    return text;
}

static void CG_DemoHudCentered(int center, float y, float scale, vec4_t color, const char *text) {
    const int width = CG_Text_WidthInFont(text, 1, scale, 0);
    CG_SetTextFont(1);
    CG_Text_Paint((float)(center - width / 2), y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
}

static void CG_DemoHudControl(int center, const char *label, const char *key) {
    static vec4_t white = { 1, 1, 1, 1 }, yellow = { 1, 1, 0, 1 };
    CG_DemoHudCentered(center, 395, 0.25f, white, label);
    CG_DemoHudCentered(center, 405, 0.15f, yellow, key);
}

void CG_DrawDemoHud(void) {
    static vec4_t white = { 1, 1, 1, 1 }, shade = { 0, 0, 0, 0.5f };
    char speed[32];
    qboolean frozen;
    int previous;
    if (!cg.demoPlayback || !cg_drawDemoHUD.integer) return;
    previous = CG_SetScreenPlacement(CG_PLACE_CENTER);
    if (CG_DemoHudFreeze() > 0) {
        demoHudRunFrames = 0;
        frozen = qtrue;
    } else {
        frozen = ++demoHudRunFrames < 4;
    }
    CG_FillRect(210, 355, 220, 60, shade);
    CG_SetTextFont(2);
    CG_Text_Paint(288, 375, 0.3f, white, CG_DemoHudTime(cg.time - cg.scoreFadeTime), 0, 0, ITEM_TEXTSTYLE_SHADOWED);
    CG_DemoHudControl(235, frozen ? "Play" : "Pause", "[SPACE]");
    if (frozen) {
        CG_DemoHudControl(320, "Advance", "[RIGHT]");
    } else {
        CG_DemoHudControl(275, "-", "[LEFT]");
        snprintf(speed, sizeof(speed), "%0.1fx", cg_timescale.value);
        CG_SetTextFont(1);
        CG_Text_Paint(290, 395, 0.25f, white, speed, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        CG_DemoHudControl(330, "+", "[RIGHT]");
        CG_DemoHudControl(370, "1.0x", "[DOWN]");
    }
    CG_DemoHudControl(410, "Hide", "[DEL]");
    CG_SetScreenPlacement(previous);
}
