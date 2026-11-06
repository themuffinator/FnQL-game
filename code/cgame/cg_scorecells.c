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
#include <stdio.h>

qboolean CG_AccuracyOverlayVisible(void) {
    return cg.snap && cg.showAccuracy && (cg.snap->ps.pm_type != PM_SPECTATOR ||
        (cg.snap->ps.pm_flags & PMF_FOLLOW));
}

/* Retail +acc/-acc at 10006d60/10006db0; repeated presses request at most
 * once per second. This console action never changes input capture. */
void CG_SetAccuracyOverlay(qboolean show) {
    if (!show) { cg.showAccuracy = qfalse; return; }
    if (!cg.snap || (cg.snap->ps.pm_type == PM_SPECTATOR && !(cg.snap->ps.pm_flags & PMF_FOLLOW))) return;
    cg.showAccuracy = qtrue;
    if (!cg.demoPlayback && (cg.time < cg.accuracyRequestTime ||
        (unsigned)cg.time - (unsigned)cg.accuracyRequestTime > 1000u)) {
        cg.accuracyRequestTime = cg.time;
        trap_SendClientCommand("acc");
    }
}

/* Retail +pstats/-pstats (10006dd0/10006e20). Unless the predicted player
 * is a free spectator, request "pstats" at most once per second, sharing
 * the +acc request time (0x10aa693c). The held flag (0x10aa6944) has no
 * other reader in retail: the reply feeds the same accuracy data. */
void CG_SetPlayerStatsRequest(qboolean down) {
    if (!down) { cg.playerStatsHeld = qfalse; return; }
    if ((cg.predictedPlayerState.pm_type != PM_SPECTATOR ||
         (cg.predictedPlayerState.pm_flags & PMF_FOLLOW)) && !cg.demoPlayback &&
        (cg.time < cg.accuracyRequestTime ||
         (unsigned)cg.time - (unsigned)cg.accuracyRequestTime > 1000u)) {
        cg.accuracyRequestTime = cg.time;
        trap_SendClientCommand("pstats");
    }
    cg.playerStatsHeld = qtrue;
}

/* Retail 10035a10/10035b10: registered weapon icons from MG through HMG,
 * omitting grapple. The rectangle height is row spacing, width is icon size. */
void CG_DrawAccuracyColumn(qboolean icons, float x, float y, float width,
        float step, float scale, vec4_t color) {
    int weapon;
    if (icons) trap_R_SetColor(color);
    for (weapon = WP_MACHINEGUN; weapon < WP_NUM_WEAPONS; ++weapon) {
        char text[24];
        if (weapon == WP_GRAPPLING_HOOK || !cg_weapons[weapon].weaponIcon) continue;
        if (icons) CG_DrawPic(x,y,width,width,cg_weapons[weapon].weaponIcon);
        else {
            snprintf(text,sizeof(text),"%d%%",cg.weaponAccuracy[weapon]);
            CG_Text_Paint(x,y,scale,color,text,0,0,0);
        }
        y += step;
    }
    if (icons) trap_R_SetColor(NULL);
}

const char *CG_ScoreMode(void) {
    static const char *const modes[GT_MAX_GAME_TYPE] = {
        "ffa", "duel", "race", "tdm", "ca", "ctf", "1fctf", NULL, "har", "ft", "dom", "ad", "rr"
    };
    return cgs.gametype >= 0 && cgs.gametype < GT_MAX_GAME_TYPE ? modes[cgs.gametype] : NULL;
}
const char *CG_ScoreMenuName(qboolean end) {
    static char name[64];
    const char *mode = CG_ScoreMode();
    qboolean team = cgs.gametype >= GT_TEAM && cgs.gametype != GT_RED_ROVER;
    if (!mode) return team ? "teamscore_menu" : "score_menu";
    snprintf(name,sizeof(name),"%s%sscore_menu_%s",end ? "end" : "",team ? "team" : "",mode);
    return name;
}

