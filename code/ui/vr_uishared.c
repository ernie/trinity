// VR menu-focus and model-fov helpers shared by the missionpack UI and
// cgame QVMs. This TU compiles into BOTH links (ui_shared.c does the same,
// srcs.mk missionpack CG_SRC and UI_SRC both list it), so it must define no
// globals and reference nothing UI-only or cgame-only: vrActive resolves
// per-link (vr_ui.c in the ui link, vr_cgame.c in the cgame link) and
// DC resolves per-link via Init_Display (uiInfo.uiDC in the ui link, cgDC in
// the cgame link).

#include "ui_shared.h"

// VR API bootstrap flag (defined in vr_ui.c for the TA UI module,
// vr_cgame.c for the missionpack cgame that also compiles this TU)
extern qboolean		vrActive;

// Display context pointer, defined in ui_shared.c and set per-link via
// Init_Display (&uiInfo.uiDC in the ui link, &cgDC in the cgame link -
// ui_main.c uses this same bare-extern pattern for the identical reason).
extern displayContextDef_t *DC;

/*
================
UI_VR_MenuFocusMoved

qtrue once per move of the focus to another item within the focused menu,
for the menu-hover haptic. Called every frame; a menu switch only records
the new menu, so a reopened menu's remembered focus does not buzz.
================
*/
qboolean UI_VR_MenuFocusMoved( void ) {
	static menuDef_t *lastMenu;
	static itemDef_t *lastItem;
	menuDef_t *menu;
	itemDef_t *item;
	qboolean moved;

	if ( !vrActive ) {
		return qfalse;
	}
	menu = Menu_GetFocused();
	item = menu ? Menu_GetFocusedItem( menu ) : NULL;
	moved = (qboolean)( menu == lastMenu && item && item != lastItem );
	lastMenu = menu;
	lastItem = item;
	return moved;
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
