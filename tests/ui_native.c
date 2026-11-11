/* FnQL-game UI behavioral gates. SPDX-License-Identifier: GPL-2.0-or-later */
#include "../code/ui/ui_local.h"
#include "../code/ui/ui_native.h"
#include "../code/game/fnql_module.h"
#undef NDEBUG
#include <assert.h>

extern void QDECL dllEntry(void **, const void *, int *);
extern const fnqlModuleInfo_t *QDECL FnQL_GetModuleInfo(void);
extern qboolean Item_EnableShowViaCvar(itemDef_t *, int);
extern qboolean Item_HandleKey(itemDef_t *, int, qboolean);
extern const char *Item_Multi_Setting(itemDef_t *);
extern itemDef_t *Menu_FindItemByName(menuDef_t *, const char *);
extern void Controls_GetConfig(void);
extern void Controls_SetConfig(qboolean);
extern void Controls_SetDefaults(void);
extern void Item_ListBox_Select(itemDef_t *, int);
extern void UI_UpdateCvars(void);
extern void Item_RunScript(itemDef_t *, const char *);
static vmCvar_t *bumpedCvar;
static int subscribedApp, soundsStarted;
static unsigned infoLow, infoHigh;
static char soundName[MAX_QPATH];
static void QDECL UpdateCvar(vmCvar_t *cvar) { if (cvar == bumpedCvar) ++cvar->modificationCount; }
static int QDECL Subscribed(int app) { subscribedApp = app; return 1; }
static sfxHandle_t QDECL RegisterSound(const char *name, qboolean compressed) { (void)compressed; Q_strncpyz(soundName, name, sizeof(soundName)); return 7; }
static void QDECL LocalSound(sfxHandle_t sfx, int channel) { assert(sfx == 7 && channel == CHAN_ANNOUNCER); ++soundsStarted; }
static void QDECL ItemInfo(unsigned low, unsigned high, unsigned long long *done, unsigned long long *total) { infoLow = low; infoHigh = high; *done = 5; *total = 10; }
static int HeadCount(float id) { (void)id; return 1; }
static const char *HeadText(float id, int index, int column, qhandle_t *handle) { (void)id; (void)index; (void)column; *handle = -1; return "keel/bright"; }
extern int BindingIDFromName(const char *);
extern int Text_Width(const char *, float, int);
extern void Text_Paint(float, float, float, vec4_t, const char *, float, int, int);

static struct { char name[64], value[128]; } cvars[128];
static int cvarCount, commands, catcher, drawn, measured, fontSeen, limitSeen, bindings;
static char command[1024], keyBindings[256][64], shaderName[MAX_QPATH];
static int stoppedCinematics;
static int registeredFlags, pictureCount, textCount;
static float textSize;
static struct { float x, y, w, h, s, t; int shader; } pictures[32];
static char paintedText[32][128];
static const char *startingWeapons = "", *customSettings = "", *settingsA = "", *settingsB = "";
extern int ui_numBots;
extern qboolean Item_Bind_HandleKey(itemDef_t *, int, qboolean);
static char *tokens;
static const char *serverInfo = "", *warmupInfo = "", *intermissionInfo = "";
static void QDECL Print(const char *text) { fputs(text, stdout); }
static void QDECL Error(const char *text) { fputs(text, stderr); abort(); }
static void QDECL Set(const char *name, const char *value) {
    int i;
    for (i = 0; i < cvarCount; ++i) if (!strcmp(cvars[i].name, name)) break;
    assert(i < 128);
    if (i == cvarCount) { ++cvarCount; Q_strncpyz(cvars[i].name, name, 64); }
    Q_strncpyz(cvars[i].value, value, 128);
}
static void QDECL Get(const char *name, char *out, int size) {
    int i;
    out[0] = 0;
    for (i = 0; i < cvarCount; ++i)
        if (!strcmp(cvars[i].name, name)) Q_strncpyz(out, cvars[i].value, size);
}
static float QDECL Value(const char *name) { char value[128]; Get(name, value, sizeof(value)); return (float)atof(value); }
static void QDECL Register(vmCvar_t *cvar, const char *name, const char *value, int flags) {
    (void)cvar; (void)value; assert(!strcmp(name, "selection")); registeredFlags = flags;
}
static void QDECL SetValue(const char *name, float value) { Set(name, va("%g", value)); }
static void QDECL Append(const char *text) { ++commands; Q_strncpyz(command, text, sizeof(command)); }
static int QDECL Catcher(void) { return catcher; }
static void QDECL SetCatcher(int value) { catcher = value; }
static void QDECL Noop(void) {}
static void QDECL SetBinding(int key, const char *command) { assert(key > 0 && key < 256); assert(command); Q_strncpyz(keyBindings[key], command, sizeof(keyBindings[key])); ++bindings; }
static void QDECL Binding(int key, char *out, int size) { assert(key >= 0 && key < 256); Q_strncpyz(out, keyBindings[key], size); }
static int QDECL Config(int index, char *out, int size) {
    const char *value = index == CS_PLAYERS + 7 ? "\\n\\target\\t\\1" : "";
    if (index == CS_SERVERINFO) value = serverInfo;
    if (index == CS_WARMUP) value = warmupInfo;
    if (index == CS_INTERMISSION) value = intermissionInfo;
    if (index == CS_STARTING_WEAPONS) value = startingWeapons;
    if (index == 0x2c0) value = customSettings;
    if (index == 0x2a9) value = settingsA;
    if (index == 0x2aa) value = settingsB;
    Q_strncpyz(out, value, size); return *out != 0;
}
static void QDECL Glconfig(glconfig_t *out) { memset(out, 0, sizeof(*out)); out->vidWidth = 960; out->vidHeight = 600; }
static int QDECL Token(int handle, pc_token_t *out) {
    char *start = tokens, *text, *end;
    if (!start) return 0;
    while (*start && *start <= 32) ++start;
    text = COM_Parse(&tokens);
    (void)handle;
    if (!*text && *start != '"') return 0;
    memset(out, 0, sizeof(*out)); Q_strncpyz(out->string, text, sizeof(out->string));
    out->floatvalue = (float)strtod(text, &end); out->intvalue = (int)out->floatvalue;
    out->type = *start == '"' ? TT_STRING : *end ? TT_NAME : TT_NUMBER;
    return 1;
}
static int QDECL Source(int handle, char *file, int *line) { (void)handle; strcpy(file, "test"); *line = 1; return 1; }
static qhandle_t QDECL Shader(const char *name) { Q_strncpyz(shaderName, name, sizeof(shaderName)); return 1; }
static e_status QDECL StopCinematic(int handle) { assert(handle >= 0); ++stoppedCinematics; return FMV_IDLE; }
static float lastColor[4];
static qboolean overstrike;
static int drawX[32], rangeEnd;
static char rangeText[8];
static void QDECL Color(const float *color) { if (color) memcpy(lastColor, color, sizeof(lastColor)); }
static void QDECL SetOverstrike(qboolean state) { overstrike = state; }
static qboolean QDECL GetOverstrike(void) { return overstrike; }
static void QDECL KeyName(int key, char *out, int size) { Com_sprintf(out, size, "%c", key); }
static void QDECL Stretch(float x, float y, float w, float h, float s, float t, float s2, float t2, qhandle_t shader) {
    int i = pictureCount++;
    (void)s2; (void)t2; assert(i < ARRAY_LEN(pictures));
    pictures[i].x = x; pictures[i].y = y; pictures[i].w = w; pictures[i].h = h;
    pictures[i].s = s; pictures[i].t = t; pictures[i].shader = shader;
}
static void QDECL Draw(int x, int y, const char *text, int font, float size, int limit, float *maxX, int force) {
    (void)x; (void)y; (void)text; (void)size; (void)maxX; (void)force;
    ++drawn; fontSeen = font; limitSeen = limit;
    textSize = size;
    if (textCount < ARRAY_LEN(paintedText)) { drawX[textCount] = x; Q_strncpyz(paintedText[textCount++], text, sizeof(paintedText[0])); }
}
static unsigned long long QDECL Measure(const char *text, const char *end, int font, float size, int limit, float *bounds) {
    (void)font; (void)size;
    assert(bounds); bounds[0] = 2; bounds[2] = 32; bounds[4] = 10;
    if (end) { rangeEnd = (int)(end - text); memcpy(rangeText, text, sizeof(rangeText) - 1); rangeText[sizeof(rangeText) - 1] = 0; }
    ++measured; limitSeen = limit; return 0;
}