qboolean CG_ScoreVisible2(int flags) {
    int team;
    qboolean spectator;
    if (!cg.snap) return qfalse;
    /* Retail 10031a40..10031a79: the duel slot's own client, or a
     * spectator (predicted PM_SPECTATOR), sees that player's items. */
    {
        const score_t *one = CG_DuelSlot(0), *two = CG_DuelSlot(1);
        qboolean watching = cg.predictedPlayerState.pm_type == PM_SPECTATOR;
        static const int fired[] = { WP_GAUNTLET, WP_MACHINEGUN, WP_SHOTGUN, WP_GRENADE_LAUNCHER,
            WP_ROCKET_LAUNCHER, WP_LIGHTNING, WP_RAILGUN, WP_PLASMAGUN, WP_BFG, WP_CHAINGUN, WP_NAILGUN,
            WP_PROX_LAUNCHER, WP_HEAVY_MACHINEGUN };
        int i;
        if ((flags & CG_SHOW_IF_PLYR1) && one && (cg.snap->ps.clientNum == one->client || watching)) return qtrue;
        if ((flags & CG_SHOW_IF_PLYR2) && two && (cg.snap->ps.clientNum == two->client || watching)) return qtrue;
        /* 10031b47: loadout cvar and which duel player is being followed. */
        if ((flags & CG_SHOW_IF_LOADOUT_ENABLED) && cg_loadout.integer) return qtrue;
        if ((flags & CG_SHOW_IF_LOADOUT_DISABLED) && !cg_loadout.integer) return qtrue;
        if ((flags & CG_SHOW_IF_1ST_PLYR_FOLLOWED) && one && one->client >= 0 && one->client < MAX_CLIENTS &&
            cg_entities[one->client].currentState.clientNum == cg.snap->ps.clientNum) return qtrue;
        if ((flags & CG_SHOW_IF_2ND_PLYR_FOLLOWED) && two && two->client >= 0 && two->client < MAX_CLIENTS &&
            cg_entities[two->client].currentState.clientNum == cg.snap->ps.clientNum) return qtrue;
        /* 10031b4f..10031cba: a weapon either duel player has fired. */
        for (i = 0; i < (int)ARRAY_LEN(fired); ++i)
            if ((flags & (CG_SHOW_IF_G_FIRED << i)) && one && two &&
                (one->weaponStats[fired[i]].shots || two->weaponStats[fired[i]].shots)) return qtrue;
    }
    /* Retail 10031a84..10031ae5 uses snapshot team and PM_SPECTATOR. */
    team = cg.snap->ps.persistant[PERS_TEAM];
    spectator = cg.snap->ps.pm_type == PM_SPECTATOR;
    return ((flags & CG_SHOW_IF_PLYR_IS_ON_RED_OR_SPEC) && (team == TEAM_RED || spectator)) ||
        ((flags & CG_SHOW_IF_PLYR_IS_ON_BLUE_OR_SPEC) && (team == TEAM_BLUE || spectator)) ||
        ((flags & CG_SHOW_IF_PLYR_IS_ON_RED_NO_SPEC) && team == TEAM_RED && !spectator) ||
        ((flags & CG_SHOW_IF_PLYR_IS_ON_BLUE_NO_SPEC) && team == TEAM_BLUE && !spectator);
}

int CG_ScoreRow(float feeder, int index) {
    int team = -1, i;
    if (index < 0) return -1;
    if (feeder == FEEDER_REDTEAM_LIST || feeder == FEEDER_REDTEAM_STATS) team = TEAM_RED;
    else if (feeder == FEEDER_BLUETEAM_LIST || feeder == FEEDER_BLUETEAM_STATS) team = TEAM_BLUE;
    else if (feeder != FEEDER_SCOREBOARD) return -1;
    for (i = 0; i < cg.numScores && i < MAX_CLIENTS; ++i) {
        if (team >= 0 && cg.scores[i].team != team) continue;
        if (index-- == 0) return cg.scores[i].client >= 0 && cg.scores[i].client < MAX_CLIENTS ? i : -1;
    }
    return -1;
}

