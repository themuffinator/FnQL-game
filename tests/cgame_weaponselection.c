/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define DECLARE_CG_CVAR
#include "../code/cgame/cg_cvar.h"
#undef DECLARE_CG_CVAR
cg_t cg;
cgs_t cgs;
static snapshot_t snapshot;
static const char *argument;
static char console[MAX_STRING_CHARS], primary[16];
static int sounds, commands;
const char *CG_Argv(int index) { CHECK(index == 1); return argument; }
void trap_Cvar_Set(const char *name,const char *value) {
    CHECK(!strcmp(name,"cg_weaponPrimary")); Q_strncpyz(primary,value,sizeof(primary));
}
void trap_SendConsoleCommand(const char *text) {
    Q_strncpyz(console,text,sizeof(console)); ++commands;
}
void trap_S_StartSound(const vec3_t origin,int entity,int channel,sfxHandle_t sound) {
    CHECK(!origin && entity == snapshot.ps.clientNum && channel == CHAN_AUTO && sound == 71); ++sounds;
}
void QDECL Com_Printf(const char *format,...) { (void)format; }
void QDECL Com_Error(int code,const char *format,...) { (void)code; (void)format; abort(); }
static void Default(vmCvar_t *cvar,const char *value) {
    memset(cvar,0,sizeof(*cvar)); Q_strncpyz(cvar->string,value,sizeof(cvar->string));
    cvar->integer=atoi(value); cvar->value=(float)atof(value);
}
static void Reset(void) {
    memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs)); memset(&snapshot,0,sizeof(snapshot));
#define CG_CVAR(v,n,d,f,h) Default(&v,d);
#define CG_CVAR_RANGE(v,n,d,l,h,f) Default(&v,d);
#include "../code/cgame/cg_cvar.h"
    cg.snap=&snapshot; cg.time=1000; cg.weaponSelectTime=123;
    cg.weaponSelect=WP_MACHINEGUN; snapshot.ps.clientNum=7; snapshot.ps.weapon=WP_MACHINEGUN;
    cgs.media.noAmmoSound=71; commands=sounds=0; console[0]=primary[0]=0; argument="";
}
static void Own(int weapon,int ammo) {
    snapshot.ps.stats[STAT_WEAPONS] |= 1 << weapon; snapshot.ps.ammo[weapon]=ammo;
}
static void Select(const char *text) { argument=text; CG_Weapon_f(); }

static void Commands(void) {
    int i;
    static const char *invalid[]={"","g","-1","0","15","16","2147483648","4294967301","99999999999999999999999999"};
    Reset(); CHECK(cg_switchOnEmpty.integer==1 && cg_switchToEmpty.integer==1 && !cg_autoswitch.integer);
    Own(WP_GAUNTLET,-1); Own(WP_MACHINEGUN,50); Own(WP_SHOTGUN,0); Own(WP_ROCKET_LAUNCHER,5); Own(WP_HEAVY_MACHINEGUN,-2);
    /* Snapshot, not prediction: deliberately contradictory inventories. */
    cg.predictedPlayerState.stats[STAT_WEAPONS]=1 << WP_RAILGUN; cg.predictedPlayerState.ammo[WP_RAILGUN]=10;
    CG_NextWeapon_f(); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER && cg.weaponSelectTime==1000);
    CG_NextWeapon_f(); CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN);
    CG_NextWeapon_f(); CHECK(cg.weaponSelect==WP_MACHINEGUN);
    CG_PrevWeapon_f(); CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN);
    CG_PrevWeapon_f(); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    Select("3"); CHECK(cg.weaponSelect==WP_SHOTGUN); // direct empty enabled by default
    cg_switchToEmpty.integer=0; cg.weaponSelect=WP_MACHINEGUN; cg.weaponSelectTime=123;
    Select("3"); CHECK(cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==1000);
    Select("7"); CHECK(cg.weaponSelect==WP_MACHINEGUN); // prediction alone cannot grant ownership
    Select("  +0005suffix"); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    Select("14"); CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN); // every nonzero ammo value
    Select("1"); CHECK(cg.weaponSelect==WP_GAUNTLET);
    for(i=0;i<ARRAY_LEN(invalid);++i) {
        cg.weaponSelectTime=123; Select(invalid[i]);
        CHECK(cg.weaponSelect==WP_GAUNTLET && cg.weaponSelectTime==123);
    }
    snapshot.ps.pm_flags=PMF_FOLLOW; Select("5"); CG_NextWeapon_f(); CG_PrevWeapon_f();
    CHECK(cg.weaponSelect==WP_GAUNTLET && cg.weaponSelectTime==123);
    snapshot.ps.pm_flags=0; cg.demoPlayback=qtrue; Select("5"); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    CG_NextWeapon_f(); CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN);
    cg.snap=NULL; cg.weaponSelectTime=123; Select("2"); CG_NextWeapon_f(); CG_PrevWeapon_f();
    CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN && cg.weaponSelectTime==123);
    Reset(); Own(WP_GAUNTLET,-1); Own(WP_SHOTGUN,0); Own(15,1);
    CG_NextWeapon_f(); CG_PrevWeapon_f(); CHECK(cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==123);
    cg.weaponSelect=INT_MAX; CG_NextWeapon_f(); CHECK(cg.weaponSelect==INT_MAX);
    cg.weaponSelect=INT_MIN; CG_PrevWeapon_f(); CHECK(cg.weaponSelect==INT_MIN);
    Reset(); Own(WP_GRAPPLING_HOOK,-1); CG_PrevWeapon_f(); CHECK(cg.weaponSelect==WP_GRAPPLING_HOOK);
}

