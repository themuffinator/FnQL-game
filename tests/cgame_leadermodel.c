/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Leader player model (retail 0x10008c40, 0x1003d260, ownerdraws 85/37). */
#include "../code/cgame/cg_local.h"
#include "../docs/contract/retail_menudef.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
weaponInfo_t cg_weapons[MAX_WEAPONS];
vmCvar_t cg_loadout;
static int ents, lights, renders, clears;
static refEntity_t added[8];
static float lightOrigin[2][3], lightColor[2][3];
static refdef_t rendered;
static char tags[8][32];
static int tagCount;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void CG_AdjustFrom640(float *x, float *y, float *w, float *h) { *x *= 2; *y *= 2; *w *= 2; *h *= 2; }
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) {
    CHECK(model == 30); VectorSet(mins, -8, -8, -4); VectorSet(maxs, 8, 8, 10);
}
int trap_R_LerpTag(orientation_t *tag, clipHandle_t model, int start, int end, float frac, const char *name) {
    CHECK(start == end && frac == 1.0f);
    if (model == 10) { CHECK(start == 40 && !strcmp(name, "tag_torso")); tag->origin[2] = 20; }
    else { CHECK(model == 20 && start == 70 && !strcmp(name, "tag_head")); tag->origin[2] = 12; }
    return 1;
}
void trap_R_ClearScene(void) { ++clears; }
void trap_R_AddRefEntityToScene(const refEntity_t *re) { CHECK(ents < 8); added[ents++] = *re; }
void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
    const int i = lights++ % 2;
    CHECK(intensity == 500);
    VectorCopy(org, lightOrigin[i]); lightColor[i][0] = r; lightColor[i][1] = g; lightColor[i][2] = b;
}
void trap_R_RenderScene(const refdef_t *fd) { rendered = *fd; ++renders; }
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, const char *tagName) {
    (void)parent; (void)parentModel; snprintf(tags[tagCount++ % 8], 32, "%s", tagName); entity->origin[2] += 1;
}
static qboolean Near(double a, double b) { return a > b - 0.01 && a < b + 0.01; }

