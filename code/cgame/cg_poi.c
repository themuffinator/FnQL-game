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

/* EV_POI pings. Observed retail behavior, cgamex86:
 * - 0x1001c228 fills a world-marker record: origin is the event origin,
 *   lifetime is entityState powerups (ms), the fade starts after
 *   powerups / 4 and generic1 names the carrier client (Quad Hog) or team.
 * - 0x1002a0d0 allocates from a 32-record queue and refuses race free
 *   spectators; a full queue drops the new marker.
 * - 0x1002a190 expires and fades records at the start of each frame.
 * - 0x10011680 draws them, hiding the snapshot team's own pings in team
 *   modes and, with Quad Hog on, the snapshot client's own ping.
 * Retail shares that queue with damage plums and other markers; FnQL-game
 * gives points of interest their own 32 records (an inference that only
 * matters when more than 32 markers of all kinds are live at once). */

void CG_RegisterPoiMedia(void) {
    cgPoiMedia_t *m = &cgs.poiMedia;
    memset(m, 0, sizeof(*m));
    /* Retail 0x100237cf, 0x100240dc and 0x100241f4 register each icon only
     * for the gametypes that use it; other gametypes keep a zero handle. */
    if (cgs.gametype == GT_FFA)
        m->quadHog = trap_R_RegisterShaderNoMip("gfx/2d/quad_hog/quadhog");
    if (cgs.gametype == GT_RED_ROVER)
        m->infected = trap_R_RegisterShaderNoMip("gfx/2d/infected/bite");
    if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_ATTACK_DEFEND)
        m->neutralFlag = trap_R_RegisterShader("sprites/neutralflagcarrier");
    /* 0x10023d98 (Harvester only) and 0x10024d40 (always). */
    if (cgs.gametype == GT_HARVESTER)
        m->harvesterCapture = trap_R_RegisterShaderNoMip("gfx/2d/har/poi_capture");
    m->flagCapture = trap_R_RegisterShader("gfx/2d/ad/poi_capture");
}

static qboolean CG_PoiExpired(const cgPointOfInterest_t *poi) {
    long long elapsed = (long long)cg.time - poi->startedAt;
    /* Retail keeps a record while time < start + lifetime. A clock that
     * moves backwards (demo seeks) expires the record instead of leaving a
     * stale icon on screen until the clock catches up. */
    return !poi->active || elapsed < 0 || elapsed >= poi->lifetime;
}

void CG_PointOfInterest(const centity_t *cent) {
    const entityState_t *s = &cent->currentState;
    cgPointOfInterest_t *poi = NULL;
    float height, scale;
    int i;
    if (!cg.snap) return;
    if (cgs.gametype == GT_RACE && (cg.snap->ps.eFlags & EF_SPECTATOR) &&
        !(cg.snap->ps.pm_flags & PMF_FOLLOW)) return;
    for (i = 0; i < CG_MAX_POINTS_OF_INTEREST; ++i) {
        if (CG_PoiExpired(&cg.pointsOfInterest[i])) {
            poi = &cg.pointsOfInterest[i];
            break;
        }
    }
    if (!poi) return;
    memset(poi, 0, sizeof(*poi));
    poi->active = qtrue;
    poi->startedAt = cg.time;
    poi->lifetime = s->powerups;
    poi->fadeDelay = s->powerups / 4; /* signed, truncating: 0x1001c280 */
    poi->color[0] = poi->color[1] = poi->color[2] = poi->color[3] = 1;
    /* The settings word decides the ping kind, not the gametype. */
    if (cgs.settingsMask & CG_SETTING_QUAD_HOG) {
        height = 48; scale = 1;
        poi->color[1] = .75f; poi->color[2] = 0;
        poi->shader = cgs.poiMedia.quadHog;
        poi->client = s->generic1;
    } else if (!(cgs.settingsMask & CG_SETTING_RR_INFECTED)) {
        height = 64; scale = .75f;
        poi->shader = cgs.poiMedia.neutralFlag;
        poi->team = s->generic1;
    } else {
        height = 48; scale = 1;
        poi->shader = cgs.poiMedia.infected;
        poi->team = s->generic1;
    }
    /* The widths are captured when the ping arrives, as retail stores the
     * cvar values in the record. */
    CG_WorldMarkerWidths(scale, &poi->worldWidth, &poi->minimum);
    VectorCopy(cent->lerpOrigin, poi->origin);
    poi->origin[2] += height;
}

