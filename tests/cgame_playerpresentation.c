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

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_animSpeed, cg_debugAnim, cg_noPlayerAnims;
vmCvar_t cg_drawSpriteSelf, cg_drawSprites, cg_teammatePOIs, cg_teammateNames, cg_drawHitFriendTime;
vmCvar_t cg_drawGun;
static const char *settingsMask = "0";
static const char *minimumSpeed = "0";
const char *CG_ConfigString(int index) {
    CHECK(index == CS_SETTINGS_MASK || index == CS_RR_PING_RATE);
    return index == CS_SETTINGS_MASK ? settingsMask : minimumSpeed;
}
static int markers;
static vec3_t markerOrigin;
static vec4_t markerColor;
static qhandle_t markerShader;
void CG_AddTeammateMarker(const vec3_t origin, const vec4_t color, qhandle_t shader) {
    VectorCopy(origin, markerOrigin); memcpy(markerColor, color, sizeof(markerColor));
    markerShader = shader; ++markers;
}
static snapshot_t snapshot;
static refEntity_t draws[8];
static int drawCount;
void QDECL CG_Error(const char *format, ...) { (void)format; abort(); }
void QDECL CG_Printf(const char *format, ...) { (void)format; }
void trap_R_AddRefEntityToScene(const refEntity_t *entity) {
    CHECK(drawCount < ARRAY_LEN(draws)); draws[drawCount++] = *entity;
}
static void DrawPass(entityState_t *state, qboolean weapon) {
    refEntity_t entity = {0};
    entity.hModel = 42; entity.customSkin = 43; entity.customShader = 44;
    entity.renderfx = RF_THIRD_PERSON | RF_LIGHTING_ORIGIN;
    entity.frame = 8; entity.oldframe = 7; entity.backlerp = .25f;
    entity.origin[2] = 123; entity.shaderRGBA[0] = 95;
    drawCount = 0;
    if (weapon) CG_AddWeaponWithPowerups(&entity,state);
    else CG_AddRefEntityWithPowerups(&entity,state,TEAM_RED);
    for (int i = 0; i < drawCount; ++i) {
        refEntity_t expected = entity;
        expected.customShader = draws[i].customShader;
        CHECK(!memcmp(&expected,&draws[i],sizeof(expected)));
    }
}
static void Draw(entityState_t *state) { DrawPass(state,qfalse); }
static void Animate(centity_t *cent, int time, int legsExpected, int torsoExpected) {
    int legsOld, legs, torsoOld, torso;
    float legsLerp, torsoLerp;
    cg.time = time;
    CG_PlayerAnimation(cent,&legsOld,&legs,&legsLerp,&torsoOld,&torso,&torsoLerp);
    CHECK(legs == legsExpected && torso == torsoExpected);
    CHECK(legsLerp >= 0 && legsLerp <= 1 && torsoLerp >= 0 && torsoLerp <= 1);
}
int main(void) {
    entityState_t state = {0};
    centity_t cent = {0};
    clientInfo_t *ci = &cgs.clientinfo[0];
    animation_t animation = {0};
    int stage;
    cgs.media.invisShader = 10; cgs.media.redQuadShader = 11;
    cgs.media.quadShader = 12; cgs.media.regenShader = 13; cgs.media.battleSuitShader = 14;
    cgs.media.icePlayerShader[0] = 20; cgs.media.icePlayerShader[1] = 21; cgs.media.icePlayerShader[2] = 22;
    Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 44);
    state.powerups = 1 << PW_FROZEN;
    for (stage = 0; stage < 4; ++stage) {
        state.generic1 = stage; Draw(&state);
        CHECK(drawCount == 2 && draws[0].customShader == 44);
        CHECK(draws[1].customShader == (stage & 2 ? 20 : stage & 1 ? 21 : 22));
    }
    state.generic1 = 0x104; Draw(&state); CHECK(draws[1].customShader == 22);
    state.powerups |= (1 << PW_QUAD) | (1 << PW_BATTLESUIT) | (1 << PW_REGEN);
    cg.time = 100; Draw(&state);
    CHECK(drawCount == 5 && draws[1].customShader == 11 && draws[2].customShader == 13);
    CHECK(draws[3].customShader == 14 && draws[4].customShader == 22);
    cg.time = 200; Draw(&state); CHECK(drawCount == 4 && draws[2].customShader == 14);
    state.powerups |= 1 << PW_INVIS; Draw(&state);
    CHECK(drawCount == 1 && draws[0].customShader == 10);
    state.powerups = 1 << PW_FROZEN; cgs.media.icePlayerShader[2] = 0;
    Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 44);

    cgs.media.noPlayerClipShader = 40; cgs.media.ghostWeaponShader = 41;
    cgs.media.battleWeaponShader = 50; cgs.media.quadWeaponShader = 51;
    settingsMask = "33554432"; CG_UpdatePlayerPresentationSettings();
    CHECK(cgs.noPlayerClipPresentation);
    state.clientNum = 1; state.powerups = (1 << PW_INVIS) | (1 << PW_FROZEN) | (1 << PW_QUAD);
    Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 40);
    state.clientNum = cg.clientNum; Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 10);
    // Body compares the actual local client; weapons compare the viewed one.
    cg.snap = &snapshot; snapshot.ps.clientNum = state.clientNum = 2;
    Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 40);
    DrawPass(&state,qtrue); CHECK(drawCount == 1 && draws[0].customShader == 10);
    state.clientNum = 1; DrawPass(&state,qtrue); CHECK(drawCount == 1 && draws[0].customShader == 40);
    cgs.media.noPlayerClipShader = 0; Draw(&state); CHECK(drawCount == 1 && draws[0].customShader == 10);
    cgs.media.noPlayerClipShader = 40;
    settingsMask = "0"; CG_UpdatePlayerPresentationSettings(); CHECK(!cgs.noPlayerClipPresentation);
    state.powerups = (1 << PW_QUAD) | (1 << PW_BATTLESUIT);
    DrawPass(&state,qtrue); CHECK(drawCount == 3 && draws[0].customShader == 44);
    CHECK(draws[1].customShader == 50 && draws[2].customShader == 51);
    cg_drawGun.integer = 3; DrawPass(&state,qtrue); CHECK(drawCount == 3); // remote weapon stays ordinary
    state.clientNum = snapshot.ps.clientNum; DrawPass(&state,qtrue);
    CHECK(drawCount == 1 && draws[0].customShader == 41);
    state.powerups |= 1 << PW_INVIS; DrawPass(&state,qtrue);
    CHECK(drawCount == 1 && draws[0].customShader == 10); // invisibility precedes ghost gun
    state.powerups = 1 << PW_QUAD; cgs.media.ghostWeaponShader = 0; DrawPass(&state,qtrue);
    CHECK(drawCount == 2 && draws[1].customShader == 51);
    cg_drawGun.integer = 1;
    { const char *invalid[] = {"", "-1", "33554432x", "4294967296", "99999999999999999999999", NULL};
      for (int i = 0; i < ARRAY_LEN(invalid); ++i) {
          cgs.noPlayerClipPresentation = qtrue; settingsMask = invalid[i];
          CG_UpdatePlayerPresentationSettings(); CHECK(!cgs.noPlayerClipPresentation);
      }
      settingsMask = " \t+33554432 \n"; CG_UpdatePlayerPresentationSettings(); CHECK(cgs.noPlayerClipPresentation);
      settingsMask = "4294967295"; CG_UpdatePlayerPresentationSettings(); CHECK(cgs.noPlayerClipPresentation);
      settingsMask = "67108864"; CG_UpdatePlayerPresentationSettings(); CHECK(!cgs.noPlayerClipPresentation);
    }
    snapshot.ps.clientNum = 0;

    cg_animSpeed.integer = 1;
    animation.firstFrame = 100; animation.numFrames = animation.loopFrames = 10;
    animation.frameLerp = animation.initialLerp = 100;
    ci->animations[LEGS_RUN] = animation;
    animation.firstFrame = 200; ci->animations[TORSO_STAND] = animation;
    cent.currentState.legsAnim = LEGS_RUN; cent.currentState.torsoAnim = TORSO_STAND;
    cg.time = 1000;
    CG_ClearLerpFrame(ci,&cent.pe.legs,LEGS_RUN);
    CG_ClearLerpFrame(ci,&cent.pe.torso,TORSO_STAND);
    Animate(&cent,1000,100,200);
    Animate(&cent,1100,101,201);
    Animate(&cent,1200,102,202);
    cent.currentState.powerups = (1 << PW_FROZEN) | (1 << PW_HASTE);
    Animate(&cent,1300,100,200); /* frozen's zero speed overrides haste */
    Animate(&cent,1400,100,200);
    Animate(&cent,1450,100,200);
    cent.currentState.powerups = 0;
    Animate(&cent,1500,105,205); /* ordinary animation resumes */
    cent.currentState.powerups = 1 << PW_HASTE;
    Animate(&cent,1600,109,209);
    cg_noPlayerAnims.integer = 1;
    { int a,b,c,d; float x = 0,y = 0;
      CG_PlayerAnimation(&cent,&a,&b,&x,&c,&d,&y); CHECK(!a && !b && !c && !d); }
    cg.snap = &snapshot; snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    cg.clientNum = 0; cg.renderingThirdPerson = qtrue;
    cgs.gametype = GT_FREEZE; cgs.clientinfo[0].team = cgs.clientinfo[1].team = TEAM_RED;
    cgs.media.friendShader = 30; cgs.media.connectionShader = 31;
    cgs.media.balloonShader = 33; cgs.media.friendHitShader = 34; cgs.media.frozenSpriteShader = 32;
    cgs.media.flagCarrierShader = 35; cgs.media.flagCarrierHitShader = 36;
    cgs.poiMedia.neutralFlag = 37; cgs.poiMedia.infected = 38;
    cg_drawSprites.integer = 1; cg_teammatePOIs.integer = 1; cg_teammateNames.integer = 0;
    cg_drawHitFriendTime.integer = 100; snapshot.ps.stats[STAT_MAX_HEALTH] = 100;
    cent.currentState.powerups = 1 << PW_FROZEN; cent.lerpOrigin[2] = 100;
    cent.pe.painTime = -100000; cg.time = 10000; drawCount = markers = 0;
    /* Retail 10041205: no self sprites unless cg_drawSpriteSelf. */
    CG_PlayerSprites(&cent); CHECK(!drawCount && !markers);
    cg_drawSpriteSelf.integer = 1;
    /* A frozen teammate (here the third-person self) is a frame marker. */
    CG_PlayerSprites(&cent); CHECK(!drawCount && markers == 1 && markerShader == 32 && markerOrigin[2] == 100);
    cg.renderingThirdPerson = qfalse;
    CG_PlayerSprites(&cent); CHECK(markers == 1); /* own marker only in third person */
    cg.renderingThirdPerson = qtrue; cg_drawSpriteSelf.integer = 0;
    cent.currentState.clientNum = cent.currentState.number = 1;
    CG_PlayerSprites(&cent); CHECK(markers == 2 && markerShader == 32);
    cent.currentState.eFlags = EF_CONNECTION;
    CG_PlayerSprites(&cent); CHECK(markers == 3 && markerShader == 31);
    /* Teammates: health tint, hit window, names and the POI switch. */
    cent.currentState.eFlags = 0; cent.currentState.powerups = 0; cent.currentState.health = 50;
    CG_PlayerSprites(&cent);
    CHECK(markers == 4 && markerShader == 30);
    CHECK(markerColor[0] == 1 && markerColor[1] == .5f && markerColor[2] == 0 && markerColor[3] == 1);
    cent.currentState.health = 150; CG_PlayerSprites(&cent); CHECK(markerColor[1] == 1 && markerColor[2] == .75f);
    cent.currentState.health = 250; CG_PlayerSprites(&cent); CHECK(markerColor[2] == 1);
    cent.currentState.health = -10; CG_PlayerSprites(&cent); CHECK(markerColor[1] == 0 && markerColor[2] == 0);
    snapshot.ps.stats[STAT_MAX_HEALTH] = 0; cent.currentState.health = 0;
    CG_PlayerSprites(&cent); CHECK(markerColor[1] == 1 && markerColor[2] == 1); /* no division by zero */
    snapshot.ps.stats[STAT_MAX_HEALTH] = 100; cent.currentState.health = 50;
    cg.predictedPlayerState.pm_type = PM_DEAD; CG_PlayerSprites(&cent);
    CHECK(markerColor[1] == 1 && markerColor[2] == 0); /* untinted yellow */
    cg.predictedPlayerState.pm_type = PM_SPECTATOR; CG_PlayerSprites(&cent); CHECK(markerColor[1] == 1);
    cg.predictedPlayerState.pm_type = PM_NORMAL; cent.currentState.health = 100;
    cent.pe.painTime = cg.time - 99; CG_PlayerSprites(&cent); CHECK(markerShader == 34);
    cent.pe.painTime = cg.time - 100; CG_PlayerSprites(&cent); CHECK(markerShader == 30);
    markers = 0; cg_teammateNames.integer = 1; CG_PlayerSprites(&cent); CHECK(!markers);
    cg_teammateNames.integer = 0; cg_teammatePOIs.integer = 0; CG_PlayerSprites(&cent); CHECK(!markers);
    cg_teammatePOIs.integer = 1; cent.currentState.eFlags = EF_DEAD; CG_PlayerSprites(&cent); CHECK(!markers);
    cent.currentState.eFlags = 0;
    /* Flag carriers in CTF, hit for 1.5 s after pain; One Flag neutral carriers. */
    cgs.gametype = GT_CTF; cent.currentState.powerups = 1 << PW_BLUEFLAG; cent.pe.painTime = cg.time - 1499;
    CG_PlayerSprites(&cent); CHECK(markers == 1 && markerShader == 36);
    cent.pe.painTime = cg.time - 1500; CG_PlayerSprites(&cent); CHECK(markerShader == 35);
    cgs.gametype = GT_1FCTF; cent.currentState.powerups = 1 << PW_NEUTRALFLAG;
    CG_PlayerSprites(&cent); CHECK(markerShader == 37);
    /* Frozen only marks teammates in Freeze Tag. */
    cgs.gametype = GT_TEAM; cent.currentState.powerups = 1 << PW_FROZEN;
    CG_PlayerSprites(&cent); CHECK(markerShader == 30);
    /* Enemies keep the 48-unit radius-10 world sprite. */
    cgs.clientinfo[2].team = TEAM_BLUE; cent.currentState.clientNum = cent.currentState.number = 2;
    cent.currentState.powerups = 0; cent.currentState.eFlags = EF_TALK; markers = drawCount = 0;
    CG_PlayerSprites(&cent);
    CHECK(!markers && drawCount == 1 && draws[0].customShader == 33 && draws[0].origin[2] == 148 && draws[0].radius == 10);
    cent.currentState.eFlags = 0; CG_PlayerSprites(&cent); CHECK(drawCount == 1); /* no friend sprite */
    /* Red Rover infection: survivors see slow infected players as bites. */
    cgs.gametype = GT_RED_ROVER; settingsMask = "67108864"; cgs.settingsMask = CG_SETTING_RR_INFECTED;
    minimumSpeed = "400"; VectorSet(cent.currentState.pos.trDelta, 300, 0, 0);
    cent.currentState.eFlags = EF_TALK; markers = drawCount = 0; /* hidden for infected enemies */
    CG_PlayerSprites(&cent); CHECK(!drawCount && markers == 1 && markerShader == 38 && markerColor[3] == .25f);
    VectorSet(cent.currentState.pos.trDelta, 500, 0, 0); CG_PlayerSprites(&cent); CHECK(markerColor[3] == 0);
    minimumSpeed = "0"; CG_PlayerSprites(&cent); CHECK(markerColor[3] == 1);
    cgs.clientinfo[0].team = TEAM_BLUE; markers = 0; CG_PlayerSprites(&cent); CHECK(!markers && !drawCount);
    cgs.clientinfo[0].team = TEAM_RED; cgs.settingsMask = 0; settingsMask = "0";
    /* cg_drawSprites is the master switch. */
    cgs.gametype = GT_TEAM; cent.currentState.clientNum = cent.currentState.number = 1;
    cent.currentState.eFlags = 0; cg_drawSprites.integer = 0; markers = drawCount = 0;
    CG_PlayerSprites(&cent); CHECK(!markers && !drawCount);
    puts("Player presentation: freeze, no-clip bodies/weapons, ghost gun, retail sprites and teammate markers OK");
    return 0;
}
