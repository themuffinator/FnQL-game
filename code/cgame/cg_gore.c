/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Gore DLC paths. Observed in retail cgame:
 *
 * - Media (0x10022ffc, 0x10020ea8) register only when FS_GetFileList finds
 *   "dlc_gibs": viewBloodBlend, bloodSpray1-4, fifteen gib models (skull,
 *   brain, chest, abdomen, intestine, arm x2, forearm x2, fist x2, leg x2,
 *   foot x2), dlc_gibs/juicer.md3, and the gibsplt1, gibimp1-3 and
 *   invul_juiced sounds. Without them every path below falls back to what
 *   retail does without the DLC.
 * - Gibbing (0x10014c80) launches the fifteen models (the skull only half
 *   the time) from the body origin at (crandom, crandom, crandom + 1) * 250
 *   (0x10014500): fragments with gravity, a 5-8 s life, bounce 0.6, blood
 *   bounce sounds and blood marks. Without the DLC: electro gibs.
 * - EV_GIB_PLAYER plays gibsplt1 instead of an electro gib sound
 *   (0x1001bf87); EV_JUICED draws the juicer with invul_juiced
 *   (0x10013920) and otherwise gibs with electro gibs (0x1001bfcf).
 * - A blood fragment's bounce plays gibimp1, 2 or 3 (rand & 3: 0, 1, else;
 *   0x1001ec25) and marks with bloodSpray3 or 4, radius 16 + (rand & 31),
 *   random rotation (0x1001da41); its trail drops a random-spray puff every
 *   50 ms (radius 20, 500 ms, falling 40; 0x1001d6a0).
 * - Bleeding (0x10013da0) is a random-spray puff, radius 32, 500 ms,
 *   falling 40. Retail sets the rotation and own-client third-person flag
 *   on a separate, empty local entity it allocates first, so the visible
 *   puff has neither; FnQL-game draws the same puff without that entity.
 * viewBloodBlend is registered but unused, as in retail. */

#include "cg_local.h"

static const char *const goreGibNames[15] = {
    "skull", "brain", "chest", "abdomen", "intestine", "arm", "arm", "forearm", "forearm",
    "fist", "fist", "leg", "leg", "foot", "foot" };

void CG_RegisterGoreDLC(void) {
    char list[1024];
    int i;
    cgs.media.goreDLC = trap_FS_GetFileList("dlc_gibs", "", list, sizeof(list)) > 0;
    /* The Team Arena juicer assets are not in the retail paks; retail uses the
     * DLC ones or none. */
    cgs.media.invulnerabilityJuicedModel = 0;
    cgs.media.invulnerabilityJuicedSound = 0;
    if (!cgs.media.goreDLC) return;
    cgs.media.dlcViewBloodBlend = trap_R_RegisterShader("viewBloodBlend");
    for (i = 0; i < 4; ++i) cgs.media.dlcBloodSpray[i] = trap_R_RegisterShader(va("bloodSpray%d", i + 1));
    for (i = 0; i < 15; ++i) cgs.media.dlcGibModels[i] = trap_R_RegisterModel(va("dlc_gibs/%s.md3", goreGibNames[i]));
    cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel("dlc_gibs/juicer.md3");
    cgs.media.dlcGibSplatSound = trap_S_RegisterSound("dlc_gibs/gibsplt1.ogg", qfalse);
    for (i = 0; i < 3; ++i) cgs.media.dlcGibImpactSounds[i] = trap_S_RegisterSound(va("dlc_gibs/gibimp%d.ogg", i + 1), qfalse);
    cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound("dlc_gibs/invul_juiced.ogg", qfalse);
}

static float CG_GoreCrandom(void) {
    return (float)(((rand() & 0x7fff) / 32767.0 - 0.5) * 2.0);
}

/* Retail 0x10014500. */
static void CG_GoreLaunchGib(const vec3_t origin, const vec3_t velocity, qhandle_t model) {
    localEntity_t *le = CG_AllocLocalEntity();
    refEntity_t *re = &le->refEntity;
    le->leType = LE_FRAGMENT;
    le->startTime = cg.time;
    le->endTime = (int)((rand() & 0x7fff) / 32767.0 * 3000.0 + (le->startTime + 5000));
    VectorCopy(origin, re->origin);
    AxisCopy(axisDefault, re->axis);
    re->hModel = model;
    le->pos.trType = TR_GRAVITY;
    VectorCopy(origin, le->pos.trBase);
    VectorCopy(velocity, le->pos.trDelta);
    le->pos.trTime = cg.time;
    le->bounceFactor = 0.6f;
    le->leBounceSoundType = LEBS_BLOOD;
    le->leMarkType = LEMT_BLOOD;
}

qboolean CG_GoreGibPlayer(const vec3_t origin) {
    int i;
    if (!cgs.media.goreDLC || !cgs.media.dlcGibModels[0]) return qfalse;
    for (i = 0; i < 15; ++i) {
        vec3_t velocity;
        if (i == 0 && (rand() & 1)) continue;
        velocity[0] = (float)(CG_GoreCrandom() * 250.0);
        velocity[1] = (float)(CG_GoreCrandom() * 250.0);
        velocity[2] = (float)(CG_GoreCrandom() * 250.0 + 250.0);
        CG_GoreLaunchGib(origin, velocity, cgs.media.dlcGibModels[i]);
    }
    return qtrue;
}

static qhandle_t CG_GoreRandomSpray(void) {
    return cgs.media.dlcBloodSpray[rand() % 4];
}

static void CG_GorePuff(const vec3_t origin, float radius, int duration, int startTime) {
    localEntity_t *blood = CG_SmokePuff(origin, vec3_origin, radius, 1, 1, 1, 1, duration, startTime, 0, 0,
                                        CG_GoreRandomSpray());
    blood->leType = LE_FALL_SCALE_FADE;
    blood->pos.trDelta[2] = 40;
}

qboolean CG_GoreBleed(const vec3_t origin) {
    if (!cgs.media.goreDLC || !cgs.media.dlcBloodSpray[0]) return qfalse;
    CG_GorePuff(origin, 32, 500, cg.time);
    return qtrue;
}

qboolean CG_GoreBloodTrail(localEntity_t *le) {
    const int step = 50;
    int t, last;
    if (!cgs.media.goreDLC || !cgs.media.dlcBloodSpray[0]) return qfalse;
    t = step * ((cg.time - cg.frametime + step) / step);
    last = step * (cg.time / step);
    for (; t <= last; t += step) {
        vec3_t origin;
        BG_EvaluateTrajectory(&le->pos, t, origin);
        CG_GorePuff(origin, 20, 500, t);
    }
    return qtrue;
}

sfxHandle_t CG_GoreImpactSound(void) {
    const int r = rand() & 3;
    if (!cgs.media.goreDLC) return 0;
    return cgs.media.dlcGibImpactSounds[r == 0 ? 0 : r == 1 ? 1 : 2];
}

qboolean CG_GoreBloodMark(const trace_t *trace) {
    int radius;
    qhandle_t shader;
    if (!cgs.media.goreDLC || !cgs.media.dlcBloodSpray[0]) return qfalse;
    radius = 16 + (rand() & 31);
    shader = cgs.media.dlcBloodSpray[2 + rand() % 2];
    CG_ImpactMark(shader, trace->endpos, trace->plane.normal, (float)((rand() & 0x7fff) / 32767.0 * 360.0),
                  1, 1, 1, 1, qtrue, (float)radius, qfalse);
    return qtrue;
}
