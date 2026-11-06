/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b))<.001)
cg_t cg;
cgs_t cgs;
weaponInfo_t cg_weapons[MAX_WEAPONS];
vmCvar_t cg_drawWeaponSelect,cg_weaponBar,cg_drawFullWeaponBar,cg_lowAmmoWeaponBarWarning,cg_lowAmmoWarningPercentile;
static snapshot_t snapshot;
static const char *practice="0";
static int nPics,nTexts,nLegacy,nRegisters,font,missingWeapon;
static vec4_t tint={1,1,1,1};
static struct { float x,y,w,h; int shader; vec4_t color; } pics[64];
static struct { float x,y,scale; int style; char text[64]; vec4_t color; } texts[32];
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code,const char *fmt,...) { (void)code; (void)fmt; abort(); }
void CG_StoreEvent(entity_event_t event,int parm,int ent) { (void)event; (void)parm; (void)ent; abort(); }
const char *CG_ConfigString(int id) { CHECK(id==CS_PRACTICE_FLAGS); return practice; }
void CG_RegisterWeapon(int weapon) {
    CHECK(weapon>0 && weapon<WP_NUM_WEAPONS); ++nRegisters;
    if (weapon==missingWeapon) return;
    cg_weapons[weapon].registered=qtrue; cg_weapons[weapon].weaponIcon=100+weapon;
    cg_weapons[weapon].item=BG_FindItemForWeapon(weapon);
}
void trap_R_SetColor(const float *color) {
    if (color) memcpy(tint,color,sizeof(tint)); else tint[0]=tint[1]=tint[2]=tint[3]=1;
}
void CG_DrawPic(float x,float y,float w,float h,qhandle_t shader) {
    CHECK(nPics<64); pics[nPics].x=x; pics[nPics].y=y; pics[nPics].w=w; pics[nPics].h=h;
    pics[nPics].shader=shader; memcpy(pics[nPics++].color,tint,sizeof(tint));
}
void CG_SetTextFont(int value) { font=value; }
int CG_Text_Width(const char *text,float scale,int limit) { (void)scale; CHECK(!limit); return (int)strlen(text)*7; }
void CG_Text_Paint(float x,float y,float scale,vec4_t color,const char *text,float adjust,int limit,int style) {
    CHECK(!adjust && !limit && nTexts<32); texts[nTexts].x=x; texts[nTexts].y=y; texts[nTexts].scale=scale; texts[nTexts].style=style;
    Q_strncpyz(texts[nTexts].text,text,sizeof(texts[nTexts].text)); memcpy(texts[nTexts++].color,color,sizeof(vec4_t));
    trap_R_SetColor(NULL);
}
float *CG_FadeColor(int start,int duration) {
    static vec4_t color={1,1,1,.75f}; CHECK(duration==WEAPON_SELECT_TIME);
    return start ? color : NULL;
}
void CG_DrawString(float x,float y,const char *str,const float *color,float w,float h,int maxChars,int flags) {
    (void)x;(void)y;(void)str;(void)color;(void)w;(void)h;(void)maxChars;(void)flags; ++nLegacy;
}
static void Run(void) { nPics=nTexts=nLegacy=nRegisters=0; CG_DrawWeaponSelect(); }
static int Pic(int shader) { int i; for(i=0;i<nPics;++i) if(pics[i].shader==shader) return i; return -1; }
static void Reset(void) {
    memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs)); memset(&snapshot,0,sizeof(snapshot)); memset(cg_weapons,0,sizeof(cg_weapons));
    cg.snap=&snapshot; cg.time=5000; cg.weaponSelectTime=100; cg.itemPickupTime=77;
    cg.predictedPlayerState.stats[STAT_HEALTH]=100;
    snapshot.ps.stats[STAT_WEAPONS]=(1<<WP_MACHINEGUN)|(1<<WP_ROCKET_LAUNCHER)|(1<<WP_RAILGUN);
    cg.predictedPlayerState.stats[STAT_WEAPONS]=snapshot.ps.stats[STAT_WEAPONS];
    snapshot.ps.weapon=WP_MACHINEGUN; cg.weaponSelect=WP_ROCKET_LAUNCHER;
    snapshot.ps.ammo[WP_MACHINEGUN]=0; snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=4; snapshot.ps.ammo[WP_RAILGUN]=-1;
    cgs.screenXScale=cgs.screenYScale=1; cgs.screenYmax=480;
    cgs.media.weaponBarLitShader=501; cgs.media.infiniteAmmoShader=502; cgs.media.selectShader=503; cgs.media.noammoShader=504;
    cg_weaponBar.integer=cg_drawWeaponSelect.integer=1; cg_drawFullWeaponBar.integer=0;
    cg_lowAmmoWeaponBarWarning.integer=2; cg_lowAmmoWeaponBarWarning.value=2; cg_lowAmmoWarningPercentile.value=.2f;
    practice="0"; missingWeapon=0; font=-1;
}
int main(void) {
    int i;
    Reset(); Run(); CHECK(nPics==5 && nTexts==2 && font==0 && cg.itemPickupTime==77);
    NEAR(pics[Pic(102)].x,1); NEAR(pics[Pic(102)].y,94); NEAR(pics[Pic(105)].y,116);
    NEAR(pics[Pic(501)].x,-3); NEAR(pics[Pic(501)].y,114); NEAR(pics[Pic(501)].w,52);
    NEAR(pics[Pic(502)].x,19); NEAR(pics[Pic(502)].y,138);
    CHECK(!strcmp(texts[0].text,"0") && !strcmp(texts[1].text,"4"));
    NEAR(texts[0].x,21); NEAR(texts[0].y,107); NEAR(texts[0].color[1],0); NEAR(texts[0].color[3],.8f);
    NEAR(texts[1].color[1],.8f); NEAR(texts[1].color[2],.2f); CHECK(texts[1].style==0);
    cg.weaponSelectTime=0; Run(); CHECK(nPics==5); // persistent, independent of switch timer
    cg_weaponBar.integer=2; Run(); NEAR(pics[Pic(102)].x,623); NEAR(pics[Pic(501)].x,590);
    NEAR(texts[0].x,613); NEAR(pics[Pic(502)].x,604);
    cg_weaponBar.integer=3; Run(); NEAR(pics[Pic(102)].x,237.5); NEAR(pics[Pic(107)].x,347.5); NEAR(pics[0].y,413); NEAR(pics[Pic(502)].x,359.5);
    cg_weaponBar.integer=1; cgs.screenXScale=1.25f; cgs.screenXBias=80; Run(); NEAR(pics[Pic(102)].x,-63); NEAR(texts[0].x,-43);
    cg_weaponBar.integer=2; Run(); NEAR(pics[Pic(102)].x,687); NEAR(texts[0].x,677);
    Reset(); cg_weapons[WP_SHOTGUN].weaponIcon=103; cg_drawFullWeaponBar.integer=1; Run(); CHECK(nPics==6 && nTexts==2 && nRegisters==3); NEAR(pics[Pic(105)].y,138);
    practice="1"; Run(); CHECK(Pic(103)==-1 && nPics==5); NEAR(pics[Pic(105)].y,138);
    practice="4294967296"; Run(); CHECK(Pic(103)>=0); practice="+1 "; Run(); CHECK(Pic(103)==-1);
    Reset(); cg.predictedPlayerState.pm_flags=PMF_FOLLOW; cg_weaponBar.integer=2; Run();
    NEAR(pics[Pic(102)].x,237.5); NEAR(pics[Pic(102)].y,452); NEAR(pics[Pic(501)].x,233.5);
    NEAR(texts[0].color[3],.6f); NEAR(texts[1].color[3],.6f);
    cg.scoreBoardShowing=qtrue; Run(); CHECK(!nPics && !nTexts);
    cg.scoreBoardShowing=qfalse; snapshot.ps.weapon=WP_ROCKET_LAUNCHER; Run(); NEAR(texts[0].color[3],.4f); NEAR(texts[1].color[3],1);
    Reset(); cg.demoPlayback=qtrue; Run(); NEAR(pics[Pic(501)].y,92);
    Reset(); cg_lowAmmoWarningPercentile.value=.21f; snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=5; Run(); NEAR(texts[1].color[1],1); // trunc(25*.21)==5
    snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=4; cg_lowAmmoWeaponBarWarning.value=1.5f; Run(); NEAR(texts[1].color[1],1);
    cg_lowAmmoWeaponBarWarning.integer=0; cg_lowAmmoWeaponBarWarning.value=0; Run(); NEAR(texts[0].color[1],1);
    Reset(); snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=INT_MAX; Run(); CHECK(!strcmp(texts[1].text,"2147483647"));
    snapshot.ps.ammo[WP_ROCKET_LAUNCHER]=-2; Run(); CHECK(nTexts==1); // retail bar accepts every negative ammo as infinite
    Reset(); cg_weaponBar.integer=4; cg.weaponSelectTime=4000; Run(); CHECK(nPics==5 && nTexts==1 && cg.itemPickupTime==0);
    NEAR(pics[Pic(102)].x,260); NEAR(pics[Pic(102)].y,380); NEAR(pics[Pic(503)].x,296); CHECK(Pic(504)>=0);
    CHECK(!strcmp(texts[0].text,"Rocket Launcher")); NEAR(texts[0].y,374); NEAR(texts[0].scale,.25f); CHECK(texts[0].style==3);
    cg.time=5300; Run(); NEAR(pics[0].color[3],.5f); cg.time=5400; Run(); CHECK(!nPics && !nTexts);
    cg.time=INT_MAX; cg.weaponSelectTime=INT_MIN; Run(); CHECK(!nPics); cg.time=100; cg.weaponSelectTime=200; Run(); NEAR(pics[0].color[3],1);
    Reset(); cg_weaponBar.integer=0; Run(); CHECK(!nPics); cg_weaponBar.integer=1; cg_drawWeaponSelect.integer=0; Run(); CHECK(!nPics);
    cg_drawWeaponSelect.integer=-2; cg.weaponSelectTime=0; Run(); CHECK(nPics>0); // inherited always-on path
    cg_drawWeaponSelect.integer=2; cg.weaponSelectTime=100; cg.predictedPlayerState.ammo[WP_MACHINEGUN]=7; Run(); CHECK(nLegacy>0);
    cg_drawWeaponSelect.integer=INT_MIN; Run(); CHECK(!nPics);
    Reset(); missingWeapon=WP_ROCKET_LAUNCHER; Run(); CHECK(Pic(105)==-1 && nPics==3);
    cg.predictedPlayerState.stats[STAT_HEALTH]=0; Run(); CHECK(!nPics); cg.predictedPlayerState.stats[STAT_HEALTH]=100;
    cg.snap=NULL; Run(); CHECK(!nPics && !nTexts);
    for(i=0;i<4;++i) NEAR(tint[i],1);
    puts("weapon bar: persistent/strip/legacy layouts, ammo, follow, anchors and bounds passed");
    return 0;
}
