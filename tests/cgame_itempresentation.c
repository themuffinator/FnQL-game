/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail world item placement: cg_itemFx bits, simple items, flag style. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define NEAR(a, b) (fabs((double)(a) - (double)(b)) < 1e-4)

static gitem_t health = { "item_health", NULL, { NULL }, NULL, NULL, 25, IT_HEALTH, 0, 0, { 0, 0 }, "", "" };
static gitem_t mega = { "item_health_mega", NULL, { NULL }, NULL, NULL, 100, IT_HEALTH, 0, 1, { 0, 0 }, "", "" };
static gitem_t rail = { "weapon_railgun", NULL, { NULL }, NULL, NULL, 10, IT_WEAPON, WP_RAILGUN, 0, { 0, 0 }, "", "" };
static gitem_t scout = { "item_scout", NULL, { NULL }, NULL, NULL, 30, IT_PERSISTANT_POWERUP, 1, 0, { 0, 0 }, "", "" };
static gitem_t redFlag = { "team_CTF_redflag", NULL, { "a.md3", NULL, NULL, NULL, "b.md3" }, NULL, NULL, 0, IT_TEAM,
    PW_REDFLAG, 0, { 64, 2048 }, "", "" };
static gitem_t quad = { "item_quad", NULL, { "q.md3", NULL, NULL, NULL, "x.md3" }, NULL, NULL, 30, IT_POWERUP,
    PW_QUAD, 1, { 0, 0 }, "", "" };

int main(void) {
    qboolean bobs;
    int fx;

    /* The row mask hides an item by gametype: the flags in One Flag only. */
    CHECK(CG_ItemHiddenInGametype(&redFlag, GT_1FCTF) && !CG_ItemHiddenInGametype(&redFlag, GT_CTF));
    CHECK(!CG_ItemHiddenInGametype(&health, GT_1FCTF) && !CG_ItemHiddenInGametype(NULL, GT_CTF));
    CHECK(!CG_ItemHiddenInGametype(&redFlag, -1) && !CG_ItemHiddenInGametype(&redFlag, 64));

    /* Bit 1: 4 + 6 sin over a shared 1024 ms cycle; nothing without it. */
    CHECK(CG_ItemBobOffset(0, 256) == 0 && CG_ItemBobOffset(6, 256) == 0);
    CHECK(NEAR(CG_ItemBobOffset(1, 0), 4) && NEAR(CG_ItemBobOffset(7, 256), 10));
    CHECK(NEAR(CG_ItemBobOffset(1, 768), -2) && NEAR(CG_ItemBobOffset(1, 512), 4));
    CHECK(NEAR(CG_ItemBobOffset(1, 1024 * 37 + 256), 10));
    CHECK(NEAR(CG_ItemBobOffset(1, 128), 4 + 6 * sin(M_PI / 4)));

    /* Bit 2: health turns fast, the rest slowly, and nothing turns without it. */
    for (fx = 0; fx < 8; fx++) {
        CHECK(CG_ItemRotation(fx, &health) == ((fx & 2) ? 2 : 0));
        CHECK(CG_ItemRotation(fx, &rail) == ((fx & 2) ? 1 : 0));
    }
    CHECK(CG_ItemRotation(2, NULL) == 1);

    /* Bit 4: a second of scale-up after a respawn. */
    CHECK(CG_ItemSpawnScale(3, 500) == 1 && CG_ItemSpawnScale(4, -1) == 1 && CG_ItemSpawnScale(4, 1000) == 1);
    CHECK(CG_ItemSpawnScale(4, 0) == 0 && NEAR(CG_ItemSpawnScale(7, 250), 0.25) && NEAR(CG_ItemSpawnScale(4, 999), 0.999));

    /* Simple items: the height offset, ten for a persistant powerup ... */
    CHECK(CG_SimpleItemLift(&health, 0, 8, 256, &bobs) == 8 && !bobs);
    CHECK(CG_SimpleItemLift(&scout, 0, 8, 256, &bobs) == 10 && !bobs);
    CHECK(CG_SimpleItemLift(&health, 2, 18, 256, &bobs) == 18 && !bobs);     /* mode 2 leaves untimed items still */
    /* ... or, bobbing, ten plus the sine, whatever the offset. */
    CHECK(NEAR(CG_SimpleItemLift(&health, 1, 8, 256, &bobs), 16) && bobs);
    CHECK(NEAR(CG_SimpleItemLift(&mega, 2, 0, 768, &bobs), 4) && bobs);
    CHECK(NEAR(CG_SimpleItemLift(&quad, 2, 18, 0, NULL), 10));
    CHECK(NEAR(CG_SimpleItemLift(&scout, 1, 8, 512, &bobs), 10) && bobs);
    CHECK(CG_SimpleItemLift(NULL, 2, 5, 0, &bobs) == 5 && !bobs);

    /* Flag style 2 picks the fifth model, for the three flags only. */
    CHECK(!CG_ItemFlagStyleModel(&redFlag, 0) && !CG_ItemFlagStyleModel(&redFlag, 1));
    CHECK(!strcmp(CG_ItemFlagStyleModel(&redFlag, 2), "b.md3"));
    CHECK(!CG_ItemFlagStyleModel(&quad, 2) && !CG_ItemFlagStyleModel(NULL, 2));

    puts("cgame item presentation OK");
    return 0;
}
