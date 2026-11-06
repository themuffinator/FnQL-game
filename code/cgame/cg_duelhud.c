/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Duel player slots for the 1ST_PLYR/2ND_PLYR ownerdraws (103-190 and
 * 193-280, retail dispatcher cases 0x48-0x5c). Observed:
 *
 * - scores_duel (0x10045440) fills two persistent slots in row order and
 *   clears them during warmup. At intermission with a single row for a new
 *   client, the departed player keeps slot one and the row goes to slot two.
 * - Names and scores come from CS_DUEL_NAME1/2 and CS_DUEL_SCORE1/2
 *   (0x10049531); names wider than 140 units are cut under 132 with "...",
 *   and an empty name at intermission shows "-" (0x10035bd0).
 * - Counters skip a slot without a valid client except at intermission
 *   (0x100361f0 and siblings) and use font 0: frags, deaths, damage, accuracy
 *   ("%d%%"), wins ("%d/%d" from the client's wins and losses), and ping
 *   tinted green to 40, yellow to 80, red beyond, at alpha 0.8.
 * - Warmup shows READY/NOT READY from STAT_CLIENTS_READY, otherwise
 *   LEADS/TIED/TRAILS against the other score (0x10037ba0).
 * - Per-weapon frags/hits/shots/damage/accuracy (0x100368a0..0x10036d60) use
 *   the item font; accuracy above the opponent's turns white at 0.8 alpha.
 * - Medals: excellent, impressive and gauntlet counts (0x10037990).
 * - FLAG draws ui/assets/flags/none.tga (FnQL-game carries no country), and
 *   AVATAR the model icon (no avatar import). Pickup rows and counts
 *   (179-187) follow 0x10036eb0/0x10037730; the health/armor bars (116)
 *   follow 0x10033040. */

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <stdio.h>

static void CG_DuelTint(vec4_t color, float r, float g, float b, float a) {
    color[0] = r; color[1] = g; color[2] = b; color[3] = a;
}

static score_t duelSlots[2];
static qboolean duelSlotsReset = qtrue;

void CG_ClearDuelSlots(void) {
    memset(duelSlots, 0, sizeof(duelSlots));
    duelSlots[0].client = duelSlots[1].client = -1;
    duelSlotsReset = qfalse;
}

void CG_DuelScoresParsed(const score_t *rows, int count) {
    int slot = 0, i;
    if (duelSlotsReset || cg.warmup) CG_ClearDuelSlots();
    if (cg.intermissionStarted && count == 1 && rows[0].client != duelSlots[0].client && duelSlots[0].client >= 0)
        slot = 1;
    for (i = 0; i < count && slot + i < 2; ++i) duelSlots[slot + i] = rows[i];
}

const score_t *CG_DuelSlot(int slot) { return slot >= 0 && slot < 2 ? &duelSlots[slot] : NULL; }

static qboolean CG_DuelSlotValid(int slot) {
    int client = duelSlots[slot].client;
    return client >= 0 && client < MAX_CLIENTS && cgs.clientinfo[client].infoValid;
}

static int CG_DuelWeapon(int index) {
    static const int order[] = { WP_GAUNTLET, WP_MACHINEGUN, WP_SHOTGUN, WP_GRENADE_LAUNCHER, WP_ROCKET_LAUNCHER,
        WP_LIGHTNING, WP_RAILGUN, WP_PLASMAGUN, WP_BFG, WP_CHAINGUN, WP_NAILGUN, WP_PROX_LAUNCHER, WP_HEAVY_MACHINEGUN };
    return index >= 0 && index < (int)ARRAY_LEN(order) ? order[index] : WP_NONE;
}

static void CG_DuelPaint(float x, float y, int align, int font, float scale, vec4_t color, const char *text, int style) {
    int width = font < 0 ? CG_Text_Width(text, scale, 0) : CG_Text_WidthInFont(text, font, scale, 0);
    if (font >= 0) CG_SetTextFont(font);
    if (align == ITEM_ALIGN_CENTER) x -= width * 0.5f;
    else if (align == ITEM_ALIGN_RIGHT) x -= width;
    CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
}

static int CG_DuelScore(int slot) { return atoi(CG_ConfigString(slot ? CS_DUEL_SCORE2 : CS_DUEL_SCORE1)); }

const char *CG_DuelName(int slot, float scale) {
    static char text[48];
    char name[40], prefix[40];
    int length;
    Q_strncpyz(name, CG_ConfigString(slot ? CS_DUEL_NAME2 : CS_DUEL_NAME1), sizeof(name));
    if (CG_Text_Width(name, scale, 0) <= 140 || !name[0]) { Q_strncpyz(text, name, sizeof(text)); return text; }
    for (length = (int)strlen(name); length > 1; --length) {
        Q_strncpyz(prefix, name, length);
        if (CG_Text_Width(prefix, scale, 0) < 132) break;
    }
    Com_sprintf(text, sizeof(text), "%s...", prefix);
    return text;
}

static float CG_DuelAligned(float x, int align, const char *text, float scale) {
    int width = CG_Text_WidthInFont(text, 0, scale, 0);
    if (align == ITEM_ALIGN_CENTER) return x - width * 0.5f;
    if (align == ITEM_ALIGN_RIGHT) return x - width;
    return x;
}

static const char *CG_DuelWait(const score_t *row, int item) {
    static char text[24];
    if (!row->itemPickups[item]) return "-";
    snprintf(text, sizeof(text), "%3.2f", (double)row->itemWait[item]);   /* rounds as retail's CRT */
    return text;
}

/* 0x10036eb0 case 0: each armor/mega item with pickups draws its icon at
 * the cursor, then the count one row (15) lower, 15 right for slot one,
 * and the average wait 15 beyond it (left for slot two), each aligned by its
 * font 0 width. The next icon starts back at x (slot one) or 15 right of
 * the wait (slot two), on the count's row. */
void CG_DrawDuelPickupRows(int slot, float x, float y, float w, float h, int align, float scale, vec4_t color, int style) {
    static const char *const icons[4] = { "pickup_RA", "pickup_YA", "pickup_GA", "pickup_MH" };
    const score_t *row = &duelSlots[slot ? 1 : 0];
    char text[16];
    int i;
    CG_SetTextFont(0);
    for (i = 0; i < 4; ++i) {
        const char *wait;
        if (!row->itemPickups[i]) continue;
        CG_DrawPic(x, y, w, h, trap_R_RegisterShaderNoMip(icons[i]));
        if (!slot) x += 15;
        y += 15;
        Com_sprintf(text, sizeof(text), "%d", row->itemPickups[i]);
        x = CG_DuelAligned(x, align, text, scale);
        CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
        x += slot ? -15 : 15;
        wait = CG_DuelWait(row, i);
        x = CG_DuelAligned(x, align, wait, scale);
        CG_Text_Paint(x, y, scale, color, wait, 0, 0, style);
        x += slot ? 15 : -30;
    }
}

/* 0x10033040: the duel client's (CS_DUEL_CLIENT1/2) health over twice its
 * handicapped maximum fills an 8-unit bar 2 below the rect top, anchored
 * right for slot one and left for slot two, orange-yellow or (with
 * cg_specDuelHealthColor) the health tint. For two seconds after a hit the
 * lost health trails beyond it in red (half alpha with the tint option);
 * otherwise the shown value catches up. Armor fills a cyan 2-unit strip at
 * +10, scaled by the handicap. The followed player uses its own snapshot
 * stats and damage time. Dead players show no bars. */
static int duelShownHealth[2];

void CG_DrawDuelHealthArmor(int slot, float x, float y, float w) {
    static const vec4_t plain = { 1, 0.8f, 0.2f, 1 }, trailSolid = { 1, 0, 0, 1 }, trailSoft = { 1, 0, 0, 0.5f };
    static const vec4_t armorColor = { 0, 1, 1, 1 };
    const centity_t *cent;
    vec4_t tint;
    int client, health, armor, maximum, painTime;
    qboolean followed;
    float full, fraction, width;
    if (!cg.snap || !cg_specDuelHealthArmor.integer) return;
    client = atoi(CG_ConfigString(slot ? CS_DUEL_CLIENT2 : CS_DUEL_CLIENT1));
    if (client < 0 || client >= MAX_CLIENTS) return;
    cent = &cg_entities[client];
    if (!(cent->currentState.eType & 0xb)) return;
    followed = cent->currentState.clientNum == cg.snap->ps.clientNum;
    health = followed ? cg.snap->ps.stats[STAT_HEALTH] : cent->currentState.health;
    armor = followed ? cg.snap->ps.stats[STAT_ARMOR] : cent->currentState.armor;
    painTime = followed ? cg.damageTime : cent->pe.painTime;
    {
        double scaled = cgs.clientinfo[client].handicap / 100.0 * cgs.startingHealth;
        maximum = scaled >= 1 && scaled <= cgs.startingHealth ? (int)scaled : cgs.startingHealth;
    }
    if (cent->currentState.eFlags & EF_DEAD) return;
    full = (float)(maximum * 2);
    if (!(full > 0)) return;
    fraction = health / full;
    if (!(fraction >= 0)) fraction = 0;
    width = fraction * w;
    if (cg_specDuelHealthColor.integer) CG_OverheadHealthColor(cent->currentState.clientNum, health, tint);
    else Vector4Copy(plain, tint);
    CG_FillRect(slot ? x : x + (w - width), y + 2, width, 8, tint);
    if (cg.time - painTime < 2000) {
        float trail = (duelShownHealth[slot] - health) / full, trailWidth;
        if (!(trail >= 0)) trail = 0;
        if (!(trail <= fraction)) trail = fraction;
        trailWidth = trail * w;
        CG_FillRect(slot ? x + width : x + (w - width) - trailWidth, y + 2, trailWidth, 8,
                    cg_specDuelHealthColor.integer ? trailSoft : trailSolid);
    } else {
        duelShownHealth[slot] = health;
    }
    if (armor) {
        float armorFraction = armor / full, armorWidth;
        if (!(armorFraction >= 0)) armorFraction = 0;
        armorWidth = (float)(armorFraction * (cgs.clientinfo[client].handicap / 100.0) * w);
        CG_FillRect(slot ? x : x + (w - armorWidth), y + 10, armorWidth, 2, armorColor);
    }
    trap_R_SetColor(NULL);
}

qboolean CG_DrawDuelStat(int ownerDraw, float x, float y, float w, float h, int align, float scale,
                         vec4_t color, int style) {
    const score_t *row;
    char text[48];
    int slot, base, other, weapon;
    if (ownerDraw >= CG_2ND_PLYR && ownerDraw <= CG_2ND_PLYR + (CG_1ST_PLYR_HUMILIATION - CG_1ST_PLYR)) {
        slot = 1; base = ownerDraw - (CG_2ND_PLYR - CG_1ST_PLYR);
    } else if (ownerDraw >= CG_1ST_PLYR && ownerDraw <= CG_1ST_PLYR_HUMILIATION) {
        slot = 0; base = ownerDraw;
    } else {
        return qfalse;
    }
    row = &duelSlots[slot];
    other = !slot;
    if (base == CG_1ST_PLYR) {
        const char *name = CG_DuelName(slot, scale);
        CG_DuelPaint(x, y, align, -1, scale, color, (!name[0] && cg.intermissionStarted) ? "-" : name, style);
        return qtrue;
    }
    if (base == CG_1ST_PLYR_SCORE) {
        int score = CG_DuelScore(slot);
        if (score == SCORE_NOT_PRESENT || score == -999) Q_strncpyz(text, "-", sizeof(text));
        else Com_sprintf(text, sizeof(text), "%d", score);
        CG_DuelPaint(x, y, align, 0, scale, color, text, style);
        return qtrue;
    }
    if (base >= CG_1ST_PLYR_FRAGS_G && base <= CG_1ST_PLYR_ACC_HMG) {
        int value, offset;
        if (base <= CG_1ST_PLYR_FRAGS_HMG) offset = base - CG_1ST_PLYR_FRAGS_G;
        else if (base <= CG_1ST_PLYR_HITS_HMG) offset = base - CG_1ST_PLYR_HITS_MG + 1;
        else if (base <= CG_1ST_PLYR_SHOTS_HMG) offset = base - CG_1ST_PLYR_SHOTS_MG + 1;
        else if (base <= CG_1ST_PLYR_DMG_HMG) offset = base - CG_1ST_PLYR_DMG_G;
        else offset = base - CG_1ST_PLYR_ACC_MG + 1;
        weapon = CG_DuelWeapon(offset);
        if (base <= CG_1ST_PLYR_FRAGS_HMG) value = row->weaponStats[weapon].kills;
        else if (base <= CG_1ST_PLYR_HITS_HMG) value = row->weaponStats[weapon].hits;
        else if (base <= CG_1ST_PLYR_SHOTS_HMG) value = row->weaponStats[weapon].shots;
        else if (base <= CG_1ST_PLYR_DMG_HMG) value = row->weaponStats[weapon].damage;
        else {
            value = row->weaponStats[weapon].accuracy;
            if (value > duelSlots[other].weaponStats[weapon].accuracy) CG_DuelTint(color, 1, 1, 1, 0.8f);
            Com_sprintf(text, sizeof(text), "%d%%", value);
            CG_DuelPaint(x, y, align, -1, scale, color, text, style);
            return qtrue;
        }
        Com_sprintf(text, sizeof(text), "%d", value);
        CG_DuelPaint(x, y, align, -1, scale, color, text, style);
        return qtrue;
    }
    if (base == CG_1ST_PLYR_HEALTH_ARMOR) {
        CG_DrawDuelHealthArmor(slot, x, y, w);
        return qtrue;
    }
    if (!CG_DuelSlotValid(slot) && !cg.intermissionStarted) return qtrue;
    switch (base) {
    case CG_1ST_PLYR_READY: {
        /* 0x10037ba0/0x10037d60: the slot's banner over the item rect, then
         * the label at scale 0.16, white, font 0, shadow 3: slot one at
         * (x + 16, y + 16), slot two right-aligned to x + 164. */
        static const char *const banners[2][5] = {
            { "ui/assets/score/1st_plyr_leads.tga", "ui/assets/score/1st_plyr_tied.tga",
              "ui/assets/score/1st_plyr_trails.tga", "ui/assets/score/1st_plyr_ready.tga",
              "ui/assets/score/1st_plyr_notready.tga" },
            { "ui/assets/score/2nd_plyr_leads.tga", "ui/assets/score/2nd_plyr_tied.tga",
              "ui/assets/score/2nd_plyr_trails.tga", "ui/assets/score/2nd_plyr_ready.tga",
              "ui/assets/score/2nd_plyr_notready.tga" } };
        static const char *const labels[5] = { "LEADS", "TIED", "TRAILS", "READY", "NOT READY" };
        vec4_t white = { 1, 1, 1, 1 };
        int state;
        if (!cg.warmup && cg.snap && cg.snap->ps.pm_type != PM_INTERMISSION) {
            int mine = CG_DuelScore(slot), theirs = CG_DuelScore(other);
            state = mine > theirs ? 0 : mine == theirs ? 1 : 2;
        } else {
            state = cg.snap && row->client >= 0 && row->client < 32 &&
                (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << row->client)) ? 3 : 4;
        }
        CG_DrawPic(x, y, w, h, trap_R_RegisterShader(banners[slot][state]));
        CG_SetTextFont(0);
        CG_Text_Paint(slot ? x + 164 - CG_Text_WidthInFont(labels[state], 0, 0.16f, 0) : x + 16, y + 16, 0.16f,
                      white, labels[state], 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        return qtrue;
    }
    case CG_1ST_PLYR_FRAGS: Com_sprintf(text, sizeof(text), "%d", row->kills); break;
    case CG_1ST_PLYR_DEATHS: Com_sprintf(text, sizeof(text), "%d", row->deaths); break;
    case CG_1ST_PLYR_DMG: Com_sprintf(text, sizeof(text), "%d", row->damageGiven); break;
    case CG_1ST_PLYR_ACC: Com_sprintf(text, sizeof(text), "%d%%", row->accuracy); break;
    case CG_1ST_PLYR_WINS:
        Com_sprintf(text, sizeof(text), "%d/%d", row->client >= 0 && row->client < MAX_CLIENTS ? cgs.clientinfo[row->client].wins : 0,
                    row->client >= 0 && row->client < MAX_CLIENTS ? cgs.clientinfo[row->client].losses : 0);
        break;
    case CG_1ST_PLYR_PING:
        if (row->ping > 80) CG_DuelTint(color, 1, 0, 0, 0.8f);
        else if (row->ping > 40) CG_DuelTint(color, 1, 1, 0, 0.8f);
        else CG_DuelTint(color, 0, 1, 0, 0.8f);
        Com_sprintf(text, sizeof(text), "%d", row->ping);
        break;
    case CG_1ST_PLYR_EXCELLENT: Com_sprintf(text, sizeof(text), "%d", row->excellentCount); break;
    case CG_1ST_PLYR_IMPRESSIVE: Com_sprintf(text, sizeof(text), "%d", row->impressiveCount); break;
    case CG_1ST_PLYR_HUMILIATION: Com_sprintf(text, sizeof(text), "%d", row->gauntletCount); break;
    case CG_1ST_PLYR_PICKUPS:
        CG_DrawDuelPickupRows(slot, x, y, w, h, align, scale, color, style);
        return qtrue;
    case CG_1ST_PLYR_PICKUPS_RA: case CG_1ST_PLYR_PICKUPS_YA:
    case CG_1ST_PLYR_PICKUPS_GA: case CG_1ST_PLYR_PICKUPS_MH:
        Com_sprintf(text, sizeof(text), "%d", row->itemPickups[base - CG_1ST_PLYR_PICKUPS_RA]);
        break;
    case CG_1ST_PLYR_AVG_PICKUP_TIME_RA: case CG_1ST_PLYR_AVG_PICKUP_TIME_YA:
    case CG_1ST_PLYR_AVG_PICKUP_TIME_GA: case CG_1ST_PLYR_AVG_PICKUP_TIME_MH:
        Q_strncpyz(text, CG_DuelWait(row, base - CG_1ST_PLYR_AVG_PICKUP_TIME_RA), sizeof(text));
        break;
    case CG_1ST_PLYR_FLAG:
        CG_DrawPic(x, y, w, h, trap_R_RegisterShaderNoMip("ui/assets/flags/none.tga"));
        return qtrue;
    case CG_1ST_PLYR_AVATAR:
        if (row->client >= 0 && row->client < MAX_CLIENTS && cgs.clientinfo[row->client].modelIcon)
            CG_DrawPic(x, y, w, h, cgs.clientinfo[row->client].modelIcon);
        return qtrue;
    default:
        return qfalse;
    }
    CG_DuelPaint(x, y, align, 0, scale, color, text, style);
    return qtrue;
}
