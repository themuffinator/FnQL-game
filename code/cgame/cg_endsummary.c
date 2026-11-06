/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* End-of-match and match-state text ownerdraws (retail handlers reached
 * from the ownerdraw dispatcher 0x1003b0f0). Observed:
 *
 * - CG_MATCH_STATE (339, 0x10034360): "MATCH SUMMARY" at intermission,
 *   otherwise "MATCH WARMUP" or "MATCH IN PROGRESS"; item font.
 * - CG_MAP_NAME (18, 0x100343d0): the CS_MESSAGE map title; item font.
 * - CG_MATCH_DETAILS (8, 0x10034420): "state - mode - map", where mode is
 *   retail's short gametype tag (0x10001080); font 0.
 * - CG_MATCH_END_CONDITION (9, 0x10034280): why the match ended; font 0.
 * - CG_MATCH_WINNER (82, 0x10033f00): the winner at intermission, the
 *   leader before it; centered on the rect origin for center alignment.
 * - CG_PLYR_END_GAME_SCORE (90, 0x100346e0): the local player's finish. */

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

static void CG_SummaryPaint(float x, float y, float scale, vec4_t color, const char *text, int style) {
    CG_Text_Paint(x, y, scale, color, text, 0, 0, style);
}

static qboolean CG_SummaryTeamMode(void) {
    return cgs.gametype >= GT_TEAM && cgs.gametype != GT_RED_ROVER;
}

const char *CG_MatchStateText(void) {
    if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) return "MATCH SUMMARY";
    return cg.warmup ? "MATCH WARMUP" : "MATCH IN PROGRESS";
}

/* Retail 0x10001080. */
const char *CG_GametypeTag(int gametype) {
    static const char *const tags[] = { "FFA", "DUEL", "RACE", "TDM", "CA", "CTF", "1F", "OB",
                                        "HAR", "FT", "DOM", "AD", "RR" };
    return gametype >= 0 && gametype < (int)ARRAY_LEN(tags) ? tags[gametype] : "ERR";
}

/* Retail 0x10034280. */
const char *CG_MatchEndCondition(void) {
    int mode = cgs.gametype;
    if (mode == GT_RACE) return "Fastest race time within the time limit";
    if (cgs.timelimit && (int64_t)cg.time - cgs.levelStartTime >= (int64_t)cgs.timelimit * 60000) {
        if (mode == GT_CTF) return "Most flag captures within the time limit";
        if (mode == GT_CLAN_ARENA) return "Most rounds won within the time limit";
        return "Highest score within the time limit";
    }
    if (mode == GT_CTF) {
        if (cgs.capturelimit && (cgs.scores1 >= cgs.capturelimit || cgs.scores2 >= cgs.capturelimit))
            return "First to reach the capture limit";
        return "First to reach the mercy limit";
    }
    if (mode == GT_CLAN_ARENA) return "First to reach the round limit";
    if (mode == GT_DOMINATION || mode == GT_ATTACK_DEFEND) return "First to reach the score limit";
    return "Highest score at the end of the game";
}

/* Retail 0x10018e60 without color (first argument 0). */
static const char *CG_PlainPlace(int rank) {
    static char text[32];
    const char *tied = "";
    char number[16];
    if (rank & RANK_TIED_FLAG) { rank &= ~RANK_TIED_FLAG; tied = "Tied for "; }
    if (rank == 1) Q_strncpyz(number, "1st", sizeof(number));
    else if (rank == 2) Q_strncpyz(number, "2nd", sizeof(number));
    else if (rank == 3) Q_strncpyz(number, "3rd", sizeof(number));
    else if (rank == 11 || rank == 12 || rank == 13) Com_sprintf(number, sizeof(number), "%ith", rank);
    else if (rank % 10 == 1) Com_sprintf(number, sizeof(number), "%ist", rank);
    else if (rank % 10 == 2) Com_sprintf(number, sizeof(number), "%ind", rank);
    else if (rank % 10 == 3) Com_sprintf(number, sizeof(number), "%ird", rank);
    else Com_sprintf(number, sizeof(number), "%ith", rank);
    Com_sprintf(text, sizeof(text), "%s%s", tied, number);
    return text;
}

