/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail chat area (0x10006910/0x10006a10): current line lifetime, history
 * ring, history gates and counts, placement, scale clamp and styles. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_chatHistoryLength;
void CG_DrawChatArea(const rectDef_t *rect, float scale);
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }

static int font = -1, paintCount;
static struct { float x, y, scale; char text[300]; int limit, style; vec4_t color; } paints[32];
void CG_SetTextFont(int value) { font = value; }
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    CHECK(paintCount < 32 && font == 1 && !adjust);
    paints[paintCount].x = x; paints[paintCount].y = y; paints[paintCount].scale = scale;
    paints[paintCount].limit = limit; paints[paintCount].style = style;
    memcpy(paints[paintCount].color, color, sizeof(vec4_t));
    snprintf(paints[paintCount++].text, 300, "%s", text);
}
static void Draw(float scale) {
    rectDef_t rect;
    memset(&rect, 0, sizeof(rect)); rect.x = 5; rect.y = 287; rect.w = 634; rect.h = 120;
    paintCount = 0;
    CG_DrawChatArea(&rect, scale);
}

int main(void) {
    char longText[400];
    int i;
    cg_chatHistoryLength.integer = 6; cg.time = 1000;
    CG_ClearChatArea();
    Draw(.2f); CHECK(!paintCount);

    /* The current line: bottom edge, font 1, white, 256 limit, shadowed. */
    CG_ChatAreaAdd("Server: fraglimit changed to 1\n", 0, 0);
    Draw(.2f);
    CHECK(paintCount == 1 && !strcmp(paints[0].text, "Server: fraglimit changed to 1"));
    CHECK(paints[0].x == 5 && paints[0].y == 407 && paints[0].limit == 256 && paints[0].style == ITEM_TEXTSTYLE_SHADOWED);
    CHECK(paints[0].color[0] == 1 && paints[0].color[3] == 1 && paints[0].scale == .2f);
    /* Scale clamps to 0.1..0.22. */
    Draw(.5f); CHECK(paints[0].scale == .22f);
    Draw(.01f); CHECK(paints[0].scale == .1f);
    /* Two seconds, inclusive; then it moves to the history. */
    cg.time = 3000; Draw(.2f); CHECK(paintCount == 1);
    cg.time = 3001; Draw(.2f); CHECK(!paintCount);
    /* bchat seconds extend the lifetime. */
    CG_ChatAreaAdd("broadcast", 0, 5000); cg.time = 10001; Draw(.2f); CHECK(paintCount == 1);
    cg.time = 10002; Draw(.2f); CHECK(!paintCount);

    /* History: shown while +chat is held or the scoreboard is up. */
    for (i = 0; i < 8; ++i) { char line[16]; snprintf(line, sizeof(line), "line %d", i); CG_ChatAreaAdd(line, 0, 0); }
    Draw(.2f); CHECK(paintCount == 1 && !strcmp(paints[0].text, "line 7"));
    CG_ChatHistoryShow(qtrue); CHECK(CG_ChatHistoryShown());
    Draw(.2f);
    CHECK(paintCount == 7 && !strcmp(paints[1].text, "line 6") && paints[1].y == 394 && !strcmp(paints[6].text, "line 1"));
    CHECK(paints[6].y == 407 - 6 * 13 && paints[6].style == ITEM_TEXTSTYLE_SHADOWED);
    /* More than ten lines left to draw are unshadowed. */
    cg_chatHistoryLength.integer = 12; Draw(.2f);
    CHECK(paintCount == 13 && paints[1].style == ITEM_TEXTSTYLE_NORMAL && paints[2].style == ITEM_TEXTSTYLE_NORMAL);
    CHECK(paints[3].style == ITEM_TEXTSTYLE_SHADOWED);
    cg_chatHistoryLength.integer = 99; Draw(.2f); CHECK(paintCount == 25);
    cg_chatHistoryLength.integer = 0; Draw(.2f); CHECK(paintCount == 1);
    cg_chatHistoryLength.integer = 6;
    CG_ChatHistoryToggle(); CHECK(!CG_ChatHistoryShown()); Draw(.2f); CHECK(paintCount == 1);
    /* Scoreboard: two lines; intermission: four. */
    cg.scoreBoardShowing = qtrue; Draw(.2f); CHECK(paintCount == 3);
    cg.predictedPlayerState.pm_type = PM_INTERMISSION; Draw(.2f); CHECK(paintCount == 5);
    cg.scoreBoardShowing = qfalse; Draw(.2f); CHECK(paintCount == 1);
    CG_ChatHistoryToggle(); Draw(.2f); CHECK(paintCount == 5);
    cg.predictedPlayerState.pm_type = PM_NORMAL; CG_ChatHistoryShow(qfalse);

    /* Line limits: 254 characters, 72 during intermission. */
    memset(longText, 'a', sizeof(longText) - 1); longText[sizeof(longText) - 1] = 0;
    CG_ChatAreaAdd(longText, 0, 0); Draw(.2f); CHECK(strlen(paints[0].text) == 254);
    cg.intermissionStarted = qtrue; CG_ChatAreaAdd(longText, 0, 0); Draw(.2f); CHECK(strlen(paints[0].text) == 72);
    CG_ChatAreaAdd("short\n", 0, 0); Draw(.2f); CHECK(!strcmp(paints[0].text, "short"));
    cg.intermissionStarted = qfalse;
    CG_ChatAreaAdd("", 0, 0); Draw(.2f); CHECK(paintCount == 1 && !paints[0].text[0]);
    CG_ChatAreaAdd("\n", 0, 0); Draw(.2f); CHECK(!paints[0].text[0]);
    CG_ChatAreaAdd(NULL, 0, 0);

    /* clearChat empties both. */
    CG_ClearChatArea(); CG_ChatHistoryShow(qtrue); Draw(.2f);
    CHECK(paintCount == 6);
    for (i = 0; i < paintCount; ++i) CHECK(!paints[i].text[0]);
    CG_DrawChatArea(NULL, .2f);

    puts("chat area: lifetime, history gates, counts, layout, limits and clear OK");
    return 0;
}
