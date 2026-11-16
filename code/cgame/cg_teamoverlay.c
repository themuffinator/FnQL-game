/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Team overlay (cg_drawTeamOverlay), retail 0x10009da0. Mode 1 is drawn by
 * the upper-right block (0x1000a770) from y 0 downwards at the right
 * placement; mode 2 by CG_Draw2D (0x10011141) above y 408 at the right
 * placement. Both need a team gametype. Observed:
 *
 * - Needs the cvar, the snapshot player on red or blue and no EF_SPECTATOR.
 *   Rows come from the tinfo list (at most 8): a client with valid info on
 *   that team, other than the local client unless cg_selfOnTeamOverlay.
 * - With s = cg_drawTeamOverlaySize: h is the height of "O" in font 0; the
 *   name column is the widest name in font 1 (limit 20), the location
 *   column the widest location (CS_LOCATIONS + 1 .. + 63, font 1), at most
 *   64. Width 13h + names + locations; rows h + 3 apart.
 * - x is 640 - width + cg_drawTeamOverlayX on the right, else the X cvar
 *   alone, truncated. Upper overlays start at y - Y and return y + height;
 *   lower ones end at y - Y and return their top. A positive Y moves the
 *   overlay up.
 * - The background is gfx/2d/colorbar in (0.75, 0.5, 0.5) for red or
 *   (0.5, 0.5, 0.75) for blue at cg_drawTeamOverlayOpacity.
 * - Each row: the name (visible length over 20 keeps 19 bytes plus "...")
 *   at x + h, white, font 1, limit 20, shadowed, with its baseline the name's
 *   height below the row top; health at x + 2h + names and armor 3h further
 *   in font 2 ("%3i", tinted by health and armor; "FROZEN" in
 *   (0.2, 0.2, 0.75) or "DEAD" in red with no armor); the weapon icon (none
 *   while frozen; defer when missing) h square at x + 8h + names; the
 *   location ("unknown" when empty) in white font 1 at x + 10h + names when
 *   there is a location column; carried powerups 1-16 except flight and
 *   frozen, h square, from x stepping left (right overlay) or from
 *   x + width - h stepping right.
 * - Health, armor, location, weapon, powerups and flags come from the
 *   client's entity state, or the snapshot player state for the snapshot
 *   client (0x1000a167..0x1000a1c1). */

#include "cg_local.h"
#include "../ui/ui_shared.h"

#define TEAM_OVERLAY_ROWS 8
#define TEAM_OVERLAY_NAME 20

/* The name as retail shows it (0x1000a23f..0x1000a306). */
void CG_TeamOverlayName(const char *raw, char *out, int size) {
    char name[40];
    const char *p;
    int visible = 0;
    Q_strncpyz(name, raw ? raw : "", sizeof(name));
    for (p = name; *p; ) {
        if (p[0] == Q_COLOR_ESCAPE && p[1] >= '0' && p[1] <= '7') {
            p += 2;
        } else {
            ++p; ++visible;
        }
    }
    if (visible > TEAM_OVERLAY_NAME) {
        char head[20];
        Q_strncpyz(head, name, sizeof(head));
        Com_sprintf(name, sizeof(name), "%s...", head);
    }
    Q_strncpyz(out, name, size);
}

static qboolean CG_TeamOverlayRow(int client, int team, int skip) {
    const clientInfo_t *ci;
    if (client < 0 || client >= MAX_CLIENTS || client == skip) return qfalse;
    ci = &cgs.clientinfo[client];
    return ci->infoValid && ci->team == team;
}

