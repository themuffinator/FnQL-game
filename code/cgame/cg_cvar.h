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

CG_CVAR( cg_ignore, "cg_ignore", "0", 0, NULL ) // used for debugging
// Retail table 100775e8: read-only HUD selector, refreshed from serverinfo.
CG_CVAR( cg_gametype, "cg_gametype", "0", CVAR_ROM, NULL )
// Shipping cvar record 100771c8; archive the local preference.
CG_CVAR_RANGE( cg_drawPregameMessages, "cg_drawPregameMessages", "1", "0", "1", CVAR_ARCHIVE )
// Shipping tables 10076b08, 10077ee8 and 10077f00.
CG_CVAR_RANGE( cg_autoswitch, "cg_autoswitch", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_switchOnEmpty, "cg_switchOnEmpty", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_switchToEmpty, "cg_switchToEmpty", "1", "0", "1", CVAR_ARCHIVE )
// Shipping tables 10077918 and 10078278-100783f8.
CG_CVAR( cg_preferredStartingWeapons, "cg_preferredStartingWeapons", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponPrimary, "cg_weaponPrimary", "", CVAR_ROM, NULL )
CG_CVAR( cg_weaponPrimaryQueued, "cg_weaponPrimaryQueued", "", CVAR_TEMP, NULL )
CG_CVAR( cg_weaponConfig, "cg_weaponConfig", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_g, "cg_weaponConfig_g", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_mg, "cg_weaponConfig_mg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_sg, "cg_weaponConfig_sg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_gl, "cg_weaponConfig_gl", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_rl, "cg_weaponConfig_rl", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_lg, "cg_weaponConfig_lg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_rg, "cg_weaponConfig_rg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_pg, "cg_weaponConfig_pg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_bfg, "cg_weaponConfig_bfg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_gh, "cg_weaponConfig_gh", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_ng, "cg_weaponConfig_ng", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_pl, "cg_weaponConfig_pl", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_cg, "cg_weaponConfig_cg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_weaponConfig_hmg, "cg_weaponConfig_hmg", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE, NULL )
// Retail cgame table 10077540, 10077d98, 10078230, 10078410-10078470.
CG_CVAR_RANGE( cg_zoomFov, "cg_zoomfov", "30", "10", "130", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_fov, "cg_fov", "100", "10", "130", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specFov, "cg_specFov", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_zoomScaling, "cg_zoomScaling", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_zoomSensitivity, "cg_zoomSensitivity", "1", "0", "10", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_zoomToggle, "cg_zoomToggle", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_zoomOutOnDeath, "cg_zoomOutOnDeath", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_waterWarp, "cg_waterWarp", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR( cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE, NULL )
// Shipping table 100777b0: countdown by default.
CG_CVAR_RANGE( cg_levelTimerDirection, "cg_levelTimerDirection", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE, NULL )
// Retail cgame tables 10077018, 10077828 and 10077840.
CG_CVAR_RANGE( cg_drawAmmoWarning, "cg_drawAmmoWarning", "2", "0", "2", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_lowAmmoWarningPercentile, "cg_lowAmmoWarningPercentile", "0.20", "0.01", "1.00", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_lowAmmoWarningSound, "cg_lowAmmoWarningSound", "1", "0", "2", CVAR_ARCHIVE )
CG_CVAR( cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE, NULL )
// Retail 10077048: default "2", range 0..29 (the UI registers the same).
CG_CVAR_RANGE( cg_drawCrosshair, "cg_drawCrosshair", "2", "0", "29", CVAR_ARCHIVE )
// Retail 10077360 / 10077378 / 10077fc0 / 10077fd8 / 10077060 / 10077078
// replace Quake III cg_drawCrosshairNames.
CG_CVAR_RANGE( cg_enemyCrosshairNames, "cg_enemyCrosshairNames", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_enemyCrosshairNamesOpacity, "cg_enemyCrosshairNamesOpacity", "0.75", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teammateCrosshairNames, "cg_teammateCrosshairNames", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teammateCrosshairNamesOpacity, "cg_teammateCrosshairNamesOpacity", "0.75", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_drawCrosshairTeamHealth, "cg_drawCrosshairTeamHealth", "2", "0", "2", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_drawCrosshairTeamHealthSize, "cg_drawCrosshairTeamHealthSize", "0.12", "0.10", "0.26", CVAR_ARCHIVE )
CG_CVAR( cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_announcer, "cg_announcer", "1", CVAR_ARCHIVE, NULL )
// Shipping table 100784d0: timeout/countdown voices use native import 40.
CG_CVAR_RANGE( cg_announcerVolume, "s_announcerVolume", "1", "0", "2", CVAR_ARCHIVE )
CG_CVAR( cg_announcerLastStandingVO, "cg_announcerLastStandingVO", "1", CVAR_ARCHIVE, NULL )
// Retail 10076a90: default "1", range 0..1; gates the domination capture voices.
CG_CVAR( cg_announcerLeadsVO, "cg_announcerLeadsVO", "1", CVAR_ARCHIVE, NULL )
// Retail 10076ac0: default "1", range 0..1; gates "teams tied" (global team sound 12).
CG_CVAR( cg_announcerTiesVO, "cg_announcerTiesVO", "1", CVAR_ARCHIVE, NULL )
// Retail 10077438: default "0", range 0..2; free spectators follow flag and
// powerup takers (0x10019d20), 2 asks the server to follow powerup carriers.
CG_CVAR( cg_followPowerup, "cg_followPowerup", "0", CVAR_ARCHIVE, NULL )
// Retail 10077648 / 10077750 / 100784b8: hit and kill beeps.
CG_CVAR_RANGE( cg_hitBeep, "cg_hitBeep", "2", "0", "3", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_killBeep, "cg_killBeep", "7", "0", "8", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_killBeepVolume, "s_killBeepVolume", "1", "0", "4", CVAR_ARCHIVE )
CG_CVAR( cg_raceBeep, "cg_raceBeep", "7", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawCheckpointRemaining, "cg_drawCheckpointRemaining", "1", CVAR_ARCHIVE, NULL )
// Retail 10077708 / 10077990 / 100779a8; consumed by race points and EV_POI pings.
CG_CVAR( cg_itemFx, "cg_itemFx", "7", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_poiMaxWidth, "cg_poiMaxWidth", "32.0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_poiMinWidth, "cg_poiMinWidth", "16.0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_buzzerSound, "cg_buzzerSound", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawSpriteSelf, "cg_drawSpriteSelf", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_drawWeaponSelect, "cg_drawWeaponSelect", "1", CVAR_ARCHIVE, NULL )
// Retail tables 10078248, 100770f0 and 10077858.
CG_CVAR_RANGE( cg_weaponBar, "cg_weaponBar", "1", "0", "4", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_drawFullWeaponBar, "cg_drawFullWeaponBar", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_lowAmmoWeaponBarWarning, "cg_lowAmmoWeaponBarWarning", "2", "0", "2", CVAR_ARCHIVE )
// Retail records 10076cb8-10076d90: the Quake Live crosshair (cg_crosshair.c).
CG_CVAR_RANGE( cg_crosshairSize, "cg_crosshairSize", "32", "0", "320", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairHealth, "cg_crosshairHealth", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairX, "cg_crosshairX", "0", "-320", "320", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairY, "cg_crosshairY", "0", "-240", "240", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairColor, "cg_crosshairColor", "25", "1", "26", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairBrightness, "cg_crosshairBrightness", "1.0", "0.0", "1.0", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairPulse, "cg_crosshairPulse", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairHitColor, "cg_crosshairHitColor", "1", "1", "26", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairHitStyle, "cg_crosshairHitStyle", "1", "0", "8", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_crosshairHitTime, "cg_crosshairHitTime", "200.0", "0.0", "1000.0", CVAR_ARCHIVE )
CG_CVAR( cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE, NULL )
// Retail record 10077768: off by default, 0..2.
CG_CVAR_RANGE( cg_lagometer, "cg_lagometer", "0", "0", "2", CVAR_ARCHIVE )
CG_CVAR( cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_railTrailRadius, "cg_railTrailRadius", "0", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_gun_frame, "cg_gun_frame", "", CVAR_ROM, NULL )
CG_CVAR( cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_centertime, "cg_centertime", "3", CVAR_CHEAT, NULL )
CG_CVAR( cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT, NULL )
CG_CVAR( cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT, NULL )
CG_CVAR( cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_errorDecay, "cg_errordecay", "100", 0, NULL )
CG_CVAR( cg_nopredict, "cg_nopredict", "0", 0, NULL )
CG_CVAR( cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_showmiss, "cg_showmiss", "0", 0, NULL )
CG_CVAR( cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT, NULL )
CG_CVAR( cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT, NULL )
CG_CVAR( cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT, NULL )
CG_CVAR( cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT, NULL )
CG_CVAR( cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT, NULL )
CG_CVAR( cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_thirdPerson, "cg_thirdPerson", "0", 0, NULL )
CG_CVAR( cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE , NULL )
CG_CVAR( cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE, NULL )
#ifdef MISSIONPACK
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE, NULL )
#else
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE, NULL )
#endif
CG_CVAR( cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO, NULL )
CG_CVAR( cg_stats, "cg_stats", "0", 0, NULL )
// Retail 1007800c / 10078024 / 1007803c / 10077ff4 / 10077124 / 10077274 replace
// Quake III cg_drawFriend: teammate markers and the sprite master switch.
CG_CVAR_RANGE( cg_teammatePOIs, "cg_teammatePOIs", "1", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teammatePOIsMaxWidth, "cg_teammatePOIsMaxWidth", "24.0", "16.0", "32.0", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teammatePOIsMinWidth, "cg_teammatePOIsMinWidth", "4.0", "2.0", "16.0", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teammateNames, "cg_teammateNames", "1", "0", "2", CVAR_ARCHIVE )
// Retail 10077e10 and 100778e8: spectator overhead names and the name fit.
CG_CVAR_RANGE( cg_specNames, "cg_specNames", "2", "0", "2", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_overheadNamesWidth, "cg_overheadNamesWidth", "75", "50", "100", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_drawHitFriendTime, "cg_drawHitFriendTime", "100", "0", "5000", CVAR_ARCHIVE )
CG_CVAR( cg_drawSprites, "cg_drawSprites", "1", CVAR_ARCHIVE, NULL )
// Retail 100779c0: flag-carrier objective marker.
CG_CVAR_RANGE( cg_flagPOIs, "cg_flagPOIs", "1", "0", "1", CVAR_ARCHIVE )
// Retail 10077090: dead-teammate marker lifetime in ms.
CG_CVAR_RANGE( cg_drawDeadFriendTime, "cg_drawDeadFriendTime", "3000.0", "0.0", "5000.0", CVAR_ARCHIVE )
// Retail 100777f8: lightning beam style, 1..5.
CG_CVAR_RANGE( cg_lightningStyle, "cg_lightningStyle", "1", "1", "5", CVAR_ARCHIVE )
CG_CVAR( cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE, NULL )
// Retail 10077810: loadout state for the CG_SHOW_IF_LOADOUT_* menu tests.
CG_CVAR( cg_loadout, "cg_loadout", "0", CVAR_ROM, NULL )
// Retail 10077d68/10077d80: spectator duel health/armor bars and their tint.
CG_CVAR_RANGE( cg_specDuelHealthArmor, "cg_specDuelHealthArmor", "1", "0", "1", CVAR_ARCHIVE )
// Retail record 100770a8: demo playback controls (cg_demohud.c).
CG_CVAR_RANGE( cg_drawDemoHUD, "cg_drawDemoHUD", "1", "0", "1", CVAR_ARCHIVE )
// Retail record 10077258: CG_SPEC_MESSAGES (cg_spectatorhud.c).
CG_CVAR_RANGE( cg_drawSpecMessages, "cg_drawSpecMessages", "1", "0", "1", CVAR_ARCHIVE )
// Retail records 10077db0-10077df8: spectator item timers (cg_itemtimers.c).
CG_CVAR_RANGE( cg_specItemTimers, "cg_specItemTimers", "7", "0", "15", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specItemTimersSize, "cg_specItemTimersSize", "0.24", "0.12", "0.35", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specItemTimersX, "cg_specItemTimersX", "10", "0", "640", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specItemTimersY, "cg_specItemTimersY", "200", "0", "480", CVAR_ARCHIVE )
// Retail records 10077e28-10077e70: spectator team vitals (cg_teamvitals.c).
CG_CVAR_RANGE( cg_specTeamVitals, "cg_specTeamVitals", "1", "0", "2", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specTeamVitalsHealthColor, "cg_specTeamVitalsHealthColor", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specTeamVitalsWidth, "cg_specTeamVitalsWidth", "100", "60", "120", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specTeamVitalsY, "cg_specTeamVitalsY", "85", "0", "480", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_specDuelHealthColor, "cg_specDuelHealthColor", "0", "0", "1", CVAR_ARCHIVE )
// Retail 10076c70, 10077f18, 10076c88: chat beeps and the chat-area history.
CG_CVAR_RANGE( cg_chatbeep, "cg_chatbeep", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_teamChatBeep, "cg_teamChatBeep", "0", "0", "1", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_chatHistoryLength, "cg_chatHistoryLength", "6", "0", "24", CVAR_ARCHIVE )
#ifdef MISSIONPACK
CG_CVAR( cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE, NULL )
#endif
// the following variables are created in other parts of the system,
// but we also reference them here
CG_CVAR( cg_buildScript, "com_buildScript", "0", 0, NULL )	// force loading of all possible data amd error on failures
CG_CVAR( cg_paused, "cl_paused", "0", CVAR_ROM, NULL )
CG_CVAR( cg_blood, "com_blood", "1", CVAR_ARCHIVE, NULL )
#ifdef MISSIONPACK
CG_CVAR( cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO, NULL )
CG_CVAR( cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO, NULL )
// Retail HUD-only overrides, distinct from inherited server team settings.
CG_CVAR( cg_scoreRedTeamName, "cg_redTeamName", "", 0, NULL )
CG_CVAR( cg_scoreBlueTeamName, "cg_blueTeamName", "", 0, NULL )
CG_CVAR( cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO, NULL )
CG_CVAR( cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO, NULL )
CG_CVAR( cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO, NULL )
CG_CVAR( cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO, NULL )
CG_CVAR( cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO, NULL )
CG_CVAR( cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE, NULL )
#endif
CG_CVAR( cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0, NULL )
CG_CVAR( cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0, NULL )
CG_CVAR( cg_timescale, "timescale", "1", 0, NULL )
CG_CVAR( cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE, NULL )
CG_CVAR( cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE, NULL )
CG_CVAR( cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT, NULL )
CG_CVAR( cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enemyModel, "cg_enemyModel", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_enemyColors, "cg_enemyColors", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_teamColors, "cg_teamColors", "", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_deadBodyDarken, "cg_deadBodyDarken", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_fovAdjust, "cg_fovAdjust", "0", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_followKiller, "cg_followKiller", "0", CVAR_ARCHIVE, NULL )
// Retail 100778d0 and 100770d8: kill-feed rows and the killer center print.
CG_CVAR_RANGE( cg_obituaryRowSize, "cg_obituaryRowSize", "5", "0", "5", CVAR_ARCHIVE )
CG_CVAR_RANGE( cg_drawFragMessages, "cg_drawFragMessages", "1", "0", "1", CVAR_ARCHIVE )

CG_CVAR( cg_damagePlum, "cg_damagePlum", "g mg sg gl rl lg rg pg bfg gh cg ng pl hmg", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_damagePlumColorStyle, "cg_damagePlumColorStyle", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_impactSparks, "cg_impactSparks", "1", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_impactSparksSize, "cg_impactSparksSize", "8", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_impactSparksLifetime, "cg_impactSparksLifetime", "250", CVAR_ARCHIVE, NULL )
CG_CVAR( cg_impactSparksVelocity, "cg_impactSparksVelocity", "128", CVAR_ARCHIVE, NULL )

#undef CG_CVAR
#undef CG_CVAR_RANGE
