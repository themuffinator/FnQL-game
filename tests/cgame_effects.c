/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static localEntity_t entities[128];
static int allocated, puffs, marks, explosions, traces, evaluated;
static int lastSample;
static trace_t result;
static const char *distanceText = "-33.1";
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) { CHECK(index == 0x2a5); return distanceText; }
localEntity_t *CG_AllocLocalEntity(void) {
    CHECK(allocated < ARRAY_LEN(entities));
    memset(&entities[allocated],0,sizeof(entities[allocated]));
    return &entities[allocated++];
}
void BG_EvaluateTrajectory(const trajectory_t *trajectory, int at, vec3_t position) {
    CHECK(trajectory->trType == TR_GRAVITY);
    VectorCopy(trajectory->trBase,position); lastSample = at; ++evaluated;
}
void CG_Trace(trace_t *out, const vec3_t start, const vec3_t mins, const vec3_t maxs,
              const vec3_t end, int skip, int mask) {
    CHECK(!mins && !maxs && skip == -1 && mask == CONTENTS_SOLID);
    CHECK(fabsf(start[2] - 30.9f) < .001f && end[2] == 64 && start[0] == 32);
    ++traces; *out = result;
}
void CG_MissileHitWall(weapon_t weapon, int client, vec3_t origin, vec3_t direction, impactSound_t sound) {
    CHECK(weapon == WP_ROCKET_LAUNCHER && client == 0 && origin[2] == 64);
    CHECK(direction[2] == 1 && sound == IMPACTSOUND_DEFAULT); ++explosions;
}
static int electroMarks, sounds, lastSound, lights, freed;
static float lastLight;
static vec3_t soundOrigin;
void trap_S_StartSound(const vec3_t origin, int entity, int channel, sfxHandle_t sound) {
    CHECK(origin && entity == ENTITYNUM_WORLD && channel == CHAN_AUTO);
    VectorCopy(origin, soundOrigin); lastSound = sound; ++sounds;
}
void trap_R_AddLightToScene(const vec3_t origin, float intensity, float r, float g, float b) {
    CHECK(origin[2] == 64 && r == 1 && g == .75f && b == 0); lastLight = intensity; ++lights;
}
void CG_FreeLocalEntity(localEntity_t *le) { le->leType = -1; ++freed; }
void CG_ImpactMark(qhandle_t shader, const vec3_t origin, const vec3_t direction,
                   float angle, float r, float g, float b, float a, qboolean alpha,
                   float radius, qboolean temporary) {
    if (shader == 25) {
        CHECK(!temporary && alpha && radius >= 4 && radius <= 11 && angle >= 0 && angle <= 360);
        CHECK(r == 1 && g == 1 && b == 1 && a == 1 && origin[2] == 5);
        ++electroMarks; return;
    }
    CHECK(!temporary && angle >= 0 && angle <= 360 && r == 1 && g == 1 && b == 1 && a == 1);
    CHECK(origin[2] == 48 && direction[2] == -1);
    if (shader == 21) CHECK(alpha && radius >= 16 && radius <= 47);
    else CHECK(shader == 23 && !alpha && radius == 64);
    ++marks;
}
localEntity_t *CG_SmokePuff(const vec3_t origin, const vec3_t velocity, float radius,
                           float r, float g, float b, float a, float duration,
                           int start, int fade, int flags, qhandle_t shader) {
    CHECK(!fade && !flags && a == 1);
    if (shader == 20) {
        CHECK(radius == 20 && r == 1 && g == 1 && b == 1 && duration == 2000);
        CHECK(start == lastSample && origin[2] == 64 && !VectorLengthSquared(velocity));
    } else {
        CHECK(shader == 22 && radius == 24 && r == .8f && g == .8f && b == .7f);
        CHECK(duration >= 240 && duration <= 360 && start == cg.time && origin[2] == 47);
        CHECK(velocity[0] >= 0 && velocity[0] <= 100 && velocity[1] >= 0 && velocity[1] <= 100);
        CHECK(velocity[2] >= -400 && velocity[2] <= 0);
    }
    ++puffs; return CG_AllocLocalEntity();
}
int main(void) {
    vec3_t origin = {32,48,64}, normal = {0,0,1};
    int i, count;
    localEntity_t fragment;
    cgs.media.gibBrain = 10; cgs.media.gibAbdomen = 11;
    cgs.media.iceTrailShader = 20; cgs.media.iceMarkShader = 21;
    cgs.media.surfacePuffShader = 22; cgs.media.burnMarkShader = 23;
    cg.time = 1000;
    CG_ThawPlayer(origin); CHECK(allocated == 7);
    for (i = 0; i < 7; ++i) {
        localEntity_t *e = &entities[i];
        CHECK(e->leType == LE_FRAGMENT && e->refEntity.hModel == (i & 1 ? 11 : 10));
        CHECK(e->startTime == 1000 && e->endTime >= 6000 && e->endTime <= 9000);
        CHECK(e->pos.trType == TR_GRAVITY && e->pos.trTime == 1000);
        CHECK(!memcmp(e->pos.trBase,origin,sizeof(origin)) && !memcmp(e->refEntity.origin,origin,sizeof(origin)));
        CHECK(e->refEntity.axis[0][0] == 1 && e->refEntity.axis[1][1] == 1 && e->refEntity.axis[2][2] == 1);
        CHECK(e->pos.trDelta[0] >= -250 && e->pos.trDelta[0] <= 250);
        CHECK(e->pos.trDelta[1] >= -250 && e->pos.trDelta[1] <= 250);
        CHECK(e->pos.trDelta[2] >= 0 && e->pos.trDelta[2] <= 500);
        CHECK(e->bounceFactor == .6f && e->leMarkType == LEMT_ICE && e->leBounceSoundType == LEBS_ICE);
    }
    fragment = entities[0];
    cgs.media.gibBrain = 0; CG_ThawPlayer(origin); CHECK(allocated == 10);
    cgs.media.gibAbdomen = 0; CG_ThawPlayer(origin); CHECK(allocated == 10);
    origin[0] = NAN; CG_ThawPlayer(origin); CHECK(allocated == 10); origin[0] = 32;
    cg.time = 1200; cg.frametime = 320; CG_ThawTrail(&fragment);
    CHECK(evaluated == 3 && puffs == 3 && lastSample == 1200);
    for (i = 10; i < 13; ++i) CHECK(entities[i].leType == LE_FALL_SCALE_FADE && entities[i].pos.trDelta[2] == 40);
    cg.frametime = -10; CG_ThawTrail(&fragment); CHECK(puffs == 3);
    cg.time = 100000; cg.frametime = INT_MAX; CG_ThawTrail(&fragment); CHECK(puffs <= 17);
    result.fraction = .5f; VectorSet(result.endpos,32,48,48); VectorSet(result.plane.normal,0,0,-1);
    CG_IceFragmentMark(&result); CHECK(marks == 1);
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
    CHECK(traces == 1 && marks == 2 && explosions == 1); count = puffs;
    CHECK(puffs >= 26 && puffs <= 27);
    result.startsolid = qtrue; CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
    CHECK(puffs == count && marks == 2 && explosions == 2);
    result.startsolid = qfalse; result.fraction = 1;
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal); CHECK(puffs == count && explosions == 3);
    { const char *invalid[] = {"", "nan", "inf", "-1e99", "garbage", "-33junk", "0"};
      for (i = 0; i < ARRAY_LEN(invalid); ++i) {
        distanceText = invalid[i]; CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
      }
      CHECK(traces == 3 && explosions == 10 && puffs == count);
    }
    CG_MissileHitWallDmgThrough(WP_NUM_WEAPONS,origin,normal); CHECK(explosions == 10);
    distanceText = "-33.1"; result.fraction = .5f; result.allsolid = qtrue;
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
    CHECK(puffs == count && explosions == 11);
    result.allsolid = qfalse; cgs.media.burnMarkShader = cgs.media.surfacePuffShader = 0;
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
    CHECK(marks == 2 && puffs == count && explosions == 12);
    result.endpos[0] = NAN;
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal);
    CHECK(marks == 2 && puffs == count && explosions == 13);
    origin[0] = INFINITY;
    CG_MissileHitWallDmgThrough(WP_ROCKET_LAUNCHER,origin,normal); CHECK(explosions == 13);
    /* Electro gibs (retail 0x10014c80 without the gore DLC). */
    {
        vec3_t body = { 32, 48, 64 };
        int k, seen[2] = { 0, 0 };
        trace_t hit;
        memset(&hit, 0, sizeof(hit)); hit.endpos[2] = 5;
        cgs.media.electroGibModel = 30;
        cgs.media.electroGibSounds[0] = 40; cgs.media.electroGibSounds[1] = 41;
        cgs.media.electroBounceSounds[0] = 42; cgs.media.electroBounceSounds[1] = 43;
        cgs.media.burnMarkShader = 25;
        allocated = 0; cg.time = 10000;
        CG_ElectroGibs(body, qtrue);
        CHECK(allocated == 11);
        for (k = 0; k < 10; ++k) {
            localEntity_t *e = &entities[k];
            float angle = k * 0.62831854820251465f;
            CHECK(e->leType == LE_FRAGMENT && e->refEntity.hModel == 30);
            CHECK(e->pos.trType == TR_QL_ACCEL && e->pos.gravity == 425 && e->bounceFactor == .35f);
            CHECK(e->leMarkType == LEMT_ELECTRO && e->leBounceSoundType == LEBS_ELECTRO);
            CHECK(e->pos.trTime == 10000 && e->startTime >= 9750 && e->startTime <= 10250);
            CHECK(e->endTime - e->startTime >= 375 && e->endTime - e->startTime <= 1125);
            CHECK(!memcmp(e->pos.trBase, body, sizeof(vec3_t)) && !memcmp(e->refEntity.origin, body, sizeof(vec3_t)));
            CHECK(e->refEntity.axis[0][0] == 1 && e->refEntity.axis[1][1] == 1 && e->refEntity.axis[2][2] == 1);
            /* (cos, -sin) ring offsets, -28 vertical, per-axis 21 +/- 10.5 scale */
            CHECK(e->pos.trDelta[2] <= -28 * 10.5f + .01f && e->pos.trDelta[2] >= -28 * 31.5f - .01f);
            CHECK(fabsf(e->pos.trDelta[0]) <= 8.25f * 31.5f + .01f && fabsf(e->pos.trDelta[1]) <= 8.25f * 31.5f + .01f);
            if (cosf(angle) > .1f) CHECK(e->pos.trDelta[0] > 0);
            if (cosf(angle) < -.1f) CHECK(e->pos.trDelta[0] < 0);
            if (sinf(angle) > .1f) CHECK(e->pos.trDelta[1] < 0);
            if (sinf(angle) < -.1f) CHECK(e->pos.trDelta[1] > 0);
        }
        CHECK(entities[0].pos.trDelta[1] == 0);
        CHECK(entities[10].leType == LE_QL_GIB_FLASH && entities[10].startTime == 10000);
        CHECK(entities[10].endTime == 10500 && entities[10].radius == 50 && entities[10].light == 400);
        CHECK(entities[10].refEntity.origin[2] == 64 && entities[10].lightColor[1] == .75f);
        allocated = 0; CG_ElectroGibs(body, qfalse);
        CHECK(allocated == 11 && entities[10].refEntity.origin[2] == 90 && entities[0].refEntity.origin[2] == 64);
        allocated = 0; cgs.media.electroGibModel = 0; CG_ElectroGibs(body, qtrue);
        CHECK(allocated == 1 && entities[0].leType == LE_QL_GIB_FLASH);
        allocated = 0; body[1] = NAN; CG_ElectroGibs(body, qtrue); CHECK(allocated == 0);
        body[1] = 48;
        /* Sound choice: only the first and fourth variants (rand() & 4). */
        for (k = 0; k < 200; ++k) {
            sfxHandle_t sfx = CG_ElectroGibSound();
            CHECK(sfx == 40 || sfx == 41); ++seen[sfx - 40];
        }
        CHECK(seen[0] && seen[1]);
        /* First bounce only, at the impact; one electro mark. */
        entities[0].leBounceSoundType = LEBS_ELECTRO; sounds = 0;
        CG_ElectroFragmentBounce(&entities[0], &hit);
        CHECK(sounds == 1 && (lastSound == 42 || lastSound == 43) && soundOrigin[2] == 5);
        CHECK(entities[0].leBounceSoundType == LEBS_NONE);
        entities[0].leMarkType = LEMT_ELECTRO;
        CG_ElectroFragmentMark(&entities[0], &hit); CHECK(electroMarks == 1 && entities[0].leMarkType == LEMT_NONE);
        cgs.media.burnMarkShader = 0; entities[0].leMarkType = LEMT_ELECTRO;
        CG_ElectroFragmentMark(&entities[0], &hit); CHECK(electroMarks == 1 && entities[0].leMarkType == LEMT_NONE);
        /* Flash light: full for half the life, then linear; removed near the viewer. */
        allocated = 0; cgs.media.electroGibModel = 30; cg.time = 10000; CG_ElectroGibs(body, qtrue);
        VectorSet(cg.refdef.vieworg, 1000, 0, 0);
        cg.time = 10100; CG_AddGibFlash(&entities[10]); CHECK(lights == 1 && lastLight == 400);
        cg.time = 10250; CG_AddGibFlash(&entities[10]); CHECK(lights == 2 && lastLight == 400);
        cg.time = 10375; CG_AddGibFlash(&entities[10]); CHECK(lights == 3 && lastLight == 200);
        cg.time = 10500; CG_AddGibFlash(&entities[10]); CHECK(lights == 3);
        VectorSet(cg.refdef.vieworg, 32, 48, 100); cg.time = 10100;
        CG_AddGibFlash(&entities[10]); CHECK(freed == 1 && lights == 3);
    }
    puts("QL effects: thaw fragments/trails/marks, through-surface impacts and electro gibs OK");
    return 0;
}
