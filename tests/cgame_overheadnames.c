/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail overhead names, health and armor (cgame 0x1000cb60): gates,
 * placement, name fitting, bars, powerup icons and the friend marker. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_specNames, cg_teammateNames, cg_overheadNamesWidth, cg_drawHitFriendTime;
static snapshot_t snapshot;

gitem_t bg_itemlist[] = {
    { NULL },
    { "item_quad", NULL, { NULL }, "icons/quad", "Quad Damage", 30, IT_POWERUP, PW_QUAD },
    { "item_haste", NULL, { NULL }, "icons/haste", "Haste", 30, IT_POWERUP, PW_HASTE },
    { "item_scout", NULL, { NULL }, "icons/scout", "Scout", 30, IT_PERSISTANT_POWERUP, PP_SCOUT },
    { "holdable_invulnerability", NULL, { NULL }, "icons/invulnerability", "Invulnerability", 60, IT_HOLDABLE, HI_INVULNERABILITY },
    { "team_CTF_redflag", NULL, { NULL }, "icons/iconf_red1", "Red Flag", 0, IT_TEAM, PW_REDFLAG },
    { NULL }
};
int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;
gitem_t *BG_FindItemForPersistantPowerup(persistantPowerup_t pp) {
    int i;
    for (i = 1; i < bg_numItems; ++i)
        if (bg_itemlist[i].giType == IT_PERSISTANT_POWERUP && bg_itemlist[i].giTag == (int)pp) return &bg_itemlist[i];
    return NULL;
}
gitem_t *BG_FindItemForHoldable(holdable_t pw) {
    int i;
    for (i = 1; i < bg_numItems; ++i)
        if (bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == (int)pw) return &bg_itemlist[i];
    return NULL;
}

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }

/* Font 0 is 3 units per character, font 1 is 2; heights are 10. */
static int Limited(const char *text, int limit) {
    int length = (int)strlen(text);
    return limit > 0 && limit < length ? limit : length;
}
int CG_Text_WidthInFont(const char *text, int font, float scale, int limit) {
    CHECK(scale == .12f && (font == 0 || font == 1));
    return Limited(text, limit) * (font ? 2 : 3);
}
int CG_Text_HeightInFont(const char *text, int font, float scale, int limit) {
    CHECK(scale == .12f && font == 1 && !limit); (void)text;
    return 10;
}

/* A flat projection: across = x / 1000, up = z / 1000. */
static qboolean projectFails;
qboolean trap_R_ProjectScreenFraction(const vec3_t point, float *across, float *up) {
    if (projectFails) return qfalse;
    *across = point[0] / 1000; *up = point[2] / 1000;
    return qtrue;
}

static vec4_t color;
static qboolean colorSet;
static int colorClears, registrations;
static struct { float x, y, w, h, s2, t2; qhandle_t shader; vec4_t color; } pics[32];
static struct { int x, y, font, limit; float size; char text[64]; vec4_t color; } texts[8];
static int picCount, textCount;
static char registered[8][MAX_QPATH];

void trap_R_SetColor(const float *rgba) {
    if (rgba) { memcpy(color, rgba, sizeof(color)); colorSet = qtrue; }
    else { colorSet = qfalse; ++colorClears; }
}
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {
    CHECK(picCount < 32 && colorSet && !s1 && !t1);
    pics[picCount].x = x; pics[picCount].y = y; pics[picCount].w = w; pics[picCount].h = h;
    pics[picCount].s2 = s2; pics[picCount].t2 = t2; pics[picCount].shader = shader;
    memcpy(pics[picCount++].color, color, sizeof(color));
}
void trap_R_DrawText(int x, int y, const char *text, int font, float size, int limit, int forceColor) {
    CHECK(textCount < 8 && colorSet && !forceColor);
    texts[textCount].x = x; texts[textCount].y = y; texts[textCount].font = font;
    texts[textCount].limit = limit; texts[textCount].size = size;
    snprintf(texts[textCount].text, sizeof(texts[textCount].text), "%s", text);
    memcpy(texts[textCount++].color, color, sizeof(color));
}
qhandle_t trap_R_RegisterShader(const char *name) {
    CHECK(registrations < 8);
    snprintf(registered[registrations], MAX_QPATH, "%s", name);
    return 500 + registrations++;
}

static void Draw(void) {
    int clears = colorClears;
    picCount = textCount = registrations = 0;
    CG_DrawOverheadNames();
    CHECK(!colorSet || colorClears > clears);
}
static qboolean Color(const float *c, float r, float g, float b, float a) {
    return c[0] == r && c[1] == g && c[2] == b && c[3] == a;
}
static qboolean Pic(int i, float x, float y, float w, float h, qhandle_t shader) {
    return i < picCount && pics[i].x == x && pics[i].y == y && pics[i].w == w && pics[i].h == h &&
           pics[i].shader == shader;
}

