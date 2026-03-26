// bg_gameplay.h -- gameplay configuration for VQ3/CPM/QL modes
// shared between game and cgame

#ifndef BG_GAMEPLAY_H
#define BG_GAMEPLAY_H

// gameplay_t enum is defined in bg_public.h (GP_VQ3, GP_CPM, GP_QL)

// armor tier types for CPM tiered armor system
#define ARMORTYPE_NONE	0
#define ARMORTYPE_GA	1
#define ARMORTYPE_YA	2
#define ARMORTYPE_RA	3

// per-weapon configuration
typedef struct {
	// combat
	int		damage;
	int		teamDamage;			// explicit team damage (same as damage unless overridden)
	int		splashDamage;		// 0 for non-splash weapons
	int		splashRadius;		// 0 for non-splash weapons
	int		speed;				// projectile speed; 0 for hitscan
	int		fireTime;
	int		count;				// SG pellets, NG burst count; 0 = N/A
	int		spread;				// MG/SG/NG/CG spread; 0 = none
	int		range;				// hitscan range (LG: 768); 0 = N/A
	// knockback
	float	knockback;			// multiplier (1.0 = normal)
	float	selfKnockback;		// self-damage knockback (explicit, not special-cased)
	// ammo
	int		ammoMax;
	int		ammoBox;
	int		initialAmmo;		// ammo on weapon pickup (MG: also used as spawn ammo)
} weaponConfig_t;

typedef struct {
	// --- Per-weapon data ---
	weaponConfig_t weapons[WP_NUM_WEAPONS];

	// --- Weapon-specific extras ---
	int		sgPatternType;			// 0=random (VQ3), 1=ring (QL), 2=CPM dual-ring

	// --- Global weapon timing ---
	int		weaponDropTime;			// VQ3: 200, CPM: 0
	int		weaponRaiseTime;		// VQ3: 250, CPM: 0, QL: 200
	int		noAmmoTime;				// VQ3: 500, CPM: 100

	// --- Global knockback ---
	int		splashZKnockback;		// VQ3/QL: 24, CPM: 36
	int		maxKnockback;			// VQ3: 200, QL: 120

	// --- Duel ammo modifier ---
	int		duelInitialAmmoHalve;	// bitmask: weapons halved in duel (CPM: GL|LG|RG)

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
	int		armorShardValue;		// VQ3: 5
	int		armorGAPickupValue;		// CPM: 50
	int		armorYAPickupValue;		// VQ3: 50, CPM: 100
	int		armorRAPickupValue;		// VQ3: 100, CPM: 150
	float	battleSuitProtection;	// VQ3: 0.5, CPM/QL: 0.25

	// --- Player spawn ---
	int		spawnHealthBonus;		// VQ3/QL: 25 (spawn at 125), CPM: 0 (spawn at 100)

	// --- Item respawn timing (seconds) ---
	int		respawnArmor;			// VQ3: 25
	int		respawnHealth;			// VQ3: 35, CPM: 30
	int		respawnAmmo;			// VQ3: 40, CPM: 30
	int		respawnPowerup;			// VQ3: 120, CPM: 90
	int		respawnBattleSuit;		// VQ3: 120
	int		respawnMegahealth;		// VQ3: 35
	int		megaStyle;				// 0=VQ3 (timer), 1=CPM (20s after holder < 100hp)
	int		startPowerups;			// 0=delayed (VQ3), 1=immediate (CPM)
} gameplayConfig_t;

const gameplayConfig_t *GP_GetConfig( int balanceMode );
int GP_GetAmmoBoxQuantity( const gameplayConfig_t *gp, int weaponTag );
int GP_GetAmmoMax( const gameplayConfig_t *gp, int weaponTag );
int GP_GetInitialAmmo( const gameplayConfig_t *gp, int weaponTag, qboolean isDuel );
float GP_ArmorProtection( const gameplayConfig_t *gp, int armorType );
int GP_ArmorMax( const gameplayConfig_t *gp, int armorType );
int GP_ConvertArmor( const gameplayConfig_t *gp, int armor, int fromType, int toType );
qboolean GP_CanGrabArmor( const gameplayConfig_t *gp, const gitem_t *item, const playerState_t *ps );

#endif // BG_GAMEPLAY_H
