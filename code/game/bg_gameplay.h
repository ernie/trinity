// bg_gameplay.h -- gameplay configuration for VQ3/CPM/QL/QLT modes
// shared between game and cgame

#ifndef BG_GAMEPLAY_H
#define BG_GAMEPLAY_H

// gameplay_t enum is defined in bg_public.h (GP_VQ3, GP_CPM, GP_QL)

// armor tier types for CPM tiered armor system
#define ARMORTYPE_NONE	0
#define ARMORTYPE_GA	1
#define ARMORTYPE_YA	2
#define ARMORTYPE_RA	3

typedef struct {
	// --- Damage (server-only, but table is shared for simplicity) ---

	int		gauntletDamage;			// VQ3: 50
	int		mgDamage;				// VQ3: 7,  CPM: 5,  QL: 5
	int		mgTeamDamage;			// VQ3: 5
	int		mgSpread;				// VQ3: 200
	int		sgDamage;				// VQ3: 10, CPM: 7,  QL: 5 (per pellet)
	int		sgCount;				// VQ3: 11, CPM: 16, QL: 20
	int		sgSpread;				// VQ3: 700, CPM: 900, QL: 994 (ring pattern)
	int		sgPatternType;			// 0=random (VQ3/CPM), 1=ring (QL)
	int		glDamage;				// VQ3: 100
	int		glSplashDamage;			// VQ3: 100
	int		glSplashRadius;			// VQ3: 150
	int		glSpeed;				// VQ3: 700, CPM: 800
	int		rlDamage;				// VQ3: 100
	int		rlSplashDamage;			// VQ3: 100, QL: 84
	int		rlSplashRadius;			// VQ3: 120
	int		rlSpeed;				// VQ3: 900, CPM: 1000, QL: 1000
	int		lgDamage;				// VQ3: 8, QL: 6
	int		lgRange;				// VQ3: 768
	int		rgDamage;				// VQ3: 100, QL: 80
	int		pgDamage;				// VQ3: 20, CPM: 15
	int		pgSplashDamage;			// VQ3: 15
	int		pgSplashRadius;			// VQ3: 20
	int		pgSpeed;				// VQ3: 2000
	int		bfgDamage;				// VQ3: 100
	int		bfgSplashDamage;		// VQ3: 100
	int		bfgSplashRadius;		// VQ3: 120, QL: 80
	int		bfgSpeed;				// VQ3: 2000, QL: 1800
	int		ghDamage;				// VQ3: 0, QL: 10

#ifdef MISSIONPACK
	// Missionpack weapons
	int		ngDamage;				// VQ3: 20, QL: 12
	int		ngSpread;				// VQ3: 500, QL: 400
	int		ngSpeed;				// VQ3: 0 (random 555-2355), QL: 1000 (fixed)
	int		ngCount;				// VQ3: 1, QL: 10 (per burst)
	int		proxSplashDamage;		// VQ3: 100
	int		proxSplashRadius;		// VQ3: 150
	int		cgDamage;				// VQ3: 7 (=MG), QL: 8
	int		cgSpread;				// VQ3: 600
#endif

	// --- Fire rates (shared - client predicts in bg_pmove.c) ---

	int		gauntletFireTime;		// VQ3: 400
	int		mgFireTime;				// VQ3: 100
	int		sgFireTime;				// VQ3: 1000
	int		glFireTime;				// VQ3: 800, CPM: 600
	int		rlFireTime;				// VQ3: 800
	int		lgFireTime;				// VQ3: 50
	int		rgFireTime;				// VQ3: 1500, CPM: 1250
	int		pgFireTime;				// VQ3: 100
	int		bfgFireTime;			// VQ3: 200, QL: 300
#ifdef MISSIONPACK
	int		ngFireTime;				// VQ3: 1000
	int		proxFireTime;			// VQ3: 800
	int		cgFireTime;				// VQ3: 30, QL: 50
#endif

	// --- Weapon switch timing (shared - client predicts) ---

	int		weaponDropTime;			// VQ3: 200, CPM: 0
	int		weaponRaiseTime;		// VQ3: 250, CPM: 0, QL: 200
	int		noAmmoTime;				// VQ3: 500, CPM: 100

	// --- Per-weapon knockback multipliers (server-only) ---

	float	gauntletKnockback;		// VQ3: 1.0, CPM: 0.5
	float	sgKnockback;			// VQ3: 1.0, CPM: 1.35
	float	glKnockback;			// VQ3: 1.0, QL: 1.10
	float	rlKnockback;			// VQ3: 1.0, CPM: 1.2, QL: 0.90
	float	rlSelfKnockback;		// VQ3: 1.0, CPM: 1.2, QL: 1.10
	float	lgKnockback;			// VQ3: 1.0, CPM: 1.55, QL: 1.75
	float	rgKnockback;			// VQ3: 1.0, QL: 0.85
	float	pgKnockback;			// VQ3: 1.0, CPM: 0.5, QL: 1.10
	float	pgSelfKnockback;		// VQ3: 1.0, CPM: 0.5, QL: 1.30
	float	ghKnockback;			// VQ3: 1.0, QL: -5 (pull toward)

	// --- Splash knockback (server-only) ---

	int		splashZKnockback;		// VQ3: 24, CPM: 40
	int		splashZKnockbackSelf;	// VQ3: 24, QL: 24
	int		maxKnockback;			// VQ3: 200, QL: 120

	// --- Ammo ---

	int		mgAmmoMax;				// VQ3: 200, CPM: 200
	int		sgAmmoMax;				// VQ3: 200, CPM: 25
	int		glAmmoMax;				// VQ3: 200, CPM: 25
	int		rlAmmoMax;				// VQ3: 200, CPM: 25
	int		lgAmmoMax;				// VQ3: 200, CPM: 150
	int		rgAmmoMax;				// VQ3: 200, CPM: 25
	int		pgAmmoMax;				// VQ3: 200, CPM: 100
	int		bfgAmmoMax;				// VQ3: 200, CPM: 25
#ifdef MISSIONPACK
	int		ngAmmoMax;				// VQ3: 200
	int		proxAmmoMax;			// VQ3: 200
	int		cgAmmoMax;				// VQ3: 200
#endif
	int		mgAmmoBox;				// VQ3: 50, CPM: 25
	int		sgAmmoBox;				// VQ3: 10, CPM: 5
	int		glAmmoBox;				// VQ3: 5
	int		rlAmmoBox;				// VQ3: 5
	int		lgAmmoBox;				// VQ3: 60, CPM: 50
	int		rgAmmoBox;				// VQ3: 10, CPM: 5
	int		pgAmmoBox;				// VQ3: 30, CPM: 50
	int		bfgAmmoBox;				// VQ3: 15, CPM: 5
#ifdef MISSIONPACK
	int		ngAmmoBox;				// VQ3: 20
	int		proxAmmoBox;			// VQ3: 10
	int		cgAmmoBox;				// VQ3: 100
#endif
	int		mgStartAmmo;			// VQ3: 100, CPM: 50
	int		mgStartAmmoTeam;		// VQ3: 50, CPM: 50, QL: 100

	// --- Armor system ---

	int		armorTiered;			// 0=flat (VQ3/QL), 1=tiered (CPM)
	float	armorProtection;		// flat mode: 0.66 (VQ3/QL)
	float	armorGAProtection;		// tiered: 0.50 (CPM)
	float	armorYAProtection;		// tiered: 0.66 (CPM)
	float	armorRAProtection;		// tiered: 0.75 (CPM)
	float	armorSelfProtection;	// tiered: 0.50 (CPM) - self-damage always 50/50
	int		armorGAMax;				// tiered: 100
	int		armorYAMax;				// tiered: 150
	int		armorRAMax;				// tiered: 200
	int		armorShardValue;		// VQ3: 5, CPM: 5
	int		armorGAPickupValue;		// CPM: 50
	int		armorYAPickupValue;		// VQ3: 50 (from bg_itemlist), CPM: 100 (item_armor_body)
	int		armorRAPickupValue;		// VQ3: 100 (from bg_itemlist), CPM: 150 (CPMA RA entity)
	float	battleSuitProtection;	// VQ3: 0.5, CPM: 0.25

	// --- Spawn health ---

	int		spawnHealthBonus;		// VQ3/QL: 25 (spawn at 125), CPM: 0 (spawn at 100)

	// --- Item respawn timing (in seconds) ---

	int		respawnArmor;			// VQ3: 25
	int		respawnHealth;			// VQ3: 35, CPM: 30
	int		respawnAmmo;			// VQ3: 40, CPM: 30
	int		respawnPowerup;			// VQ3: 120, CPM: 60
	int		respawnBattleSuit;		// VQ3: 120, CPM: 120 (separate from powerup)
	int		respawnMegahealth;		// VQ3: 35
	int		megaStyle;				// 0=VQ3 (timer), 1=CPM (20s after holder < 100hp)
	int		startPowerups;			// 0=delayed (VQ3), 1=immediate (CPM)
} gameplayConfig_t;

const gameplayConfig_t *GP_GetConfig( int balanceMode );
int GP_GetAmmoBoxQuantity( const gameplayConfig_t *cb, int weaponTag );
int GP_GetAmmoMax( const gameplayConfig_t *cb, int weaponTag );
float GP_ArmorProtection( const gameplayConfig_t *cb, int armorType );
int GP_ArmorMax( const gameplayConfig_t *cb, int armorType );
int GP_ConvertArmor( const gameplayConfig_t *cb, int armor, int fromType, int toType );
qboolean GP_CanGrabArmor( const gameplayConfig_t *cb, const gitem_t *item, const playerState_t *ps );

#endif // BG_GAMEPLAY_H
