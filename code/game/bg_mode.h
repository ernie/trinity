// bg_mode.h -- game mode definitions, shared across game/cgame/ui

#ifndef BG_MODE_H
#define BG_MODE_H

// A "mode" is the single player-facing profile that selects both a movement
// physics model and a combat ruleset. g_mode is the only latched serverinfo
// cvar; g_movement / g_gameplay are derived from it (see BG_ModeToAxes).
typedef enum {
	MODE_VQ3,	// 0 - Quake 3
	MODE_CPM,	// 1 - CPMA
	MODE_QL,	// 2 - Quake Live
	MODE_QLT	// 3 - Quake Live Turbo (QL combat + CPM-style air control)
} mode_t;

#define MODE_COUNT 4

// mode -> the movement/gameplay axis values the existing tables are indexed by
void BG_ModeToAxes( int mode, int *movement, int *gameplay );

// human-readable profile label, e.g. "Quake 3" or "QL Turbo"
const char *BG_ModeName( int mode );

#endif // BG_MODE_H
