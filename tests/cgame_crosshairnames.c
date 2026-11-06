/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail crosshair names and teammate health (0x1000c670). */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_teammateNames, cg_teammateCrosshairNames, cg_teammateCrosshairNamesOpacity;
vmCvar_t cg_enemyCrosshairNames, cg_enemyCrosshairNamesOpacity;
vmCvar_t cg_drawCrosshairTeamHealth, cg_drawCrosshairTeamHealthSize;
static snapshot_t snapshot;
static int paints;
static struct { float x, y, scale; vec4_t color; char text[64]; int style; } painted[8];

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
int CG_Text_Width(const char *text, float scale, int limit) {
    CHECK(!limit); return (int)(strlen(text) * 100 * scale);
}
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    CHECK(paints < 8 && !adjust && !limit);
    painted[paints].x = x; painted[paints].y = y; painted[paints].scale = scale;
    memcpy(painted[paints].color, color, sizeof(vec4_t));
    snprintf(painted[paints].text, sizeof(painted[paints].text), "%s", text);
    painted[paints++].style = style;
}

static int Draw(void) {
    vec4_t color = { 1, 1, 1, 0.8f };
    paints = 0;
    CG_DrawCrosshairNameText(color);
    return paints;
}

int main(void) {
    cg.snap = &snapshot; cg.crosshairClientNum = 3;
    strcpy(cgs.clientinfo[3].name, "Target");          /* width 0.26*600 = 156 */
    cgs.clientinfo[3].team = TEAM_BLUE; cgs.clientinfo[3].health = 50; cgs.clientinfo[3].armor = 150;
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; snapshot.ps.stats[STAT_MAX_HEALTH] = 100;
    cg_enemyCrosshairNames.integer = 1; cg_enemyCrosshairNamesOpacity.value = .75f;
    cg_teammateCrosshairNames.integer = 0; cg_teammateCrosshairNamesOpacity.value = .5f;
    cg_teammateNames.integer = 1; cg_drawCrosshairTeamHealth.value = 2; cg_drawCrosshairTeamHealthSize.value = .12f;

    /* Enemy (team mode, other team): centered at y 190, scale 0.26, shadowed. */
    cgs.gametype = GT_TEAM;
    CHECK(Draw() == 1 && !strcmp(painted[0].text, "Target"));
    CHECK(painted[0].x == 320 - 78 && painted[0].y == 190 && painted[0].scale == .26f);
    CHECK(painted[0].style == ITEM_TEXTSTYLE_SHADOWED && painted[0].color[3] == .8f * .75f);
    cg_enemyCrosshairNames.integer = 0; CHECK(Draw() == 0);
    cg_enemyCrosshairNames.integer = 1;
    /* Individual modes treat everyone as an enemy, even the same team field. */
    cgs.gametype = GT_FFA; cgs.clientinfo[3].team = TEAM_RED; CHECK(Draw() == 1 && painted[0].color[3] == .8f * .75f);

    /* Teammates: only with cg_teammateCrosshairNames and without cg_teammateNames. */
    cgs.gametype = GT_CTF; CHECK(Draw() == 0);
    cg_teammateNames.integer = 0; CHECK(Draw() == 0);
    cg_teammateCrosshairNames.integer = 1; cg_enemyCrosshairNames.integer = 0;
    CHECK(Draw() == 4 && painted[0].color[3] == .8f * .5f);
    /* Health line: "50 " right-aligned at 320, "/" at 320, " 150" at 325,
     * y = 198 + 16 * size, tinted in mode 2. */
    CHECK(!strcmp(painted[1].text, "50 ") && painted[1].scale == .12f);
    CHECK(painted[1].x == 320 - (int)(3 * 100 * .12f) && painted[1].y == .12f * 16 + 198);
    CHECK(painted[1].color[0] == 1 && painted[1].color[1] == .5f && painted[1].color[2] == 0);
    CHECK(!strcmp(painted[2].text, "/") && painted[2].x == 320 && painted[2].color[1] == 1 && painted[2].color[2] == 1);
    CHECK(!strcmp(painted[3].text, " 150") && painted[3].x == 325);
    CHECK(painted[3].color[1] == 1 && painted[3].color[2] == .75f);
    CHECK(painted[1].color[3] == .8f * .5f && painted[3].style == ITEM_TEXTSTYLE_SHADOWED);
    /* Mode 1 is white; mode 0 omits the line. */
    cg_drawCrosshairTeamHealth.value = 1;
    CHECK(Draw() == 4 && painted[1].color[1] == 1 && painted[1].color[2] == 1 && painted[3].color[2] == 1);
    cg_drawCrosshairTeamHealth.value = 0; CHECK(Draw() == 1);
    cg_drawCrosshairTeamHealth.value = 2;
    /* Tint edges: zero maximum, high armor. */
    snapshot.ps.stats[STAT_MAX_HEALTH] = 0; CHECK(Draw() == 4 && painted[1].color[1] == 1 && painted[1].color[2] == 1);
    snapshot.ps.stats[STAT_MAX_HEALTH] = 100; cgs.clientinfo[3].armor = 400;
    CHECK(Draw() == 4 && painted[3].color[2] == 1);

    /* Invalid input draws nothing. */
    cg.crosshairClientNum = -1; CHECK(Draw() == 0);
    cg.crosshairClientNum = MAX_CLIENTS; CHECK(Draw() == 0);
    cg.crosshairClientNum = 3; cg.snap = NULL; CHECK(Draw() == 0);
    paints = 0; cg.snap = &snapshot; CG_DrawCrosshairNameText(NULL); CHECK(paints == 0);

    puts("Crosshair names: enemy/teammate gates, opacity, placement and teammate health OK");
    return 0;
}
