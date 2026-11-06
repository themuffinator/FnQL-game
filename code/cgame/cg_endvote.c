/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* End-of-match map vote ownerdraws (ui/endgamevote.menu). Observed:
 *
 * - CS_ROTATION_TITLES (0x2c1) holds "map_N", "title_N" and "gt_N" for the
 *   three choices; CS_ROTATION_VOTES (0x2c2) holds their counts as "0".."2".
 * - CG_VOTEGAMETYPE1..3 (0x100356f0): gt_N eight units above the item, font
 *   0. CG_VOTEMAP1..3 (0x100355d0): map_N. CG_VOTENAME1..3 (0x10035660):
 *   title_N. These shift left by half or all of their font 0 width for
 *   center or right alignment (0x1002e050).
 * - CG_VOTESHOT1..3 (0x10035790): levelshots/preview/<map_N>, or
 *   levelshots/preview/default for an empty map.
 * - CG_VOTECOUNT1..3 (0x10035820): "Votes: <count>" in the item font.
 * - CG_VOTETIMER (0x10035920): "Voting ends in %i second(s)." for the 20
 *   seconds after CS_VOTE_TIME, then "Voting has ended."; font 0. */

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <stdint.h>

static float CG_VoteAligned(float x, const char *text, int align, float scale) {
    if (align == ITEM_ALIGN_CENTER) return x - CG_Text_WidthInFont(text, 0, scale, 0) * 0.5f;
    if (align == ITEM_ALIGN_RIGHT) return x - CG_Text_WidthInFont(text, 0, scale, 0);
    return x;
}

const char *CG_VoteChoice(const char *key, int choice) {
    char name[16];
    if (choice < 0 || choice > 2) return "";
    Com_sprintf(name, sizeof(name), "%s_%i", key, choice);
    return Info_ValueForKey(CG_ConfigString(CS_ROTATION_TITLES), name);
}

const char *CG_VoteTimerText(void) {
    static char text[48];
    int seconds = (int)(((int64_t)cgs.voteTime - cg.time + 20000) / 1000);
    if (seconds < 1) return "Voting has ended.";
    Com_sprintf(text, sizeof(text), seconds == 1 ? "Voting ends in %i second." : "Voting ends in %i seconds.", seconds);
    return text;
}

qboolean CG_DrawEndVote(int ownerDraw, float x, float y, float w, float h, int align, float scale,
                        vec4_t color, int style) {
    const char *text;
    int choice;
    switch (ownerDraw) {
    case CG_VOTEGAMETYPE1: case CG_VOTEGAMETYPE2: case CG_VOTEGAMETYPE3:
        text = CG_VoteChoice("gt", ownerDraw - CG_VOTEGAMETYPE1);
        CG_SetTextFont(0);
        CG_Text_Paint(CG_VoteAligned(x, text, align, scale), y - 8, scale, color, text, 0, 0, style);
        return qtrue;
    case CG_VOTEMAP1: case CG_VOTEMAP2: case CG_VOTEMAP3:
    case CG_VOTENAME1: case CG_VOTENAME2: case CG_VOTENAME3:
        choice = ownerDraw >= CG_VOTENAME1 ? ownerDraw - CG_VOTENAME1 : ownerDraw - CG_VOTEMAP1;
        text = CG_VoteChoice(ownerDraw >= CG_VOTENAME1 ? "title" : "map", choice);
        CG_SetTextFont(0);
        CG_Text_Paint(CG_VoteAligned(x, text, align, scale), y, scale, color, text, 0, 0, style);
        return qtrue;
    case CG_VOTESHOT1: case CG_VOTESHOT2: case CG_VOTESHOT3: {
        char path[MAX_QPATH];
        text = CG_VoteChoice("map", ownerDraw - CG_VOTESHOT1);
        Com_sprintf(path, sizeof(path), "levelshots/preview/%s", text[0] ? text : "default");
        CG_DrawPic(x, y, w, h, trap_R_RegisterShaderNoMip(path));
        return qtrue;
    }
    case CG_VOTECOUNT1: case CG_VOTECOUNT2: case CG_VOTECOUNT3: {
        char key[4], line[64];
        Com_sprintf(key, sizeof(key), "%i", ownerDraw - CG_VOTECOUNT1);
        Com_sprintf(line, sizeof(line), "Votes: %s", Info_ValueForKey(CG_ConfigString(CS_ROTATION_VOTES), key));
        text = line;
        if (align == ITEM_ALIGN_CENTER) x -= CG_Text_Width(text, scale, 0) * 0.5f;
        else if (align == ITEM_ALIGN_RIGHT) x -= CG_Text_Width(text, scale, 0);
        CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
        return qtrue;
    }
    case CG_VOTETIMER:
        text = CG_VoteTimerText();
        CG_SetTextFont(0);
        CG_Text_Paint(CG_VoteAligned(x, text, align, scale), y, scale, color, text, 0, 0, style);
        return qtrue;
    default:
        return qfalse;
    }
}
