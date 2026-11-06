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
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

/* Retail cgame parsers: FFA 10045090, duel 10045440, race 10045db0,
 * TDM 10045f20, CA 100467e0, CTF 10046bf0, FT 100476a0, RR 10047de0.
 * These declarative wire-field maps adapt the contract to our score model.
 * See docs/CGAME.md for server-side corroboration and malformed-input policy. */
#define F(member) offsetof(score_t, member)
static const size_t ffa[] = {F(client),F(score),F(ping),F(time),F(accuracy),F(impressiveCount),
    F(excellentCount),F(gauntletCount),F(defendCount),F(assistCount),F(perfect),F(captures),
    F(alive),F(kills),F(deaths),F(bestWeapon),F(bestAccuracy),F(damageGiven)};
static const size_t legacy[] = {F(client),F(score),F(ping),F(time),F(scoreFlags),F(powerUps),
    F(accuracy),F(impressiveCount),F(excellentCount),F(gauntletCount),F(defendCount),
    F(assistCount),F(perfect),F(captures),F(alive),F(kills),F(deaths),F(bestWeapon)};
static const size_t small[] = {F(client),F(score),F(ping),F(time),F(captures),F(alive),F(kills),F(deaths)};
static const size_t race[] = {F(client),F(score),F(ping),F(time)};
static const size_t tdm[] = {F(client),F(team),F(score),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(impressiveCount),F(excellentCount),F(gauntletCount),
    F(teamKills),F(teamKilled),F(damageGiven)};
static const size_t ft[] = {F(client),F(team),F(score),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(impressiveCount),F(excellentCount),F(gauntletCount),
    F(assistCount),F(teamKills),F(teamKilled),F(damageGiven),F(alive)};
static const size_t ca[] = {F(client),F(team),F(score),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(bestAccuracy),F(damageGiven),F(impressiveCount),
    F(excellentCount),F(gauntletCount),F(perfect),F(alive)};
static const size_t ctf[] = {F(client),F(team),F(score),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(impressiveCount),F(excellentCount),F(gauntletCount),
    F(defendCount),F(assistCount),F(captures),F(perfect),F(alive)};
static const size_t rr[] = {F(client),F(score),F(roundScore),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(bestAccuracy),F(damageGiven),F(impressiveCount),
    F(excellentCount),F(defendCount),F(gauntletCount),F(assistCount),F(perfect),F(captures),F(alive)};
static const size_t duel[] = {F(client),F(score),F(ping),F(time),F(kills),F(deaths),
    F(accuracy),F(bestWeapon),F(damageGiven),F(impressiveCount),F(excellentCount),F(gauntletCount),F(perfect)};
static const size_t tdmStats[] = {F(suicides),F(teamKills),F(teamKilled),F(damageGiven),F(damageTaken),
    F(itemPickups[0]),F(itemPickups[1]),F(itemPickups[2]),F(itemPickups[3]),F(powerupPickups[0]),F(powerupPickups[1])};
static const size_t ctfStats[] = {F(suicides),F(damageGiven),F(damageTaken),F(itemPickups[0]),
    F(itemPickups[1]),F(itemPickups[2]),F(itemPickups[3]),F(powerupPickups[0]),F(powerupPickups[1]),
    F(powerupPickups[2]),F(powerupPickups[3]),F(powerupPickups[4])};
#undef F