static void CheckMenus(void) {
    menuDef_t *menu;
    itemDef_t *item;
    char fixture[] = "{ name gate rect 0 0 640 480 backgroundSize 0 0 1920 1080 ownerdrawFlag2 8 "
        "itemDef { name gated type 1 visible 1 cvarTest first showCvar { yes } cvarTest2 second hideCvar2 { hidden } } "
        "itemDef { name gated4 type 1 visible 1 cvarTest3 third showCvar3 { on } cvarTest4 fourth hideCvar4 { off } outlineimage ui/assets/outline } "
        "itemDef { name presetA type 15 cvarPreset { alpha 2 beta enabled } } "
        "itemDef { name presetB type 15 cvarPreset { alpha 3 beta NULL } } "
        "itemDef { name presets type 16 cvara selection cvarPresetList { A presetA B presetB } rect 10 10 100 20 visible 1 } "
        "itemDef { name slider type 14 cvarInt volume 5 0 10 precision 2 maxPaintChars 4 } }";
    tokens = fixture; Menu_New(1);
    menu = Menus_FindByName("gate"); assert(menu && menu->itemCount == 6);
    /* Retail menu keyword table 0x1002ad6c: ownerdrawFlag2 sets the second flag word. */
    assert(menu->window.ownerDrawFlags2 == 8 && !menu->window.ownerDrawFlags);
    assert(registeredFlags == CVAR_ARCHIVE);
    item = Menu_FindItemByName(menu, "gated");
    Set("first", "yes"); Set("second", "hidden"); assert(!Item_EnableShowViaCvar(item, CVAR_SHOW));
    Set("second", "visible"); assert(Item_EnableShowViaCvar(item, CVAR_SHOW));
    Set("first", "no"); assert(!Item_EnableShowViaCvar(item, CVAR_SHOW));
    /* Retail slots 3 and 4 gate independently; outlineimage registers art. */
    item = Menu_FindItemByName(menu, "gated4");
    assert(item && item->outlineImage && !strcmp(shaderName, "ui/assets/outline"));
    Set("third", "on"); Set("fourth", "on"); assert(Item_EnableShowViaCvar(item, CVAR_SHOW));
    Set("fourth", "off"); assert(!Item_EnableShowViaCvar(item, CVAR_SHOW));
    Set("fourth", "on"); Set("third", "off"); assert(!Item_EnableShowViaCvar(item, CVAR_SHOW));
    item = Menu_FindItemByName(menu, "presets");
    Set("alpha", "2.0"); Set("beta", "enabled");
    assert(!strcmp(Item_Multi_Setting(item), "Custom"));
    Set("selection", "A"); assert(!strcmp(Item_Multi_Setting(item), "A"));
    item->window.flags |= WINDOW_HASFOCUS;
    uiInfo.uiDC.cursorx = item->window.rect.x + 2; uiInfo.uiDC.cursory = item->window.rect.y + 2;
    assert(Item_HandleKey(item, K_ENTER, qtrue));
    assert(Value("alpha") == 3 && !strcmp(Item_Multi_Setting(item), "B"));
    /* Retail 0x100187b2 stores a numeric preset value as "%f" (0x10001670
     * accepts digits and one '.'); other values are stored as written. */
    { char value[128]; Get("alpha", value, sizeof(value)); assert(!strcmp(value, "3.000000")); }
    assert(Item_PresetValueIsNumber("0") && Item_PresetValueIsNumber("1000") && Item_PresetValueIsNumber(".5"));
    assert(!Item_PresetValueIsNumber("") && !Item_PresetValueIsNumber("-1") && !Item_PresetValueIsNumber("1.2.3"));
    assert(!Item_PresetValueIsNumber("enabled") && !Item_PresetValueIsNumber("1e3"));
    { char value[128]; Get("beta", value, sizeof(value)); assert(!*value); }
    Set("beta", "custom"); assert(!strcmp(Item_Multi_Setting(item), "Custom"));
    { char value[128]; Get("selection", value, sizeof(value)); assert(!strcmp(value, "Custom")); }
    item = Menu_FindItemByName(menu, "slider"); assert(item->integerCvar && item->precision == 2 && item->typeData);
    Menus_Activate(menu); menu->window.flags |= WINDOW_FORCED; assert(UI_MenusVisible()); Menus_CloseAll(); assert(!UI_MenusVisible());
    {
        /* Retail toggle 0x10016580 keeps its own per-menu state. */
        itemDef_t owner = {0}; const char *script;
        char toggle[] = "{ toggle gate }";
        tokens = toggle; assert(PC_Script_Parse(1, &script));
        Item_RunScript(&owner, script); assert((menu->window.flags & WINDOW_VISIBLE) && menu->toggled);
        Item_RunScript(&owner, script); assert(!(menu->window.flags & WINDOW_VISIBLE) && !menu->toggled);
        menu->window.flags &= ~WINDOW_FORCED;
    }
}

