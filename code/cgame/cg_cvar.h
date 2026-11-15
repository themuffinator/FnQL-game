#ifdef EXTERN_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags, description ) extern vmCvar_t vmCvar;
	#define CG_CVAR_RANGE( vmCvar, cvarName, defaultString, minimum, maximum, cvarFlags ) extern vmCvar_t vmCvar;
#endif

#ifdef DECLARE_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags, description ) vmCvar_t vmCvar;
	#define CG_CVAR_RANGE( vmCvar, cvarName, defaultString, minimum, maximum, cvarFlags ) vmCvar_t vmCvar;
#endif

#ifdef CG_CVAR_LIST
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags, description ) { & vmCvar, cvarName, defaultString, cvarFlags, description, NULL, NULL },
	#define CG_CVAR_RANGE( vmCvar, cvarName, defaultString, minimum, maximum, cvarFlags ) { & vmCvar, cvarName, defaultString, cvarFlags, NULL, minimum, maximum },
#endif

// The retail cgame cvar table (cgamex86.dll 0x100769e8, 295 rows of 24 bytes:
// vmCvar, name, default, minimum, maximum, flags), row for row and in retail
// order.  Each line ends with its retail row address.  Names, defaults, ranges
// and flags are held to docs/contract/retail_tables.json "cgameCvars" by gate
// G2 (scripts/check_wire_contract.py); the flag bits are the retail engine's
// (CVAR_PROTECTED, CVAR_VM_CREATED and CVAR_CLOUD decide how the engine
// guards and stores a setting), so they are part of the module ABI.
// A row with empty bounds registers through the plain import.
CG_CVAR( cg_armorTiered, "cg_armorTiered", "0", CVAR_ROM, NULL ) // 10076a00
CG_CVAR_RANGE( cg_addMarks, "cg_marks", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076a18
CG_CVAR_RANGE( cg_allowTaunt, "cg_allowTaunt", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076a30
CG_CVAR( cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT, NULL ) // 10076a48
CG_CVAR_RANGE( cg_announcer, "cg_announcer", "1", "1", "3", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076a60
CG_CVAR_RANGE( cg_announcerLastStandingVO, "cg_announcerLastStandingVO", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076a78
CG_CVAR_RANGE( cg_announcerLeadsVO, "cg_announcerLeadsVO", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076a90
CG_CVAR_RANGE( cg_announcerRewardsVO, "cg_announcerRewardsVO", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076aa8
CG_CVAR_RANGE( cg_announcerTiesVO, "cg_announcerTiesVO", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076ac0
CG_CVAR_RANGE( cg_autoAction, "cg_autoAction", "3", "0", "3", CVAR_ARCHIVE | CVAR_USERINFO | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076ad8
CG_CVAR_RANGE( cg_autoHop, "cg_autoHop", "1", "0", "1", CVAR_ARCHIVE | CVAR_USERINFO | CVAR_PROTECTED | CVAR_CLOUD ) // 10076af0
CG_CVAR_RANGE( cg_autoswitch, "cg_autoswitch", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076b08
CG_CVAR_RANGE( cg_autoProjectileNudge, "cg_autoProjectileNudge", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076b20
CG_CVAR( cg_bigFont, "ui_bigFont", "0.4", CVAR_ROM, NULL ) // 10076b38
CG_CVAR_RANGE( cg_blood, "cg_blood", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076b50
CG_CVAR( cg_scoreBlueTeamName, "cg_blueTeamName", "", 0, NULL ) // 10076b68
CG_CVAR_RANGE( cg_bob, "cg_bob", "0.25", "0.0", "1.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076b80
CG_CVAR_RANGE( cg_brassTime, "cg_brassTime", "2500", "0", "10000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076b98
CG_CVAR_RANGE( cg_bubbleTrail, "cg_bubbleTrail", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076bb0
CG_CVAR_RANGE( cg_buzzerSound, "cg_buzzerSound", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076bc8
CG_CVAR( cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT, NULL ) // 10076be0
CG_CVAR( cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT, NULL ) // 10076bf8
CG_CVAR( cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_CHEAT, NULL ) // 10076c10
CG_CVAR( cg_cameraSmartMode, "cg_cameraSmartMode", "0", CVAR_CHEAT, NULL ) // 10076c28
CG_CVAR( cg_cameraThirdPersonSmartMode, "cg_cameraThirdPersonSmartMode", "1", CVAR_CHEAT, NULL ) // 10076c40
CG_CVAR( cg_centertime, "cg_centertime", "3", CVAR_CHEAT, NULL ) // 10076c58
CG_CVAR_RANGE( cg_chatbeep, "cg_chatbeep", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076c70
CG_CVAR_RANGE( cg_chatHistoryLength, "cg_chatHistoryLength", "6", "0", "24", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076c88
CG_CVAR_RANGE( cg_complaintWarning, "cg_complaintWarning", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076ca0
CG_CVAR_RANGE( cg_crosshairBrightness, "cg_crosshairBrightness", "1.0", "0.0", "1.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076cb8
CG_CVAR_RANGE( cg_crosshairColor, "cg_crosshairColor", "25", "1", "26", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076cd0
CG_CVAR_RANGE( cg_crosshairHealth, "cg_crosshairHealth", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076ce8
CG_CVAR_RANGE( cg_crosshairHitColor, "cg_crosshairHitColor", "1", "1", "26", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d00
CG_CVAR_RANGE( cg_crosshairHitStyle, "cg_crosshairHitStyle", "1", "0", "8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d18
CG_CVAR_RANGE( cg_crosshairHitTime, "cg_crosshairHitTime", "200.0", "0.0", "1000.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d30
CG_CVAR_RANGE( cg_crosshairPulse, "cg_crosshairPulse", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d48
CG_CVAR_RANGE( cg_crosshairSize, "cg_crosshairSize", "32", "0", "320", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d60
CG_CVAR_RANGE( cg_crosshairX, "cg_crosshairX", "0", "-320", "320", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d78
CG_CVAR_RANGE( cg_crosshairY, "cg_crosshairY", "0", "-240", "240", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076d90
CG_CVAR( cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", 0, NULL ) // 10076da8
CG_CVAR( cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", 0, NULL ) // 10076dc0
CG_CVAR( cg_damagePlum, "cg_damagePlum", "g mg sg gl rl lg rg pg bfg gh cg ng pl hmg", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10076dd8
CG_CVAR_RANGE( cg_damagePlumColorStyle, "cg_damagePlumColorStyle", "1", "1", "3", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076df0
CG_CVAR( cg_deadBodyColor, "cg_deadBodyColor", "0x101010FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10076e08
CG_CVAR_RANGE( cg_deadBodyDarken, "cg_deadBodyDarken", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076e20
CG_CVAR( cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT, NULL ) // 10076e38
CG_CVAR( cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT, NULL ) // 10076e50
CG_CVAR( cg_debugFlags, "cg_debugFlags", "0", CVAR_CHEAT, NULL ) // 10076e68
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10076e80
CG_CVAR( cg_disableLoadout_g, "cg_disableLoadout_g", "0", CVAR_ROM, NULL ) // 10076e98
CG_CVAR( cg_disableLoadout_mg, "cg_disableLoadout_mg", "0", CVAR_ROM, NULL ) // 10076eb0
CG_CVAR( cg_disableLoadout_sg, "cg_disableLoadout_sg", "0", CVAR_ROM, NULL ) // 10076ec8
CG_CVAR( cg_disableLoadout_gl, "cg_disableLoadout_gl", "0", CVAR_ROM, NULL ) // 10076ee0
CG_CVAR( cg_disableLoadout_rl, "cg_disableLoadout_rl", "0", CVAR_ROM, NULL ) // 10076ef8
CG_CVAR( cg_disableLoadout_lg, "cg_disableLoadout_lg", "0", CVAR_ROM, NULL ) // 10076f10
CG_CVAR( cg_disableLoadout_rg, "cg_disableLoadout_rg", "0", CVAR_ROM, NULL ) // 10076f28
CG_CVAR( cg_disableLoadout_pg, "cg_disableLoadout_pg", "0", CVAR_ROM, NULL ) // 10076f40
CG_CVAR( cg_disableLoadout_bfg, "cg_disableLoadout_bfg", "0", CVAR_ROM, NULL ) // 10076f58
CG_CVAR( cg_disableLoadout_gh, "cg_disableLoadout_gh", "0", CVAR_ROM, NULL ) // 10076f70
CG_CVAR( cg_disableLoadout_ng, "cg_disableLoadout_ng", "0", CVAR_ROM, NULL ) // 10076f88
CG_CVAR( cg_disableLoadout_pl, "cg_disableLoadout_pl", "0", CVAR_ROM, NULL ) // 10076fa0
CG_CVAR( cg_disableLoadout_cg, "cg_disableLoadout_cg", "0", CVAR_ROM, NULL ) // 10076fb8
CG_CVAR( cg_disableLoadout_hmg, "cg_disableLoadout_hmg", "0", CVAR_ROM, NULL ) // 10076fd0
CG_CVAR_RANGE( cg_draw2D, "cg_draw2D", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10076fe8
CG_CVAR_RANGE( cg_draw3dIcons, "cg_draw3dIcons", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077000
CG_CVAR_RANGE( cg_drawAmmoWarning, "cg_drawAmmoWarning", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077018
CG_CVAR_RANGE( cg_drawAttacker, "cg_drawAttacker", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077030
CG_CVAR_RANGE( cg_drawCrosshair, "cg_drawCrosshair", "2", "0", "29", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077048
CG_CVAR_RANGE( cg_drawCrosshairTeamHealth, "cg_drawCrosshairTeamHealth", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077060
CG_CVAR_RANGE( cg_drawCrosshairTeamHealthSize, "cg_drawCrosshairTeamHealthSize", "0.12", "0.10", "0.26", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077078
CG_CVAR_RANGE( cg_drawDeadFriendTime, "cg_drawDeadFriendTime", "3000.0", "0.0", "5000.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077090
CG_CVAR_RANGE( cg_drawDemoHUD, "cg_drawDemoHUD", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100770a8
CG_CVAR_RANGE( cg_drawFPS, "cg_drawFPS", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100770c0
CG_CVAR_RANGE( cg_drawFragMessages, "cg_drawFragMessages", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100770d8
CG_CVAR_RANGE( cg_drawFullWeaponBar, "cg_drawFullWeaponBar", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100770f0
CG_CVAR_RANGE( cg_drawGun, "cg_drawGun", "1", "0", "3", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077108
CG_CVAR_RANGE( cg_drawHitFriendTime, "cg_drawHitFriendTime", "100", "0", "5000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077120
CG_CVAR_RANGE( cg_drawIcons, "cg_drawIcons", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077138
CG_CVAR_RANGE( cg_drawInputCmds, "cg_drawInputCmds", "0", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077150
CG_CVAR_RANGE( cg_drawInputCmdsSize, "cg_drawInputCmdsSize", "24", "16", "200", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077168
CG_CVAR_RANGE( cg_drawInputCmdsX, "cg_drawInputCmdsX", "320", "0", "640", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077180
CG_CVAR_RANGE( cg_drawInputCmdsY, "cg_drawInputCmdsY", "240", "0", "480", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077198
CG_CVAR_RANGE( cg_drawItemPickups, "cg_drawItemPickups", "3", "0", "7", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100771b0
CG_CVAR_RANGE( cg_drawPregameMessages, "cg_drawPregameMessages", "1", "0", "1", CVAR_VM_CREATED | CVAR_CLOUD ) // 100771c8
CG_CVAR_RANGE( cg_drawProfileImages, "cg_drawProfileImages", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100771e0
CG_CVAR_RANGE( cg_drawCheckpointRemaining, "cg_drawCheckpointRemaining", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100771f8
CG_CVAR_RANGE( cg_drawRewards, "cg_drawRewards", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077210
CG_CVAR_RANGE( cg_drawRewardsRowSize, "cg_drawRewardsRowSize", "1", "1", "9", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077228
CG_CVAR_RANGE( cg_drawSnapshot, "cg_drawSnapshot", "0", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077240
CG_CVAR_RANGE( cg_drawSpecMessages, "cg_drawSpecMessages", "1", "0", "1", CVAR_VM_CREATED | CVAR_CLOUD ) // 10077258
CG_CVAR_RANGE( cg_drawSprites, "cg_drawSprites", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077270
CG_CVAR_RANGE( cg_drawSpriteSelf, "cg_drawSpriteSelf", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077288
CG_CVAR_RANGE( cg_drawStatus, "cg_drawStatus", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100772a0
CG_CVAR_RANGE( cg_drawTeamOverlay, "cg_drawTeamOverlay", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100772b8
CG_CVAR_RANGE( cg_drawTeamOverlayOpacity, "cg_drawTeamOverlayOpacity", "0.75", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100772d0
CG_CVAR_RANGE( cg_drawTeamOverlaySize, "cg_drawTeamOverlaySize", "0.16", "0.12", "0.22", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100772e8
CG_CVAR_RANGE( cg_drawTeamOverlayX, "cg_drawTeamOverlayX", "0", "-640", "640", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077300
CG_CVAR_RANGE( cg_drawTeamOverlayY, "cg_drawTeamOverlayY", "0", "-480", "480", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077318
CG_CVAR_RANGE( cg_drawTieredArmorAvailability, "cg_drawTieredArmorAvailability", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077330
CG_CVAR( cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO, NULL ) // 10077348
CG_CVAR_RANGE( cg_enemyCrosshairNames, "cg_enemyCrosshairNames", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077360
CG_CVAR_RANGE( cg_enemyCrosshairNamesOpacity, "cg_enemyCrosshairNamesOpacity", "0.75", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077378
CG_CVAR( cg_enemyHeadColor, "cg_enemyHeadColor", "0x2a8000FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077390
CG_CVAR( cg_enemyLowerColor, "cg_enemyLowerColor", "0x2a8000FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100773a8
CG_CVAR( cg_enemyUpperColor, "cg_enemyUpperColor", "0x2a8000FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100773c0
CG_CVAR_RANGE( cg_errorDecay, "cg_errordecay", "100", "0", "100", CVAR_VM_CREATED | CVAR_CLOUD ) // 100773d8
CG_CVAR( cg_filter_angles, "cg_filter_angles", "0", 0, NULL ) // 100773f0
CG_CVAR_RANGE( cg_flagStyle, "cg_flagStyle", "1", "1", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077408
CG_CVAR_RANGE( cg_followKiller, "cg_followKiller", "0", "0", "1", CVAR_VM_CREATED ) // 10077420
CG_CVAR_RANGE( cg_followPowerup, "cg_followPowerup", "0", "0", "2", CVAR_VM_CREATED ) // 10077438
CG_CVAR( cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT, NULL ) // 10077450
CG_CVAR( cg_forceBlueTeamModel, "cg_forceBlueTeamModel", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077468
CG_CVAR_RANGE( cg_forceDrawCrosshair, "cg_forceDrawCrosshair", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077480
CG_CVAR( cg_forceEnemyModel, "cg_forceEnemyModel", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077498
CG_CVAR( cg_forceEnemySkin, "cg_forceEnemySkin", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100774b0
CG_CVAR_RANGE( cg_forceEnemyWeaponColor, "cg_forceEnemyWeaponColor", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100774c8
CG_CVAR( cg_forceRedTeamModel, "cg_forceRedTeamModel", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100774e0
CG_CVAR( cg_forceTeamModel, "cg_forceTeamModel", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100774f8
CG_CVAR( cg_forceTeamSkin, "cg_forceTeamSkin", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077510
CG_CVAR_RANGE( cg_forceTeamWeaponColor, "cg_forceTeamWeaponColor", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077528
CG_CVAR_RANGE( cg_fov, "cg_fov", "100", "10", "130", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077540
CG_CVAR( cg_gameInfo1, "cg_gameInfo1", "0", CVAR_ROM, NULL ) // 10077558
CG_CVAR( cg_gameInfo2, "cg_gameInfo2", "0", CVAR_ROM, NULL ) // 10077570
CG_CVAR( cg_gameInfo3, "cg_gameInfo3", "0", CVAR_ROM, NULL ) // 10077588
CG_CVAR( cg_gameInfo4, "cg_gameInfo4", "0", CVAR_ROM, NULL ) // 100775a0
CG_CVAR( cg_gameInfo5, "cg_gameInfo5", "0", CVAR_ROM, NULL ) // 100775b8
CG_CVAR( cg_gameInfo6, "cg_gameInfo6", "0", CVAR_ROM, NULL ) // 100775d0
CG_CVAR( cg_gametype, "cg_gametype", "0", CVAR_ROM, NULL ) // 100775e8
CG_CVAR_RANGE( cg_gun_x, "cg_gunX", "0", "-10", "10", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077600
CG_CVAR_RANGE( cg_gun_y, "cg_gunY", "0", "-10", "20", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077618
CG_CVAR_RANGE( cg_gun_z, "cg_gunZ", "0", "-8", "0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077630
CG_CVAR_RANGE( cg_hitBeep, "cg_hitBeep", "2", "0", "3", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077648
CG_CVAR( cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077660
CG_CVAR( cg_ignoreMouseInput, "cg_ignoreMouseInput", "0", CVAR_ROM, NULL ) // 10077678
CG_CVAR_RANGE( cg_impactMarkTime, "cg_impactMarkTime", "10000", "0", "10000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077690
CG_CVAR_RANGE( cg_impactSparks, "cg_impactSparks", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100776a8
CG_CVAR_RANGE( cg_impactSparksLifetime, "cg_impactSparksLifetime", "250", "0", "1000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100776c0
CG_CVAR_RANGE( cg_impactSparksSize, "cg_impactSparksSize", "8", "2", "16", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100776d8
CG_CVAR_RANGE( cg_impactSparksVelocity, "cg_impactSparksVelocity", "128", "-128", "128", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100776f0
CG_CVAR_RANGE( cg_itemFx, "cg_itemFx", "7", "0", "7", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077708
CG_CVAR_RANGE( cg_itemTimers, "cg_itemTimers", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077720
CG_CVAR_RANGE( cg_kickScale, "cg_kickScale", "0.25", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077738
CG_CVAR_RANGE( cg_killBeep, "cg_killBeep", "7", "0", "8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077750
CG_CVAR_RANGE( cg_lagometer, "cg_lagometer", "0", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077768
CG_CVAR( cg_lastmsg, "cg_lastmsg", "", 0, NULL ) // 10077780
CG_CVAR( cg_latchedHookOffset, "cg_latchedHookOffset", "1.0", CVAR_ROM, NULL ) // 10077798
CG_CVAR_RANGE( cg_levelTimerDirection, "cg_levelTimerDirection", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100777b0
CG_CVAR_RANGE( cg_lightningImpact, "cg_lightningImpact", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100777c8
CG_CVAR_RANGE( cg_lightningImpactCap, "cg_lightningImpactCap", "192", "0", "768", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100777e0
CG_CVAR_RANGE( cg_lightningStyle, "cg_lightningStyle", "1", "1", "5", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100777f8
CG_CVAR( cg_loadout, "cg_loadout", "0", CVAR_ROM, NULL ) // 10077810
CG_CVAR_RANGE( cg_lowAmmoWarningPercentile, "cg_lowAmmoWarningPercentile", "0.20", "0.01", "1.00", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077828
CG_CVAR_RANGE( cg_lowAmmoWarningSound, "cg_lowAmmoWarningSound", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077840
CG_CVAR_RANGE( cg_lowAmmoWeaponBarWarning, "cg_lowAmmoWeaponBarWarning", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077858
CG_CVAR_RANGE( cg_muzzleFlash, "cg_muzzleFlash", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077870
CG_CVAR( cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT, NULL ) // 10077888
CG_CVAR_RANGE( cg_nopredict, "cg_nopredict", "0", "0", "1", CVAR_PROTECTED | CVAR_VM_CREATED ) // 100778a0
CG_CVAR( cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", 0, NULL ) // 100778b8
CG_CVAR_RANGE( cg_obituaryRowSize, "cg_obituaryRowSize", "5", "0", "5", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED ) // 100778d0
CG_CVAR_RANGE( cg_overheadNamesWidth, "cg_overheadNamesWidth", "75", "50", "100", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100778e8
CG_CVAR( cg_paused, "cl_paused", "0", CVAR_ROM, NULL ) // 10077900
CG_CVAR( cg_preferredStartingWeapons, "cg_preferredStartingWeapons", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077918
CG_CVAR_RANGE( cg_plasmaStyle, "cg_plasmaStyle", "1", "1", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077930
CG_CVAR_RANGE( cg_playerLean, "cg_playerLean", "1", "0", "1", CVAR_ARCHIVE | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077948
CG_CVAR( cg_playerTeam, "cg_playerTeam", "", CVAR_ROM, NULL ) // 10077960
CG_CVAR( cg_playVoiceChats, "cg_playVoiceChats", "0", CVAR_CHEAT, NULL ) // 10077978
CG_CVAR_RANGE( cg_poiMaxWidth, "cg_poiMaxWidth", "32.0", "16.0", "32.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077990
CG_CVAR_RANGE( cg_poiMinWidth, "cg_poiMinWidth", "16.0", "2.0", "16.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100779a8
CG_CVAR_RANGE( cg_flagPOIs, "cg_flagPOIs", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100779c0
CG_CVAR_RANGE( cg_powerupPOIs, "cg_powerupPOIs", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100779d8
CG_CVAR( cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE | CVAR_USERINFO | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100779f0
CG_CVAR( cg_predictLocalRailshots, "cg_predictLocalRailshots", "1", 0, NULL ) // 10077a08
CG_CVAR( cg_projectileNudge, "cg_projectileNudge", "0", CVAR_CHEAT, NULL ) // 10077a20
CG_CVAR_RANGE( cg_raceBeep, "cg_raceBeep", "7", "0", "8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077a38
CG_CVAR_RANGE( cg_railStyle, "cg_railStyle", "1", "1", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077a50
CG_CVAR_RANGE( cg_railTrailTime, "cg_railTrailTime", "400", "0", "2000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077a68
CG_CVAR( cg_recordSPDemo, "ui_recordSPDemo", "0", 0, NULL ) // 10077a80
CG_CVAR( cg_recordSPDemoName, "ui_recordSPDemoName", "", 0, NULL ) // 10077a98
CG_CVAR( cg_scoreRedTeamName, "cg_redTeamName", "", 0, NULL ) // 10077ab0
CG_CVAR_RANGE( cg_rocketStyle, "cg_rocketStyle", "1", "1", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077ac8
CG_CVAR( cg_scalePlayerModelsToBB, "cg_scalePlayerModelsToBB", "1", CVAR_CHEAT, NULL ) // 10077ae0
CG_CVAR_RANGE( cg_scorePlum, "cg_scorePlums", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077af8
CG_CVAR( cg_screenDamage, "cg_screenDamage", "0x700000C8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077b10
CG_CVAR( cg_screenDamage_Self, "cg_screenDamage_Self", "0x00000000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077b28
CG_CVAR( cg_screenDamage_Team, "cg_screenDamage_Team", "0x700000C8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077b40
CG_CVAR( cg_screenDamageAlpha, "cg_screenDamageAlpha", "200", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077b58
CG_CVAR( cg_screenDamageAlpha_Team, "cg_screenDamageAlpha_Team", "200", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077b70
CG_CVAR_RANGE( cg_selfOnTeamOverlay, "cg_selfOnTeamOverlay", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077b88
CG_CVAR_RANGE( cg_shadows, "cg_shadows", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077ba0
CG_CVAR( cg_showmiss, "cg_showmiss", "0", 0, NULL ) // 10077bb8
CG_CVAR( cg_showVoiceText, "cg_showVoiceText", "0", CVAR_CHEAT, NULL ) // 10077bd0
CG_CVAR_RANGE( cg_simpleItems, "cg_simpleItems", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077be8
CG_CVAR_RANGE( cg_simpleItemsBob, "cg_simpleItemsBob", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077c00
CG_CVAR_RANGE( cg_simpleItemsHeightOffset, "cg_simpleItemsHeightOffset", "8", "0", "18", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077c18
CG_CVAR_RANGE( cg_simpleItemsRadius, "cg_simpleItemsRadius", "15", "8", "24", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077c30
CG_CVAR( cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO, NULL ) // 10077c48
CG_CVAR( cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO, NULL ) // 10077c60
CG_CVAR( g_skipTrainingEnable, "g_skipTrainingEnable", "0", 0, NULL ) // 10077c78
CG_CVAR( cg_smallFont, "ui_smallFont", "0.25", CVAR_ROM, NULL ) // 10077c90
CG_CVAR_RANGE( cg_smoke_SG, "cg_smoke_SG", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077ca8
CG_CVAR_RANGE( cg_smokeRadius_dust, "cg_smokeRadius_dust", "24", "0", "32", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077cc0
CG_CVAR_RANGE( cg_smokeRadius_flight, "cg_smokeRadius_flight", "8", "0", "16", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077cd8
CG_CVAR_RANGE( cg_smokeRadius_GL, "cg_smokeRadius_GL", "64", "0", "64", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077cf0
CG_CVAR_RANGE( cg_smokeRadius_haste, "cg_smokeRadius_haste", "8", "0", "16", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d08
CG_CVAR_RANGE( cg_smokeRadius_NG, "cg_smokeRadius_NG", "16", "0", "16", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d20
CG_CVAR_RANGE( cg_smokeRadius_RL, "cg_smokeRadius_RL", "32", "0", "32", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d38
CG_CVAR( cg_spectating, "cg_spectating", "0", CVAR_ROM, NULL ) // 10077d50
CG_CVAR_RANGE( cg_specDuelHealthArmor, "cg_specDuelHealthArmor", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d68
CG_CVAR_RANGE( cg_specDuelHealthColor, "cg_specDuelHealthColor", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d80
CG_CVAR_RANGE( cg_specFov, "cg_specFov", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077d98
CG_CVAR_RANGE( cg_specItemTimers, "cg_specItemTimers", "7", "0", "15", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077db0
CG_CVAR_RANGE( cg_specItemTimersSize, "cg_specItemTimersSize", "0.24", "0.12", "0.35", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077dc8
CG_CVAR_RANGE( cg_specItemTimersX, "cg_specItemTimersX", "10", "0", "640", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077de0
CG_CVAR_RANGE( cg_specItemTimersY, "cg_specItemTimersY", "200", "0", "480", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077df8
CG_CVAR_RANGE( cg_specNames, "cg_specNames", "2", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e10
CG_CVAR_RANGE( cg_specTeamVitals, "cg_specTeamVitals", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e28
CG_CVAR_RANGE( cg_specTeamVitalsHealthColor, "cg_specTeamVitalsHealthColor", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e40
CG_CVAR_RANGE( cg_specTeamVitalsWidth, "cg_specTeamVitalsWidth", "100", "60", "120", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e58
CG_CVAR_RANGE( cg_specTeamVitalsY, "cg_specTeamVitalsY", "85", "0", "480", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e70
CG_CVAR_RANGE( cg_speedometer, "cg_speedometer", "0", "0", "4", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077e88
CG_CVAR( cg_stats, "cg_stats", "0", CVAR_CHEAT, NULL ) // 10077ea0
CG_CVAR( cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE, NULL ) // 10077eb8
CG_CVAR( cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT, NULL ) // 10077ed0
CG_CVAR_RANGE( cg_switchOnEmpty, "cg_switchOnEmpty", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077ee8
CG_CVAR_RANGE( cg_switchToEmpty, "cg_switchToEmpty", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077f00
CG_CVAR_RANGE( cg_teamChatBeep, "cg_teamChatBeep", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077f18
CG_CVAR_RANGE( cg_teamChatHeight, "cg_teamChatHeight", "0", "0", "8", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077f30
CG_CVAR_RANGE( cg_teamChatsOnly, "cg_teamChatsOnly", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077f48
CG_CVAR_RANGE( cg_teamChatTime, "cg_teamChatTime", "3000", "0", "5000", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077f60
CG_CVAR( cg_teamHeadColor, "cg_teamHeadColor", "0x808080FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077f78
CG_CVAR( cg_teamLowerColor, "cg_teamLowerColor", "0x808080FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077f90
CG_CVAR( cg_teamUpperColor, "cg_teamUpperColor", "0x808080FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10077fa8
CG_CVAR_RANGE( cg_teammateCrosshairNames, "cg_teammateCrosshairNames", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077fc0
CG_CVAR_RANGE( cg_teammateCrosshairNamesOpacity, "cg_teammateCrosshairNamesOpacity", "0.75", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077fd8
CG_CVAR_RANGE( cg_teammateNames, "cg_teammateNames", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10077ff0
CG_CVAR_RANGE( cg_teammatePOIs, "cg_teammatePOIs", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078008
CG_CVAR_RANGE( cg_teammatePOIsMaxWidth, "cg_teammatePOIsMaxWidth", "24.0", "16.0", "32.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078020
CG_CVAR_RANGE( cg_teammatePOIsMinWidth, "cg_teammatePOIsMinWidth", "4.0", "2.0", "16.0", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078038
CG_CVAR( cg_thirdPerson, "cg_thirdPerson", "0", CVAR_CHEAT, NULL ) // 10078050
CG_CVAR( cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT, NULL ) // 10078068
CG_CVAR( cg_thirdPersonPitch, "cg_thirdPersonPitch", "-1", CVAR_CHEAT, NULL ) // 10078080
CG_CVAR( cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_CHEAT, NULL ) // 10078098
CG_CVAR_RANGE( cg_timescale, "timescale", "1", "0.1", "10", CVAR_VM_CREATED ) // 100780b0
CG_CVAR( cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0, NULL ) // 100780c8
CG_CVAR( cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0, NULL ) // 100780e0
CG_CVAR( cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT, NULL ) // 100780f8
CG_CVAR( cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT, NULL ) // 10078110
CG_CVAR( cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT, NULL ) // 10078128
CG_CVAR( cg_trackPlayer, "cg_trackPlayer", "-1", CVAR_CHEAT, NULL ) // 10078140
CG_CVAR( g_training, "g_training", "0", 0, NULL ) // 10078158
CG_CVAR_RANGE( cg_trueLightning, "cg_trueLightning", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078170
CG_CVAR_RANGE( cg_trueShotgun, "cg_trueShotgun", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078188
CG_CVAR_RANGE( cg_useItemMessage, "cg_useItemMessage", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100781a0
CG_CVAR_RANGE( cg_useItemWarning, "cg_useItemWarning", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100781b8
CG_CVAR_RANGE( cg_viewsize, "cg_viewsize", "100", "30", "100", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100781d0
CG_CVAR_RANGE( cg_vignette, "cg_vignette", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100781e8
CG_CVAR_RANGE( cg_voiceChatIndicator, "cg_voiceChatIndicator", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078200
CG_CVAR( ui_votestring, "ui_votestring", "", CVAR_CHEAT, NULL ) // 10078218
CG_CVAR_RANGE( cg_waterWarp, "cg_waterWarp", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078230
CG_CVAR_RANGE( cg_weaponBar, "cg_weaponBar", "1", "0", "4", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078248
CG_CVAR( cg_weaponColor_grenade, "cg_weaponColor_grenade", "0x007000FF", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078260
CG_CVAR( cg_weaponConfig, "cg_weaponConfig", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078278
CG_CVAR( cg_weaponConfig_g, "cg_weaponConfig_g", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078290
CG_CVAR( cg_weaponConfig_mg, "cg_weaponConfig_mg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100782a8
CG_CVAR( cg_weaponConfig_sg, "cg_weaponConfig_sg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100782c0
CG_CVAR( cg_weaponConfig_gl, "cg_weaponConfig_gl", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100782d8
CG_CVAR( cg_weaponConfig_rl, "cg_weaponConfig_rl", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100782f0
CG_CVAR( cg_weaponConfig_lg, "cg_weaponConfig_lg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078308
CG_CVAR( cg_weaponConfig_rg, "cg_weaponConfig_rg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078320
CG_CVAR( cg_weaponConfig_pg, "cg_weaponConfig_pg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078338
CG_CVAR( cg_weaponConfig_bfg, "cg_weaponConfig_bfg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078350
CG_CVAR( cg_weaponConfig_gh, "cg_weaponConfig_gh", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078368
CG_CVAR( cg_weaponConfig_ng, "cg_weaponConfig_ng", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078380
CG_CVAR( cg_weaponConfig_pl, "cg_weaponConfig_pl", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 10078398
CG_CVAR( cg_weaponConfig_cg, "cg_weaponConfig_cg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100783b0
CG_CVAR( cg_weaponConfig_hmg, "cg_weaponConfig_hmg", "", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100783c8
CG_CVAR( cg_weaponPrimary, "cg_weaponPrimary", "", CVAR_ROM, NULL ) // 100783e0
CG_CVAR( cg_weaponPrimaryQueued, "cg_weaponPrimaryQueued", "", CVAR_TEMP, NULL ) // 100783f8
CG_CVAR_RANGE( cg_zoomFov, "cg_zoomfov", "30", "10", "130", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078410
CG_CVAR_RANGE( cg_zoomOutOnDeath, "cg_zoomOutOnDeath", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078428
CG_CVAR_RANGE( cg_zoomScaling, "cg_zoomScaling", "1", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078440
CG_CVAR_RANGE( cg_zoomSensitivity, "cg_zoomSensitivity", "1", "0", "10", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078458
CG_CVAR_RANGE( cg_zoomToggle, "cg_zoomToggle", "0", "0", "1", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078470
CG_CVAR_RANGE( com_maxfps, "com_maxfps", "125", "30", "250", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 10078488
CG_CVAR( s_ambient, "s_ambient", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_PROTECTED | CVAR_CLOUD, NULL ) // 100784a0
CG_CVAR_RANGE( cg_killBeepVolume, "s_killBeepVolume", "1", "0", "4", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100784b8
CG_CVAR_RANGE( cg_announcerVolume, "s_announcerVolume", "1", "0", "2", CVAR_ARCHIVE | CVAR_PROTECTED | CVAR_VM_CREATED | CVAR_CLOUD ) // 100784d0
CG_CVAR( sv_fps, "sv_fps", "40", CVAR_ROM, NULL ) // 100784e8
CG_CVAR( ui_endMapVotingDisabled, "ui_endMapVotingDisabled", "0", CVAR_ROM, NULL ) // 10078500
CG_CVAR( ui_gameTypeVotingDisabled, "ui_gameTypeVotingDisabled", "0", CVAR_ROM, NULL ) // 10078518
CG_CVAR( ui_intermission, "ui_intermission", "0", CVAR_ROM, NULL ) // 10078530
CG_CVAR( ui_mainmenu, "ui_mainmenu", "1", CVAR_ROM, NULL ) // 10078548
CG_CVAR( ui_mapVotingDisabled, "ui_mapVotingDisabled", "0", CVAR_ROM, NULL ) // 10078560
CG_CVAR( ui_voteactive, "ui_voteactive", "0", CVAR_ROM, NULL ) // 10078578
CG_CVAR( ui_warmup, "ui_warmup", "0", CVAR_ROM, NULL ) // 10078590

// FnQL-game locals that retail's cgame does not register.  Inherited code
// still reads them.  They are local settings only: none may carry
// CVAR_USERINFO or CVAR_SERVERINFO, because a retail server would then see a
// key no retail client sends (G2 holds this list to an allowlist).
CG_CVAR( cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawWeaponSelect, "cg_drawWeaponSelect", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_railTrailRadius, "cg_railTrailRadius", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_gun_frame, "cg_gun_frame", "", CVAR_ROM, NULL )
CG_CVAR( cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_buildScript, "com_buildScript", "0", 0, NULL )
CG_CVAR( cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE, NULL )
CG_CVAR( cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enableBreath, "g_enableBreath", "0", 0, NULL )
CG_CVAR( cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enemyModel, "cg_enemyModel", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enemyColors, "cg_enemyColors", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_teamColors, "cg_teamColors", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_fovAdjust, "cg_fovAdjust", "0", CVAR_ARCHIVE, NULL )

#undef CG_CVAR
#undef CG_CVAR_RANGE
