// bg_gameplay.c -- gameplay configuration tables
// shared between game and cgame

#include "q_shared.h"
#include "bg_public.h"
#include "bg_gameplay.h"

// Balance configurations for each mode.
// VQ3 values match original Quake 3 defaults.
// CPM values verified against actual CPMA mod (cpma-news.org guide has inaccuracies).
// QL values from authoritative local QL server cvar dump and in-game testing.

static const gameplayConfig_t gp_configs[] = {

	// ===== GP_VQ3 (0) - Vanilla Quake 3 =====
	{
		// weapons[WP_NUM_WEAPONS]
		{
		/* WP_NONE */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0,		/* selfKnockback */	0,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_GAUNTLET */ {
			/* damage */		50,		/* teamDamage */	50,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		400,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_MACHINEGUN */ {
			/* damage */		7,		/* teamDamage */	5,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		100,
			/* count */			0,		/* spread */		200,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		50,		/* initialAmmo */	100,
		},
		/* WP_SHOTGUN */ {
			/* damage */		10,		/* teamDamage */	10,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1000,
			/* count */			11,		/* spread */		700,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		10,		/* initialAmmo */	10,
		},
		/* WP_GRENADE_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			700,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_ROCKET_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	120,
			/* speed */			900,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_LIGHTNING */ {
			/* damage */		8,		/* teamDamage */	8,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		50,
			/* count */			0,		/* spread */		0,		/* range */			768,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		60,		/* initialAmmo */	100,
		},
		/* WP_RAILGUN */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1500,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		10,		/* initialAmmo */	10,
		},
		/* WP_PLASMAGUN */ {
			/* damage */		20,		/* teamDamage */	20,
			/* splashDamage */	15,		/* splashRadius */	20,
			/* speed */			2000,	/* fireTime */		100,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		30,		/* initialAmmo */	50,
		},
		/* WP_BFG */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	120,
			/* speed */			2000,	/* fireTime */		200,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		15,		/* initialAmmo */	20,
		},
		/* WP_GRAPPLING_HOOK */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
#ifdef MISSIONPACK
		/* WP_NAILGUN */ {
			/* damage */		20,		/* teamDamage */	20,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1000,
			/* count */			15,		/* spread */		500,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		20,		/* initialAmmo */	10,
		},
		/* WP_PROX_LAUNCHER */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			700,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		10,		/* initialAmmo */	5,
		},
		/* WP_CHAINGUN */ {
			/* damage */		7,		/* teamDamage */	7,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		30,
			/* count */			0,		/* spread */		600,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		100,	/* initialAmmo */	80,
		},
#endif
		},
		/* sgPatternType */		0,		// random
		// weapon switch
		/* weaponDropTime */	200,
		/* weaponRaiseTime */	250,
		/* noAmmoTime */		500,
		// splash knockback
		/* splashZKnockback */		24,
		/* maxKnockback */			200,
		// duel
		/* duelInitialAmmoHalve */	0,
		// armor
		/* armorTiered */		0,
		/* armorProtection */	0.66f,
		/* armorGAProtection */	0.66f,
		/* armorYAProtection */	0.66f,
		/* armorRAProtection */	0.66f,
		/* armorSelfProtection */ 0.66f,
		/* armorGAMax */		100,
		/* armorYAMax */		150,
		/* armorRAMax */		200,
		/* armorShardValue */	5,
		/* armorGAPickupValue */50,
		/* armorYAPickupValue */50,
		/* armorRAPickupValue */100,
		/* battleSuitProtection */ 0.5f,
		// spawn
		/* spawnHealthBonus */	25,
		// respawn timing (seconds)
		/* respawnArmor */		25,
		/* respawnHealth */		35,
		/* respawnAmmo */		40,
		/* respawnPowerup */	120,
		/* respawnBattleSuit */	120,
		/* respawnMegahealth */	35,
		/* megaStyle */			0,		// timer-based
		/* startPowerups */		0,		// delayed
	},

	// ===== GP_CPM (1) - CPMA =====
	// Verified against actual CPMA mod (cpma-news.org guide has inaccuracies)
	{
		// weapons[WP_NUM_WEAPONS]
		{
		/* WP_NONE */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0,		/* selfKnockback */	0,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_GAUNTLET */ {
			/* damage */		50,		/* teamDamage */	50,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		400,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0.5f,	/* selfKnockback */	0.5f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_MACHINEGUN */ {
			/* damage */		5,		/* teamDamage */	5,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		100,
			/* count */			0,		/* spread */		200,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		100,	/* ammoBox */		50,		/* initialAmmo */	50,
		},
		/* WP_SHOTGUN */ {
			/* damage */		6,		/* teamDamage */	6,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		950,
			/* count */			16,		/* spread */		750,	/* range */			0,
			/* knockback */		0.33f,	/* selfKnockback */	0.33f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_GRENADE_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			800,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_ROCKET_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	120,
			/* speed */			1000,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.2f,	/* selfKnockback */	1.2f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_LIGHTNING */ {
			/* damage */		10,		/* teamDamage */	10,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		66,
			/* count */			0,		/* spread */		0,		/* range */			768,
			/* knockback */		1.50f,	/* selfKnockback */	1.50f,
			/* ammoMax */		150,	/* ammoBox */		50,		/* initialAmmo */	100,
		},
		/* WP_RAILGUN */ {
			/* damage */		80,		/* teamDamage */	80,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1250,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_PLASMAGUN */ {
			/* damage */		18,		/* teamDamage */	18,
			/* splashDamage */	14,		/* splashRadius */	20,
			/* speed */			2000,	/* fireTime */		100,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		100,	/* ammoBox */		50,		/* initialAmmo */	50,
		},
		/* WP_BFG */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	120,
			/* speed */			1800,	/* fireTime */		1250,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	5,
		},
		/* WP_GRAPPLING_HOOK */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
#ifdef MISSIONPACK
		/* WP_NAILGUN */ {
			/* damage */		20,		/* teamDamage */	20,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1000,
			/* count */			15,		/* spread */		500,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		20,		/* initialAmmo */	10,
		},
		/* WP_PROX_LAUNCHER */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			800,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		5,		/* ammoBox */		10,		/* initialAmmo */	5,
		},
		/* WP_CHAINGUN */ {
			/* damage */		5,		/* teamDamage */	5,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		30,
			/* count */			0,		/* spread */		600,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		100,	/* ammoBox */		100,	/* initialAmmo */	80,
		},
#endif
		},
		/* sgPatternType */		2,		// CPM dual-ring (8 inner + 8 outer, offset 22.5)
		// weapon switch
		/* weaponDropTime */	0,
		/* weaponRaiseTime */	0,
		/* noAmmoTime */		100,
		// splash knockback
		/* splashZKnockback */		36,
		/* maxKnockback */			200,
		// duel
		/* duelInitialAmmoHalve */	(1<<WP_GRENADE_LAUNCHER)|(1<<WP_LIGHTNING)|(1<<WP_RAILGUN),
		// armor (tiered)
		/* armorTiered */		1,
		/* armorProtection */	0.66f,	// fallback for non-tiered code paths
		/* armorGAProtection */	0.50f,
		/* armorYAProtection */	0.66f,
		/* armorRAProtection */	0.75f,
		/* armorSelfProtection */ 0.50f,
		/* armorGAMax */		100,
		/* armorYAMax */		150,
		/* armorRAMax */		200,
		/* armorShardValue */	5,
		/* armorGAPickupValue */50,
		/* armorYAPickupValue */100,	// armorsystem 0: YA=100
		/* armorRAPickupValue */150,	// CPM RA sets to 150
		/* battleSuitProtection */ 0.25f,
		// spawn
		/* spawnHealthBonus */	0,		// CPM: spawn at 100hp
		// respawn timing (seconds)
		/* respawnArmor */		25,
		/* respawnHealth */		30,
		/* respawnAmmo */		30,
		/* respawnPowerup */	90,
		/* respawnBattleSuit */	120,
		/* respawnMegahealth */	35,		// not used when megaStyle=1
		/* megaStyle */			1,		// CPM: respawns 20s after holder <= 100hp
		/* startPowerups */		1,		// immediate
	},

	// ===== GP_QL (2) - Quake Live =====
	{
		// weapons[WP_NUM_WEAPONS]
		{
		/* WP_NONE */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0,		/* selfKnockback */	0,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_GAUNTLET */ {
			/* damage */		50,		/* teamDamage */	50,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		400,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
		/* WP_MACHINEGUN */ {
			/* damage */		5,		/* teamDamage */	5,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		100,
			/* count */			0,		/* spread */		200,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		150,	/* ammoBox */		50,		/* initialAmmo */	100,
		},
		/* WP_SHOTGUN */ {
			/* damage */		5,		/* teamDamage */	5,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1000,
			/* count */			20,		/* spread */		750,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_GRENADE_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			700,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.10f,	/* selfKnockback */	1.10f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_ROCKET_LAUNCHER */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	84,		/* splashRadius */	120,
			/* speed */			1000,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0.90f,	/* selfKnockback */	1.10f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_LIGHTNING */ {
			/* damage */		6,		/* teamDamage */	6,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		50,
			/* count */			0,		/* spread */		0,		/* range */			768,
			/* knockback */		1.75f,	/* selfKnockback */	1.75f,
			/* ammoMax */		150,	/* ammoBox */		50,		/* initialAmmo */	100,
		},
		/* WP_RAILGUN */ {
			/* damage */		80,		/* teamDamage */	80,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		1500,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		0.85f,	/* selfKnockback */	0.85f,
			/* ammoMax */		25,		/* ammoBox */		10,		/* initialAmmo */	10,
		},
		/* WP_PLASMAGUN */ {
			/* damage */		20,		/* teamDamage */	20,
			/* splashDamage */	15,		/* splashRadius */	20,
			/* speed */			2000,	/* fireTime */		100,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.10f,	/* selfKnockback */	1.30f,
			/* ammoMax */		150,	/* ammoBox */		50,		/* initialAmmo */	50,
		},
		/* WP_BFG */ {
			/* damage */		100,	/* teamDamage */	100,
			/* splashDamage */	100,	/* splashRadius */	80,
			/* speed */			1800,	/* fireTime */		300,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		5,		/* initialAmmo */	10,
		},
		/* WP_GRAPPLING_HOOK */ {
			/* damage */		10,		/* teamDamage */	10,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		0,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		-5.0f,	/* selfKnockback */	-5.0f,
			/* ammoMax */		0,		/* ammoBox */		0,		/* initialAmmo */	0,
		},
#ifdef MISSIONPACK
		/* WP_NAILGUN */ {
			/* damage */		12,		/* teamDamage */	12,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			1000,	/* fireTime */		1000,
			/* count */			10,		/* spread */		400,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		25,		/* ammoBox */		20,		/* initialAmmo */	10,
		},
		/* WP_PROX_LAUNCHER */ {
			/* damage */		0,		/* teamDamage */	0,
			/* splashDamage */	100,	/* splashRadius */	150,
			/* speed */			700,	/* fireTime */		800,
			/* count */			0,		/* spread */		0,		/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		5,		/* ammoBox */		10,		/* initialAmmo */	5,
		},
		/* WP_CHAINGUN */ {
			/* damage */		8,		/* teamDamage */	8,
			/* splashDamage */	0,		/* splashRadius */	0,
			/* speed */			0,		/* fireTime */		50,
			/* count */			0,		/* spread */		600,	/* range */			0,
			/* knockback */		1.0f,	/* selfKnockback */	1.0f,
			/* ammoMax */		200,	/* ammoBox */		100,	/* initialAmmo */	100,
		},
#endif
		},
		/* sgPatternType */		1,		// ring pattern
		// weapon switch
		/* weaponDropTime */	200,
		/* weaponRaiseTime */	200,
		/* noAmmoTime */		500,
		// splash knockback
		/* splashZKnockback */		24,
		/* maxKnockback */			120,
		// duel
		/* duelInitialAmmoHalve */	0,
		// armor (flat, same as VQ3)
		/* armorTiered */		0,
		/* armorProtection */	0.66f,
		/* armorGAProtection */	0.66f,
		/* armorYAProtection */	0.66f,
		/* armorRAProtection */	0.66f,
		/* armorSelfProtection */ 0.66f,
		/* armorGAMax */		100,
		/* armorYAMax */		150,
		/* armorRAMax */		200,
		/* armorShardValue */	5,
		/* armorGAPickupValue */50,
		/* armorYAPickupValue */50,
		/* armorRAPickupValue */100,
		/* battleSuitProtection */ 0.25f,
		// spawn
		/* spawnHealthBonus */	25,
		// respawn timing (seconds)
		/* respawnArmor */		25,
		/* respawnHealth */		35,
		/* respawnAmmo */		40,
		/* respawnPowerup */	120,
		/* respawnBattleSuit */	120,
		/* respawnMegahealth */	35,
		/* megaStyle */			0,		// timer-based
		/* startPowerups */		0,		// delayed
	},
};

const gameplayConfig_t *GP_GetConfig( int balanceMode ) {
	if ( balanceMode < GP_VQ3 || balanceMode > GP_QL )
		balanceMode = GP_VQ3;
	return &gp_configs[balanceMode];
}

int GP_GetAmmoBoxQuantity( const gameplayConfig_t *gp, int weaponTag ) {
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 0;
	return gp->weapons[weaponTag].ammoBox;
}

int GP_GetAmmoMax( const gameplayConfig_t *gp, int weaponTag ) {
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 200;
	return gp->weapons[weaponTag].ammoMax;
}

int GP_GetInitialAmmo( const gameplayConfig_t *gp, int weaponTag, qboolean isDuel ) {
	int ammo;
	if ( weaponTag <= WP_NONE || weaponTag >= WP_NUM_WEAPONS )
		return 0;
	ammo = gp->weapons[weaponTag].initialAmmo;
	if ( isDuel && ( gp->duelInitialAmmoHalve & ( 1 << weaponTag ) ) ) {
		ammo /= 2;
	}
	return ammo;
}

// convert armor points from one tier to another based on protection ratios
int GP_ConvertArmor( const gameplayConfig_t *gp, int armor, int fromType, int toType ) {
	float fromProt, toProt;
	if ( fromType == toType || armor <= 0 ) return armor;
	fromProt = GP_ArmorProtection( gp, fromType );
	toProt = GP_ArmorProtection( gp, toType );
	return (int)( armor * fromProt / toProt );
}

// returns the protection rate for a given armor tier
float GP_ArmorProtection( const gameplayConfig_t *gp, int armorType ) {
	if ( armorType >= ARMORTYPE_RA ) return gp->armorRAProtection;
	if ( armorType >= ARMORTYPE_YA ) return gp->armorYAProtection;
	return gp->armorGAProtection;
}

// returns the max armor for a given armor tier
int GP_ArmorMax( const gameplayConfig_t *gp, int armorType ) {
	if ( armorType >= ARMORTYPE_RA ) return gp->armorRAMax;
	if ( armorType >= ARMORTYPE_YA ) return gp->armorYAMax;
	return gp->armorGAMax;
}

qboolean GP_CanGrabArmor( const gameplayConfig_t *gp, const gitem_t *item, const playerState_t *ps ) {
	// GA (item_armor_jacket) is CPM-only
	if ( item->quantity == 25 && !gp->armorTiered ) {
		return qfalse;
	}

	if ( !gp->armorTiered ) {
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
			toProt = gp->armorRAProtection;
			itemMax = gp->armorRAMax;
		} else if ( item->quantity == 50 ) {
			toProt = gp->armorYAProtection;
			itemMax = gp->armorYAMax;
		} else if ( item->quantity == 25 ) {
			toProt = gp->armorGAProtection;
			itemMax = gp->armorGAMax;
		} else {
			// shards: pickable unless already at 200 (RA cap)
			return ( curArmor >= gp->armorRAMax ) ? qfalse : qtrue;
		}

		// convert current armor to the item's tier
		fromProt = GP_ArmorProtection( gp, curType );

		converted = ( curType == ARMORTYPE_NONE || curArmor <= 0 )
			? 0 : (int)( curArmor * fromProt / toProt );

		return ( converted >= itemMax ) ? qfalse : qtrue;
	}
}
