/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"

/* Retail weapon descriptors at 10079094 (48-byte stride). These tokens are
 * shared by preferredStartingWeapons and weaponPrimaryQueued. */
static const char *const weaponTokens[WP_NUM_WEAPONS] = {
    "", "g", "mg", "sg", "gl", "rl", "lg", "rg", "pg", "bfg",
    "gh", "ng", "pl", "cg", "hmg"
};

static qboolean ValidWeapon(int weapon) {
    return weapon > WP_NONE && weapon < WP_NUM_WEAPONS;
}

static qboolean OwnedWeapon(int weapon) {
    return cg.snap && ValidWeapon(weapon) &&
        (cg.snap->ps.stats[STAT_WEAPONS] & (1u << weapon));
}

/* Nonzero, including negative infinite-ammo values, is the shipping test
 * at 10053c15/10053d30/10053d69. Predicted inventory is not consulted. */
static qboolean LoadedWeapon(int weapon) {
    return OwnedWeapon(weapon) && cg.snap->ps.ammo[weapon] != 0;
}

static int WeaponToken(const char *token) {
    int weapon;
    for (weapon = WP_GAUNTLET; weapon < WP_NUM_WEAPONS; ++weapon)
        if (!Q_stricmp(token, weaponTokens[weapon])) return weapon;
    return WP_NONE;
}

/* Preserve atoi's decimal prefix syntax for valid weapon numbers without
 * its undefined overflowing conversion. Reserved slot 15 is not a weapon. */
static int WeaponNumber(const char *text) {
    unsigned value = 0;
    while (*text && (unsigned char)*text <= ' ') ++text;
    if (*text == '+') ++text;
    if (*text < '0' || *text > '9') return WP_NONE;
    do {
        value = value * 10 + (*text++ - '0');
        if (value >= WP_NUM_WEAPONS) return WP_NONE;
    } while (*text >= '0' && *text <= '9');
    return (int)value;
}

static void CycleWeapon(int direction) {
    int candidate, step;
    if (!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
        cg.weaponSelect < WP_NONE || cg.weaponSelect >= MAX_WEAPONS) return;
    candidate = cg.weaponSelect;
    /* 10053ba0: skip gauntlet/reserved slots, refresh the timer only on
     * success, and ignore switchToEmpty. Bound the retail reverse walk's
     * slot-16 read instead of indexing beyond the ammo array. */
    for (step = 0; step < MAX_WEAPONS; ++step) {
        candidate = (candidate + direction + MAX_WEAPONS) % MAX_WEAPONS;
        if (candidate != WP_GAUNTLET && LoadedWeapon(candidate)) {
            cg.weaponSelect = candidate;
            cg.weaponSelectTime = cg.time;
            return;
        }
    }
}

void CG_NextWeapon_f(void) { CycleWeapon(1); }
void CG_PrevWeapon_f(void) { CycleWeapon(-1); }

void CG_Weapon_f(void) {
    const char *argument;
    int weapon;
    if (!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW)) return;
    argument = CG_Argv(1);
    /* Shipping 10053c93 compares four case-insensitive characters. */
    if (!Q_stricmpn(argument, "toggle", 4)) {
        weapon = cg.weaponSelectPrevious;
        if ((!ValidWeapon(weapon) || !cg.snap->ps.ammo[weapon]) &&
            !cg_switchToEmpty.integer) {
            /* 10053cc9: only zero ammo triggers primary fallback, not a
             * missing ownership bit. The second fallback is initialized to
             * zero at 10029a3a and has no subsequent writer in retail. */
            weapon = weapon != cg_weaponPrimary.integer ? cg_weaponPrimary.integer : WP_NONE;
        }
    } else {
        weapon = WeaponNumber(argument);
    }
    if (!ValidWeapon(weapon)) return;
    /* 10053d18 precedes ownership/ammo rejection. */
    cg.weaponSelectTime = cg.time;
    if (OwnedWeapon(weapon) && (cg.snap->ps.ammo[weapon] || cg_switchToEmpty.integer))
        cg.weaponSelect = weapon;
}