/* Retail 0x100346e0; empty for Race and spectators. */
const char *CG_EndGameScoreText(void) {
    static char text[64];
    const playerState_t *ps;
    int score;
    if (!cg.snap) return "";
    ps = &cg.snap->ps;
    if (cgs.gametype == GT_RACE || ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) return "";
    score = ps->persistant[PERS_SCORE];
    if (cgs.gametype >= GT_TEAM) {
        int captures = ps->persistant[PERS_CAPTURES], assists = ps->persistant[PERS_ASSIST_COUNT];
        int defends = ps->persistant[PERS_DEFEND_COUNT];
        if (captures > 0)
            Com_sprintf(text, sizeof(text), cgs.gametype == GT_HARVESTER ? "You captured %d skull%s." :
                        "You had %d flag capture%s.", captures, captures == 1 ? "" : "s");
        else if (assists > 0) Com_sprintf(text, sizeof(text), "You had %d assist%s.", assists, assists == 1 ? "" : "s");
        else if (defends > 0) Com_sprintf(text, sizeof(text), "You had %d defend%s.", defends, defends == 1 ? "" : "s");
        else if (score == SCORE_NOT_PRESENT) return "";
        else if (score == -999) return "You forfeited the match.";
        else Com_sprintf(text, sizeof(text), "You finished with a score of %d.", score);
        return text;
    }
    if (score == SCORE_NOT_PRESENT) return "";
    if (score == -999) return "You forfeited the match.";
    /* Retail 0x100347bc clears the tie flag first: no "Tied for". */
    Com_sprintf(text, sizeof(text), "You finished %s with a score of %d",
                CG_PlainPlace((ps->persistant[PERS_RANK] + 1) & ~RANK_TIED_FLAG), score);
    return text;
}

/* Retail 0x10001320: "-m:ss.mmm" with a zero-padded seconds field. */
static const char *CG_SummaryRaceTime(int milliseconds) {
    static char text[32];
    float seconds = (float)(milliseconds < 0 ? -(double)milliseconds : milliseconds) / 1000.0f;
    int minutes = (int)seconds / 60;
    float rest = seconds - minutes * 60.0f;
    /* The C runtime rounds the milliseconds as retail's does; the bg_lib
     * formatter behind Com_sprintf truncates. */
    snprintf(text, sizeof(text), "%s%d:%s%.03f", milliseconds < 0 ? "-" : "", minutes, rest < 10 ? "0" : "", (double)rest);
    return text;
}

/* Retail 0x10033f00: the leader's line, NULL when there is nothing to say. */
const char *CG_MatchWinnerText(float scale) {
    static char text[128];
    char name[64];
    int first = cgs.scores1, second = cgs.scores2, lead, trail;
    qboolean team = CG_SummaryTeamMode();
    if (!team) {
        const char *source = CG_StandingsName(first != -999 ? 0 : 1);
        lead = first != -999 ? first : second;
        trail = first != -999 ? second : first;
        Q_strncpyz(name, source, 40);
        if (CG_Text_WidthInFont(name, 0, scale, 0) > 200) {
            int length = (int)strlen(name);
            char prefix[40];
            while (length > 1) {
                Q_strncpyz(prefix, source, length);
                if (CG_Text_WidthInFont(prefix, 0, scale, 0) < 192) break;
                --length;
            }
            Com_sprintf(name, sizeof(name), "%s...", prefix);
        }
        if (first == second && first == SCORE_NOT_PRESENT) return NULL;
    } else if (first < second) {
        Q_strncpyz(name, "^4Blue Team", sizeof(name)); lead = second; trail = first;
    } else {
        Q_strncpyz(name, "^1Red Team", sizeof(name)); lead = first; trail = second;
    }
    if (cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION) {
        if (first == -999 || second == -999) Com_sprintf(text, sizeof(text), "%s^7 WINS by forfeit", name);
        else if (team) Com_sprintf(text, sizeof(text), "%s^7 WINS by a score of %d to %d", name, lead, trail);
        else Com_sprintf(text, sizeof(text), "%s^7 WINS", name);
        return text;
    }
    if (team && first == second) {
        Com_sprintf(text, sizeof(text), "Teams are tied with a score of %d", lead);
        return text;
    }
    if (cgs.gametype != GT_RACE) {
        Com_sprintf(text, sizeof(text), "%s^7 leads with a score of %d", name, lead);
        return text;
    }
    if (lead == INT_MAX || lead <= 0) return NULL;
    Com_sprintf(text, sizeof(text), "%s^7 leads with a score of %s", name, CG_SummaryRaceTime(lead));
    return text;
}

/* Retail 0x100345f0: the selected scoreboard row's best weapon. Grapple
 * and unknown values draw nothing. */
const char *CG_BestWeaponName(void) {
    static const char *const names[] = { "Gauntlet", "Machine Gun", "Shotgun", "Grenade", "Rocket Launcher",
        "Lightning Gun", "Rail Gun", "Plasma Gun", "BFG", NULL, "Nail Gun", "Proximity Mines", "Chain Gun",
        "Heavy Machinegun" };
    int weapon;
    if (cg.selectedScore < 0 || cg.selectedScore >= cg.numScores) return NULL;
    weapon = cg.scores[cg.selectedScore].bestWeapon - 1;
    if (weapon < 0 || weapon >= (int)ARRAY_LEN(names)) return NULL;
    return names[weapon];
}

