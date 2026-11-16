/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Weapon colours. Observed:
 *
 * - The teammate test (0x1003cad0): outside team modes, or unless the view
 *   follows a player as a spectator, two teams are mates when equal and not
 *   free; a following spectator counts blue as the enemy and anything else
 *   as a mate. (Retail then consults a byte inside the force-team-model
 *   strings, whose meaning is open; it is not reproduced.)
 * - Grenades (0x100175f0, 0x10015190): the model's shaderRGBA is
 *   cg_weaponColor_grenade (0xRRGGBBAA). With cg_forceEnemyWeaponColor or
 *   cg_forceTeamWeaponColor at 1, an enemy's grenade takes all four bytes of
 *   cg_enemyUpperColor (enemy forcing on) and a teammate's, other than the
 *   viewer's own, cg_teamUpperColor (team forcing on); outside team modes
 *   everyone else is an enemy. */

#include "cg_local.h"

qboolean CG_WeaponColorTeammate(int myTeam, int otherTeam) {
    const qboolean following = cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW);
    if (cgs.gametype < GT_TEAM || !following || myTeam == TEAM_RED || myTeam == TEAM_BLUE)
        return myTeam == otherTeam && myTeam != TEAM_FREE;
    return otherTeam != TEAM_BLUE;
}

static void CG_ColorBytes(int color, byte rgba[4]) {
    rgba[0] = (byte)((color >> 24) & 0xff);
    rgba[1] = (byte)((color >> 16) & 0xff);
    rgba[2] = (byte)((color >> 8) & 0xff);
    rgba[3] = (byte)(color & 0xff);
}

void CG_GrenadeColor(const entityState_t *es, byte rgba[4]) {
    int color = cg_weaponColor_grenade.integer;
    if (cg.snap && (cg_forceTeamWeaponColor.integer == 1 || cg_forceEnemyWeaponColor.integer == 1)) {
        const int me = cg.snap->ps.clientNum, owner = es->clientNum;
        if (me >= 0 && me < MAX_CLIENTS && owner >= 0 && owner < MAX_CLIENTS) {
            const qboolean mate = CG_WeaponColorTeammate(cgs.clientinfo[me].team, cgs.clientinfo[owner].team);
            if ((cgs.gametype >= GT_TEAM || me != owner) && !mate) {
                if (cg_forceEnemyWeaponColor.integer == 1) color = cg_enemyUpperColor.integer;
            } else if (cg_forceTeamWeaponColor.integer == 1 && owner != me) {
                color = cg_teamUpperColor.integer;
            }
        }
    }
    CG_ColorBytes(color, rgba);
}

/* Rail colours (0x1004f280 float, 0x1004f420 byte variant): with either
 * force cvar at 1, a shooter other than the viewer is a teammate (team
 * modes, by the teammate test; elsewhere nobody) or an enemy; a forced
 * teammate takes cg_teamUpperColor and a forced enemy cg_enemyUpperColor,
 * at 0.75 of each colour byte. Returns qfalse when the beam keeps the
 * shooter's own colours. */
qboolean CG_ForcedRailColor(const clientInfo_t *ci, float rgb[3], byte bytes[3]) {
    const int me = cg.predictedPlayerState.clientNum;
    const int shooter = (int)(ci - cgs.clientinfo);
    qboolean mate;
    int color, i;
    if (cg_forceTeamWeaponColor.integer != 1 && cg_forceEnemyWeaponColor.integer != 1) return qfalse;
    if (me < 0 || me >= MAX_CLIENTS || shooter == me) return qfalse;
    mate = cgs.gametype < GT_TEAM ? qfalse : CG_WeaponColorTeammate(cgs.clientinfo[me].team, ci->team);
    if (mate && cg_forceTeamWeaponColor.integer == 1) color = cg_teamUpperColor.integer;
    else if (!mate && cg_forceEnemyWeaponColor.integer == 1) color = cg_enemyUpperColor.integer;
    else return qfalse;
    for (i = 0; i < 3; i++) {
        const int value = (color >> (24 - 8 * i)) & 0xff;
        rgb[i] = value / 255.0f * 0.75f;
        bytes[i] = (byte)(int)(value * 0.75f);
    }
    return qtrue;
}