static void CheckCvarCaptions(void) {
    /* Retail Item_SetTextExtents 0x10019f73: a CENTER or RIGHT cvar caption
     * without text is placed off-screen; a LEFT one paints its value. */
    extern void Item_Text_Paint(itemDef_t *);
    menuDef_t menu = {0};
    itemDef_t item = {0};
    int before;
    uiInfo.uiDC.textWidth = Text_Width; uiInfo.uiDC.textHeight = Text_Height; uiInfo.uiDC.drawText = Text_Paint;
    item.parent = &menu; item.cvar = "caption"; item.textscale = .2f;
    item.window.foreColor[0] = item.window.foreColor[1] = item.window.foreColor[2] = item.window.foreColor[3] = 1;
    Set("caption", "20.0");
    item.textalignment = ITEM_ALIGN_CENTER; before = drawn; Item_Text_Paint(&item); assert(drawn == before);
    item.textalignment = ITEM_ALIGN_RIGHT; item.textRect.w = 0; Item_Text_Paint(&item); assert(drawn == before);
    item.textalignment = ITEM_ALIGN_LEFT; item.textRect.w = 0; Item_Text_Paint(&item); assert(drawn > before);
    item.text = "Label"; item.textalignment = ITEM_ALIGN_CENTER; item.textRect.w = 0;
    before = drawn; Item_Text_Paint(&item); assert(drawn > before);
}

