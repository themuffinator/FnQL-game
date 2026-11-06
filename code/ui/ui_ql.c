/*
 * FnQL-game -- clean-room Quake Live compatible game modules.
 * Copyright (C) 2026 FnQL-game contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "ui_local.h"
#include "ui_native.h"

static unsigned int UI_SliderColor(float value);

/* IDs and menu command grammar: retail menudef.h and UI_RunMenuScript
 * 0x1000b0e0 (vote 0x1000c35b, player commands 0x1000cd91..0x1000d300). */
static const char *const modeNames[] = {
    "Free For All", "Duel", "Race", "Team Deathmatch", "Clan Arena",
    "Capture the Flag", "One Flag CTF", "Overload", "Harvester",
    "Freeze Tag", "Domination", "Attack and Defend", "Red Rover"
};
static const char *const modeTokens[] = {
    "ffa", "duel", "race", "tdm", "ca", "ctf", "oneflag", "overload",
    "har", "ft", "dom", "ad", "rr"
};

void UI_QLGameTypes(void) {
    int i;
    uiInfo.numGameTypes = uiInfo.numJoinGameTypes = GT_MAX_GAME_TYPE;
    for (i = 0; i < GT_MAX_GAME_TYPE; ++i) {
        uiInfo.gameTypes[i].gameType = modeNames[i];
        uiInfo.gameTypes[i].gtEnum = i;
        uiInfo.joinGameTypes[i] = uiInfo.gameTypes[i];
    }
}

int UI_QLArenaTypeBits(const char *types) {
    char buffer[MAX_INFO_VALUE], *cursor, *token;
    int bits = 0, i;
    Q_strncpyz(buffer, types, sizeof(buffer)); cursor = buffer;
    while (*(token = COM_Parse(&cursor))) {
        for (i = 0; i < GT_MAX_GAME_TYPE; ++i)
            if (!Q_stricmp(token, modeTokens[i])) bits |= 1 << i;
        if (!Q_stricmp(token, "tourney")) bits |= 1 << GT_TOURNAMENT;
        if (!Q_stricmp(token, "team")) bits |= 1 << GT_TEAM;
        if (!Q_stricmp(token, "clanarena")) bits |= 1 << GT_CLAN_ARENA;
        if (!Q_stricmp(token, "harvester")) bits |= 1 << GT_HARVESTER;
        if (!Q_stricmp(token, "freezetag")) bits |= 1 << GT_FREEZE;
        if (!Q_stricmp(token, "domination")) bits |= 1 << GT_DOMINATION;
        if (!Q_stricmp(token, "attackdefend")) bits |= 1 << GT_ATTACK_DEFEND;
        if (!Q_stricmp(token, "redrover")) bits |= 1 << GT_RED_ROVER;
    }
    return bits ? bits : 1 << GT_FFA;
}

int UI_QLMapIndex(int row) {
    int filter = (int)trap_Cvar_VariableValue("ui_cvGameType"), i;
    if (row < 0) return -1;
    for (i = 0; i < uiInfo.mapCount; ++i) {
        if (filter >= 0 && filter < GT_MAX_GAME_TYPE && !(uiInfo.mapList[i].typeBits & (1 << filter))) continue;
        if (row-- == 0) return i;
    }
    return -1;
}

int UI_QLMapCount(void) {
    int count = 0, i, filter = (int)trap_Cvar_VariableValue("ui_cvGameType");
    for (i = 0; i < uiInfo.mapCount; ++i)
        if (filter < 0 || filter >= GT_MAX_GAME_TYPE || (uiInfo.mapList[i].typeBits & (1 << filter))) ++count;
    return count;
}

void UI_QLSelectMap(int row) {
    int index = UI_QLMapIndex(row);
    if (uiInfo.serverStatus.currentServerCinematic >= 0) {
        trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
        uiInfo.serverStatus.currentServerCinematic = -1;
    }
    uiInfo.serverStatus.currentServerPreview = 0;
    if (index >= 0) {
        mapInfo *map = &uiInfo.mapList[index];
        const char *image = map->imageName;
        if (!image && map->mapLoadName) image = va("levelshots/preview/%s", map->mapLoadName);
        if (image) uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip(image);
    }
    ui_currentNetMap.integer = index;
    ui_mapIndex.integer = index < 0 ? -1 : row;
    trap_Cvar_SetValue("ui_currentNetMap", (float)index);
    trap_Cvar_SetValue("ui_mapIndex", (float)ui_mapIndex.integer);
}

