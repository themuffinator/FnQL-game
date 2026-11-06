/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "cg_local.h"
#include <errno.h>
#include <stdint.h>

static qboolean CG_EffectPositionValid(const vec3_t position) {
    int i;
    for (i = 0; i < 3; ++i)
        if (!(position[i] >= -131072.0f && position[i] <= 131072.0f)) return qfalse;
    return qtrue;
}

/* Retail 100147d0: seven alternating brain/abdomen fragments. The models
 * supplied by QL are ice, independent of the inherited blood/gibs toggles. */
void CG_ThawPlayer(const vec3_t origin) {
    int i, axis;
    if (!CG_EffectPositionValid(origin)) return;
    for (i = 0; i < 7; ++i) {
        qhandle_t model = i & 1 ? cgs.media.gibAbdomen : cgs.media.gibBrain;
        localEntity_t *fragment;
        if (!model) continue;
        fragment = CG_AllocLocalEntity();
        fragment->leType = LE_FRAGMENT;
        fragment->startTime = cg.time;
        fragment->endTime = (int)((unsigned)cg.time + 5000u + (unsigned)(random() * 3000));
        fragment->refEntity.hModel = model;
        AxisClear(fragment->refEntity.axis);
        VectorCopy(origin,fragment->refEntity.origin);
        fragment->pos.trType = TR_GRAVITY;
        fragment->pos.trTime = cg.time;
        VectorCopy(origin,fragment->pos.trBase);
        for (axis = 0; axis < 3; ++axis) fragment->pos.trDelta[axis] = crandom() * 250;
        fragment->pos.trDelta[2] += 250;
        fragment->bounceFactor = 0.6f;
        fragment->leMarkType = LEMT_ICE;
        fragment->leBounceSoundType = LEBS_ICE;
    }
}

/* Electro gibs, the retail gib effect without the gore DLC (0x10014c80:
 * gore models and blood sprays are registered only when FS_GetFileList finds
 * dlc_gibs, 0x10022ffc). Ten spheres leave a ring around a point eight
 * units along the event's fixed (0, 0, -1) direction (CG_EntityEvent
 * prologue 0x10019ef0); each velocity is the ring offset plus 20 along that
 * direction, scaled per axis by 21 +/- 10.5. Fragments use retail
 * trajectory type 6 with gravity 425, bounce 0.35, a start time jittered by
 * +/-250 ms and a life of 375..1125 ms after it (0x10014360). The event
 * passes 1, which drops retail's +8 sphere and +26 flash height offsets. */
void CG_ElectroGibs(const vec3_t origin, qboolean event) {
    static const vec3_t direction = { 0, 0, -1 };
    /* 0x10014df5: the least-aligned axis is x, projected onto the plane
     * and crossed with the direction. */
    static const vec3_t right = { 1, 0, 0 }, up = { 0, -1, 0 };
    vec3_t base;
    localEntity_t *flash;
    int i, axis;
    if (!CG_EffectPositionValid(origin)) return;
    VectorCopy(origin, base);
    if (!event) base[2] += 8;
    for (i = 0; cgs.media.electroGibModel && i < 10; ++i) {
        float angle = i * 0.62831854820251465f;
        float radiusCos = crandom() * 2.75f + 5.5f;
        float c = cosf(angle) * radiusCos;
        float radiusSin = crandom() * 2.75f + 5.5f;
        float s = sinf(angle) * radiusSin;
        vec3_t velocity;
        localEntity_t *le;
        for (axis = 0; axis < 3; ++axis) {
            float offset = direction[axis] * 8 + right[axis] * c + up[axis] * s;
            velocity[axis] = offset + direction[axis] * 20;
        }
        for (axis = 0; axis < 3; ++axis) velocity[axis] *= crandom() * 10.5f + 21;
        le = CG_AllocLocalEntity();
        le->leType = LE_FRAGMENT;
        le->startTime = (int)(crandom() * 250 + cg.time);
        le->endTime = (int)(random() * 750 + (float)(le->startTime + 375));
        VectorCopy(origin, le->refEntity.origin);
        AxisClear(le->refEntity.axis);
        le->refEntity.hModel = cgs.media.electroGibModel;
        le->pos.trType = TR_QL_ACCEL;
        le->pos.gravity = 425;
        le->pos.trTime = cg.time;
        VectorCopy(origin, le->pos.trBase);
        VectorCopy(velocity, le->pos.trDelta);
        le->bounceFactor = 0.35f;
        le->leMarkType = LEMT_ELECTRO;
        le->leBounceSoundType = LEBS_ELECTRO;
    }
    /* 0x10013fe0: a 500 ms orange light (intensity 400). Retail also queues
     * a deathEffect sprite and 36 tracer particles, but never sets their life
     * rate, so they draw with zero color (inference); only the light is
     * reproduced. */
    flash = CG_AllocLocalEntity();
    flash->leType = LE_QL_GIB_FLASH;
    flash->startTime = cg.time;
    flash->endTime = cg.time + 500;
    flash->radius = 50;
    flash->light = 400;
    VectorSet(flash->lightColor, 1, .75f, 0);
    VectorCopy(origin, flash->refEntity.origin);
    if (!event) flash->refEntity.origin[2] += 26;
}

/* 0x1001bf7d: rand() & 4 selects only the first or fourth sound. */
sfxHandle_t CG_ElectroGibSound(void) {
    return cgs.media.electroGibSounds[(rand() & 4) ? 1 : 0];
}

/* 0x1001ec7a: the first bounce plays electrogib_bounce_01 or _04 at the
 * impact; later bounces are silent. */
