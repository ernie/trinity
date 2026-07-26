// bg_mode.c -- game mode tables and labels
// shared between game, cgame, and ui

#include "q_shared.h"
#include "bg_public.h"
#include "bg_mode.h"

static const char *mode_names[MODE_COUNT] = {
	"Quake 3",
	"CPMA",
	"Quake Live",
	"QL Turbo",
};

const char *BG_ModeName( int mode ) {
	if ( mode < MODE_VQ3 || mode >= MODE_COUNT )
		mode = MODE_VQ3;
	return mode_names[mode];
}

// Per-mode tuning tables, column-major: one parameter per line, four mode
// columns. Mode_GetConfig transposes them into modeConfig_t on first use.

// movement physics                                                VQ3     CPM      QL     QLT
static const float	md_groundAccelerate[MODE_COUNT]     = {  10.0f,  15.0f,  10.0f,  10.0f };
static const float	md_friction[MODE_COUNT]             = {   6.0f,   6.0f,   6.0f,   6.0f };
static const int	md_jumpVelocity[MODE_COUNT]         = {    270,    270,    275,    275 };
static const qboolean	md_autoHop[MODE_COUNT]              = { qfalse, qfalse,  qtrue,  qtrue };
static const qboolean	md_rampJump[MODE_COUNT]             = { qfalse,  qtrue, qfalse,  qtrue };
static const int	md_doubleJumpBonus[MODE_COUNT]      = {      0,    100,      0,      0 };
static const float	md_airStopAccel[MODE_COUNT]         = {   1.0f,   2.5f,   1.0f,   2.5f };
static const float	md_airControl[MODE_COUNT]           = {   0.0f, 150.0f,   0.0f, 150.0f };
static const float	md_strafeAccel[MODE_COUNT]          = {   1.0f,  70.0f,   1.0f,  70.0f };
static const float	md_airWishspeedCap[MODE_COUNT]      = { MODE_NO_WISHSPEED_CAP, 30.0f, MODE_NO_WISHSPEED_CAP, 30.0f };

// combat globals                                                  VQ3     CPM      QL     QLT
static const int	md_sgPatternType[MODE_COUNT]        = {      0,      2,      1,      1 };
static const int	md_weaponDropTime[MODE_COUNT]       = {    200,      0,    200,    200 };
static const int	md_weaponRaiseTime[MODE_COUNT]      = {    250,      0,    200,    200 };
static const int	md_noAmmoTime[MODE_COUNT]           = {    500,    100,    500,    500 };
static const int	md_splashZKnockback[MODE_COUNT]     = {     24,     36,     24,     24 };
static const int	md_maxKnockback[MODE_COUNT]         = {    200,    200,    120,    120 };
static const int	md_duelInitialAmmoHalve[MODE_COUNT] = { 0, (1<<WP_GRENADE_LAUNCHER)|(1<<WP_LIGHTNING)|(1<<WP_RAILGUN), 0, 0 };
static const int	md_armorTiered[MODE_COUNT]          = {      0,      1,      0,      0 };
static const float	md_armorProtection[MODE_COUNT]      = {  0.66f,  0.66f,  0.66f,  0.66f };
static const float	md_armorGAProtection[MODE_COUNT]    = {  0.66f,  0.50f,  0.66f,  0.66f };
static const float	md_armorYAProtection[MODE_COUNT]    = {  0.66f,  0.66f,  0.66f,  0.66f };
static const float	md_armorRAProtection[MODE_COUNT]    = {  0.66f,  0.75f,  0.66f,  0.66f };
static const float	md_armorSelfProtection[MODE_COUNT]  = {  0.66f,  0.50f,  0.66f,  0.66f };
static const int	md_armorGAMax[MODE_COUNT]           = {    100,    100,    100,    100 };
static const int	md_armorYAMax[MODE_COUNT]           = {    150,    150,    150,    150 };
static const int	md_armorRAMax[MODE_COUNT]           = {    200,    200,    200,    200 };
static const int	md_armorShardValue[MODE_COUNT]      = {      5,      5,      5,      5 };
static const int	md_armorGAPickupValue[MODE_COUNT]   = {     50,     50,     50,     50 };
static const int	md_armorYAPickupValue[MODE_COUNT]   = {     50,    100,     50,     50 };
static const int	md_armorRAPickupValue[MODE_COUNT]   = {    100,    150,    100,    100 };
static const float	md_battleSuitProtection[MODE_COUNT] = {   0.5f,  0.25f,  0.25f,  0.25f };
static const int	md_spawnHealthBonus[MODE_COUNT]     = {     25,      0,     25,     25 };
static const int	md_respawnArmor[MODE_COUNT]         = {     25,     25,     25,     25 };
static const int	md_respawnHealth[MODE_COUNT]        = {     35,     30,     35,     35 };
static const int	md_respawnAmmo[MODE_COUNT]          = {     40,     30,     40,     40 };
static const int	md_respawnPowerup[MODE_COUNT]       = {    120,     90,    120,    120 };
static const int	md_respawnBattleSuit[MODE_COUNT]    = {    120,    120,    120,    120 };
static const int	md_respawnMegahealth[MODE_COUNT]    = {     35,     35,     35,     35 };
static const int	md_megaStyle[MODE_COUNT]            = {      0,      1,      0,      0 };
static const int	md_startPowerups[MODE_COUNT]        = {      0,      1,      0,      0 };