static qboolean UI_CommandToken(const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    if (!p || !*p) return qfalse;
    for (; *p; ++p) if (!(isalnum(*p) || *p == '_' || *p == '-' || *p == '/')) return qfalse;
    return qtrue;
}

/* Retail closeingame 0x1000c151: release UI input, clear held keys, close
 * every menu and unpause. */
static void UI_CloseMenus(void) {
    trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
    trap_Key_ClearStates();
    Menus_CloseAll();
    trap_Cvar_Set("cl_paused", "0");
}

void UI_QLCycleBot(int direction) {
    int count = UI_GetNumBots();
    if (count <= 0) { uiInfo.botIndex = -1; return; }
    if (uiInfo.botIndex < 0 || uiInfo.botIndex >= count) uiInfo.botIndex = 0;
    else uiInfo.botIndex = (uiInfo.botIndex + (direction < 0 ? count - 1 : 1)) % count;
}

/* Retail 0x10011510 (and the init pass in 0x10011730) derives only the
 * read-only Bright flag from the cg_* forced model and skin. It never writes
 * ui_force*Model/Skin: those hold the preset-list names the menus chose. */
void UI_QLRefreshModelBright(qboolean team) {
    char model[MAX_CVAR_VALUE_STRING], skin[MAX_CVAR_VALUE_STRING];
    const char *side = team ? "Team" : "Enemy";
    qboolean bright;
    trap_Cvar_VariableStringBuffer(va("cg_force%sModel", side), model, sizeof(model));
    trap_Cvar_VariableStringBuffer(va("cg_force%sSkin", side), skin, sizeof(skin));
    /* Retail lowercases at most 64 characters of each value. */
    if (strlen(model) > 64) model[64] = 0;
    if (strlen(skin) > 64) skin[64] = 0;
    Q_strlwr(model); Q_strlwr(skin);
    bright = strstr(skin, "bright") || (strstr(model, "bright") && !*skin);
    trap_Cvar_Set(va("ui_force%sModelBright", side), bright ? "1" : "0");
}
void UI_QLTeamModelChanged(vmCvar_t *cvar) { (void)cvar; UI_QLRefreshModelBright(qtrue); }
void UI_QLEnemyModelChanged(vmCvar_t *cvar) { (void)cvar; UI_QLRefreshModelBright(qfalse); }

/* Retail 0x10011690: preview the selected announcer and mirror its number. */
void UI_QLAnnouncerChanged(vmCvar_t *cvar) {
    const char *sample = cvar->integer == 2 ? "sound/misc/vo_evil.wav" :
        cvar->integer == 3 ? "sound/misc/vo_female.wav" : "sound/misc/vo_default.wav";
    sfxHandle_t sound = trap_S_RegisterSound(sample, qfalse);
    if (sound) trap_S_StartLocalSound(sound, CHAN_ANNOUNCER);
    trap_Cvar_Set("ui_announcer", va("%i", cvar->integer));
}

