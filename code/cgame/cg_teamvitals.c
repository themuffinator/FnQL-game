/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Spectator team vitals (cg_specTeamVitals), retail 0x1000fab0, drawn in
 * the spectator branch after the item timers (0x100110c0). Observed:
 *
 * - Needs the cvar nonzero, the scoreboard hidden and a team game; walks
 *   the client entities below sv_maxclients whose eType has a bit of 0xb
 *   and whose client info is valid, skipping spectators.
 * - Red rows stack at x 5 with the left placement, blue rows at
 *   640 - width - 23 with the right placement; both start at
 *   cg_specTeamVitalsY and advance 21 (22 with health colours, which also
 *   lower the row by 5 while drawing).
 * - Health and armor come from the entity state, or from the snapshot for
 *   the followed client, which also gets ui/assets/voiceWindow (18 square
 *   at x - 1, y - 17). A dead player's health is zero. The maximum is the
 *   handicapped g_startingHealth (as for the overhead names).
 * - A faint client number (font 1, 0.12, white at quarter alpha) sits at
 *   x - 5. The first carried powerup's icon (flight and frozen excluded),
 *   else friend_dead for the dead, else the weapon icon, draws 16 square
 *   at (x, y - 16).
 * - The name is colour-stripped (^0..^7 and control characters), and if
 *   it is wider (font 0, 0.18) than the width less 21 in mode 2, cut to one
 *   character fewer than the fitting count and given "...".
 * - Default bars: a team fill (pure red or blue, quarter alpha) and the
 *   health bar (team colour at 0.75, or the bright team colour at 0.5 over
 *   the maximum) at (x + 18, int(y - 16)), text height + 2 high; the name
 *   in white; a 6 high armor bar in the bright team colour at y - 6.
 *   Health colours (cg_specTeamVitalsHealthColor 1): a black quarter-alpha
 *   4 high fill and the health bar in the overhead-name health tint at
 *   y - 5, the name in the overhead team colour and a 1 high cyan armor bar
 *   at y - 1. Bars scale by value / maximum, full width above it.
 * - Mode 2 adds the health number (font 1) at x + 19 and moves the name
 *   21 right; both sit at y - text height + 1, shadowed.
 * Retail writes these alphas into shared colour tables; this module keeps
 * private copies. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

static const float *CG_VitalsBrightColor(int team, vec4_t color, float alpha) {
    static const vec4_t colors[5] = {
        { 1.0f, 0.8f, 0.2f, 1 }, { 1.0f, 0.2f, 0.1f, 1 }, { 0.2f, 0.4f, 1.0f, 1 },
        { 0.75f, 0.75f, 0.75f, 1 }, { 1, 1, 1, 1 } };
    Vector4Copy(colors[team >= TEAM_FREE && team <= TEAM_SPECTATOR ? team : 4], color);
    color[3] = alpha;
    return color;
}

static const float *CG_VitalsPureColor(int team, vec4_t color, float alpha) {
    color[0] = team == TEAM_RED ? 1.0f : 0.0f;
    color[1] = 0;
    color[2] = team == TEAM_RED ? 0.0f : 1.0f;
    color[3] = alpha;
    return color;
}

/* Retail 0x100574c0: drop ^0..^7 colour pairs and control characters. */
void CG_VitalsCleanName(char *name) {
    const char *in = name;
    char *out = name;
    while (*in) {
        if (in[0] == '^' && in[1] >= '0' && in[1] <= '7') {
            in += 2;
            continue;
        }
        if ((unsigned char)*in >= 0x20) *out++ = *in;
        ++in;
    }
    *out = 0;
}

/* Fit the cleaned name into maxWidth; returns the paint limit. */
int CG_VitalsFitName(char *name, int size, float maxWidth) {
    int limit = (int)strlen(name);
    char cut[64];
    if (CG_Text_WidthInFont(name, 0, 0.18f, limit) <= maxWidth) return limit;
    do {
        --limit;
    } while (limit > 0 && CG_Text_WidthInFont(name, 0, 0.18f, limit) > maxWidth);
    if (limit < 1) limit = 1;
    Q_strncpyz(cut, name, limit < (int)sizeof(cut) ? limit : (int)sizeof(cut));
    Com_sprintf(name, size, "%s...", cut);
    return limit + 3;
}

static float CG_VitalsBarWidth(int value, int maximum, float width) {
    if (value > maximum) return width;
    return (float)((double)value / maximum * width);
}

static const gitem_t *CG_VitalsPowerup(int powerups) {
    int powerup;
    if (!powerups) return NULL;
    for (powerup = 1; powerup <= PW_NUM_POWERUPS; ++powerup) {
        const gitem_t *item;
        if (!(powerups & (1 << powerup)) || powerup == PW_FLIGHT || powerup == PW_FROZEN) continue;
        item = powerup < PW_NUM_POWERUPS ? CG_PowerupItem(powerup) : NULL;
        if (item) return item;
    }
    return NULL;
}

