/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* Quake Live loading screen (retail 0x1001ce10, 0x1001cb80, 0x1001cbe0,
 * 0x100249d0, 0x100574c0). */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
gitem_t bg_itemlist[3];
int bg_numItems = 2;

typedef enum { EV_UPDATE, EV_VIEW, EV_FRAMETIME, EV_PIC, EV_STRETCH, EV_FILL, EV_TEXT } evType_t;
typedef struct {
    evType_t type;
    float x, y, w, h, s1, s2;
    qhandle_t shader;
    float color[4];
    char text[128];
    int font, style, placement, value;
} event_t;
static event_t events[64];
static int numEvents, placement, textFont, settingsUpdates;
static char configs[MAX_CONFIGSTRINGS][256];
static char registered[64][MAX_QPATH];
static int numRegistered;
static const char *missingShader;

static event_t *Push(evType_t type) {
    event_t *e;
    CHECK(numEvents < 64);
    e = &events[numEvents++];
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->placement = placement;
    return e;
}

void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
const char *CG_ConfigString(int index) { CHECK(index >= 0 && index < MAX_CONFIGSTRINGS); return configs[index]; }
void trap_UpdateScreen(void) { Push(EV_UPDATE); }
void trap_AdvertisementBridge_UpdateLoadingViewParameters(void) { Push(EV_VIEW); }
void trap_AdvertisementBridge_SetFrameTime(int frameTime) { Push(EV_FRAMETIME)->value = frameTime; }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) {
    if (missingShader && !strcmp(name, missingShader)) return 0;
    CHECK(numRegistered < 64);
    Q_strncpyz(registered[numRegistered], name, MAX_QPATH);
    return 100 + numRegistered++;
}
void trap_R_SetColor(const float *rgba) { (void)rgba; }
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {
    event_t *e = Push(EV_STRETCH);
    CHECK(t1 == 0 && t2 == 1);
    e->x = x; e->y = y; e->w = w; e->h = h; e->s1 = s1; e->s2 = s2; e->shader = shader;
}
void CG_DrawPic(float x, float y, float w, float h, qhandle_t shader) {
    event_t *e = Push(EV_PIC);
    e->x = x; e->y = y; e->w = w; e->h = h; e->shader = shader;
}
void CG_FillRect(float x, float y, float w, float h, const float *color) {
    event_t *e = Push(EV_FILL);
    e->x = x; e->y = y; e->w = w; e->h = h; memcpy(e->color, color, sizeof(e->color));
}
int CG_SetScreenPlacement(int p) { const int previous = placement; placement = p; return previous; }
void CG_SetTextFont(int font) { textFont = font; }
/* Odd widths make integer and float halving differ; heights depend on font. */
int CG_Text_WidthInFont(const char *text, int font, float scale, int limit) {
    (void)scale; (void)limit; return (int)strlen(text) * (font + 10) + 1;
}
int CG_Text_HeightInFont(const char *text, int font, float scale, int limit) {
    (void)text; (void)limit; return font == 0 ? (int)(scale * 25) : 7;
}
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    event_t *e = Push(EV_TEXT);
    CHECK(adjust == 0 && limit == 0 && color[0] == 1 && color[3] == 1);
    e->x = x; e->y = y; e->w = scale; e->font = textFont; e->style = style;
    Q_strncpyz(e->text, text, sizeof(e->text));
}
void CG_UpdatePlayerPresentationSettings(void) { ++settingsUpdates; }

static qboolean Near(float a, float b) { return a > b - 0.001f && a < b + 0.001f; }
static const event_t *Find(evType_t type, int nth) {
    int i;
    for (i = 0; i < numEvents; ++i)
        if (events[i].type == type && nth-- == 0) return &events[i];
    return NULL;
}

