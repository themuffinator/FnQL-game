/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Spectator item timers: per-frame countdown and ordering (retail 0x1001ca90
 * and 0x1001ca10) and the painter (0x1000f700). */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_specItemTimers, cg_specItemTimersSize, cg_specItemTimersX, cg_specItemTimersY;
gitem_t bg_itemlist[] = {
    { NULL },
    { "item_armor_body", NULL, { NULL }, "icons/iconr_red", "Heavy Armor", 100, IT_ARMOR, ARMOR_TIER_RED },
    { "item_armor_combat", NULL, { NULL }, "icons/iconr_yellow", "Armor", 50, IT_ARMOR, ARMOR_TIER_YELLOW },
    { "item_health_mega", NULL, { NULL }, "icons/iconh_mega", "Mega Health", 100, IT_HEALTH, HEALTH_TIER_MEGA },
    { "item_quad", NULL, { NULL }, "icons/quad", "Quad Damage", 30, IT_POWERUP, PW_QUAD },
    { "item_armor_jacket", NULL, { NULL }, "icons/iconr_green", "Armor", 25, IT_ARMOR, ARMOR_TIER_GREEN },
};
int bg_numItems = (int)(sizeof(bg_itemlist) / sizeof(bg_itemlist[0]));
static char timeoutStart[16] = "";

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) { return index == CS_TIMEOUT_START_TIME ? timeoutStart : ""; }

static int fills, pics, texts, font = -1;
static float fillRect[8][4], picRect[8][4], textX[8], textY[8], textScale[8];
static const float *fillColor[8];
static char textValue[8][16], *shaderName[8];
static const char *lastShader;
qhandle_t trap_R_RegisterShader(const char *name) { lastShader = name; return 5; }
void CG_FillRect(float x, float y, float w, float h, const float *color) {
    fillRect[fills][0] = x; fillRect[fills][1] = y; fillRect[fills][2] = w; fillRect[fills][3] = h; fillColor[fills++] = color;
}
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    (void)shader; shaderName[pics] = (char *)lastShader;
    picRect[pics][0] = x; picRect[pics][1] = y; picRect[pics][2] = w; picRect[pics++][3] = h;
}
void CG_SetTextFont(int value) { font = value; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)adjust; (void)limit;
    CHECK(style == ITEM_TEXTSTYLE_SHADOWED && color[0] == 1 && color[3] == 1 && font == 0);
    textX[texts] = x; textY[texts] = y; textScale[texts] = scale; snprintf(textValue[texts++], 16, "%s", text);
}

static void Add(int entity, int side, int item, int remaining, int distance, int group, float x, float y, float z) {
    cgTimedItem_t *t = &cg.timedItems[cg.timedItemCount++];
    t->entity = entity; t->side = side; t->item = item; t->remaining = remaining;
    t->distance = distance; t->group = group; t->origin[0] = x; t->origin[1] = y; t->origin[2] = z;
}
static void Reset(void) { CG_ClearTimedItems(); fills = pics = texts = 0; }
static qboolean Near(float a, float b) { return a > b - 0.001f && a < b + 0.001f; }

