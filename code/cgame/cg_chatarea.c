/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Retail HUD chat area (cgame producer 0x10006910, ownerdraw
 * CG_AREA_NEW_CHAT drawer 0x10006a10). Observed:
 *
 * - "print", "chat", "tchat" and "bchat" lines become the current message,
 *   shown for two seconds plus any bchat duration. A trailing newline is
 *   dropped; lines keep at most 254 characters, 72 during intermission.
 * - A replaced or expired message moves into a 24-line history.
 * - The current message sits at the rect's bottom edge. History lines go
 *   upwards 13 units apart while +chat/togglechathistory is on or the
 *   scoreboard is up: cg_chatHistoryLength (6, 0..24) lines, 2 with the
 *   scoreboard, 4 at intermission.
 * - Text uses font 1, white, scale clamped to 0.1..0.22, a 256-character
 *   limit and shadow style 3; while more than ten history lines remain to
 *   be drawn they are unshadowed. */

#include "cg_local.h"
#include "../ui/ui_shared.h"

#define CHAT_HISTORY 24
#define CHAT_LIFETIME 2000

typedef struct {
    int time, expire, type;
    char text[256];
} cgChatLine_t;

static cgChatLine_t chatCurrent;
static cgChatLine_t chatHistory[CHAT_HISTORY];
static int chatHead = CHAT_HISTORY - 1;
static qboolean chatHistoryShown;

void CG_ClearChatArea(void) {
    memset(&chatCurrent, 0, sizeof(chatCurrent));
    memset(chatHistory, 0, sizeof(chatHistory));
    chatHead = CHAT_HISTORY - 1;
}

static void CG_ChatAreaPush(void) {
    chatHead = (chatHead + 1) % CHAT_HISTORY;
    chatHistory[chatHead] = chatCurrent;
}

void CG_ChatAreaAdd(const char *text, int type, int extraMsec) {
    size_t length, limit = cg.intermissionStarted ? 73 : 255;
    if (!text) return;
    if (chatCurrent.time) CG_ChatAreaPush();
    memset(&chatCurrent, 0, sizeof(chatCurrent));
    chatCurrent.time = cg.time;
    chatCurrent.expire = cg.time + extraMsec + CHAT_LIFETIME;
    chatCurrent.type = type;
    length = strlen(text);
    if (length && text[length - 1] == '\n' && length < limit) limit = length;
    if (limit > 1) memcpy(chatCurrent.text, text, length < limit - 1 ? length : limit - 1);
}

void CG_ChatHistoryShow(qboolean show) { chatHistoryShown = show; }
void CG_ChatHistoryToggle(void) { chatHistoryShown = !chatHistoryShown; }
qboolean CG_ChatHistoryShown(void) { return chatHistoryShown; }

static void CG_ChatAreaLine(float x, float y, float scale, const char *text, int style) {
    vec4_t white = { 1, 1, 1, 1 };
    CG_Text_Paint(x, y, scale, white, text, 0, 256, style);
}

void CG_DrawChatArea(const rectDef_t *rect, float scale) {
    int lines = cg_chatHistoryLength.integer, index;
    float y;
    if (!rect) return;
    y = rect->y + rect->h;
    if (!(scale >= 0.1f)) scale = 0.1f;
    else if (scale > 0.22f) scale = 0.22f;
    CG_SetTextFont(1);
    if (chatCurrent.time) {
        if (cg.time <= chatCurrent.expire) {
            CG_ChatAreaLine(rect->x, y, scale, chatCurrent.text, ITEM_TEXTSTYLE_SHADOWED);
        } else {
            CG_ChatAreaPush();
            memset(&chatCurrent, 0, sizeof(chatCurrent));
        }
    }
    if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) lines = 4;
    else if (cg.scoreBoardShowing) lines = 2;
    if (!chatHistoryShown && !cg.scoreBoardShowing) return;
    if (lines > CHAT_HISTORY) lines = CHAT_HISTORY;
    for (index = chatHead; lines > 0; --lines) {
        y -= 13;
        /* Retail keys the shadow on the lines still to draw. */
        CG_ChatAreaLine(rect->x, y, scale, chatHistory[index].text,
                        lines > 10 ? ITEM_TEXTSTYLE_NORMAL : ITEM_TEXTSTYLE_SHADOWED);
        if (--index < 0) index = CHAT_HISTORY - 1;
    }
}
