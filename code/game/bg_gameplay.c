// bg_gameplay.c -- gameplay configuration tables
// shared between game and cgame

#include "q_shared.h"
#include "bg_public.h"
#include "bg_gameplay.h"

// Balance configurations for each mode.
// VQ3 values match current hardcoded defaults exactly.
// CPM values from community sources and testing.
// QL values from authoritative local QL server cvar dump (verified identical FFA/TDM).
// QLT is identical to QL for combat (turbo only changes movement physics).

static const gameplayConfig_t gp_configs[] = {

	// ===== GP_VQ3 (0) - Vanilla Quake 3 =====
	{
		// damage
		/* gauntletDamage */	50,
		/* mgDamage */			7,
		/* mgTeamDamage */		5,
		/* mgSpread */			200,
		/* sgDamage */			10,
		/* sgCount */			11,
		/* sgSpread */			700,
		/* sgPatternType */		0,		// random
		/* glDamage */			100,
		/* glSplashDamage */	100,
		/* glSplashRadius */	150,
		/* glSpeed */			700,
		/* rlDamage */			100,
		/* rlSplashDamage */	100,
		/* rlSplashRadius */	120,
		/* rlSpeed */			900,
		/* lgDamage */			8,
		/* lgRange */			768,
		/* rgDamage */			100,
		/* pgDamage */			20,
		/* pgSplashDamage */	15,
		/* pgSplashRadius */	20,
		/* pgSpeed */			2000,
		/* bfgDamage */			100,
		/* bfgSplashDamage */	100,
		/* bfgSplashRadius */	120,
		/* bfgSpeed */			2000,
		/* ghDamage */			0,
#ifdef MISSIONPACK
		/* ngDamage */			20,
		/* ngSpread */			500,
		/* ngSpeed */			0,		// 0 = random (555-2355)
		/* ngCount */			15,
		/* proxSplashDamage */	100,
		/* proxSplashRadius */	150,
		/* cgDamage */			7,		// same as MG
		/* cgSpread */			600,
#endif
		// fire rates
		/* gauntletFireTime */	400,
		/* mgFireTime */		100,
		/* sgFireTime */		1000,
		/* glFireTime */		800,
		/* rlFireTime */		800,
		/* lgFireTime */		50,
		/* rgFireTime */		1500,
		/* pgFireTime */		100,
		/* bfgFireTime */		200,
#ifdef MISSIONPACK
		/* ngFireTime */		1000,
		/* proxFireTime */		800,
		/* cgFireTime */		30,
#endif
		// weapon switch
		/* weaponDropTime */	200,
		/* weaponRaiseTime */	250,
		/* noAmmoTime */		500,
		// knockback multipliers
		/* gauntletKnockback */	1.0f,
		/* sgKnockback */		1.0f,
		/* glKnockback */		1.0f,
		/* rlKnockback */		1.0f,
		/* rlSelfKnockback */	1.0f,
		/* lgKnockback */		1.0f,
		/* rgKnockback */		1.0f,
		/* pgKnockback */		1.0f,
		/* pgSelfKnockback */	1.0f,
		/* ghKnockback */		1.0f,
		// splash knockback
		/* splashZKnockback */		24,
		/* splashZKnockbackSelf */	24,
		/* maxKnockback */			200,
		// ammo max
		/* mgAmmoMax */			200,
		/* sgAmmoMax */			200,
		/* glAmmoMax */			200,
		/* rlAmmoMax */			200,
		/* lgAmmoMax */			200,
		/* rgAmmoMax */			200,
		/* pgAmmoMax */			200,
		/* bfgAmmoMax */		200,
#ifdef MISSIONPACK
		/* ngAmmoMax */			200,
		/* proxAmmoMax */		200,
		/* cgAmmoMax */			200,
#endif
		// ammo box
		/* mgAmmoBox */			50,
		/* sgAmmoBox */			10,
		/* glAmmoBox */			5,
		/* rlAmmoBox */			5,
		/* lgAmmoBox */			60,
		/* rgAmmoBox */			10,
		/* pgAmmoBox */			30,
		/* bfgAmmoBox */		15,
#ifdef MISSIONPACK
		/* ngAmmoBox */			20,
		/* proxAmmoBox */		10,
		/* cgAmmoBox */			100,
#endif
		/* mgStartAmmo */		100,
		/* mgStartAmmoTeam */	50,
		// armor
		/* armorTiered */		0,
		/* armorProtection */	0.66f,
		/* armorGAProtection */	0.66f,	// unused outside of CPM
		/* armorYAProtection */	0.66f,	// unused outside of CPM
		/* armorRAProtection */	0.66f,	// unused outside of CPM
		/* armorSelfProtection */ 0.66f,	// unused outside of CPM
		/* armorGAMax */		100,	// unused outside of CPM
		/* armorYAMax */		150,	// unused outside of CPM
		/* armorRAMax */		200,	// unused outside of CPM
		/* armorShardValue */	5,
		/* armorGAPickupValue */50,		// unused outside of CPM
		/* armorYAPickupValue */50,		// from bg_itemlist quantity
		/* armorRAPickupValue */100,	// from bg_itemlist quantity
		/* battleSuitProtection */ 0.5f,
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
	// Based on http://cpma-news.org/guides/content/basics
	// with a few adjustments from testing
	{
		// damage
		/* gauntletDamage */	50,
		/* mgDamage */			5,
		/* mgTeamDamage */		5,
		/* mgSpread */			200,
		/* sgDamage */			6,
		/* sgCount */			16,
		/* sgSpread */			750,
		/* sgPatternType */		2,		// CPM dual-ring pattern (8 inner + 8 outer, offset 22.5°)
		/* glDamage */			100,
		/* glSplashDamage */	100,
		/* glSplashRadius */	150,
		/* glSpeed */			800,
		/* rlDamage */			100,
		/* rlSplashDamage */	100,
		/* rlSplashRadius */	120,
		/* rlSpeed */			1000,
		/* lgDamage */			10,
		/* lgRange */			768,
		/* rgDamage */			80,
		/* pgDamage */			18,
		/* pgSplashDamage */	15,
		/* pgSplashRadius */	20,
		/* pgSpeed */			2000,
		/* bfgDamage */			100,
		/* bfgSplashDamage */	100,
		/* bfgSplashRadius */	120,
		/* bfgSpeed */			1800,
		/* ghDamage */			0,
#ifdef MISSIONPACK
		/* ngDamage */			20,
		/* ngSpread */			500,
		/* ngSpeed */			0,		// random
		/* ngCount */			15,
		/* proxSplashDamage */	100,
		/* proxSplashRadius */	150,
		/* cgDamage */			5,		// same as CPM MG
		/* cgSpread */			600,
#endif
		// fire rates
		/* gauntletFireTime */	400,
		/* mgFireTime */		100,
		/* sgFireTime */		950,
		/* glFireTime */		800,
		/* rlFireTime */		800,
		/* lgFireTime */		66,
		/* rgFireTime */		1250,
		/* pgFireTime */		100,
		/* bfgFireTime */		1250,
#ifdef MISSIONPACK
		/* ngFireTime */		1000,
		/* proxFireTime */		800,
		/* cgFireTime */		30,
#endif
		// weapon switch
		/* weaponDropTime */	0,
		/* weaponRaiseTime */	0,
		/* noAmmoTime */		100,
		// knockback multipliers
		/* gauntletKnockback */	0.5f,
		/* sgKnockback */		0.33f,
		/* glKnockback */		1.0f,
		/* rlKnockback */		1.2f,
		/* rlSelfKnockback */	1.2f,
		/* lgKnockback */		1.50f,
		/* rgKnockback */		1.0f,
		/* pgKnockback */		1.0f,
		/* pgSelfKnockback */	0.5f,
		/* ghKnockback */		1.0f,
		// splash knockback
		/* splashZKnockback */		40,
		/* splashZKnockbackSelf */	40,
		/* maxKnockback */			200,
		// ammo max (from cpma-news.org)
		/* mgAmmoMax */			200,
		/* sgAmmoMax */			25,
		/* glAmmoMax */			25,
		/* rlAmmoMax */			25,
		/* lgAmmoMax */			150,
		/* rgAmmoMax */			25,
		/* pgAmmoMax */			100,
		/* bfgAmmoMax */		25,
#ifdef MISSIONPACK
		/* ngAmmoMax */			100,
		/* proxAmmoMax */		100,
		/* cgAmmoMax */			100,
#endif
		// ammo box
		/* mgAmmoBox */			25,
		/* sgAmmoBox */			5,
		/* glAmmoBox */			5,
		/* rlAmmoBox */			5,
		/* lgAmmoBox */			50,
		/* rgAmmoBox */			5,
		/* pgAmmoBox */			50,
		/* bfgAmmoBox */		5,
#ifdef MISSIONPACK
		/* ngAmmoBox */			20,
		/* proxAmmoBox */		10,
		/* cgAmmoBox */			100,
#endif
		/* mgStartAmmo */		50,
		/* mgStartAmmoTeam */	50,
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
		// damage (from authoritative local QL server cvar dump)
		/* gauntletDamage */	50,
		/* mgDamage */			5,
		/* mgTeamDamage */		5,
		/* mgSpread */			200,
		/* sgDamage */			5,
		/* sgCount */			20,
		/* sgSpread */			750,
		/* sgPatternType */		1,		// ring pattern
		/* glDamage */			100,
		/* glSplashDamage */	100,
		/* glSplashRadius */	150,
		/* glSpeed */			700,
		/* rlDamage */			100,
		/* rlSplashDamage */	84,
		/* rlSplashRadius */	120,
		/* rlSpeed */			1000,
		/* lgDamage */			6,
		/* lgRange */			768,
		/* rgDamage */			80,
		/* pgDamage */			20,
		/* pgSplashDamage */	15,
		/* pgSplashRadius */	20,
		/* pgSpeed */			2000,
		/* bfgDamage */			100,
		/* bfgSplashDamage */	100,
		/* bfgSplashRadius */	80,
		/* bfgSpeed */			1800,
		/* ghDamage */			10,
#ifdef MISSIONPACK
		/* ngDamage */			12,
		/* ngSpread */			400,
		/* ngSpeed */			1000,	// fixed speed
		/* ngCount */			10,		// burst fire
		/* proxSplashDamage */	100,
		/* proxSplashRadius */	150,
		/* cgDamage */			8,
		/* cgSpread */			600,
#endif
		// fire rates
		/* gauntletFireTime */	400,
		/* mgFireTime */		100,
		/* sgFireTime */		1000,
		/* glFireTime */		800,
		/* rlFireTime */		800,
		/* lgFireTime */		50,
		/* rgFireTime */		1500,
		/* pgFireTime */		100,
		/* bfgFireTime */		300,
#ifdef MISSIONPACK
		/* ngFireTime */		1000,
		/* proxFireTime */		800,
		/* cgFireTime */		50,
#endif
		// weapon switch
		/* weaponDropTime */	200,
		/* weaponRaiseTime */	200,
		/* noAmmoTime */		500,
		// knockback multipliers (from QL cvar dump)
		/* gauntletKnockback */	1.0f,
		/* sgKnockback */		1.0f,
		/* glKnockback */		1.10f,
		/* rlKnockback */		0.90f,
		/* rlSelfKnockback */	1.10f,
		/* lgKnockback */		1.75f,
		/* rgKnockback */		0.85f,
		/* pgKnockback */		1.10f,
		/* pgSelfKnockback */	1.30f,
		/* ghKnockback */		-5.0f,	// grapple pulls
		// splash knockback
		/* splashZKnockback */		24,
		/* splashZKnockbackSelf */	24,
		/* maxKnockback */			120,
		// ammo max (QL uses same as VQ3)
		/* mgAmmoMax */			200,
		/* sgAmmoMax */			200,
		/* glAmmoMax */			200,
		/* rlAmmoMax */			200,
		/* lgAmmoMax */			200,
		/* rgAmmoMax */			200,
		/* pgAmmoMax */			200,
		/* bfgAmmoMax */		200,
#ifdef MISSIONPACK
		/* ngAmmoMax */			200,
		/* proxAmmoMax */		200,
		/* cgAmmoMax */			200,
#endif
		// ammo box
		/* mgAmmoBox */			50,
		/* sgAmmoBox */			10,
		/* glAmmoBox */			5,
		/* rlAmmoBox */			5,
		/* lgAmmoBox */			60,
		/* rgAmmoBox */			10,
		/* pgAmmoBox */			30,
		/* bfgAmmoBox */		15,
#ifdef MISSIONPACK
		/* ngAmmoBox */			20,
		/* proxAmmoBox */		10,
		/* cgAmmoBox */			100,
#endif
		/* mgStartAmmo */		100,
		/* mgStartAmmoTeam */	100,
		// armor (flat, same as VQ3)
		/* armorTiered */		0,
		/* armorProtection */	0.66f,
		/* armorGAProtection */	0.66f,	// unused outside of CPM
		/* armorYAProtection */	0.66f,	// unused outside of CPM
		/* armorRAProtection */	0.66f,	// unused outside of CPM
		/* armorSelfProtection */ 0.66f,	// unused outside of CPM
		/* armorGAMax */		100,	// unused outside of CPM
		/* armorYAMax */		150,	// unused outside of CPM
		/* armorRAMax */		200,	// unused outside of CPM
		/* armorShardValue */	5,
		/* armorGAPickupValue */50,		// unused outside of CPM
		/* armorYAPickupValue */50,
		/* armorRAPickupValue */100,
		/* battleSuitProtection */ 0.25f,	// g_battleSuitDampen = 0.25
		/* spawnHealthBonus */	25,
		// respawn timing (seconds) - QL uses same as VQ3
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

int GP_GetAmmoBoxQuantity( const gameplayConfig_t *cb, int weaponTag ) {
	switch ( weaponTag ) {
	case WP_MACHINEGUN:			return cb->mgAmmoBox;
	case WP_SHOTGUN:			return cb->sgAmmoBox;
	case WP_GRENADE_LAUNCHER:	return cb->glAmmoBox;
	case WP_ROCKET_LAUNCHER:	return cb->rlAmmoBox;
	case WP_LIGHTNING:			return cb->lgAmmoBox;
	case WP_RAILGUN:			return cb->rgAmmoBox;
	case WP_PLASMAGUN:			return cb->pgAmmoBox;
#ifdef MISSIONPACK
	case WP_NAILGUN:			return cb->ngAmmoBox;
	case WP_PROX_LAUNCHER:		return cb->proxAmmoBox;
	case WP_CHAINGUN:			return cb->cgAmmoBox;
#endif
	default:					return 0;
	}
}

int GP_GetAmmoMax( const gameplayConfig_t *cb, int weaponTag ) {
	switch ( weaponTag ) {
	case WP_MACHINEGUN:			return cb->mgAmmoMax;
	case WP_SHOTGUN:			return cb->sgAmmoMax;
	case WP_GRENADE_LAUNCHER:	return cb->glAmmoMax;
	case WP_ROCKET_LAUNCHER:	return cb->rlAmmoMax;
	case WP_LIGHTNING:			return cb->lgAmmoMax;
	case WP_RAILGUN:			return cb->rgAmmoMax;
	case WP_PLASMAGUN:			return cb->pgAmmoMax;
	case WP_BFG:				return cb->bfgAmmoMax;
#ifdef MISSIONPACK
	case WP_NAILGUN:			return cb->ngAmmoMax;
	case WP_PROX_LAUNCHER:		return cb->proxAmmoMax;
	case WP_CHAINGUN:			return cb->cgAmmoMax;
#endif
	default:					return 200;
	}
}

// convert armor points from one tier to another based on protection ratios
int GP_ConvertArmor( const gameplayConfig_t *cb, int armor, int fromType, int toType ) {
	float fromProt, toProt;
	if ( fromType == toType || armor <= 0 ) return armor;
	fromProt = GP_ArmorProtection( cb, fromType );
	toProt = GP_ArmorProtection( cb, toType );
	return (int)( armor * fromProt / toProt );
}

// returns the protection rate for a given armor tier
float GP_ArmorProtection( const gameplayConfig_t *cb, int armorType ) {
	if ( armorType >= ARMORTYPE_RA ) return cb->armorRAProtection;
	if ( armorType >= ARMORTYPE_YA ) return cb->armorYAProtection;
	return cb->armorGAProtection;
}

// returns the max armor for a given armor tier
int GP_ArmorMax( const gameplayConfig_t *cb, int armorType ) {
	if ( armorType >= ARMORTYPE_RA ) return cb->armorRAMax;
	if ( armorType >= ARMORTYPE_YA ) return cb->armorYAMax;
	return cb->armorGAMax;
}

qboolean GP_CanGrabArmor( const gameplayConfig_t *cb, const gitem_t *item, const playerState_t *ps ) {
	// GA (item_armor_jacket) is CPM-only
	if ( item->quantity == 25 && !cb->armorTiered ) {
		return qfalse;
	}

	if ( !cb->armorTiered ) {
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
			toProt = cb->armorRAProtection;
			itemMax = cb->armorRAMax;
		} else if ( item->quantity == 50 ) {
			toProt = cb->armorYAProtection;
			itemMax = cb->armorYAMax;
		} else if ( item->quantity == 25 ) {
			toProt = cb->armorGAProtection;
			itemMax = cb->armorGAMax;
		} else {
			// shards: pickable unless already at 200 (RA cap)
			return ( curArmor >= cb->armorRAMax ) ? qfalse : qtrue;
		}

		// convert current armor to the item's tier
		fromProt = GP_ArmorProtection( cb, curType );

		converted = ( curType == ARMORTYPE_NONE || curArmor <= 0 )
			? 0 : (int)( curArmor * fromProt / toProt );

		return ( converted >= itemMax ) ? qfalse : qtrue;
	}
}
