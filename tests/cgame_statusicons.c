/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail armor-tier and health tints and the held-key icons. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
itemInfo_t cg_items[MAX_ITEMS];
static snapshot_t snapshot;
gitem_t bg_itemlist[] = {
    { NULL },
    { "item_key_silver", NULL, { NULL }, "icons/key_silver", "Silver Key", 0, IT_KEY, KEY_SILVER },
    { "item_key_gold", NULL, { NULL }, "icons/key_gold", "Gold Key", 0, IT_KEY, KEY_GOLD },
    { "item_key_master", NULL, { NULL }, "icons/key_master", "Master Key", 0, IT_KEY, KEY_MASTER },
    { NULL }
};
int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void CG_RegisterItemVisuals(int item) { cg_items[item].icon = 100 + item; }
static int lastClient = -1, lastHealth = -1;
void CG_OverheadHealthColor(int client, int value, vec4_t color) {
    lastClient = client; lastHealth = value; color[0] = 1; color[1] = .5f; color[2] = 0; color[3] = 1;
}
static vec4_t tint;
static int tinted, pics;
static struct { float x, y, w, h; qhandle_t shader; } drawn[4];
void trap_R_SetColor(const float *rgba) { if (rgba) { memcpy(tint, rgba, sizeof(tint)); tinted = 1; } else tinted = 0; }
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    CHECK(pics < 4);
    drawn[pics].x = x; drawn[pics].y = y; drawn[pics].w = w; drawn[pics].h = h; drawn[pics++].shader = shader;
}
static qboolean Tint(float r, float g, float b, float a) { return tint[0] == r && tint[1] == g && tint[2] == b && tint[3] == a; }

int main(void) {
    const vec4_t color = { .3f, .3f, .3f, .6f };
    cg.snap = &snapshot;
    /* Armor tiers: green, yellow, red at the item's alpha. */
    snapshot.ps.stats[STAT_ARMOR_TIER] = 0; pics = 0;
    CG_DrawArmorTierColorized(1, 2, 3, 4, color, 9);
    CHECK(pics == 1 && drawn[0].shader == 9 && drawn[0].w == 3 && Tint(0, 1, 0, .6f) && !tinted);
    snapshot.ps.stats[STAT_ARMOR_TIER] = 1; CG_DrawArmorTierColorized(1, 2, 3, 4, color, 9); CHECK(Tint(1, 1, 0, .6f));
    snapshot.ps.stats[STAT_ARMOR_TIER] = 2; CG_DrawArmorTierColorized(1, 2, 3, 4, color, 9); CHECK(Tint(1, 0, 0, .6f));
    snapshot.ps.stats[STAT_ARMOR_TIER] = 7; CG_DrawArmorTierColorized(1, 2, 3, 4, color, 9); CHECK(Tint(.3f, .3f, .3f, .6f));

    /* Health: the followed player's, zero while its entity is dead. */
    snapshot.ps.clientNum = 4; snapshot.ps.stats[STAT_HEALTH] = 87; cg_entities[4].currentState.clientNum = 4;
    pics = 0; CG_DrawHealthColorized(5, 6, 7, 8, color, 11);
    CHECK(pics == 1 && drawn[0].shader == 11 && lastClient == 4 && lastHealth == 87 && Tint(1, .5f, 0, .6f));
    cg_entities[4].currentState.eFlags = EF_DEAD; CG_DrawHealthColorized(5, 6, 7, 8, color, 11); CHECK(lastHealth == 0);
    snapshot.ps.clientNum = -1; pics = 0; CG_DrawHealthColorized(5, 6, 7, 8, color, 11); CHECK(!pics);
    snapshot.ps.clientNum = 4;

    /* Keys: held ones only, half a width apart. */
    snapshot.ps.stats[STAT_KEY_MASK] = KEY_SILVER | KEY_MASTER; pics = 0;
    CG_DrawHeldKeys(10, 20, 16, 16);
    CHECK(pics == 2 && drawn[0].x == 10 && drawn[0].shader == 101 && drawn[1].x == 18 && drawn[1].shader == 103);
    snapshot.ps.stats[STAT_KEY_MASK] = KEY_SILVER | KEY_GOLD | KEY_MASTER; pics = 0;
    CG_DrawHeldKeys(10, 20, 16, 16); CHECK(pics == 3 && drawn[2].x == 26 && drawn[1].shader == 102);
    snapshot.ps.stats[STAT_KEY_MASK] = 0; pics = 0; CG_DrawHeldKeys(10, 20, 16, 16); CHECK(!pics);
    cg.snap = NULL; CG_DrawHeldKeys(10, 20, 16, 16); CG_DrawHealthColorized(0, 0, 1, 1, color, 1);
    CG_DrawArmorTierColorized(0, 0, 1, 1, color, 1); CHECK(!pics);

    puts("status icons: armor tier and health tints, held keys OK");
    return 0;
}
