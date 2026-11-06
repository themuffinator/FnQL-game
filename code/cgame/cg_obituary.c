/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail obituaries (cgame 0x10019130) and the HUD kill feed (producer
 * 0x10018f60, ownerdraw CG_PLAYER_OBIT drawer 0x1002e9b0). Observed:
 *
 * - Names are the player string's "n" cut to 28 characters plus "^7"; in
 *   team modes the feed copies drop ^0..^7 pairs and control bytes so the
 *   team colors show.
 * - The weapon icon follows the means of death (cg_weapons[].weaponIcon);
 *   everything else uses icons/icon_frag, thaws icons/thaw.
 * - Single-party deaths and self kills print "%s %s." and feed the target
 *   alone; kills feed attacker, icon and target and print
 *   "%s %s %s%s"; a missing attacker prints "%s died.".
 * - The killer's center print ("You fragged ...") is gated by
 *   cg_drawFragMessages and drawn at y 90, scale 0.3.
 * - The feed holds at most cg_obituaryRowSize (5, 0..5) rows, each shown
 *   for two seconds and faded over the last 200 ms. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

#define OBITUARY_SLOTS 16
#define OBITUARY_LIFETIME 2000
#define OBITUARY_FADE 200

typedef struct {
    qboolean active;
    int time;
    char target[40];
    int targetTeam;
    char attacker[40];
    int attackerTeam;
    qboolean hasAttacker;
    qhandle_t icon;
} cgObituaryRow_t;

static cgObituaryRow_t cg_obituaries[OBITUARY_SLOTS];

void CG_ClearObituaries(void) { memset(cg_obituaries, 0, sizeof(cg_obituaries)); }

int CG_ObituaryRows(void) {
    int i, count = 0;
    for (i = 0; i < OBITUARY_SLOTS; ++i) if (cg_obituaries[i].active) ++count;
    return count;
}

/* Retail 0x100574c0 as used by the producer: drop ^0..^7 and controls. */
static void CG_ObituaryStrip(char *text) {
    const char *read = text;
    char *write = text;
    while (*read) {
        if (read[0] == '^' && read[1] >= '0' && read[1] <= '7') { read += 2; continue; }
        if ((unsigned char)*read >= 0x20) *write++ = *read;
        ++read;
    }
    *write = 0;
}

static void CG_ObituaryShift(void) {
    memmove(&cg_obituaries[0], &cg_obituaries[1], sizeof(cg_obituaries[0]) * (OBITUARY_SLOTS - 1));
    memset(&cg_obituaries[OBITUARY_SLOTS - 1], 0, sizeof(cg_obituaries[0]));
}

void CG_AddObituary(const char *target, const char *attacker, int targetTeam, int attackerTeam,
                    qboolean hasAttacker, qhandle_t icon) {
    int slot, rows = cg_obituaryRowSize.integer;
    cgObituaryRow_t *row;
    for (slot = 0; slot < OBITUARY_SLOTS && cg_obituaries[slot].active; ++slot) {}
    if (slot == OBITUARY_SLOTS) { CG_ObituaryShift(); slot = OBITUARY_SLOTS - 1; }
    row = &cg_obituaries[slot];
    memset(row, 0, sizeof(*row));
    row->active = qtrue;
    Q_strncpyz(row->target, target ? target : "", sizeof(row->target));
    Q_strncpyz(row->attacker, attacker ? attacker : "", sizeof(row->attacker));
    row->targetTeam = targetTeam;
    row->attackerTeam = attackerTeam;
    row->hasAttacker = hasAttacker;
    row->icon = icon;
    row->time = cg.time;
    if (cgs.gametype >= GT_TEAM) {
        CG_ObituaryStrip(row->attacker);
        CG_ObituaryStrip(row->target);
    }
    /* Keep only the newest cg_obituaryRowSize rows. */
    if (rows < 0) rows = 0;
    if (rows > OBITUARY_SLOTS) rows = OBITUARY_SLOTS;
    if (slot >= rows) {
        if (rows > 0) memmove(&cg_obituaries[0], &cg_obituaries[1], sizeof(cg_obituaries[0]) * rows);
        memset(&cg_obituaries[rows], 0, sizeof(cg_obituaries[0]) * (OBITUARY_SLOTS - rows));
    }
}