static void CheckLegacyPainting(void) {
    rectDef_t rect = {10, 20, 16, 16};
    menuDef_t menu = {0};
    itemDef_t item = {0};
    editFieldDef_t edit = {0};
    extern void Item_Slider_Paint(itemDef_t *);
    float x = 0, y = 20, w = 640, h = 480;
    UI_AdjustFrom640(&x, &y, &w, &h);
    /* Retail 0x1000fa03: the 960x600 bias is just under 80 and yscale just
     * over 1.25, so a 60-unit left edge truncates to pixel 154 as in retail. */
    assert(x > 79.999f && x < 80 && y > 25 && y < 25.0001f && w == 800 && h > 600 && h < 600.001f);
    { float edge = 60; UI_AdjustFrom640(&edge, NULL, NULL, NULL); assert((int)edge == 154); }
    { float xs, ys, b; UI_ComputeCanvas(640, 480, &xs, &ys, &b); assert(xs == 1 && ys == 1 && b == 0); }
    { float xs, ys, b; UI_ComputeCanvas(1280, 1024, &xs, &ys, &b); assert(xs == 2 && b == 0 && ys > 2.13f && ys < 2.14f); }
    Text_Paint(0, 20, .25f, colorWhite, "size", 0, 0, 0);
    assert(textSize == 18.75f);
    menu.fullScreen = qtrue; menu.window.flags = WINDOW_VISIBLE;
    menu.backgroundSize.w = 1920; menu.backgroundSize.h = 1080; menu.window.background = 101;
    pictureCount = 0; Menu_Paint(&menu, qfalse);
    assert(pictureCount == 1 && pictures[0].x == 0 && pictures[0].w == 960);
    assert(fabsf(pictures[0].s - .05f) < .0001f);
    Set("web_browserActive", "1"); Menu_Paint(&menu, qfalse); assert(pictureCount == 1);
    Set("web_browserActive", "0");
    item.parent = &menu; item.type = ITEM_TYPE_SLIDER_COLOR; item.cvar = "volume";
    item.typeData = &edit; edit.maxVal = 100;
    uiInfo.uiDC.Assets.fxBasePic = 102; uiInfo.uiDC.Assets.sliderThumb = 103;
    pictureCount = 0; Item_Slider_Paint(&item);
    assert(pictureCount == 2 && pictures[0].shader == 102 && pictures[1].shader == 103);
    serverInfo = "\\g_gametype\\0\\timelimit\\15\\fraglimit\\50\\roundlimit\\8";
    textCount = 0; UI_QLOwnerDraw(UI_SERVER_SETTINGS, &rect, .18f, colorWhite, 0, 0);
    assert(textCount == 3 && !strcmp(paintedText[0], "Free For All"));
    assert(!strcmp(paintedText[1], "Time Limit: 15") && !strcmp(paintedText[2], "Frag Limit: 50"));
    customSettings = "1310720"; /* Instagib is absent: Quad Hog plus Drop Health. */
    textCount = 0; UI_QLOwnerDraw(UI_SERVER_SETTINGS, &rect, .18f, colorWhite, 0, 0);
    assert(textCount == 5 && !strcmp(paintedText[3], "Quad Hog") && !strcmp(paintedText[4], "Drop Health"));
    customSettings = "1179648"; /* Instagib suppresses Drop Health. */
    textCount = 0; UI_QLOwnerDraw(UI_SERVER_SETTINGS, &rect, .18f, colorWhite, 0, 0);
    assert(textCount == 4 && !strcmp(paintedText[3], "InstaGib"));
    pictureCount = 0; startingWeapons = "3";
    UI_QLOwnerDraw(UI_STARTING_WEAPONS, &rect, .18f, colorWhite, 0, 0);
    assert(pictureCount == 2 && pictures[0].w == 20 && pictures[1].x - pictures[0].x == 30);
    Set("cg_loadout", "1"); Set("cg_weaponPrimaryQueued", "rl");
    pictureCount = textCount = 0;
    UI_QLOwnerDraw(UI_STARTING_WEAPONS, &rect, .18f, colorWhite, 0, 0);
    assert(pictureCount == 3 && textCount == 1 && !strcmp(paintedText[0], "+"));
    assert(!strcmp(shaderName, "icons/iconw_rocket"));
    Set("cg_loadout", "0");
    serverInfo = customSettings = startingWeapons = "";
}

static void CheckScoreMenuMetadata(void) {
    char fixture[2048];
    menuDef_t *menu;
    itemDef_t *item;
    listBoxDef_t *list;
    int i;
    strcpy(fixture,"{ name scoreboard_gate itemDef { name rows type 6 widescreen 1 ownerdrawflag2 32768 elementcolor 1 1 1 1 selectedcolor 1 1 0 1 altrowcolor 0.25 0.25 0.25 0.4 columns 33");
    for (i=0;i<33;++i) strcat(fixture," 10 20 5");
    strcat(fixture," elementheight 18 } }");
    tokens = fixture; Menu_New(1);
    menu = Menus_FindByName("scoreboard_gate"); assert(menu && menu->itemCount == 1);
    item = menu->items[0]; list = item->typeData;
    assert(item->widescreenSet && item->widescreen == 1 && item->window.ownerDrawFlags2 == 32768);
    assert(list && list->numColumns == 32 && list->elementHeight == 18);
    assert(list->columnInfo[31].width == 20 && list->hasElementColor && list->hasSelectedColor && list->hasAltRowColor);
    assert(list->selectedColor[2] == 0 && list->altRowColor[3] == 0.4f);
}

