/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"

/* Retail cgame 10042e50 compares the selected snapshot weapon's ammo to a
 * FLOAT threshold (weapon table 10079078 + weapon*30h). Other weapons and
 * the loadout's total ammo do not affect the warning. */
static int CG_SelectedAmmoWarning(const playerState_t *ps) {
    int ammo, capacity;
    float fraction = cg_lowAmmoWarningPercentile.value;
    if (!ps || ps->weapon <= WP_NONE || ps->weapon >= WP_NUM_WEAPONS) return 0;
    ammo = ps->ammo[ps->weapon];
    capacity = bg_weaponMaxAmmo[ps->weapon];
    if (ammo < 0 || capacity <= 0) return 0;
    if (fraction != fraction) fraction = 0.20f;
    if (fraction < 0.01f) fraction = 0.01f;
    if (fraction > 1.0f) fraction = 1.0f;
    if (ammo >= capacity * fraction) return 0;
    return ammo ? 1 : 2;
}

void CG_CheckAmmo(void) {
    int warning = CG_SelectedAmmoWarning(cg.snap ? &cg.snap->ps : NULL);
    int previous = cg.lowAmmoWarning;
    sfxHandle_t sound = 0;
    cg.lowAmmoWarning = warning;
    if (warning == previous) return;
    if (warning == 2) sound = cgs.media.noAmmoSound;
    else if (warning == 1) {
        // Retail compares the float setting exactly; fractional modes mute.
        if (cg_lowAmmoWarningSound.value == 1.0f) sound = cgs.media.lowAmmoSound;
        else if (cg_lowAmmoWarningSound.value == 2.0f) sound = cgs.media.noAmmoSound;
    }
    if (sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND);
}

void CG_DrawAmmoWarning(void) {
    const char *text;
    float scale, y;
    int width;
    if (!cg_drawAmmoWarning.integer || !cg.lowAmmoWarning) return;
    text = cg.lowAmmoWarning == 2 ? "OUT OF AMMO" : "LOW AMMO WARNING";
    // Retail 1000e390: two sizes, centered, normal font, stronger shadow.
    scale = cg_drawAmmoWarning.integer == 1 ? 0.35f : 0.24f;
    y = cg_drawAmmoWarning.integer == 1 ? 55.0f : 52.0f;
    CG_SetTextFont(0);
    width = CG_Text_Width(text,scale,0);
    CG_Text_Paint(320 - width / 2, y, scale, colorWhite, text, 0, 0, 6);
}
