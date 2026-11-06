/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Gore DLC paths (retail 0x10022ffc, 0x10014c80, 0x10014500, 0x10013da0,
 * 0x1001d6a0, 0x1001da41, 0x1001ec25). */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static int haveDLC, handles = 100, allocated, puffs, marks;
static localEntity_t les[64];
static char registered[32][MAX_QPATH];
static int registrations;
static float puffRadius, markRadius;
static int puffDuration;
static qhandle_t puffShader, markShader;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
int trap_FS_GetFileList(const char *path, const char *ext, char *list, int size) {
    CHECK(!strcmp(path, "dlc_gibs") && !strcmp(ext, "") && size == 1024); (void)list; return haveDLC ? 17 : 0;
}
static qhandle_t Register(const char *name) { snprintf(registered[registrations++ % 32], MAX_QPATH, "%s", name); return handles++; }
qhandle_t trap_R_RegisterShader(const char *name) { return Register(name); }
qhandle_t trap_R_RegisterModel(const char *name) { return Register(name); }
sfxHandle_t trap_S_RegisterSound(const char *name, qboolean compressed) { (void)compressed; return Register(name); }
localEntity_t *CG_AllocLocalEntity(void) { CHECK(allocated < 64); memset(&les[allocated], 0, sizeof(les[0])); return &les[allocated++]; }
localEntity_t *CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration,
                            int startTime, int fadeInTime, int leFlags, qhandle_t hShader) {
    (void)p; (void)vel; (void)r; (void)g; (void)b; (void)a; (void)startTime; (void)fadeInTime; (void)leFlags;
    puffRadius = radius; puffDuration = (int)duration; puffShader = hShader; ++puffs;
    return CG_AllocLocalEntity();
}
void CG_ImpactMark(qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a,
                   qboolean alphaFade, float radius, qboolean temporary) {
    (void)origin; (void)dir; (void)orientation; (void)r; (void)g; (void)b; (void)a;
    CHECK(alphaFade && !temporary); markShader = shader; markRadius = radius; ++marks;
}
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result) { (void)tr; (void)atTime; VectorClear(result); }

int main(void) {
    vec3_t origin = { 10, 20, 30 };
    trace_t trace;
    int i, gibs;
    /* Without the DLC nothing registers and every path declines. */
    cgs.media.invulnerabilityJuicedModel = 5; cgs.media.invulnerabilityJuicedSound = 6;
    CG_RegisterGoreDLC();
    CHECK(!cgs.media.goreDLC && !registrations && !cgs.media.invulnerabilityJuicedModel && !cgs.media.invulnerabilityJuicedSound);
    CHECK(!CG_GoreGibPlayer(origin) && !CG_GoreBleed(origin) && !CG_GoreImpactSound() && !allocated);
    memset(&trace, 0, sizeof(trace)); CHECK(!CG_GoreBloodMark(&trace) && !marks);
    /* With dlc_gibs: sprays, fifteen gibs, the juicer and four sounds. */
    haveDLC = 1; CG_RegisterGoreDLC();
    CHECK(cgs.media.goreDLC && registrations == 1 + 4 + 15 + 1 + 1 + 3 + 1);
    CHECK(!strcmp(registered[0], "viewBloodBlend") && !strcmp(registered[1], "bloodSpray1") && !strcmp(registered[4], "bloodSpray4"));
    CHECK(!strcmp(registered[5], "dlc_gibs/skull.md3") && !strcmp(registered[19], "dlc_gibs/foot.md3"));
    CHECK(!strcmp(registered[20], "dlc_gibs/juicer.md3") && !strcmp(registered[21], "dlc_gibs/gibsplt1.ogg"));
    CHECK(!strcmp(registered[24], "dlc_gibs/gibimp3.ogg") && !strcmp(registered[25], "dlc_gibs/invul_juiced.ogg"));
    CHECK(cgs.media.invulnerabilityJuicedModel != 0 && cgs.media.invulnerabilityJuicedSound != 0);
    /* Gibbing: fourteen or fifteen fragments with retail's launch values. */
    cg.time = 1000; allocated = 0;
    CHECK(CG_GoreGibPlayer(origin));
    gibs = allocated; CHECK(gibs == 14 || gibs == 15);
    for (i = 0; i < gibs; ++i) {
        const localEntity_t *le = &les[i];
        CHECK(le->leType == LE_FRAGMENT && le->pos.trType == TR_GRAVITY && le->bounceFactor == 0.6f);
        CHECK(le->leBounceSoundType == LEBS_BLOOD && le->leMarkType == LEMT_BLOOD);
        CHECK(le->endTime >= 6000 && le->endTime <= 9000 && VectorCompare(le->pos.trBase, origin));
        CHECK(fabs(le->pos.trDelta[0]) <= 250 && le->pos.trDelta[2] >= 0 && le->pos.trDelta[2] <= 500);
    }
    CHECK(les[gibs - 1].refEntity.hModel == cgs.media.dlcGibModels[14]);
    /* Bleeding and trails: falling random-spray puffs. */
    allocated = puffs = 0; CHECK(CG_GoreBleed(origin) && puffs == 1 && puffRadius == 32 && puffDuration == 500);
    CHECK(les[0].leType == LE_FALL_SCALE_FADE && les[0].pos.trDelta[2] == 40);
    CHECK(puffShader >= cgs.media.dlcBloodSpray[0] && puffShader <= cgs.media.dlcBloodSpray[3]);
    allocated = puffs = 0; cg.time = 1000; cg.frametime = 150;
    CHECK(CG_GoreBloodTrail(&les[0]) && puffs == 3 && puffRadius == 20 && puffDuration == 500);
    /* Impacts: a gibimp sound and a spray 3/4 mark of radius 16..47. */
    for (i = 0; i < 20; ++i) {
        const sfxHandle_t s = CG_GoreImpactSound();
        CHECK(s == cgs.media.dlcGibImpactSounds[0] || s == cgs.media.dlcGibImpactSounds[1] || s == cgs.media.dlcGibImpactSounds[2]);
        CHECK(CG_GoreBloodMark(&trace));
        CHECK((markShader == cgs.media.dlcBloodSpray[2] || markShader == cgs.media.dlcBloodSpray[3]) && markRadius >= 16 && markRadius <= 47);
    }
    puts("gore DLC: gating, registration, gibs, bleeding, trails, sounds and marks OK");
    return 0;
}
