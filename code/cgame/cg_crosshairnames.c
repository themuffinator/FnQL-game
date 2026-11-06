/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "cg_local.h"
#include "../ui/ui_shared.h"

/* Retail crosshair health tint (0x1000c822): red to yellow below the
 * snapshot's maximum health, yellow towards white above it. */
static void CG_CrosshairHealthTint(vec4_t color, int value, int maximum) {
    color[0] = 1;
    if (value < maximum) {
        float fraction = maximum > 0 ? (float)value / maximum : 0;
        color[1] = fraction < 1 ? fraction : 1;
        color[2] = 0;
    } else {
        float fraction = maximum > 0 ? value / (2.0f * maximum) : 1;
        color[1] = 1;
        color[2] = fraction > 0 ? (fraction < 1 ? fraction : 1) : 0;
    }
}

/* Retail 0x1000c670 after the crosshair scan, given the faded name color.
 * Teammates (team modes, the snapshot's team) are named only with
 * cg_teammateCrosshairNames and without cg_teammateNames; everyone else only
 * with cg_enemyCrosshairNames. The matching opacity scales the fade. The
 * name is centered at y 190, scale 0.26, shadowed. With
 * cg_drawCrosshairTeamHealth a teammate's "health / armor" follows below,
 * scaled by cg_drawCrosshairTeamHealthSize and tinted in mode 2. */
void CG_DrawCrosshairNameText(float *color) {
    const clientInfo_t *info;
    qboolean teammate;
    int client = cg.crosshairClientNum;
    if (!color || !cg.snap || client < 0 || client >= MAX_CLIENTS) return;
    info = &cgs.clientinfo[client];
    teammate = cgs.gametype >= GT_TEAM && cg.snap->ps.persistant[PERS_TEAM] == info->team;
    if (teammate) {
        if (cg_teammateNames.integer || !cg_teammateCrosshairNames.integer) return;
        color[3] *= cg_teammateCrosshairNamesOpacity.value;
    } else {
        if (!cg_enemyCrosshairNames.integer) return;
        color[3] *= cg_enemyCrosshairNamesOpacity.value;
    }
    CG_Text_Paint(320 - CG_Text_Width(info->name, 0.26f, 0) * 0.5f, 190, 0.26f, color, info->name,
                  0, 0, ITEM_TEXTSTYLE_SHADOWED);
    if (cg_drawCrosshairTeamHealth.value != 0 && teammate) {
        float size = cg_drawCrosshairTeamHealthSize.value, y = size * 16 + 198;
        qboolean tint = cg_drawCrosshairTeamHealth.value == 2;
        int maximum = cg.snap->ps.stats[STAT_MAX_HEALTH];
        char text[32];
        if (tint) CG_CrosshairHealthTint(color, info->health, maximum);
        Com_sprintf(text, sizeof(text), "%i ", info->health);
        CG_Text_Paint(320 - CG_Text_Width(text, size, 0), y, size, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        color[0] = color[1] = color[2] = 1;
        CG_Text_Paint(320, y, size, color, "/", 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        if (tint) CG_CrosshairHealthTint(color, info->armor, maximum);
        Com_sprintf(text, sizeof(text), " %i", info->armor);
        CG_Text_Paint(325, y, size, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
    }
}
