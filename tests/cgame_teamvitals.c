/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Spectator team vitals (retail 0x1000fab0): row placement, icons, name
 * fitting and the default and health-colour bars. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
vmCvar_t cg_specTeamVitals, cg_specTeamVitalsHealthColor, cg_specTeamVitalsWidth, cg_specTeamVitalsY;
static snapshot_t snapshot;
static gitem_t quad = { "item_quad", NULL, { NULL }, "icons/quad", "Quad Damage", 30, IT_POWERUP, PW_QUAD };

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const gitem_t *CG_PowerupItem(int powerup) { return powerup == PW_QUAD ? &quad : NULL; }
void CG_OverheadHealthColor(int client, int value, vec4_t color) { color[0] = (float)client; color[1] = (float)value; color[2] = 7; color[3] = 1; }
const float *CG_OverheadTeamColor(int team) { static vec4_t c = { 9, 9, 9, 1 }; c[0] = (float)team; return c; }

static int placement, font, fills, pics, texts;
static float fill[32][8], pic[32][5], text[32][5];
static char textValue[32][48];
static int placements[32];
int CG_SetScreenPlacement(int value) { int old = placement; placement = value; return old; }
void CG_SetTextFont(int value) { font = value; }
int CG_Text_WidthInFont(const char *s, int f, float scale, int limit) {
    int n = (int)strlen(s);
    (void)f; (void)scale;
    return 6 * (limit > 0 && limit < n ? limit : n);
}
int CG_Text_HeightInFont(const char *s, int f, float scale, int limit) { (void)s; (void)f; (void)scale; (void)limit; return 9; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *s, float adjust, int limit, int style) {
    (void)adjust;
    CHECK(style == ITEM_TEXTSTYLE_SHADOWED);
    text[texts][0] = x; text[texts][1] = y; text[texts][2] = scale; text[texts][3] = (float)font; text[texts][4] = (float)limit;
    placements[texts] = placement;
    snprintf(textValue[texts++], 48, "%s|%g", s, color[3]);
}
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    pic[pics][0] = x; pic[pics][1] = y; pic[pics][2] = w; pic[pics][3] = h; pic[pics++][4] = (float)shader;
}
void CG_FillRect(float x, float y, float w, float h, const float *c) {
    float *f = fill[fills++];
    f[0] = x; f[1] = y; f[2] = w; f[3] = h; f[4] = c[0]; f[5] = c[1]; f[6] = c[2]; f[7] = c[3];
}
qhandle_t trap_R_RegisterShader(const char *name) { return !strcmp(name, "icons/quad") ? 44 : 1; }

static void Player(int n, int team, const char *name, int health, int armor, int weapon) {
    cg_entities[n].currentState.eType = ET_PLAYER; cg_entities[n].currentState.clientNum = n;
    cg_entities[n].currentState.health = health; cg_entities[n].currentState.armor = armor;
    cg_entities[n].currentState.weapon = weapon;
    cgs.clientinfo[n].infoValid = qtrue; cgs.clientinfo[n].team = team; cgs.clientinfo[n].handicap = 100;
    Q_strncpyz(cgs.clientinfo[n].name, name, sizeof(cgs.clientinfo[n].name));
}
static void Reset(void) { fills = pics = texts = 0; }
static qboolean Fill(int i, float x, float y, float w, float h, float r, float g, float b, float a) {
    const float *f = fill[i];
    return f[0] == x && f[1] == y && f[2] == w && f[3] == h && f[4] == r && f[5] == g && f[6] == b && f[7] == a;
}