// ---- per-weapon, per-field (rows = weapon, cols = VQ3 CPM QL QLT) ----
static const int	wp_damage[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {      50,      50,      50,      50 },
	/* MACHINEGUN       */ {       7,       5,       5,       5 },
	/* SHOTGUN          */ {      10,       6,       5,       5 },
	/* GRENADE_LAUNCHER */ {     100,     100,     100,     100 },
	/* ROCKET_LAUNCHER  */ {     100,     100,     100,     100 },
	/* LIGHTNING        */ {       8,      10,       6,       6 },
	/* RAILGUN          */ {     100,      80,      80,      80 },
	/* PLASMAGUN        */ {      20,      18,      20,      20 },
	/* BFG              */ {     100,     100,     100,     100 },
	/* GRAPPLING_HOOK   */ {       0,       0,      10,      10 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {      20,      12,      12,      12 },
	/* PROX_LAUNCHER    */ {       0,       0,       0,       0 },
	/* CHAINGUN         */ {       7,       8,       8,       8 },
#endif
};

static const int	wp_teamDamage[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {      50,      50,      50,      50 },
	/* MACHINEGUN       */ {       5,       5,       5,       5 },
	/* SHOTGUN          */ {      10,       6,       5,       5 },
	/* GRENADE_LAUNCHER */ {     100,     100,     100,     100 },
	/* ROCKET_LAUNCHER  */ {     100,     100,     100,     100 },
	/* LIGHTNING        */ {       8,      10,       6,       6 },
	/* RAILGUN          */ {     100,      80,      80,      80 },
	/* PLASMAGUN        */ {      20,      18,      20,      20 },
	/* BFG              */ {     100,     100,     100,     100 },
	/* GRAPPLING_HOOK   */ {       0,       0,      10,      10 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {      20,      12,      12,      12 },
	/* PROX_LAUNCHER    */ {       0,       0,       0,       0 },
	/* CHAINGUN         */ {       7,       8,       8,       8 },
#endif
};

static const int	wp_splashDamage[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {       0,       0,       0,       0 },
	/* SHOTGUN          */ {       0,       0,       0,       0 },
	/* GRENADE_LAUNCHER */ {     100,     100,     100,     100 },
	/* ROCKET_LAUNCHER  */ {     100,     100,      84,      84 },
	/* LIGHTNING        */ {       0,       0,       0,       0 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {      15,      14,      15,      15 },
	/* BFG              */ {     100,       0,     100,     100 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {       0,       0,       0,       0 },
	/* PROX_LAUNCHER    */ {     100,     100,     100,     100 },
	/* CHAINGUN         */ {       0,       0,       0,       0 },
#endif
};

static const int	wp_splashRadius[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {       0,       0,       0,       0 },
	/* SHOTGUN          */ {       0,       0,       0,       0 },
	/* GRENADE_LAUNCHER */ {     150,     150,     150,     150 },
	/* ROCKET_LAUNCHER  */ {     120,     120,     120,     120 },
	/* LIGHTNING        */ {       0,       0,       0,       0 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {      20,      20,      20,      20 },
	/* BFG              */ {     120,       0,      80,      80 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {       0,       0,       0,       0 },
	/* PROX_LAUNCHER    */ {     150,     150,     150,     150 },
	/* CHAINGUN         */ {       0,       0,       0,       0 },
#endif
};

static const int	wp_speed[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {       0,       0,       0,       0 },
	/* SHOTGUN          */ {       0,       0,       0,       0 },
	/* GRENADE_LAUNCHER */ {     700,     800,     700,     700 },
	/* ROCKET_LAUNCHER  */ {     900,     900,    1000,    1000 },
	/* LIGHTNING        */ {       0,       0,       0,       0 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {    2000,    2000,    2000,    2000 },
	/* BFG              */ {    2000,    2000,    1800,    1800 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {       0,    1000,    1000,    1000 },
	/* PROX_LAUNCHER    */ {     700,     800,     700,     700 },
	/* CHAINGUN         */ {       0,       0,       0,       0 },
#endif
};

static const int	wp_fireTime[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {     400,     400,     400,     400 },
	/* MACHINEGUN       */ {     100,     100,     100,     100 },
	/* SHOTGUN          */ {    1000,    1000,    1000,    1000 },
	/* GRENADE_LAUNCHER */ {     800,     800,     800,     800 },
	/* ROCKET_LAUNCHER  */ {     800,     800,     800,     800 },
	/* LIGHTNING        */ {      50,      66,      50,      50 },
	/* RAILGUN          */ {    1500,    1250,    1500,    1500 },
	/* PLASMAGUN        */ {     100,     100,     100,     100 },
	/* BFG              */ {     200,    1000,     300,     300 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {    1000,    1000,    1000,    1000 },
	/* PROX_LAUNCHER    */ {     800,     800,     800,     800 },
	/* CHAINGUN         */ {      30,      50,      50,      50 },
#endif
};

static const int	wp_count[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {       0,       0,       0,       0 },
	/* SHOTGUN          */ {      11,      16,      20,      20 },
	/* GRENADE_LAUNCHER */ {       0,       0,       0,       0 },
	/* ROCKET_LAUNCHER  */ {       0,       0,       0,       0 },
	/* LIGHTNING        */ {       0,       0,       0,       0 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {       0,       0,       0,       0 },
	/* BFG              */ {       0,       0,       0,       0 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {      15,      10,      10,      10 },
	/* PROX_LAUNCHER    */ {       0,       0,       0,       0 },
	/* CHAINGUN         */ {       0,       0,       0,       0 },
#endif
};

static const int	wp_spread[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {     200,     200,     200,     200 },
	/* SHOTGUN          */ {     700,     750,     750,     750 },
	/* GRENADE_LAUNCHER */ {       0,       0,       0,       0 },
	/* ROCKET_LAUNCHER  */ {       0,       0,       0,       0 },
	/* LIGHTNING        */ {       0,       0,       0,       0 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {       0,       0,       0,       0 },
	/* BFG              */ {       0,       0,       0,       0 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {     500,     400,     400,     400 },
	/* PROX_LAUNCHER    */ {       0,       0,       0,       0 },
	/* CHAINGUN         */ {     600,     600,     600,     600 },
#endif
};

static const int	wp_range[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {       0,       0,       0,       0 },
	/* SHOTGUN          */ {       0,       0,       0,       0 },
	/* GRENADE_LAUNCHER */ {       0,       0,       0,       0 },
	/* ROCKET_LAUNCHER  */ {       0,       0,       0,       0 },
	/* LIGHTNING        */ {     768,     768,     768,     768 },
	/* RAILGUN          */ {       0,       0,       0,       0 },
	/* PLASMAGUN        */ {       0,       0,       0,       0 },
	/* BFG              */ {       0,       0,       0,       0 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {       0,       0,       0,       0 },
	/* PROX_LAUNCHER    */ {       0,       0,       0,       0 },
	/* CHAINGUN         */ {       0,       0,       0,       0 },
#endif
};

static const float	wp_knockback[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {    0.0f,    0.0f,    0.0f,    0.0f },
	/* GAUNTLET         */ {    1.0f,    0.5f,    1.0f,    1.0f },
	/* MACHINEGUN       */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* SHOTGUN          */ {    1.0f,   0.33f,    1.0f,    1.0f },
	/* GRENADE_LAUNCHER */ {    1.0f,    1.0f,   1.10f,   1.10f },
	/* ROCKET_LAUNCHER  */ {    1.0f,  1.125f,   0.90f,   0.90f },
	/* LIGHTNING        */ {    1.0f,   1.50f,   1.75f,   1.75f },
	/* RAILGUN          */ {    1.0f,    1.0f,   0.85f,   0.85f },
	/* PLASMAGUN        */ {    1.0f,    1.0f,   1.10f,   1.10f },
	/* BFG              */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* GRAPPLING_HOOK   */ {    1.0f,    1.0f,   -5.0f,   -5.0f },
#ifdef MISSIONPACK
	/* NAILGUN          */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* PROX_LAUNCHER    */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* CHAINGUN         */ {    1.0f,    1.0f,    1.0f,    1.0f },
#endif
};

static const float	wp_selfKnockback[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {    0.0f,    0.0f,    0.0f,    0.0f },
	/* GAUNTLET         */ {    1.0f,    0.5f,    1.0f,    1.0f },
	/* MACHINEGUN       */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* SHOTGUN          */ {    1.0f,   0.33f,    1.0f,    1.0f },
	/* GRENADE_LAUNCHER */ {    1.0f,    1.0f,   1.10f,   1.10f },
	/* ROCKET_LAUNCHER  */ {    1.0f,  1.125f,   1.10f,   1.10f },
	/* LIGHTNING        */ {    1.0f,   1.50f,   1.75f,   1.75f },
	/* RAILGUN          */ {    1.0f,    1.0f,   0.85f,   0.85f },
	/* PLASMAGUN        */ {    1.0f,    1.0f,   1.30f,   1.30f },
	/* BFG              */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* GRAPPLING_HOOK   */ {    1.0f,    1.0f,   -5.0f,   -5.0f },
#ifdef MISSIONPACK
	/* NAILGUN          */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* PROX_LAUNCHER    */ {    1.0f,    1.0f,    1.0f,    1.0f },
	/* CHAINGUN         */ {    1.0f,    1.0f,    1.0f,    1.0f },
#endif
};

static const int	wp_ammoMax[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {     200,     100,     150,     150 },
	/* SHOTGUN          */ {     200,      25,      25,      25 },
	/* GRENADE_LAUNCHER */ {     200,      25,      25,      25 },
	/* ROCKET_LAUNCHER  */ {     200,      25,      25,      25 },
	/* LIGHTNING        */ {     200,     150,     150,     150 },
	/* RAILGUN          */ {     200,      25,      25,      25 },
	/* PLASMAGUN        */ {     200,     100,     150,     150 },
	/* BFG              */ {     200,      25,      25,      25 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {     200,      25,      25,      25 },
	/* PROX_LAUNCHER    */ {     200,       5,       5,       5 },
	/* CHAINGUN         */ {     200,     200,     200,     200 },
#endif
};

static const int	wp_ammoBox[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {      50,      50,      50,      50 },
	/* SHOTGUN          */ {      10,       5,       5,       5 },
	/* GRENADE_LAUNCHER */ {       5,       5,       5,       5 },
	/* ROCKET_LAUNCHER  */ {       5,       5,       5,       5 },
	/* LIGHTNING        */ {      60,      50,      50,      50 },
	/* RAILGUN          */ {      10,       5,      10,      10 },
	/* PLASMAGUN        */ {      30,      50,      50,      50 },
	/* BFG              */ {      15,       5,       5,       5 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {      20,      20,      20,      20 },
	/* PROX_LAUNCHER    */ {      10,      10,      10,      10 },
	/* CHAINGUN         */ {     100,     100,     100,     100 },
#endif
};

static const int	wp_initialAmmo[WP_NUM_WEAPONS][MODE_COUNT] = {
	/* NONE             */ {       0,       0,       0,       0 },
	/* GAUNTLET         */ {       0,       0,       0,       0 },
	/* MACHINEGUN       */ {     100,      50,     100,     100 },
	/* SHOTGUN          */ {      10,      10,      10,      10 },
	/* GRENADE_LAUNCHER */ {      10,      10,      10,      10 },
	/* ROCKET_LAUNCHER  */ {      10,      10,      10,      10 },
	/* LIGHTNING        */ {     100,     100,     100,     100 },
	/* RAILGUN          */ {      10,      10,      10,      10 },
	/* PLASMAGUN        */ {      50,      50,      50,      50 },
	/* BFG              */ {      20,       5,      10,      10 },
	/* GRAPPLING_HOOK   */ {       0,       0,       0,       0 },
#ifdef MISSIONPACK
	/* NAILGUN          */ {      10,      10,      10,      10 },
	/* PROX_LAUNCHER    */ {       5,       5,       5,       5 },
	/* CHAINGUN         */ {      80,     100,     100,     100 },
#endif
};

static modeConfig_t	mode_configs[MODE_COUNT];
static qboolean		mode_configs_built = qfalse;

static void Mode_BuildConfigs( void ) {
	int m, w;
	for ( m = 0; m < MODE_COUNT; m++ ) {
		mode_configs[m].groundAccelerate = md_groundAccelerate[m];
		mode_configs[m].friction         = md_friction[m];
		mode_configs[m].jumpVelocity     = md_jumpVelocity[m];
		mode_configs[m].autoHop          = md_autoHop[m];
		mode_configs[m].rampJump         = md_rampJump[m];
		mode_configs[m].doubleJumpBonus  = md_doubleJumpBonus[m];
		mode_configs[m].airStopAccel     = md_airStopAccel[m];
		mode_configs[m].airControl       = md_airControl[m];
		mode_configs[m].strafeAccel      = md_strafeAccel[m];
		mode_configs[m].airWishspeedCap  = md_airWishspeedCap[m];

		mode_configs[m].sgPatternType        = md_sgPatternType[m];
		mode_configs[m].weaponDropTime       = md_weaponDropTime[m];
		mode_configs[m].weaponRaiseTime      = md_weaponRaiseTime[m];
		mode_configs[m].noAmmoTime           = md_noAmmoTime[m];
		mode_configs[m].splashZKnockback     = md_splashZKnockback[m];
		mode_configs[m].maxKnockback         = md_maxKnockback[m];
		mode_configs[m].duelInitialAmmoHalve = md_duelInitialAmmoHalve[m];
		mode_configs[m].armorTiered          = md_armorTiered[m];
		mode_configs[m].armorProtection      = md_armorProtection[m];
		mode_configs[m].armorGAProtection    = md_armorGAProtection[m];
		mode_configs[m].armorYAProtection    = md_armorYAProtection[m];
		mode_configs[m].armorRAProtection    = md_armorRAProtection[m];
		mode_configs[m].armorSelfProtection  = md_armorSelfProtection[m];
		mode_configs[m].armorGAMax           = md_armorGAMax[m];
		mode_configs[m].armorYAMax           = md_armorYAMax[m];
		mode_configs[m].armorRAMax           = md_armorRAMax[m];
		mode_configs[m].armorShardValue      = md_armorShardValue[m];
		mode_configs[m].armorGAPickupValue   = md_armorGAPickupValue[m];
		mode_configs[m].armorYAPickupValue   = md_armorYAPickupValue[m];
		mode_configs[m].armorRAPickupValue   = md_armorRAPickupValue[m];
		mode_configs[m].battleSuitProtection = md_battleSuitProtection[m];
		mode_configs[m].spawnHealthBonus     = md_spawnHealthBonus[m];
		mode_configs[m].respawnArmor         = md_respawnArmor[m];
		mode_configs[m].respawnHealth        = md_respawnHealth[m];
		mode_configs[m].respawnAmmo          = md_respawnAmmo[m];
		mode_configs[m].respawnPowerup       = md_respawnPowerup[m];
		mode_configs[m].respawnBattleSuit    = md_respawnBattleSuit[m];
		mode_configs[m].respawnMegahealth    = md_respawnMegahealth[m];
		mode_configs[m].megaStyle            = md_megaStyle[m];
		mode_configs[m].startPowerups        = md_startPowerups[m];

		for ( w = 0; w < WP_NUM_WEAPONS; w++ ) {
			mode_configs[m].weapons[w].damage        = wp_damage[w][m];
			mode_configs[m].weapons[w].teamDamage    = wp_teamDamage[w][m];
			mode_configs[m].weapons[w].splashDamage  = wp_splashDamage[w][m];
			mode_configs[m].weapons[w].splashRadius  = wp_splashRadius[w][m];
			mode_configs[m].weapons[w].speed         = wp_speed[w][m];
			mode_configs[m].weapons[w].fireTime      = wp_fireTime[w][m];
			mode_configs[m].weapons[w].count         = wp_count[w][m];
			mode_configs[m].weapons[w].spread        = wp_spread[w][m];
			mode_configs[m].weapons[w].range         = wp_range[w][m];
			mode_configs[m].weapons[w].knockback     = wp_knockback[w][m];
			mode_configs[m].weapons[w].selfKnockback = wp_selfKnockback[w][m];
			mode_configs[m].weapons[w].ammoMax       = wp_ammoMax[w][m];
			mode_configs[m].weapons[w].ammoBox       = wp_ammoBox[w][m];
			mode_configs[m].weapons[w].initialAmmo   = wp_initialAmmo[w][m];
		}
	}
	mode_configs_built = qtrue;
}

const modeConfig_t *Mode_GetConfig( int mode ) {
	if ( !mode_configs_built )
		Mode_BuildConfigs();
	if ( mode < MODE_VQ3 || mode >= MODE_COUNT )
		mode = MODE_VQ3;
	return &mode_configs[mode];
}

int Mode_GetAmmoBoxQuantity( const modeConfig_t *mc, int weaponTag ) {
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 0;
	return mc->weapons[weaponTag].ammoBox;
}

int Mode_GetAmmoMax( const modeConfig_t *mc, int weaponTag ) {
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 200;
	return mc->weapons[weaponTag].ammoMax;
}

int Mode_GetInitialAmmo( const modeConfig_t *mc, int weaponTag, qboolean isDuel ) {
	int ammo;
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 0;
	ammo = mc->weapons[weaponTag].initialAmmo;
	if ( isDuel && ( mc->duelInitialAmmoHalve & ( 1 << weaponTag ) ) ) {
		ammo /= 2;
	}
	return ammo;
}

// convert armor points from one tier to another based on protection ratios
int Mode_ConvertArmor( const modeConfig_t *mc, int armor, int fromType, int toType ) {
	float fromProt, toProt;
	if ( fromType == toType || armor <= 0 ) return armor;
	fromProt = Mode_ArmorProtection( mc, fromType );
	toProt = Mode_ArmorProtection( mc, toType );
	return (int)( armor * fromProt / toProt );
}

// returns the protection rate for a given armor tier
float Mode_ArmorProtection( const modeConfig_t *mc, int armorType ) {
	if ( armorType >= ARMORTYPE_RA ) return mc->armorRAProtection;
	if ( armorType >= ARMORTYPE_YA ) return mc->armorYAProtection;
	return mc->armorGAProtection;
}

// shared flat/tier/self selection so server and client absorb identically
float Mode_DamageArmorProtection( const modeConfig_t *mc, int armorType, qboolean selfDamage ) {
	if ( !mc->armorTiered )
		return mc->armorProtection;
	if ( selfDamage )
		return mc->armorSelfProtection;
	return Mode_ArmorProtection( mc, armorType );
}

qboolean Mode_CanGrabArmor( const modeConfig_t *mc, const gitem_t *item, const playerState_t *ps ) {
	// GA (item_armor_jacket) is CPM-only
	if ( item->quantity == 25 && !mc->armorTiered ) {
		return qfalse;
	}

	if ( !mc->armorTiered ) {
		// flat mode (VQ3/QL): cap at MAX_HEALTH * 2
		return ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) ? qfalse : qtrue;
	}

	// tiered mode (CPM)
	// convert current armor to the item's tier rate, then check against that tier's cap
	{
		int curType = ps->stats[STAT_ARMORTYPE];
		int curArmor = ps->stats[STAT_ARMOR];
		float fromProt, toProt;
		int converted, itemMax;

		if ( item->quantity == 100 ) {
			toProt = mc->armorRAProtection;
			itemMax = mc->armorRAMax;
		} else if ( item->quantity == 50 ) {
			toProt = mc->armorYAProtection;
			itemMax = mc->armorYAMax;
		} else if ( item->quantity == 25 ) {
			toProt = mc->armorGAProtection;
			itemMax = mc->armorGAMax;
		} else {
			// shards: pickable unless already at 200 (RA cap)
			return ( curArmor >= mc->armorRAMax ) ? qfalse : qtrue;
		}

		// convert current armor to the item's tier
		fromProt = Mode_ArmorProtection( mc, curType );

		converted = ( curType == ARMORTYPE_NONE || curArmor <= 0 )
			? 0 : (int)( curArmor * fromProt / toProt );

		return ( converted >= itemMax ) ? qfalse : qtrue;
	}
}