typedef struct { int next, count; qboolean valid; } scoreReader_t;
static int CG_ScoreInt(scoreReader_t *reader) {
    char text[64], *p;
    unsigned magnitude = 0, limit;
    qboolean negative;
    if (!reader->valid || reader->next >= reader->count) { reader->valid = qfalse; return 0; }
    trap_Argv(reader->next++, text, sizeof(text));
    p = text; negative = *p == '-';
    if (*p == '-' || *p == '+') ++p;
    limit = negative ? (unsigned)INT_MAX + 1u : (unsigned)INT_MAX;
    if (!*p) reader->valid = qfalse;
    for (; *p; ++p) {
        unsigned digit = (unsigned)(*p - '0');
        if (digit > 9 || magnitude > (limit - digit) / 10u) { reader->valid = qfalse; return 0; }
        magnitude = magnitude * 10u + digit;
    }
    if (negative && magnitude == (unsigned)INT_MAX + 1u) return INT_MIN;
    return negative ? -(int)magnitude : (int)magnitude;
}
static float CG_ScoreWait(scoreReader_t *reader) {
    char text[64], *end;
    double value;
    if (!reader->valid || reader->next >= reader->count) { reader->valid = qfalse; return 0; }
    trap_Argv(reader->next++, text, sizeof(text));
    value = strtod(text, &end);
    if (end == text || *end || !(value >= 0 && value <= INT_MAX)) reader->valid = qfalse;
    return reader->valid ? (float)value : 0;
}
static void CG_ScoreFields(scoreReader_t *reader, score_t *score, const size_t *fields, int count) {
    int i;
    for (i = 0; i < count && reader->valid; ++i) {
        int value = CG_ScoreInt(reader);
        memcpy((char *)score + fields[i], &value, sizeof(value));
    }
}

/* Retail 100487b0: client indices only; vitals travel in the snapshot.
 * The exact inherited six-field form remains accepted for legacy senders. */
static qboolean CG_TeamInfoCommand(scoreReader_t *reader) {
    int count = CG_ScoreInt(reader), stride, i, j;
    int rows[TEAM_MAXOVERLAY][6] = {{0}};
    qboolean seen[MAX_CLIENTS] = {0};
    if (!reader->valid || count < 0 || count > TEAM_MAXOVERLAY) return qfalse;
    stride = reader->count - reader->next == count ? 1 : 6;
    if (reader->count - reader->next != count * stride) return qfalse;
    for (i = 0; i < count; ++i) {
        for (j = 0; j < stride; ++j) rows[i][j] = CG_ScoreInt(reader);
        if (!reader->valid || rows[i][0] < 0 || rows[i][0] >= MAX_CLIENTS || seen[rows[i][0]]) return qfalse;
        if (stride == 6 && (rows[i][1] < 0 || rows[i][1] >= MAX_LOCATIONS ||
            rows[i][4] < WP_NONE || rows[i][4] >= WP_NUM_WEAPONS)) return qfalse;
        seen[rows[i][0]] = qtrue;
    }
    memset(sortedTeamPlayers,0,sizeof(int) * TEAM_MAXOVERLAY);
    for (i = 0; i < count; ++i) {
        clientInfo_t *info = &cgs.clientinfo[rows[i][0]];
        sortedTeamPlayers[i] = rows[i][0];
        if (stride == 6) {
            info->location = rows[i][1]; info->health = rows[i][2]; info->armor = rows[i][3];
            info->curWeapon = rows[i][4]; info->powerups = rows[i][5];
        }
    }
    numSortedTeamPlayers = count;
    return qtrue;
}

/* Adapt the retail overlay's direct entity/own-player reads (1000a173..1bb)
 * to the inherited HUD's clientInfo fields. Apply only the current snapshot,
 * never an interpolated future state, a corpse or an unrelated entity. */
void CG_UpdateTeamInfoFromSnapshot(const snapshot_t *snapshot) {
    int i, powerups = 0;
    clientInfo_t *info;
    if (!snapshot || snapshot->numEntities < 0 || snapshot->numEntities > MAX_ENTITIES_IN_SNAPSHOT) return;
    for (i = 0; i < snapshot->numEntities; ++i) {
        const entityState_t *state = &snapshot->entities[i];
        if (state->number < 0 || state->number >= MAX_CLIENTS || state->clientNum != state->number) continue;
        if (state->eType != ET_PLAYER && state->eType != ET_INVISIBLE) continue;
        info = &cgs.clientinfo[state->number];
        info->health = state->health; info->armor = state->armor;
        info->location = state->location >= 0 && state->location < MAX_LOCATIONS ? state->location : 0;
        info->curWeapon = state->weapon >= WP_NONE && state->weapon < WP_NUM_WEAPONS ? state->weapon : WP_NONE;
        info->powerups = state->powerups;
    }
    if (snapshot->ps.clientNum < 0 || snapshot->ps.clientNum >= MAX_CLIENTS) return;
    info = &cgs.clientinfo[snapshot->ps.clientNum];
    info->health = snapshot->ps.stats[STAT_HEALTH]; info->armor = snapshot->ps.stats[STAT_ARMOR];
    info->curWeapon = snapshot->ps.weapon >= WP_NONE && snapshot->ps.weapon < WP_NUM_WEAPONS ? snapshot->ps.weapon : WP_NONE;
    for (i = 0; i < MAX_POWERUPS; ++i) if (snapshot->ps.powerups[i]) powerups |= 1 << i;
    info->powerups = powerups;
}

