// Vendored VR API - shared platform query (vr_platform). Compiles into the
// cgame and ui modules; the game module has no use for it.
#ifndef VR_PLATFORM_H
#define VR_PLATFORM_H

#include "q_shared.h"

typedef enum {
	VRP_NONE,
	VRP_PC,
	VRP_QUEST
} vrPlatform_t;

// VRP_NONE is 0 and contributes no bits, so the enum cannot double as a mask.
#define VRPM_FLAT	( 1 << VRP_NONE )
#define VRPM_PC		( 1 << VRP_PC )
#define VRPM_QUEST	( 1 << VRP_QUEST )

vrPlatform_t VR_Platform( qboolean vrActive );

#endif