/* Pass 5 interaction behaviours; the interaction gate compares them with retail. */
static void CheckInteraction(void) {
    extern void Text_PaintWithCursor(float, float, float, vec4_t, const char *, int, char, int, int);
    extern qboolean Item_Slider_HandleKey(itemDef_t *, int, qboolean);
    extern void Item_StartCapture(itemDef_t *, int);
    extern void Menu_PaintAll(void);
    extern void Menu_HandleKey(menuDef_t *, int, qboolean);
    itemDef_t slider = {0}, edit = {0};
    editFieldDef_t limits = {0}, field = {0};
    menuDef_t menu = {0}, *parsed;
    char value[128], fixture[] = "{ name plainslider itemDef { name plain type 10 cvarFloat vol 5 0 10 } }";
    /* Retail item init (0x1002001a) leaves a slider's precision at 0. */
    tokens = fixture; Menu_New(1);
    parsed = Menus_FindByName("plainslider"); assert(parsed && !Menu_FindItemByName(parsed, "plain")->precision);
    /* Retail Item_Slider_HandleKey 0x10019315: a track click stores "%f". */
    limits.minVal = 0; limits.maxVal = 10;
    slider.type = ITEM_TYPE_SLIDER; slider.cvar = "vol"; slider.typeData = &limits; slider.integerCvar = qtrue;
    slider.window.rect.x = 100; slider.window.rect.y = 100; slider.window.rect.w = 200; slider.window.rect.h = 20;
    slider.window.flags = WINDOW_HASFOCUS;
    uiInfo.uiDC.cursorx = 172; uiInfo.uiDC.cursory = 105;
    assert(Item_Slider_HandleKey(&slider, K_MOUSE1, qtrue));
    Get("vol", value, sizeof(value)); assert(!strcmp(value, "7.500000"));
    /* A thumb drag (0x10018ee3..0x10018f8f) stores ceil()'d "%d" for cvarInt,
     * "%.Nf" with a precision and "%f" without one. */
    Set("vol", "5"); uiInfo.uiDC.cursorx = 148; Item_StartCapture(&slider, K_MOUSE1);
    uiInfo.uiDC.cursorx = 172; Menu_PaintAll(); Get("vol", value, sizeof(value)); assert(!strcmp(value, "8"));
    slider.integerCvar = qfalse; slider.precision = 2;
    Menu_PaintAll(); Get("vol", value, sizeof(value)); assert(!strcmp(value, "7.50"));
    slider.precision = 0; Menu_PaintAll(); Get("vol", value, sizeof(value)); assert(!strcmp(value, "7.500000"));
    Menus_CloseAll();
    /* Retail Menu_HandleKey 0x10019dd5/0x10019e26: editing starts in insert mode. */
    uiInfo.uiDC.setOverstrikeMode = SetOverstrike; uiInfo.uiDC.getOverstrikeMode = GetOverstrike;
    field.maxChars = field.maxPaintChars = 8;
    edit.type = ITEM_TYPE_EDITFIELD; edit.cvar = "fieldvalue"; edit.typeData = &field; edit.parent = &menu;
    edit.window.flags = WINDOW_HASFOCUS | WINDOW_VISIBLE; edit.window.rect.w = 100; edit.window.rect.h = 20;
    menu.items[0] = &edit; menu.itemCount = 1; menu.window.rect.w = 640; menu.window.rect.h = 480; menu.window.flags = WINDOW_HASFOCUS | WINDOW_VISIBLE;
    overstrike = qtrue; Menu_HandleKey(&menu, K_ENTER, qtrue); assert(!overstrike);
    Menus_CloseAll();
    /* A press also starts a capture probe, which asks the feeder for a count. */
    uiInfo.uiDC.feederCount = HeadCount; uiInfo.uiDC.cursorx = 5; uiInfo.uiDC.cursory = 5;
    overstrike = qtrue; Menu_HandleKey(&menu, K_MOUSE1, qtrue); assert(!overstrike);
    Menus_CloseAll();
    /* Retail Text_PaintWithCursor 0x10004070 at 960x600 (stored bias
     * 79.999985, scale 1.25, pixel x truncated); the Measure stub reports
     * every range 30 px wide. The cursor shows while
     * UI time bit 8 is set; insert mode pulls '|' left by 0.8 of its width. */
    uiInfo.uiDC.realTime = 0; textCount = 0;
    Text_PaintWithCursor(0, 20, .25f, colorWhite, "ab", 1, '|', 0, 0);
    assert(textCount == 1 && drawX[0] == 79 && !strcmp(paintedText[0], "ab"));
    uiInfo.uiDC.realTime = 0x100; textCount = 0;
    Text_PaintWithCursor(0, 20, .25f, colorWhite, "ab", 1, '|', 0, 0);
    assert(textCount == 2 && !strcmp(paintedText[1], "|") && drawX[1] == 85 && rangeEnd == 1);
    textCount = 0; Text_PaintWithCursor(0, 20, .25f, colorWhite, "ab", 1, '_', 0, 0);
    assert(textCount == 2 && !strcmp(paintedText[1], "_") && drawX[1] == 109);
    /* A space before the cursor measures as '_'; a cursor past the text
     * keeps its raw position. */
    textCount = 0; Text_PaintWithCursor(0, 20, .25f, colorWhite, "a ", 2, '|', 0, 0);
    assert(rangeEnd == 2 && !strcmp(rangeText, "a_") && !strcmp(paintedText[0], "a "));
    textCount = 0; Text_PaintWithCursor(0, 20, .25f, colorWhite, "", 1, '|', 0, 0);
    assert(rangeEnd == 1 && !*rangeText && textCount == 2);
    uiInfo.uiDC.realTime = 0;
}

