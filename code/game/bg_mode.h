// bg_mode.h -- game mode definitions, shared across game/cgame/ui

#ifndef BG_MODE_H
#define BG_MODE_H

// A mode pairs a movement physics model with a combat ruleset. g_mode
// (latched, serverinfo) indexes the per-mode tables below.
typedef enum {
	MODE_VQ3,	// 0 - Quake 3
	MODE_CPM,	// 1 - CPMA
	MODE_QL,	// 2 - Quake Live
	MODE_QLT	// 3 - Quake Live Turbo (QL combat + CPM-style air control)
} mode_t;

#define MODE_COUNT 4

// sentinel "no cap" for airWishspeedCap (never exceeded; disables the strafe clamp)
#define MODE_NO_WISHSPEED_CAP 99999.0f

// human-readable profile label, e.g. "Quake 3" or "QL Turbo"
const char *BG_ModeName( int mode );

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

// Per-mode configuration: movement physics plus combat/weapon balance, one
// struct per mode. Built once from the column-major source tables in bg_mode.c
// (Mode_GetConfig transposes lazily on first use).
typedef struct {
	// movement physics (extracted from bg_pmove.c)
	float		groundAccelerate;
	float		friction;
	int			jumpVelocity;
	qboolean	autoHop;			// QL/QLT: holding jump re-jumps on landing
	qboolean	rampJump;			// additive jump velocity when already rising
	int			doubleJumpBonus;	// extra z-velocity within 400ms of last jump (CPM)
	float		airStopAccel;		// accel when air-moving against velocity
	float		airControl;			// 0 disables air control (VQ3/QL)
	float		strafeAccel;		// accel blended toward during pure strafe
	float		airWishspeedCap;	// strafe wishspeed clamp (MODE_NO_WISHSPEED_CAP = none)

	// --- combat: per-weapon data ---
	weaponConfig_t weapons[WP_NUM_WEAPONS];

	// --- weapon-specific extras ---
	int		sgPatternType;			// 0=random (VQ3), 1=ring (QL), 2=CPM dual-ring

	// --- global weapon timing ---
	int		weaponDropTime;
	int		weaponRaiseTime;
	int		noAmmoTime;

	// --- global knockback ---
	int		splashZKnockback;
	int		maxKnockback;

	// --- duel ammo modifier ---
	int		duelInitialAmmoHalve;	// bitmask: weapons halved in duel (CPM: GL|LG|RG)

	// --- armor system ---
	int		armorTiered;			// 0=flat (VQ3/QL), 1=tiered (CPM)
	float	armorProtection;		// flat-mode protection (VQ3/QL)
	float	armorGAProtection;		// tiered (CPM)
	float	armorYAProtection;
	float	armorRAProtection;
	float	armorSelfProtection;	// tiered self-damage
	int		armorGAMax;
	int		armorYAMax;
	int		armorRAMax;
	int		armorShardValue;
	int		armorGAPickupValue;
	int		armorYAPickupValue;
	int		armorRAPickupValue;
	float	battleSuitProtection;

	// --- player spawn ---
	int		spawnHealthBonus;		// VQ3/QL: 25 (spawn at 125), CPM: 0 (spawn at 100)

	// --- item respawn timing (seconds) ---
	int		respawnArmor;
	int		respawnHealth;
	int		respawnAmmo;
	int		respawnPowerup;
	int		respawnBattleSuit;
	int		respawnMegahealth;
	int		megaStyle;				// 0=VQ3 (timer), 1=CPM (20s after holder < 100hp)
	int		startPowerups;			// 0=delayed (VQ3), 1=immediate (CPM)
} modeConfig_t;

const modeConfig_t *Mode_GetConfig( int mode );

// combat/armor accessors over the resolved mode config
int Mode_GetAmmoBoxQuantity( const modeConfig_t *mc, int weaponTag );
int Mode_GetAmmoMax( const modeConfig_t *mc, int weaponTag );
int Mode_GetInitialAmmo( const modeConfig_t *mc, int weaponTag, qboolean isDuel );
float Mode_ArmorProtection( const modeConfig_t *mc, int armorType );
int Mode_ArmorMax( const modeConfig_t *mc, int armorType );
int Mode_ConvertArmor( const modeConfig_t *mc, int armor, int fromType, int toType );
qboolean Mode_CanGrabArmor( const modeConfig_t *mc, const gitem_t *item, const playerState_t *ps );

#endif // BG_MODE_H