/* Retail 0x1003a0d0: award winners come from CS_AWARD_OFFENSE, _DEFENSE,
 * _VALUABLE, _ITEM_CONTROL, _ACCURACY and _DAMAGE (CG_ParseServerinfo
 * 0x1004972c). Retail shows the Steam avatar with cg_drawProfileImages when
 * one exists, else the model icon; FnQL-game has no avatar import wired
 * (slot 127), so it shows the model icon. */
int CG_AwardClient(int ownerDraw) {
    int index;
    switch (ownerDraw) {
    case CG_MOST_VALUABLE_OFFENSIVE_PLYR: index = CS_AWARD_OFFENSE; break;
    case CG_MOST_VALUABLE_DEFENSIVE_PLYR: index = CS_AWARD_DEFENSE; break;
    case CG_MOST_VALUABLE_PLYR: index = CS_AWARD_VALUABLE; break;
    case CG_BEST_ITEMCONTROL_PLYR: index = CS_AWARD_ITEM_CONTROL; break;
    case CG_MOST_ACCURATE_PLYR: index = CS_AWARD_ACCURACY; break;
    case CG_MOST_DAMAGEDEALT_PLYR: index = CS_AWARD_DAMAGE; break;
    default: return -1;
    }
    return atoi(CG_ConfigString(index));
}

qboolean CG_DrawAwardPicture(int ownerDraw, float x, float y, float w, float h) {
    int client = CG_AwardClient(ownerDraw);
    if (ownerDraw < CG_MOST_VALUABLE_OFFENSIVE_PLYR || ownerDraw > CG_MOST_DAMAGEDEALT_PLYR) return qfalse;
    if (client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].modelIcon) return qtrue;
    CG_DrawPic(x, y, w, h, cgs.clientinfo[client].modelIcon);
    return qtrue;
}

/* Retail 0x10032f30: clients with info over sv_maxclients, or over
 * teamsize (twice that in team modes, capped) outside Duel. */
const char *CG_PlayerCountText(void) {
    static char text[32];
    int i, connected = 0, limit = cgs.maxclients;
    for (i = 0; i < cgs.maxclients && i < MAX_CLIENTS; ++i) if (cgs.clientinfo[i].infoValid) ++connected;
    if (cgs.gametype != GT_TOURNAMENT && cgs.teamSize) {
        limit = cgs.gametype >= GT_TEAM ? cgs.teamSize * 2 : cgs.teamSize;
        if (limit > cgs.maxclients) limit = cgs.maxclients;
    }
    Com_sprintf(text, sizeof(text), "%d/%d Players", connected, limit);
    return text;
}

qboolean CG_DrawEndSummary(int ownerDraw, float x, float y, int align, float scale, vec4_t color, int style) {
    const char *text;
    switch (ownerDraw) {
    case CG_PLAYER_COUNTS:
        text = CG_PlayerCountText();
        if (align == ITEM_ALIGN_CENTER) x -= CG_Text_Width(text, scale, 0) * 0.5f;
        else if (align == ITEM_ALIGN_RIGHT) x -= CG_Text_Width(text, scale, 0);
        CG_SummaryPaint(x, y, scale, color, text, style);
        return qtrue;
    case CG_MATCH_STATE:
        CG_SummaryPaint(x, y, scale, color, CG_MatchStateText(), style);
        return qtrue;
    case CG_MAP_NAME:
        CG_SummaryPaint(x, y, scale, color, CG_ConfigString(CS_MESSAGE), style);
        return qtrue;
    case CG_MATCH_DETAILS:
        CG_SetTextFont(0);
        CG_SummaryPaint(x, y, scale, color, va("%s - %s - %s", CG_MatchStateText(), CG_GametypeTag(cgs.gametype),
                                               CG_ConfigString(CS_MESSAGE)), style);
        return qtrue;
    case CG_MATCH_END_CONDITION:
        CG_SetTextFont(0);
        CG_SummaryPaint(x, y, scale, color, CG_MatchEndCondition(), style);
        return qtrue;
    case CG_MATCH_WINNER:
        CG_SetTextFont(0);
        text = CG_MatchWinnerText(scale);
        if (text) {
            float start = x;
            if (align == ITEM_ALIGN_CENTER) start -= CG_Text_WidthInFont(text, 0, scale, 0) / 2;
            CG_SummaryPaint(start, y, scale, color, text, style);
        }
        return qtrue;
    case CG_PLYR_BEST_WEAPON_NAME:
        text = CG_BestWeaponName();
        if (text) CG_SummaryPaint(x, y, scale, color, text, style);
        return qtrue;
    case CG_PLYR_END_GAME_SCORE:
        CG_SetTextFont(0);
        CG_SummaryPaint(x, y, scale, color, CG_EndGameScoreText(), style);
        return qtrue;
    default:
        return qfalse;
    }
}
