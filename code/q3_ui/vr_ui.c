// VR API bootstrap for the baseq3 UI module. Name-resolves the extension traps
// and registers the vr_shared_t mirror so the UI runs VR-aware under the VR
// engine, and degrades to normal mouse mode on a flatscreen host. Mirrors the
// field-proven cgame bootstrap (vr_cgame.c).
#include "ui_local.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"

const char vr_api_sentinel[] = VR_API_SENTINEL;

vr_shared_t vr_state;
vr_shared_t *vr = &vr_state;
qboolean vrActive = qfalse;

#ifdef Q3_VM
qboolean (*trap_GetValue)( char *value, int valueSize, const char *key );
void	(*trap_VR_RegisterState)( void *state, int stateSize, int apiMajor, int apiMinor );
void	(*trap_HapticEvent)( const char *description, int position, int channel, int intensity, float yaw, float height );
void	(*trap_VKeyboard_Show)( void );
void	(*trap_VKeyboard_Hide)( void );
qboolean (*trap_VKeyboard_IsActive)( void );
qboolean (*trap_VKeyboard_HandleKey)( int key );
#else
int dll_com_trapGetValue;
int dll_trap_VR_RegisterState;
int dll_trap_HapticEvent;
int dll_trap_VKeyboard_Show;
int dll_trap_VKeyboard_Hide;
int dll_trap_VKeyboard_IsActive;
int dll_trap_VKeyboard_HandleKey;
#endif

/*
================
UI_VR_Platform

Menu content keys on this, never on the raw vr_platform cvar; see VR_Platform
for the two-gate rule.
================
*/
vrPlatform_t UI_VR_Platform( void ) {
	return VR_Platform( vrActive );
}

void UI_VR_Init( void ) {
	char ext[64];

	// keep the sentinel referenced so the toolchain retains it in the image
	if ( vr_api_sentinel[0] != 'T' )
		return;

	trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
	if ( !ext[0] )
		return;		// flatscreen engine: dormant, normal mouse mode

#ifdef Q3_VM
	trap_GetValue = (void*)~atoi( ext );
#else
	dll_com_trapGetValue = atoi( ext );
#endif

	// trap_VR_RegisterState is the VR handshake; an engine can expose
	// trap_GetValue for non-VR extensions yet not answer this, which means
	// "not a VR engine" - stay dormant.
	if ( !VR_RESOLVE( trap_VR_RegisterState, ext ) )
		return;

	vr_state.structSize = sizeof( vr_state );
	vr_state.apiVersion = VR_API_MAJOR;
	trap_VR_RegisterState( &vr_state, sizeof( vr_state ), VR_API_MAJOR, VR_API_MINOR );
	vrActive = qtrue;

	// The rest of the VR trap set is part of the v1 contract, so a registered
	// engine provides all of it - bind unconditionally.
	VR_RESOLVE( trap_HapticEvent, ext );
	VR_RESOLVE( trap_VKeyboard_Show, ext );
	VR_RESOLVE( trap_VKeyboard_Hide, ext );
	VR_RESOLVE( trap_VKeyboard_IsActive, ext );
	VR_RESOLVE( trap_VKeyboard_HandleKey, ext );

	// engine cursor-registration replacement: the UI owns the menu cursor
	vr->menuCursorActive = qtrue;
}

/*
================
UI_VR_Shutdown

Mirror unwind on UI shutdown - releases the menu cursor.
Safe on a dormant mirror (writes into the local zeroed struct).
================
*/
void UI_VR_Shutdown( void ) {
	vr->menuCursorActive = qfalse;
}

void UI_VRHaptic( const char *description, int position, int channel, int intensity, float yaw, float height ) {
	if ( !vrActive )
		return;
	trap_HapticEvent( description, position, channel, intensity, yaw, height );
}

// Virtual keyboard wrappers; dormant on a flatscreen host.

void UI_VKeyboardShow( void ) {
	if ( !vrActive )
		return;
	trap_VKeyboard_Show();
}

void UI_VKeyboardHide( void ) {
	if ( !vrActive )
		return;
	trap_VKeyboard_Hide();
}

qboolean UI_VKeyboardIsActive( void ) {
	if ( !vrActive )
		return qfalse;
	return trap_VKeyboard_IsActive();
}

qboolean UI_VKeyboardHandleKey( int key ) {
	if ( !vrActive )
		return qfalse;
	return trap_VKeyboard_HandleKey( key );
}

qboolean UI_VR_StickNavActive( void ) {
	return vrActive && vr->menuStickNavActive;
}

/*
================
UI_VR_KeyEvent

First-chance key routing: an active virtual keyboard consumes its keys.
qfalse lets the stock menu key path run (including for unconsumed keys
while the keyboard is up - stock behavior preserved).
================
*/
qboolean UI_VR_KeyEvent( int key ) {
	if ( UI_VKeyboardIsActive() && UI_VKeyboardHandleKey( key ) ) {
		return qtrue;
	}
	return qfalse;
}

qboolean UI_VR_CursorOverride( float *x, float *y ) {
	if ( vrActive && vr->menuCursorActive && !vr->menuStickNavActive ) {
		*x = vr->menuCursorX;
		*y = vr->menuCursorY;
		return qtrue;
	}
	return qfalse;
}

qboolean UI_VR_HideCursor( void ) {
	return UI_VKeyboardIsActive() || ( vrActive && vr->menuStickNavActive );
}