qboolean UI_QLScript(const char *name) {
    static const struct { const char *script, *command, *suffix; } players[] = {
        {"kickPlayer", "callvote clientkick", ""}, {"tempbanPlayer", "tempban", ""},
        {"banPlayer", "ban", ""}, {"mutePlayer", "mute", ""},
        {"unmutePlayer", "unmute", ""}, {"modPlayer", "addmod", ""},
        {"adminPlayer", "addadmin", ""}, {"deopPlayer", "demote", ""},
        {"putred", "put", " r"}, {"putblue", "put", " b"}, {"putspec", "put", " s"},
        {"clientmutePlayer", "clientmute", ""}, {"clientviewProfile", "clientviewprofile", ""},
        {"clientFriendInvite", "clientfriendinvite", ""}
    };
    int i;
    /* Retail 0x1000c4ba selects the bot definition, including in team modes. */
    if (!Q_stricmp(name, "addBot")) {
        if (uiInfo.botIndex >= 0 && uiInfo.botIndex < UI_GetNumBots()) {
            const char *bot = UI_GetBotNameByNumber(uiInfo.botIndex);
            if (UI_CommandToken(bot) && uiInfo.skillIndex >= 0 && uiInfo.skillIndex < 5)
                trap_Cmd_ExecuteText(EXEC_APPEND, va("addbot %s %d %s\n", bot,
                    uiInfo.skillIndex + 1, uiInfo.redBlue ? "Blue" : "Red"));
        }
        return qtrue;
    }
    if (!Q_stricmp(name, "closeingame")) { UI_CloseMenus(); return qtrue; }
    if (!Q_stricmp(name, "RunDemo")) {
        int row = uiInfo.demoIndex;
        if (row >= 0 && row < uiInfo.demoCount && uiInfo.demoList[row]) {
            const char *demo = uiInfo.demoList[row], *p;
            for (p = demo; *p; ++p)
                if ((unsigned char)*p < 32 || *p == '"' || *p == ';') return qtrue;
            if (*demo) trap_Cmd_ExecuteText(EXEC_APPEND, va("demo \"%s\"\n", demo));
        }
        return qtrue;
    }
    for (i = 0; i < sizeof(players) / sizeof(players[0]); ++i) {
        if (!Q_stricmp(name, players[i].script)) {
            int row = uiInfo.playerIndex;
            if (row >= 0 && row < uiInfo.playerCount) {
                int client = uiInfo.playerClientNums[row];
                char info[MAX_INFO_STRING];
                if (client >= 0 && client < MAX_CLIENTS && trap_GetConfigString(CS_PLAYERS + client, info, sizeof(info)) && *info) {
                    trap_Cmd_ExecuteText(EXEC_APPEND, va("%s %d%s\n", players[i].command, client, players[i].suffix));
                    UI_CloseMenus();
                }
            }
            return qtrue;
        }
    }
    if (!Q_stricmp(name, "voteMap")) {
        int index = UI_QLMapIndex(ui_mapIndex.integer);
        int mode = (int)trap_Cvar_VariableValue("ui_cvGameType");
        if (index >= 0 && index < uiInfo.mapCount && UI_CommandToken(uiInfo.mapList[index].mapLoadName))
            trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote map %s %s\n", uiInfo.mapList[index].mapLoadName,
                mode >= 0 && mode < GT_MAX_GAME_TYPE ? modeTokens[mode] : ""));
        return qtrue;
    }
    if (!Q_stricmp(name, "updateCallvoteMapPreview")) {
        int row = ui_mapIndex.integer;
        UI_QLSelectMap(UI_QLMapIndex(row) < 0 ? 0 : row);
        return qtrue;
    }
    if (!Q_stricmp(name, "loadArenas")) {
        if (!uiInfo.mapCount) UI_LoadArenas();
        UI_QLSelectMap(0); return qtrue;
    }
    /* Retail Leave 0x1000c059: disconnect and keep the UI on the main menu. */
    if (!Q_stricmp(name, "leave")) {
        trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
        trap_Key_SetCatcher(KEYCATCH_UI);
        Menus_CloseAll();
        Menus_ActivateByName("main");
        return qtrue;
    }
    /* Retail 0x1000ccac..0x1000cd1b: openWebGameSettings shares the team path. */
    if (!Q_stricmp(name, "teamModelChanged") || !Q_stricmp(name, "openWebGameSettings")) {
        UI_QLRefreshModelBright(qtrue); return qtrue;
    }
    if (!Q_stricmp(name, "enemyModelChanged")) { UI_QLRefreshModelBright(qfalse); return qtrue; }
    /* Retail 0x1000c1d1..0x1000c285: fast renderer restart, window kept. */
    if (!Q_stricmp(name, "setFullScreen") || !Q_stricmp(name, "setWindowed") || !Q_stricmp(name, "toggleFullscreen")) {
        qboolean fullscreen = !Q_stricmp(name, "setFullScreen") ||
            (!Q_stricmp(name, "toggleFullscreen") && trap_Cvar_VariableValue("r_fullscreen") == 0);
        trap_Cvar_Set("r_fullscreen", fullscreen ? "1" : "0");
        trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart fast\n");
        return qtrue;
    }
    if (!Q_stricmp(name, "teamColorDefaults") || !Q_stricmp(name, "enemyColorDefaults")) {
        const char *side = !Q_stricmp(name, "teamColorDefaults") ? "team" : "enemy";
        const char *value = !strcmp(side, "team") ? "96" : "27";
        char packed[16];
        static const char *parts[] = {"Head", "Upper", "Lower"};
        snprintf(packed, sizeof(packed), "0x%08x", UI_SliderColor((float)atof(value)));
        for (i = 0; i < 3; ++i) {
            trap_Cvar_Set(va("ui_%s%sColor", side, parts[i]), value);
            /* Reapply defaults even if another settings path changed cg_*
             * while the corresponding slider value stayed unchanged. */
            trap_Cvar_Set(va("cg_%s%sColor", side, parts[i]), packed);
        }
        return qtrue;
    }
    return qfalse;
}