int main(void) {
    char name[64];
    cg_specTeamVitals.integer = 1; cg_specTeamVitalsWidth.integer = 100; cg_specTeamVitalsY.integer = 85;
    cgs.gametype = GT_TEAM; cgs.maxclients = 4; cgs.startingHealth = 100; cg.snap = &snapshot;
    snapshot.ps.clientNum = 1; snapshot.ps.stats[STAT_HEALTH] = 150; snapshot.ps.stats[STAT_ARMOR] = 30;
    cg_weapons[WP_ROCKET_LAUNCHER].weaponIcon = 33; cgs.media.friendDeadShader = 66; cgs.media.voiceWindowShader = 77;
    Player(0, TEAM_RED, "Player^1Zero", 50, 100, WP_ROCKET_LAUNCHER);
    Player(1, TEAM_BLUE, "Blue", 10, 10, WP_ROCKET_LAUNCHER);
    Player(2, TEAM_RED, "Dead", 80, 0, WP_ROCKET_LAUNCHER);
    Player(3, TEAM_SPECTATOR, "Watcher", 100, 0, WP_ROCKET_LAUNCHER);
    cg_entities[1].currentState.powerups = 1 << PW_QUAD;
    cg_entities[2].currentState.eFlags = EF_DEAD;

    /* Name cleaning and fitting. */
    strcpy(name, "^1Red^9X\x01y"); CG_VitalsCleanName(name); CHECK(!strcmp(name, "Red^9Xy"));
    strcpy(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcd");
    CHECK(CG_VitalsFitName(name, sizeof(name), 100) == 19 && !strcmp(name, "ABCDEFGHIJKLMNO..."));
    strcpy(name, "Short"); CHECK(CG_VitalsFitName(name, sizeof(name), 100) == 5 && !strcmp(name, "Short"));

    /* Gates. */
    Reset(); cgs.gametype = GT_FFA; CG_DrawTeamVitals(); CHECK(!texts); cgs.gametype = GT_TEAM;
    cg.scoreBoardShowing = qtrue; CG_DrawTeamVitals(); CHECK(!texts); cg.scoreBoardShowing = qfalse;

    /* Default mode: red row 0 at (5, 85). */
    Reset(); CG_DrawTeamVitals();
    CHECK(texts == 6 && pics == 4 && fills == 9);
    CHECK(text[0][0] == 0 && text[0][1] == 85 && text[0][2] == 0.12f && text[0][3] == 1 && !strcmp(textValue[0], "0|0.25"));
    CHECK(placements[0] == CG_PLACE_LEFT && placement == CG_PLACE_DEFAULT);
    CHECK(pic[0][0] == 5 && pic[0][1] == 69 && pic[0][2] == 16 && pic[0][4] == 33);
    CHECK(Fill(0, 23, 69, 100, 11, 1, 0, 0, 0.25f));
    CHECK(Fill(1, 23, 69, 50, 11, 1, 0, 0, 0.75f));
    CHECK(text[1][0] == 24 && text[1][1] == 76 && text[1][3] == 0 && text[1][4] == 10 && !strcmp(textValue[1], "PlayerZero|1"));
    CHECK(Fill(2, 23, 79, 100, 6, 1, 0.2f, 0.1f, 1));
    /* Blue followed row: snapshot values, marker, powerup icon, right side. */
    CHECK(text[2][0] == 512 && text[2][1] == 85 && placements[2] == CG_PLACE_RIGHT);
    CHECK(pic[1][0] == 516 && pic[1][1] == 68 && pic[1][2] == 18 && pic[1][4] == 77);
    CHECK(pic[2][0] == 517 && pic[2][4] == 44);
    CHECK(Fill(4, 535, 69, 100, 11, 0.2f, 0.4f, 1, 0.5f));    /* 150 over the maximum */
    CHECK(Fill(5, 535, 79, 30, 6, 0.2f, 0.4f, 1, 1));
    /* Dead red row stacks 21 lower with the dead icon and an empty bar. */
    CHECK(text[4][1] == 106 && pic[3][1] == 90 && pic[3][4] == 66);
    CHECK(Fill(7, 23, 90, 0, 11, 1, 0, 0, 0.75f));

    /* Mode 2: health number and a shifted, narrower name. */
    cg_specTeamVitals.integer = 2; Reset(); CG_DrawTeamVitals();
    CHECK(!strcmp(textValue[1], "50|1") && text[1][0] == 24 && text[1][3] == 1 && text[1][2] == 0.18f);
    CHECK(text[2][0] == 45 && !strcmp(textValue[2], "PlayerZero|1"));

    /* Health colours: tint bar at y - 5, team-coloured name, cyan armor line. */
    cg_specTeamVitals.integer = 1; cg_specTeamVitalsHealthColor.integer = 1; Reset(); CG_DrawTeamVitals();
    CHECK(Fill(0, 23, 80, 100, 4, 0, 0, 0, 0.25f));
    CHECK(Fill(1, 23, 80, 50, 4, 0, 50, 7, 1));
    CHECK(text[1][1] == 77 && !strcmp(textValue[1], "PlayerZero|1"));
    CHECK(Fill(2, 23, 84, 100, 1, 0, 1, 1, 1));
    CHECK(text[4][1] == 107);    /* rows advance 22 */
    puts("team vitals: placement, icons, names and bars OK");
    return 0;
}
