/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* The leader's player model in a menu rect. Observed in retail cgame:
 *
 * - Per-model height scale (0x1003d260, after a model loads and when the
 *   appearance settings change): 56 / (legs tag_torso z at the LEGS_IDLE
 *   first frame + 24 + torso tag_head z at the TORSO_STAND first frame +
 *   head maxs z) * g_playerModelScale; Orbb models use 1. The client-info
 *   copy carries it (0x1003de5a).
 * - CG_1STPLACE_PLYR_MODEL (85, 0x10034900) and CG_PLAYERMODEL (37,
 *   0x10034980) both draw scores[0].client through 0x10008c40: 85 with that
 *   row's best weapon, TORSO_ATTACK2 for no weapon or the gauntlet and
 *   TORSO_ATTACK otherwise, yawed 160; 37 with TORSO_ATTACK, yawed 210, and
 *   the selected weapon, or the snapshot's primary choice with cg_loadout.
 *   Both pitch 5.
 * - 0x10008c40 needs valid info and legs, torso and head models. The rect
 *   goes through the 640 adjust; fov_x is int(width / 640 * 90), fov_y the
 *   matching vertical angle. The model stands 1.1 * (s + 24) / tan(fov_x / 2)
 *   ahead at height (s - 24) * -0.5, s being 32, or 32 / (scale * 0.85) with
 *   a height scale. Legs (LEGS_IDLE first frame, yawed), torso (first frame
 *   of its animation) on tag_torso and head on tag_head use no shadow, the
 *   lighting origin and colour 0 128 0 255; the weapon goes on tag_weapon,
 *   with a barrel on tag_barrel for the gauntlet, machinegun and BFG. A white
 *   and a red light (500) sit 100 behind-left-above and 200 behind; the
 *   scene has no world.
 * Weapon 10 (the grapple) also carries its "_ammo" hook model on tag_ammo. */

#include "cg_local.h"
#include "../../docs/contract/retail_menudef.h"

void CG_ComputeModelHeightScale(clientInfo_t *ci, const char *modelName) {
    vec3_t mins, maxs;
    orientation_t tag;
    float height;
    if (!ci) return;
    if (modelName && !Q_stricmpn(modelName, "orbb", 4)) {
        ci->modelHeightScale = 1.0f;
        return;
    }
    trap_R_ModelBounds(ci->headModel, mins, maxs);
    memset(&tag, 0, sizeof(tag));
    trap_R_LerpTag(&tag, ci->legsModel, ci->animations[LEGS_IDLE].firstFrame,
                   ci->animations[LEGS_IDLE].firstFrame, 1.0f, "tag_torso");
    height = tag.origin[2] + 24.0f;
    memset(&tag, 0, sizeof(tag));
    trap_R_LerpTag(&tag, ci->torsoModel, ci->animations[TORSO_STAND].firstFrame,
                   ci->animations[TORSO_STAND].firstFrame, 1.0f, "tag_head");
    height = maxs[2] + tag.origin[2] + height;
    /* Retail divides unguarded; a degenerate model keeps the unscaled view. */
    ci->modelHeightScale = height > 0 ? (float)(56.0 / height * cgs.playerModelScale) : 0.0f;
}

void CG_RefreshModelHeightScales(void) {
    int i;
    for (i = 0; i < cgs.maxclients && i < MAX_CLIENTS; ++i) {
        clientInfo_t *ci = &cgs.clientinfo[i];
        if (ci->infoValid) CG_ComputeModelHeightScale(ci, ci->modelName);
    }
}

static void CG_LeaderModelPart(refEntity_t *part, qhandle_t model, qhandle_t skin, const vec3_t origin) {
    memset(part, 0, sizeof(*part));
    part->hModel = model;
    part->customSkin = skin;
    part->renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
    VectorCopy(origin, part->origin);
    VectorCopy(origin, part->lightingOrigin);
    AxisClear(part->axis);
}

static void CG_LeaderModelColor(refEntity_t *part) {
    part->shaderRGBA[0] = 0;
    part->shaderRGBA[1] = 0x80;
    part->shaderRGBA[2] = 0;
    part->shaderRGBA[3] = 0xff;
}

qboolean CG_LeaderModelView(float x, float y, float w, float h, float heightScale, refdef_t *refdef, vec3_t origin) {
    float scale = 32.0f, length, across;
    CG_AdjustFrom640(&x, &y, &w, &h);
    memset(refdef, 0, sizeof(*refdef));
    refdef->x = (int)x;
    refdef->y = (int)y;
    refdef->width = (int)w;
    refdef->height = (int)h;
    if (refdef->width <= 0 || refdef->height <= 0) return qfalse;
    refdef->rdflags = RDF_NOWORLDMODEL;
    AxisClear(refdef->viewaxis);
    refdef->fov_x = (float)(int)(refdef->width / 640.0 * 90.0);
    across = (float)(refdef->width / tan(refdef->fov_x / 360.0 * 3.1415927410125732));
    refdef->fov_y = (float)(atan2((double)refdef->height, (double)across) * 114.591552734375);
    if (heightScale != 0) scale = (float)(32.0 / (heightScale * 0.85));
    length = (float)((scale - -24.0) * 1.100000023841858);
    origin[0] = (float)(length / tan(refdef->fov_x * 3.1415927410125732 / 180.0 * 0.5));
    origin[1] = 0;
    origin[2] = (float)((scale - 24.0) * -0.5);
    refdef->time = cg.time;
    return qtrue;
}