/* Retail 0x1000f9f0; displayContextDef_t is module-private, never shared
 * with the renderer. Only glconfig crosses this boundary. */
int UI_RefreshDisplayContext(void) {
    trap_GetGlconfig(&uiInfo.uiDC.glconfig);
    UI_ComputeCanvas(uiInfo.uiDC.glconfig.vidWidth, uiInfo.uiDC.glconfig.vidHeight,
        &uiInfo.uiDC.xscale, &uiInfo.uiDC.yscale, &uiInfo.uiDC.bias);
    return (int)(intptr_t)&uiInfo.uiDC;
}

/* Retail 0x1000fa03..0x1000faae. A wide display centres a 4:3 canvas using
 * the float constant 4/3 (1.3333333730697632) and a float 1/480, both in
 * double arithmetic stored as float. The resulting bias (79.99999 at
 * 960x600) is not rounded away: it decides where truncated pixel edges fall.
 * Other displays stretch with 1/640 and 1/480 and no bias. A degenerate mode
 * keeps a unit scale. */
void UI_ComputeCanvas(int width, int height, float *xscale, float *yscale, float *bias) {
    if (width <= 0 || height <= 0) {
        *xscale = *yscale = 1; *bias = 0;
        return;
    }
    if (width * 480 > height * 640) {
        *bias = (float)(((double)width - (double)height * 1.3333333730697632) * 0.5);
        *yscale = (float)((double)height * 0.0020833334419876337);
        *xscale = (float)((double)height / 3.0 * 4.0 / 640.0);
    } else {
        *bias = 0;
        *yscale = (float)((double)height * 0.0020833333333333333);
        *xscale = (float)((double)width * 0.0015625);
    }
}

/* Retail Text_Paint/Text_GetDimensions 0x10003ec0/0x10003d90: point size is
 * the item scale times vidHeight / 768 * 96, stored as float. */
float UI_TextPointSize(float scale) {
    const int height = uiInfo.uiDC.glconfig.vidHeight > 0 ? uiInfo.uiDC.glconfig.vidHeight : 480;
    return (float)((double)height / 768.0 * 96.0) * scale;
}

void UI_ForEachArenaName(void (QDECL *callback)(const char *)) {
    int i;
    if (!callback) return;
    if (!uiInfo.mapCount) UI_LoadArenas();
    for (i = 0; i < uiInfo.mapCount; ++i)
        if (uiInfo.mapList[i].mapName) callback(uiInfo.mapList[i].mapName);
}

void UI_DrawAdvertisementWaitScreen(const char *name) {
    menuDef_t *menu = name ? Menus_FindByName(name) : NULL;
    if (menu) Menu_Paint(menu, qtrue);
    /* Retail advertisement export 0x10011130: font 0, half-size at 178/440. */
    UI_SetTextFont(0);
    Text_PaintCenter(320, 178, .5f, colorWhite, "Waiting on Advertisement", 0);
    Text_PaintCenter(320, 440, .5f, colorWhite, "Press ESC to cancel", 0);
}

/* Retail slider callback 0x10011240, palette data at 0x1002bb88.
 * The palette is an interoperability fact; interpolation is bounded here
 * instead of permitting the retail index to wrap outside the palette. */
static unsigned int UI_SliderColor(float value) {
    static const unsigned int palette[] = {
        0x800000ff, 0x802300ff, 0x804000ff, 0x805e00ff, 0x808000ff,
        0x5e8000ff, 0x408000ff, 0x238000ff, 0x008000ff, 0x008023ff,
        0x008040ff, 0x00805eff, 0x008080ff, 0x005e80ff, 0x004080ff,
        0x002380ff, 0x000080ff, 0x230080ff, 0x400080ff, 0x5e0080ff,
        0x800080ff, 0x80005eff, 0x800040ff, 0x800023ff, 0x808080ff, 0x404040ff
    };
    float position = value <= 1 ? 0 : value * .25f, fraction;
    unsigned int packed = 0;
    int index, shift;
    if (position >= 25) return palette[25];
    index = (int)position; fraction = position - index;
    for (shift = 0; shift <= 24; shift += 8) {
        float a = (float)((palette[index] >> shift) & 255);
        float b = (float)((palette[index + 1] >> shift) & 255);
        packed |= (unsigned int)(a + (b - a) * fraction + .5f) << shift;
    }
    return packed;
}

