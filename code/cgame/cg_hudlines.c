/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail CG_Draw2D's code-drawn lines, beside the menu HUD. Observed:
 *
 * - Item pickups (0x1000a800, from the lower-left block 0x1000abf0 at the
 *   left placement with y 408): with cg_drawItemPickups set, the scoreboard
 *   hidden and the snapshot player alive, a line 48 above y for three
 *   seconds (CG_FadeColor, 200 ms fade), font 1 at 0.28 unshadowed, from x 5:
 *   bit 4 the pickup time "%i:%i%i" (0 in warmup; counting down with
 *   cg_levelTimerDirection 1 inside a time limit, up past it or without
 *   one), then 13 units; bit 1 the 16-square icon, then 24; bit 2 the
 *   pickup name, then 8; any bit "x%i" for repeats of the item last shown,
 *   except powerups and team items. Text sits its own height below y.
 * - Rewards (0x1000ac30, centre placement): each lasts 2000 ms, fading over
 *   the last 200. When the shown one ends the stack pops even without
 *   cg_drawRewards, and cg_announcerRewardsVO plays the next sound. Up to
 *   cg_drawRewardsRowSize icons (32 square, 48 apart, y 42) are centred on
 *   320; more show one icon at 296 and "%d" at (336, 66), font 0, 0.2, in
 *   opaque white.
 * - Input commands (0x1000b610, centre placement): with cg_drawInputCmds,
 *   not a free spectator (and following in mode 2), the followed player's
 *   or demo's mirrored moves, else the current user command, drawn as 16
 *   square race arrows around (X, Y) at a cg_drawInputCmdsSize gap.
 * - Speedometer (0x1000b8f0): a 128-sample ring of the predicted
 *   horizontal speed (0x1004de3e). Mode 1 is a 48 square graph at
 *   (592, 384) over the lagometer image at the right placement; modes 2..
 *   a 128 x 32 graph at (256, 241 + crosshair size / 2), alpha 0.75, at the
 *   centre placement. Bars grow green to half the graph height for three
 *   times ps.speed over the height less 5, then yellow. Modes 1..3 add the
 *   current speed ("%d", measured at 0.175, painted at 0.15, font 0,
 *   shadowed); mode 3 draws only that and mode 4 nothing.
 * - Voice indicator (CG_Draw2D 0x10010d90, after the centre print):
 *   cg_voiceChatIndicator 1 at the right placement, 2 at the left, from
 *   y 150 down 15 a row: a team-tinted fill, the speaking icon and the
 *   name (font 0, 0.25). A remote speaker is dropped 500 ms after the last
 *   report.
 * - Frame rate (0x10009c10, right placement): four-frame average, shown as
 *   com_maxfps when within one, "%ifps" in font 2 at 0.25 right-aligned to
 *   635, shadowed.
 * - Vignette (0x10009d60): gfx/misc/vignette over the 640x480 canvas,
 *   stretched, with cg_vignette. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

static const vec4_t hudWhite = { 1, 1, 1, 1 };

/* ---------------------------------------------------------------- pickups */

int CG_ItemPickupSeconds(int pickupTime) {
    int levelStart = cgs.levelStartTime, limit = cgs.timelimit * 60000, now, elapsed;
    if (cg.warmup) return 0;
    if (!cgs.timelimit) return (pickupTime - levelStart) / 1000;
    now = atoi(CG_ConfigString(CS_TIMEOUT_START_TIME));
    if (!now) now = cg.time;
    if (limit - now + levelStart < 0) elapsed = pickupTime - limit - levelStart;
    else if (cg_levelTimerDirection.integer != 1) elapsed = pickupTime - levelStart;
    else elapsed = levelStart + limit - pickupTime;
    return elapsed / 1000;
}