int main(void) {
    uiNativeImports_t imports = {0}; void **exports; int version, before;
    imports.trap_Print = Print; imports.trap_Error = Error;
    imports.trap_Cvar_Set = Set; imports.trap_Cvar_SetValue = SetValue;
    imports.trap_Cvar_Register = Register; imports.trap_R_DrawStretchPic = Stretch;
    imports.trap_Cvar_VariableValue = Value; imports.trap_Cvar_VariableStringBuffer = Get;
    imports.AppendCommand = Append; imports.trap_Key_GetCatcher = Catcher;
    imports.trap_Key_SetCatcher = SetCatcher; imports.trap_Key_ClearStates = Noop; imports.trap_Cvar_Update = UpdateCvar;
    imports.trap_S_RegisterSound = RegisterSound; imports.trap_S_StartLocalSound = LocalSound;
    imports.trap_Key_GetBindingBuf = Binding; imports.trap_GetConfigString = Config;
    imports.trap_GetGlconfig = Glconfig; imports.trap_PC_ReadToken = Token;
    imports.trap_PC_SourceFileAndLine = Source; imports.trap_R_RegisterShaderNoMip = Shader;
    imports.trap_CIN_StopCinematic = StopCinematic; imports.trap_R_SetColor = Color; imports.DrawText = Draw; imports.MeasureText = Measure;
    dllEntry((void **)&exports, &imports, &version);
    assert(version == 8 && exports && sizeof(imports) == 97 * sizeof(void (*)(void)));
    for (before = 0; before < 14; ++before) assert(exports[before]);
    assert(!strcmp(FnQL_GetModuleInfo()->module, "ui"));
    assert(!(FnQL_GetModuleInfo()->capabilities & FNQL_MODULE_CAP_RETAIL_WIRE));
    assert(offsetof(uiNativeImports_t, AppendCommand) == 20 * sizeof(void *));
    assert(offsetof(uiNativeImports_t, MeasureText) == 95 * sizeof(void *));
    trap_Cmd_ExecuteText(EXEC_APPEND, "echo native\n"); assert(!strcmp(command, "echo native\n"));
    uiInfo.uiDC.getCVarValue = Value; uiInfo.uiDC.getCVarString = Get; uiInfo.uiDC.setCVar = Set;
    uiInfo.uiDC.registerCvar = Register; uiInfo.uiDC.drawHandlePic = UI_DrawHandlePic;
    uiInfo.uiDC.drawStretchPic = Stretch; uiInfo.uiDC.setColor = Color;
    uiInfo.uiDC.getBindingBuf = Binding; uiInfo.uiDC.setBinding = SetBinding; uiInfo.uiDC.executeText = trap_Cmd_ExecuteText; uiInfo.uiDC.registerShaderNoMip = Shader;
    Init_Display(&uiInfo.uiDC); UI_InitMemory(); String_Init();
    UI_RefreshDisplayContext(); UI_SetTextFont(2);
    assert(Text_Width("native", .25f, 0) == 24 && measured && limitSeen == -1);
    Text_Paint(0, 20, .25f, colorWhite, "native", 0, 0, 0);
    assert(drawn && fontSeen == 2 && limitSeen == -1);
    {
        /* Retail export 3 (0x10010000): absolute drawable pixels mapped onto
         * the centered 4:3 canvas (960x600: bias ~80, scale 1.25), truncated. */
        extern void _UI_MouseEvent(int, int);
        _UI_MouseEvent(80 + 125, 300); assert(uiInfo.uiDC.cursorx == 100 && uiInfo.uiDC.cursory == 240);
        _UI_MouseEvent(80 + 125, 300); assert(uiInfo.uiDC.cursorx == 100 && uiInfo.uiDC.cursory == 240);
        _UI_MouseEvent(0, 0); assert(uiInfo.uiDC.cursorx == -63 && uiInfo.uiDC.cursory == 0);
    }
    CheckMenus();
    CheckScoreMenuMetadata();
    CheckLegacyPainting();
    CheckCvarCaptions();
    CheckInteraction();
    {
        itemDef_t item = {0}; char value[128]; const char *script;
        char fixture[] = "{ setcvar cleared \"\" ; setcvar brace \"}\" ; setcvar after ok }";
        tokens = fixture; assert(PC_Script_Parse(1, &script));
        Set("cleared", "old"); Item_RunScript(&item, script);
        Get("cleared", value, sizeof(value)); assert(!*value);
        Get("brace", value, sizeof(value)); assert(!strcmp(value, "}"));
        Get("after", value, sizeof(value)); assert(!strcmp(value, "ok"));
    }
    {
        itemDef_t list = {0}; char value[128];
        uiInfo.uiDC.feederCount = HeadCount; uiInfo.uiDC.feederItemText = HeadText;
        list.special = FEEDER_HEADS; list.cvar = "cg_forceTeamModel";
        Item_ListBox_Select(&list, 0); Get(list.cvar, value, sizeof(value));
        assert(!strcmp(value, "keel/bright"));
        list.cvar = "cg_forceEnemyModel"; Item_ListBox_Select(&list, 9);
        Get(list.cvar, value, sizeof(value)); assert(!*value);
        Item_ListBox_Select(&list, 0); Get(list.cvar, value, sizeof(value));
        assert(!strcmp(value, "keel/bright"));
    }
    Controls_GetConfig(); before = commands; Controls_SetConfig(qfalse);
    assert(!bindings && commands == before);
    assert(BindingIDFromName("weapon 14") >= 0 && BindingIDFromName("readyup") >= 0);
    assert(BindingIDFromName("+voice") >= 0 && BindingIDFromName("weapon toggle") >= 0);
    /* Retail table 0x1002a0d0: order, QL rows and default keys. */
    assert(BindingIDFromName("+scores") == 0 && BindingIDFromName("follow 9") == 80);
    assert(BindingIDFromName("forfeit") >= 0 && BindingIDFromName("droprune") >= 0);
    assert(BindingIDFromName("togglechathistory") >= 0 && BindingIDFromName("+pstats") >= 0);
    assert(BindingIDFromName("messagemode3") > 80 && BindingIDFromName("messagemode4") > 80);
    memset(keyBindings, 0, sizeof(keyBindings));
    Controls_SetDefaults(); Controls_SetConfig(qtrue);
    assert(bindings > 0 && commands == before + 1 && !strcmp(command, "in_restart\n"));
    assert(!strcmp(keyBindings['w'], "+forward") && !strcmp(keyBindings[K_MOUSE1], "+attack"));
    assert(!strcmp(keyBindings[K_MOUSE2], "+zoom") && !strcmp(keyBindings[K_F3], "readyup"));
    assert(!strcmp(keyBindings[K_KP_INS], "follow 0") && !strcmp(keyBindings[K_KP_PGUP], "follow 9"));
    /* Retail order: dropweapon follows +left on the left arrow. */
    assert(!strcmp(keyBindings[K_LEFTARROW], "dropweapon") && !*keyBindings[0]);
    {
        itemDef_t bind = {0};
        memset(keyBindings, 0, sizeof(keyBindings));
        SetBinding('x', "weapon 14"); SetBinding('y', "weapon 14"); SetBinding('u', "unrelated");
        Controls_GetConfig();
        bind.type = ITEM_TYPE_BIND; bind.cvar = "weapon 14";
        bind.window.rect.w = bind.window.rect.h = 100;
        uiInfo.uiDC.cursorx = uiInfo.uiDC.cursory = 5;
        Item_Bind_HandleKey(&bind, K_ENTER, qtrue); assert(Display_KeyBindPending());
        before = commands;
        Item_Bind_HandleKey(&bind, K_BACKSPACE, qfalse); assert(Display_KeyBindPending() && commands == before);
        Item_Bind_HandleKey(&bind, K_BACKSPACE, qtrue);
        assert(!*keyBindings['x'] && !*keyBindings['y'] && !strcmp(keyBindings['u'], "unrelated"));
        Item_Bind_HandleKey(&bind, K_ENTER, qtrue);
        Item_Bind_HandleKey(&bind, 'z', qtrue); assert(!strcmp(keyBindings['z'], "weapon 14"));
        {
            /* Retail Item_Bind_HandleKey (0x1001bbac..0x1001bbbe) clears only the
             * waiting flag after a bind: the bound item keeps the red focus tint. */
            extern void Item_Bind_Paint(itemDef_t *);
            menuDef_t parent = {0};
            parent.focusColor[0] = parent.focusColor[1] = parent.focusColor[2] = parent.focusColor[3] = 1;
            uiInfo.uiDC.keynumToStringBuf = KeyName;
            bind.parent = &parent; bind.window.flags |= WINDOW_HASFOCUS; uiInfo.uiDC.realTime = 0;
            assert(!Display_KeyBindPending());
            Item_Bind_Paint(&bind); assert(fabs(lastColor[0] - .9f) < 1e-4f && fabs(lastColor[1] - .5f) < 1e-4f);
            bind.window.flags &= ~WINDOW_HASFOCUS; bind.parent = NULL;
        }
        Item_Bind_HandleKey(&bind, K_ENTER, qtrue); Menus_CloseAll(); assert(!Display_KeyBindPending());
    }
    ui_numBots = 2; uiInfo.botIndex = 1; UI_QLCycleBot(1); assert(uiInfo.botIndex == 0);
    UI_QLCycleBot(-1); assert(uiInfo.botIndex == 1);
    ui_numBots = 0; UI_QLCycleBot(1); assert(uiInfo.botIndex == -1);
    before = commands; UI_QLScript("addBot"); assert(commands == before);
    ui_numBots = 1; uiInfo.botIndex = 0; uiInfo.skillIndex = 4; uiInfo.redBlue = 1;
    UI_QLScript("addBot"); assert(!strcmp(command, "addbot Sarge 5 Blue\n"));
    before = commands; uiInfo.skillIndex = 99; UI_QLScript("addBot"); assert(commands == before);
    before = commands; uiInfo.demoCount = 0; UI_QLScript("RunDemo"); assert(commands == before);
    uiInfo.demoCount = 1; uiInfo.demoIndex = 0; uiInfo.demoList[0] = "Mixed Case Demo";
    UI_QLScript("RunDemo"); assert(!strcmp(command, "demo \"Mixed Case Demo\"\n"));
    before = commands; uiInfo.demoList[0] = "demo;quit"; UI_QLScript("RunDemo"); assert(commands == before);
    {
        char color[128];
        UI_QLSetCvar("ui_teamHeadColor", "1"); Get("cg_teamHeadColor", color, sizeof(color));
        assert(!strcmp(color, "0x800000ff"));
        UI_QLSetCvar("ui_teamHeadColor", "0"); Get("cg_teamHeadColor", color, sizeof(color));
        assert(!strcmp(color, "0x800000ff"));
        UI_QLSetCvar("ui_enemyColor", "100"); Get("cg_enemyLowerColor", color, sizeof(color));
        assert(!strcmp(color, "0x404040ff"));
        ui_teamUpperColor.value = 96; UI_QLSliderColorChanged(&ui_teamUpperColor); Get("cg_teamUpperColor", color, sizeof(color));
        assert(!strcmp(color, "0x808080ff"));
        /* Retail 0x100118a0: no callback for a never-registered count, then
         * one for each later modification. */
        Set("cg_teamHeadColor", "0x11111111"); ui_teamHeadColor.value = 96;
        ui_teamHeadColor.modificationCount = 0; bumpedCvar = &ui_teamHeadColor;
        UI_UpdateCvars(); Get("cg_teamHeadColor", color, sizeof(color)); assert(!strcmp(color, "0x11111111"));
        UI_UpdateCvars(); Get("cg_teamHeadColor", color, sizeof(color)); assert(!strcmp(color, "0x808080ff"));
        bumpedCvar = NULL;
        ui_teamColor.value = 1; UI_QLSliderColorChanged(&ui_teamColor);
        Get("cg_teamLowerColor", color, sizeof(color)); assert(!strcmp(color, "0x800000ff"));
        Set("cg_teamUpperColor", "0x800000ff"); UI_QLScript("teamColorDefaults");
        Get("cg_teamUpperColor", color, sizeof(color)); assert(!strcmp(color, "0x808080ff"));
        UI_QLSetCvar("ui_screenDamage", "1000"); Get("cg_screenDamage", color, sizeof(color));
        assert(!strcmp(color, "0x404040ff"));
    }
    Set("cg_loadout", "1"); Set("ui_warmup", "-1");
    assert(UI_QLVisible(UI_SHOW_IF_LOADOUT_ENABLED | UI_SHOW_IF_WARMUP));
    assert(!UI_QLVisible(UI_SHOW_IF_LOADOUT_DISABLED));
    assert(!UI_QLVisible(UI_SHOW_IF_NOT_WARMUP));
    serverInfo = "\\g_gametype\\3"; Set("g_gametype", "3");
    assert(UI_QLVisible(UI_SHOW_NOTFFA) && !UI_QLVisible(UI_SHOW_FFA));
    serverInfo = "\\g_gametype\\0"; Set("g_gametype", "0");
    assert(UI_QLVisible(UI_SHOW_FFA) && !UI_QLVisible(UI_SHOW_NOTFFA));
    Set("ui_intermission", "1"); assert(!UI_QLVisible(UI_SHOW_IF_NOT_INTERMISSION));
    serverInfo = warmupInfo = intermissionInfo = "";
    Set("cg_loadout", "0"); Set("ui_warmup", "0"); Set("ui_intermission", "0");
    assert(UI_QLVisible(UI_SHOW_IF_LOADOUT_DISABLED | UI_SHOW_IF_NOT_WARMUP));
    UI_QLGameTypes(); assert(uiInfo.numGameTypes == 13 && uiInfo.gameTypes[2].gtEnum == GT_RACE);
    /* Retail name table 0x1002c0f4 spells out "and". */
    assert(!strcmp(uiInfo.gameTypes[GT_ATTACK_DEFEND].gameType, "Attack and Defend"));
    uiInfo.mapCount = 2;
    uiInfo.mapList[0].mapLoadName = "campgrounds"; uiInfo.mapList[0].typeBits = 1 << GT_FFA;
    uiInfo.mapList[1].mapLoadName = "ironworks"; uiInfo.mapList[1].typeBits = 1 << GT_CTF;
    Set("ui_cvGameType", "5"); assert(UI_QLMapCount() == 1 && UI_QLMapIndex(0) == 1);
    UI_QLSelectMap(0); assert(ui_currentNetMap.integer == 1);
    assert(!strcmp(shaderName, "levelshots/preview/ironworks") && stoppedCinematics == 1);
    assert(uiInfo.serverStatus.currentServerPreview == 1 && uiInfo.serverStatus.currentServerCinematic == -1);
    assert(UI_QLScript("voteMap") && !strcmp(command, "callvote map ironworks ctf\n"));
    before = commands; UI_QLSelectMap(20); UI_QLScript("voteMap"); assert(commands == before);
    ui_currentNetMap.integer = 1; uiInfo.mapList[1].mapLoadName = "map;quit";
    UI_QLScript("voteMap"); assert(commands == before);
    uiInfo.playerCount = 1; uiInfo.playerIndex = 0; uiInfo.playerClientNums[0] = 7;
    catcher = KEYCATCH_CONSOLE | KEYCATCH_UI;
    UI_QLScript("kickPlayer"); assert(!strcmp(command, "callvote clientkick 7\n") && catcher == KEYCATCH_CONSOLE);
    before = commands; uiInfo.playerClientNums[0] = MAX_CLIENTS; UI_QLScript("banPlayer"); assert(commands == before);
    assert(!UI_QLScript("unknown"));
    {
        char value[128]; vmCvar_t announcer = {0};
        /* Retail 0x10011510: Bright from cg_* only; preset names persist. */
        Set("cg_forceTeamModel", "Keel/Bright"); Set("cg_forceTeamSkin", "");
        Set("ui_forceTeamModel", "Keel Bright"); UI_QLScript("teamModelChanged");
        Get("ui_forceTeamModelBright", value, sizeof(value)); assert(!strcmp(value, "1"));
        Get("ui_forceTeamModel", value, sizeof(value)); assert(!strcmp(value, "Keel Bright"));
        Set("cg_forceTeamSkin", "sport"); UI_QLScript("openWebGameSettings");
        Get("ui_forceTeamModelBright", value, sizeof(value)); assert(!strcmp(value, "0"));
        Set("cg_forceEnemyModel", "keel"); Set("cg_forceEnemySkin", "BRIGHT"); UI_QLScript("enemyModelChanged");
        Get("ui_forceEnemyModelBright", value, sizeof(value)); assert(!strcmp(value, "1"));
        /* Retail 0x1000c1d1..0x1000c285 and Leave 0x1000c059. */
        Set("r_fullscreen", "1"); assert(UI_QLScript("setWindowed"));
        Get("r_fullscreen", value, sizeof(value)); assert(!strcmp(value, "0") && !strcmp(command, "vid_restart fast\n"));
        assert(UI_QLScript("toggleFullscreen")); Get("r_fullscreen", value, sizeof(value)); assert(!strcmp(value, "1"));
        catcher = KEYCATCH_CONSOLE; assert(UI_QLScript("leave"));
        assert(!strcmp(command, "disconnect\n") && catcher == KEYCATCH_UI);
        /* Retail 0x10011690 announcer preview. */
        announcer.integer = 3; UI_QLAnnouncerChanged(&announcer);
        assert(soundsStarted == 1 && !strcmp(soundName, "sound/misc/vo_female.wav"));
        Get("ui_announcer", value, sizeof(value)); assert(!strcmp(value, "3"));
    }
    {
        /* Imports 93 and 96 and export 9 follow retail. */
        unsigned long long done, total;
        assert(((qboolean (QDECL *)(void))exports[9])());
        assert(!trap_IsSubscribedApp(343500));
        imports.IsSubscribedApp = Subscribed; assert(trap_IsSubscribedApp(343500) && subscribedApp == 343500);
        trap_DownloadInfo(0x123456789abcull, &done, &total); assert(!done && !total);
        imports.DownloadInfo = ItemInfo; trap_DownloadInfo(0x123456789abcull, &done, &total);
        assert(done == 5 && total == 10 && infoLow == 0x56789abcu && infoHigh == 0x1234u);
    }
    assert(UI_QLArenaTypeBits("ctf") == (1 << GT_CTF));
    assert(UI_QLArenaTypeBits("oneflag") == (1 << GT_1FCTF));
    puts("UI native ABI, text, menus, presets, filters and command boundaries OK");
    return 0;
}