static void ToggleAndFrames(void) {
    Reset(); Own(WP_MACHINEGUN,50); Own(WP_ROCKET_LAUNCHER,5); Own(WP_RAILGUN,10);
    CG_UpdateWeaponSelection(); CHECK(cg.weaponSelectPrevious==0 && cg.weaponSelectObserved==WP_MACHINEGUN);
    Select("5"); Select("7"); CHECK(cg.weaponSelectPrevious==0);
    CG_UpdateWeaponSelection(); CHECK(cg.weaponSelectPrevious==WP_MACHINEGUN && cg.weaponSelectObserved==WP_RAILGUN);
    Select("TOGGLE"); CHECK(cg.weaponSelect==WP_MACHINEGUN);
    Select("togg-suffix"); CHECK(cg.weaponSelect==WP_MACHINEGUN); // no new rendered frame yet
    CG_UpdateWeaponSelection(); Select("toggle"); CHECK(cg.weaponSelect==WP_RAILGUN);
    cg.weaponSelectPrevious=WP_MACHINEGUN; snapshot.ps.ammo[WP_MACHINEGUN]=0; cg_switchToEmpty.integer=0;
    cg_weaponPrimary.integer=WP_ROCKET_LAUNCHER; Select("toggle"); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    cg.weaponSelectTime=123; cg_weaponPrimary.integer=WP_MACHINEGUN; Select("toggle");
    CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER && cg.weaponSelectTime==123); // zero fallback, not a third history entry
    cg_switchToEmpty.integer=1; Select("toggle"); CHECK(cg.weaponSelect==WP_MACHINEGUN);
    cg_switchToEmpty.integer=0; cg_weaponPrimary.integer=WP_ROCKET_LAUNCHER;
    cg.weaponSelectPrevious=WP_RAILGUN; snapshot.ps.stats[STAT_WEAPONS]&=~(1 << WP_RAILGUN);
    Select("toggle"); CHECK(cg.weaponSelect==WP_MACHINEGUN); // retained ammo + unowned does not trigger fallback
    cg.weaponSelectPrevious=INT_MAX; Select("toggle"); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    cg.weaponSelectPrevious=INT_MIN; cg_weaponPrimary.integer=INT_MAX; cg.weaponSelectTime=123;
    Select("toggle"); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER && cg.weaponSelectTime==123);
    Reset(); strcpy(cg_weaponConfig.string,"echo default"); strcpy(cg_weaponConfig_rl.string,"echo rocket; echo ready");
    CG_UpdateWeaponSelection(); CHECK(commands==1 && !strcmp(console,"echo default\n"));
    CG_UpdateWeaponSelection(); CHECK(commands==1);
    cg.weaponSelect=WP_ROCKET_LAUNCHER; CG_UpdateWeaponSelection(); CHECK(commands==2 && !strcmp(console,"echo rocket; echo ready\n"));
    cg.weaponSelect=WP_RAILGUN; CG_UpdateWeaponSelection(); CHECK(commands==3 && !strcmp(console,"echo default\n"));
    cg.weaponSelect=INT_MAX; CG_UpdateWeaponSelection(); CHECK(commands==3 && cg.weaponSelectPrevious==WP_RAILGUN);
    cg.weaponSelect=WP_MACHINEGUN; CG_UpdateWeaponSelection(); CHECK(commands==4 && cg.weaponSelectPrevious==0);
}

