/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Overhead names, health and armor (retail cgame 0x1000cb60, drawn from
 * CG_Draw2D after the world markers). Observed retail behavior:
 *
 * - Free-flying spectators see nothing unless cg_specNames is set; anyone
 *   following a player, or playing, does.
 * - Each valid, living, non-local player entity in front of the view is
 *   placed 40 units above its origin and located on screen by its window
 *   fractions (engine import 121, window[2] across and window[3] up).
 * - Mode 2 of cg_teammateNames (teammates) or cg_specNames (spectators)
 *   always names the player; mode 1 only when the point falls in the
 *   central window band (0.4..0.6 across, 0.4..0.7 up).
 * - The same modes, when nonzero, add four health segments and an armor
 *   strip under the name, then the carried powerup icons, or the friend
 *   marker for a teammate without powerups. Frozen players get neither.
 *
 * Retail also runs a trace from the view to the player, but never reads the
 * result; FnQL-game skips it. Positions are in pixels, as retail draws them
 * directly with the native text and picture imports. */

#include "cg_local.h"
#include <limits.h>
#include <stdio.h>

#define OVERHEAD_TEXT_SCALE 0.12f
#define OVERHEAD_MIN_BOX 11

/* Retail 0x10012b50 / 0x10078610: name colors by team. */
static const vec4_t cg_overheadTeamColors[5] = {
    { 1.0f, 1.0f, 1.0f, 1.0f },       /* TEAM_FREE */
    { 1.0f, 0.5f, 0.5f, 1.0f },       /* TEAM_RED */
    { 0.5f, 0.75f, 1.0f, 1.0f },      /* TEAM_BLUE */
    { 0.85f, 0.85f, 0.85f, 1.0f },    /* TEAM_SPECTATOR */
    { 1.0f, 0.8f, 0.2f, 1.0f },       /* anything else */
};

const float *CG_OverheadTeamColor(int team) {
    return cg_overheadTeamColors[team >= TEAM_FREE && team <= TEAM_SPECTATOR ? team : 4];
}

/* Retail 0x1000ca90: the player's handicapped maximum health is
 * handicap / 100 * g_startingHealth, falling back to g_startingHealth when
 * that is below 1 or above it. Below the maximum the tint runs red to
 * yellow; at or above it, yellow towards white (blue = value / 2max). */
void CG_OverheadHealthColor(int client, int value, vec4_t color) {
    int starting = cgs.startingHealth, maximum = starting;
    if (client >= 0 && client < MAX_CLIENTS) {
        double scaled = cgs.clientinfo[client].handicap / 100.0 * starting;
        if (scaled >= INT_MIN && scaled <= INT_MAX) maximum = (int)scaled;
    }
    if (maximum < 1 || maximum > starting) maximum = starting;
    color[0] = 1;
    color[3] = 1;
    if (value < maximum) {
        float fraction = maximum > 0 ? (float)value / maximum : 0;
        color[1] = fraction > 0 ? fraction : 0;
        color[2] = 0;
    } else {
        float fraction = maximum > 0 ? value / (2.0f * maximum) : 1;
        color[1] = 1;
        color[2] = fraction > 1 ? 1 : fraction;
    }
}

/* Retail 0x100011c0: the runes find their persistent-powerup items,
 * invulnerability its holdable, everything else a powerup or team item. */
const gitem_t *CG_PowerupItem(int powerup) {
    int i;
    if (powerup >= PW_SCOUT && powerup <= PW_ARMORREGEN)
        return BG_FindItemForPersistantPowerup(powerup - PW_SCOUT + PP_SCOUT);
    if (powerup == PW_INVULNERABILITY) return BG_FindItemForHoldable(HI_INVULNERABILITY);
    for (i = 0; i < bg_numItems; ++i)
        if (bg_itemlist[i].giType == IT_POWERUP && bg_itemlist[i].giTag == powerup) return &bg_itemlist[i];
    for (i = 0; i < bg_numItems; ++i)
        if (bg_itemlist[i].giType == IT_TEAM && bg_itemlist[i].giTag == powerup) return &bg_itemlist[i];
    return NULL;
}

/* Retail 0x100574c0: drop ^0..^7 color pairs and control characters
 * (bytes from 0x80 up are kept); 0x10057430 then counts what is left,
 * skipping any pair the first pass exposed. */
