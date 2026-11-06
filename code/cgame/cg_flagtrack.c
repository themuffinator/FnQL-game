/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* CG_FLAG_STATUS (337), retail 0x10030240: the spectator flag track.
 * Observed:
 *
 * - Drawn for spectators (EF_SPECTATOR) in CTF, Overload, Harvester,
 *   Domination and Attack & Defend; One Flag and Freeze Tag draw nothing.
 * - The bases come from CS_RED/BLUE_FLAG_ORIGIN ("x", "y", "z" keys). Each
 *   flag starts at its last seen position (objective bases, 0x10aba984/990)
 *   and moves to a client entity below sv_maxclients with valid info that
 *   carries it (PW_REDFLAG / PW_BLUEFLAG in the entity powerups, at its
 *   lerped origin); a later client wins.
 * - With the bases apart and either flag away from base: a half-alpha red
 *   fill left of the rect x and a blue one right of it, each w / 2 wide and
 *   int(h / 8) high. An away red flag sits at x + w/2 - h/2 - w * d / D,
 *   d being its distance to the blue base clamped to the base distance D;
 *   an away blue flag at x - w/2 - h/2 + w * d / D, d measured to the red
 *   base. Each gets the track pointer (h/2 square at +h/4, on the bar) and
 *   its flag icon (h square, h/4 under the bar): stolen while taken (1),
 *   dropped otherwise. */

#include "cg_local.h"

static void CG_FlagTrackBase(int index, vec3_t origin) {
    const char *info = CG_ConfigString(index);
    origin[0] = (float)atof(Info_ValueForKey(info, "x"));
    origin[1] = (float)atof(Info_ValueForKey(info, "y"));
    origin[2] = (float)atof(Info_ValueForKey(info, "z"));
}

static float CG_FlagTrackDistance(const vec3_t a, const vec3_t b) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return (float)sqrt((double)dy * dy + (double)dx * dx + (double)dz * dz);
}

/* d / D as retail rounds it, after clamping d to D. */
static float CG_FlagTrackRatio(const vec3_t base, const vec3_t flag, float length) {
    float distance = CG_FlagTrackDistance(base, flag);
    if (distance > length) distance = length;
    return (float)(distance / length);
}

static void CG_FlagTrackIcon(float iconX, float y, float h, float thickness, qhandle_t icon) {
    const double quarter = h * 0.25;
    const float half = (float)(h * 0.5);
    CG_DrawPic((float)(quarter + iconX), y, half, half, CG_FlagTrackPointer());
    CG_DrawPic(iconX, (float)(thickness + y + quarter), h, h, icon);
}

void CG_DrawFlagTrack(float x, float y, float w, float h) {
    static const vec4_t red = { 1, 0, 0, 0.5f }, blue = { 0, 0, 1, 0.5f };
    vec3_t redBase, blueBase, redFlag, blueFlag;
    float length, thickness, half;
    int i;
    if (cgs.gametype < GT_CTF || cgs.gametype > GT_ATTACK_DEFEND || cgs.gametype == GT_FREEZE) return;
    if (!(cg.predictedPlayerState.eFlags & EF_SPECTATOR) || cgs.gametype == GT_1FCTF) return;
    thickness = (float)(int)(h * 0.125);
    CG_FlagTrackBase(CS_RED_FLAG_ORIGIN, redBase);
    CG_FlagTrackBase(CS_BLUE_FLAG_ORIGIN, blueBase);
    VectorCopy(cg.objectiveBases[1], redFlag);
    VectorCopy(cg.objectiveBases[2], blueFlag);
    for (i = 0; i < cgs.maxclients && i < MAX_CLIENTS; ++i) {
        const centity_t *cent = &cg_entities[i];
        const int client = cent->currentState.clientNum;
        if (client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid) continue;
        if (cent->currentState.powerups & (1 << PW_REDFLAG)) VectorCopy(cent->lerpOrigin, redFlag);
        if (cent->currentState.powerups & (1 << PW_BLUEFLAG)) VectorCopy(cent->lerpOrigin, blueFlag);
    }
    length = CG_FlagTrackDistance(blueBase, redBase);
    if (!(length > 0) || (!cgs.redflag && !cgs.blueflag)) return;
    half = (float)(w * 0.5);
    CG_FillRect(x - half, y, half, thickness, red);
    CG_FillRect(x, y, half, thickness, blue);
    if (cgs.redflag) {
        const float ratio = CG_FlagTrackRatio(blueBase, redFlag, length);
        const float iconX = (float)(0.5 * w + (x - h * 0.5) - w * ratio);
        CG_FlagTrackIcon(iconX, y, h, thickness, CG_FlagStatusIcon(1, cgs.redflag == FLAG_TAKEN ? 3 : 2));
    }
    if (cgs.blueflag) {
        const float ratio = CG_FlagTrackRatio(redBase, blueFlag, length);
        const float iconX = (float)(w * ratio + x - h * 0.5 - 0.5 * w);
        CG_FlagTrackIcon(iconX, y, h, thickness, CG_FlagStatusIcon(2, cgs.blueflag == FLAG_TAKEN ? 3 : 2));
    }
}