/* Ownerdraw CG_PLAYER_OBIT: one row per obituary from the rect's top,
 * advancing by the target name's height plus two units. */
void CG_DrawObituaries(const rectDef_t *rect, float scale) {
    int i, y;
    if (!rect || !(rect->y > -1e6f && rect->y < 1e6f)) return;
    y = (int)rect->y;
    for (i = 0; i < OBITUARY_SLOTS; ++i) {
        int start = cg_obituaries[i].time;
        if (!cg_obituaries[i].active) continue;
        if (start > 0 && cg.time >= start + OBITUARY_LIFETIME) { CG_ObituaryShift(); --i; }
    }
    CG_SetTextFont(0);
    for (i = 0; i < OBITUARY_SLOTS; ++i) {
        const cgObituaryRow_t *row = &cg_obituaries[i];
        vec4_t color = { 1, 1, 1, 1 };
        int age = cg.time - row->time, height, attackerWidth = 0;
        if (!row->active) continue;
        if (row->time <= 0 || age < 0 || age >= OBITUARY_LIFETIME) continue;
        if (OBITUARY_LIFETIME - age < OBITUARY_FADE) color[3] = (OBITUARY_LIFETIME - age) / (float)OBITUARY_FADE;
        height = CG_Text_HeightInFont(row->target, 0, scale, 0);
        if (row->hasAttacker) {
            memcpy(color, CG_OverheadTeamColor(row->attackerTeam), sizeof(float) * 3);
            CG_Text_Paint(rect->x, (float)y, scale, color, row->attacker, 0, 0, ITEM_TEXTSTYLE_NORMAL);
            attackerWidth = CG_Text_WidthInFont(row->attacker, 0, scale, 0);
        }
        if (row->icon) {
            color[0] = color[1] = color[2] = 1;
            trap_R_SetColor(color);
            CG_DrawPic(rect->x + attackerWidth + height / 2, (float)(y - height), (float)height, (float)height, row->icon);
            trap_R_SetColor(NULL);
        }
        memcpy(color, CG_OverheadTeamColor(row->targetTeam), sizeof(float) * 3);
        CG_Text_Paint(rect->x + attackerWidth + height * 2, (float)y, scale, color, row->target, 0, 0,
                      ITEM_TEXTSTYLE_NORMAL);
        y += height + 2;
    }
}

/* Retail 0x10019394: the icon each means of death shows. */
static qhandle_t CG_ObituaryIcon(int mod) {
    int weapon;
    switch (mod) {
    case MOD_SHOTGUN: weapon = WP_SHOTGUN; break;
    case MOD_GAUNTLET: weapon = WP_GAUNTLET; break;
    case MOD_MACHINEGUN: weapon = WP_MACHINEGUN; break;
    case MOD_GRENADE: case MOD_GRENADE_SPLASH: weapon = WP_GRENADE_LAUNCHER; break;
    case MOD_ROCKET: case MOD_ROCKET_SPLASH: weapon = WP_ROCKET_LAUNCHER; break;
    case MOD_PLASMA: case MOD_PLASMA_SPLASH: weapon = WP_PLASMAGUN; break;
    case MOD_RAILGUN: case MOD_RAILGUN_HEADSHOT: weapon = WP_RAILGUN; break;
    case MOD_LIGHTNING: case MOD_LIGHTNING_DISCHARGE: weapon = WP_LIGHTNING; break;
    case MOD_BFG: case MOD_BFG_SPLASH: weapon = WP_BFG; break;
    case MOD_NAIL: weapon = WP_NAILGUN; break;
    case MOD_CHAINGUN: weapon = WP_CHAINGUN; break;
    case MOD_PROXIMITY_MINE: weapon = WP_PROX_LAUNCHER; break;
    case MOD_GRAPPLE: weapon = WP_GRAPPLING_HOOK; break;
    case MOD_HMG: weapon = WP_HEAVY_MACHINEGUN; break;
    case MOD_THAW: return cgs.media.thawIcon;
    default: return cgs.media.obituaryIcon;
    }
    return cg_weapons[weapon].weaponIcon;
}