/* Retail slider callback 0x10011240 targets. ui_teamColor/ui_enemyColor set
 * all three body parts; the rest set one cg_* packed color each. */
typedef struct { vmCvar_t *cvar; const char *name, *targets[3]; } sliderColor_t;
static const sliderColor_t *UI_QLSliderTargets(const vmCvar_t *cvar, const char *name) {
    static const sliderColor_t sliders[] = {
        {&ui_teamColor, "ui_teamColor", {"cg_teamUpperColor", "cg_teamLowerColor", "cg_teamHeadColor"}},
        {&ui_enemyColor, "ui_enemyColor", {"cg_enemyUpperColor", "cg_enemyLowerColor", "cg_enemyHeadColor"}},
        {&ui_teamHeadColor, "ui_teamHeadColor", {"cg_teamHeadColor"}},
        {&ui_teamUpperColor, "ui_teamUpperColor", {"cg_teamUpperColor"}},
        {&ui_teamLowerColor, "ui_teamLowerColor", {"cg_teamLowerColor"}},
        {&ui_enemyHeadColor, "ui_enemyHeadColor", {"cg_enemyHeadColor"}},
        {&ui_enemyUpperColor, "ui_enemyUpperColor", {"cg_enemyUpperColor"}},
        {&ui_enemyLowerColor, "ui_enemyLowerColor", {"cg_enemyLowerColor"}},
        {&ui_screenDamage, "ui_screenDamage", {"cg_screenDamage"}},
        {&ui_screenDamageTeam, "ui_screenDamage_Team", {"cg_screenDamage_Team"}}
    };
    int i;
    for (i = 0; i < ARRAY_LEN(sliders); ++i)
        if ((cvar && cvar == sliders[i].cvar) || (name && !Q_stricmp(name, sliders[i].name))) return &sliders[i];
    return NULL;
}
static void UI_QLApplySliderColor(const sliderColor_t *slider, float value) {
    char color[16];
    int i;
    /* Retail ignores values at or below 0.001, so an unset slider never
     * clears a cg color. The upper bound is a robustness addition. */
    if (!slider || !(value > .001f) || value > 1e6f) return;
    snprintf(color, sizeof(color), "0x%08x", UI_SliderColor(value));
    for (i = 0; i < 3 && slider->targets[i]; ++i) trap_Cvar_Set(slider->targets[i], color);
}
void UI_QLSliderColorChanged(vmCvar_t *cvar) {
    UI_QLApplySliderColor(UI_QLSliderTargets(cvar, NULL), cvar->value);
}

/* Widget edits apply at once; console/config edits arrive through the cvar
 * table's change callbacks in UI_UpdateCvars, as in retail. */
void UI_QLSetCvar(const char *name, const char *value) {
    const sliderColor_t *slider;
    trap_Cvar_Set(name, value);
    if ((slider = UI_QLSliderTargets(NULL, name)) != NULL) UI_QLApplySliderColor(slider, (float)atof(value));
}

/* Retail 0x10009d30 consumes the cgame-published UI cvars, including
 * cg_loadout (not the similarly named server setting g_loadout). */
qboolean UI_QLVisible(int flags) {
    int mode = (int)trap_Cvar_VariableValue("g_gametype");
    int loadout = (int)trap_Cvar_VariableValue("cg_loadout");
    float warmup = trap_Cvar_VariableValue("ui_warmup");
    if ((flags & UI_SHOW_FFA) && mode != GT_FFA) return qfalse;
    if ((flags & UI_SHOW_NOTFFA) && mode == GT_FFA) return qfalse;
    if ((flags & UI_SHOW_IF_LOADOUT_ENABLED) && loadout != 1) return qfalse;
    if ((flags & UI_SHOW_IF_LOADOUT_DISABLED) && loadout == 1) return qfalse;
    if ((flags & UI_SHOW_IF_NOT_INTERMISSION) && trap_Cvar_VariableValue("ui_intermission") == 1) return qfalse;
    if ((flags & UI_SHOW_IF_WARMUP) && warmup >= 0) return qfalse;
    if ((flags & UI_SHOW_IF_NOT_WARMUP) && warmup < 0) return qfalse;
    return qtrue;
}