int CG_DrawItemPickups(int y) {
    const int flags = cg_drawItemPickups.integer;
    const int item = cg.itemPickup;
    float *color;
    int x = 5;
    if (!flags || cg.scoreBoardShowing || !cg.snap || cg.snap->ps.stats[STAT_HEALTH] <= 0) return y;
    y -= 48;
    color = item ? CG_FadeColor(cg.itemPickupTime, 3000) : NULL;
    if (!color || item <= 0 || item >= bg_numItems) {
        cg.itemPickupShown = 0;
        cg.itemPickupCount = 1;
        return y;
    }
    CG_RegisterItemVisuals(item);
    trap_R_SetColor(color);
    CG_SetTextFont(1);
    if (flags & 4) {
        const int seconds = CG_ItemPickupSeconds(cg.itemPickupTime);
        const int minutes = seconds / 60, rest = seconds - minutes * 60;
        const char *text = va("%i:%i%i", minutes, rest / 10, rest % 10);
        CG_Text_Paint(5, (float)(CG_Text_HeightInFont(text, 1, 0.28f, 0) + y), 0.28f, color, text, 0, 0, 0);
        x = CG_Text_WidthInFont(text, 1, 0.28f, 0) + 13;
    }
    if (flags & 1) {
        CG_DrawPic((float)x, (float)y, 16, 16, cg_items[item].icon);
        x += 24;
    }
    if (flags & 2) {
        const char *text = bg_itemlist[item].pickup_name;
        CG_Text_Paint((float)x, (float)(CG_Text_HeightInFont(text, 1, 0.28f, 0) + y), 0.28f, color, text, 0, 0, 0);
        x += CG_Text_WidthInFont(text, 1, 0.28f, 0) + 8;
    }
    if (cg.itemPickupCount > 1 && bg_itemlist[item].giType != IT_POWERUP && bg_itemlist[item].giType != IT_TEAM) {
        const char *text = va("x%i", cg.itemPickupCount);
        CG_Text_Paint((float)x, (float)(CG_Text_HeightInFont(text, 1, 0.28f, 0) + y), 0.28f, color, text, 0, 0, 0);
    }
    CG_SetTextFont(0);
    cg.itemPickupShown = item;
    trap_R_SetColor(NULL);
    return y;
}

/* ---------------------------------------------------------------- rewards */

void CG_DrawRewards(void) {
    static vec4_t color = { 1, 1, 1, 1 };
    const float *paint = color;
    int i, count;
    const int elapsed = cg.time - cg.rewardTime;
    if (cg.rewardTime && elapsed < 2000) {
        const int remaining = 2000 - elapsed;
        color[0] = color[1] = color[2] = 1;
        color[3] = remaining >= 200 ? 1.0f : (float)remaining / 200.0f;
    } else {
        if (cg.rewardStack <= 0) return;
        for (i = 0; i < cg.rewardStack; i++) {
            cg.rewardSound[i] = cg.rewardSound[i + 1];
            cg.rewardShader[i] = cg.rewardShader[i + 1];
            cg.rewardCount[i] = cg.rewardCount[i + 1];
        }
        cg.rewardTime = cg.time;
        cg.rewardStack--;
        color[0] = color[1] = color[2] = color[3] = 1;
        paint = cg.time ? color : NULL;
        if (cg_announcerRewardsVO.integer) CG_AddBufferedSound(cg.rewardSound[0]);
    }
    if (!cg_drawRewards.integer) return;
    trap_R_SetColor(paint);
    count = cg.rewardCount[0];
    if (count <= cg_drawRewardsRowSize.integer) {
        float x = (float)(320 - (count * 48) / 2);
        for (i = 0; i < count; i++, x += 48) CG_DrawPic(x, 42, 32, 32, cg.rewardShader[0]);
    } else {
        CG_DrawPic(296, 42, 32, 32, cg.rewardShader[0]);
        CG_SetTextFont(0);
        CG_Text_Paint(336, 66, 0.2f, (float *)hudWhite, va("%d", count), 0, 0, 0);
    }
    trap_R_SetColor(NULL);
}

/* ---------------------------------------------------------- input commands */