/* Retail single-party messages (0x10019394). */
static const char *CG_ObituarySingle(int mod, int attacker) {
    switch (mod) {
    case MOD_WATER: return "sank like a rock";
    case MOD_SLIME: return "melted";
    case MOD_LAVA: return "does a back flip into the lava";
    case MOD_CRUSH: return "was squished";
    case MOD_FALLING: return "cratered";
    case MOD_SUICIDE: return "suicides";
    case MOD_TARGET_LASER: return "saw the light";
    case MOD_TRIGGER_HURT: return "was in the wrong place";
    case MOD_SWITCHTEAM: return "switched teams";
    case MOD_THAW: return attacker == ENTITYNUM_WORLD ? "was auto-thawed" : NULL;
    default: return NULL;
    }
}

/* Retail self kills (0x1001949f): gendered, "it's" is retail's spelling. */
static const char *CG_ObituarySelf(int mod, gender_t gender) {
#define GENDERED(her, it, his) (gender == GENDER_FEMALE ? her : gender == GENDER_NEUTER ? it : his)
    switch (mod) {
    case MOD_GRENADE_SPLASH:
        return GENDERED("tripped on her own grenade", "tripped on its own grenade", "tripped on his own grenade");
    case MOD_ROCKET_SPLASH: return GENDERED("blew herself up", "blew itself up", "blew himself up");
    case MOD_PLASMA_SPLASH: return GENDERED("melted herself", "melted itself", "melted himself");
    case MOD_BFG_SPLASH: return "should have used a smaller gun";
    case MOD_PROXIMITY_MINE: return GENDERED("found her prox mine", "found it's prox mine", "found his prox mine");
    case MOD_KAMIKAZE: return "goes out with a bang";
    case MOD_LIGHTNING_DISCHARGE: return GENDERED("discharged herself", "discharged itself", "discharged himself");
    default: return GENDERED("killed herself", "killed itself", "killed himself");
    }
#undef GENDERED
}

/* Retail kill messages (table 0x10019acc). */
static const char *CG_ObituaryKill(int mod, const char **suffix) {
    *suffix = "";
    switch (mod) {
    case MOD_SHOTGUN: return "was gunned down by";
    case MOD_GAUNTLET: return "was pummeled by";
    case MOD_MACHINEGUN: return "was machinegunned by";
    case MOD_GRENADE: *suffix = "'s grenade"; return "ate";
    case MOD_GRENADE_SPLASH: *suffix = "'s shrapnel"; return "was shredded by";
    case MOD_ROCKET: *suffix = "'s rocket"; return "ate";
    case MOD_ROCKET_SPLASH: *suffix = "'s rocket"; return "almost dodged";
    case MOD_PLASMA: case MOD_PLASMA_SPLASH: *suffix = "'s plasmagun"; return "was melted by";
    case MOD_RAILGUN: case MOD_RAILGUN_HEADSHOT: return "was railed by";
    case MOD_LIGHTNING: return "was electrocuted by";
    case MOD_BFG: case MOD_BFG_SPLASH: *suffix = "'s BFG"; return "was blasted by";
    case MOD_TELEFRAG: *suffix = "'s personal space"; return "tried to invade";
    case MOD_NAIL: return "was nailed by";
    case MOD_CHAINGUN: *suffix = "'s Chaingun"; return "got lead poisoning from";
    case MOD_PROXIMITY_MINE: *suffix = "'s Prox Mine"; return "was too close to";
    case MOD_KAMIKAZE: *suffix = "'s Kamikaze blast"; return "falls to";
    case MOD_JUICED: return "was juiced by";
    case MOD_GRAPPLE: return "was caught by";
    case MOD_THAW: return "was thawed by";
    case MOD_LIGHTNING_DISCHARGE: return "was discharged by";
    case MOD_HMG: *suffix = "'s HMG"; return "was ripped up by";
    default: return "was killed by";
    }
}

/* Retail player string name: "n" cut to 28 characters plus "^7". */
static void CG_ObituaryName(const char *info, char *out, size_t size) {
    char name[29];
    Q_strncpyz(name, Info_ValueForKey(info, "n"), sizeof(name));
    Com_sprintf(out, size, "%s^7", name);
}