void CG_ElectroFragmentBounce(localEntity_t *le, const trace_t *trace) {
    sfxHandle_t sound = cgs.media.electroBounceSounds[(rand() & 4) ? 1 : 0];
    if (sound) trap_S_StartSound(trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, sound);
    le->leBounceSoundType = LEBS_NONE;
}

/* 0x1001e316: one burn mark of radius 4..11 at a random rotation. */
void CG_ElectroFragmentMark(localEntity_t *le, const trace_t *trace) {
    int radius = (rand() & 7) + 4;
    if (cgs.media.burnMarkShader)
        CG_ImpactMark(cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random() * 360,
                      1, 1, 1, 1, qtrue, radius, qfalse);
    le->leMarkType = LEMT_NONE;
}

/* 0x1001f730 light half: full intensity for the first half of the life,
 * then linear to zero; a viewer inside the 50-unit radius removes it. */
void CG_AddGibFlash(localEntity_t *le) {
    vec3_t delta;
    float span = (float)(le->endTime - le->startTime), fraction, scale = 1;
    VectorSubtract(le->refEntity.origin, cg.refdef.vieworg, delta);
    if (VectorLength(delta) < le->radius) {
        CG_FreeLocalEntity(le);
        return;
    }
    fraction = span > 0 ? (cg.time - le->startTime) / span : 1;
    if (fraction >= .5f) scale = 1 - (fraction - .5f) * 2;
    if (scale > 0)
        trap_R_AddLightToScene(le->refEntity.origin, le->light * scale,
                               le->lightColor[0], le->lightColor[1], le->lightColor[2]);
}

/* Retail 1001d790: 150 ms trail samples, falling/scaling ice puffs. Limit
 * catch-up to the visible two-second lifetime after a hitch or demo seek. */
void CG_ThawTrail(const localEntity_t *fragment) {
    int64_t first, last, previous, t;
    trajectory_t trajectory;
    if (cg.frametime <= 0 || !cgs.media.iceTrailShader) return;
    trajectory = fragment->pos; /* Puff allocation may recycle an old fragment. */
    previous = (int64_t)cg.time - cg.frametime;
    if (previous < (int64_t)cg.time - 2000) previous = (int64_t)cg.time - 2000;
    first = (previous + 150) / 150 * 150;
    last = (int64_t)cg.time / 150 * 150;
    for (t = first; t <= last; t += 150) {
        vec3_t origin;
        localEntity_t *puff;
        BG_EvaluateTrajectory(&trajectory,(int)t,origin);
        if (!CG_EffectPositionValid(origin)) continue;
        puff = CG_SmokePuff(origin,vec3_origin,20,1,1,1,1,2000,(int)t,0,0,cgs.media.iceTrailShader);
        puff->leType = LE_FALL_SCALE_FADE;
        puff->pos.trDelta[2] = 40;
    }
}

/* Retail 1001e76d/10024af0: one alpha-fading ice mark, radius 16..47. */
void CG_IceFragmentMark(const trace_t *trace) {
    if (cgs.media.iceMarkShader)
        CG_ImpactMark(cgs.media.iceMarkShader,trace->endpos,trace->plane.normal,
                      random()*360,1,1,1,1,qtrue,16+(rand()&31),qfalse);
}

/* CS 0x2a5 is the through-surface probe distance (retail 10049ebd), despite
 * its inherited CS_DOM_CAPTURE_TIME name. Read at use so updates take effect.
 * Invalid/oversized native values disable the backside effect safely. */
static float CG_ThroughDistance(void) {
    const char *text = CG_ConfigString(CS_DOM_CAPTURE_TIME);
    char *end;
    double distance;
    errno = 0;
    distance = strtod(text,&end);
    if (end == text || *end || errno || !(distance >= -131072 && distance <= 131072)) return 0;
    return (float)distance;
}

void CG_MissileHitWallDmgThrough(int weapon, vec3_t origin, vec3_t direction) {
    vec3_t start, debrisOrigin;
    trace_t trace;
    float distance;
    int i, axis;
    if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ||
        !CG_EffectPositionValid(origin) || !CG_EffectPositionValid(direction)) return;
    distance = CG_ThroughDistance();
    VectorMA(origin,distance,direction,start);
    if (distance && CG_EffectPositionValid(start)) {
        /* Retail 10054e23: trace from beyond the hit back toward the impact.
         * Backside debris requires a hit outside solid (10054e31..e47). */
        CG_Trace(&trace,start,NULL,NULL,origin,-1,CONTENTS_SOLID);
        if (!trace.startsolid && !trace.allsolid && trace.fraction >= 0 && trace.fraction < 1 &&
            CG_EffectPositionValid(trace.endpos) && CG_EffectPositionValid(trace.plane.normal)) {
            if (cgs.media.burnMarkShader)
                CG_ImpactMark(cgs.media.burnMarkShader,trace.endpos,trace.plane.normal,
                              random()*360,1,1,1,1,qfalse,64,qfalse);
            VectorAdd(trace.endpos,trace.plane.normal,debrisOrigin);
            for (i = 0; i < 10 && cgs.media.surfacePuffShader; ++i) {
                vec3_t velocity;
                float speed = 250 + crandom()*150;
                for (axis = 0; axis < 3; ++axis)
                    velocity[axis] = trace.plane.normal[axis]*speed + 50 - crandom()*50;
                CG_SmokePuff(debrisOrigin,velocity,24,.8f,.8f,.7f,1,
                             400-speed*.4f,cg.time,0,0,cgs.media.surfacePuffShader);
            }
        }
    }
    /* All cases retain the ordinary explosion, sound and front impact mark. */
    CG_MissileHitWall(weapon,0,origin,direction,IMPACTSOUND_DEFAULT);
}
