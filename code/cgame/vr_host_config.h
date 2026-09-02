// Vendored VR API - host configuration. This is the ONE file in the drop the
// host tree is meant to edit: it tells vr_cgame.c which optional host
// features exist. Everything else in the drop keeps the never-edit rule.
//
// A stock tree also uses this file to satisfy small contract gaps, e.g.:
//   #define Q_sscanf sscanf
#ifndef VR_HOST_CONFIG_H
#define VR_HOST_CONFIG_H

// This tree has TrinityVision TV playback with timeline scrubbing
// (cgs.tvPlayback / cgs.tvScrubActive / cgs.tvScrubSavedMenuYaw).
// 0: the drop skips the scrub menu-yaw save/restore coupling.
#define VR_HOST_HAS_TV 1

// This tree fires warmup/countdown announcer events from the HUD pass
// (CG_WarmupEvents). 0: the drop never calls it and it need not exist.
#define VR_HOST_HAS_WARMUP_EVENTS 1

// This tree hands the grapple out per server (cgs.grappleEnabled) and holds a
// captured player on a mover (CG_GrappleLatchAnchor, CG_GrappleOwnerRGBA).
#define VR_HOST_HAS_GRAPPLE 1

// This tree clips view-path traces against movers at their drawn pose
// (CG_TraceRender). 0: the drop traces through CG_Trace.
#define VR_HOST_HAS_TRACE_RENDER 1

// Optional: the weapon wheel's default set - what "*" and an empty
// cg_weaponSelectorWeapons expand to - in the cvar's token language.
// Tokens are whitespace-separated (commas are not separators); a token is
// a weapon_t number or an item classname with or without the weapon_
// prefix ("rocketlauncher", "weapon_railgun", "5"). "*" expands the
// default set ("*" here means the built-in set); "!name" hides a weapon
// from later mentions and expansions; first mention wins. A list of only
// exclusions means the default set minus those weapons.
// Unset: the built-in set (every weapon except the gauntlet and the
// grappling hook).
//#define VR_WHEEL_DEFAULT_WEAPONS "*"

#endif // VR_HOST_CONFIG_H