void UI_QLPaletteColor(int index, vec4_t color) {
    unsigned int packed = UI_SliderColor(index <= 1 ? 1 : (index - 1) * 4.0f);
    int i;
    for (i = 0; i < 3; ++i) color[i] = ((packed >> (24 - i * 8)) & 255) / 128.0f;
    color[3] = 1;
}

/* Retail packed-color parse in the previews: hexadecimal when the text
 * contains "0x", otherwise atoi (so an empty value is 0). */
static unsigned int UI_QLPackedColor(const char *name) {
    char value[64];
    unsigned int packed = 0;
    trap_Cvar_VariableStringBuffer(name, value, sizeof(value));
    if (strstr(value, "0x")) sscanf(value, "0x%08x", &packed);
    else packed = (unsigned int)atoi(value);
    return packed;
}

/* Retail forced-model previews: team 0x10005850, enemy 0x10005c20, red
 * 0x10005ff0 and blue 0x100062a0. Team/enemy use cg_force*Model with an
 * optional cg_force*Skin (a skin alone selects sarge); red/blue use
 * cg_forceRed/BlueTeamModel and the team colors. An unset model draws
 * nothing. Previews are rebuilt when the model text changes. */
static void UI_QLModel(rectDef_t *rect, int which) {
    static const char *const modelCvars[] = {"cg_forceTeamModel", "cg_forceEnemyModel", "cg_forceRedTeamModel", "cg_forceBlueTeamModel"};
    static const char *const parts[] = {"Lower", "Upper", "Head"};
    static playerInfo_t previews[4];
    static char previous[4][MAX_QPATH * 2];
    char model[MAX_QPATH * 2], skin[MAX_QPATH];
    const char *colors = which == 1 ? "enemy" : "team";
    vec3_t angles = {0, 170, 0};
    int part, channel;
    trap_Cvar_VariableStringBuffer(modelCvars[which], model, sizeof(model));
    skin[0] = 0;
    if (which < 2) trap_Cvar_VariableStringBuffer(which ? "cg_forceEnemySkin" : "cg_forceTeamSkin", skin, sizeof(skin));
    if (!*model) {
        if (!*skin) { previous[which][0] = 0; return; }
        Q_strncpyz(model, "sarge", sizeof(model));
    }
    if (*skin) {
        char *slash = strchr(model, '/');
        if (slash) *slash = 0;
        Q_strcat(model, sizeof(model), "/");
        Q_strcat(model, sizeof(model), skin);
    }
    if (strcmp(previous[which], model)) {
        Q_strncpyz(previous[which], model, sizeof(previous[which]));
        UI_PlayerInfo_SetModel(&previews[which], model, model, "");
        UI_PlayerInfo_SetInfo(&previews[which], LEGS_IDLE, TORSO_STAND,
            angles, vec3_origin, WP_MACHINEGUN, qfalse);
    }
    previews[which].tintColors = qtrue;
    for (part = 0; part < 3; ++part) {
        const unsigned int packed = UI_QLPackedColor(va("cg_%s%sColor", colors, parts[part]));
        for (channel = 0; channel < 4; ++channel)
            previews[which].partColors[part][channel] = (byte)(packed >> (24 - channel * 8));
    }
    UI_DrawPlayer(rect->x, rect->y, rect->w, rect->h, &previews[which], uiInfo.uiDC.realTime / 2);
}

/* Display facts from retail UI 0x10007030..0x100086e0. The menu owns
 * the panel rectangle; the server's published settings own its contents. */
