/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail obituaries (0x10019130) and the CG_PLAYER_OBIT kill feed
 * (0x10018f60/0x1002e9b0): messages, icons, center prints, follow-killer,
 * row limit, fade, expiry and layout. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
weaponInfo_t cg_weapons[MAX_WEAPONS];
vmCvar_t cg_obituaryRowSize, cg_drawFragMessages, cg_followKiller, cg_cameraOrbit;
void CG_DrawObituaries(const rectDef_t *rect, float scale);
static snapshot_t snapshot;
static char players[MAX_CLIENTS][64];
static char printed[256], centered[256];
static int centerY, deadMarkers;
static float centerScale;

const char *CG_ConfigString(int index) {
    return index >= CS_PLAYERS && index < CS_PLAYERS + MAX_CLIENTS ? players[index - CS_PLAYERS] : "";
}
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void QDECL CG_Error(const char *fmt, ...) { (void)fmt; abort(); }
void QDECL CG_Printf(const char *fmt, ...) {
    va_list args; va_start(args, fmt); vsnprintf(printed, sizeof(printed), fmt, args); va_end(args);
}
void CG_DeadTeammateMarker(const entityState_t *obituary) { (void)obituary; ++deadMarkers; }
const char *CG_PlaceString(int rank) { return rank == 2 ? "^12nd^7" : "?"; }
void CG_CenterPrintNative(const char *text, int y, float scale) {
    snprintf(centered, sizeof(centered), "%s", text); centerY = y; centerScale = scale;
}
static const vec4_t teamColors[5] = { { 1, 1, 1, 1 }, { 1, .5f, .5f, 1 }, { .5f, .75f, 1, 1 }, { .85f, .85f, .85f, 1 }, { 1, .8f, .2f, 1 } };
const float *CG_OverheadTeamColor(int team) { return teamColors[team >= 0 && team <= 3 ? team : 4]; }

/* Font 0 is 6 units per character; every row is 10 high. */
static int fontSet = -1;
void CG_SetTextFont(int font) { fontSet = font; }
int CG_Text_WidthInFont(const char *text, int font, float scale, int limit) {
    CHECK(font == 0 && scale == .2f && !limit); return (int)strlen(text) * 6;
}
int CG_Text_HeightInFont(const char *text, int font, float scale, int limit) {
    CHECK(font == 0 && scale == .2f && !limit); (void)text; return 10;
}
static struct { float x, y; vec4_t color; char text[64]; } paints[16];
static struct { float x, y, w, h; qhandle_t shader; vec4_t color; } pics[16];
static int paintCount, picCount;
static vec4_t current;
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    CHECK(paintCount < 16 && fontSet == 0 && scale == .2f && !adjust && !limit && style == ITEM_TEXTSTYLE_NORMAL);
    paints[paintCount].x = x; paints[paintCount].y = y;
    memcpy(paints[paintCount].color, color, sizeof(vec4_t));
    snprintf(paints[paintCount++].text, 64, "%s", text);
}
void trap_R_SetColor(const float *rgba) { if (rgba) memcpy(current, rgba, sizeof(current)); else current[3] = -1; }
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    CHECK(picCount < 16 && current[3] >= 0);
    pics[picCount].x = x; pics[picCount].y = y; pics[picCount].w = w; pics[picCount].h = h;
    pics[picCount].shader = shader; memcpy(pics[picCount++].color, current, sizeof(current));
}

static void Draw(void) {
    rectDef_t rect;
    memset(&rect, 0, sizeof(rect)); rect.x = 10; rect.y = 100.7f;
    paintCount = picCount = 0;
    CG_DrawObituaries(&rect, .2f);
}
static void Kill(int target, int attacker, int mod) {
    entityState_t es;
    memset(&es, 0, sizeof(es));
    es.otherEntityNum = target; es.otherEntityNum2 = attacker; es.eventParm = mod;
    printed[0] = centered[0] = 0;
    CG_RetailObituary(&es);
}
static qboolean Color(const float *c, float r, float g, float b, float a) {
    return c[0] == r && c[1] == g && c[2] == b && c[3] == a;
}

