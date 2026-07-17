// Vendored VR API - shared game/cgame movement + entity-state hooks (vr_bg).
// Compiles into both the game and cgame modules, like the other bg_ files.
// Every hook is dormancy-safe: on a flatscreen engine the mirror stays zeroed,
// so the transforms return stock values and the unpack no-ops.
#ifndef VR_BG_H
#define VR_BG_H

#include "q_shared.h"
// bg_public.h has no include guard (id convention); callers include it
// before this header.

#define EF_VR_PLAYER		0x00000400  // Shares bit with EF_MOVER_STOP (safe: movers aren't players)

// Decoded head orientation, stored per player by the game and cgame modules
typedef struct {
	float	pitch;
	float	yawOffset;
} vrHeadOrient_t;

qboolean BG_VR_DeadViewLocked( const playerState_t *ps );
qboolean BG_VR_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
float BG_VR_PmoveFriction( const playerState_t *ps, float stockFriction );
float BG_VR_PmoveAccelerate( const playerState_t *ps, float stockAccelerate );
void BG_VR_HeadToEntityState( const playerState_t *ps, entityState_t *s );

#endif
