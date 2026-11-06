/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b)) < .001)
cg_t cg;
cgs_t cgs;
static int measures, draws, resets, measuredFont, measuredLimit;
static float measuredSize, boundsValue[5] = {-3, -18, 51, 4, 22};
static vec4_t currentColor;
static struct { int x,y,font,limit,force; float size; vec4_t color; } calls[8];
void trap_R_MeasureTextBounds(const char *text, int font, float size, int limit, float bounds[5]) {
    CHECK(!strcmp(text,"Ab")); ++measures;
    measuredFont=font; measuredLimit=limit; measuredSize=size;
    memcpy(bounds,boundsValue,sizeof(boundsValue));
}
void trap_R_MeasureText(const char *text, int font, float size, int limit, float *width, float *height) {
    (void)text; (void)font; (void)size; CHECK(limit==1); *width=10; *height=12;
}
void trap_R_SetColor(const float *color) {
    if (color) memcpy(currentColor,color,sizeof(currentColor)); else ++resets;
}
void trap_R_DrawText(int x, int y, const char *text, int font, float size, int limit, int force) {
    CHECK(text && draws<8);
    calls[draws].x=x; calls[draws].y=y; calls[draws].font=font;
    calls[draws].size=size; calls[draws].limit=limit; calls[draws].force=force;
    memcpy(calls[draws].color,currentColor,sizeof(currentColor)); ++draws;
}
void trap_R_DrawTextClipped(int x,int y,const char *text,int font,float size,float *edge) {
    CHECK(edge); NEAR(*edge,205);
    trap_R_DrawText(x,y,text,font,size,0,0); *edge=180;
}
float CG_ScreenXBias(void) { return cgs.screenXBias; }
void CG_AdjustFrom640(float *x, float *y, float *w, float *h) {
    *x=*x*cgs.screenXScale+cgs.screenXBias; *y=*y*cgs.screenYScale+cgs.screenYBias;
    *w*=cgs.screenXScale; *h*=cgs.screenYScale;
}
void trap_R_DrawStretchPic(float x,float y,float w,float h,float s,float t,float s2,float t2,qhandle_t shader) {
    CHECK(x==105 && y==50 && w==20 && h==10);
    CHECK(s==0 && t==0 && s2==1 && t2==1 && shader==7);
}
static void Geometry(int height,float sx,float sy,float bx,float by) {
    cgs.glconfig.vidHeight=height; cgs.screenXScale=sx; cgs.screenYScale=sy;
    cgs.screenXBias=bx; cgs.screenYBias=by; draws=resets=0;
}
int main(void) {
    vec4_t color={.2f,.4f,.6f,.8f};
    int before;
    Geometry(480,1,1,0,0); CG_SetTextFont(2);
    CHECK(CG_Text_Width("Ab",.5f,3)==54); CHECK(CG_Text_Height("Ab",.5f,3)==22);
    CHECK(measuredFont==2 && measuredLimit==3); NEAR(measuredSize,30);
    CG_Text_Paint(20,40,.5f,color,"Ab",0,3,ITEM_TEXTSTYLE_SHADOWED);
    CHECK(draws==2 && resets==1 && calls[0].x==21 && calls[0].y==41);
    CHECK(calls[0].force==1 && calls[1].force==0 && calls[1].limit==3);
    CHECK(calls[0].color[0]==0); NEAR(calls[0].color[3],.8f);
    NEAR(calls[1].color[0],.2f); NEAR(calls[1].size,measuredSize);
    Geometry(600,1.25f,1.25f,80,0);
    CHECK(CG_Text_Width("Ab",.5f,0)==43); CHECK(CG_Text_Height("Ab",.5f,0)==17);
    NEAR(measuredSize,37.5f);
    CG_Text_Paint(20,40,.5f,color,"Ab",0,0,ITEM_TEXTSTYLE_SHADOWEDMORE);
    CHECK(draws==2 && calls[0].x==107 && calls[0].y==52 && calls[1].x==105 && calls[1].y==50);
    NEAR(calls[1].size,measuredSize);
    CG_Text_PaintChar(20,40,8,4,2,0,0,1,1,7);
    {
        float edge=100;
        draws=resets=0; CG_Text_PaintClipped(&edge,20,40,.5f,color,"Ab");
        CHECK(draws==1 && resets==1 && calls[0].x==105 && calls[0].y==50 && calls[0].font==2);
        NEAR(edge,80); NEAR(calls[0].size,37.5); NEAR(calls[0].color[3],.8);
        CG_Text_PaintClipped(NULL,20,40,.5f,color,"Ab");
        edge=INFINITY; CG_Text_PaintClipped(&edge,20,40,.5f,color,"Ab");
        edge=100; CG_Text_PaintClipped(&edge,NAN,40,.5f,color,"Ab"); CHECK(draws==1);
    }
    Geometry(800,1,1,0,160); CG_SetTextFont(-1);
    CG_Text_Paint(20,40,.5f,color,"Ab",0,0,0);
    CHECK(draws==1 && calls[0].font==0 && calls[0].y==200); NEAR(calls[0].size,50);
    Geometry(600,2,1.25f,0,0); CG_SetTextFont(1);
    CG_Text_Paint(20,40,.5f,color,"Ab",0,0,ITEM_TEXTSTYLE_SHADOWEDMORE);
    CHECK(calls[0].x==44 && calls[0].y==52 && calls[1].font==1);
    Geometry(480,1,1,0,0); CG_SetTextFont(3);
    /* Retail ignores the adjust argument: one whole-string draw. */
    CG_Text_Paint(20,40,.5f,color,"Ab",2,2,0);
    CHECK(draws==1 && calls[0].font==0 && calls[0].x==20 && calls[0].limit==2);
    before=measures;
    CHECK(CG_Text_Width(NULL,1,0)==0 && CG_Text_Height("",1,0)==0);
    CHECK(CG_Text_Width("Ab",0,0)==0 && CG_Text_Width("Ab",-1,0)==0);
    CHECK(CG_Text_Width("Ab",NAN,0)==0 && CG_Text_Height("Ab",INFINITY,0)==0);
    CHECK(measures==before);
    draws=0;
    CG_Text_Paint(NAN,0,1,color,"Ab",0,0,0);
    CG_Text_Paint(INFINITY,0,1,color,"Ab",0,0,0);
    CG_Text_Paint((float)INT_MAX,0,1,color,"Ab",0,0,0);
    CG_Text_Paint(0,0,NAN,color,"Ab",0,0,0); CHECK(draws==0);
    boundsValue[2]=NAN; CHECK(CG_Text_Width("Ab",1,0)==0);
    boundsValue[4]=-1; CHECK(CG_Text_Height("Ab",1,0)==0);
    Geometry(0,1,1,0,0); CHECK(CG_Text_Width("Ab",1,0)==0);
    Geometry(480,0,1,0,0); CHECK(CG_Text_Height("Ab",1,0)==0);
    puts("cgame text: pixel sizing, five-float bounds, canvas, shadows and guards passed");
    return 0;
}