/* One-frame markers (retail allocator lifetime 1 ms, 0x1002a156): the
 * world point already includes the record's height. A full queue drops the
 * marker. */
static void CG_AddFrameMarker(const vec3_t point, float worldWidth, float minimum,
                              const vec4_t color, qhandle_t shader) {
    cgTeammateMarker_t *marker;
    if (cg.teammateMarkerCount < 0 || cg.teammateMarkerCount >= CG_MAX_TEAMMATE_MARKERS) return;
    marker = &cg.teammateMarkers[cg.teammateMarkerCount++];
    VectorCopy(point, marker->origin);
    memcpy(marker->color, color, sizeof(marker->color));
    marker->worldWidth = worldWidth;
    marker->minimum = minimum;
    marker->shader = shader;
}

/* Retail queues a teammate's sprite 48 units above the player, sized by
 * the cg_teammatePOIs widths (cvars 10078024 and 1007803c, limits 16..32
 * and 2..16). */
void CG_AddTeammateMarker(const vec3_t origin, const vec4_t color, qhandle_t shader) {
    vec3_t point;
    float world = cg_teammatePOIsMaxWidth.value, least = cg_teammatePOIsMinWidth.value;
    if (!(world >= 16)) world = 16;
    if (world > 32) world = 32;
    if (!(least >= 2)) least = 2;
    if (least > 16) least = 16;
    VectorCopy(origin, point);
    point[2] += 48;
    CG_AddFrameMarker(point, world, least, color, shader);
}

/* Retail records the last drawn position of each team flag item (by tag,
 * 0x100160a7) and team base (by modelindex, 0x1001891b): neutral, red and
 * blue (0x10aba978/984/990). */
void CG_RecordObjectiveBase(int team, const vec3_t origin) {
    int slot = team == TEAM_RED ? 1 : team == TEAM_BLUE ? 2 : 0;
    if (!origin || !(origin[0] == origin[0] && origin[1] == origin[1] && origin[2] == origin[2])) return;
    VectorCopy(origin, cg.objectiveBases[slot]);
    cg.objectiveBaseSeen[slot] = qtrue;
}

static void CG_ObjectiveMarker(int slot, const vec4_t color, qhandle_t shader) {
    vec3_t point;
    float world, least;
    /* Retail would mark the world origin for a base it never drew; FnQL-game
     * skips it until the base has been seen. */
    if (!shader || !cg.objectiveBaseSeen[slot]) return;
    CG_WorldMarkerWidths(1, &world, &least);
    VectorCopy(cg.objectiveBases[slot], point);
    point[2] += 64;
    CG_AddFrameMarker(point, world, least, color, shader);
}

/* Retail 0x10040870, run for the snapshot player's own entity: in
 * Harvester a skull carrier sees the enemy base; a flag carrier sees where
 * to score. CTF needs the carrier's own flag at base and marks the own base;
 * One Flag marks the enemy base for the neutral flag. cg_flagPOIs gates the
 * flag marker only. */