static void UI_QLServerSettings(rectDef_t *rect, float scale, vec4_t color, int style) {
    typedef struct { const char *label, *key; unsigned int modes, bit; int source, omit; } setting_t;
    static const setting_t rows[] = {
        {"Time Limit: %i", "timelimit", ~0u},
        {"Frag Limit: %i", "fraglimit", 0x0f},
        {"Mercy Limit: %i", "mercylimit", ~7u, 0, 0, 0},
        {"Capture Limit: %i", "capturelimit", 1u << GT_CTF},
        {"Round Limit: %i", "roundlimit", (1u << GT_CLAN_ARENA) | (1u << GT_FREEZE) | (1u << GT_ATTACK_DEFEND) | (1u << GT_RED_ROVER)},
        {"Score Limit: %i", "scorelimit", (1u << GT_DOMINATION) | (1u << GT_ATTACK_DEFEND)},
        {"Air Control", NULL, ~0u, 0x2000},
        {"Ramp Jumping", NULL, ~0u, 0x4000},
        {"Tiered Armor", "armor_tiered", ~0u, 0, 1, 0},
        {"Weapon Switching", NULL, ~0u, 0x10000},
        {"%ix Quad", "g_quadDamageFactor", ~0u, 0, 2, 3},
        {"Physics", NULL, ~0u, 0x8000},
        {"Gravity %i", "g_gravity", ~0u, 0, 2, 800},
        {"InstaGib", NULL, ~0u, 0x20000},
        {"Quad Hog", NULL, ~0u, 0x40000},
        {"Infected", NULL, 1u << GT_RED_ROVER, 0x4000000},
        {"Regen Health", NULL, ~0u, 0x80000},
        {"Drop Health", NULL, ~0u, 0x100000},
        {"Vampiric Damage", NULL, ~0u, 0x200000},
        {"Item Spawning", NULL, ~0u, 0x400000},
        {"Headshots", NULL, ~0u, 0x800000},
        {"Rail Jumping", NULL, ~0u, 0x1000000}
    };
    char info[3][MAX_INFO_STRING], custom[MAX_INFO_STRING], lines[32][64];
    unsigned int mask, weapons, modeBit;
    int mode, count = 1, i, icon = 0;
    float x, y;
    trap_GetConfigString(CS_SERVERINFO, info[0], sizeof(info[0]));
    trap_GetConfigString(0x2a9, info[1], sizeof(info[1]));
    trap_GetConfigString(0x2aa, info[2], sizeof(info[2]));
    trap_GetConfigString(0x2c0, custom, sizeof(custom));
    mask = (unsigned int)strtoul(custom, NULL, 10);
    mode = atoi(Info_ValueForKey(info[0], "g_gametype"));
    modeBit = mode >= 0 && mode < GT_MAX_GAME_TYPE ? 1u << mode : 0;
    Q_strncpyz(lines[0], modeBit ? modeNames[mode] : "Unknown Gametype", sizeof(lines[0]));
    for (i = 0; i < ARRAY_LEN(rows); ++i) {
        const setting_t *row = &rows[i];
        const char *value;
        int number;
        if (!(row->modes & modeBit)) continue;
        if (row->bit) {
            if (!(mask & row->bit) || (row->bit == 0x100000 && (mask & 0x20000))) continue;
            Q_strncpyz(lines[count++], row->label, sizeof(lines[0]));
        } else {
            value = Info_ValueForKey(info[row->source], row->key);
            number = atoi(value);
            if (!*value || ((row->source || i == 2) && number == row->omit)) continue;
            Com_sprintf(lines[count++], sizeof(lines[0]), row->label, number);
        }
    }
    weapons = mask & 0x1fff;
    if (weapons || count == 1) Q_strncpyz(lines[count++], weapons ? "MODIFIED WEAPONS:" : "Default Settings", sizeof(lines[0]));
    for (i = 0; i < count; ++i) {
        /* The retail column begins one line below the preceding column. */
        Text_Paint(rect->x + 110 * (i / 8), rect->y + 12 * ((i % 8) + i / 8),
            scale, color, lines[i], 0, 0, style);
    }
    x = rect->x + 110 * ((count - 1) / 8);
    y = rect->y + 12 * (((count - 1) % 8) + (count - 1) / 8) + 6;
    for (i = 1; i <= 13; ++i) {
        gitem_t *weapon;
        float ix, iy;
        if (!(weapons & (1u << (i - 1)))) continue;
        if (mode == GT_RACE && ((1u << (i - 1)) & 0xdc)) continue;
        weapon = BG_FindItemForWeapon(i);
        if (!weapon || !weapon->icon) continue;
        ix = x + (icon % 8) * 12; iy = y + (icon / 8) * 12;
        UI_DrawHandlePic(ix, iy, 8, 8, trap_R_RegisterShaderNoMip(weapon->icon));
        UI_DrawHandlePic(ix + 6, iy + 4, 4, 4, trap_R_RegisterShaderNoMip("icons/modified.tga"));
        ++icon;
    }
}

