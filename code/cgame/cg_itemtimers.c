/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Spectator item timers (cg_specItemTimers). Observed in retail cgame:
 *
 * - EV_ITEM_PICKUP_SPEC fills up to ten records (0x10019d90; see
 *   CG_RecordSpectatorPickup). Map restart clears them (0x1001c8d0).
 * - Once per frame, beside the frame-time update (0x1004e80b calling
 *   0x1001ca90), a spectator's live records lose the frame time, a
 *   non-positive remainder becoming zero, unless CS_TIMEOUT_START_TIME is
 *   set (0x10049cb5); then the filled records are sorted (0x1001ca10):
 *   outside CTF by origin z, y, x, all descending; in CTF by side
 *   descending, flag distance ascending, then group ascending when both are
 *   grouped (else origin z descending) and entity number descending.
 * - The painter (0x1000f700) runs in the spectator branch after the
 *   followed-view block (0x100110b5). It needs the cvar nonzero, the
 *   scoreboard hidden, a record and EF_SPECTATOR. Bits filter the rows:
 *   1 powerups, 2 mega health, 4 red armor, 8 yellow armor. Size is
 *   cg_specItemTimersSize * 100 from (cg_specItemTimersX, Y). Red (side 1)
 *   and blue (side 2) rows get a 2.25 * size wide, quarter-alpha fill. An
 *   ungrouped icon is size square; grouped icons are half size and step
 *   size / 4 diagonally per side count. The whole seconds left (ceil) draw
 *   white, shadowed, in font 0 at the cvar scale, at
 *   (x + size + 3, y + size - 5). The row advances size + 2 unless the
 *   icon is grouped and its side count is below the grouped-nonred total.
 * Retail writes the quarter alpha into its shared red and blue colour
 * tables; this module uses private colours instead. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

void CG_ClearTimedItems(void) {
    memset(cg.timedItems, 0, sizeof(cg.timedItems));
    cg.timedItemCount = 0;
}

/* Retail subtracts ints and lets them wrap; do the same without signed
 * overflow. */
static int CG_TimedItemDiff(int a, int b) {
    return (int)((unsigned)a - (unsigned)b);
}

static int QDECL CG_TimedItemCompare(const void *left, const void *right) {
    const cgTimedItem_t *a = (const cgTimedItem_t *)left, *b = (const cgTimedItem_t *)right;
    int result;
    if (cgs.gametype != GT_CTF) {
        result = CG_TimedItemDiff((int)b->origin[2], (int)a->origin[2]);
        if (!result) result = CG_TimedItemDiff((int)b->origin[1], (int)a->origin[1]);
        if (!result) result = CG_TimedItemDiff((int)b->origin[0], (int)a->origin[0]);
        return result;
    }
    result = CG_TimedItemDiff(b->side, a->side);
    if (!result) result = CG_TimedItemDiff(a->distance, b->distance);
    if (!result) {
        if (a->group && b->group) result = CG_TimedItemDiff(a->group, b->group);
        else result = CG_TimedItemDiff((int)b->origin[2], (int)a->origin[2]);
        if (!result) result = CG_TimedItemDiff(b->entity, a->entity);
    }
    return result;
}

void CG_UpdateTimedItems(int frametime) {
    qboolean paused;
    int i, count;
    if (!(cg.predictedPlayerState.eFlags & EF_SPECTATOR)) return;
    paused = atoi(CG_ConfigString(CS_TIMEOUT_START_TIME)) != 0;
    if (!paused) {
        for (i = 0; i < CG_MAX_TIMED_ITEMS; ++i) {
            cgTimedItem_t *timer = &cg.timedItems[i];
            if (!timer->entity) continue;
            timer->remaining = timer->remaining > 0 ? timer->remaining - frametime : 0;
        }
    }
    count = cg.timedItemCount;
    if (count > CG_MAX_TIMED_ITEMS) count = CG_MAX_TIMED_ITEMS;
    if (count > 1) qsort(cg.timedItems, (size_t)count, sizeof(cg.timedItems[0]), CG_TimedItemCompare);
}

qboolean CG_TimedItemShown(int mask, const gitem_t *item) {
    if (!(mask & 1) && item->giType == IT_POWERUP) return qfalse;
    if (!(mask & 2) && item->giType == IT_HEALTH && item->giTag == HEALTH_TIER_MEGA) return qfalse;
    if (!(mask & 4) && item->giType == IT_ARMOR && item->giTag == ARMOR_TIER_RED) return qfalse;
    if (!(mask & 8) && item->giType == IT_ARMOR && item->giTag == ARMOR_TIER_YELLOW) return qfalse;
    return qtrue;
}

void CG_DrawItemTimers(void) {
    static const vec4_t sideTint[2] = { { 1.0f, 0.2f, 0.1f, 0.25f }, { 0.2f, 0.4f, 1.0f, 0.25f } };
    static vec4_t white = { 1, 1, 1, 1 };
    const int mask = cg_specItemTimers.integer;
    float size, x, y;
    int i, total = 0, redGrouped = 0, otherGrouped = 0, sideCount = 0;
    if (!mask || cg.scoreBoardShowing || cg.timedItemCount <= 0 ||
        !(cg.predictedPlayerState.eFlags & EF_SPECTATOR)) return;
    size = (float)(cg_specItemTimersSize.value * 100.0);
    x = (float)cg_specItemTimersX.integer;
    y = (float)cg_specItemTimersY.integer;
    for (i = 0; i < CG_MAX_TIMED_ITEMS; ++i) {
        const cgTimedItem_t *timer = &cg.timedItems[i];
        if (timer->entity && timer->side != 1 && timer->group) ++total;
    }
    for (i = 0; i < CG_MAX_TIMED_ITEMS; ++i) {
        const cgTimedItem_t *timer = &cg.timedItems[i];
        const gitem_t *item;
        qhandle_t icon;
        int seconds;
        if (!timer->entity || timer->item <= 0 || timer->item >= bg_numItems) continue;
        item = &bg_itemlist[timer->item];
        if (!CG_TimedItemShown(mask, item)) continue;
        seconds = (int)ceil(timer->remaining / 1000.0);
        if (timer->side == 1 || timer->side == 2) {
            CG_FillRect(x, y, (float)(size * 2.25), size, sideTint[timer->side - 1]);
        }
        icon = trap_R_RegisterShader(item->icon);
        if (timer->group) {
            const double step = size * 0.25;
            float count;
            sideCount = timer->side == 1 ? ++redGrouped : ++otherGrouped;
            count = (float)sideCount;
            CG_DrawPic((float)(count * step - 1.0 + x - step), (float)(count * step - 1.0 + y - step),
                       (float)(size * 0.5), (float)(size * 0.5), icon);
        } else {
            CG_DrawPic(x, y, size, size, icon);
        }
        if (seconds > 0) {
            CG_SetTextFont(0);
            CG_Text_Paint((float)(size + x + 3.0), (float)(size + y - 5.0), cg_specItemTimersSize.value,
                          white, va("%d", seconds), 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        }
        if (!timer->group || sideCount >= total) y = (float)(size + 2.0 + y);
    }
}