/* Retail killer center print (0x100195df), cg_drawFragMessages only. */
static void CG_ObituaryFragMessage(const char *targetName, int mod, qboolean teammate) {
    const char *text;
    if (!cg_drawFragMessages.integer) return;
    if (cgs.gametype == GT_RACE || cgs.gametype == GT_RED_ROVER) {
        text = va("You fragged %s.", targetName);
    } else if (cgs.gametype == GT_FREEZE) {
        if (mod == MOD_THAW) text = va("You thawed %s.", targetName);
        else if (!teammate) text = va("You froze %s.", targetName);
        else if (mod == MOD_TELEFRAG) text = va("You froze %s, your teammate.", targetName);
        else text = va("You froze %s, your teammate.\n\n^1Watch your fire!^7", targetName);
    } else if (cgs.gametype >= GT_TEAM) {
        if (!teammate) text = va("You fragged %s.", targetName);
        else if (mod == MOD_TELEFRAG) text = va("You fragged %s, your teammate.", targetName);
        else text = va("You fragged %s, your teammate.\n\n^1Watch your fire!^7", targetName);
    } else {
        /* Retail's g_training tutorial variant is not hosted by FnQL-game. */
        text = va("You fragged %s\n%s place with %i.", targetName,
                  CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
    }
    if (!cg_cameraOrbit.integer) CG_CenterPrintNative(text, 90, 0.3f);
}

void CG_RetailObituary(const entityState_t *ent) {
    int target = ent->otherEntityNum, attacker = ent->otherEntityNum2, mod = ent->eventParm;
    int targetTeam, attackerTeam = 0;
    const char *targetInfo, *attackerInfo = NULL, *message;
    char targetName[40], attackerName[40] = "";
    qboolean teammate;
    qhandle_t icon;

    if (target < 0 || target >= MAX_CLIENTS) CG_Error("CG_Obituary: target out of range");
    if (target == cg.clientNum) cg.killerName[0] = 0;
    targetTeam = cgs.clientinfo[target].team;
    teammate = cgs.gametype >= GT_TEAM && cg.snap && cg.snap->ps.persistant[PERS_TEAM] == targetTeam;
    if (attacker < 0 || attacker >= MAX_CLIENTS) {
        attacker = ENTITYNUM_WORLD;
    } else {
        attackerInfo = CG_ConfigString(CS_PLAYERS + attacker);
        attackerTeam = cgs.clientinfo[attacker].team;
    }
    targetInfo = CG_ConfigString(CS_PLAYERS + target);
    if (!targetInfo[0] || !cg.snap) return;
    CG_DeadTeammateMarker(ent);
    CG_ObituaryName(targetInfo, targetName, sizeof(targetName));
    icon = CG_ObituaryIcon(mod);

    message = CG_ObituarySingle(mod, attacker);
    if (attacker == target && mod != MOD_SWITCHTEAM)
        message = CG_ObituarySelf(mod, cgs.clientinfo[target].gender);
    if (message) {
        CG_Printf("%s %s.\n", targetName, message);
        CG_AddObituary(targetName, "", targetTeam, attackerTeam, qfalse, icon);
        return;
    }

    if (attacker == cg.snap->ps.clientNum) CG_ObituaryFragMessage(targetName, mod, teammate);
    /* Retail 0x10019765: a following spectator moves to the killer. */
    if (cg_followKiller.integer && (cg.snap->ps.pm_flags & PMF_FOLLOW) && target == cg.snap->ps.clientNum &&
        (cg.snap->ps.eFlags & EF_SPECTATOR) && attacker < MAX_CLIENTS) {
        cg.followClient = attacker;
        cg.followTime = cg.time + 400;
    }
    /* Retail tests the configstring pointer, not its contents. */
    if (attackerInfo) {
        CG_ObituaryName(attackerInfo, attackerName, sizeof(attackerName));
        if (target == cg.snap->ps.clientNum) {
            Q_strncpyz(cg.killerName, attackerName, sizeof(cg.killerName));
            cg.killerIcon = icon;
        }
    } else {
        attacker = ENTITYNUM_WORLD;
    }
    CG_AddObituary(targetName, attackerName, targetTeam, attackerTeam, attackerInfo != NULL, icon);
    if (attacker != ENTITYNUM_WORLD) {
        const char *suffix;
        message = CG_ObituaryKill(mod, &suffix);
        CG_Printf("%s %s %s%s\n", targetName, message, attackerName, suffix);
        return;
    }
    CG_Printf("%s died.\n", targetName);
}
