// bg_mode.c -- game mode -> movement/gameplay axis mapping and labels
// shared between game, cgame, and ui

#include "q_shared.h"
#include "bg_public.h"
#include "bg_mode.h"

// The one seam between the unified g_mode knob and the two axis-indexed tables.
// Only QL Turbo is asymmetric: QLT movement, QL gameplay.
static const struct {
	int			movement;	// pmMovement_t
	int			gameplay;	// gameplay_t
	const char	*name;
} mode_axes[MODE_COUNT] = {
	{ PM_MOVEMENT_VQ3, GP_VQ3, "Quake 3"    },
	{ PM_MOVEMENT_CPM, GP_CPM, "CPMA"       },
	{ PM_MOVEMENT_QL,  GP_QL,  "Quake Live" },
	{ PM_MOVEMENT_QLT, GP_QL,  "QL Turbo"   },
};

void BG_ModeToAxes( int mode, int *movement, int *gameplay ) {
	if ( mode < MODE_VQ3 || mode >= MODE_COUNT )
		mode = MODE_VQ3;
	if ( movement )
		*movement = mode_axes[mode].movement;
	if ( gameplay )
		*gameplay = mode_axes[mode].gameplay;
}

const char *BG_ModeName( int mode ) {
	if ( mode < MODE_VQ3 || mode >= MODE_COUNT )
		mode = MODE_VQ3;
	return mode_axes[mode].name;
}