static void CG_OverheadCleanName(char *text) {
    const char *read = text;
    char *write = text;
    while (*read) {
        if (read[0] == '^' && read[1] >= '0' && read[1] <= '7') {
            read += 2;
            continue;
        }
        if ((unsigned char)*read >= 0x20) *write++ = *read;
        ++read;
    }
    *write = 0;
}

static int CG_OverheadPrintLength(const char *text) {
    int length = 0;
    while (*text) {
        if (text[0] == '^' && text[1] >= '0' && text[1] <= '7') text += 2;
        else { ++text; ++length; }
    }
    return length;
}

/* Names wider than cg_overheadNamesWidth (font 0) lose characters until
 * they fit; retail then keeps one character fewer and appends "...". */
static int CG_OverheadFitName(char *name, size_t size) {
    int length = CG_OverheadPrintLength(name), limit = cg_overheadNamesWidth.integer;
    char prefix[40];
    if (CG_Text_WidthInFont(name, 0, OVERHEAD_TEXT_SCALE, length) <= limit) return length;
    do {
        --length;
    } while (length > 1 && CG_Text_WidthInFont(name, 0, OVERHEAD_TEXT_SCALE, length) > limit);
    if (length < 1) length = 1;
    Q_strncpyz(prefix, name, length < (int)sizeof(prefix) ? length : (int)sizeof(prefix));
    snprintf(name, size, "%s...", prefix); /* retail keeps 39 characters */
    return length + 3;
}

static void CG_OverheadPicture(float x, float y, float w, float h, float s2, float t2, qhandle_t shader) {
    trap_R_DrawStretchPic(x, y, w, h, 0, 0, s2, t2, shader);
}

