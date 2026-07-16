// VR screen-transform and model-fov math shared by the missionpack UI and
// cgame QVMs. This TU compiles into BOTH links (ui_shared.c does the same,
// srcs.mk missionpack CG_SRC and UI_SRC both list it), so it must define no
// globals and reference nothing UI-only or cgame-only: vr/vrActive resolve
// per-link (vr_ui.c in the ui link, vr_cgame.c in the cgame link) and
// DC resolves per-link via Init_Display (uiInfo.uiDC in the ui link, cgDC in
// the cgame link).

#include "ui_shared.h"
#include "../game/vr_shared.h"

// VR API bootstrap mirror (defined in vr_ui.c for the TA UI module,
// vr_cgame.c for the missionpack cgame that also compiles this TU)
extern vr_shared_t	*vr;
extern qboolean		vrActive;

// Display context pointer, defined in ui_shared.c and set per-link via
// Init_Display (&uiInfo.uiDC in the ui link, &cgDC in the cgame link -
// ui_main.c uses this same bare-extern pattern for the identical reason).
extern displayContextDef_t *DC;

/*
================
UI_GetProjectionCenterYOffset

Returns the Y offset (in virtual 480 coordinates) of the optical center
from the geometric center (240). VR headsets have asymmetric FOV, shifting
the optical center upward.
================
*/
static float UI_GetProjectionCenterYOffset( void )
{
	float tanUp;
	float tanDown;
	float tanHeight;

	if ( vr == NULL ) {
		return 0.0f;
	}

	tanUp = tan( vr->fov_angle_up );
	tanDown = tan( vr->fov_angle_down );
	tanHeight = tanUp - tanDown;

	if ( fabs( tanHeight ) > 0.001f ) {
		float m9 = ( tanUp + tanDown ) / tanHeight;
		// Projection center Y in virtual 480 coords = 240 * (1 + m9)
		// Offset from geometric center = 240 * m9
		return 240.0f * m9;
	}

	return 0.0f;
}

/*
================
UI_GetViewable4x3Dimensions

Calculate the maximum 4:3 area that fits within the framebuffer.
For ultra-wide headsets (e.g., Pimax 8KX with ~2:1 ratio), we may be
height-limited rather than width-limited.
================
*/
static void UI_GetViewable4x3Dimensions( float *outWidth, float *outHeight )
{
	float fbWidth = DC->glconfig.vidWidth;
	float fbHeight = DC->glconfig.vidHeight;
	float heightFromWidth = fbWidth * 0.75f;			// 4:3 height if we use full width
	float widthFromHeight = fbHeight * ( 4.0f / 3.0f );	// 4:3 width if we use full height

	if ( heightFromWidth <= fbHeight ) {
		// Normal case: width-limited, full width fits with 4:3 height
		*outWidth = fbWidth;
		*outHeight = heightFromWidth;
	} else {
		// Ultra-wide case: height-limited, constrain width to fit 4:3
		*outHeight = fbHeight;
		*outWidth = widthFromHeight;
	}
}

/*
================
UI_VR_AdjustFrom640

Shared VR virtual-screen transform for the missionpack UI and cgame draw
paths (both compile ui_shared.c, so both need the identical mapping - menu
hit-testing is done in raw 640 space, and any draw path using a different
mapping would drift the visible target away from its clickable rect).
Scales into the centered 4:3 viewable box and applies the optical-center Y
offset for the headset's asymmetric FOV; VRFM_FIRSTPERSON overrides the Y
scale/offset to the full-width safe area. Returns qfalse (x/y/w/h
untouched) when the virtual screen isn't active, so the caller falls
through to its own transform.
================
*/
qboolean UI_VR_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float viewableWidth;
	float viewableHeight;
	float xscale;
	float yscale;
	float xoffset;
	float yoffset;

	if ( !vrActive || !vr->virtual_screen ) {
		return qfalse;
	}

	// VR menus render into the centered 4:3 viewable box; scale is uniform
	// there (viewableWidth/640 == viewableHeight/480), so no per-axis or
	// pillarbox-bias term is needed - xoffset/yoffset do the centering.
	UI_GetViewable4x3Dimensions( &viewableWidth, &viewableHeight );
	xscale = viewableWidth / 640.0f;
	yscale = viewableHeight / 480.0f;
	xoffset = ( DC->glconfig.vidWidth - viewableWidth ) / 2.0f;
	yoffset = ( DC->glconfig.vidHeight - viewableHeight ) / 2.0f + UI_GetProjectionCenterYOffset() * yscale;

	// For VRFM_FIRSTPERSON, we're rendering to the full framebuffer
	// but only displaying the centered 4:3 portion, so adjust scale and offset
	if ( vr->first_person_following ) {
		// Calculate the 4:3 safe area height
		float safeHeight = ( DC->glconfig.vidWidth * 3.0f ) / 4.0f;

		// Recalculate Y scale based on the visible 4:3 area, not full height
		yscale = safeHeight / 480.0f;
		yoffset = ( DC->glconfig.vidHeight - safeHeight ) / 2.0f + UI_GetProjectionCenterYOffset() * yscale;
	}

	*x = *x * xscale + xoffset;
	*y = *y * yscale + yoffset;
	*w *= xscale;
	*h *= yscale;

	return qtrue;
}

/*
================
UI_VR_CompensateModelFov

Pre-widen a NOWORLDMODEL refdef fov so the Vulkan renderer's 4:3 cropFactor
rescale restores the intended aspect (see UI_DrawTrinitySigil / UI_DrawPlayer
/ Item_Model_Paint). Flatscreen keeps the desired fov unchanged. Origin math
at each call site must stay on the DESIRED fov, not the value written here.
================
*/
void UI_VR_CompensateModelFov( refdef_t *rd, float desiredFovX, float desiredFovY ) {
	if ( vrActive ) {
		float cropHeight = DC->glconfig.vidWidth * 0.75f;
		float cropFactor = DC->glconfig.vidHeight / cropHeight;
		rd->fov_x = 2.0f * RAD2DEG( atan2( tan( DEG2RAD( desiredFovX ) * 0.5f ) / cropFactor, 1.0f ) );
		rd->fov_y = 2.0f * RAD2DEG( atan2( tan( DEG2RAD( desiredFovY ) * 0.5f ) / cropFactor, 1.0f ) );
	} else {
		rd->fov_x = desiredFovX;
		rd->fov_y = desiredFovY;
	}
}