static const char *CG_ScoreDamage(int damage) {
    static char text[32];
    if (damage < 1000) snprintf(text,sizeof(text),"%d",damage);
    else snprintf(text,sizeof(text),damage < 10000 ? "%.1fk" : "%.0fk",damage / 1000.0);
    return text;
}
static const char *CG_ScoreNumber(int number) {
    static char text[24];
    snprintf(text,sizeof(text),"%d",number);
    return text;
}
static const char *CG_ScorePercent(int number) {
    static char text[24];
    snprintf(text,sizeof(text),"%d%%",number);
    return text;
}
const char *CG_SelectedPlayerAccuracy(void) {
    /* Retail 10034590 reads the selected score row, not an average of weapon
     * percentages. acc requests include a score update from the server. */
    return CG_ScorePercent(cg.selectedScore >= 0 && cg.selectedScore < cg.numScores &&
        cg.selectedScore < MAX_CLIENTS ? cg.scores[cg.selectedScore].accuracy : 0);
}
static const char *CG_ScoreRatio(int a, int b) {
    static char text[48];
    snprintf(text,sizeof(text),"%d/%d",a,b);
    return text;
}

/* End-of-match stats tables, feeders 17/18 (retail dispatcher 0x10028830):
 * TDM and Freeze Tag use 0x10028300, Clan Arena 0x10027720, CTF, One Flag,
 * Harvester, Domination and A&D 0x10026cf0; other modes have none. */
static const char *CG_StatsName(const char *name) {
    /* Column 0: names at least 100 units wide at scale 0.16 are cut until
     * under 92 units and given "..." (the copy keeps 39 characters). */
    static char text[64];
    char buffer[40], prefix[40];
    int keep;
    if (CG_Text_Width(name, 0.16f, 0) < 100) return name;
    snprintf(buffer, sizeof(buffer), "%s", name);
    for (keep = (int)strlen(buffer); keep > 0; --keep) {
        snprintf(prefix, (size_t)keep, "%s", buffer);
        if (CG_Text_Width(prefix, 0.16f, 0) < 92) break;
    }
    snprintf(text, sizeof(text), "%s...", prefix);
    text[39] = 0;
    return text;
}

