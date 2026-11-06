/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* CG_SPEC_MESSAGES (retail 0x10034d70): round notice, spectator mode text
 * and the join hints. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_drawSpecMessages;
static snapshot_t snapshot;
static menuDef_t followMenu;
static qboolean haveFollowMenu;
static int texts, placement, placements[8], styles[8];
static float textX[8], textY[8], scales[8], alphas[8];
static char textValue[8][64];

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
menuDef_t *Menus_FindByName(const char *name) {
    return haveFollowMenu && !strcmp(name, "comp_specfollowhud_menu") ? &followMenu : NULL;
}
void Menu_Paint(menuDef_t *menu, qboolean forcePaint) { (void)menu; (void)forcePaint; }
void CG_DrawChatArea(const rectDef_t *rect, float scale) { (void)rect; (void)scale; }
const float *CG_OverheadTeamColor(int team) { (void)team; return colorWhite; }
int CG_SetScreenPlacement(int value) { int old = placement; placement = value; return old; }
void CG_SetTextFont(int font) { CHECK(font == 0); }
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 7; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)adjust; (void)limit;
    textX[texts] = x; textY[texts] = y; scales[texts] = scale; alphas[texts] = color[3];
    placements[texts] = placement; styles[texts] = style;
    snprintf(textValue[texts++], 64, "%s", text);
}

int main(void) {
    rectDef_t rect = { 370, 18, 50, 12 };
    cg.snap = &snapshot; cg_drawSpecMessages.integer = 1;
    /* Clan Arena: a dead team player sees only the centred round notice. */
    cgs.gametype = GT_CLAN_ARENA; snapshot.ps.pm_type = PM_SPECTATOR; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    CG_DrawSpecMessages(&rect);
    CHECK(texts == 1 && !strcmp(textValue[0], "Round In Progress") && textX[0] == 320 - 119 / 2 && textY[0] == 60);
    CHECK(scales[0] == 0.35f && placements[0] == CG_PLACE_CENTER && placement == CG_PLACE_DEFAULT);
    CHECK(styles[0] == ITEM_TEXTSTYLE_SHADOWED);
    /* Team spectator in a team game: mode text, hint and join hints. */
    texts = 0; cgs.gametype = GT_TEAM; snapshot.ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
    snapshot.ps.eFlags = EF_SPECTATOR;
    CG_DrawSpecMessages(&rect);
    CHECK(texts == 4 && !strcmp(textValue[0], "SPECTATOR MODE") && textX[0] == 370 && textY[0] == 18);
    CHECK(scales[0] == 0.22f && styles[0] == ITEM_TEXTSTYLE_NORMAL && placements[0] == CG_PLACE_DEFAULT);
    CHECK(!strcmp(textValue[1], "Press mouse button 1 to cycle through players") && textY[1] == 30 && alphas[1] == 0.7f);
    CHECK(!strcmp(textValue[2], "press ESC and use the JOIN buttons") && textX[2] == 20 && textY[2] == 453);
    CHECK(!strcmp(textValue[3], "to enter the game") && textY[3] == 470 && placements[3] == CG_PLACE_LEFT);
    CHECK(scales[3] == 0.28f && styles[3] == ITEM_TEXTSTYLE_SHADOWED && placement == CG_PLACE_DEFAULT);
    /* Following with the follow HUD loaded drops the join hints. */
    texts = 0; haveFollowMenu = qtrue; snapshot.ps.pm_flags = PMF_FOLLOW;
    CG_DrawSpecMessages(&rect); CHECK(texts == 2);
    haveFollowMenu = qfalse; texts = 0; CG_DrawSpecMessages(&rect); CHECK(texts == 4);
    snapshot.ps.pm_flags = 0;
    /* Duel: "waiting to play" follows client 0's spectator-only flag. */
    texts = 0; cgs.gametype = GT_TOURNAMENT; CG_DrawSpecMessages(&rect);
    CHECK(texts == 3 && !strcmp(textValue[2], "waiting to play") && textY[2] == 461);
    texts = 0; cgs.clientinfo[0].spectatorOnly = 1; CG_DrawSpecMessages(&rect); CHECK(texts == 2);
    /* FFA: no join hints. No eFlags or the cvar off: nothing. */
    texts = 0; cgs.gametype = GT_FFA; CG_DrawSpecMessages(&rect); CHECK(texts == 2);
    texts = 0; snapshot.ps.eFlags = 0; CG_DrawSpecMessages(&rect); CHECK(texts == 0);
    snapshot.ps.eFlags = EF_SPECTATOR; cg_drawSpecMessages.integer = 0; CG_DrawSpecMessages(&rect); CHECK(texts == 0);
    puts("spectator messages: round notice, mode text and join hints OK");
    return 0;
}
