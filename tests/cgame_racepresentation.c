/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_itemFx, cg_poiMaxWidth, cg_poiMinWidth;
static snapshot_t snapshot;
static refEntity_t scene[8];
static int submissions, registrations, pictures, projects, colorClears;
static float pictureX,pictureY,pictureSize,projectionScale = 1;
static vec4_t pictureColor;
static qhandle_t pictureShader;
static vec3_t lastProjected;
static qboolean behind;
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
qhandle_t trap_R_RegisterModel(const char *path) { CHECK(strstr(path,".md3")); return ++registrations; }
qhandle_t trap_R_RegisterSkin(const char *path) { CHECK(strstr(path,".skin")); return ++registrations; }
qhandle_t trap_R_RegisterShaderNoMip(const char *path) { CHECK(!strncmp(path,"gfx/2d/race/",12)); return ++registrations; }
void trap_R_AddRefEntityToScene(const refEntity_t *entity) { CHECK(submissions < 8); scene[submissions++] = *entity; }
qboolean trap_R_ProjectPoint(const vec3_t point, vec4_t window) {
    ++projects; VectorCopy(point,lastProjected);
    window[0] = point[0]; window[1] = point[2]*projectionScale;
    window[2] = 0; window[3] = 1; return !behind;
}
void trap_R_SetColor(const float *color) {
    if (color) memcpy(pictureColor,color,sizeof(vec4_t)); else ++colorClears;
}
void trap_R_DrawStretchPic(float x,float y,float w,float h,float s,float t,float s2,float t2,qhandle_t shader) {
    CHECK(w == h && !s && !t && s2 == 1 && t2 == 1);
    pictureX=x; pictureY=y; pictureSize=w; pictureShader=shader; ++pictures;
}
static void Scene(centity_t *cent, int expected) {
    submissions = 0; CHECK(CG_DrawRacePoint(cent)); CHECK(submissions == expected);
}
int main(void) {
    centity_t *cent = &cg_entities[70];
    cgRaceMedia_t *m = &cgs.raceMedia;
    int before;
    cg.snap = &snapshot; snapshot.ps.clientNum = 2; cgs.gametype = GT_RACE;
    CG_RegisterRaceMedia(); CHECK(registrations == 11);
    cgs.gameModels[1] = m->start; cgs.gameModels[2] = m->inactive;
    cgs.gameModels[3] = m->finish; cgs.gameModels[4] = 99;
    cent->currentState.eType = ET_GENERAL; cent->currentState.number = 70;
    cent->currentState.modelindex = 2; cent->currentState.frame = 3;
    cent->currentValid = qtrue;
    VectorSet(cent->lerpOrigin,100,200,300);
    AxisClear(cg.autoAxis); AxisClear(cg.autoAxisFast); cg.autoAxisFast[0][0] = .5f;
    cg.race.active = qtrue; cg.race.nextPoint = 70; cg.race.followingPoint = 71;
    Scene(cent,2);
    CHECK(scene[0].hModel == m->base && scene[0].customSkin == m->blueSkin && scene[0].origin[2] == 284);
    CHECK(scene[1].hModel == m->checkpoint && scene[1].origin[2] == 300 && scene[1].axis[0][0] == 1);
    CHECK(scene[1].frame == 3 && scene[1].oldframe == 3);
    cg_itemFx.integer = 3; cg.time = 256; Scene(cent,2);
    CHECK(scene[1].origin[2] == 310 && scene[1].oldorigin[2] == 300 && scene[1].axis[0][0] == .5f);
    cent->currentState.generic1 = 1; Scene(cent,1); CHECK(scene[0].hModel == m->checkpoint);
    cent->currentState.generic1 = 0; cent->currentState.modelindex2 = 1;
    Scene(cent,2); CHECK(scene[0].customSkin == m->redSkin && scene[1].hModel == m->finish);
    cent->currentState.modelindex2 = 0; snapshot.ps.eFlags = EF_SPECTATOR;
    Scene(cent,2); CHECK(scene[0].customSkin == m->neutralSkin && scene[1].hModel == m->inactive);
    snapshot.ps.pm_flags = PMF_FOLLOW; Scene(cent,2); CHECK(scene[1].hModel == m->checkpoint);
    snapshot.ps.eFlags = snapshot.ps.pm_flags = 0;
    cg.race.nextPoint = 71; cg.race.followingPoint = -1;
    Scene(cent,1); CHECK(scene[0].hModel == m->base); /* Hide only later flag, not its projector. */
    cg.race.followingPoint = 70; Scene(cent,2); CHECK(scene[1].hModel == m->inactive);
    cg.race.followingPoint = -1; cg.race.active = qfalse; Scene(cent,2);
    cg.race.nextPoint = 70;
    { qhandle_t saved = m->checkpoint; m->checkpoint = 0;
      Scene(cent,2); CHECK(scene[1].hModel == m->inactive); m->checkpoint = saved; }
    cg_itemFx.integer = -1; Scene(cent,2); CHECK(scene[1].origin[2] == 300 && scene[1].axis[0][0] == 1);
    cent->currentState.modelindex = 1; cg.race.nextPoint = -1;
    snapshot.numEntities = 1; snapshot.entities[0].number = 70;
    cg.refdef.x = 10; cg.refdef.y = 20; cg.refdef.height = 600; AxisClear(cg.refdef.viewaxis);
    cgs.screenYScale = 1.25f; cg_poiMaxWidth.value = 32; cg_poiMinWidth.value = 16;
    CG_DrawRaceMarkers(); CHECK(pictures == 1 && pictureShader == m->icons[0]);
    CHECK(pictureSize == 32 && pictureX == 94 && pictureY == 224 && pictureColor[1] == 1 && !pictureColor[0]);
    CHECK(lastProjected[2] == 380 && colorClears == pictures);
    projectionScale = .1f; CG_DrawRaceMarkers(); CHECK(pictureSize == 20); /* Far minimum. */
    projectionScale = 2; CG_DrawRaceMarkers(); CHECK(pictureSize == 64); /* World width, not a pixel cap. */
    cg.race.nextPoint = 70; cent->currentState.modelindex = 2; projectionScale = 1;
    CG_DrawRaceMarkers(); CHECK(pictureShader == m->icons[1] && pictureColor[0] == .2f && pictureColor[1] == .6f);
    cent->currentState.modelindex2 = 1; CG_DrawRaceMarkers(); CHECK(pictureShader == m->icons[2] && pictureColor[0] == 1);
    before = pictures; behind = qtrue; CG_DrawRaceMarkers(); CHECK(pictures == before); behind = qfalse;
    snapshot.ps.eFlags = EF_SPECTATOR; CG_DrawRaceMarkers(); CHECK(pictures == before);
    snapshot.ps.pm_flags = PMF_FOLLOW; CG_DrawRaceMarkers(); CHECK(pictures == before+1);
    snapshot.ps.eFlags = snapshot.ps.pm_flags = 0;
    cent->currentValid = qfalse; before = pictures; CG_DrawRaceMarkers(); CHECK(pictures == before); cent->currentValid = qtrue;
    cg_poiMaxWidth.value = NAN; cg_poiMinWidth.value = NAN;
    CG_DrawRaceMarkers(); CHECK(pictureSize == 16);
    cg_poiMaxWidth.value = 99; cg_poiMinWidth.value = 99;
    CG_DrawRaceMarkers(); CHECK(pictureSize == 32);
    cent->currentState.modelindex = MAX_MODELS; CHECK(!CG_DrawRacePoint(cent));
    cent->currentState.modelindex = 4; cg.race.nextPoint = -1; CHECK(!CG_DrawRacePoint(cent));
    cgs.gametype = GT_FFA; cent->currentState.modelindex = 1; CHECK(!CG_DrawRacePoint(cent));
    before = pictures; CG_DrawRaceMarkers(); CHECK(pictures == before);
    CG_RegisterRaceMedia(); CHECK(!m->start && registrations == 11);
    puts("Race presentation: progress models, spectator/follow, bases, item effects and projected markers OK");
    return 0;
}