static const char *CG_StatsCell(const score_t *score, const clientInfo_t *info, int column) {
    static char text[48];
    int table = cgs.gametype == GT_TEAM || cgs.gametype == GT_FREEZE ? 0 :
                cgs.gametype == GT_CLAN_ARENA ? 1 :
                (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER ||
                 cgs.gametype == GT_DOMINATION || cgs.gametype == GT_ATTACK_DEFEND) ? 2 : -1;
    int value;
    if (table < 0) return "";
    if (column == 0) return CG_StatsName(info->name);
    if (column == 1) {
        if (score->score == SCORE_NOT_PRESENT || score->score == -999) return "^3-";
        snprintf(text, sizeof(text), "^3%i", score->score);
        return text;
    }
    if (column == 2) return CG_ScoreNumber(score->kills);
    if (column == 3) return CG_ScoreNumber(score->deaths - score->teamKilled - score->suicides);
    if (table == 0) {
        switch (column) {
        case 4: return CG_ScoreNumber(score->suicides);
        case 5: return CG_ScoreNumber(score->teamKills);
        case 6: return CG_ScoreNumber(score->teamKilled);
        case 7: return CG_ScoreNumber(score->teamKilled - score->teamKills - score->deaths + score->kills);
        case 8: return CG_ScoreNumber(score->damageGiven);
        case 9: return CG_ScoreNumber(score->damageTaken);
        case 10: snprintf(text, sizeof(text), "%i%%", score->accuracy); return text;
        case 11: case 12: case 13: case 14: return CG_ScoreNumber(score->itemPickups[column - 11]);
        case 15: case 16: return CG_ScoreNumber(score->powerupPickups[column - 15]);
        case 17: snprintf(text, sizeof(text), "%4i", score->time); return text;
        default: return "";
        }
    }
    if (table == 1) {
        /* Weapon columns 8..15: machinegun..plasma, then heavy machinegun. */
        static const int weapons[] = { WP_MACHINEGUN, WP_SHOTGUN, WP_GRENADE_LAUNCHER, WP_ROCKET_LAUNCHER,
                                       WP_LIGHTNING, WP_RAILGUN, WP_PLASMAGUN, WP_HEAVY_MACHINEGUN };
        switch (column) {
        case 4: return CG_ScoreNumber(score->damageGiven);
        case 5: return CG_ScoreNumber(score->damageTaken);
        case 6: snprintf(text, sizeof(text), "%i%%", score->accuracy); return text;
        case 7: return CG_ScoreNumber(score->weaponStats[WP_GAUNTLET].kills);
        case 16: snprintf(text, sizeof(text), "%4i", score->time); return text;
        default:
            if (column < 8 || column > 15) return "";
            value = weapons[column - 8];
            snprintf(text, sizeof(text), "^3%i ^7%i%%", score->weaponStats[value].kills,
                     score->weaponStats[value].accuracy);
            return text;
        }
    }
    switch (column) {
    case 4: return CG_ScoreNumber(score->suicides);
    case 5: return CG_ScoreNumber(score->kills - score->deaths);
    case 6: return CG_ScoreNumber(score->damageGiven);
    case 7: return CG_ScoreNumber(score->damageTaken);
    case 8: snprintf(text, sizeof(text), "%i%%", score->accuracy); return text;
    case 9: case 10: case 11: case 12: return CG_ScoreNumber(score->itemPickups[column - 9]);
    case 13: case 14: case 15: case 16: case 17: return CG_ScoreNumber(score->powerupPickups[column - 13]);
    case 18: snprintf(text, sizeof(text), "%4i", score->time); return text;
    default: return "";
    }
}

/* Retail leaves 10026160/100268b0/10027240/10027e30: column numbers are
 * menu contracts, not the former Q3 feeder columns. Keep lookup and formatting
 * independent of painting, so bounds and mode-specific values can be tested. */