void UI_VR_OnMenuMove( void ) {
	UI_VRHaptic( "menu_move", 0, 0, 30, 0, 0 );
}

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
	float fbWidth = uis.glconfig.vidWidth;
	float fbHeight = uis.glconfig.vidHeight;
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
UI_VR_UpdateScale

Recompute uis.scale/biasX/biasY (and the derived cursor/screen bounds) each
frame so that ALL draw paths share one transform: both the UI_AdjustFrom640-
routed widgets/cursor AND the direct-scale text draws (UI_DrawBannerString2 /
UI_DrawProportionalString2 / UI_DrawString2, which apply uis.scale/biasX/biasY
themselves and bypass UI_AdjustFrom640).

In VR the centered viewable 4:3 box and optical-center Y offset are baked into
uis.* here. On flatscreen the stock aspect-preserving values are restored so a
VR -> non-VR transition can't leave VR values stuck.
================
*/
void UI_VR_UpdateScale( void )
{
	float vw;
	float vh;
	float safeHeight;

	if ( vrActive && vr->virtual_screen ) {
		// VR menus render into the centered 4:3 viewable box; scale is uniform
		// there (vw/640 == vh/480), so biasX/biasY do the centering.
		UI_GetViewable4x3Dimensions( &vw, &vh );
		uis.scale = vw / 640.0f;
		uis.biasX = ( uis.glconfig.vidWidth - vw ) / 2.0f;
		uis.biasY = ( uis.glconfig.vidHeight - vh ) / 2.0f + UI_GetProjectionCenterYOffset() * uis.scale;

		// For VRFM_FIRSTPERSON we render to the full framebuffer but only display
		// the centered 4:3 portion, so scale/bias off the visible safe area.
		if ( vr->first_person_following ) {
			safeHeight = ( uis.glconfig.vidWidth * 3.0f ) / 4.0f;
			uis.scale = safeHeight / 480.0f;
			uis.biasY = ( uis.glconfig.vidHeight - safeHeight ) / 2.0f + UI_GetProjectionCenterYOffset() * uis.scale;
			// biasX unchanged: xscale still vw/640 == uis.scale when width-limited
		}
	} else if ( vrActive && vr->sp_intermission_active ) {
		// SP intermission: drawing to the HUD buffer (1280x960), 2x scale, no offset
		uis.scale = 2.0f;
		uis.biasX = 0.0f;
		uis.biasY = 0.0f;
	} else {
		// stock flatscreen aspect-preserving values (mirrors ui_main.c:147-166)
		uis.biasX = 0.0f;
		uis.biasY = 0.0f;
		// for 640x480 virtualized screen
		if ( uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640 ) {
			// wide screen, scale by height
			uis.scale = uis.glconfig.vidHeight * ( 1.0f / 480.0f );
			uis.biasX = 0.5f * ( uis.glconfig.vidWidth - ( uis.glconfig.vidHeight * ( 640.0f / 480.0f ) ) );
		} else {
			// no wide screen, scale by width
			uis.scale = uis.glconfig.vidWidth * ( 1.0f / 640.0f );
			uis.biasY = 0.5f * ( uis.glconfig.vidHeight - ( uis.glconfig.vidWidth * ( 480.0f / 640 ) ) );
		}
	}

	// derived fields kept consistent with the chosen scale/bias in every branch
	uis.screenXmin = 0.0f - ( uis.biasX / uis.scale );
	uis.screenXmax = 640.0f + ( uis.biasX / uis.scale );
	uis.screenYmin = 0.0f - ( uis.biasY / uis.scale );
	uis.screenYmax = 480.0f + ( uis.biasY / uis.scale );

	uis.cursorScaleR = 1.0f / uis.scale;
	if ( uis.cursorScaleR < 0.5f ) {
		uis.cursorScaleR = 0.5f;
	}
}

/*
================
UI_VR_FillScreen

Whole-framebuffer fill for full-bleed screens (connect/loading).
Deliberately bypasses the 640x480 transform: in VR the framebuffer
must be covered edge-to-edge; on flatscreen this matches the stock
full-window stretch.
================
*/
void UI_VR_FillScreen( qhandle_t shader ) {
	trap_R_DrawStretchPic( 0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, shader );
}

/*
=================
UI_VR_CompensateModelFov

Pre-widen a NOWORLDMODEL refdef fov so the Vulkan renderer's 4:3 cropFactor
rescale restores the intended aspect (see UI_DrawPlayer). Flatscreen keeps the
desired fov unchanged. Origin math must stay on the DESIRED fov, not the value
written here.
=================
*/
void UI_VR_CompensateModelFov( refdef_t *rd, float desiredFovX, float desiredFovY ) {
	if ( vrActive ) {
		float cropHeight = uis.glconfig.vidWidth * 0.75f;
		float cropFactor = uis.glconfig.vidHeight / cropHeight;
		rd->fov_x = 2.0f * RAD2DEG( atan2( tan( DEG2RAD( desiredFovX ) * 0.5f ) / cropFactor, 1.0f ) );
		rd->fov_y = 2.0f * RAD2DEG( atan2( tan( DEG2RAD( desiredFovY ) * 0.5f ) / cropFactor, 1.0f ) );
	} else {
		rd->fov_x = desiredFovX;
		rd->fov_y = desiredFovY;
	}
}
