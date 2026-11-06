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
#include "../../docs/contract/retail_menudef.h"

/* Retail flag-status icons, registered unconditionally at 0x10023a47..
 * 0x10023b51: neutral, red and blue sets of at-base, taken, dropped and
 * stolen, indexed [team][state] as the drawers address them. */
static qhandle_t flagIcons[3][4];
static qhandle_t flagTrackPointer;	/* 0x10023b66: gfx/2d/flag_status/track_pointer */

qhandle_t CG_FlagStatusIcon(int team, int state) {
    if (team < 0 || team > 2 || state < 0 || state > 3) return 0;
    return flagIcons[team][state];
}

qhandle_t CG_FlagTrackPointer(void) { return flagTrackPointer; }

void CG_RegisterFlagStatusIcons(void) {
    static const char *const teams[3] = { "", "red_", "blue_" };
    static const char *const states[4] = { "at_base", "taken", "dropped", "stolen" };
    char path[MAX_QPATH];
    int team, state;
    for (team = 0; team < 3; ++team) {
        for (state = 0; state < 4; ++state) {
            Com_sprintf(path, sizeof(path), "gfx/2d/flag_status/%sflag_%s.tga", teams[team], states[state]);
            flagIcons[team][state] = trap_R_RegisterShaderNoMip(path);
        }
    }
    flagTrackPointer = trap_R_RegisterShaderNoMip("gfx/2d/flag_status/track_pointer.tga");
}

static void CG_DrawFlagIcon(float x, float y, float w, float h, qhandle_t shader, qboolean white) {
    static const vec4_t opaque = { 1, 1, 1, 1 }; /* 0x10076214 */
    if (white) trap_R_SetColor(opaque);
    CG_DrawPic(x, y, w, h, shader);
    trap_R_SetColor(NULL);
}

/* Retail 0x10030a80, ownerdraws 283/288 (red) and 310/315 (blue). The
 * flag-status draws show at base, taken or dropped; the base-status draws
 * only at base or taken. One Flag shows the stolen icon, neutral while the
 * neutral flag is with that team. Freeze Tag and the individual modes draw
 * nothing; Harvester and Domination leave the menu color in place. */
qboolean CG_DrawTeamFlagStatus(int ownerDraw, float x, float y, float w, float h) {
    qboolean red = ownerDraw == CG_RED_FLAGSTATUS || ownerDraw == CG_RED_BASESTATUS;
    qboolean base = ownerDraw == CG_RED_BASESTATUS || ownerDraw == CG_BLUE_BASESTATUS;
    int team = red ? 1 : 2, status = red ? cgs.redflag : cgs.blueflag, state = 0;
    if (ownerDraw != CG_RED_FLAGSTATUS && ownerDraw != CG_RED_BASESTATUS &&
        ownerDraw != CG_BLUE_FLAGSTATUS && ownerDraw != CG_BLUE_BASESTATUS) return qfalse;
    if ((cgs.gametype < GT_CTF || cgs.gametype > GT_ATTACK_DEFEND) || cgs.gametype == GT_FREEZE) return qtrue;
    if (cgs.gametype == GT_1FCTF) {
        state = 3;
        if ((red && cgs.flagStatus == FLAG_TAKEN_RED) || (!red && cgs.flagStatus == FLAG_TAKEN_BLUE)) team = 0;
    } else if (status == FLAG_ATBASE) {
        state = 0;
    } else if (status == FLAG_TAKEN || base) {
        state = 1;
    } else if (status == FLAG_DROPPED) {
        state = 2;
    }
    CG_DrawFlagIcon(x, y, w, h, flagIcons[team][state],
                    cgs.gametype != GT_HARVESTER && cgs.gametype != GT_DOMINATION);
    return qtrue;
}

/* Retail 0x10030ff0, ownerdraw 54 in One Flag CTF: the neutral flag's
 * state; a carried flag uses the stolen icon, nudged 9 units down while
 * the carrying team is not ahead (or has no score) and up otherwise. */
void CG_DrawOneFlagStatus(float x, float y, float w, float h) {
    int status = cgs.flagStatus, state;
    float offset = 0;
    if (cgs.gametype != GT_1FCTF || status < 0 || status > FLAG_TAKEN_BLUE) return;
    state = status;
    if (status == FLAG_TAKEN_RED) {
        state = 3;
        offset = cgs.scores1 == SCORE_NOT_PRESENT || cgs.scores1 < cgs.scores2 ? 9 : -9;
    } else if (status == FLAG_TAKEN_BLUE) {
        state = 3;
        offset = cgs.scores2 == SCORE_NOT_PRESENT || cgs.scores2 <= cgs.scores1 ? 9 : -9;
    }
    CG_DrawFlagIcon(x, y + offset, w, h, flagIcons[0][state], qtrue);
}
