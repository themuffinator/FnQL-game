/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Team end-of-match statistics (ownerdraws 287-336). Observed:
 *
 * - The scores_tdm/scores_ctf team headers (retail 0x10aa6864 red,
 *   0x10aa68a8 blue) hold, in order: red, yellow, green armor, mega,
 *   quad, battle suit, regen, haste, invis, flag, medkit pickups, then the
 *   quad, battle suit, regen, haste, invis and flag held seconds; FnQL-game
 *   parses them into cg.scoreTeamStats in the same order.
 * - Single pickup counts (0x10037f80 cases 1-11 and 13-23) draw "%d" in
 *   the item font, aligned by its width.
 * - Held times (0x10039ae0) draw "m:ss" (0x10057470), or "-" when zero,
 *   in font 0.
 * - Average ping (0x10039e70): the team's score rows averaged, "(%d)",
 *   green to 40, yellow to 80, red beyond, alpha 0.8; font 0.
 * - Player count (0x100333c0): clients on the team; TDM, CA, CTF, One
 *   Flag, Harvester, Domination and A&D show "(%d)" or "(%d/%d)" with
 *   teamsize; other modes "%d Player(s)", or "%d/%d Players" when teamsize
 *   fits twice in sv_maxclients.
 * - Timeouts (0x10033530): "TO: %d" from CS_RED/BLUE_TIMEOUTS_LEFT, empty
 *   meaning serverinfo g_timeoutCount (0x100495f5). The dispatcher
 *   (0x1003b99d/0x1003b9d4) draws nothing unless g_timeoutCount is above
 *   zero, so servers without timeouts show no count.
 * The map pickup strips (292/319) are CG_DrawTeamPickupStrip. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

enum { TS_RA, TS_YA, TS_GA, TS_MH, TS_QUAD, TS_BS, TS_REGEN, TS_HASTE, TS_INVIS, TS_FLAG, TS_MEDKIT,
       TS_QUAD_TIME, TS_BS_TIME, TS_REGEN_TIME, TS_HASTE_TIME, TS_INVIS_TIME, TS_FLAG_TIME };

static void CG_TeamStatPaint(float x, float y, int align, int font, float scale, vec4_t color, const char *text, int style) {
    int width = font < 0 ? CG_Text_Width(text, scale, 0) : CG_Text_WidthInFont(text, font, scale, 0);
    if (font >= 0) CG_SetTextFont(font);
    if (align == ITEM_ALIGN_CENTER) x -= width * 0.5f;
    else if (align == ITEM_ALIGN_RIGHT) x -= width;
    CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
}

const char *CG_TeamHeldTime(int seconds) {
    static char text[24];
    if (!seconds) return "-";
    Com_sprintf(text, sizeof(text), "%i:%i%i", seconds / 60, seconds % 60 / 10, seconds % 60 % 10);
    return text;
}

int CG_TeamAveragePing(int team) {
    int i, total = 0, count = 0;
    for (i = 0; i < cg.numScores; ++i)
        if (cg.scores[i].team == team) { total += cg.scores[i].ping; ++count; }
    return count > 0 ? total / count : 0;
}

const char *CG_TeamPlayerCountText(int team) {
    static char text[32];
    int i, count = 0;
    for (i = 0; i < cgs.maxclients && i < MAX_CLIENTS; ++i)
        if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == team) ++count;
    switch (cgs.gametype) {
    case GT_TEAM: case GT_CLAN_ARENA: case GT_CTF: case GT_1FCTF: case GT_HARVESTER:
    case GT_DOMINATION: case GT_ATTACK_DEFEND:
        if (cgs.teamSize <= 0) Com_sprintf(text, sizeof(text), "(%d)", count);
        else Com_sprintf(text, sizeof(text), "(%d/%d)", count, cgs.teamSize);
        break;
    default:
        if (cgs.teamSize <= 0 || cgs.teamSize * 2 > cgs.maxclients)
            Com_sprintf(text, sizeof(text), "%d Player%s", count, count == 1 ? "" : "s");
        else Com_sprintf(text, sizeof(text), "%d/%d Players", count, cgs.teamSize);
        break;
    }
    return text;
}

int CG_TeamTimeoutsLeft(int team) {
    const char *value = CG_ConfigString(team == TEAM_BLUE ? CS_BLUE_TIMEOUTS_LEFT : CS_RED_TIMEOUTS_LEFT);
    if (!value[0]) value = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "g_timeoutCount");
    return atoi(value);
}

/* Retail 0x10037f80 cases 0/12: the team's pickup strip from the rect's
 * truncated origin. Armor, mega and medkit icons (rect size) carry their
 * count at (+14, +15) (+11 for yellow armor) and advance 25; the flag and
 * powerup icons sit 7 units higher with the count at (+14, +8) and the held
 * time ("m:ss" or "-") at (+2, +15), advancing 28. Items without pickups
 * are skipped. The flag is the opponent's, or the neutral one in One Flag.
 * Text uses the item font. */
