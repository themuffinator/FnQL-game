/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Tinted status pictures used by every retail HUD. Observed:
 *
 * - CG_ARMORTIERED_COLORIZED (43, 0x1002f780): the item's picture tinted
 *   by stats[STAT_ARMOR_TIER] (0 green, 1 yellow, 2 red) at the item's
 *   alpha. Retail leaves the tint unset for other tiers; FnQL-game keeps the
 *   item color there.
 * - CG_HEALTH_COLORIZED (338, 0x1002f6b0): the picture tinted by the
 *   followed player's health (0 when its entity is dead) through the
 *   handicapped health tint 0x1000ca90, at the item's alpha.
 * - CG_PLAYER_HASKEY (59, 0x10031f90): the silver, gold and master key
 *   icons held in stats[STAT_KEY_MASK], each half a width further right. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

static void CG_StatusPicture(float x, float y, float w, float h, const float *tint, qhandle_t shader) {
    trap_R_SetColor(tint);
    CG_DrawPic(x, y, w, h, shader);
    trap_R_SetColor(NULL);
}

void CG_DrawArmorTierColorized(float x, float y, float w, float h, const vec4_t color, qhandle_t shader) {
    vec4_t tint;
    int tier;
    if (!cg.snap) return;
    tier = cg.snap->ps.stats[STAT_ARMOR_TIER];
    Vector4Copy(color, tint);
    if (tier == 0) VectorSet(tint, 0, 1, 0);
    else if (tier == 1) VectorSet(tint, 1, 1, 0);
    else if (tier == 2) VectorSet(tint, 1, 0, 0);
    CG_StatusPicture(x, y, w, h, tint, shader);
}

void CG_DrawHealthColorized(float x, float y, float w, float h, const vec4_t color, qhandle_t shader) {
    const centity_t *cent;
    vec4_t tint;
    int client, health;
    if (!cg.snap) return;
    client = cg.snap->ps.clientNum;
    if (client < 0 || client >= MAX_GENTITIES) return;
    cent = &cg_entities[client];
    health = (cent->currentState.eFlags & EF_DEAD) ? 0 : cg.snap->ps.stats[STAT_HEALTH];
    CG_OverheadHealthColor(cent->currentState.clientNum, health, tint);
    tint[3] = color[3];
    CG_StatusPicture(x, y, w, h, tint, shader);
}

void CG_DrawHeldKeys(float x, float y, float w, float h) {
    static const int keys[] = { KEY_SILVER, KEY_GOLD, KEY_MASTER };
    float offset = 0;
    int i, j;
    if (!cg.snap) return;
    for (i = 0; i < (int)ARRAY_LEN(keys); ++i) {
        if (!(cg.snap->ps.stats[STAT_KEY_MASK] & keys[i])) continue;
        for (j = 1; j < bg_numItems; ++j)
            if (bg_itemlist[j].giType == IT_KEY && bg_itemlist[j].giTag == keys[i]) break;
        if (j >= bg_numItems) continue;
        CG_RegisterItemVisuals(j);
        CG_DrawPic(x + offset, y, w, h, cg_items[j].icon);
        offset += w * 0.5f;
    }
}