int main(void) {
    cg_specItemTimers.integer = 7; cg_specItemTimersSize.value = 0.24f;
    cg_specItemTimersX.integer = 10; cg_specItemTimersY.integer = 200;

    /* Countdown and order need a spectator. */
    Reset(); Add(1, 0, 1, 5000, 0, 0, 0, 0, 10); Add(2, 0, 4, 2000, 0, 0, 0, 0, 50);
    CG_UpdateTimedItems(16);
    CHECK(cg.timedItems[0].entity == 1 && cg.timedItems[0].remaining == 5000);
    cg.predictedPlayerState.eFlags = EF_SPECTATOR;
    CG_UpdateTimedItems(16);
    /* Outside CTF: highest origin first. */
    CHECK(cg.timedItems[0].entity == 2 && cg.timedItems[0].remaining == 1984 && cg.timedItems[1].remaining == 4984);
    /* A timeout freezes the countdown but still sorts. */
    strcpy(timeoutStart, "12345"); CG_UpdateTimedItems(16);
    CHECK(cg.timedItems[0].remaining == 1984); timeoutStart[0] = 0;
    /* Expired records clamp to zero after going one frame negative. */
    cg.timedItems[0].remaining = 10; CG_UpdateTimedItems(16); CHECK(cg.timedItems[0].remaining == -6);
    CG_UpdateTimedItems(16); CHECK(cg.timedItems[0].remaining == 0);
    /* Ties fall through to y then x, descending. */
    Reset(); Add(1, 0, 1, 1, 0, 0, 5, 1, 9); Add(2, 0, 1, 1, 0, 0, 1, 7, 9); Add(3, 0, 1, 1, 0, 0, 8, 1, 9);
    CG_UpdateTimedItems(0);
    CHECK(cg.timedItems[0].entity == 2 && cg.timedItems[1].entity == 3 && cg.timedItems[2].entity == 1);
    /* CTF: side descending, distance ascending, group ascending, entity descending. */
    cgs.gametype = GT_CTF;
    Reset(); Add(1, 1, 1, 1, 50, 0, 0, 0, 0); Add(2, 2, 1, 1, 90, 0, 0, 0, 0); Add(3, 2, 1, 1, 10, 0, 0, 0, 0);
    Add(4, 1, 1, 1, 50, 2, 0, 0, 0); Add(5, 1, 1, 1, 50, 1, 0, 0, 0); Add(6, 1, 1, 1, 50, 1, 0, 0, 0);
    CG_UpdateTimedItems(0);
    CHECK(cg.timedItems[0].entity == 3 && cg.timedItems[1].entity == 2);
    /* An ungrouped record compares by z, then entity, so it trails here. */
    CHECK(cg.timedItems[2].entity == 6 && cg.timedItems[3].entity == 5 && cg.timedItems[4].entity == 4);
    CHECK(cg.timedItems[5].entity == 1);
    cgs.gametype = GT_TEAM;

    /* Cvar bits. */
    CHECK(CG_TimedItemShown(7, &bg_itemlist[4]) && !CG_TimedItemShown(6, &bg_itemlist[4]));
    CHECK(CG_TimedItemShown(7, &bg_itemlist[3]) && !CG_TimedItemShown(5, &bg_itemlist[3]));
    CHECK(CG_TimedItemShown(7, &bg_itemlist[1]) && !CG_TimedItemShown(3, &bg_itemlist[1]));
    CHECK(!CG_TimedItemShown(7, &bg_itemlist[2]) && CG_TimedItemShown(8, &bg_itemlist[2]));
    CHECK(CG_TimedItemShown(0, &bg_itemlist[5]));

    /* Painter: ungrouped neutral, then a red-side row and a filtered one. */
    Reset(); Add(1, 0, 4, 12001, 0, 0, 0, 0, 0); Add(2, 1, 1, 0, 0, 0, 0, 0, 0); Add(3, 0, 2, 9000, 0, 0, 0, 0, 0);
    CG_DrawItemTimers();
    CHECK(pics == 2 && fills == 1 && texts == 1);
    CHECK(!strcmp(shaderName[0], "icons/quad") && picRect[0][0] == 10 && picRect[0][1] == 200 && Near(picRect[0][2], 24));
    CHECK(!strcmp(textValue[0], "13") && Near(textX[0], 37) && Near(textY[0], 219) && Near(textScale[0], 0.24f));
    CHECK(Near(fillRect[0][0], 10) && Near(fillRect[0][1], 226) && Near(fillRect[0][2], 54) && Near(fillRect[0][3], 24));
    CHECK(fillColor[0][0] == 1 && Near(fillColor[0][1], 0.2f) && fillColor[0][3] == 0.25f);
    CHECK(Near(picRect[1][1], 226));    /* expired: icon without a count */
    /* Hidden with the scoreboard, without records, or for players. */
    fills = pics = texts = 0; cg.scoreBoardShowing = qtrue; CG_DrawItemTimers(); CHECK(!pics);
    cg.scoreBoardShowing = qfalse; cg.predictedPlayerState.eFlags = 0; CG_DrawItemTimers(); CHECK(!pics);
    cg.predictedPlayerState.eFlags = EF_SPECTATOR; cg_specItemTimers.integer = 0; CG_DrawItemTimers(); CHECK(!pics);
    cg_specItemTimers.integer = 7;
    /* Grouped icons: half size, offset size/4 per side count; rows stack until
     * the side count reaches the grouped non-red total. */
    Reset(); Add(1, 2, 1, 1000, 0, 1, 0, 0, 0); Add(2, 2, 1, 1000, 0, 1, 0, 0, 0); Add(3, 0, 4, 1000, 0, 0, 0, 0, 0);
    CG_DrawItemTimers();
    CHECK(pics == 3 && Near(picRect[0][0], 9) && Near(picRect[0][1], 199) && Near(picRect[0][2], 12));
    CHECK(Near(picRect[1][0], 15) && Near(picRect[1][1], 205));
    CHECK(Near(picRect[2][1], 226));
    CHECK(fills == 2 && Near(fillColor[0][2], 1.0f) && Near(fillRect[1][1], 200));
    puts("item timers: countdown, ordering, filters and layout OK");
    return 0;
}