/* A teammate standing so its marker point projects to (across, up). */
static centity_t *Player(int number, int team, float across, float up) {
    centity_t *cent = &cg_entities[number];
    memset(cent, 0, sizeof(*cent));
    cent->currentValid = qtrue;
    cent->currentState.number = number;
    cent->currentState.clientNum = number;
    cent->currentState.eType = ET_PLAYER;
    cent->currentState.health = 100;
    cent->currentState.armor = 50;
    VectorSet(cent->lerpOrigin, across * 1000, 0, up * 1000 - 40);
    cgs.clientinfo[number].infoValid = qtrue;
    cgs.clientinfo[number].team = team;
    cgs.clientinfo[number].handicap = 100;
    strcpy(cgs.clientinfo[number].name, "^1Ma^7te");
    return cent;
}

int main(void) {
    centity_t *mate;
    vec4_t tint;
    int i;
    cg.snap = &snapshot;
    cgs.glconfig.vidWidth = 640; cgs.glconfig.vidHeight = 480; cgs.screenYScale = 1;
    cgs.maxclients = 8; cgs.gametype = GT_TEAM; cgs.startingHealth = 100;
    cgs.media.whiteShader = 1; cgs.media.healthSegmentShader = 2;
    cgs.media.friendShader = 3; cgs.media.friendHitShader = 4;
    snapshot.ps.clientNum = 0; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    snapshot.ps.stats[STAT_MAX_HEALTH] = 100;
    VectorSet(cg.refdef.vieworg, 0, -100, 0); VectorSet(cg.refdef.viewaxis[0], 0, 1, 0);
    cg_specNames.integer = 2; cg_teammateNames.integer = 2; cg_overheadNamesWidth.integer = 75;
    cg_drawHitFriendTime.integer = 100; cg.time = 10000;

    /* Health tint (0x1000ca90): the handicapped g_startingHealth maximum. */
    cgs.clientinfo[5].handicap = 100;
    CG_OverheadHealthColor(5, 50, tint); CHECK(Color(tint, 1, .5f, 0, 1));
    CG_OverheadHealthColor(5, -10, tint); CHECK(Color(tint, 1, 0, 0, 1));
    CG_OverheadHealthColor(5, 100, tint); CHECK(Color(tint, 1, 1, .5f, 1));
    CG_OverheadHealthColor(5, 400, tint); CHECK(Color(tint, 1, 1, 1, 1));
    cgs.clientinfo[5].handicap = 50; CG_OverheadHealthColor(5, 50, tint); CHECK(Color(tint, 1, 1, .5f, 1));
    cgs.clientinfo[5].handicap = 0; CG_OverheadHealthColor(5, 50, tint); CHECK(Color(tint, 1, .5f, 0, 1));
    cgs.clientinfo[5].handicap = 300; CG_OverheadHealthColor(5, 50, tint); CHECK(Color(tint, 1, .5f, 0, 1));
    cgs.startingHealth = 0; CG_OverheadHealthColor(5, 50, tint); CHECK(Color(tint, 1, 1, 1, 1));
    CG_OverheadHealthColor(5, -1, tint); CHECK(Color(tint, 1, 0, 0, 1));
    cgs.startingHealth = 100;
    /* Team name colors (0x10012b50). */
    CHECK(Color(CG_OverheadTeamColor(TEAM_RED), 1, .5f, .5f, 1) && Color(CG_OverheadTeamColor(TEAM_BLUE), .5f, .75f, 1, 1));
    CHECK(Color(CG_OverheadTeamColor(TEAM_FREE), 1, 1, 1, 1) && Color(CG_OverheadTeamColor(TEAM_SPECTATOR), .85f, .85f, .85f, 1));
    CHECK(Color(CG_OverheadTeamColor(9), 1, .8f, .2f, 1) && Color(CG_OverheadTeamColor(-1), 1, .8f, .2f, 1));

    /* Teammate in mode 2, centered: box, shadow, name, four health
     * segments, the armor strip and the friend marker. "Mate" is 8 wide in
     * font 1, so the 11-unit box centers it 1 unit in. */
    mate = Player(5, TEAM_RED, .5f, .5f);
    Draw();
    CHECK(textCount == 2 && picCount == 7);
    CHECK(Pic(0, 313.5f, 211, 13, 18, 1) && Color(pics[0].color, 0, 0, 0, .5f) && !pics[0].s2 && !pics[0].t2);
    CHECK(!strcmp(texts[0].text, "Mate") && texts[0].x == 316 && texts[0].y == 223 && texts[0].font == 1);
    CHECK(texts[0].limit == 4 && Color(texts[0].color, 0, 0, 0, 1));
    CHECK(texts[0].size == (float)((float)(480 / 768.0 * 96.0) * (double).12f));
    CHECK(texts[1].x == 315 && texts[1].y == 222 && Color(texts[1].color, 1, .5f, .5f, 1));
    for (i = 0; i < 4; ++i)
        CHECK(Pic(1 + i, 314.5f + 3 * i, 224, 2, 2, 2) && pics[1 + i].s2 == 1 && Color(pics[1 + i].color, 1, 1, .5f, 1));
    CHECK(Pic(5, 314.5f, 227, 5, 1, 1) && Color(pics[5].color, 0, .625f, 1, 1));
    CHECK(Pic(6, 316, 229, 8, 8, 3) && Color(pics[6].color, 1, 1, .5f, 1));
    /* A recent hit turns the marker red (cg_drawHitFriendTime). */
    mate->pe.painTime = cg.time - 99; Draw(); CHECK(Pic(6, 316, 229, 8, 8, 4) && Color(pics[6].color, 1, 0, 0, 1));
    mate->pe.painTime = cg.time - 100; Draw(); CHECK(Pic(6, 316, 229, 8, 8, 3));

    /* Segments past 25/50/75% of STAT_MAX_HEALTH; armor scales to 11. */
    mate->currentState.health = 26; mate->currentState.armor = 100; Draw();
    CHECK(picCount == 5 && Pic(3, 314.5f, 227, 11, 1, 1) && Color(pics[1].color, 1, 26 / 100.0f, 0, 1));
    mate->currentState.health = 25; mate->currentState.armor = 0; Draw();
    CHECK(picCount == 3 && Pic(1, 314.5f, 224, 2, 2, 2) && Pic(2, 316, 229, 8, 8, 3));
    mate->currentState.health = 76; mate->currentState.armor = 9; Draw();
    CHECK(picCount == 7 && Pic(5, 314.5f, 227, 0, 1, 1));
    mate->currentState.health = 100; mate->currentState.armor = 50;

    /* Mode 1 names only inside the central band; bars stay. */
    cg_teammateNames.integer = 1;
    Draw(); CHECK(textCount == 2);
    mate = Player(5, TEAM_RED, .39f, .5f); Draw(); CHECK(textCount == 0 && picCount == 6);
    mate = Player(5, TEAM_RED, .59f, .69f); Draw(); CHECK(textCount == 2 && picCount == 7);
    mate = Player(5, TEAM_RED, .5f, .71f); Draw(); CHECK(textCount == 0 && picCount == 6);
    mate = Player(5, TEAM_RED, .5f, .41f); Draw(); CHECK(textCount == 2);
    /* Mode 0 draws nothing for teammates. */
    cg_teammateNames.integer = 0; Draw(); CHECK(!textCount && !picCount);
    cg_teammateNames.integer = 2;

    /* Enemies and individual modes get nothing while playing. */
    mate = Player(5, TEAM_BLUE, .5f, .5f); Draw(); CHECK(!textCount && !picCount);
    cgs.gametype = GT_FFA; mate = Player(5, TEAM_FREE, .5f, .5f); snapshot.ps.persistant[PERS_TEAM] = TEAM_FREE;
    Draw(); CHECK(!textCount && !picCount);
    cgs.gametype = GT_TEAM; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;

    /* Spectators: the player's team color, bars, no friend marker. */
    snapshot.ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
    mate = Player(5, TEAM_BLUE, .5f, .5f); Draw();
    CHECK(textCount == 2 && Color(texts[1].color, .5f, .75f, 1, 1) && picCount == 6);
    cgs.gametype = GT_FFA; mate = Player(5, TEAM_FREE, .5f, .5f); Draw();
    CHECK(textCount == 2 && Color(texts[1].color, .85f, .85f, .85f, 1));
    cgs.gametype = GT_TEAM;
    cg_specNames.integer = 1; mate = Player(5, TEAM_BLUE, .3f, .5f); Draw(); CHECK(!textCount && picCount == 5);
    cg_specNames.integer = 0; Draw(); CHECK(!textCount && !picCount);
    /* Free-flying spectators need cg_specNames; followers run the scan. */
    cg.predictedPlayerState.pm_type = PM_SPECTATOR; Draw(); CHECK(!picCount);
    cg_specNames.integer = 2; Draw(); CHECK(textCount == 2 && picCount == 6);
    cg_specNames.integer = 0; cg.predictedPlayerState.pm_flags = PMF_FOLLOW;
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; mate = Player(5, TEAM_RED, .5f, .5f); Draw();
    CHECK(textCount == 2 && picCount == 7);
    cg.predictedPlayerState.pm_type = PM_NORMAL; cg.predictedPlayerState.pm_flags = 0; cg_specNames.integer = 2;

    /* Powerups: flight is never shown, frozen removes the bars; icons are
     * 8 wide with 2-unit gaps, centered, and replace the friend marker. */
    mate->currentState.powerups = (1 << PW_QUAD) | (1 << PW_FLIGHT) | (1 << PW_SCOUT);
    Draw();
    CHECK(picCount == 8 && Pic(6, 311, 229, 8, 8, 500) && Pic(7, 321, 229, 8, 8, 501));
    CHECK(Color(pics[6].color, 1, 1, 1, 1) && !strcmp(registered[0], "icons/quad") && !strcmp(registered[1], "icons/scout"));
    mate->currentState.powerups = (1 << PW_INVULNERABILITY) | (1 << PW_REDFLAG) | (1 << PW_HASTE);
    Draw();
    CHECK(picCount == 9 && Pic(6, 306, 229, 8, 8, 500) && !strcmp(registered[0], "icons/iconf_red1"));
    CHECK(!strcmp(registered[1], "icons/haste") && !strcmp(registered[2], "icons/invulnerability"));
    /* An item-less powerup still counts toward the centering. */
    mate->currentState.powerups = (1 << PW_QUAD) | (1 << PW_GUARD);
    Draw(); CHECK(picCount == 7 && Pic(6, 311, 229, 8, 8, 500));
    mate->currentState.powerups = 1 << PW_FROZEN;
    Draw(); CHECK(textCount == 2 && picCount == 1 && Pic(0, 313.5f, 211, 13, 12, 1));
    mate->currentState.powerups = 0;

    /* Long names are fitted in font 0 to cg_overheadNamesWidth, keep one
     * character fewer and gain "..."; the box follows the font 1 width. */
    cg_overheadNamesWidth.integer = 50;
    strcpy(cgs.clientinfo[5].name, "^3ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    Draw();
    CHECK(!strcmp(texts[1].text, "ABCDEFGHIJKLMNO...") && texts[1].limit == 19);
    CHECK(Pic(0, 320 - 18 - 1, 211, 38, 18, 1) && texts[1].x == 302);
    strcpy(cgs.clientinfo[5].name, "ABCDEFGHIJKLMNOP"); Draw();   /* exactly 48 wide */
    CHECK(!strcmp(texts[1].text, "ABCDEFGHIJKLMNOP") && texts[1].limit == 16);
    strcpy(cgs.clientinfo[5].name, "\001Bad\177^9ok"); Draw();
    CHECK(!strcmp(texts[1].text, "Bad\177^9ok"));
    strcpy(cgs.clientinfo[5].name, "^^11"); Draw(); CHECK(!strcmp(texts[1].text, "^1") && texts[1].limit == 0);
    cg_overheadNamesWidth.integer = 75;
    strcpy(cgs.clientinfo[5].name, "Mate");

    /* Rejections: self, invalid, non-players, the dead, behind the view,
     * no client info, failed projections and clients past sv_maxclients. */
    Draw(); CHECK(picCount == 7);
    snapshot.ps.clientNum = 5; Draw(); CHECK(!picCount); snapshot.ps.clientNum = 0;
    mate->currentValid = qfalse; Draw(); CHECK(!picCount); mate->currentValid = qtrue;
    mate->currentState.eType = ET_ITEM; Draw(); CHECK(!picCount); mate->currentState.eType = ET_PLAYER;
    mate->currentState.eFlags = EF_DEAD; Draw(); CHECK(!picCount); mate->currentState.eFlags = 0;
    VectorSet(cg.refdef.viewaxis[0], 0, -1, 0); Draw(); CHECK(!picCount); VectorSet(cg.refdef.viewaxis[0], 0, 1, 0);
    cgs.clientinfo[5].infoValid = qfalse; Draw(); CHECK(!picCount); cgs.clientinfo[5].infoValid = qtrue;
    projectFails = qtrue; Draw(); CHECK(!picCount); projectFails = qfalse;
    mate->currentState.clientNum = MAX_CLIENTS; Draw(); CHECK(!picCount); mate->currentState.clientNum = 5;
    cgs.maxclients = 5; Draw(); CHECK(!picCount); cgs.maxclients = 8;
    cg.snap = NULL; Draw(); CHECK(!picCount);

    puts("overhead names: gates, placement, fitting, bars, icons and friend marker OK");
    return 0;
}