const char *CG_ScoreCell(float feeder, int index, int column, qhandle_t *handle) {
    int row = CG_ScoreRow(feeder,index);
    const score_t *score;
    const clientInfo_t *info;
    qboolean team = feeder == FEEDER_REDTEAM_LIST || feeder == FEEDER_BLUETEAM_LIST;
    if (!handle) return "";
    *handle = -1;
    if (row < 0) return "";
    score = &cg.scores[row]; info = &cgs.clientinfo[score->client];
    if (!info->infoValid || info->team == TEAM_SPECTATOR) return "";
    if (feeder == FEEDER_REDTEAM_STATS || feeder == FEEDER_BLUETEAM_STATS) return CG_StatsCell(score, info, column);
    if (column == 1) {
        /* Every retail player feeder (0x10026294, 0x10026a08, 0x10027398,
         * 0x10027cf4, 0x10027f8c): muted icon, else the speaking icon. */
        if (CG_ClientMuted(score->client)) *handle = cgs.media.mutedIcon;
        else if (info->speaking) *handle = cgs.media.speakingIcon;
        return "";
    }
    if (cgs.gametype == GT_RACE && !team) {
        static char time[32];
        if (column == 0) *handle = info->modelIcon;
        if (column == 3) return info->name;
        if (column == 4) {
            if (score->score < 0) return "-";
            snprintf(time,sizeof(time),"%d:%02d.%03d",score->score/60000,(score->score/1000)%60,score->score%1000);
            return time;
        }
        if (column == 5) return CG_ScoreNumber(score->time);
        if (column == 6) return CG_ScoreNumber(score->ping);
        return "";
    }
    if (column == 0) { *handle = info->modelIcon; return ""; }
    if (column == 5) return info->name;
    if (column == 2 && !team && info->handicap < cgs.startingHealth && !info->botSkill) return CG_ScoreNumber(info->handicap);
    if (column == 3) {
        /* Retail 0x1002631a: during play the first carried powerup's icon
         * (0x100261b4, flight and frozen skipped); in warmup and at
         * intermission the green or red ready arrow. */
        if (!cg.warmup && cg.snap && cg.snap->ps.pm_type != PM_INTERMISSION) {
            int powerups = cg_entities[score->client].currentState.powerups, j;
            for (j = 1; j <= 16; ++j) {
                const gitem_t *item;
                if (!(powerups & (1 << j)) || j == PW_FLIGHT || j == PW_FROZEN) continue;
                item = CG_PowerupItem(j);
                if (item) { *handle = trap_R_RegisterShader(item->icon); break; }
            }
        } else if (cg.snap) {
            qboolean ready = score->client < 32 && (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client));
            *handle = trap_R_RegisterShader(ready ? "ui/assets/score/arrowg.tga" : "ui/assets/score/arrowr.tga");
        }
        return "";
    }
    if (column < 5) return ""; /* friend and country icons need their own contracts */
    if (!team) {
        switch (column) {
        case 6: return cgs.gametype == GT_TOURNAMENT ? CG_ScoreRatio(info->wins,info->losses) : "";
        case 7: return score->score == -9999 || score->score == -999 ? "-" : CG_ScoreNumber(score->score);
        case 8: return CG_ScoreRatio(score->kills,score->deaths);
        case 9: return CG_ScoreDamage(score->damageGiven);
        case 10:
            if (score->damageGiven > 0 && score->bestWeapon > 0 && score->bestWeapon < WP_NUM_WEAPONS)
                *handle = cg_weapons[score->bestWeapon].weaponIcon;
            return "";
        case 11: return score->damageGiven > 0 && score->bestWeapon > 0 ? CG_ScorePercent(score->bestAccuracy) : "";
        case 12: return CG_ScoreNumber(score->time);
        case 13: return CG_ScoreNumber(score->ping);
        default: return "";
        }
    }
    if (column == 6) return score->score == -9999 || score->score == -999 ? "-" : CG_ScoreNumber(score->score);
    if (cgs.gametype == GT_TEAM || cgs.gametype == GT_FREEZE) {
        if (column == 7) {
            if (cgs.gametype == GT_FREEZE) return CG_ScoreRatio(score->kills,score->deaths);
            /* Explicit unsigned operations retain wrap without signed UB. */
            return CG_ScoreNumber((int)((unsigned)score->kills + (unsigned)score->teamKilled - (unsigned)score->teamKills - (unsigned)score->deaths));
        }
        if (column == 8) return cgs.gametype == GT_FREEZE ? CG_ScoreNumber(score->assistCount) :
            (cgs.dmflags & 0x20000) ? CG_ScorePercent(score->accuracy) : CG_ScoreDamage(score->damageGiven);
        if (column == 9) return CG_ScoreNumber(score->ping);
    } else if (cgs.gametype == GT_CLAN_ARENA) {
        if (column == 7) return CG_ScoreRatio(score->kills,score->deaths);
        if (column == 8) return CG_ScoreDamage(score->damageGiven);
        if (column == 9 && score->damageGiven > 0 && score->bestWeapon > 0 && score->bestWeapon < WP_NUM_WEAPONS)
            *handle = cg_weapons[score->bestWeapon].weaponIcon;
        if (column == 10) return score->damageGiven > 0 && score->bestWeapon > 0 ? CG_ScorePercent(score->bestAccuracy) : "";
        if (column == 11) return CG_ScoreNumber(score->ping);
    } else {
        if (column == 7) return CG_ScoreRatio(score->kills,score->deaths);
        if (column == 8) return CG_ScoreNumber(score->captures);
        if (column == 9) return CG_ScoreNumber(score->assistCount);
        if (column == 10) return CG_ScoreNumber(score->defendCount);
        if (column == 11) return CG_ScoreNumber(score->ping);
    }
    return "";
}