qboolean UI_QLOwnerDraw(int id, rectDef_t *rect, float scale, vec4_t color, int style, qhandle_t shader) {
    char text[MAX_INFO_STRING];
    int i;
    if (id == UI_CROSSHAIR_COLOR) {
        int selected = (int)trap_Cvar_VariableValue("cg_crosshairColor") - 1;
        vec4_t tint = {1, 1, 1, 1};
        qhandle_t marker;
        if (selected < 0 || selected > 25) selected = 0;
        if (trap_Cvar_VariableValue("cg_crosshairHealth")) tint[0] = tint[1] = tint[2] = .25f;
        /* Retail 0x10009660 uses the fx asset strip. Its 26-entry selector
         * spans the seven fx handles and the adjacent crosshair bank
         * (0x10755588/0x107555a4). Preserve that lookup with bounded arrays. */
        marker = selected < 7 ? uiInfo.uiDC.Assets.fxPic[selected] :
            uiInfo.uiDC.Assets.crosshairShader[selected - 7];
        trap_R_SetColor(tint);
        UI_DrawHandlePic(rect->x, rect->y - 14, 128, 8, uiInfo.uiDC.Assets.fxBasePic);
        UI_DrawHandlePic(rect->x + selected * 16 + 8, rect->y - 16, 16, 12, marker);
        trap_R_SetColor(NULL);
    } else if (id == UI_NEXTMAP || id == UI_VOTESTRING) {
        /* Next-map configstring: retail UI next-map ownerdraw (configstring 0x29a); vote menu uses ui_votestring. */
        if (id == UI_NEXTMAP) trap_GetConfigString(0x29a, text, sizeof(text));
        else trap_Cvar_VariableStringBuffer("ui_votestring", text, sizeof(text));
        Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, style);
    } else if (id >= UI_TEAMPLAYERMODEL && id <= UI_BLUETEAMMODEL) {
        UI_QLModel(rect, id - UI_TEAMPLAYERMODEL);
    } else if (id == UI_STARTING_WEAPONS) {
        unsigned int mask;
        float x = rect->x, y = rect->y;
        trap_GetConfigString(CS_STARTING_WEAPONS, text, sizeof(text));
        mask = (unsigned int)strtoul(text, NULL, 10);
        /* Retail 0x100088bf: bit zero is gauntlet; 0x10008923 advances
         * horizontally by 1.5 times the menu-authored icon width. */
        for (i = 1; i < WP_NUM_WEAPONS; ++i) if (mask & (1u << (i - 1))) {
            gitem_t *weapon = BG_FindItemForWeapon(i);
            if (weapon && weapon->icon) {
                UI_DrawHandlePic(x, y, rect->w, rect->h, trap_R_RegisterShaderNoMip(weapon->icon));
                x += rect->w * 1.5f;
            }
        }
        if (trap_Cvar_VariableValue("cg_loadout") == 1) {
            /* Retail 0x10008a9c..0x10008b3a appends the queued primary,
             * using HMG when its short weapon token is absent/invalid. */
            static const char *tokens[] = {"g", "mg", "sg", "gl", "rl", "lg", "rg", "pg", "bfg", "gh", "ng", "pl", "cg", "hmg"};
            char *cursor, *token;
            int weaponNum = WP_HEAVY_MACHINEGUN;
            gitem_t *weapon;
            UI_SetTextFont(0);
            Text_Paint(x, y + (rect->h + Text_Height("+", scale, 0)) * .5f,
                scale, color, "+", 0, 0, style);
            trap_Cvar_VariableStringBuffer("cg_weaponPrimaryQueued", text, sizeof(text));
            cursor = text; token = COM_Parse(&cursor);
            for (i = 0; i < ARRAY_LEN(tokens); ++i) if (!Q_stricmp(token, tokens[i])) { weaponNum = i + 1; break; }
            weapon = BG_FindItemForWeapon(weaponNum);
            if (weapon && weapon->icon) {
                trap_R_SetColor(colorWhite);
                UI_DrawHandlePic(x + rect->w, y, rect->w, rect->h, trap_R_RegisterShaderNoMip(weapon->icon));
                trap_R_SetColor(NULL);
            }
        }
    } else if (id == UI_SERVER_SETTINGS) {
        UI_QLServerSettings(rect, scale, color, style);
    } else if (id == UI_ADVERT) {
        if (shader) UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, shader);
    } else return qfalse;
    return qtrue;
}
