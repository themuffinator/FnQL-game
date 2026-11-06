/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Quake Live crosshair, retail 0x1000c2a0 with hit styles 0x1000c0a0.
 * Observed:
 *
 * - cg_drawCrosshair picks crosshair n % 30; zero (or a multiple of 30)
 *   and third-person views draw nothing.
 * - Colour: with cg_crosshairHealth the health/armor tint, otherwise the
 *   26-entry cg_crosshairColor table; either scaled by
 *   cg_crosshairBrightness, alpha 1.
 * - Size cg_crosshairSize (default 32). When the last item pickup is newer
 *   than the last hit, cg_crosshairPulse grows it by up to t / 200 for
 *   200 ms; otherwise the hit style applies.
 * - A hit is recorded when PERS_HITS rises outside intermission for a
 *   non-spectator (0x10043c2c): the time, and level generic1 / 64 + 1.
 *   cg_crosshairHitStyle 1-8 combine a colour (by level: 1 light blue,
 *   2 yellow, 3 orange, 4 red; or the cg_crosshairHitColor entry) and a
 *   growth of f * (level, or 1 for the fixed styles), f = elapsed /
 *   cg_crosshairHitTime, while elapsed is in (0, hit time). The hit colour
 *   replaces the crosshair colour unscaled.
 * - Placement uses vidWidth / 640 for size and offsets on both axes,
 *   centred in the refdef, with cg_crosshairX/Y. */

#include "cg_local.h"

static const float crosshairColors[26][3] = {
    { 1, 0, 0 }, { 1, 0.2734375f, 0 }, { 1, 0.5f, 0 }, { 1, 0.734375f, 0 }, { 1, 1, 0 },
    { 0.734375f, 1, 0 }, { 0.5f, 1, 0 }, { 0.2734375f, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0.2734375f },
    { 0, 1, 0.5f }, { 0, 1, 0.734375f }, { 0, 1, 1 }, { 0, 0.734375f, 1 }, { 0, 0.5f, 1 },
    { 0, 0.2734375f, 1 }, { 0, 0, 1 }, { 0.2734375f, 0, 1 }, { 0.5f, 0, 1 }, { 0.734375f, 0, 1 },
    { 1, 0, 1 }, { 1, 0, 0.734375f }, { 1, 0, 0.5f }, { 1, 0, 0.2734375f }, { 1, 1, 1 },
    { 0.5f, 0.5f, 0.5f } };

void CG_CrosshairColorEntry(int index, vec4_t color) {
    int entry = index - 1;
    if (entry < 0 || entry > 25) entry = 0;
    VectorCopy(crosshairColors[entry], color);
    color[3] = 1;
}

void CG_RecordCrosshairHit(const playerState_t *ps, const playerState_t *ops) {
    if (ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS]) {
        cg.crosshairHitLevel = (ps->generic1 >> 6) + 1;
        cg.crosshairHitTime = cg.time;
    }
}

/* Retail 0x1000c0a0. Returns qtrue when it set the colour. */
qboolean CG_CrosshairHitStyle(float *w, float *h, vec4_t color) {
    static const struct { qboolean tint, grow, fixed, custom; } styles[8] = {
        { qtrue, qfalse, qfalse, qfalse }, { qtrue, qfalse, qfalse, qtrue },
        { qfalse, qtrue, qfalse, qfalse }, { qtrue, qtrue, qfalse, qfalse },
        { qtrue, qtrue, qfalse, qtrue }, { qfalse, qtrue, qtrue, qfalse },
        { qtrue, qtrue, qtrue, qfalse }, { qtrue, qtrue, qtrue, qtrue } };
    const int style = cg_crosshairHitStyle.integer - 1;
    float elapsed, fraction;
    if (style < 0 || style > 7) return qfalse;
    elapsed = (float)(cg.time - cg.crosshairHitTime);
    if (!(elapsed > 0) || !(cg_crosshairHitTime.value > elapsed)) return qfalse;
    fraction = elapsed / cg_crosshairHitTime.value;
    if (styles[style].grow) {
        const float amount = styles[style].fixed ? 1.0f : (float)cg.crosshairHitLevel;
        const float factor = (float)(amount * fraction + 1.0);
        *h *= factor;
        *w *= factor;
    }
    if (!styles[style].tint) return qfalse;
    if (styles[style].custom) {
        CG_CrosshairColorEntry(cg_crosshairHitColor.integer, color);
        return qtrue;
    }
    color[0] = 0.25f; color[1] = 0.5f; color[2] = 1; color[3] = 1;
    switch (cg.crosshairHitLevel) {
    case 2: color[0] = 1; color[1] = 1; color[2] = 0; break;
    case 3: color[0] = 1; color[1] = 0.5f; color[2] = 0; break;
    case 4: color[0] = 1; color[1] = 0; color[2] = 0; break;
    default: break;
    }
    return qtrue;
}

void CG_DrawQLCrosshair(void) {
    const int index = cg_drawCrosshair.integer;
    vec4_t color;
    float w, h, x, y, scale, brightness = cg_crosshairBrightness.value;
    if (index <= 0 || index % 30 == 0 || cg.renderingThirdPerson || !cg.snap) return;
    if (cg_crosshairHealth.integer) {
        CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH], cg.snap->ps.stats[STAT_ARMOR], color);
    } else {
        CG_CrosshairColorEntry(cg_crosshairColor.integer, color);
    }
    color[0] *= brightness; color[1] *= brightness; color[2] *= brightness; color[3] = 1;
    w = h = cg_crosshairSize.value;
    if (cg.itemPickupBlendTime > cg.crosshairHitTime) {
        if (cg_crosshairPulse.integer) {
            const float elapsed = (float)(cg.time - cg.itemPickupBlendTime);
            if (elapsed > 0 && 200.0 > elapsed) w = h = (float)((elapsed / 200.0 + 1.0) * w);
        }
    } else {
        CG_CrosshairHitStyle(&w, &h, color);
    }
    trap_R_SetColor(color);
    scale = (float)(cgs.glconfig.vidWidth / 640.0);
    x = (float)cg_crosshairX.integer * scale;
    y = (float)cg_crosshairY.integer * scale;
    w *= scale;
    h *= scale;
    trap_R_DrawStretchPic((float)(cg.refdef.x + x + (cg.refdef.width - h) * 0.5),
                          (float)(cg.refdef.y + y + (cg.refdef.height - w) * 0.5), h, w, 0, 0, 1, 1,
                          cgs.media.crosshairShader[(index % 30 - 1 + CG_NUM_CROSSHAIRS) % CG_NUM_CROSSHAIRS]);
    trap_R_SetColor(NULL);
}
