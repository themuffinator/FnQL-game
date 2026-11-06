/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <float.h>
#include <limits.h>

static int cg_textFont;
void CG_SetTextFont(int font) { cg_textFont = font >= 0 && font <= 2 ? font : 0; }

/* Retail cgame 100082b0/10008440 uses vidHeight / 768 * 96 for both
 * measurement and painting. Rasterized bounds must be measured at that
 * physical size, then converted back to the existing logical HUD canvas. */
static float CG_TextPixelSize(float scale) {
    double size = (double)scale * cgs.glconfig.vidHeight / 8.0;
    if (!(cgs.screenXScale > 0 && cgs.screenXScale <= FLT_MAX) ||
        !(cgs.screenYScale > 0 && cgs.screenYScale <= FLT_MAX) ||
        cgs.glconfig.vidHeight <= 0 || !(size > 0 && size <= FLT_MAX)) return 0;
    return (float)size;
}
static qboolean CG_TextCoordinate(float value) {
    return (double)value >= INT_MIN && (double)value <= INT_MAX;
}
static int CG_TextDimension(const char *text, int font, float scale, int limit, qboolean height) {
    float bounds[5], size = CG_TextPixelSize(scale);
    double extent;
    if (!text || !*text || !size) return 0;
    trap_R_MeasureTextBounds(text, font, size, limit, bounds);
    /* Import 124: left, top, right, bottom, ascent. Height is ascent, not
     * bottom minus top (retail 10008327). Use double before subtracting. */
    extent = height ? (double)bounds[4] / cgs.screenYScale :
        ((double)bounds[2] - bounds[0]) / cgs.screenXScale;
    if (!(extent > 0)) return 0;
    if (extent >= INT_MAX) return INT_MAX;
    return (int)extent;
}
int CG_Text_Width(const char *text, float scale, int limit) {
    return CG_TextDimension(text, cg_textFont, scale, limit, qfalse);
}
int CG_Text_Height(const char *text, float scale, int limit) {
    return CG_TextDimension(text, cg_textFont, scale, limit, qtrue);
}
/* Retail 0x100082b0 takes the font per call; the overhead names measure
 * with font 0 while fitting and font 1 for the box. */
int CG_Text_WidthInFont(const char *text, int font, float scale, int limit) {
    return CG_TextDimension(text, font >= 0 && font <= 2 ? font : 0, scale, limit, qfalse);
}
int CG_Text_HeightInFont(const char *text, int font, float scale, int limit) {
    return CG_TextDimension(text, font >= 0 && font <= 2 ? font : 0, scale, limit, qtrue);
}

/* Glyph-quad helper retained from cg_draw.c, Copyright (C) 1999-2000
 * Id Software, Inc. */
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
    float size = CG_TextPixelSize(scale);
    float px = x * cgs.screenXScale + CG_ScreenXBias();
    float py = y * cgs.screenYScale + cgs.screenYBias;
    if (!text || !size || !CG_TextCoordinate(px) || !CG_TextCoordinate(py)) return;
    if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
        float offset = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
        float sx = px + offset * cgs.screenXScale;
        float sy = py + offset * cgs.screenYScale;
        vec4_t shadow = {0, 0, 0, color[3]};
        trap_R_SetColor(shadow);
        if (CG_TextCoordinate(sx) && CG_TextCoordinate(sy))
            trap_R_DrawText((int)sx, (int)sy, text, cg_textFont, size, limit, 1);
    }
    /* Retail 0x10008440 never reads its adjust argument (stack slot 7): the
     * string always goes to the engine whole, so extra glyph spacing has no
     * effect, as in retail. */
    (void)adjust;
    trap_R_SetColor(color);
    trap_R_DrawText((int)px, (int)py, text, cg_textFont, size, limit, 0);
    trap_R_SetColor(NULL);
}

/* Retail 100087c0: native glyph clipping returns its boundary through maxX.
 * Keep this separate from legacy extra-spacing and shadow painting. */
void CG_Text_PaintClipped(float *maxX, float x, float y, float scale, vec4_t color, const char *text) {
    float size=CG_TextPixelSize(scale), edge;
    float bias=CG_ScreenXBias();
    float px=x*cgs.screenXScale+bias;
    float py=y*cgs.screenYScale+cgs.screenYBias;
    if (!maxX || !text || !size || !CG_TextCoordinate(px) || !CG_TextCoordinate(py)) return;
    edge=*maxX*cgs.screenXScale+bias;
    if (!CG_TextCoordinate(edge)) return;
    trap_R_SetColor(color);
    trap_R_DrawTextClipped((int)px,(int)py,text,cg_textFont,size,&edge);
    trap_R_SetColor(NULL);
    if (CG_TextCoordinate(edge)) *maxX=(edge-bias)/cgs.screenXScale;
}


