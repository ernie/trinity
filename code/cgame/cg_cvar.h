#ifdef EXTERN_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) extern vmCvar_t vmCvar;
	#define CG_CVAR_PLATFORM( vmCvar, platformMask, defaultString )
#endif

#ifdef DECLARE_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) vmCvar_t vmCvar;
	#define CG_CVAR_PLATFORM( vmCvar, platformMask, defaultString )
#endif

#ifdef CG_CVAR_LIST
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) { & vmCvar, cvarName, defaultString, cvarFlags },
	#define CG_CVAR_PLATFORM( vmCvar, platformMask, defaultString )
#endif

#ifdef CG_CVAR_PLATFORM_LIST
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags )
	#define CG_CVAR_PLATFORM( vmCvar, platformMask, defaultString ) { & vmCvar, platformMask, defaultString },
#endif

// Several stock defaults are deliberately different here, on every platform
// (not VR-gated): cg_drawAttacker 0, cg_drawCrosshairNames 0, cg_scorePlums 0,
// cg_smoothClients 1, cg_oldRocket 0, cg_trueLightning 1.

CG_CVAR( cg_ignore, "cg_ignore", "0", 0 ) // used for debugging
CG_CVAR( cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE )
CG_CVAR( cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE )
CG_CVAR( cg_fov, "cg_fov", "90", CVAR_ARCHIVE )
CG_CVAR( cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE )
CG_CVAR( cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE )
CG_CVAR( cg_playerShadow, "cg_playerShadow", "1", CVAR_ARCHIVE )
CG_CVAR( cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE )
CG_CVAR( cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawViewers, "cg_drawViewers", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE )
CG_CVAR( cg_weaponbob, "cg_weaponbob", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawAttacker, "cg_drawAttacker", "0", CVAR_ARCHIVE_ND )
CG_CVAR( cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE )
CG_CVAR( cg_drawCrosshairNames, "cg_drawCrosshairNames", "0", CVAR_ARCHIVE_ND )
CG_CVAR( cg_fragMessage, "cg_fragMessage", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawWeaponSelect, "cg_drawWeaponSelect", "1", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE_ND )
CG_CVAR( cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE )
CG_CVAR( cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE )
CG_CVAR( cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE )
CG_CVAR( cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE )
CG_CVAR( cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE )
CG_CVAR( cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  )
CG_CVAR( cg_railTrailRadius, "cg_railTrailRadius", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_gun_frame, "cg_gun_frame", "", CVAR_ROM )
CG_CVAR( cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE )
CG_CVAR( cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE )
CG_CVAR( cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE )
CG_CVAR( cg_centertime, "cg_centertime", "3", CVAR_CHEAT )
CG_CVAR( cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE_ND )
CG_CVAR( cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE_ND )
CG_CVAR( cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE_ND )
CG_CVAR( cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE_ND )
CG_CVAR( cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE_ND )
CG_CVAR( cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT )
CG_CVAR( cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT )
CG_CVAR( cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT )
CG_CVAR( cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT )
CG_CVAR( cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT )
CG_CVAR( cg_errorDecay, "cg_errordecay", "100", 0 )
CG_CVAR( cg_nopredict, "cg_nopredict", "0", 0 )
CG_CVAR( cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT )
CG_CVAR( cg_showmiss, "cg_showmiss", "0", 0 )
CG_CVAR( cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT )
CG_CVAR( cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT )
CG_CVAR( cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT )
CG_CVAR( cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT )
CG_CVAR( cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT )
CG_CVAR( cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT )
CG_CVAR( cg_thirdPerson, "cg_thirdPerson", "0", 0 )
CG_CVAR( cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  )
CG_CVAR( cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE )
#ifdef MISSIONPACK
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE )
#else
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE )
#endif
CG_CVAR( cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE )
CG_CVAR( cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO )
CG_CVAR( cg_stats, "cg_stats", "0", 0 )
CG_CVAR( cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawVoipSpeakers, "cg_drawVoipSpeakers", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawFFABackground, "cg_drawFFABackground", "0", CVAR_ARCHIVE )
CG_CVAR( cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE )
#ifdef MISSIONPACK
CG_CVAR( cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE )
CG_CVAR( cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE )
#endif
// the following variables are created in other parts of the system,
// but we also reference them here
CG_CVAR( cg_buildScript, "com_buildScript", "0", 0 )	// force loading of all possible data amd error on failures
CG_CVAR( cg_paused, "cl_paused", "0", CVAR_ROM )
CG_CVAR( cg_blood, "com_blood", "2", CVAR_ARCHIVE_ND )	// 0 off, 1 classic, 2 enhanced
CG_CVAR( cg_bloodGibSpray, "cg_bloodGibSpray", "1.0", CVAR_ARCHIVE_ND )	// 0-1 gib-spray gout density
CG_CVAR_PLATFORM( cg_bloodGibSpray, VRPM_QUEST, "0.2" )
CG_CVAR( cg_bloodGoutScale, "cg_bloodGoutScale", "1.0", CVAR_ARCHIVE_ND )	// 0-1 gout sprite radius scale
CG_CVAR( cg_bloodNearCull, "cg_bloodNearCull", "0.5", CVAR_ARCHIVE_ND )	// cull a gout once it covers > this fraction of screen height (0=off)
CG_CVAR_PLATFORM( cg_bloodNearCull, VRPM_QUEST, "0.3" )
CG_CVAR( cg_bloodBleedGouts, "cg_bloodBleedGouts", "8", CVAR_ARCHIVE_ND )	// max gouts per hit
CG_CVAR( cg_bloodBleedDecals, "cg_bloodBleedDecals", "8", CVAR_ARCHIVE_ND )	// max decals per hit
CG_CVAR( cg_bloodTrailStep, "cg_bloodTrailStep", "10", CVAR_ARCHIVE_ND )	// units between trail gouts
CG_CVAR_PLATFORM( cg_bloodTrailStep, VRPM_QUEST, "30" )
CG_CVAR( cg_bloodTrailDecalStep, "cg_bloodTrailDecalStep", "3", CVAR_ARCHIVE_ND )	// trail decal every Nth gout; 0=none
CG_CVAR( cg_bloodDecals, "cg_bloodDecals", "1", CVAR_ARCHIVE_ND )	// master gate for projected blood decals
CG_CVAR( cg_bloodDecalScale, "cg_bloodDecalScale", "1.0", CVAR_ARCHIVE_ND )	// painted decal size multiplier
CG_CVAR_PLATFORM( cg_bloodDecalScale, VRPM_QUEST, "0.75" )
CG_CVAR( cg_bloodDecalReach, "cg_bloodDecalReach", "1.0", CVAR_ARCHIVE_ND )	// surface-reach multiplier (decoupled from size)
CG_CVAR( cg_bloodDecalTime, "cg_bloodDecalTime", "8000", CVAR_ARCHIVE_ND )	// decal lifetime (ms)
CG_CVAR_PLATFORM( cg_bloodDecalTime, VRPM_QUEST, "4000" )
#ifdef MISSIONPACK
CG_CVAR( cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE )
CG_CVAR( cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE )
CG_CVAR( cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO )
CG_CVAR( cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO )
CG_CVAR( cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO )
CG_CVAR( cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO )
CG_CVAR( cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE )
CG_CVAR( cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE )
CG_CVAR( cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO )
CG_CVAR( cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE )
#endif
CG_CVAR( cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT )
CG_CVAR( cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE )
CG_CVAR( cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0 )
CG_CVAR( cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0 )
CG_CVAR( cg_timescale, "timescale", "1", 0 )
CG_CVAR( cg_scorePlum, "cg_scorePlums", "0", CVAR_USERINFO | CVAR_ARCHIVE_ND )
CG_CVAR( cg_damagePlums, "cg_damagePlums", "0", CVAR_USERINFO | CVAR_ARCHIVE )
CG_CVAR( cg_damagePlumScale, "cg_damagePlumScale", "1.0", CVAR_ARCHIVE_ND )
CG_CVAR( cg_smoothClients, "cg_smoothClients", "1", CVAR_USERINFO | CVAR_ARCHIVE_ND )
CG_CVAR( cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT )
CG_CVAR( cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE )
CG_CVAR( cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE )
CG_CVAR( cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE )
CG_CVAR( cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE )
CG_CVAR( cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE )
CG_CVAR( cg_oldRocket, "cg_oldRocket", "0", CVAR_ARCHIVE_ND )
CG_CVAR( cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE )
CG_CVAR( cg_trueLightning, "cg_trueLightning", "1", CVAR_ARCHIVE_ND )
CG_CVAR( cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE )
CG_CVAR( cg_enemyModel, "cg_enemyModel", "", CVAR_ARCHIVE )
CG_CVAR( cg_enemyColors, "cg_enemyColors", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamColors, "cg_teamColors", "", CVAR_ARCHIVE )
CG_CVAR( cg_deadBodyDarken, "cg_deadBodyDarken", "1", CVAR_ARCHIVE )
CG_CVAR( cg_fovAdjust, "cg_fovAdjust", "0", CVAR_ARCHIVE )
CG_CVAR( cg_followKiller, "cg_followKiller", "0", CVAR_ARCHIVE )
CG_CVAR( cg_damageEffect, "cg_damageEffect", "0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairColor, "cg_crosshairColor", "0", CVAR_ARCHIVE_ND )
CG_CVAR( cg_followMode, "cg_followMode", "0", CVAR_ARCHIVE )
CG_CVAR( cg_tvTimeline, "cg_tvTimeline", "1", CVAR_ARCHIVE )
CG_CVAR( cg_tvTime, "cl_tvTime", "0", CVAR_ROM )
CG_CVAR( cg_tvDuration, "cl_tvDuration", "0", CVAR_ROM )
CG_CVAR( cg_tvSkip, "cg_tvSkip", "10", CVAR_ARCHIVE )
CG_CVAR( cg_downloadName,  "cl_downloadName",  "", CVAR_ROM )
CG_CVAR( cg_downloadSize,  "cl_downloadSize",  "0", CVAR_ROM )
CG_CVAR( cg_downloadCount, "cl_downloadCount", "0", CVAR_ROM )
CG_CVAR( cg_downloadTime,  "cl_downloadTime",  "0", CVAR_ROM )
CG_CVAR( cg_tvdTimeout,    "cg_tvdTimeout",    "10", CVAR_ARCHIVE )
CG_CVAR( cg_tvdOffer,      "cl_tvdOffer",      "", CVAR_ROM )
CG_CVAR( cg_voteYesKey,    "cl_voteYesKey",    "", CVAR_ROM )
CG_CVAR( cg_voteNoKey,     "cl_voteNoKey",     "", CVAR_ROM )
CG_CVAR( cg_voipUseVAD,    "cl_voipUseVAD",    "0", CVAR_ARCHIVE )
CG_CVAR( cg_voipLevel,  "cl_voipLevel",  "0", 0 )
CG_CVAR( cg_voipLevels, "cl_voipLevels", "",  0 )
CG_CVAR( cg_voipVADMuted,  "cl_voipVADMuted",  "0", CVAR_ARCHIVE )

CG_CVAR( cg_trueShotgun, "cg_trueShotgun", "0", CVAR_ARCHIVE )
CG_CVAR( cg_trinityAnnounce, "cg_trinityAnnounce", "1", CVAR_ARCHIVE )

#undef CG_CVAR
#undef CG_CVAR_PLATFORM