int main(void) {
    const event_t *e;
    int i;

    /* Loading strings: screen update, loading view, then frame time 1000. */
    CG_LoadingString("sounds");
    CHECK(!strcmp(cg.infoScreenText, "sounds") && numEvents == 3);
    CHECK(events[0].type == EV_UPDATE && events[1].type == EV_VIEW);
    CHECK(events[2].type == EV_FRAMETIME && events[2].value == 1000);
    CG_LoadingString("\xe9t\xe9");  /* high bytes are not masked */
    CHECK((unsigned char)cg.infoScreenText[0] == 0xe9);

    /* Names: ^0-^7 and control bytes go, others stay; an empty name stays empty. */
    {
        char name[64] = "^1Sa^9rg\x01" "e^7\xe9";
        CG_LoadingCleanName(name);
        CHECK(!strcmp(name, "Sa^9rge\xe9"));
    }
    Q_strncpyz(configs[CS_PLAYERS + 3], "\\n\\^4An^7arki\\model\\keel/bright", sizeof(configs[0]));
    numEvents = numRegistered = 0;
    CG_LoadingClient(3);
    CHECK(!strcmp(cg.infoScreenText, "Anarki") && !strcmp(registered[0], "models/players/keel/icon_bright.tga"));
    missingShader = "models/players/visor/icon_default.tga";
    Q_strncpyz(configs[CS_PLAYERS + 4], "\\n\\\\model\\visor", sizeof(configs[0]));
    numRegistered = 0;
    CG_LoadingClient(4);
    CHECK(!cg.infoScreenText[0] && !strcmp(registered[0], "models/players/characters/visor/icon_default.tga"));
    missingShader = NULL;
    CG_LoadingClient(-1); CG_LoadingClient(MAX_CLIENTS);  /* ignored */

    /* Items: icons register (up to 38), text is the pickup name. */
    bg_itemlist[1].icon = "icons/iconr_red"; bg_itemlist[1].pickup_name = "Red Armor";
    numRegistered = 0;
    for (i = 0; i < 40; ++i) { numEvents = 0; CG_LoadingItem(1); }
    CHECK(numRegistered == 38 && !strcmp(cg.infoScreenText, "Red Armor"));
    CG_LoadingItem(5);  /* out of range: ignored */
    CHECK(!strcmp(cg.infoScreenText, "Red Armor"));

    /* Gametype names and the 16:9 crop. */
    CHECK(!strcmp(CG_LoadingGametypeName(0), "Free For All") && !strcmp(CG_LoadingGametypeName(1), "Duel"));
    CHECK(!strcmp(CG_LoadingGametypeName(12), "Red Rover") && !strcmp(CG_LoadingGametypeName(13), "Unknown Gametype"));
    CHECK(!strcmp(CG_LoadingGametypeName(-1), "Unknown Gametype"));
    CHECK(Near(CG_LoadingImageCrop(1920, 1080), 0) && Near(CG_LoadingImageCrop(1440, 1080), 0.125f));
    CHECK(Near(CG_LoadingImageCrop(2560, 1080), -1.0f / 6) && CG_LoadingImageCrop(0, 0) == 0);

    /* Loading frame: levelshot, bars, logos, map text, authors, gametype,
     * loading text and progress. */
    cgs.glconfig.vidWidth = 1440; cgs.glconfig.vidHeight = 1080;
    cgs.gametype = GT_CTF;
    cgs.media.loadingLogoBackShader = 11; cgs.media.loadingLogoShader = 12;
    cgs.media.loadingGametypeBackShader = 13; cgs.media.loadingSmokeShader = 14;
    Q_strncpyz(configs[CS_SERVERINFO], "\\mapname\\campgrounds", sizeof(configs[0]));
    Q_strncpyz(configs[CS_MESSAGE], "Campgrounds Redux", sizeof(configs[0]));
    Q_strncpyz(configs[CS_MAP_AUTHOR], "id Software", sizeof(configs[0]));
    Q_strncpyz(configs[CS_MAP_AUTHOR2], "Quake Live", sizeof(configs[0]));
    CG_LoadingStageDone(); CG_LoadingStageDone();
    CHECK(CG_LoadingStagesDone() == 2);
    Q_strncpyz(cg.infoScreenText, "graphics", sizeof(cg.infoScreenText));
    numEvents = numRegistered = settingsUpdates = 0; placement = 0;
    CG_DrawInformation();
    CHECK(settingsUpdates == 1 && !strcmp(registered[0], "levelshots/campgrounds.tga") && placement == 0);
    e = Find(EV_STRETCH, 0);
    CHECK(e && e->shader == 100 && e->w == 1440 && e->h == 1080 && Near(e->s1, 0.125f) && Near(e->s2, 0.875f));
    e = Find(EV_FILL, 0);
    CHECK(e->x == -400 && e->y == 0 && e->w == 1440 && e->h == 64 && e->placement == CG_PLACE_LEFT);
    CHECK(e->color[0] == 0.375f && e->color[1] == 0.125f && e->color[3] == 0.75f);
    CHECK(Find(EV_PIC, 0)->shader == 11 && Find(EV_PIC, 1)->shader == 12 && Find(EV_PIC, 1)->w == 256);
    e = Find(EV_FILL, 1);
    CHECK(e->y == 400 && e->h == 80 && e->color[0] == 0 && e->color[3] == 0.75f);
    e = Find(EV_TEXT, 0);
    CHECK(!strcmp(e->text, "Campgrounds Redux") && e->x == 10 && e->y == 420 && e->font == 0 && e->w == 0.8f);
    CHECK(e->style == ITEM_TEXTSTYLE_NORMAL && e->placement == CG_PLACE_LEFT);
    e = Find(EV_TEXT, 1);
    CHECK(!strcmp(e->text, "id Software") && e->y == 454 && e->font == 1 && e->w == 0.25f);
    CHECK(!strcmp(Find(EV_TEXT, 2)->text, "Quake Live") && Find(EV_TEXT, 2)->y == 469);
    e = Find(EV_PIC, 2);
    CHECK(e->shader == 13 && e->x == 384 && e->placement == CG_PLACE_RIGHT);
    e = Find(EV_TEXT, 3);
    CHECK(!strcmp(e->text, "Capture the Flag") && e->x == 620 - (16 * 10 + 1) && e->y == 30 && e->placement == CG_PLACE_RIGHT);
    e = Find(EV_TEXT, 4);
    CHECK(!strcmp(e->text, "Loading graphics") && e->y == 370 && e->font == 1 && e->w == 0.3f);
    CHECK(e->x == 320 - (16 * 11 + 1) / 2 && e->style == ITEM_TEXTSTYLE_SHADOWED && e->placement == CG_PLACE_CENTER);
    for (i = 0; i < 4; ++i) {
        e = Find(EV_FILL, 2 + i);
        CHECK(e->x == 292 + i * 16 && e->y == 380 && e->w == 8 && e->h == 8 && e->placement == CG_PLACE_CENTER);
        CHECK(i < 2 ? e->color[3] == 1 && e->color[0] == 0.375f : e->color[3] == 0.75f && e->color[0] == 0);
    }
    CHECK(!Find(EV_FILL, 6) && !Find(EV_TEXT, 5));

    /* No authors: neither author line; a missing levelshot uses unknownmap. */
    configs[CS_MAP_AUTHOR][0] = 0;
    missingShader = "levelshots/campgrounds.tga";
    numEvents = numRegistered = 0;
    CG_DrawInformation();
    CHECK(!strcmp(registered[0], "menu/art/unknownmap") && Find(EV_STRETCH, 0)->shader == 100);
    CHECK(!strcmp(Find(EV_TEXT, 1)->text, "Capture the Flag"));
    missingShader = NULL;

    /* No text and no snapshot: awaiting the snapshot. */
    cg.infoScreenText[0] = 0; cg.snap = NULL;
    numEvents = 0;
    CG_DrawInformation();
    CHECK(!strcmp(Find(EV_TEXT, 2)->text, "Awaiting Snapshot") && Find(EV_FILL, 2)->color[3] == 1);

    /* No text but a snapshot (not active): smoke and "Awaiting gamestate...". */
    {
        static snapshot_t snap;
        cg.snap = &snap;
        cgs.glconfig.vidWidth = 1920;
        numEvents = 0; placement = CG_PLACE_RIGHT;
        CG_DrawInformation();
        CHECK(placement == CG_PLACE_RIGHT && !Find(EV_FILL, 0) && !Find(EV_PIC, 0));
        e = Find(EV_STRETCH, 0);
        CHECK(e->shader == 14 && e->w == 1920 && Near(e->s1, 0) && Near(e->s2, 1));
        e = Find(EV_TEXT, 0);
        CHECK(!strcmp(e->text, "Awaiting gamestate...") && e->y == 178 && e->font == 0 && e->w == 0.3f);
        CHECK(Near(e->x, 320 - (21 * 10 + 1) * 0.5f) && e->style == ITEM_TEXTSTYLE_SHADOWED);
        CHECK(e->placement == CG_PLACE_CENTER && !Find(EV_TEXT, 1));
    }
    puts("loading screen: strings, names, icons, crop, layout, progress and awaiting states OK");
    return 0;
}
