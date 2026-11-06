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
#include "../../docs/contract/retail_weaponcolors.h"
#include <stdio.h>

/* Retail cgame 0x10001e50 consumes at most sixteen case-insensitive tokens.
 * Weapon IDs are the wire IDs, notably ng/pl/cg = 11/12/13. */
static qboolean CG_PlumWeaponEnabled(int weapon) {
    static const char *const tokens[] = {
        "", "g", "mg", "sg", "gl", "rl", "lg", "rg", "pg", "bfg", "gh", "ng", "pl", "cg", "hmg"
    };
    char *cursor = cg_damagePlum.string;
    int i;
    if (weapon <= WP_NONE || weapon >= ARRAY_LEN(tokens)) return qfalse;
    for (i = 0; i < 16; ++i) {
        const char *token = COM_ParseExt(&cursor, qfalse);
        if (!token[0]) break;
        if (!Q_stricmp(token, tokens[weapon])) return qtrue;
    }
    return qfalse;
}

static float CG_PlumRandom(void) {
    /* Retail 0x100137e3; unsigned arithmetic defines wrap on every platform. */
    cg.damagePlumRandom = cg.damagePlumRandom * 69069u + 1u;
    return (cg.damagePlumRandom & 65535u) * (1.0f / 65536.0f);
}

void CG_DamagePlum(const centity_t *cent) {
    const entityState_t *event = &cent->currentState;
    cgDamagePlum_t *plum = NULL;
    int i, damage = event->time;
    /* Retail 0x1001c3cc addresses the snapshot player, including follow mode.
     * 0x1002a0d0 suppresses the world-marker queue for race free spectators. */
    if (!cg.snap || event->clientNum != cg.snap->ps.clientNum ||
        !CG_PlumWeaponEnabled(event->generic1)) return;
    if (cgs.gametype == GT_RACE && (cg.snap->ps.eFlags & EF_SPECTATOR) &&
        !(cg.snap->ps.pm_flags & PMF_FOLLOW)) return;
    for (i = 0; i < CG_MAX_DAMAGE_PLUMS; ++i) {
        cgDamagePlum_t *candidate = &cg.damagePlums[i];
        if (!candidate->active || (unsigned)cg.time - (unsigned)candidate->startedAt >= 1000u) {
            plum = candidate;
            break;
        }
    }
    if (!plum) return; /* Full queue drops new text, never overwrites live text. */
    memset(plum, 0, sizeof(*plum));
    plum->active = qtrue;
    plum->startedAt = cg.time;
    /* Native bounded formatting also handles INT_MIN (the inherited VM
     * formatter negates signed values and cannot represent that magnitude). */
    snprintf(plum->text, sizeof(plum->text), "%i", damage);
    /* Retail 0x10013660: color thresholds are strict greater-than tests. */
    if (cg_damagePlumColorStyle.integer == 3) {
        memcpy(plum->color, cg_weaponPlumColors[event->generic1], sizeof(plum->color));
    } else if (cg_damagePlumColorStyle.integer == 2) {
        plum->color[0] = damage > 25 ? 1.0f : 0.25f;
        plum->color[1] = damage > 75 ? 0.0f : damage > 50 ? 0.5f : damage > 25 ? 1.0f : 0.5f;
        plum->color[2] = damage > 25 ? 0.0f : 1.0f;
        plum->color[3] = 1.0f;
    } else {
        float fraction = damage * 0.01f;
        if (fraction < 0) fraction = 0;
        if (fraction > 1) fraction = 1;
        plum->color[0] = plum->color[3] = 1;
        plum->color[1] = plum->color[2] = 1 - fraction;
    }
    for (i = 0; i < 3; ++i) plum->origin[i] = cent->lerpOrigin[i] + CG_PlumRandom() * 20 - 10;
    plum->horizontalSpeed = CG_PlumRandom() * 100 - 50;
    plum->verticalSpeed = -120 - CG_PlumRandom() * 20;
}

void CG_DrawDamagePlums(void) {
    /* Retail draws plums through the world-marker drawer 0x10011680. The
     * plum record (0x10013660) sets no world width, so the projected marker
     * size is zero: the text is painted at the projected point with the
     * generic HUD text painter 0x10008440 at constant scale 0.15, without
     * width or height centering. Its offsets are vx*t and vy*t + 150*t*t in
     * the 640x480 canvas (x stretched by vidWidth/640, y by vidHeight/480),
     * t in seconds of the one-second life. There is no distance scaling. */
    float size = 0.15f * cgs.glconfig.vidHeight / 8.0f;
    int i;
    if (!(size > 0)) return;
    for (i = 0; i < CG_MAX_DAMAGE_PLUMS; ++i) {
        cgDamagePlum_t *plum = &cg.damagePlums[i];
        unsigned elapsed = (unsigned)cg.time - (unsigned)plum->startedAt;
        vec4_t window, color;
        float seconds, x, y;
        if (!plum->active) continue;
        if (elapsed >= 1000u) { plum->active = qfalse; continue; }
        if (!trap_R_ProjectPoint(plum->origin, window)) continue;
        seconds = elapsed * 0.001f;
        x = cg.refdef.x + window[0] + plum->horizontalSpeed * seconds * (cgs.glconfig.vidWidth / 640.0f);
        y = cg.refdef.y + cg.refdef.height - window[1] +
            (plum->verticalSpeed * seconds + 150 * seconds * seconds) * (cgs.glconfig.vidHeight / 480.0f);
        if (!(x > -4096 && x < cgs.glconfig.vidWidth + 4096.0f) ||
            !(y > -4096 && y < cgs.glconfig.vidHeight + 4096.0f)) continue;
        memcpy(color, plum->color, sizeof(color));
        /* 0x1002a1f6: full alpha for 600 ms, then linear to zero. */
        if (elapsed > 600u) color[3] *= (1000u - elapsed) / 400.0f;
        trap_R_SetColor(color);
        trap_R_DrawText((int)x, (int)y, plum->text, 0, size, -1, 0);
        trap_R_SetColor(NULL);
    }
}
