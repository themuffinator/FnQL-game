/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* EV_POI world pings: registration, ping kinds, fade, suppression, queue
 * bounds and placement through the shared world-marker projection. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_poiMaxWidth, cg_poiMinWidth, cg_teammatePOIsMaxWidth, cg_teammatePOIsMinWidth, cg_flagPOIs, cg_drawDeadFriendTime;
static const char *playerString = "n\\Mate";
const char *CG_ConfigString(int index) { return index == CS_PLAYERS + 6 ? playerString : ""; }
static snapshot_t snapshot;
static int registrations, pictures, colorClears;
static char registered[4][MAX_QPATH];
static qboolean registeredNoMip[4];
static float pictureX, pictureY, pictureSize;
static vec4_t pictureColor;
static qhandle_t pictureShader;

static qhandle_t Register(const char *path, qboolean noMip) {
    CHECK(registrations < 4);
    snprintf(registered[registrations], MAX_QPATH, "%s", path);
    registeredNoMip[registrations] = noMip;
    return 40 + ++registrations;
}
qhandle_t trap_R_RegisterShader(const char *path) { return Register(path, qfalse); }
qhandle_t trap_R_RegisterShaderNoMip(const char *path) { return Register(path, qtrue); }
/* A flat projection: x follows world x, y follows world z. */
qboolean trap_R_ProjectPoint(const vec3_t point, vec4_t window) {
    window[0] = point[0]; window[1] = point[2]; window[2] = 0; window[3] = 1;
    return qtrue;
}
void trap_R_SetColor(const float *color) {
    if (color) memcpy(pictureColor, color, sizeof(vec4_t)); else ++colorClears;
}
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s, float t,
                           float s2, float t2, qhandle_t shader) {
    CHECK(w == h && !s && !t && s2 == 1 && t2 == 1);
    pictureX = x; pictureY = y; pictureSize = w; pictureShader = shader; ++pictures;
}

static int Draw(void) {
    int before = pictures, clears = colorClears;
    CG_DrawPointsOfInterest();
    CHECK(colorClears - clears == pictures - before);
    return pictures - before;
}

static void Ping(centity_t *cent, int lifetime, int who) {
    cent->currentState.event = EV_POI;
    cent->currentState.powerups = lifetime;
    cent->currentState.generic1 = who;
    CG_PointOfInterest(cent);
}

static int Active(void) {
    int i, count = 0;
    for (i = 0; i < CG_MAX_POINTS_OF_INTEREST; ++i) count += cg.pointsOfInterest[i].active;
    return count;
}

static void Media(int gametype) {
    registrations = 0; cgs.gametype = gametype;
    CG_RegisterPoiMedia();
}

static void Reset(void) {
    memset(cg.pointsOfInterest, 0, sizeof(cg.pointsOfInterest));
}