void CG_DrawInputCommands(void) {
    const playerState_t *ps;
    int forward, right, up;
    float s, x, y;
    if (!cg_drawInputCmds.integer || !cg.snap) return;
    if (cg.predictedPlayerState.pm_type == PM_SPECTATOR && !(cg.predictedPlayerState.pm_flags & PMF_FOLLOW)) return;
    if (cg_drawInputCmds.integer == 2 && !(cg.predictedPlayerState.pm_flags & PMF_FOLLOW)) return;
    ps = &cg.snap->ps;
    if ((ps->pm_flags & PMF_FOLLOW) || cg.demoPlayback) {
        forward = ps->forwardmove; right = ps->rightmove; up = ps->upmove;
    } else {
        usercmd_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cmd);
        forward = cmd.forwardmove; right = cmd.rightmove; up = cmd.upmove;
    }
    s = cg_drawInputCmdsSize.value;
    x = (float)cg_drawInputCmdsX.integer;
    y = (float)cg_drawInputCmdsY.integer;
    if (forward > 0) CG_DrawPic(x - 8, y - s - 16, 16, 16, cgs.media.inputCmdShaders[0]);
    else if (forward < 0) CG_DrawPic(x - 8, y + s, 16, 16, cgs.media.inputCmdShaders[1]);
    if (right > 0) CG_DrawPic(x + s, y - 8, 16, 16, cgs.media.inputCmdShaders[2]);
    else if (right < 0) CG_DrawPic(x - s - 16, y - 8, 16, 16, cgs.media.inputCmdShaders[3]);
    if (up > 0) CG_DrawPic(x + s + 16, y - s - 16, 16, 16, cgs.media.inputCmdShaders[0]);
    else if (up < 0) CG_DrawPic(x + s + 16, y + s, 16, 16, cgs.media.inputCmdShaders[1]);
}

/* ------------------------------------------------------------- speedometer */

#define SPEED_SAMPLES 128
static float speedSamples[SPEED_SAMPLES];
static int speedHead, speedCount;

void CG_SampleSpeed(float speed) {
    speedHead = (speedHead + 1) % SPEED_SAMPLES;
    speedSamples[speedHead] = speed;
    if (speedCount < SPEED_SAMPLES - 1) speedCount++;
}

float CG_CurrentSpeedSample(void) { return speedSamples[speedHead]; }

