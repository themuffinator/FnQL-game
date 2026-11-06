#ifdef EXTERN_UI_CVAR
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) extern vmCvar_t vmCvar;
	#define UI_CVAR_CB( vmCvar, cvarName, defaultString, cvarFlags, onChange ) extern vmCvar_t vmCvar;
#endif

#ifdef DECLARE_UI_CVAR
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) vmCvar_t vmCvar;
	#define UI_CVAR_CB( vmCvar, cvarName, defaultString, cvarFlags, onChange ) vmCvar_t vmCvar;
#endif

#ifdef UI_CVAR_LIST
	#define UI_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) { & vmCvar, cvarName, defaultString, cvarFlags, NULL },
	#define UI_CVAR_CB( vmCvar, cvarName, defaultString, cvarFlags, onChange ) { & vmCvar, cvarName, defaultString, cvarFlags, onChange },
#endif

UI_CVAR( ui_serverFilterType, "ui_serverFilterType", "0", CVAR_ARCHIVE )
UI_CVAR( ui_cvGameType, "ui_cvGameType", "-1", 0 )

UI_CVAR( ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE )

UI_CVAR( ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE )

UI_CVAR( ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE )

UI_CVAR( ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM )
UI_CVAR( ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE )

UI_CVAR( ui_spSelection, "ui_spSelection", "", CVAR_ROM )

UI_CVAR( ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE )
UI_CVAR( ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE )
UI_CVAR( ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE )

UI_CVAR( ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE )
UI_CVAR( ui_drawCrosshair, "cg_drawCrosshair", "2", CVAR_ARCHIVE )
UI_CVAR( ui_marks, "cg_marks", "1", CVAR_ARCHIVE )