void CG_ObjectiveMarkers(const centity_t *cent) {
    static const vec4_t red = { 1, .2f, .2f, 1 }, blue = { .2f, .6f, 1, 1 };
    const entityState_t *s = &cent->currentState;
    int client, team, powerups = s->powerups, slot = -1;
    qboolean ownBase = qtrue;
    const float *color = NULL;
    if (!cg.snap || s->clientNum != cg.snap->ps.clientNum) return;
    client = cg.snap->ps.clientNum;
    if (client < 0 || client >= MAX_CLIENTS) return;
    team = cgs.clientinfo[client].team;
    if (cgs.gametype == GT_HARVESTER && (s->generic1 & 0x3f) && (team == TEAM_RED || team == TEAM_BLUE))
        CG_ObjectiveMarker(team == TEAM_RED ? 2 : 1, team == TEAM_RED ? blue : red, cgs.poiMedia.harvesterCapture);
    if (!powerups) return;
    if (cgs.gametype == GT_CTF) {
        if (team == TEAM_RED && cgs.redflag >= 1) return;
        if (team == TEAM_BLUE && cgs.blueflag >= 1) return;
    } else if (cgs.gametype == GT_1FCTF) {
        ownBase = qfalse;
    }
    if (powerups & (1 << PW_REDFLAG)) { slot = ownBase ? 2 : 1; color = ownBase ? blue : red; }
    if (powerups & (1 << PW_BLUEFLAG)) { slot = ownBase ? 1 : 2; color = ownBase ? red : blue; }
    if (powerups & (1 << PW_NEUTRALFLAG)) {
        if (ownBase) { slot = team == TEAM_RED ? 1 : 2; color = team == TEAM_RED ? red : blue; }
        else { slot = team == TEAM_RED ? 2 : 1; color = team == TEAM_RED ? red : blue; }
    }
    if (slot < 0 || !cg_flagPOIs.integer) return;
    CG_ObjectiveMarker(slot, color, cgs.poiMedia.flagCapture);
}

/* Retail obituary handler 0x10019246: a teammate's death (team modes,
 * the snapshot's team, not the local client, not Red Rover or Freeze Tag)
 * leaves a red friend_dead marker 16 units below the event origin, drawn 64
 * units above that point with the teammate widths for cg_drawDeadFriendTime
 * milliseconds. The cvar and widths are captured at the death. */
void CG_DeadTeammateMarker(const entityState_t *obituary) {
    int target = obituary->otherEntityNum, i;
    float world = cg_teammatePOIsMaxWidth.value, least = cg_teammatePOIsMinWidth.value;
    float lifetime = cg_drawDeadFriendTime.value;
    cgDeadFriend_t *marker = NULL;
    if (!cg.snap || target < 0 || target >= MAX_CLIENTS || target == cg.clientNum) return;
    if (!CG_ConfigString(CS_PLAYERS + target)[0] || !(lifetime > 0)) return;
    if (cgs.gametype < GT_TEAM || cgs.gametype == GT_RED_ROVER || cgs.gametype == GT_FREEZE) return;
    if (cg.snap->ps.persistant[PERS_TEAM] != cgs.clientinfo[target].team) return;
    for (i = 0; i < CG_MAX_TEAMMATE_MARKERS; ++i) {
        cgDeadFriend_t *candidate = &cg.deadFriends[i];
        if (!candidate->active || (double)cg.time - candidate->startedAt >= candidate->lifetime ||
            cg.time < candidate->startedAt) {
            marker = candidate;
            break;
        }
    }
    if (!marker) return;
    if (!(world >= 16)) world = 16;
    if (world > 32) world = 32;
    if (!(least >= 2)) least = 2;
    if (least > 16) least = 16;
    marker->active = qtrue;
    marker->startedAt = cg.time;
    marker->lifetime = lifetime > 5000 ? 5000 : lifetime;
    marker->worldWidth = world;
    marker->minimum = least;
    VectorCopy(obituary->pos.trBase, marker->origin);
    marker->origin[2] += 64 - 16;
}

