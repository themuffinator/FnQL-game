/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail end-of-match ownerdraw text: match state, map name, details, end
 * condition, winner and the local player's finish line. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
static snapshot_t snapshot;
static char names[2][40] = { "Keel", "Visor" };
static int font = -1;
static char painted[256];
static float paintX;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
static int picShader = -1;
static float picW;
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) { (void)x; (void)y; (void)h; picShader = shader; picW = w; }
const char *CG_ConfigString(int index) {
    if (index == CS_AWARD_VALUABLE) return "3";
    if (index == CS_AWARD_DAMAGE) return "70";
    return index == CS_MESSAGE ? "Campgrounds" : "";
}
const char *CG_StandingsName(int slot) { return names[slot]; }
void CG_SetTextFont(int value) { font = value; }
/* Font 0: 10 units per character. */
int CG_Text_Width(const char *text, float scale, int limit) { (void)scale; (void)limit; return (int)strlen(text) * 10; }
int CG_Text_WidthInFont(const char *text, int f, float scale, int limit) {
    CHECK(f == 0 && !limit); (void)scale; return (int)strlen(text) * 10;
}
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    (void)y; (void)scale; (void)color; (void)adjust; (void)limit; (void)style;
    paintX = x; snprintf(painted, sizeof(painted), "%s", text);
}
static const char *Draw(int ownerDraw, int align) {
    vec4_t color = { 1, 1, 1, 1 };
    painted[0] = 0; paintX = -1;
    CHECK(CG_DrawEndSummary(ownerDraw, 300, 50, align, .3f, color, 0));
    return painted;
}