float CG_DrawTeamOverlay(float y, qboolean right, qboolean upper) {
    static const vec4_t white = { 1, 1, 1, 1 };
    static const vec4_t frozen = { 0.2f, 0.2f, 0.75f, 1 };
    static const vec4_t dead = { 1, 0, 0, 1 };
    float s = cg_drawTeamOverlaySize.value, top, rowY, totalH;
    int team, skip, count, rows = 0, h, nameW = 0, locW = 0, totalW, x, i, ret;
    vec4_t back;

    if (!cg_drawTeamOverlay.integer || !cg.snap) return y;
    team = cg.snap->ps.persistant[PERS_TEAM];
    if ((team != TEAM_RED && team != TEAM_BLUE) || (cg.snap->ps.eFlags & EF_SPECTATOR)) return y;
    skip = cg_selfOnTeamOverlay.integer ? -1 : cg.clientNum;
    count = numSortedTeamPlayers < TEAM_OVERLAY_ROWS ? numSortedTeamPlayers : TEAM_OVERLAY_ROWS;
    h = CG_Text_HeightInFont("O", 0, s, 0);
    for (i = 0; i < count; ++i) {
        int width;
        if (!CG_TeamOverlayRow(sortedTeamPlayers[i], team, skip)) continue;
        ++rows;
        width = CG_Text_WidthInFont(cgs.clientinfo[sortedTeamPlayers[i]].name, 1, s, TEAM_OVERLAY_NAME);
        if (width > nameW) nameW = width;
    }
    if (!rows) return y;
    for (i = 1; i < 64; ++i) {
        const char *location = CG_ConfigString(CS_LOCATIONS + i);
        if (location && *location) {
            int width = CG_Text_WidthInFont(location, 1, s, 0);
            if (width > locW) locW = width;
        }
    }
    if (locW > 64) locW = 64;
    totalW = 13 * h + locW + nameW;
    x = right ? (int)((float)(640 - totalW) + cg_drawTeamOverlayX.value) : (int)cg_drawTeamOverlayX.value;
    totalH = (float)((h + 3) * rows);
    if (upper) {
        ret = (int)(totalH + y);
        top = y - cg_drawTeamOverlayY.value;
    } else {
        top = y - (totalH + cg_drawTeamOverlayY.value);
        ret = (int)top;
    }

    back[0] = team == TEAM_RED ? 0.75f : 0.5f;
    back[1] = 0.5f;
    back[2] = team == TEAM_RED ? 0.5f : 0.75f;
    back[3] = cg_drawTeamOverlayOpacity.value;
    trap_R_SetColor(back);
    CG_DrawPic((float)x, top, (float)totalW, totalH, cgs.media.teamStatusBar);
    trap_R_SetColor(NULL);

    rowY = top;
    for (i = 0; i < count; ++i) {
        const int client = sortedTeamPlayers[i];
        const entityState_t *state;
        int health, armor, location, weapon, powerups, eFlags, column, j, icon;
        char name[40], healthText[16], armorText[16];
        float textY;
        vec4_t tint;
        if (!CG_TeamOverlayRow(client, team, skip)) continue;
        state = &cg_entities[client].currentState;
        if (state->clientNum == cg.snap->ps.clientNum) {
            health = cg.snap->ps.stats[STAT_HEALTH];
            armor = cg.snap->ps.stats[STAT_ARMOR];
            location = cg.snap->ps.location;
        } else {
            health = state->health;
            armor = state->armor;
            location = state->location;
        }
        weapon = state->weapon;
        powerups = state->powerups;
        eFlags = state->eFlags;

        CG_TeamOverlayName(cgs.clientinfo[client].name, name, sizeof(name));
        textY = rowY + CG_Text_HeightInFont(name, 1, s, 0);
        CG_SetTextFont(1);
        CG_Text_Paint((float)(x + h), textY, s, (float *)white, name, 0, TEAM_OVERLAY_NAME, ITEM_TEXTSTYLE_SHADOWED);

        column = x + 2 * h + nameW;
        if (powerups & (1 << PW_FROZEN)) {
            Vector4Copy(frozen, tint);
            Q_strncpyz(healthText, "FROZEN", sizeof(healthText));
            armorText[0] = '\0';
        } else if (health <= 0 || (eFlags & EF_DEAD)) {
            Vector4Copy(dead, tint);
            Q_strncpyz(healthText, "DEAD", sizeof(healthText));
            armorText[0] = '\0';
        } else {
            CG_GetColorForHealth(health, armor, tint);
            Com_sprintf(healthText, sizeof(healthText), "%3i", health);
            Com_sprintf(armorText, sizeof(armorText), "%3i", armor);
        }
        CG_SetTextFont(2);
        CG_Text_Paint((float)column, textY, s, tint, healthText, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        CG_Text_Paint((float)(column + 3 * h), textY, s, tint, armorText, 0, 0, ITEM_TEXTSTYLE_SHADOWED);

        column += 6 * h;
        if (!(powerups & (1 << PW_FROZEN))) {
            qhandle_t shader = weapon >= 0 && weapon < MAX_WEAPONS ? cg_weapons[weapon].weaponIcon : 0;
            CG_DrawPic((float)column, rowY, (float)h, (float)h, shader ? shader : cgs.media.deferShader);
        }
        if (locW) {
            const char *where = location >= 0 && location < MAX_LOCATIONS ?
                CG_ConfigString(CS_LOCATIONS + location) : NULL;
            if (!where || !*where) where = "unknown";
            CG_SetTextFont(1);
            CG_Text_Paint((float)(column + 2 * h), textY, s, (float *)white, where, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        }
        CG_SetTextFont(0);

        icon = right ? x : x + totalW - h;
        for (j = 1; j <= 16; ++j) {
            const gitem_t *item;
            if (!(powerups & (1 << j)) || j == PW_FLIGHT || j == PW_FROZEN) continue;
            item = CG_PowerupItem(j);
            if (!item) continue;
            CG_DrawPic((float)icon, rowY, (float)h, (float)h, trap_R_RegisterShader(item->icon));
            icon += right ? -h : h;
        }
        rowY += (float)(h + 3);
    }
    return (float)ret;
}