int main(void) {
    static const int gametypes[] = {
        GT_FFA, GT_TOURNAMENT, GT_RACE, GT_TEAM, GT_CLAN_ARENA, GT_CTF, GT_1FCTF,
        GT_OBELISK, GT_HARVESTER, GT_FREEZE, GT_DOMINATION, GT_ATTACK_DEFEND, GT_RED_ROVER
    };
    centity_t cent;
    cgPointOfInterest_t *poi = &cg.pointsOfInterest[0];
    int i;

    /* Registration follows retail 0x100237cf / 0x100240dc / 0x100241f4. */
    for (i = 0; i < ARRAY_LEN(gametypes); ++i) {
        Media(gametypes[i]);
        /* The A&D capture icon is registered for every gametype (0x10024d40),
         * last; Harvester adds its own capture icon (0x10023d98). */
        CHECK(registrations >= 1 && !registeredNoMip[registrations - 1]);
        CHECK(!strcmp(registered[registrations - 1], "gfx/2d/ad/poi_capture"));
        CHECK(cgs.poiMedia.flagCapture == 40 + registrations);
        if (cgs.gametype == GT_FFA) {
            CHECK(registrations == 2 && registeredNoMip[0] && cgs.poiMedia.quadHog == 41);
            CHECK(!strcmp(registered[0], "gfx/2d/quad_hog/quadhog"));
        } else if (cgs.gametype == GT_RED_ROVER) {
            CHECK(registrations == 2 && registeredNoMip[0] && cgs.poiMedia.infected == 41);
            CHECK(!strcmp(registered[0], "gfx/2d/infected/bite"));
        } else if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_ATTACK_DEFEND) {
            CHECK(registrations == 2 && !registeredNoMip[0] && cgs.poiMedia.neutralFlag == 41);
            CHECK(!strcmp(registered[0], "sprites/neutralflagcarrier"));
        } else if (cgs.gametype == GT_HARVESTER) {
            CHECK(registrations == 2 && registeredNoMip[0] && cgs.poiMedia.harvesterCapture == 41);
            CHECK(!strcmp(registered[0], "gfx/2d/har/poi_capture"));
        } else {
            CHECK(registrations == 1 && !cgs.poiMedia.quadHog && !cgs.poiMedia.infected &&
                  !cgs.poiMedia.neutralFlag && !cgs.poiMedia.harvesterCapture);
        }
    }

    memset(&cent, 0, sizeof(cent));
    VectorSet(cent.lerpOrigin, 100, 200, 300);
    cg.snap = &snapshot; snapshot.ps.clientNum = 2;
    cg_poiMaxWidth.value = 32; cg_poiMinWidth.value = 16;
    cgs.screenYScale = 1;
    cg.refdef.x = 10; cg.refdef.y = 20; cg.refdef.height = 480; cg.refdef.width = 640;
    VectorSet(cg.refdef.vieworg, 100, 0, 300);
    VectorSet(cg.refdef.viewaxis[0], 0, 1, 0);
    VectorSet(cg.refdef.viewaxis[1], -1, 0, 0);
    VectorSet(cg.refdef.viewaxis[2], 0, 0, 1);

    /* Quad Hog (settings bit 18): 48 units up, gold, carrier client. */
    Media(GT_FFA); cgs.settingsMask = CG_SETTING_QUAD_HOG;
    cg.time = 1000; Ping(&cent, 4000, 5);
    CHECK(poi->active && poi->startedAt == 1000 && poi->lifetime == 4000 && poi->fadeDelay == 1000);
    CHECK(poi->origin[0] == 100 && poi->origin[1] == 200 && poi->origin[2] == 348);
    CHECK(poi->color[0] == 1 && poi->color[1] == .75f && poi->color[2] == 0 && poi->color[3] == 1);
    CHECK(poi->shader == cgs.poiMedia.quadHog && poi->client == 5 && poi->team == 0);
    CHECK(poi->worldWidth == 32 && poi->minimum == 16);
    CHECK(Draw() == 1 && pictureShader == cgs.poiMedia.quadHog && pictureColor[3] == 1);
    /* Projected half-width probe 16 up -> 32 pixels, bottom anchored. */
    CHECK(pictureSize == 32 && pictureX == 10 + 100 - 16 && pictureY == 20 + 480 - 348 - 32);
    cg.time = 2000; CHECK(Draw() == 1 && pictureColor[3] == 1); /* at the delay */
    cg.time = 3500; CHECK(Draw() == 1 && fabsf(pictureColor[3] - .5f) < 1e-6f);
    cg.time = 4999; CHECK(Draw() == 1 && pictureColor[3] > 0 && pictureColor[3] < .001f);
    CHECK(pictureColor[1] == .75f); /* only alpha fades */
    cg.time = 5000; CHECK(Draw() == 0 && !poi->active);
    /* The carrier never sees its own ping; others, and followers of
     * others, do. The test uses the snapshot client. */
    cg.time = 6000; Ping(&cent, 4000, 2); CHECK(Draw() == 0 && poi->active);
    snapshot.ps.clientNum = 3; CHECK(Draw() == 1);
    /* The minimum width applies when the projection is small. */
    cg_poiMaxWidth.value = 16; cg_poiMinWidth.value = 24; Reset(); Ping(&cent, 4000, 5);
    CHECK(poi->worldWidth == 16 && poi->minimum == 16); /* min clamps to 16 */
    CHECK(Draw() == 1 && pictureSize == 16);
    cgs.screenYScale = 2; CHECK(Draw() == 1 && pictureSize == 32); cgs.screenYScale = 1;
    cg_poiMaxWidth.value = 64; cg_poiMinWidth.value = NAN; Reset(); Ping(&cent, 4000, 5);
    CHECK(poi->worldWidth == 32 && poi->minimum == 2);
    cg_poiMaxWidth.value = 32; cg_poiMinWidth.value = 16;
    /* Widths are captured per ping. */
    cg_poiMaxWidth.value = 20; CHECK(Draw() == 1 && pictureSize == 32); cg_poiMaxWidth.value = 32;

    /* Behind or on the view plane: not drawn, but still live. */
    VectorSet(cg.refdef.vieworg, 100, 300, 300); CHECK(Draw() == 0 && poi->active);
    VectorSet(cg.refdef.vieworg, 100, 200, 300); CHECK(Draw() == 0);
    VectorSet(cg.refdef.vieworg, 100, 0, 300);
    cg.refdef.vieworg[1] = NAN; CHECK(Draw() == 0); cg.refdef.vieworg[1] = 0;

    /* Neutral flag carriers (no mutator bit): 64 up, 0.75 widths, white,
     * team in generic1, hidden from that team in team modes. */
    Media(GT_1FCTF); cgs.settingsMask = 0; Reset();
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; snapshot.ps.clientNum = 2;
    Ping(&cent, 1000, TEAM_RED);
    CHECK(poi->origin[2] == 364 && poi->worldWidth == 24 && poi->minimum == 12 && poi->fadeDelay == 250);
    CHECK(poi->color[0] == 1 && poi->color[1] == 1 && poi->color[2] == 1 && poi->color[3] == 1);
    CHECK(poi->shader == cgs.poiMedia.neutralFlag && poi->team == TEAM_RED && poi->client == 0);
    CHECK(Draw() == 0);
    snapshot.ps.persistant[PERS_TEAM] = TEAM_BLUE;
    CHECK(Draw() == 1 && pictureSize == 24 && pictureShader == cgs.poiMedia.neutralFlag);
    snapshot.ps.persistant[PERS_TEAM] = TEAM_SPECTATOR; CHECK(Draw() == 1);
    /* Quad Hog's own-client test reads client, still zero here. */
    cgs.settingsMask = CG_SETTING_QUAD_HOG; snapshot.ps.clientNum = 0; CHECK(Draw() == 0);
    cgs.settingsMask = 0; snapshot.ps.clientNum = 2;
    /* Team filtering starts at team deathmatch (gametype 3). */
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED;
    cgs.gametype = GT_RACE; CHECK(Draw() == 1);
    cgs.gametype = GT_TEAM; CHECK(Draw() == 0);
    cgs.gametype = GT_1FCTF; snapshot.ps.persistant[PERS_TEAM] = TEAM_BLUE;
    /* The handle is captured per ping; a zero handle draws nothing. */
    poi->shader = 0; CHECK(Draw() == 0 && poi->active);
    poi->shader = cgs.poiMedia.neutralFlag; CHECK(Draw() == 1);

    /* Red Rover infection (bit 26 without bit 18): 48 up, white, bite. */
    Media(GT_RED_ROVER); cgs.settingsMask = CG_SETTING_RR_INFECTED; Reset();
    snapshot.ps.persistant[PERS_TEAM] = TEAM_BLUE;
    Ping(&cent, 2000, TEAM_RED);
    CHECK(poi->origin[2] == 348 && poi->worldWidth == 32 && poi->shader == cgs.poiMedia.infected);
    CHECK(poi->team == TEAM_RED && poi->color[1] == 1 && Draw() == 1);
    cgs.settingsMask = CG_SETTING_RR_INFECTED | CG_SETTING_QUAD_HOG; Reset();
    Ping(&cent, 2000, 9); CHECK(poi->client == 9 && poi->color[2] == 0); /* Quad Hog wins */

    /* Lifetime is powerups; non-positive or tiny lifetimes never linger. */
    Media(GT_1FCTF); cgs.settingsMask = 0; Reset();
    Ping(&cent, 0, TEAM_RED); CHECK(Draw() == 0 && !poi->active);
    Ping(&cent, -50, TEAM_RED); CHECK(poi->fadeDelay == -12 && Draw() == 0 && !poi->active);
    Ping(&cent, 3, TEAM_RED); CHECK(poi->fadeDelay == 0 && Draw() == 1 && pictureColor[3] == 1);
    cg.time += 2; CHECK(Draw() == 1 && fabsf(pictureColor[3] - 1.0f / 3.0f) < 1e-6f);
    cg.time += 1; CHECK(Draw() == 0);
    Ping(&cent, INT_MAX, TEAM_RED); CHECK(Draw() == 1 && pictureColor[3] == 1);
    /* A clock moving backwards expires markers instead of freezing them. */
    cg.time -= 1; CHECK(Draw() == 0 && !poi->active);

    /* The 32-record queue drops new pings when full and reuses expired
     * records, even before the next draw clears them. */
    Reset(); cg.time = 10000;
    for (i = 0; i < CG_MAX_POINTS_OF_INTEREST; ++i) Ping(&cent, 100 + i, TEAM_RED);
    CHECK(Active() == CG_MAX_POINTS_OF_INTEREST);
    Ping(&cent, 5000, TEAM_BLUE);
    for (i = 0; i < CG_MAX_POINTS_OF_INTEREST; ++i) CHECK(cg.pointsOfInterest[i].team == TEAM_RED);
    cg.time = 10100; Ping(&cent, 5000, TEAM_BLUE);
    CHECK(cg.pointsOfInterest[0].team == TEAM_BLUE && cg.pointsOfInterest[0].lifetime == 5000);
    CHECK(cg.pointsOfInterest[1].team == TEAM_RED);
    /* Slot 0 is the snapshot team's own ping and stays hidden. */
    CHECK(Draw() == CG_MAX_POINTS_OF_INTEREST - 1 && Active() == CG_MAX_POINTS_OF_INTEREST);
    cg.time = 10132; Draw(); CHECK(Active() == 1);

    /* Race free spectators get no markers; following spectators do. */
    cgs.gametype = GT_RACE; Reset();
    snapshot.ps.eFlags = EF_SPECTATOR; Ping(&cent, 1000, 1); CHECK(Active() == 0);
    snapshot.ps.pm_flags = PMF_FOLLOW; Ping(&cent, 1000, 1); CHECK(Active() == 1);
    snapshot.ps.eFlags = 0; snapshot.ps.pm_flags = 0;
    /* Without a snapshot nothing is queued or drawn. */
    Reset(); cg.snap = NULL; Ping(&cent, 1000, 1); CHECK(Active() == 0 && Draw() == 0);

    /* Teammate markers: one frame, 48 up, teammate widths, bounded. */
    {
        vec3_t origin = { 100, 200, 300 };
        vec4_t tint = { 1, .5f, 0, 1 };
        cg.snap = &snapshot; Reset();
        VectorSet(cg.refdef.vieworg, 100, 0, 300); cgs.screenYScale = 1;
        cg_teammatePOIsMaxWidth.value = 24; cg_teammatePOIsMinWidth.value = 4;
        CG_AddTeammateMarker(origin, tint, 77);
        CHECK(cg.teammateMarkerCount == 1 && cg.teammateMarkers[0].origin[2] == 348);
        CHECK(Draw() == 1 && pictureShader == 77 && pictureSize == 24 && pictureColor[1] == .5f);
        CHECK(cg.teammateMarkerCount == 0 && Draw() == 0); /* drawn once */
        cg_teammatePOIsMaxWidth.value = 99; cg_teammatePOIsMinWidth.value = NAN;
        CG_AddTeammateMarker(origin, tint, 77);
        CHECK(cg.teammateMarkers[0].worldWidth == 32 && cg.teammateMarkers[0].minimum == 2);
        CHECK(Draw() == 1);
        for (i = 0; i < CG_MAX_TEAMMATE_MARKERS + 5; ++i) CG_AddTeammateMarker(origin, tint, 77);
        CHECK(cg.teammateMarkerCount == CG_MAX_TEAMMATE_MARKERS && Draw() == CG_MAX_TEAMMATE_MARKERS);
        CG_AddTeammateMarker(origin, tint, 0); CHECK(Draw() == 0); /* no shader */
        VectorSet(cg.refdef.vieworg, 100, 400, 300); CG_AddTeammateMarker(origin, tint, 77);
        CHECK(Draw() == 0); /* behind the view */
    }

    /* Objective markers (retail 0x10040870) for the snapshot player. */
    {
        centity_t self;
        vec3_t redBase = { 10, 20, 30 }, blueBase = { 100, 200, 300 }, nanBase = { NAN, 0, 0 };
        cgTeammateMarker_t *m = &cg.teammateMarkers[0];
        memset(&self, 0, sizeof(self)); memset(&snapshot, 0, sizeof(snapshot));
        memset(cg.objectiveBaseSeen, 0, sizeof(cg.objectiveBaseSeen));
        cg.snap = &snapshot; snapshot.ps.clientNum = 4; self.currentState.clientNum = 4;
        cg.teammateMarkerCount = 0; cg_flagPOIs.integer = 1;
        cg_poiMaxWidth.value = 32; cg_poiMinWidth.value = 16;
        cgs.poiMedia.flagCapture = 90; cgs.poiMedia.harvesterCapture = 91;
        cgs.gametype = GT_CTF; cgs.clientinfo[4].team = TEAM_RED; cgs.redflag = cgs.blueflag = 0;
        self.currentState.powerups = 1 << PW_BLUEFLAG;
        CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 0); /* bases never seen */
        CG_RecordObjectiveBase(TEAM_RED, redBase); CG_RecordObjectiveBase(TEAM_BLUE, blueBase);
        CG_RecordObjectiveBase(TEAM_FREE, nanBase); CHECK(!cg.objectiveBaseSeen[0]);
        /* CTF: carrying the enemy flag marks the own base while the own flag is home. */
        CG_ObjectiveMarkers(&self);
        CHECK(cg.teammateMarkerCount == 1 && m->origin[0] == 10 && m->origin[2] == 94 && m->shader == 90);
        CHECK(m->color[0] == 1 && m->color[1] == .2f && m->worldWidth == 32 && m->minimum == 16);
        cg.teammateMarkerCount = 0; cgs.redflag = 1; CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 0);
        cgs.redflag = 0; cg_flagPOIs.integer = 0; CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 0);
        cg_flagPOIs.integer = 1;
        cgs.clientinfo[4].team = TEAM_BLUE; self.currentState.powerups = 1 << PW_REDFLAG;
        CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 1 && m->origin[0] == 100 && m->color[2] == 1);
        cg.teammateMarkerCount = 0; cgs.blueflag = 2; CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 0);
        /* One Flag: the neutral flag goes to the enemy base, in the own team's color. */
        cgs.gametype = GT_1FCTF; self.currentState.powerups = 1 << PW_NEUTRALFLAG;
        CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 1 && m->origin[0] == 10 && m->color[2] == 1);
        cg.teammateMarkerCount = 0; cgs.clientinfo[4].team = TEAM_RED;
        CG_ObjectiveMarkers(&self); CHECK(m->origin[0] == 100 && m->color[0] == 1 && m->color[1] == .2f);
        /* Other modes with a neutral flag mark the own base. */
        cg.teammateMarkerCount = 0; cgs.gametype = GT_ATTACK_DEFEND;
        CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 1 && m->origin[0] == 10);
        /* Harvester skull carriers see the enemy base, independent of cg_flagPOIs. */
        cg.teammateMarkerCount = 0; cgs.gametype = GT_HARVESTER; self.currentState.powerups = 0;
        self.currentState.generic1 = 3; cg_flagPOIs.integer = 0;
        CG_ObjectiveMarkers(&self);
        CHECK(cg.teammateMarkerCount == 1 && m->origin[0] == 100 && m->shader == 91 && m->color[2] == 1);
        cg.teammateMarkerCount = 0; self.currentState.generic1 = 0x40; CG_ObjectiveMarkers(&self);
        CHECK(cg.teammateMarkerCount == 0); /* only the skull bits count */
        /* Only the snapshot player's own entity. */
        self.currentState.generic1 = 3; self.currentState.clientNum = 5;
        CG_ObjectiveMarkers(&self); CHECK(cg.teammateMarkerCount == 0);
        cg.teammateMarkerCount = 0;
    }

    /* Dead teammates (retail obituary 0x10019246). */
    {
        entityState_t obit;
        cgDeadFriend_t *dead = &cg.deadFriends[0];
        memset(&obit, 0, sizeof(obit)); memset(cg.deadFriends, 0, sizeof(cg.deadFriends));
        memset(&snapshot, 0, sizeof(snapshot)); cg.snap = &snapshot; cg.clientNum = 2;
        snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; cgs.clientinfo[6].team = TEAM_RED;
        cgs.gametype = GT_TEAM; cg.time = 50000; cg_drawDeadFriendTime.value = 3000;
        cg_teammatePOIsMaxWidth.value = 24; cg_teammatePOIsMinWidth.value = 4;
        obit.otherEntityNum = 6; VectorSet(obit.pos.trBase, 100, 200, 300);
        CG_DeadTeammateMarker(&obit);
        CHECK(dead->active && dead->origin[2] == 348 && dead->lifetime == 3000 && dead->worldWidth == 24);
        cgs.media.friendDeadShader = 66; VectorSet(cg.refdef.vieworg, 100, 0, 300);
        VectorSet(cg.refdef.viewaxis[0], 0, 1, 0); VectorSet(cg.refdef.viewaxis[2], 0, 0, 1);
        CHECK(Draw() == 1 && pictureShader == 66 && pictureColor[0] == 1 && pictureColor[1] == 0);
        cg.time = 52999; CHECK(Draw() == 1);
        cg.time = 53000; CHECK(Draw() == 0 && !dead->active);
        /* Enemies, the local client, Red Rover, Freeze Tag, individual modes,
         * an empty player string and a zero time leave no marker. */
        cg.time = 60000; cgs.clientinfo[6].team = TEAM_BLUE; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cgs.clientinfo[6].team = TEAM_RED;
        cgs.gametype = GT_FREEZE; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cgs.gametype = GT_RED_ROVER; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cgs.gametype = GT_FFA; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cgs.gametype = GT_CTF; cg_drawDeadFriendTime.value = 0; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cg_drawDeadFriendTime.value = 3000; playerString = ""; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        playerString = "n\\Mate"; cg.clientNum = 6; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        cg.clientNum = 2; obit.otherEntityNum = MAX_CLIENTS; CG_DeadTeammateMarker(&obit); CHECK(!dead->active);
        obit.otherEntityNum = 6; cg_drawDeadFriendTime.value = 99999;
        CG_DeadTeammateMarker(&obit); CHECK(dead->active && dead->lifetime == 5000);
        /* A full queue drops new markers. */
        for (i = 0; i < CG_MAX_TEAMMATE_MARKERS + 3; ++i) CG_DeadTeammateMarker(&obit);
        for (i = 0; i < CG_MAX_TEAMMATE_MARKERS; ++i) CHECK(cg.deadFriends[i].active);
        memset(cg.deadFriends, 0, sizeof(cg.deadFriends));
    }

    puts("POI: registration, Quad Hog/neutral flag/infected pings, fade, filters, queue, placement, teammate, objective and dead-teammate markers OK");
    return 0;
}