int main(void) {
    cg.snap = &snapshot;
    CHECK(!CG_DrawEndSummary(CG_PLAYER_HEALTH, 0, 0, 0, 1, NULL, 0));
    /* Match state and details. */
    cg.warmup = 5; CHECK(!strcmp(CG_MatchStateText(), "MATCH WARMUP"));
    cg.warmup = 0; CHECK(!strcmp(CG_MatchStateText(), "MATCH IN PROGRESS"));
    cg.predictedPlayerState.pm_type = PM_INTERMISSION; CHECK(!strcmp(Draw(CG_MATCH_STATE, 0), "MATCH SUMMARY"));
    CHECK(!strcmp(Draw(CG_MAP_NAME, 0), "Campgrounds"));
    cgs.gametype = GT_FFA;
    CHECK(!strcmp(Draw(CG_MATCH_DETAILS, 0), "MATCH SUMMARY - FFA - Campgrounds") && font == 0);
    CHECK(!strcmp(CG_GametypeTag(GT_FREEZE), "FT") && !strcmp(CG_GametypeTag(99), "ERR") && !strcmp(CG_GametypeTag(-1), "ERR"));

    /* End conditions (0x10034280). */
    CHECK(!strcmp(Draw(CG_MATCH_END_CONDITION, 0), "Highest score at the end of the game"));
    cgs.gametype = GT_RACE; CHECK(!strcmp(CG_MatchEndCondition(), "Fastest race time within the time limit"));
    cgs.gametype = GT_CTF; CHECK(!strcmp(CG_MatchEndCondition(), "First to reach the mercy limit"));
    cgs.capturelimit = 8; cgs.scores2 = 8; CHECK(!strcmp(CG_MatchEndCondition(), "First to reach the capture limit"));
    cgs.timelimit = 10; cgs.levelStartTime = 1000; cg.time = 601000;
    CHECK(!strcmp(CG_MatchEndCondition(), "Most flag captures within the time limit"));
    cgs.gametype = GT_CLAN_ARENA; CHECK(!strcmp(CG_MatchEndCondition(), "Most rounds won within the time limit"));
    cgs.gametype = GT_TEAM; CHECK(!strcmp(CG_MatchEndCondition(), "Highest score within the time limit"));
    cg.time = 600999; CHECK(!strcmp(CG_MatchEndCondition(), "Highest score at the end of the game"));
    cgs.gametype = GT_CLAN_ARENA; CHECK(!strcmp(CG_MatchEndCondition(), "First to reach the round limit"));
    cgs.gametype = GT_DOMINATION; CHECK(!strcmp(CG_MatchEndCondition(), "First to reach the score limit"));
    cgs.gametype = GT_ATTACK_DEFEND; CHECK(!strcmp(CG_MatchEndCondition(), "First to reach the score limit"));
    cgs.timelimit = 0;

    /* Winner (0x10033f00). */
    cgs.gametype = GT_FFA; cgs.scores1 = 1; cgs.scores2 = 0; snapshot.ps.pm_type = PM_INTERMISSION;
    CHECK(!strcmp(Draw(CG_MATCH_WINNER, 0), "Keel^7 WINS") && paintX == 300);
    Draw(CG_MATCH_WINNER, ITEM_ALIGN_CENTER); CHECK(paintX == 300 - 55);   /* 110 wide */
    cgs.scores1 = -999; CHECK(!strcmp(CG_MatchWinnerText(.3f), "Visor^7 WINS by forfeit"));
    cgs.scores1 = SCORE_NOT_PRESENT; cgs.scores2 = SCORE_NOT_PRESENT; CHECK(!CG_MatchWinnerText(.3f));
    Draw(CG_MATCH_WINNER, 0); CHECK(!painted[0]);
    cgs.scores1 = 4; cgs.scores2 = 2; snapshot.ps.pm_type = PM_NORMAL;
    CHECK(!strcmp(CG_MatchWinnerText(.3f), "Keel^7 leads with a score of 4"));
    /* Long names: cut under 192 units and given "...". */
    strcpy(names[0], "AVeryLongPlayerNameIndeed");   /* 250 wide */
    CHECK(!strcmp(CG_MatchWinnerText(.3f), "AVeryLongPlayerName...^7 leads with a score of 4"));
    strcpy(names[0], "TwentyCharacterName!"); CHECK(!strcmp(CG_MatchWinnerText(.3f), "TwentyCharacterName!^7 leads with a score of 4"));
    strcpy(names[0], "Keel");
    cgs.gametype = GT_RACE; cgs.scores1 = 65432;
    CHECK(!strcmp(CG_MatchWinnerText(.3f), "Keel^7 leads with a score of 1:05.432"));
    cgs.scores1 = 0; CHECK(!CG_MatchWinnerText(.3f));
    cgs.scores1 = INT_MAX; CHECK(!CG_MatchWinnerText(.3f));
    /* Team modes: literal team names, scores, ties. */
    cgs.gametype = GT_TEAM; cgs.scores1 = 3; cgs.scores2 = 7;
    CHECK(!strcmp(CG_MatchWinnerText(.3f), "^4Blue Team^7 leads with a score of 7"));
    cgs.scores2 = 3; CHECK(!strcmp(CG_MatchWinnerText(.3f), "Teams are tied with a score of 3"));
    snapshot.ps.pm_type = PM_INTERMISSION; CHECK(!strcmp(CG_MatchWinnerText(.3f), "^1Red Team^7 WINS by a score of 3 to 3"));
    cgs.scores2 = 9; CHECK(!strcmp(CG_MatchWinnerText(.3f), "^4Blue Team^7 WINS by a score of 9 to 3"));
    cgs.gametype = GT_RED_ROVER; cgs.scores1 = 9; cgs.scores2 = 2; CHECK(!strcmp(CG_MatchWinnerText(.3f), "Keel^7 WINS"));

    /* Your finish (0x100346e0). */
    cgs.gametype = GT_FFA; snapshot.ps.persistant[PERS_RANK] = 1; snapshot.ps.persistant[PERS_SCORE] = 0;
    CHECK(!strcmp(Draw(CG_PLYR_END_GAME_SCORE, 0), "You finished 2nd with a score of 0"));
    snapshot.ps.persistant[PERS_RANK] = 0 | RANK_TIED_FLAG; snapshot.ps.persistant[PERS_SCORE] = 5;
    CHECK(!strcmp(CG_EndGameScoreText(), "You finished 1st with a score of 5"));
    snapshot.ps.persistant[PERS_RANK] = 10; CHECK(!strcmp(CG_EndGameScoreText(), "You finished 11th with a score of 5"));
    snapshot.ps.persistant[PERS_RANK] = 20; CHECK(!strcmp(CG_EndGameScoreText(), "You finished 21st with a score of 5"));
    snapshot.ps.persistant[PERS_SCORE] = -999; CHECK(!strcmp(CG_EndGameScoreText(), "You forfeited the match."));
    snapshot.ps.persistant[PERS_SCORE] = SCORE_NOT_PRESENT; CHECK(!CG_EndGameScoreText()[0]);
    cgs.gametype = GT_RACE; CHECK(!CG_EndGameScoreText()[0]);
    cgs.gametype = GT_FFA; snapshot.ps.persistant[PERS_TEAM] = TEAM_SPECTATOR; CHECK(!CG_EndGameScoreText()[0]);
    snapshot.ps.persistant[PERS_TEAM] = TEAM_RED; cgs.gametype = GT_CTF; snapshot.ps.persistant[PERS_SCORE] = 12;
    CHECK(!strcmp(CG_EndGameScoreText(), "You finished with a score of 12."));
    snapshot.ps.persistant[PERS_DEFEND_COUNT] = 1; CHECK(!strcmp(CG_EndGameScoreText(), "You had 1 defend."));
    snapshot.ps.persistant[PERS_ASSIST_COUNT] = 2; CHECK(!strcmp(CG_EndGameScoreText(), "You had 2 assists."));
    snapshot.ps.persistant[PERS_CAPTURES] = 1; CHECK(!strcmp(CG_EndGameScoreText(), "You had 1 flag capture."));
    cgs.gametype = GT_HARVESTER; snapshot.ps.persistant[PERS_CAPTURES] = 3;
    CHECK(!strcmp(CG_EndGameScoreText(), "You captured 3 skulls."));
    cg.snap = NULL; CHECK(!CG_EndGameScoreText()[0]);

    /* Best weapon (0x100345f0): the selected row's weapon name. */
    cg.numScores = 2; cg.selectedScore = 1; cg.scores[1].bestWeapon = WP_MACHINEGUN;
    CHECK(!strcmp(Draw(CG_PLYR_BEST_WEAPON_NAME, 0), "Machine Gun"));
    cg.scores[1].bestWeapon = WP_HEAVY_MACHINEGUN; CHECK(!strcmp(CG_BestWeaponName(), "Heavy Machinegun"));
    cg.scores[1].bestWeapon = WP_BFG; CHECK(!strcmp(CG_BestWeaponName(), "BFG"));
    cg.scores[1].bestWeapon = WP_GRAPPLING_HOOK; CHECK(!CG_BestWeaponName());
    cg.scores[1].bestWeapon = 0; CHECK(!CG_BestWeaponName()); Draw(CG_PLYR_BEST_WEAPON_NAME, 0); CHECK(!painted[0]);
    cg.scores[1].bestWeapon = 15; CHECK(!CG_BestWeaponName());
    cg.selectedScore = 2; CHECK(!CG_BestWeaponName());
    cg.selectedScore = -1; CHECK(!CG_BestWeaponName());

    /* Awards (0x1003a0d0): the winner's model icon. */
    cgs.clientinfo[3].modelIcon = 44; cgs.clientinfo[0].modelIcon = 0;
    CHECK(CG_AwardClient(CG_MOST_VALUABLE_PLYR) == 3 && CG_AwardClient(CG_MOST_ACCURATE_PLYR) == 0);
    CHECK(CG_AwardClient(CG_PLAYER_HEALTH) == -1 && !CG_DrawAwardPicture(CG_PLAYER_HEALTH, 0, 0, 1, 1));
    CHECK(CG_DrawAwardPicture(CG_MOST_VALUABLE_PLYR, 1, 2, 30, 30) && picShader == 44 && picW == 30);
    picShader = -1; CHECK(CG_DrawAwardPicture(CG_MOST_ACCURATE_PLYR, 1, 2, 30, 30) && picShader == -1);
    CHECK(CG_DrawAwardPicture(CG_MOST_DAMAGEDEALT_PLYR, 1, 2, 30, 30) && picShader == -1);

    /* Player counts (0x10032f30). */
    cgs.maxclients = 8; cgs.clientinfo[2].infoValid = qtrue; cgs.clientinfo[6].infoValid = qtrue;
    cgs.clientinfo[9].infoValid = qtrue; cgs.gametype = GT_TOURNAMENT; cgs.teamSize = 2;
    CHECK(!strcmp(Draw(CG_PLAYER_COUNTS, ITEM_ALIGN_RIGHT), "2/8 Players") && paintX == 300 - 110);
    cgs.gametype = GT_FFA; CHECK(!strcmp(CG_PlayerCountText(), "2/2 Players"));
    cgs.gametype = GT_CTF; CHECK(!strcmp(CG_PlayerCountText(), "2/4 Players"));
    cgs.teamSize = 8; CHECK(!strcmp(CG_PlayerCountText(), "2/8 Players"));
    cgs.teamSize = 0; CHECK(!strcmp(CG_PlayerCountText(), "2/8 Players"));

    puts("end summary: state, map, details, end condition, winner and finish OK");
    return 0;
}