static void CG_DrawDeadFriends(void) {
    static const vec4_t red = { 1, 0, 0, 1 };
    int i;
    for (i = 0; i < CG_MAX_TEAMMATE_MARKERS; ++i) {
        cgDeadFriend_t *marker = &cg.deadFriends[i];
        vec3_t delta;
        float x, y, size;
        if (!marker->active) continue;
        if (cg.time < marker->startedAt || (double)cg.time - marker->startedAt >= marker->lifetime) {
            marker->active = qfalse;
            continue;
        }
        if (!cgs.media.friendDeadShader) continue;
        VectorSubtract(marker->origin, cg.refdef.vieworg, delta);
        if (!(DotProduct(delta, cg.refdef.viewaxis[0]) > 0)) continue;
        if (!CG_ProjectWorldMarker(marker->origin, marker->worldWidth, marker->minimum, &x, &y, &size)) continue;
        trap_R_SetColor(red);
        trap_R_DrawStretchPic(x, y, size, size, 0, 0, 1, 1, cgs.media.friendDeadShader);
        trap_R_SetColor(NULL);
    }
}

static void CG_DrawTeammateMarkers(void) {
    int i, count = cg.teammateMarkerCount;
    cg.teammateMarkerCount = 0; /* one frame only (lifetime 1 ms, 0x1002a156) */
    if (count > CG_MAX_TEAMMATE_MARKERS) count = CG_MAX_TEAMMATE_MARKERS;
    for (i = 0; i < count; ++i) {
        const cgTeammateMarker_t *marker = &cg.teammateMarkers[i];
        vec3_t delta;
        float x, y, size;
        VectorSubtract(marker->origin, cg.refdef.vieworg, delta);
        if (!(DotProduct(delta, cg.refdef.viewaxis[0]) > 0) || !marker->shader) continue;
        if (!CG_ProjectWorldMarker(marker->origin, marker->worldWidth, marker->minimum, &x, &y, &size)) continue;
        trap_R_SetColor(marker->color);
        trap_R_DrawStretchPic(x, y, size, size, 0, 0, 1, 1, marker->shader);
        trap_R_SetColor(NULL);
    }
}

void CG_DrawPointsOfInterest(void) {
    int i;
    CG_DrawTeammateMarkers();
    CG_DrawDeadFriends();
    if (!cg.snap) return;
    for (i = 0; i < CG_MAX_POINTS_OF_INTEREST; ++i) {
        cgPointOfInterest_t *poi = &cg.pointsOfInterest[i];
        vec3_t delta;
        vec4_t color;
        float x, y, size;
        if (!poi->active) continue;
        if (CG_PoiExpired(poi)) { poi->active = qfalse; continue; }
        memcpy(color, poi->color, sizeof(color));
        if (poi->lifetime > 0) {
            int elapsed = cg.time - poi->startedAt;
            if (elapsed >= poi->fadeDelay) {
                /* 0x1002a1f6: 1 - (elapsed - delay) / (lifetime - delay),
                 * clamped to [0, 1]. lifetime > delay whenever lifetime > 0. */
                float alpha = 1.0f - (float)(elapsed - poi->fadeDelay) /
                                     (float)(poi->lifetime - poi->fadeDelay);
                color[3] = alpha < 0 ? 0 : alpha > 1 ? 1 : alpha;
            }
        }
        if (cgs.gametype >= GT_TEAM && poi->team == cg.snap->ps.persistant[PERS_TEAM]) continue;
        if ((cgs.settingsMask & CG_SETTING_QUAD_HOG) && poi->client == cg.snap->ps.clientNum) continue;
        /* 0x10011708: skip points on or behind the view plane (and NaN). */
        VectorSubtract(poi->origin, cg.refdef.vieworg, delta);
        if (!(DotProduct(delta, cg.refdef.viewaxis[0]) > 0)) continue;
        if (!poi->shader) continue;
        if (!CG_ProjectWorldMarker(poi->origin, poi->worldWidth, poi->minimum, &x, &y, &size)) continue;
        trap_R_SetColor(color);
        trap_R_DrawStretchPic(x, y, size, size, 0, 0, 1, 1, poi->shader);
        trap_R_SetColor(NULL);
    }
}