void CG_DrawLeaderModel(float x, float y, float w, float h, int torsoAnim, int weapon, const vec3_t angles) {
    static const vec3_t still = { 0, 0, 0 };
    /* Retail reads row 0 directly; its score parser first resets that row's
     * client to -1 (0x10045475), which the row count stands in for here. */
    const int client = cg.numScores > 0 ? cg.scores[0].client : -1;
    clientInfo_t *ci;
    refdef_t refdef;
    refEntity_t legs, torso, head, gun, barrel;
    vec3_t origin, light;
    if (client < 0 || client >= MAX_CLIENTS || torsoAnim < 0 || torsoAnim >= MAX_TOTALANIMATIONS) return;
    ci = &cgs.clientinfo[client];
    if (!ci->infoValid || !ci->legsModel || !ci->torsoModel || !ci->headModel) return;
    if (!CG_LeaderModelView(x, y, w, h, ci->modelHeightScale, &refdef, origin)) return;
    trap_R_ClearScene();

    CG_LeaderModelPart(&legs, ci->legsModel, ci->legsSkin, origin);
    legs.frame = ci->animations[LEGS_IDLE].firstFrame;
    AnglesToAxis(angles, legs.axis);
    CG_LeaderModelColor(&legs);
    trap_R_AddRefEntityToScene(&legs);

    CG_LeaderModelPart(&torso, ci->torsoModel, ci->torsoSkin, origin);
    torso.frame = ci->animations[torsoAnim].firstFrame;
    AnglesToAxis(still, torso.axis);
    CG_LeaderModelColor(&torso);
    CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");
    trap_R_AddRefEntityToScene(&torso);

    CG_LeaderModelPart(&head, ci->headModel, ci->headSkin, origin);
    CG_LeaderModelColor(&head);
    CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");
    trap_R_AddRefEntityToScene(&head);

    if (weapon > WP_NONE && weapon < MAX_WEAPONS && cg_weapons[weapon].weaponModel) {
        CG_LeaderModelPart(&gun, cg_weapons[weapon].weaponModel, 0, origin);
        CG_PositionRotatedEntityOnTag(&gun, &torso, ci->torsoModel, "tag_weapon");
        trap_R_AddRefEntityToScene(&gun);
        if ((weapon == WP_MACHINEGUN || weapon == WP_GAUNTLET || weapon == WP_BFG) && cg_weapons[weapon].barrelModel) {
            CG_LeaderModelPart(&barrel, cg_weapons[weapon].barrelModel, 0, origin);
            CG_PositionRotatedEntityOnTag(&barrel, &gun, cg_weapons[weapon].weaponModel, "tag_barrel");
            trap_R_AddRefEntityToScene(&barrel);
        }
        if (weapon == WP_GRAPPLING_HOOK && cg_weapons[weapon].attachedAmmoModel) {
            CG_LeaderModelPart(&barrel, cg_weapons[weapon].attachedAmmoModel, 0, origin);
            CG_PositionRotatedEntityOnTag(&barrel, &gun, cg_weapons[weapon].weaponModel, "tag_ammo");
            trap_R_AddRefEntityToScene(&barrel);
        }
    }

    VectorCopy(origin, light);
    light[0] -= 100; light[1] += 100; light[2] += 100;
    trap_R_AddLightToScene(light, 500, 1, 1, 1);
    light[0] -= 100; light[1] -= 100; light[2] -= 100;
    trap_R_AddLightToScene(light, 500, 1, 0, 0);
    trap_R_RenderScene(&refdef);
}

/* Ownerdraws 85 and 37 (0x10034900, 0x10034980). */
void CG_DrawLeaderModelOwnerDraw(int ownerDraw, float x, float y, float w, float h) {
    if (ownerDraw == CG_1STPLACE_PLYR_MODEL) {
        const vec3_t angles = { 5, 160, 0 };
        const int weapon = cg.numScores > 0 ? cg.scores[0].bestWeapon : WP_NONE;
        CG_DrawLeaderModel(x, y, w, h, weapon == WP_NONE || weapon == WP_GAUNTLET ? TORSO_ATTACK2 : TORSO_ATTACK,
                           weapon, angles);
    } else if (ownerDraw == CG_PLAYERMODEL) {
        const vec3_t angles = { 5, 210, 0 };
        int weapon = cg.weaponSelect;
        if (cg_loadout.integer && cg.snap) weapon = cg.snap->ps.weaponPrimary;   /* snapshot +0xc0 */
        CG_DrawLeaderModel(x, y, w, h, TORSO_ATTACK, weapon, angles);
    }
}
