/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_lowAmmoWarningPercentile, cg_lowAmmoWarningSound, cg_drawAmmoWarning;
static snapshot_t snapshot;
static int sounds, lastSound, textCalls, lastFont;
static float textX, textY, textScale;
static char lastText[32];
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
void CG_StoreEvent(entity_event_t event, int parm, int entity) {
    (void)event; (void)parm; (void)entity; abort();
}
void trap_S_StartLocalSound(sfxHandle_t sound, int channel) {
    CHECK(channel == CHAN_LOCAL_SOUND); lastSound = sound; ++sounds;
}
void CG_SetTextFont(int font) { lastFont = font; }
int CG_Text_Width(const char *text, float scale, int limit) {
    CHECK(!limit && scale > 0 && strlen(text) < sizeof(lastText)); return 101;
}
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text,
    float adjust, int limit, int style) {
    CHECK(!adjust && !limit && style == 6 && !memcmp(color,colorWhite,sizeof(vec4_t)));
    textX = x; textY = y; textScale = scale; strcpy(lastText,text); ++textCalls;
}
static void Check(int weapon, int ammo, int expected, int expectedSound) {
    int before = sounds;
    snapshot.ps.weapon = weapon;
    if (weapon >= 0 && weapon < MAX_WEAPONS) snapshot.ps.ammo[weapon] = ammo;
    CG_CheckAmmo();
    if (cg.lowAmmoWarning != expected) fprintf(stderr,"weapon %d ammo %d fraction %.6f: %d != %d\n",
        weapon,ammo,cg_lowAmmoWarningPercentile.value,cg.lowAmmoWarning,expected);
    CHECK(cg.lowAmmoWarning == expected);
    CHECK(sounds == before + !!expectedSound);
    if (expectedSound) CHECK(lastSound == expectedSound);
}
int main(void) {
    int weapon, before;
    cg.snap = &snapshot; cgs.media.lowAmmoSound = 10; cgs.media.noAmmoSound = 11;
    cg_lowAmmoWarningPercentile.value = .20f; cg_lowAmmoWarningSound.value = 1;
    Check(WP_ROCKET_LAUNCHER,5,0,0);
    Check(WP_ROCKET_LAUNCHER,4,1,10);
    Check(WP_ROCKET_LAUNCHER,1,1,0);
    Check(WP_ROCKET_LAUNCHER,0,2,11);
    Check(WP_ROCKET_LAUNCHER,0,2,0);
    Check(WP_ROCKET_LAUNCHER,2,1,10);
    Check(WP_ROCKET_LAUNCHER,25,0,0);
    // Unselected weapons and even the weapon ownership mask are irrelevant.
    snapshot.ps.ammo[WP_MACHINEGUN] = INT_MAX;
    snapshot.ps.stats[STAT_WEAPONS] = 0;
    cg.predictedPlayerState.weapon = WP_MACHINEGUN;
    Check(WP_ROCKET_LAUNCHER,2,1,10);
    Check(WP_GAUNTLET,-1,0,0); Check(WP_GRAPPLING_HOOK,-1,0,0);
    Check(WP_GAUNTLET,0,0,0); Check(WP_MACHINEGUN,-1,0,0);
    Check(WP_MACHINEGUN,-2,0,0); // corrupt negative ammo is not a false warning
    Check(WP_NONE,0,0,0); Check(-1,0,0,0); Check(WP_NUM_WEAPONS,0,0,0);
    for (weapon = WP_MACHINEGUN; weapon < WP_NUM_WEAPONS; ++weapon) {
        int threshold = bg_weaponMaxAmmo[weapon] / 5;
        if (bg_weaponMaxAmmo[weapon] <= 0) continue;
        Check(weapon,threshold,0,0);
        if (threshold > 1) Check(weapon,threshold-1,1,10);
        Check(weapon,0,2,11);
        Check(weapon,INT_MAX,0,0);
        Check(weapon,-1,0,0);
    }
    cg_lowAmmoWarningPercentile.value = .22f;
    Check(WP_ROCKET_LAUNCHER,5,1,10); // 5 < 5.5, no integer truncation
    Check(WP_ROCKET_LAUNCHER,6,0,0);
    cg_lowAmmoWarningPercentile.value = .21f;
    Check(WP_PROX_LAUNCHER,1,1,10); Check(WP_PROX_LAUNCHER,2,0,0);
    cg_lowAmmoWarningPercentile.value = 1; Check(WP_ROCKET_LAUNCHER,24,1,10);
    Check(WP_ROCKET_LAUNCHER,25,0,0);
    cg_lowAmmoWarningPercentile.value = NAN; Check(WP_ROCKET_LAUNCHER,4,1,10);
    cg_lowAmmoWarningPercentile.value = -1; Check(WP_ROCKET_LAUNCHER,1,0,0);
    Check(WP_ROCKET_LAUNCHER,0,2,11);
    cg_lowAmmoWarningPercentile.value = INFINITY; Check(WP_ROCKET_LAUNCHER,24,1,10);
    Check(WP_ROCKET_LAUNCHER,25,0,0);
    cg_lowAmmoWarningPercentile.value = .2f;
    cg_lowAmmoWarningSound.value = 0;
    Check(WP_ROCKET_LAUNCHER,4,1,0); Check(WP_ROCKET_LAUNCHER,0,2,11);
    Check(WP_ROCKET_LAUNCHER,25,0,0);
    cg_lowAmmoWarningSound.value = 2;
    Check(WP_ROCKET_LAUNCHER,4,1,11); Check(WP_ROCKET_LAUNCHER,0,2,11);
    Check(WP_ROCKET_LAUNCHER,25,0,0);
    cg_lowAmmoWarningSound.value = 1.5f; Check(WP_ROCKET_LAUNCHER,4,1,0);
    cg.snap = NULL; CG_CheckAmmo(); CHECK(!cg.lowAmmoWarning);
    cg.snap = &snapshot; cgs.media.noAmmoSound = 0; Check(WP_ROCKET_LAUNCHER,0,2,0);
    cg_drawAmmoWarning.integer = 0; CG_DrawAmmoWarning(); CHECK(!textCalls);
    cg_drawAmmoWarning.integer = 2; lastFont = -1; CG_DrawAmmoWarning();
    CHECK(textCalls == 1 && lastFont == 0 && !strcmp(lastText,"OUT OF AMMO"));
    CHECK(textX == 270 && textY == 52 && textScale == .24f);
    cg.lowAmmoWarning = 1; cg_drawAmmoWarning.integer = 1; CG_DrawAmmoWarning();
    CHECK(textCalls == 2 && !strcmp(lastText,"LOW AMMO WARNING"));
    CHECK(textX == 270 && textY == 55 && textScale == .35f);
    before = textCalls; cg.lowAmmoWarning = 0; CG_DrawAmmoWarning(); CHECK(textCalls == before);
    puts("Selected weapon ammo, fractional thresholds, warning transitions and drawing passed");
    return 0;
}