static void SpawnAndPrimary(void) {
    int i;
    static const char *tokens[]={"g","mg","sg","gl","rl","lg","rg","pg","bfg","gh","ng","pl","cg","hmg"};
    Reset();
    for(i=0;i<ARRAY_LEN(tokens);++i) {
        strcpy(cg_weaponPrimaryQueued.string,tokens[i]); CG_UpdateWeaponSelection(); CHECK(cg.weaponPrimary==i+1);
    }
    strcpy(cg_weaponPrimaryQueued.string," \"RG\" mg"); CG_UpdateWeaponSelection(); CHECK(cg.weaponPrimary==WP_RAILGUN);
    CG_SelectSpawnWeapon(); CHECK(!strcmp(primary,"7") && cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==1000);
    strcpy(cg_weaponPrimaryQueued.string,"7"); CG_UpdateWeaponSelection(); CHECK(cg.weaponPrimary==0);
    strcpy(cg_weaponPrimaryQueued.string,"bad rg"); CG_UpdateWeaponSelection(); CHECK(cg.weaponPrimary==0);
    Own(WP_ROCKET_LAUNCHER,0); Own(WP_RAILGUN,10);
    strcpy(cg_preferredStartingWeapons.string,"bad hmg RL rg"); CG_SelectSpawnWeapon(); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    strcpy(cg_preferredStartingWeapons.string,"999999999999999999999 15 7"); CG_SelectSpawnWeapon(); CHECK(cg.weaponSelect==WP_RAILGUN);
    strcpy(cg_preferredStartingWeapons.string,"1 1 1 1 1 1 1 1 1 1 1 1 1 1 rg");
    CG_SelectSpawnWeapon(); CHECK(cg.weaponSelect==WP_MACHINEGUN); // first fourteen tokens only
    cg.weaponPrimary=INT_MAX; snapshot.ps.weapon=INT_MAX; CG_SelectSpawnWeapon(); CHECK(cg.weaponSelect==0 && !strcmp(primary,"0"));
    cg.snap=NULL; CG_SelectSpawnWeapon(); CHECK(cg.weaponSelect==0);
}

static void EmptyAndPickup(void) {
    Reset(); Own(WP_GAUNTLET,-1); Own(WP_MACHINEGUN,0); Own(WP_RAILGUN,2); Own(WP_HEAVY_MACHINEGUN,0); Own(15,100);
    cg.predictedPlayerState.stats[STAT_WEAPONS]=1 << WP_HEAVY_MACHINEGUN; cg.predictedPlayerState.ammo[WP_HEAVY_MACHINEGUN]=99;
    CG_OutOfAmmoChange(); CHECK(cg.weaponSelect==WP_RAILGUN && cg.weaponSelectTime==1000 && !sounds);
    snapshot.ps.ammo[WP_RAILGUN]=0; CG_OutOfAmmoChange(); CHECK(cg.weaponSelect==WP_GAUNTLET);
    Own(WP_HEAVY_MACHINEGUN,-2); snapshot.ps.pm_flags=PMF_FOLLOW; cg.demoPlayback=qtrue;
    CG_OutOfAmmoChange(); CHECK(cg.weaponSelect==WP_HEAVY_MACHINEGUN); // event already targets snapshot client
    cg_switchOnEmpty.integer=0; cg.weaponSelectTime=123; CG_OutOfAmmoChange();
    CHECK(sounds==1 && cg.weaponSelectTime==123 && cg.weaponSelect==WP_HEAVY_MACHINEGUN);
    cg.snap=NULL; CG_OutOfAmmoChange(); CHECK(sounds==1);
    Reset(); CG_OutOfAmmoChange(); CHECK(cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==1000);
    cg.weaponSelectTime=123; CG_SelectPickedUpWeapon(WP_ROCKET_LAUNCHER); CHECK(cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==123);
    cg_autoswitch.integer=1; CG_SelectPickedUpWeapon(WP_ROCKET_LAUNCHER); CHECK(cg.weaponSelect==WP_ROCKET_LAUNCHER);
    CG_SelectPickedUpWeapon(WP_MACHINEGUN); CHECK(cg.weaponSelect==WP_MACHINEGUN && cg.weaponSelectTime==1000);
    CG_SelectPickedUpWeapon(INT_MAX); CG_SelectPickedUpWeapon(-1); CG_SelectPickedUpWeapon(15); CHECK(cg.weaponSelect==WP_MACHINEGUN);
}

int main(void) {
    Commands(); ToggleAndFrames(); SpawnAndPrimary(); EmptyAndPickup();
    puts("cgame weapon selection: snapshot policy, cycling/toggle/history, empty/pickup, spawn/primary and config dispatch OK");
    return 0;
}
