/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b)) < .001)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
static snapshot_t snapshot;
static int pictures, texts, resets, lastStyle, lastShader;
static float px,py,pw,ph,uv[4],textScale;
static vec4_t tint, input={.1f,.2f,.3f,.4f};
static char painted[32];
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void trap_R_SetColor(const float *color) {
    if (color) memcpy(tint,color,sizeof(tint)); else ++resets;
}
void CG_AdjustFrom640(float *x,float *y,float *w,float *h) {
    *x=*x*cgs.screenXScale+cgs.screenXBias; *y=*y*cgs.screenYScale+cgs.screenYBias;
    *w*=cgs.screenXScale; *h*=cgs.screenYScale;
}
void trap_R_DrawStretchPic(float x,float y,float w,float h,float s,float t,float s2,float t2,qhandle_t shader) {
    ++pictures; px=x; py=y; pw=w; ph=h; uv[0]=s; uv[1]=t; uv[2]=s2; uv[3]=t2; lastShader=shader;
}
void CG_DrawPic(float x,float y,float w,float h,qhandle_t shader) {
    CG_AdjustFrom640(&x,&y,&w,&h); trap_R_DrawStretchPic(x,y,w,h,0,0,1,1,shader);
}
int CG_Text_Width(const char *text,float scale,int limit) { (void)scale; CHECK(limit==0); return (int)strlen(text)*10; }
int CG_Text_Height(const char *text,float scale,int limit) { (void)text; (void)scale; CHECK(limit==0); return 17; }
void CG_Text_Paint(float x,float y,float scale,vec4_t color,const char *text,float adjust,int limit,int style) {
    CHECK(adjust==0 && limit==0); ++texts; px=x; py=y; textScale=scale; lastStyle=style;
    memcpy(tint,color,sizeof(tint)); Q_strncpyz(painted,text,sizeof(painted));
}
static void Clear(void) { pictures=texts=resets=0; }
static void Draw(int owner,int align,int shader) {
    Clear(); CHECK(CG_DrawStatusHud(owner,100,200,128,32,align,.62f,input,shader,3));
}
static void Bar(int owner,int value,float x,float y,float w,float h,float s,float t,float s2) {
    int stat=owner==CG_PLAYER_HEALTH_BAR_100 || owner==CG_PLAYER_HEALTH_BAR_200 ? STAT_HEALTH : STAT_ARMOR;
    snapshot.ps.stats[stat]=value; Draw(owner,0,7);
    CHECK(pictures==1 && !texts && resets==1 && lastShader==7);
    NEAR(px,x); NEAR(py,y); NEAR(pw,w); NEAR(ph,h); NEAR(uv[0],s); NEAR(uv[1],t); NEAR(uv[2],s2); NEAR(uv[3],1);
}
int main(void) {
    int i;
    vec4_t original;
    static const float colors[5][3]={{1,.8f,.2f},{1,.2f,.1f},{.2f,.4f,1},{.75f,.75f,.75f},{1,1,1}};
    memcpy(original,input,sizeof(input));
    cg.snap=&snapshot; snapshot.ps.clientNum=5;
    cgs.screenXScale=cgs.screenYScale=1; cgs.media.whiteShader=9; cgs.media.infiniteAmmoShader=11;
    snapshot.ps.stats[STAT_HEALTH]=125; snapshot.ps.stats[STAT_ARMOR]=50; snapshot.ps.stats[STAT_MAX_HEALTH]=100;
    Draw(CG_PLAYER_HEALTH,ITEM_ALIGN_CENTER,0);
    CHECK(texts==1 && pictures==0 && !strcmp(painted,"125") && lastStyle==3); NEAR(px,85); NEAR(py,217); NEAR(textScale,.62f);
    Draw(CG_PLAYER_ARMOR_VALUE,ITEM_ALIGN_RIGHT,0); CHECK(!strcmp(painted,"50")); NEAR(px,80);
    Draw(CG_PLAYER_HEALTH,ITEM_ALIGN_LEFT,0); NEAR(px,100);
    snapshot.ps.stats[STAT_HEALTH]=-25; Draw(CG_PLAYER_HEALTH,ITEM_ALIGN_LEFT,0); CHECK(!strcmp(painted,"-25"));
    Draw(CG_PLAYER_ARMOR_VALUE,ITEM_ALIGN_CENTER,7); CHECK(pictures==1 && texts==0 && resets==1); NEAR(px,100); NEAR(pw,128); NEAR(tint[3],.4f);
    cg_entities[5].currentState.weapon=WP_ROCKET_LAUNCHER; cg.predictedPlayerState.weapon=WP_RAILGUN;
    snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=23; snapshot.ps.ammo[WP_RAILGUN]=99;
    CHECK(CG_StatusAmmo()==23); Draw(CG_PLAYER_AMMO_VALUE,ITEM_ALIGN_CENTER,0); CHECK(!strcmp(painted,"23")); NEAR(px,90);
    snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=-1;
    Draw(CG_PLAYER_AMMO_VALUE,ITEM_ALIGN_CENTER,7); CHECK(pictures==1 && texts==0 && lastShader==11); NEAR(px,84); NEAR(pw,32); NEAR(ph,32);
    Draw(CG_PLAYER_AMMO_VALUE,ITEM_ALIGN_RIGHT,0); NEAR(px,68);
    Clear(); CHECK(CG_DrawStatusHud(CG_PLAYER_AMMO_VALUE,101.8f,200,128,33,1,.62f,input,0,3)); NEAR(px,84);
    cgs.media.infiniteAmmoShader=0; Draw(CG_PLAYER_AMMO_VALUE,0,0); CHECK(!pictures && !texts); cgs.media.infiniteAmmoShader=11;
    snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=-2; Draw(CG_PLAYER_AMMO_VALUE,0,0); CHECK(!pictures && !texts);
    for (i=0;i<WP_NUM_WEAPONS;++i) {
        cg_entities[5].currentState.weapon=i; snapshot.ps.ammo[i]=-1; Draw(CG_PLAYER_AMMO_VALUE,0,0);
        CHECK(pictures==(i!=WP_NONE && i!=WP_GAUNTLET && i!=WP_GRAPPLING_HOOK));
    }
    cg_entities[5].currentState.weapon=WP_NUM_WEAPONS; CHECK(CG_StatusAmmo()==-1);
    Draw(CG_PLAYER_AMMO_VALUE,0,0); CHECK(!pictures && !texts);
    snapshot.ps.clientNum=MAX_CLIENTS; Draw(CG_PLAYER_AMMO_VALUE,0,0); CHECK(!pictures && !texts && CG_StatusAmmo()==-1);
    snapshot.ps.clientNum=-1; CHECK(CG_StatusAmmo()==-1); snapshot.ps.clientNum=5;
    Bar(CG_PLAYER_HEALTH_BAR_100,25,100,200,32,32,0,0,.25f);
    Bar(CG_PLAYER_HEALTH_BAR_100,250,100,200,128,32,0,0,1);
    Bar(CG_PLAYER_ARMOR_BAR_100,25,196,200,32,32,.75f,0,1);
    Bar(CG_PLAYER_ARMOR_BAR_100,250,100,200,128,32,0,0,1);
    Bar(CG_PLAYER_HEALTH_BAR_200,150,100,221,128,11,0,.65625f,1);
    Bar(CG_PLAYER_HEALTH_BAR_200,250,100,210,128,22,0,.3125f,1);
    Bar(CG_PLAYER_ARMOR_BAR_200,150,100,221,128,11,0,.65625f,1);
    Bar(CG_PLAYER_ARMOR_BAR_200,300,100,188,128,44,0,-.375f,1);
    snapshot.ps.stats[STAT_MAX_HEALTH]=150;
    Bar(CG_PLAYER_HEALTH_BAR_100,75,100,200,64,32,0,0,.5f);
    Bar(CG_PLAYER_HEALTH_BAR_200,225,100,221,128,11,0,.65625f,1);
    snapshot.ps.stats[STAT_MAX_HEALTH]=0;
    Bar(CG_PLAYER_HEALTH_BAR_100,50,100,200,64,32,0,0,.5f);
    snapshot.ps.stats[STAT_HEALTH]=INT_MIN; Draw(CG_PLAYER_HEALTH_BAR_200,0,7); CHECK(!pictures);
    snapshot.ps.stats[STAT_ARMOR]=0; Draw(CG_PLAYER_ARMOR_BAR_100,0,7); CHECK(!pictures);
    for (i=0;i<5;++i) {
        snapshot.ps.persistant[PERS_TEAM]=i; snapshot.ps.stats[STAT_HEALTH]=100;
        Draw(CG_TEAM_COLORIZED,0,7); CHECK(pictures==1 && lastShader==7 && resets==1);
        NEAR(tint[0],colors[i][0]); NEAR(tint[1],colors[i][1]); NEAR(tint[2],colors[i][2]); NEAR(tint[3],.4f);
        Draw(CG_PLAYER_HEALTH_BAR_100,0,7); NEAR(tint[3],1);
    }
    snapshot.ps.persistant[PERS_TEAM]=-1; Draw(CG_TEAM_COLORIZED,0,0); NEAR(tint[0],1); CHECK(lastShader==9);
    cgs.screenXScale=1.25f; cgs.screenYScale=1.5f; cgs.screenXBias=80; cgs.screenYBias=10;
    Bar(CG_PLAYER_HEALTH_BAR_100,50,205,310,80,48,0,0,.5f);
    Clear(); CHECK(CG_DrawStatusHud(CG_PLAYER_HEALTH_BAR_100,0,0,0,10,0,1,input,7,0)); CHECK(!pictures);
    CHECK(CG_DrawStatusHud(CG_PLAYER_HEALTH_BAR_100,NAN,0,10,10,0,1,input,7,0)); CHECK(!pictures);
    CHECK(!CG_DrawStatusHud(CG_PLAYER_SCORE,0,0,10,10,0,1,input,7,0));
    cg.snap=NULL; Draw(CG_PLAYER_HEALTH,0,0); CHECK(!pictures && !texts && CG_StatusAmmo()==-1);
    CHECK(!memcmp(input,original,sizeof(input)));
    puts("cgame status HUD: anchors, infinite ammo, bar crops, team colors and bounds passed");
    return 0;
}