void CG_DrawTeamPickupStrip(int team, float rx, float ry, float w, float h, float scale, vec4_t color, int style) {
    static const struct { int stat; const char *shader; float textX; } armor[] = {
        { TS_RA, "pickup_RA", 14 }, { TS_YA, "pickup_YA", 11 }, { TS_GA, "pickup_GA", 14 },
        { TS_MH, "pickup_MH", 14 }, { TS_MEDKIT, "pickup_MK", 14 } };
    static const struct { int stat, time; const char *shader; } powers[] = {
        { TS_FLAG, TS_FLAG_TIME, NULL }, { TS_QUAD, TS_QUAD_TIME, "pickup_QUAD" }, { TS_BS, TS_BS_TIME, "pickup_BS" },
        { TS_REGEN, TS_REGEN_TIME, "pickup_REGEN" }, { TS_HASTE, TS_HASTE_TIME, "pickup_HASTE" },
        { TS_INVIS, TS_INVIS_TIME, "pickup_INVIS" } };
    const int *stats = cg.scoreTeamStats[team == TEAM_BLUE ? 1 : 0];
    char text[24];
    int i, x, y;
    if (!(rx > -1e6f && rx < 1e6f && ry > -1e6f && ry < 1e6f)) return;
    x = (int)rx; y = (int)ry;
    for (i = 0; i < (int)ARRAY_LEN(armor); ++i) {
        if (stats[armor[i].stat] <= 0) continue;
        CG_DrawPic((float)x, (float)y, w, h, trap_R_RegisterShaderNoMip(armor[i].shader));
        Com_sprintf(text, sizeof(text), "%d", stats[armor[i].stat]);
        CG_Text_Paint(x + armor[i].textX, (float)(y + 15), scale, color, text, 0, 0, style);
        x += 25;
    }
    for (i = 0; i < (int)ARRAY_LEN(powers); ++i) {
        const char *shader = powers[i].shader;
        if (stats[powers[i].stat] <= 0) continue;
        if (!shader) shader = cgs.gametype == GT_1FCTF ? "pickup_NTRLFLAG" : team == TEAM_BLUE ? "pickup_REDFLAG" : "pickup_BLUEFLAG";
        CG_DrawPic((float)x, (float)(y - 7), w, h, trap_R_RegisterShaderNoMip(shader));
        Com_sprintf(text, sizeof(text), "%d", stats[powers[i].stat]);
        CG_Text_Paint((float)(x + 14), (float)(y + 8), scale, color, text, 0, 0, style);
        CG_Text_Paint((float)(x + 2), (float)(y + 15), scale, color, CG_TeamHeldTime(stats[powers[i].time]), 0, 0, style);
        x += 28;
    }
}

qboolean CG_DrawTeamStat(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style) {
    static const int counts[] = { TS_RA, TS_YA, TS_GA, TS_MH, TS_QUAD, TS_BS, -1, -1, TS_FLAG, TS_MEDKIT,
                                  TS_REGEN, TS_HASTE, TS_INVIS };
    static const int times[] = { TS_QUAD_TIME, TS_BS_TIME, -1, -1, -1, -1, -1, TS_FLAG_TIME, TS_REGEN_TIME,
                                 TS_HASTE_TIME, TS_INVIS_TIME };
    char text[32];
    int team, offset, stat;
    if (ownerDraw >= CG_RED_AVG_PING && ownerDraw <= CG_RED_TEAM_TIMEHELD_INVIS) {
        team = TEAM_RED; offset = ownerDraw - CG_RED_AVG_PING;
    } else if (ownerDraw >= CG_BLUE_AVG_PING && ownerDraw <= CG_BLUE_TEAM_TIMEHELD_INVIS) {
        team = TEAM_BLUE; offset = ownerDraw - CG_BLUE_AVG_PING;
    } else {
        return qfalse;
    }
    ownerDraw = CG_RED_AVG_PING + offset;   /* fold blue onto the red ids */
    switch (ownerDraw) {
    case CG_RED_AVG_PING: {
        int ping = CG_TeamAveragePing(team);
        vec4_t tint = { 0, 1, 0, 0.8f };
        if (ping > 40) { tint[0] = 1; tint[1] = ping > 80 ? 0 : 1; }
        Vector4Copy(tint, color);
        Com_sprintf(text, sizeof(text), "(%d)", ping);
        CG_TeamStatPaint(x, y, align, 0, scale, color, text, style);
        return qtrue;
    }
    case CG_RED_PLAYER_COUNT:
        CG_TeamStatPaint(x, y, align, 0, scale, color, CG_TeamPlayerCountText(team), style);
        return qtrue;
    case CG_RED_TIMEOUT_COUNT:
        if (cgs.timeoutCount <= 0) return qtrue;
        Com_sprintf(text, sizeof(text), "TO: %d", CG_TeamTimeoutsLeft(team));
        CG_TeamStatPaint(x, y, align, 0, scale, color, text, style);
        return qtrue;
    default:
        break;
    }
    if (ownerDraw >= CG_RED_TEAM_PICKUPS_RA && ownerDraw <= CG_RED_TEAM_PICKUPS_INVIS) {
        stat = counts[ownerDraw - CG_RED_TEAM_PICKUPS_RA];
        if (stat < 0) {
            stat = times[ownerDraw - CG_RED_TEAM_TIMEHELD_QUAD];
            CG_TeamStatPaint(x, y, align, 0, scale, color, CG_TeamHeldTime(cg.scoreTeamStats[team - TEAM_RED][stat]), style);
            return qtrue;
        }
        Com_sprintf(text, sizeof(text), "%d", cg.scoreTeamStats[team - TEAM_RED][stat]);
        CG_TeamStatPaint(x, y, align, -1, scale, color, text, style);
        return qtrue;
    }
    if (ownerDraw >= CG_RED_TEAM_TIMEHELD_FLAG && ownerDraw <= CG_RED_TEAM_TIMEHELD_INVIS) {
        stat = times[ownerDraw - CG_RED_TEAM_TIMEHELD_QUAD];
        CG_TeamStatPaint(x, y, align, 0, scale, color, CG_TeamHeldTime(cg.scoreTeamStats[team - TEAM_RED][stat]), style);
        return qtrue;
    }
    return qfalse;   /* map pickup strip and base/flag ownerdraws stay with the dispatcher */
}
