/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail spectator HUD. Observed:
 *
 * - CG_Draw2D (0x10010fd9) takes the spectator branch while the predicted
 *   state carries EF_SPECTATOR or PM_SPECTATOR, also while following.
 * - 0x1000dad0 paints comp_specfollowhud_menu while following, otherwise
 *   comp_spechud_menu (neither while the scoreboard is up), then always the
 *   four comp_spec{red,blue}_{top,bottom} panels (their menu flags pick the
 *   leading team), then the chat area at (5, 287, 634, 120). Retail looks
 *   the menus up once after loading (0x10006eb0); FnQL-game loads the two
 *   comp_spectator files and looks them up by name.
 * - CG_FOLLOW_PLAYER_NAME (94) and _EX (95) share 0x10033bf0: the followed
 *   client's name, prefixed "Following - " for 94, cut to 79 characters,
 *   in the item font and style, in its team color in team modes, shifted
 *   left by half or all of its width for center or right alignment. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

void CG_DrawChatArea(const rectDef_t *rect, float scale);

qboolean CG_SpectatorHudActive(void) {
    return (cg.predictedPlayerState.eFlags & EF_SPECTATOR) || cg.predictedPlayerState.pm_type == PM_SPECTATOR;
}

static void CG_PaintNamedMenu(const char *name) {
    menuDef_t *menu = Menus_FindByName(name);
    if (menu) Menu_Paint(menu, qtrue);
}

void CG_DrawSpectatorHud(void) {
    static const char *const panels[] = { "comp_specred_top", "comp_specred_bottom",
                                          "comp_specblue_top", "comp_specblue_bottom" };
    rectDef_t chat;
    int i;
    if (!cg.scoreBoardShowing) {
        menuDef_t *menu = NULL;
        if (cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW)) menu = Menus_FindByName("comp_specfollowhud_menu");
        if (!menu) menu = Menus_FindByName("comp_spechud_menu");
        if (menu) Menu_Paint(menu, qtrue);
    }
    for (i = 0; i < (int)ARRAY_LEN(panels); ++i) CG_PaintNamedMenu(panels[i]);
    if (cg.scoreBoardShowing) return;
    chat.x = 5; chat.y = 287; chat.w = 634; chat.h = 120;
    CG_DrawChatArea(&chat, 1.0f);
}

/* CG_SPEC_MESSAGES (35), retail 0x10034d70; placed by ui/spectator.menu
 * and spectator_follow.menu at the item rect, ignoring its style. With
 * cg_drawSpecMessages set:
 * - Clan Arena and Attack & Defend: a PM_SPECTATOR player still on a team
 *   sees "Round In Progress" centred at y 60 (font 0, 0.35, shadowed) and
 *   nothing else.
 * - Otherwise, with any predicted-state eFlags: "SPECTATOR MODE" (0.22,
 *   white) at the rect and the mouse-button hint (0.18, 0.73 grey at 0.7)
 *   12 below, both font 0 and unstyled. Retail looks up the +moveup key
 *   only after drawing and never uses it.
 * - Unless the follow HUD menu exists and the view follows a player, the
 *   join hints draw shadowed at 0.28, left-placed: in duel "waiting to
 *   play" at (20, 461) while client 0's "so" is clear (retail reads client
 *   0, not the local client), in team games "press ESC and use the JOIN
 *   buttons" (20, 453) and "to enter the game" (20, 470).
 * Retail also skips the second branch while its stats/intermission menu
 * flag (0x10aa6934) is set; FnQL-game has no such flag (inferred
 * equivalent: those menus replace the spectator HUD). */
void CG_DrawSpecMessages(const rectDef_t *rect) {
    static vec4_t white = { 1, 1, 1, 1 }, hint = { 0.73f, 0.73f, 0.73f, 0.7f };
    static const char round[] = "Round In Progress";
    const playerState_t *ps;
    int previous;
    if (!cg_drawSpecMessages.integer || !cg.snap || !rect) return;
    ps = &cg.snap->ps;
    CG_SetTextFont(0);
    if ((cgs.gametype == GT_CLAN_ARENA || cgs.gametype == GT_ATTACK_DEFEND) &&
        ps->pm_type == PM_SPECTATOR && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR) {
        const int width = CG_Text_Width(round, 0.35f, 0);
        previous = CG_SetScreenPlacement(CG_PLACE_CENTER);
        CG_Text_Paint((float)(320 - width / 2), 60, 0.35f, white, round, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        CG_SetScreenPlacement(previous);
        return;
    }
    if (!ps->eFlags) return;
    CG_Text_Paint(rect->x, rect->y, 0.22f, white, "SPECTATOR MODE", 0, 0, ITEM_TEXTSTYLE_NORMAL);
    CG_Text_Paint(rect->x, (float)(rect->y + 12.0), 0.18f, hint, "Press mouse button 1 to cycle through players", 0, 0,
                  ITEM_TEXTSTYLE_NORMAL);
    if (Menus_FindByName("comp_specfollowhud_menu") && (ps->pm_flags & PMF_FOLLOW)) return;
    previous = CG_SetScreenPlacement(CG_PLACE_LEFT);
    if (cgs.gametype == GT_TOURNAMENT) {
        if (!cgs.clientinfo[0].spectatorOnly)
            CG_Text_Paint(20, 461, 0.28f, white, "waiting to play", 0, 0, ITEM_TEXTSTYLE_SHADOWED);
    } else if (cgs.gametype >= GT_TEAM) {
        CG_Text_Paint(20, 453, 0.28f, white, "press ESC and use the JOIN buttons", 0, 0, ITEM_TEXTSTYLE_SHADOWED);
        CG_Text_Paint(20, 470, 0.28f, white, "to enter the game", 0, 0, ITEM_TEXTSTYLE_SHADOWED);
    }
    CG_SetScreenPlacement(previous);
}

void CG_DrawFollowPlayerName(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style) {
    char text[80];
    const float *ink = color;
    const clientInfo_t *info;
    int client;
    if (!cg.snap) return;
    client = cg.snap->ps.clientNum;
    if (client < 0 || client >= MAX_CLIENTS) return;
    info = &cgs.clientinfo[client];
    if (cgs.gametype >= GT_TEAM) ink = CG_OverheadTeamColor(info->team);
    Com_sprintf(text, sizeof(text), "%s%s", ownerDraw == CG_FOLLOW_PLAYER_NAME ? "Following - " : "", info->name);
    if (align == ITEM_ALIGN_CENTER) x -= CG_Text_Width(text, scale, 0) * 0.5f;
    else if (align == ITEM_ALIGN_RIGHT) x -= CG_Text_Width(text, scale, 0);
    {
        vec4_t paint;
        Vector4Copy(ink, paint);
        if (ink != color) paint[3] = 1;
        CG_Text_Paint(x, y, scale, paint, text, 0, 0, style);
    }
}