void CG_DrawTeamVitals(void) {
    static vec4_t numberColor = { 1, 1, 1, 0.25f }, white = { 1, 1, 1, 1 }, shade = { 0, 0, 0, 0.25f },
                  cyan = { 0, 1, 1, 1 };
    const qboolean healthColors = cg_specTeamVitalsHealthColor.integer != 0;
    const float step = healthColors ? 5.0f : 1.0f;
    const float nameShift = cg_specTeamVitals.integer == 2 ? 21.0f : 0.0f;
    float rowY[TEAM_NUM_TEAMS];
    int i;
    if (!cg_specTeamVitals.integer || cg.scoreBoardShowing || cgs.gametype < GT_TEAM || !cg.snap) return;
    rowY[TEAM_RED] = rowY[TEAM_BLUE] = (float)cg_specTeamVitalsY.integer;
    for (i = 0; i < cgs.maxclients && i < MAX_CLIENTS; ++i) {
        const entityState_t *state = &cg_entities[i].currentState;
        const clientInfo_t *info;
        const gitem_t *powerup;
        const float *textColor, *barColor;
        vec4_t teamColor, barTint;
        char name[64], text[16];
        int client = state->clientNum, team, health, armor, maximum, limit, textHeight;
        qboolean followed, dead;
        float width, x, y, barY, barHeight, armorOffset, armorHeight;
        qhandle_t icon;
        if (!(state->eType & 0xb) || client < 0 || client >= MAX_CLIENTS) continue;
        info = &cgs.clientinfo[client];
        if (!info->infoValid) continue;
        team = info->team;
        if (team == TEAM_SPECTATOR || (team != TEAM_RED && team != TEAM_BLUE)) continue;
        followed = client == cg.snap->ps.clientNum;
        health = followed ? cg.snap->ps.stats[STAT_HEALTH] : state->health;
        armor = followed ? cg.snap->ps.stats[STAT_ARMOR] : state->armor;
        dead = (state->eFlags & EF_DEAD) != 0;
        width = (float)cg_specTeamVitalsWidth.integer;
        x = team == TEAM_RED ? 5.0f : (float)(int)(640.0 - width - 5.0 - 16.0 - 2.0);
        CG_SetScreenPlacement(team == TEAM_RED ? CG_PLACE_LEFT : CG_PLACE_RIGHT);
        maximum = cgs.startingHealth;
        {
            double scaled = info->handicap / 100.0 * cgs.startingHealth;
            if (scaled >= 1 && scaled <= cgs.startingHealth) maximum = (int)scaled;
        }
        y = rowY[team];
        Com_sprintf(text, sizeof(text), "%d", client);
        CG_SetTextFont(1);
        CG_Text_Paint(x - 5.0f, y, 0.12f, numberColor, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        if (followed) CG_DrawPic(x - 1.0f, y - 17.0f, 18, 18, cgs.media.voiceWindowShader);
        powerup = CG_VitalsPowerup(state->powerups);
        if (powerup) icon = trap_R_RegisterShader(powerup->icon);
        else if (state->powerups) icon = 0;
        else if (dead) icon = cgs.media.friendDeadShader;
        else icon = state->weapon > WP_NONE && state->weapon < MAX_WEAPONS ? cg_weapons[state->weapon].weaponIcon : 0;
        if (powerup || !state->powerups) CG_DrawPic(x, y - 16.0f, 16, 16, icon);
        if (healthColors) rowY[team] = y - step;

        Q_strncpyz(name, info->name, 40);
        CG_VitalsCleanName(name);
        limit = CG_VitalsFitName(name, sizeof(name), width - nameShift);
        textHeight = CG_Text_HeightInFont(name, 0, 0.18f, limit);

        if (!healthColors) {
            barY = (float)(int)(rowY[team] - 16.0f);
            CG_FillRect(x + 18.0f, barY, width, (float)(textHeight + 2), CG_VitalsPureColor(team, teamColor, 0.25f));
        }
        if (dead) health = 0;
        if (healthColors) {
            CG_OverheadHealthColor(client, health, barTint);
            barColor = barTint;
            CG_FillRect(x + 18.0f, rowY[team], width, 4, shade);
            barY = (float)(int)rowY[team];
            barHeight = 4;
        } else {
            barColor = health > maximum ? CG_VitalsBrightColor(team, barTint, 0.5f)
                                        : CG_VitalsPureColor(team, barTint, 0.75f);
            barHeight = (float)(textHeight + 2);
        }
        CG_FillRect(x + 18.0f, barY, CG_VitalsBarWidth(health, maximum, width), barHeight, barColor);

        if (healthColors) {
            rowY[team] += step;
            textColor = CG_OverheadTeamColor(team);
        } else {
            rowY[team] -= step;
            textColor = white;
        }
        y = rowY[team];
        if (nameShift != 0) {
            Com_sprintf(text, sizeof(text), "%d", health);
            CG_SetTextFont(1);
            CG_Text_Paint(x + 19.0f, y - textHeight + 1.0f, 0.18f, (float *)textColor, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        }
        CG_SetTextFont(0);
        CG_Text_Paint(x + 18.0f + nameShift + 1.0f, y - textHeight + 1.0f, 0.18f, (float *)textColor, name, 0, limit,
                      ITEM_TEXTSTYLE_SHADOWED);

        if (healthColors) {
            barColor = cyan; armorOffset = -1.0f; armorHeight = 1.0f;
        } else {
            barColor = CG_VitalsBrightColor(team, barTint, 1.0f); armorOffset = -5.0f; armorHeight = 6.0f;
        }
        CG_FillRect(x + 18.0f, y + armorOffset, CG_VitalsBarWidth(armor, maximum, width), armorHeight, barColor);
        CG_SetScreenPlacement(CG_PLACE_DEFAULT);
        rowY[team] = y + 22.0f;
    }
}