static void CG_DrawOverheadName(int number) {
    static const vec4_t box = { 0, 0, 0, 0.5f }, shadow = { 0, 0, 0, 1 }, armorColor = { 0, 0.625f, 1, 1 };
    static const vec4_t white = { 1, 1, 1, 1 }, hitColor = { 1, 0, 0, 1 };
    const centity_t *cent = &cg_entities[number];
    const entityState_t *s = &cent->currentState;
    const clientInfo_t *info;
    vec3_t point, delta;
    vec4_t health;
    float across, up, scale = cgs.screenYScale, vidWidth = (float)cgs.glconfig.vidWidth;
    float vidHeight = (float)cgs.glconfig.vidHeight, nameHeight, top;
    char name[40];
    int client = s->clientNum, length, nameWidth, boxWidth, offset = 0, count = 0, j, team, ownTeam;
    qboolean frozen = qfalse, teammate, spectating, showName, showBars;
    const float *color;

    if (!cent->currentValid || client == cg.snap->ps.clientNum || s->eType != ET_PLAYER || (s->eFlags & EF_DEAD))
        return;
    if (client < 0 || client >= MAX_CLIENTS) return;
    VectorCopy(cent->lerpOrigin, point);
    point[2] += 40;
    VectorSubtract(point, cg.refdef.vieworg, delta);
    if (!(DotProduct(delta, cg.refdef.viewaxis[0]) > 0)) return;
    info = &cgs.clientinfo[client];
    if (!info->infoValid) return;
    if (!trap_R_ProjectScreenFraction(point, &across, &up)) return;

    Q_strncpyz(name, info->name, sizeof(name));
    CG_OverheadCleanName(name);
    length = CG_OverheadFitName(name, sizeof(name));
    nameWidth = CG_Text_WidthInFont(name, 1, OVERHEAD_TEXT_SCALE, 0);
    nameHeight = (float)CG_Text_HeightInFont(name, 1, OVERHEAD_TEXT_SCALE, 0);
    boxWidth = nameWidth > OVERHEAD_MIN_BOX ? nameWidth : OVERHEAD_MIN_BOX;

    for (j = 1; j <= 16; ++j) {
        if (!(s->powerups & (1 << j)) || j == PW_FLIGHT) continue;
        if (j == PW_FROZEN) frozen = qtrue;
        else ++count;
    }

    ownTeam = cg.snap->ps.persistant[PERS_TEAM];
    team = info->team;
    teammate = cgs.gametype >= GT_TEAM && ownTeam == team;
    spectating = ownTeam == TEAM_SPECTATOR;
    color = CG_OverheadTeamColor(ownTeam);
    if (spectating && cgs.gametype >= GT_TEAM) color = CG_OverheadTeamColor(team);

    if (spectating && cg_specNames.integer == 2) showName = qtrue;
    else if (teammate && cg_teammateNames.integer == 2) showName = qtrue;
    else if ((spectating && (cg_specNames.integer & 1)) || (teammate && (cg_teammateNames.integer & 1)))
        showName = (double)across > 0.4 && (double)across < 0.6 && (double)up > 0.4 && (double)up < 0.7;
    else showName = qfalse;

    if (frozen) showBars = qfalse;
    else if (teammate && cg_teammateNames.integer) showBars = qtrue;
    else showBars = spectating && cg_specNames.integer;

    /* Rows are measured down from the projected point: the name's baseline
     * sits 8 units above its height, the bars 6, the icons 1. */
    top = vidHeight - up * vidHeight - nameHeight * scale;
    if (showName) {
        float x = across * vidWidth - boxWidth * 0.5f * scale, y = top - 8 * scale;
        float size = (float)((float)(cgs.glconfig.vidHeight / 768.0 * 96.0) * (double)OVERHEAD_TEXT_SCALE);
        if (boxWidth > nameWidth) offset = (boxWidth - nameWidth) / 2;
        trap_R_SetColor(box);
        CG_OverheadPicture(x - scale, y - scale - nameHeight * scale, boxWidth * scale + 2 * scale,
                           nameHeight * scale + 2 * scale + (showBars ? 6 * scale : 0), 0, 0, cgs.media.whiteShader);
        trap_R_SetColor(shadow);
        trap_R_DrawText((int)(x + scale + offset * scale), (int)(y + scale), name, 1, size, length, 0);
        trap_R_SetColor(color);
        trap_R_DrawText((int)(x + offset * scale), (int)y, name, 1, size, length, 0);
        trap_R_SetColor(NULL);
    }
    if (!showBars) return;

    {
        float x = across * vidWidth - 5.5f * scale, y = top - 6 * scale, iconY = top - scale;
        int maximum = cg.snap->ps.stats[STAT_MAX_HEALTH], armorWidth = 11;
        static const float segments[3] = { 0.25f, 0.5f, 0.75f };
        int i;
        CG_OverheadHealthColor(client, s->health, health);
        trap_R_SetColor(health);
        CG_OverheadPicture(x, y, 2 * scale, 2 * scale, 1, 1, cgs.media.healthSegmentShader);
        trap_R_SetColor(NULL);
        for (i = 0; i < 3; ++i) {
            if (!((double)s->health > (double)segments[i] * maximum)) continue;
            trap_R_SetColor(health);
            CG_OverheadPicture(x + 3 * (i + 1) * scale, y, 2 * scale, 2 * scale, 1, 1, cgs.media.healthSegmentShader);
            trap_R_SetColor(NULL);
        }
        if (s->armor < maximum) armorWidth = maximum > 0 ? (int)((double)s->armor / maximum * 11.0) : 0;
        if (s->armor) {
            trap_R_SetColor(armorColor);
            CG_OverheadPicture(x, y + 3 * scale, armorWidth * scale, scale, 0, 0, cgs.media.whiteShader);
            trap_R_SetColor(NULL);
        }

        if (count) {
            float iconX = across * vidWidth - 4 * count * scale - (count - 1) * scale;
            for (j = 1; j <= 16; ++j) {
                const gitem_t *item;
                if (!(s->powerups & (1 << j)) || j == PW_FLIGHT || j == PW_FROZEN) continue;
                item = CG_PowerupItem(j);
                if (!item) continue;
                trap_R_SetColor(white);
                CG_OverheadPicture(iconX, iconY, 8 * scale, 8 * scale, 1, 1, trap_R_RegisterShader(item->icon));
                trap_R_SetColor(NULL);
                iconX += 10 * scale;
            }
        } else if (teammate) {
            qboolean hit = cg.time - cent->pe.painTime < cg_drawHitFriendTime.integer;
            trap_R_SetColor(hit ? hitColor : health);
            CG_OverheadPicture(across * vidWidth - 4 * scale, iconY, 8 * scale, 8 * scale, 1, 1,
                               hit ? cgs.media.friendHitShader : cgs.media.friendShader);
            trap_R_SetColor(NULL);
        }
    }
}

void CG_DrawOverheadNames(void) {
    int i, players;
    if (!cg.snap) return;
    if (cg.predictedPlayerState.pm_type == PM_SPECTATOR && !(cg.predictedPlayerState.pm_flags & PMF_FOLLOW) &&
        !cg_specNames.integer)
        return;
    players = cgs.maxclients < MAX_CLIENTS ? cgs.maxclients : MAX_CLIENTS;
    for (i = 0; i < players; ++i) CG_DrawOverheadName(i);
}