void CG_DrawSpeedometer(void) {
    const int mode = cg_speedometer.integer;
    float x, y, w, h, alpha, scaleY, barW;
    int placement, i, index;
    if (!mode || mode >= 4 || !cg.snap) return;
    if (cg.snap->ps.pm_type == PM_SPECTATOR && !(cg.predictedPlayerState.pm_flags & PMF_FOLLOW)) return;
    if (mode == 1) {
        x = 592; y = 384; w = 48; h = 48; alpha = 1;
        placement = CG_SetScreenPlacement(CG_PLACE_RIGHT);
    } else {
        x = 256; y = (float)(cg_crosshairSize.integer / 2 + 241); w = 128; h = 32; alpha = 0.75f;
        placement = CG_SetScreenPlacement(CG_PLACE_CENTER);
    }
    if (mode < 3) {
        const vec4_t green = { 0, 1, 0, alpha }, yellow = { 1, 1, 0, alpha };
        float sx = x, sy = y, sw = w, sh = h;
        if (mode == 1) CG_DrawPic(x, y, w, h, cgs.media.lagometerShader);
        CG_AdjustFrom640(&sx, &sy, &sw, &sh);
        scaleY = cg.predictedPlayerState.speed > 0 ? (sh - 5) / (3.0f * cg.predictedPlayerState.speed) : 0;
        barW = sw / (float)SPEED_SAMPLES;
        index = (speedHead + 1) % SPEED_SAMPLES;
        for (i = 0; i < speedCount; i++, index = (index + 1) % SPEED_SAMPLES) {
            const float v = speedSamples[index];
            float full, bar;
            if (!(v > 0)) continue;
            full = scaleY * v;
            bar = full < sh * 0.5f ? full : sh * 0.5f;
            trap_R_SetColor(green);
            trap_R_DrawStretchPic(sx + i * barW, sy + sh - bar, barW, bar, 0, 0, 0, 0, cgs.media.whiteShader);
            if (full > sh * 0.5f) {
                const float over = full - sh * 0.5f < sh * 0.5f ? full - sh * 0.5f : sh * 0.5f;
                const float top = sy + sh - (full < sh ? full : sh);
                trap_R_SetColor(yellow);
                trap_R_DrawStretchPic(sx + i * barW, top, barW, over, 0, 0, 0, 0, cgs.media.whiteShader);
            }
        }
        trap_R_SetColor(NULL);
    }
    {
        const char *text = va("%d", (int)speedSamples[speedHead]);
        const int height = CG_Text_HeightInFont(text, 0, 0.175f, 0);
        CG_SetTextFont(0);
        if (mode == 1) {
            CG_SetScreenPlacement(CG_PLACE_RIGHT);
            CG_Text_Paint(592, 384 + 1 + (float)height, 0.15f, (float *)hudWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        } else {
            const int width = CG_Text_WidthInFont(text, 0, 0.175f, 0);
            CG_Text_Paint((float)(320 - width / 2), y + 1 + (float)height, 0.15f, (float *)hudWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        }
    }
    CG_SetScreenPlacement(placement);
}

/* ---------------------------------------------------------- voice speakers */

void CG_DrawVoiceIndicator(void) {
    static const vec4_t red = { 0.5f, 0, 0, 0.5f }, blue = { 0, 0.25f, 0.5f, 0.5f }, grey = { 0.5f, 0.5f, 0.5f, 0.5f };
    const int mode = cg_voiceChatIndicator.integer;
    int placement, client;
    float y = 150;
    if (!mode || !cg.snap || cg.snap->ps.pm_type == PM_INTERMISSION) return;
    placement = CG_SetScreenPlacement(mode == 1 ? CG_PLACE_RIGHT : CG_PLACE_LEFT);
    CG_SetTextFont(0);
    for (client = 0; client < cgs.maxclients && client < MAX_CLIENTS; client++) {
        clientInfo_t *ci = &cgs.clientinfo[client];
        float w, x;
        if (!ci->speaking) continue;
        w = (float)(CG_Text_WidthInFont(ci->name, 0, 0.25f, 0) + 25);
        x = mode == 1 ? 640 - w : 0;
        CG_FillRect(x, y - 10, w, 15, ci->team == TEAM_RED ? red : ci->team == TEAM_BLUE ? blue : grey);
        CG_DrawPic(x + 5, y - 8, 10, 10, cgs.media.speakingIcon);
        CG_Text_Paint(x + 20, y + 1, 0.25f, (float *)hudWhite, ci->name, 0, 0, 0);
        y += 15;
        if (cg.time > ci->speakingTime + 500 && client != cg.clientNum) ci->speaking = 0;
    }
    CG_SetScreenPlacement(placement);
}

/* -------------------------------------------------------------- frame rate */

float CG_DrawFrameRate(float y) {
    static int previous, times[4], frames;
    const int now = trap_Milliseconds();
    const char *text;
    int total = 0, fps, i, height, placement;
    times[frames % 4] = now - previous;
    previous = now;
    if (++frames <= 4) return y;
    for (i = 0; i < 4; i++) total += times[i];
    if (!total) total = 1;
    fps = 4000 / total;
    if (abs(com_maxfps.integer - fps) <= 1) fps = com_maxfps.integer;
    text = va("%ifps", fps);
    height = CG_Text_HeightInFont(text, 2, 0.25f, 0);
    placement = CG_SetScreenPlacement(CG_PLACE_RIGHT);
    CG_SetTextFont(2);
    CG_Text_Paint((float)(635 - CG_Text_WidthInFont(text, 2, 0.25f, 0)), y + height, 0.25f, (float *)hudWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
    CG_SetTextFont(0);
    CG_SetScreenPlacement(placement);
    return y + height;
}

/* ---------------------------------------------------------------- vignette */

void CG_DrawVignette(void) {
    /* Retail's placement 0 stretches the 640x480 canvas over the screen;
     * FnQL-game's default placement centres it, so draw in pixels. */
    if (!cg_vignette.integer) return;
    trap_R_DrawStretchPic(0, 0, (float)cgs.glconfig.vidWidth, (float)cgs.glconfig.vidHeight,
                          0, 0, 1, 1, cgs.media.vignetteShader);
}

/* CG_SPEEDOMETER (96), retail 0x100350c0: the current sample in font 0 with
 * the item's scale, colour and style, centred on the origin for centre
 * alignment. Its presence turns an unset speedometer into mode 4 (text
 * from the HUD only). */
void CG_DrawSpeedometerOwnerDraw(float x, float y, int align, float scale, vec4_t color, int style) {
    const char *text;
    if (!cg_speedometer.integer) trap_Cvar_Set("cg_speedometer", "4");
    text = va("%d", (int)speedSamples[speedHead]);
    CG_SetTextFont(0);
    if (align == ITEM_ALIGN_CENTER) x -= (float)(CG_Text_WidthInFont(text, 0, scale, 0) / 2);
    CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
}
