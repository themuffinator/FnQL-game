/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail vote and team-kill complaint prompt: text, precedence, placement. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_complaintWarning;
static struct { float x, y, scale; vec4_t color; char text[256]; int style; } paints[4];
static int numPaints, beeps;

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
int trap_Key_GetKey(const char *binding) {
    if (!strcmp(binding, "vote yes")) return 0x91;
    CHECK(!strcmp(binding, "vote no"));
    return 0x92;
}
void trap_Key_KeynumToStringBuf(int key, char *buffer, int size) {
    Q_strncpyz(buffer, key == 0x91 ? "F1" : key == 0x92 ? "F2" : "???", size);
}
void trap_S_StartLocalSound(sfxHandle_t sound, int channel) { CHECK(sound == 9 && channel == CHAN_LOCAL_SOUND); ++beeps; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    CHECK(numPaints < 4 && adjust == 0 && limit == 0);
    paints[numPaints].x = x; paints[numPaints].y = y; paints[numPaints].scale = scale; paints[numPaints].style = style;
    Vector4Copy(color, paints[numPaints].color);
    snprintf(paints[numPaints].text, sizeof(paints[numPaints].text), "%s", text);
    ++numPaints;
}

static void Reset(int gametype) {
    memset(&cg, 0, sizeof(cg)); memset(&cgs, 0, sizeof(cgs));
    cgs.gametype = gametype; cgs.media.talkSound = 9; cg.time = 50000;
    cg_complaintWarning.integer = 1;
    Q_strncpyz(cgs.clientinfo[3].name, "Visor", sizeof(cgs.clientinfo[3].name));
    CG_ClearComplaint();
    numPaints = beeps = 0;
}

int main(void) {
    char first[MAX_STRING_CHARS], second[256];

    /* Nothing to show. */
    Reset(GT_TEAM);
    CHECK(cgs.complaintClient == -1 && !cgs.complaintEndTime);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0 && !first[0] && !second[0]);
    CG_DrawVote(); CHECK(!numPaints && !beeps);

    /* A vote: seconds left of thirty, both counts and the bound keys. */
    cgs.voteTime = cg.time - 4500; cgs.voteYes = 2; cgs.voteNo = 1; cgs.voteModified = qtrue;
    Q_strncpyz(cgs.voteString, "map campgrounds", sizeof(cgs.voteString));
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2);
    CHECK(!strcmp(first, "VOTE(25s):map campgrounds Yes(F1):2 No(F2):1"));
    CHECK(!strcmp(second, "or press ESC then click Vote"));
    CG_DrawVote();
    CHECK(numPaints == 2 && beeps == 1 && !cgs.voteModified);
    CHECK(paints[0].x == 4 && paints[0].y == 300 && paints[0].scale == 0.22f && paints[0].style == ITEM_TEXTSTYLE_SHADOWED);
    CHECK(paints[0].color[0] == 1 && paints[0].color[1] == 1 && paints[0].color[2] == 0 && paints[0].color[3] == 1);
    CHECK(paints[1].x == 8 && paints[1].y == 312 && paints[1].color[2] == 1 && paints[1].style == ITEM_TEXTSTYLE_SHADOWED);
    numPaints = 0; CG_DrawVote(); CHECK(numPaints == 2 && beeps == 1);     /* one beep per change */
    cgs.voteTime = cg.time - 31000;                                        /* an overdue vote reads zero */
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2 && !strncmp(first, "VOTE(0s):", 9));

    /* A complaint outranks the vote for fifteen seconds and never beeps. */
    cgs.voteTime = cg.time - 4500; cgs.voteModified = qtrue;
    CG_SetComplaint(3);
    CHECK(cgs.complaintClient == 3 && cgs.complaintEndTime == cg.time + 15000);
    cg.time += 2500;
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2);
    CHECK(!strcmp(first, "File complaint against Visor for team-killing?"));
    CHECK(!strcmp(second, "Press 'F1' for Yes, or 'F2' for No (12s)"));
    numPaints = 0; CG_DrawVote();
    CHECK(numPaints == 2 && beeps == 1 && cgs.voteModified && paints[0].x == 4 && paints[1].x == 8);
    /* ... not in a demo, not without cg_complaintWarning, not once expired: the vote shows instead. */
    cg.demoPlayback = qtrue;
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2 && !strncmp(first, "VOTE(", 5));
    cg.demoPlayback = qfalse; cg_complaintWarning.integer = 0;
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2 && !strncmp(first, "VOTE(", 5));
    cg_complaintWarning.integer = 1; cg.time += 12500;
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 2 && !strncmp(first, "VOTE(", 5));

    /* Notices last ten seconds and stand alone, one unit further left. */
    Reset(GT_CTF);
    CG_SetComplaint(-1); CHECK(cgs.complaintEndTime == cg.time + 10000);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 1 && !first[0]);
    CHECK(!strcmp(second, "Your complaint has been filed."));
    CG_DrawVote();
    CHECK(numPaints == 1 && paints[0].x == 3 && paints[0].y == 300 && paints[0].color[2] == 1);
    CG_SetComplaint(-2); CG_VotePromptText(first, sizeof(first), second, sizeof(second));
    CHECK(!strcmp(second, "Your complaint has been dismissed."));
    CG_SetComplaint(-3); CG_VotePromptText(first, sizeof(first), second, sizeof(second));
    CHECK(!strcmp(second, "Comlaints cannot be filed against server admins."));
    CG_SetComplaint(-4); CG_VotePromptText(first, sizeof(first), second, sizeof(second));
    CHECK(!strcmp(second, "You received friendly fire from a server admin."));
    CG_SetComplaint(-5); CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0);
    cg.demoPlayback = qtrue; CG_SetComplaint(-1);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0);

    /* No complaints outside the team modes, nor in Red Rover. */
    Reset(GT_FFA); CG_SetComplaint(3);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0);
    Reset(GT_RED_ROVER); CG_SetComplaint(3);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0);

    /* A client number out of range is not indexed. */
    Reset(GT_TEAM); CG_SetComplaint(MAX_CLIENTS);
    CHECK(CG_VotePromptText(first, sizeof(first), second, sizeof(second)) == 0);

    puts("cgame vote prompt OK");
    return 0;
}