UI_CVAR( ui_server1, "server1", "", CVAR_ARCHIVE )
UI_CVAR( ui_server2, "server2", "", CVAR_ARCHIVE )
UI_CVAR( ui_server3, "server3", "", CVAR_ARCHIVE )
UI_CVAR( ui_server4, "server4", "", CVAR_ARCHIVE )
UI_CVAR( ui_server5, "server5", "", CVAR_ARCHIVE )
UI_CVAR( ui_server6, "server6", "", CVAR_ARCHIVE )
UI_CVAR( ui_server7, "server7", "", CVAR_ARCHIVE )
UI_CVAR( ui_server8, "server8", "", CVAR_ARCHIVE )
UI_CVAR( ui_server9, "server9", "", CVAR_ARCHIVE )
UI_CVAR( ui_server10, "server10", "", CVAR_ARCHIVE )
UI_CVAR( ui_server11, "server11", "", CVAR_ARCHIVE )
UI_CVAR( ui_server12, "server12", "", CVAR_ARCHIVE )
UI_CVAR( ui_server13, "server13", "", CVAR_ARCHIVE )
UI_CVAR( ui_server14, "server14", "", CVAR_ARCHIVE )
UI_CVAR( ui_server15, "server15", "", CVAR_ARCHIVE )
UI_CVAR( ui_server16, "server16", "", CVAR_ARCHIVE )
UI_CVAR( ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM )
UI_CVAR( ui_new, "ui_new", "0", CVAR_TEMP )
UI_CVAR( ui_debug, "ui_debug", "0", CVAR_TEMP )
UI_CVAR( ui_initialized, "ui_initialized", "0", CVAR_TEMP )
UI_CVAR( ui_teamName, "ui_teamName", "Pagans", CVAR_ARCHIVE )
UI_CVAR( ui_opponentName, "ui_opponentName", "Stroggs", CVAR_ARCHIVE )
UI_CVAR( ui_redteam, "ui_redteam", "Pagans", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam, "ui_blueteam", "Stroggs", CVAR_ARCHIVE )
UI_CVAR( ui_dedicated, "ui_dedicated", "0", CVAR_ARCHIVE )
UI_CVAR( ui_gameType, "ui_gametype", "3", CVAR_ARCHIVE )
UI_CVAR( ui_joinGameType, "ui_joinGametype", "0", CVAR_ARCHIVE )
UI_CVAR( ui_netGameType, "ui_netGametype", "3", CVAR_ARCHIVE )
UI_CVAR( ui_actualNetGameType, "ui_actualNetGametype", "3", CVAR_ARCHIVE )
UI_CVAR( ui_redteam1, "ui_redteam1", "0", CVAR_ARCHIVE )
UI_CVAR( ui_redteam2, "ui_redteam2", "0", CVAR_ARCHIVE )
UI_CVAR( ui_redteam3, "ui_redteam3", "0", CVAR_ARCHIVE )
UI_CVAR( ui_redteam4, "ui_redteam4", "0", CVAR_ARCHIVE )
UI_CVAR( ui_redteam5, "ui_redteam5", "0", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam1, "ui_blueteam1", "0", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam2, "ui_blueteam2", "0", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam3, "ui_blueteam3", "0", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam4, "ui_blueteam4", "0", CVAR_ARCHIVE )
UI_CVAR( ui_blueteam5, "ui_blueteam5", "0", CVAR_ARCHIVE )
UI_CVAR( ui_netSource, "ui_netSource", "0", CVAR_ARCHIVE )
UI_CVAR( ui_menuFiles, "ui_menuFiles", "ui/menus.txt", CVAR_ARCHIVE )
UI_CVAR( ui_currentTier, "ui_currentTier", "0", CVAR_ARCHIVE )
UI_CVAR( ui_currentMap, "ui_currentMap", "0", CVAR_ARCHIVE )
UI_CVAR( ui_currentNetMap, "ui_currentNetMap", "0", CVAR_ARCHIVE )
UI_CVAR( ui_mapIndex, "ui_mapIndex", "0", CVAR_ARCHIVE )
UI_CVAR( ui_currentOpponent, "ui_currentOpponent", "0", CVAR_ARCHIVE )
UI_CVAR( ui_selectedPlayer, "cg_selectedPlayer", "0", CVAR_ARCHIVE )
UI_CVAR( ui_selectedPlayerName, "cg_selectedPlayerName", "", CVAR_ARCHIVE )
UI_CVAR( ui_lastServerRefresh_0, "ui_lastServerRefresh_0", "", CVAR_ARCHIVE )
UI_CVAR( ui_lastServerRefresh_1, "ui_lastServerRefresh_1", "", CVAR_ARCHIVE )
UI_CVAR( ui_lastServerRefresh_2, "ui_lastServerRefresh_2", "", CVAR_ARCHIVE )
UI_CVAR( ui_lastServerRefresh_3, "ui_lastServerRefresh_3", "", CVAR_ARCHIVE )
UI_CVAR( ui_singlePlayerActive, "ui_singlePlayerActive", "0", 0 )
UI_CVAR( ui_scoreAccuracy, "ui_scoreAccuracy", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreImpressives, "ui_scoreImpressives", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreExcellents, "ui_scoreExcellents", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreCaptures, "ui_scoreCaptures", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreDefends, "ui_scoreDefends", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreAssists, "ui_scoreAssists", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreGauntlets, "ui_scoreGauntlets", "0",CVAR_ARCHIVE )
UI_CVAR( ui_scoreScore, "ui_scoreScore", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scorePerfect, "ui_scorePerfect", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreTeam, "ui_scoreTeam", "0 to 0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreBase, "ui_scoreBase", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreTime, "ui_scoreTime", "00:00", CVAR_ARCHIVE )
UI_CVAR( ui_scoreTimeBonus, "ui_scoreTimeBonus", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreSkillBonus, "ui_scoreSkillBonus", "0", CVAR_ARCHIVE )
UI_CVAR( ui_scoreShutoutBonus, "ui_scoreShutoutBonus", "0", CVAR_ARCHIVE )
UI_CVAR( ui_fragLimit, "ui_fragLimit", "10", 0 )
UI_CVAR( ui_captureLimit, "ui_captureLimit", "5", 0 )
UI_CVAR( ui_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE | UI_CVAR_PROTECTED )
UI_CVAR( ui_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE | UI_CVAR_PROTECTED )
UI_CVAR( ui_findPlayer, "ui_findPlayer", "Sarge", CVAR_ARCHIVE )
UI_CVAR( ui_Q3Model, "ui_q3model", "0", CVAR_ARCHIVE )
UI_CVAR( ui_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE )
UI_CVAR( ui_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE )
UI_CVAR( ui_realWarmUp, "g_warmup", "10", CVAR_ARCHIVE )
UI_CVAR( ui_realCaptureLimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART )
UI_CVAR( ui_serverStatusTimeOut, "ui_serverStatusTimeOut", "7000", CVAR_ARCHIVE )

/* Retail cvar table (uix86.dll 0x1002afd8, 130 rows of {cvar, name,
 * default, change callback, flags}; walked by 0x100118a0). The rows below
 * are the retail entries the baseline lacked, with retail defaults and flags.
 * Callbacks run only when a later update changes the modification count. */
UI_CVAR_CB( ui_cgAnnouncer, "cg_announcer", "1", CVAR_TEMP, UI_QLAnnouncerChanged )
UI_CVAR( ui_bloomPreset, "ui_bloomPreset", "Default", CVAR_ARCHIVE )
UI_CVAR( ui_enemyCrosshairNames, "cg_enemyCrosshairNames", "1", CVAR_ARCHIVE )
UI_CVAR_CB( ui_teamColor, "ui_teamColor", "0", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_teamHeadColor, "ui_teamHeadColor", "96", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_teamUpperColor, "ui_teamUpperColor", "96", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_teamLowerColor, "ui_teamLowerColor", "96", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_enemyColor, "ui_enemyColor", "0", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_enemyHeadColor, "ui_enemyHeadColor", "27", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_enemyUpperColor, "ui_enemyUpperColor", "27", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_enemyLowerColor, "ui_enemyLowerColor", "27", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_screenDamage, "ui_screenDamage", "0", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_screenDamageTeam, "ui_screenDamage_Team", "0", CVAR_ARCHIVE, UI_QLSliderColorChanged )
UI_CVAR_CB( ui_forceTeamModel, "ui_forceTeamModel", "", CVAR_TEMP, UI_QLTeamModelChanged )
UI_CVAR_CB( ui_forceTeamSkin, "ui_forceTeamSkin", "", CVAR_TEMP, UI_QLTeamModelChanged )
UI_CVAR( ui_forceTeamModelBright, "ui_forceTeamModelBright", "0", CVAR_ROM )
UI_CVAR_CB( ui_forceEnemyModel, "ui_forceEnemyModel", "", CVAR_TEMP, UI_QLEnemyModelChanged )
UI_CVAR_CB( ui_forceEnemySkin, "ui_forceEnemySkin", "", CVAR_TEMP, UI_QLEnemyModelChanged )
UI_CVAR( ui_forceEnemyModelBright, "ui_forceEnemyModelBright", "0", CVAR_ROM )
UI_CVAR( ui_gibs, "ui_gibs", "1", CVAR_ROM )
UI_CVAR( ui_mainmenu, "ui_mainmenu", "1", CVAR_ROM )
UI_CVAR( ui_priv, "ui_priv", "0", CVAR_ROM )

#undef UI_CVAR
#undef UI_CVAR_CB