int main(void) {
    clientInfo_t *ci = &cgs.clientinfo[3];
    refdef_t view;
    vec3_t origin;
    const vec3_t angles = { 5, 160, 0 };
    /* Height scale: 56 / (20 + 24 + 12 + 10) * g_playerModelScale. */
    ci->legsModel = 10; ci->torsoModel = 20; ci->headModel = 30;
    ci->animations[LEGS_IDLE].firstFrame = 40; ci->animations[TORSO_STAND].firstFrame = 70;
    cgs.playerModelScale = 1.1f;
    CG_ComputeModelHeightScale(ci, "sarge");
    CHECK(Near(ci->modelHeightScale, 56.0 / 66.0 * 1.1f));
    CG_ComputeModelHeightScale(ci, "orbb/red"); CHECK(ci->modelHeightScale == 1.0f);
    /* View: 200x150 adjusted to 400x300; fov_x int(400/640*90) = 56. */
    CHECK(CG_LeaderModelView(10, 20, 200, 150, 0, &view, origin));
    CHECK(view.x == 20 && view.y == 40 && view.width == 400 && view.height == 300 && view.fov_x == 56);
    CHECK(view.rdflags == RDF_NOWORLDMODEL && view.viewaxis[0][0] == 1);
    CHECK(Near(view.fov_y, atan2(300.0, 400.0 / tan(56 / 360.0 * M_PI)) * 360.0 / M_PI));
    CHECK(Near(origin[0], 61.6 / tan(28 * M_PI / 180.0)) && origin[1] == 0 && origin[2] == -4);
    CHECK(CG_LeaderModelView(10, 20, 200, 150, 1.0f, &view, origin));
    CHECK(Near(origin[2], (32.0 / 0.85 - 24.0) * -0.5));
    /* Draw: legs, torso, head, gun and machinegun barrel, two lights. */
    ci->infoValid = qtrue; ci->legsSkin = 11; ci->torsoSkin = 21; ci->headSkin = 31;
    ci->animations[TORSO_ATTACK].firstFrame = 90; ci->animations[TORSO_ATTACK2].firstFrame = 95;
    cg_weapons[WP_MACHINEGUN].weaponModel = 50; cg_weapons[WP_MACHINEGUN].barrelModel = 51;
    cg.numScores = 1; cg.scores[0].client = 3; cg.scores[0].bestWeapon = WP_MACHINEGUN; cg.time = 777;
    CG_DrawLeaderModelOwnerDraw(CG_1STPLACE_PLYR_MODEL, 10, 20, 200, 150);
    CHECK(clears == 1 && renders == 1 && ents == 5 && lights == 2 && rendered.time == 777);
    CHECK(added[0].hModel == 10 && added[0].customSkin == 11 && added[0].frame == 40);
    CHECK(added[0].renderfx == (RF_LIGHTING_ORIGIN | RF_NOSHADOW) && added[0].shaderRGBA[1] == 0x80 && added[0].shaderRGBA[3] == 0xff);
    CHECK(added[0].shaderRGBA[0] == 0 && VectorCompare(added[0].origin, added[0].lightingOrigin));
    CHECK(added[1].hModel == 20 && added[1].frame == 90 && added[2].hModel == 30 && added[2].frame == 0);
    CHECK(added[3].hModel == 50 && added[4].hModel == 51 && added[3].shaderRGBA[3] == 0);
    CHECK(!strcmp(tags[0], "tag_torso") && !strcmp(tags[1], "tag_head") && !strcmp(tags[2], "tag_weapon") && !strcmp(tags[3], "tag_barrel"));
    CHECK(lightColor[0][1] == 1 && lightColor[1][0] == 1 && lightColor[1][1] == 0);
    CHECK(Near(lightOrigin[1][0], added[0].origin[0] - 200) && Near(lightOrigin[0][1], 100));
    /* Gauntlet leaders use TORSO_ATTACK2; rocket launchers carry no barrel. */
    ents = tagCount = lights = 0; cg.scores[0].bestWeapon = WP_ROCKET_LAUNCHER; cg_weapons[WP_ROCKET_LAUNCHER].weaponModel = 60;
    CG_DrawLeaderModelOwnerDraw(CG_1STPLACE_PLYR_MODEL, 10, 20, 200, 150); CHECK(ents == 4 && added[1].frame == 90);
    ents = 0; cg.scores[0].bestWeapon = WP_GAUNTLET;
    CG_DrawLeaderModelOwnerDraw(CG_1STPLACE_PLYR_MODEL, 10, 20, 200, 150); CHECK(ents == 3 && added[1].frame == 95);
    /* Player model: selected weapon, or the loadout primary. */
    ents = 0; cg.weaponSelect = WP_MACHINEGUN; CG_DrawLeaderModelOwnerDraw(CG_PLAYERMODEL, 10, 20, 200, 150);
    CHECK(ents == 5 && added[1].frame == 90 && added[3].hModel == 50);
    {
        static snapshot_t snap;
        snap.ps.weaponPrimary = WP_ROCKET_LAUNCHER; cg.snap = &snap; cg_loadout.integer = 1;
        ents = 0; CG_DrawLeaderModelOwnerDraw(CG_PLAYERMODEL, 10, 20, 200, 150); CHECK(ents == 4 && added[3].hModel == 60);
    }
    /* The grapple carries its hook on tag_ammo. */
    cg_weapons[WP_GRAPPLING_HOOK].weaponModel = 70; cg_weapons[WP_GRAPPLING_HOOK].attachedAmmoModel = 71;
    ents = tagCount = 0; cg.snap->ps.weaponPrimary = WP_GRAPPLING_HOOK; CG_DrawLeaderModelOwnerDraw(CG_PLAYERMODEL, 10, 20, 200, 150);
    CHECK(ents == 5 && added[3].hModel == 70 && added[4].hModel == 71 && !strcmp(tags[3], "tag_ammo"));
    /* No scores, or invalid models: nothing. */
    ents = renders = 0; cg.numScores = 0; CG_DrawLeaderModelOwnerDraw(CG_PLAYERMODEL, 10, 20, 200, 150); CHECK(!ents && !renders);
    cg.numScores = 1; ci->headModel = 0; CG_DrawLeaderModelOwnerDraw(CG_PLAYERMODEL, 10, 20, 200, 150); CHECK(!renders);
    puts("leader model: height scale, view, parts and weapons OK");
    return 0;
}