int main(void) {
    int i;
    cg.snap = &snapshot; cg.time = 10000; cg.clientNum = 0; snapshot.ps.clientNum = 0;
    cgs.gametype = GT_TEAM; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    cg_obituaryRowSize.integer = 5; cg_drawFragMessages.integer = 1;
    cgs.media.obituaryIcon = 70; cgs.media.thawIcon = 71;
    cg_weapons[WP_ROCKET_LAUNCHER].weaponIcon = 75; cg_weapons[WP_RAILGUN].weaponIcon = 77;
    cg_weapons[WP_HEAVY_MACHINEGUN].weaponIcon = 84; cg_weapons[WP_LIGHTNING].weaponIcon = 76;
    strcpy(players[3], "n\\^1Killer\\t\\2"); cgs.clientinfo[3].team = TEAM_BLUE;
    strcpy(players[5], "n\\Victim\\t\\1"); cgs.clientinfo[5].team = TEAM_RED;

    /* A kill: console line, both names and the weapon icon in the feed. */
    Kill(5, 3, MOD_ROCKET_SPLASH);
    CHECK(!strcmp(printed, "Victim^7 almost dodged ^1Killer^7's rocket\n") && deadMarkers == 1);
    CHECK(CG_ObituaryRows() == 1);
    Draw();
    CHECK(paintCount == 2 && picCount == 1);
    /* Team modes strip the names; the attacker is in its team color. */
    CHECK(!strcmp(paints[0].text, "Killer") && paints[0].x == 10 && paints[0].y == 100);
    CHECK(Color(paints[0].color, .5f, .75f, 1, 1));
    CHECK(pics[0].x == 10 + 36 + 5 && pics[0].y == 90 && pics[0].w == 10 && pics[0].shader == 75);
    CHECK(Color(pics[0].color, 1, 1, 1, 1));
    CHECK(!strcmp(paints[1].text, "Victim") && paints[1].x == 10 + 36 + 20 && Color(paints[1].color, 1, .5f, .5f, 1));
    /* Fade over the final 200 ms, then expire after two seconds. */
    cg.time = 11900; Draw(); CHECK(paints[0].color[3] == .5f && pics[0].color[3] == .5f);
    cg.time = 12000; Draw(); CHECK(!paintCount && !CG_ObituaryRows());

    /* Single-party deaths feed the target alone after the default icon. */
    cg.time = 20000;
    Kill(5, ENTITYNUM_WORLD, MOD_FALLING); CHECK(!strcmp(printed, "Victim^7 cratered.\n"));
    Draw();
    CHECK(paintCount == 1 && picCount == 1 && pics[0].x == 15 && pics[0].shader == 70 && paints[0].x == 30);
    CG_ClearObituaries();
    /* Self kills are gendered; switching teams is not a self kill. */
    cgs.clientinfo[5].gender = GENDER_FEMALE;
    Kill(5, 5, MOD_ROCKET_SPLASH); CHECK(!strcmp(printed, "Victim^7 blew herself up.\n"));
    cgs.clientinfo[5].gender = GENDER_NEUTER;
    Kill(5, 5, MOD_PROXIMITY_MINE); CHECK(!strcmp(printed, "Victim^7 found it's prox mine.\n"));
    cgs.clientinfo[5].gender = GENDER_MALE;
    Kill(5, 5, MOD_SUICIDE); CHECK(!strcmp(printed, "Victim^7 killed himself.\n"));
    Kill(5, 5, MOD_SWITCHTEAM); CHECK(!strcmp(printed, "Victim^7 switched teams.\n"));
    Kill(5, ENTITYNUM_WORLD, MOD_THAW); CHECK(!strcmp(printed, "Victim^7 was auto-thawed.\n"));
    Draw(); CHECK(pics[picCount - 1].shader == 71);
    Kill(5, 3, MOD_THAW); CHECK(!strcmp(printed, "Victim^7 was thawed by ^1Killer^7\n"));
    Kill(5, 3, MOD_HMG); CHECK(!strcmp(printed, "Victim^7 was ripped up by ^1Killer^7's HMG\n"));
    Kill(5, 3, MOD_TELEFRAG); CHECK(!strcmp(printed, "Victim^7 tried to invade ^1Killer^7's personal space\n"));
    Kill(5, 3, MOD_RAILGUN_HEADSHOT); CHECK(!strcmp(printed, "Victim^7 was railed by ^1Killer^7\n"));
    Kill(5, ENTITYNUM_WORLD, MOD_UNKNOWN); CHECK(!strcmp(printed, "Victim^7 died.\n"));

    /* Rows: only the newest cg_obituaryRowSize survive; 0 hides all. */
    CHECK(CG_ObituaryRows() == 5);
    Draw(); CHECK(!strcmp(paints[paintCount - 1].text, "Victim") && picCount == 5);
    CHECK(pics[4].shader == 70 && pics[3].shader == 77 && pics[2].shader == 70); /* died, railed, telefrag */
    for (i = 0; i < 5; ++i) CHECK(pics[i].y == 90 + 12 * i);
    cg_obituaryRowSize.integer = 2; Kill(5, 3, MOD_LIGHTNING); CHECK(CG_ObituaryRows() == 2);
    cg_obituaryRowSize.integer = 0; Kill(5, 3, MOD_LIGHTNING); CHECK(CG_ObituaryRows() == 0);
    cg_obituaryRowSize.integer = 5;

    /* The killer's center print: team modes, Freeze Tag, individual modes. */
    snapshot.ps.clientNum = 3; snapshot.ps.persistant[PERS_TEAM] = TEAM_BLUE;
    Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You fragged Victim^7.") && centerY == 90 && centerScale == .3f);
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; cgs.clientinfo[3].team = TEAM_RED;
    Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You fragged Victim^7, your teammate.\n\n^1Watch your fire!^7"));
    Kill(5, 3, MOD_TELEFRAG); CHECK(!strcmp(centered, "You fragged Victim^7, your teammate."));
    cgs.gametype = GT_FREEZE;
    Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You froze Victim^7, your teammate.\n\n^1Watch your fire!^7"));
    Kill(5, 3, MOD_THAW); CHECK(!strcmp(centered, "You thawed Victim^7."));
    snapshot.ps.persistant[PERS_TEAM] = TEAM_BLUE; Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You froze Victim^7."));
    cgs.gametype = GT_RED_ROVER; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You fragged Victim^7."));
    cgs.gametype = GT_FFA; snapshot.ps.persistant[PERS_RANK] = 1; snapshot.ps.persistant[PERS_SCORE] = 12;
    Kill(5, 3, MOD_ROCKET); CHECK(!strcmp(centered, "You fragged Victim^7\n^12nd^7 place with 12."));
    cg_drawFragMessages.integer = 0; Kill(5, 3, MOD_ROCKET); CHECK(!centered[0]);
    cg_drawFragMessages.integer = 1; cg_cameraOrbit.integer = 1; Kill(5, 3, MOD_ROCKET); CHECK(!centered[0]);
    cg_cameraOrbit.integer = 0;
    /* Individual modes keep the colored names in the feed. */
    CG_ClearObituaries(); Kill(5, 3, MOD_RAILGUN); Draw(); CHECK(!strcmp(paints[0].text, "^1Killer^7"));
    /* Suicide-type deaths never center print. */
    Kill(3, 3, MOD_ROCKET_SPLASH); CHECK(!centered[0]);

    /* The victim remembers its killer and the weapon icon. */
    snapshot.ps.clientNum = 5; cg.clientNum = 5;
    Kill(5, 3, MOD_RAILGUN); CHECK(!strcmp(cg.killerName, "^1Killer^7") && cg.killerIcon == 77);
    Kill(5, ENTITYNUM_WORLD, MOD_FALLING); CHECK(!cg.killerName[0]);
    /* A following spectator moves to the killer 400 ms later. */
    cg_followKiller.integer = 1; snapshot.ps.pm_flags = PMF_FOLLOW; snapshot.ps.eFlags = EF_SPECTATOR;
    cg.followTime = 0; Kill(5, 3, MOD_RAILGUN); CHECK(cg.followClient == 3 && cg.followTime == cg.time + 400);
    cg.followTime = 0; Kill(5, ENTITYNUM_WORLD, MOD_UNKNOWN); CHECK(!cg.followTime);
    snapshot.ps.eFlags = 0; Kill(5, 3, MOD_RAILGUN); CHECK(!cg.followTime);
    /* A disconnected attacker still names its (empty) player string. */
    players[3][0] = 0; Kill(5, 3, MOD_RAILGUN); CHECK(!strcmp(printed, "Victim^7 was railed by ^7\n"));
    /* No target info: nothing at all. */
    players[5][0] = 0; printed[0] = 0; i = CG_ObituaryRows(); Kill(5, 3, MOD_RAILGUN);
    CHECK(!printed[0] && CG_ObituaryRows() == i);

    puts("obituaries: messages, icons, center prints, follow, rows, fade and layout OK");
    return 0;
}