/* Auxiliary statistics address the scoreboard row, NOT the client number. */
static qboolean CG_ScoreDetails(const char *command, scoreReader_t *reader) {
    score_t updated;
    int row = CG_ScoreInt(reader), w;
    if (row < 0 || row >= cg.numScores) return qfalse;
    updated = cg.scores[row];
    if (!strcmp(command, "tdmstats")) CG_ScoreFields(reader, &updated, tdmStats, ARRAY_LEN(tdmStats));
    else if (!strcmp(command, "ctfstats")) CG_ScoreFields(reader, &updated, ctfStats, ARRAY_LEN(ctfStats));
    else {
        updated.damageGiven = CG_ScoreInt(reader); updated.damageTaken = CG_ScoreInt(reader);
        /* Retail castats includes reserved weapon slot 15. */
        for (w = 1; w < 16; ++w) {
            updated.weaponStats[w].accuracy = CG_ScoreInt(reader);
            updated.weaponStats[w].kills = CG_ScoreInt(reader);
        }
    }
    if (!reader->valid || reader->next != reader->count) return qfalse;
    cg.scores[row] = updated;
    return qtrue;
}

qboolean CG_ParseScoreCommand(const char *command) {
    const size_t *fields;
    int stride, header = 0, count, rows, i, j, teamScores[2] = {0,0}, teamStats[2][17] = {{0}};
    qboolean explicitTeam = qfalse, isDuel = qfalse, isRace = qfalse;
    qboolean clients[MAX_CLIENTS] = {0};
    score_t scores[MAX_CLIENTS] = {{0}};
    scoreReader_t reader = {1, trap_Argc(), qtrue};
    if (!strcmp(command,"tinfo")) {
        if (!CG_TeamInfoCommand(&reader)) goto invalid;
        return qtrue;
    }
    if (!strcmp(command,"acc") || !strcmp(command,"pstats")) {
        int accuracy[WP_NUM_WEAPONS];
        /* Retail 100481a0: fifteen slots, including WP_NONE. Percentages can
         * exceed 100; do not clamp server values. */
        for (i = 0; i < WP_NUM_WEAPONS; ++i) accuracy[i] = CG_ScoreInt(&reader);
        if (!reader.valid || reader.next != reader.count) goto invalid;
        memcpy(cg.weaponAccuracy,accuracy,sizeof(accuracy));
        return qtrue;
    }
    if (!strcmp(command, "tdmstats") || !strcmp(command, "ctfstats") || !strcmp(command, "castats")) {
        if (!CG_ScoreDetails(command, &reader)) goto invalid;
        return qtrue;
    }
    if (!strcmp(command, "scores_ad") || !strcmp(command, "adscores")) {
        int rounds[20];
        for (i = 0; i < 20; ++i) rounds[i] = CG_ScoreInt(&reader);
        for (i = 0; i < 2; ++i) teamScores[i] = CG_ScoreInt(&reader);
        if (!reader.valid || reader.next != reader.count) goto invalid;
        memcpy(cg.scoreRoundStats, rounds, sizeof(rounds));
        memcpy(cg.teamScores, teamScores, sizeof(teamScores));
        return qtrue;
    }
#define FORMAT(name, map) if (!strcmp(command, name)) { fields = map; stride = ARRAY_LEN(map); }
    FORMAT("scores_ffa", ffa)
    else FORMAT("scores", legacy)
    else FORMAT("smscores", small)
    else FORMAT("scores_rr", rr)
    else if (!strcmp(command, "scores_duel")) { fields = duel; stride = 91; isDuel = qtrue; }
    else if (!strcmp(command, "scores_race")) { fields = race; stride = 4; isRace = qtrue; }
    else if (!strcmp(command, "scores_tdm")) { fields = tdm; stride = ARRAY_LEN(tdm); header = 14; explicitTeam = qtrue; }
    else if (!strcmp(command, "scores_ft")) { fields = ft; stride = ARRAY_LEN(ft); header = 14; explicitTeam = qtrue; }
    else if (!strcmp(command, "scores_ctf")) { fields = ctf; stride = ARRAY_LEN(ctf); header = 17; explicitTeam = qtrue; }
    else if (!strcmp(command, "scores_ca")) { fields = ca; stride = ARRAY_LEN(ca); explicitTeam = qtrue; }
    else return qfalse;
#undef FORMAT
    for (i = 0; i < 2; ++i) for (j = 0; j < header; ++j)
        teamStats[i][header == 14 && j >= 9 ? j + 2 : j] = CG_ScoreInt(&reader);
    count = CG_ScoreInt(&reader);
    if (!reader.valid || count < 0 || count > MAX_CLIENTS || (isDuel && count > 2)) goto invalid;
    if (!isDuel && !isRace) {
        teamScores[0] = CG_ScoreInt(&reader); teamScores[1] = CG_ScoreInt(&reader);
    }
    /* Preserve the inherited 14-field command when its exact shape identifies
     * it, while retail's 'scores' command uses eighteen fields. */
    if (fields == legacy && reader.count - reader.next == count * 14) stride = 14;
    rows = count;
    if (isRace) {
        /* Retail counts playing clients but may append connected spectators.
         * Validate the suffix without changing its displayed row count. */
        if ((reader.count - reader.next) % stride) goto invalid;
        rows = (reader.count - reader.next) / stride;
        if (rows < count || rows > MAX_CLIENTS) goto invalid;
    }
    if (!reader.valid || reader.count - reader.next != rows * stride) goto invalid;
    for (i = 0; i < rows; ++i) {
        score_t *score = &scores[i];
        CG_ScoreFields(&reader, score, fields, isDuel ? ARRAY_LEN(duel) : stride);
        if (isDuel) {
            for (j = 0; j < 4; ++j) {
                score->itemPickups[j] = CG_ScoreInt(&reader);
                score->itemWait[j] = CG_ScoreWait(&reader);
            }
            for (j = 1; j < WP_NUM_WEAPONS; ++j) {
                score->weaponStats[j].hits = CG_ScoreInt(&reader);
                score->weaponStats[j].shots = CG_ScoreInt(&reader);
                score->weaponStats[j].accuracy = CG_ScoreInt(&reader);
                score->weaponStats[j].damage = CG_ScoreInt(&reader);
                score->weaponStats[j].kills = CG_ScoreInt(&reader);
            }
        }
        if (!reader.valid || score->client < 0 || score->client >= MAX_CLIENTS || clients[score->client] ||
            score->bestWeapon < 0 || score->bestWeapon >= WP_NUM_WEAPONS) goto invalid;
        clients[score->client] = qtrue;
        if (!explicitTeam) score->team = cgs.clientinfo[score->client].team;
        if (score->team < TEAM_FREE || score->team > TEAM_SPECTATOR) goto invalid;
        score->perfect = score->perfect != 0;
    }
    if (!reader.valid || reader.next != reader.count) goto invalid;
    /* Commit only complete, valid updates. Never redirect a bad client to 0. */
    memcpy(cg.scores, scores, sizeof(scores));
    memcpy(cg.teamScores, teamScores, sizeof(teamScores));
    memcpy(cg.scoreTeamStats, teamStats, sizeof(teamStats));
    cg.numScores = count;
    cg.selectedScore = 0;
    if (isDuel) CG_DuelScoresParsed(scores, count);
    for (i = 0; i < count; ++i) {
        cgs.clientinfo[scores[i].client].score = scores[i].score;
        if (fields == legacy) cgs.clientinfo[scores[i].client].powerups = scores[i].powerUps;
    }
    CG_SetScoreSelection(NULL);
    return qtrue;
invalid:
    CG_Printf("Ignored malformed %s scoreboard command\n", command);
    return qtrue;
}