void CG_OutOfAmmoChange(void) {
    int weapon;
    if (!cg.snap) return;
    /* EV_NOAMMO 1001a6bc-1001a6ec: the caller identifies the snapshot
     * client. Follow/demo events still take this path. */
    if (!cg_switchOnEmpty.integer) {
        trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.noAmmoSound);
        return;
    }
    cg.weaponSelectTime = cg.time;
    for (weapon = WP_NUM_WEAPONS - 1; weapon > WP_NONE; --weapon) {
        if (LoadedWeapon(weapon)) {
            cg.weaponSelect = weapon;
            return;
        }
    }
}

void CG_SelectPickedUpWeapon(int weapon) {
    /* 10019c8a-10019c98 includes the machinegun. */
    if (cg_autoswitch.integer && ValidWeapon(weapon)) {
        cg.weaponSelect = weapon;
        cg.weaponSelectTime = cg.time;
    }
}

void CG_SelectSpawnWeapon(void) {
    char *cursor = cg_preferredStartingWeapons.string;
    int tokenCount;
    if (!cg.snap) return;
    /* 100433d0 publishes the queued primary from the preceding frame. */
    trap_Cvar_Set("cg_weaponPrimary", va("%d", ValidWeapon(cg.weaponPrimary) ? cg.weaponPrimary : WP_NONE));
    cg.weaponSelectTime = cg.time;
    cg.weaponSelect = ValidWeapon(cg.snap->ps.weapon) ? cg.snap->ps.weapon : WP_NONE;
    /* 10043300: at most fourteen preference tokens, ownership only (an
     * empty preferred weapon remains eligible), then server selection. */
    for (tokenCount = 0; tokenCount < WP_NUM_WEAPONS - 1; ++tokenCount) {
        const char *token = COM_ParseExt(&cursor, qtrue);
        int weapon;
        if (!token[0]) break;
        weapon = WeaponToken(token);
        if (!weapon) weapon = WeaponNumber(token);
        if (OwnedWeapon(weapon)) {
            cg.weaponSelect = weapon;
            return;
        }
    }
}

void CG_UpdateWeaponSelection(void) {
    static vmCvar_t *const configs[WP_NUM_WEAPONS] = {
        NULL, &cg_weaponConfig_g, &cg_weaponConfig_mg, &cg_weaponConfig_sg,
        &cg_weaponConfig_gl, &cg_weaponConfig_rl, &cg_weaponConfig_lg,
        &cg_weaponConfig_rg, &cg_weaponConfig_pg, &cg_weaponConfig_bfg,
        &cg_weaponConfig_gh, &cg_weaponConfig_ng, &cg_weaponConfig_pl,
        &cg_weaponConfig_cg, &cg_weaponConfig_hmg
    };
    char *cursor = cg_weaponPrimaryQueued.string;
    /* 1004e660/10001df0: queued primary accepts the first named token,
     * unlike preferredStartingWeapons it does not accept numeric IDs. */
    cg.weaponPrimary = WeaponToken(COM_ParseExt(&cursor, qtrue));
    if (cg.weaponSelect != cg.weaponSelectObserved) {
        if (ValidWeapon(cg.weaponSelect)) {
            const char *commands = configs[cg.weaponSelect]->string;
            if (!commands[0]) commands = cg_weaponConfig.string;
            if (commands[0]) trap_SendConsoleCommand(va("%s\n", commands));
        }
        /* 1004e756: history observes the final selection once per frame,
         * not each command/pickup. This also sees spawn and drop changes. */
        cg.weaponSelectPrevious = ValidWeapon(cg.weaponSelectObserved) ? cg.weaponSelectObserved : WP_NONE;
        cg.weaponSelectObserved = cg.weaponSelect;
    }
}
