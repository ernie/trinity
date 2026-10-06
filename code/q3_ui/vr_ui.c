// VR UI registration and input helpers.
#include "ui_local.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"
#include "../game/vr_bindmenu.h"
#include "../game/vr_glyph.h"

const char vr_api_sentinel[] = VR_API_SENTINEL;

vr_shared_t vr_state;
vr_shared_t *vr = &vr_state;
qboolean vrActive = qfalse;

#ifdef Q3_VM
qboolean (*trap_GetValue)( char *value, int valueSize, const char *key );
void	(*trap_VR_RegisterState)( void *state, int stateSize, int apiMajor, int apiMinor );
void	(*trap_HapticEvent)( const char *event, int position, int channel, int intensity, float yaw, float height );
void	(*trap_VKeyboard_Show)( void );
void	(*trap_VKeyboard_Hide)( void );
qboolean (*trap_VKeyboard_IsActive)( void );
qboolean (*trap_VKeyboard_HandleKey)( int key );
void	(*trap_VR_BindCapture)( void );
#else
int dll_com_trapGetValue;
int dll_trap_VR_RegisterState;
int dll_trap_HapticEvent;
int dll_trap_VKeyboard_Show;
int dll_trap_VKeyboard_Hide;
int dll_trap_VKeyboard_IsActive;
int dll_trap_VKeyboard_HandleKey;
int dll_trap_VR_BindCapture;
#endif

static qboolean bindCaptureAvailable;
// The engine's first VR key code; engines number their keys differently.
static int vrKeyFirst;

// The host's own transform, kept for the frames VR draws without the virtual screen or the HUD buffer.
static float hostXscale, hostYscale, hostBias, hostBiasY;
static qboolean hostScaleSaved;

int UI_VR_KeyIndex( int key ) {
	return bindCaptureAvailable && key >= vrKeyFirst && key < vrKeyFirst + VRBM_KEYS ? key - vrKeyFirst : -1;
}

static void UI_VRBind_Read( const char *context, int key, char *buf, int size ) {
	if ( !trap_GetValue( buf, size, va( "vr_binding %s %i", context, vrKeyFirst + key ) ) )
		buf[0] = '\0';
}

static void UI_VRBind_Bind( const char *context, int key, const char *command ) {
	char name[32];
	trap_Key_KeynumToStringBuf( vrKeyFirst + key, name, sizeof( name ) );
	trap_Cmd_ExecuteText( EXEC_NOW, va( "vrbind %s %s \"%s\"\n", context, name, command ) );
}

static void UI_VRBind_Unbind( const char *context, int key ) {
	char name[32];
	trap_Key_KeynumToStringBuf( vrKeyFirst + key, name, sizeof( name ) );
	trap_Cmd_ExecuteText( EXEC_NOW, va( "vrunbind %s %s\n", context, name ) );
}

static void UI_VRBind_CancelKeys( char *buf, int size ) {
	VRG_KeysFor( "global", "+key ESCAPE", buf, size );
}

static void UI_VRBind_Capture( void ) {
	trap_VR_BindCapture();
}

static int UI_VRBind_Defaults( const char *context, const char *command, int keys[2] ) {
	char value[16];
	int count = 0;
	while ( count < 2 && trap_GetValue( value, sizeof( value ), va( "vr_keydefault %s %i %s", context, count, command ) ) ) {
		keys[count] = atoi( value ) - vrKeyFirst;
		if ( keys[count] < 0 || keys[count] >= VRBM_KEYS )
			break;
		count++;
	}
	return count;
}

static const vrbmIO_t uiVRBindIO = {UI_VRBind_Read, UI_VRBind_Bind, UI_VRBind_Unbind,
									UI_VRBind_CancelKeys, UI_VRBind_Capture, UI_VRBind_Defaults};

static qhandle_t vrGlyphShader;

static int UI_VRGlyph_Query( const char *key, char *buf, int size ) {
	return vrActive && trap_GetValue( buf, size, key );
}

static void UI_VRGlyph_Cell( float x, float y, float size, int cell, const float *color ) {
	const float s = ( cell % VRG_GRID ) / (float)VRG_GRID, t = ( cell / VRG_GRID ) / (float)VRG_GRID;
	float w = size, h = size;
	if ( !vrGlyphShader ) {
		vrGlyphShader = trap_R_RegisterShaderNoMip( "gfx/vr/glyphs" );
	}
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s + 1.0f / VRG_GRID, t + 1.0f / VRG_GRID, vrGlyphShader );
	trap_R_SetColor( NULL );
}

static const vrgIO_t uiVRGlyphIO = {UI_VRGlyph_Query, UI_VRGlyph_Cell};

static float UI_VRGlyph_Width( const char *run, void *user ) {
	(void)user;
	return Q_PrintStrlen( run ) * SMALLCHAR_WIDTH;
}

static void UI_VRGlyph_Text( float x, float y, const char *run, const float *color, void *user ) {
	UI_DrawString( (int)x, (int)y, run, *(int *)user, (float *)color );
}

// Small-font glyphs fill the character cell, a pixel short top and bottom.
static void UI_VRGlyph_Font( vrgFont_t *font, int *style ) {
	font->glyph = SMALLCHAR_HEIGHT - 2;
	font->glyphY = 1;
	font->width = UI_VRGlyph_Width;
	font->text = UI_VRGlyph_Text;
	font->user = style;
}

float UI_VR_GlyphWidth( const char *text ) {
	vrgFont_t font;
	int style = UI_LEFT | UI_SMALLFONT;
	UI_VRGlyph_Font( &font, &style );
	return VRG_Width( &font, text );
}

void UI_VR_GlyphString( int x, int y, const char *text, int style, vec4_t color ) {
	vrgFont_t font;
	int runStyle = ( style & ~UI_FORMATMASK ) | UI_LEFT | UI_SMALLFONT;
	float w;
	UI_VRGlyph_Font( &font, &runStyle );
	w = VRG_Width( &font, text );
	if ( ( style & UI_FORMATMASK ) == UI_CENTER ) {
		x -= (int)( w / 2 );
	} else if ( ( style & UI_FORMATMASK ) == UI_RIGHT ) {
		x -= (int)w;
	}
	VRG_Paint( &font, x, y, color, text );
}

qboolean UI_VR_BindingsAvailable( void ) {
	return vrActive && bindCaptureAvailable;
}

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
#ifdef Q3_VM
	trap_GetValue = NULL;
#else
	dll_com_trapGetValue = 0;
#endif
	trap_Cvar_Set("ui_vrModeSwitchAvailable", UI_VR_CanSwitchMode() ? "1" : "0");

	vrActive = qfalse;
	VRG_SetIO( &uiVRGlyphIO );
	bindCaptureAvailable = qfalse;
	hostScaleSaved = qfalse;
	memset( &vr_state, 0, sizeof( vr_state ) );

	// keep the sentinel referenced so the toolchain retains it in the image
	if ( vr_api_sentinel[0] != 'T' )
		return;

	trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
	if ( !ext[0] )
		return;		// no extensions at all: dormant, normal mouse mode

#ifdef Q3_VM
	trap_GetValue = (void*)~atoi( ext );
#else
	dll_com_trapGetValue = atoi( ext );
#endif

	if ( !VR_RESOLVE( trap_HapticEvent, ext ) ||
	     !VR_RESOLVE( trap_VKeyboard_Show, ext ) ||
	     !VR_RESOLVE( trap_VKeyboard_Hide, ext ) ||
	     !VR_RESOLVE( trap_VKeyboard_IsActive, ext ) ||
	     !VR_RESOLVE( trap_VKeyboard_HandleKey, ext ) ) {
		return;
	}
	vrActive = VR_RegisterMirror( &vr_state );
	if ( !vrActive )
		return;
	if ( VR_RESOLVE( trap_VR_BindCapture, ext ) && trap_GetValue( ext, sizeof( ext ), "vr_keyfirst" ) ) {
		vrKeyFirst = atoi( ext );
		bindCaptureAvailable = qtrue;
		VRBM_SetIO( &uiVRBindIO );
	}
	vr->menuYawLocked = qfalse;
	vr->menuCursorActive = vrActive;
}
void UI_VR_Shutdown( void ) {
	vrActive = qfalse;
	vr->menuCursorActive = qfalse;
}

void UI_VRHaptic( const char *event, int position, int channel, int intensity, float yaw, float height ) {
	if ( !vrActive )
		return;
	trap_HapticEvent( event, position, channel, intensity, yaw, height );
}

// Virtual keyboard wrappers; dormant on a flatscreen engine.

void UI_VKeyboardShow( void ) {
	if ( !vrActive )
		return;
	trap_VKeyboard_Show();
}

qboolean UI_VKeyboardIsActive( void ) {
	if ( !vrActive )
		return qfalse;
	return trap_VKeyboard_IsActive();
}

qboolean UI_VR_StickNavActive( void ) {
	return vrActive && vr->pointerMode == VR_POINTER_STICK;
}

// The keyboard's own Tab, arrows and DONE send these; the field keeps focus until the keyboard closes.
static qboolean UI_VR_KeyboardNavKey( int key ) {
	switch ( key ) {
	case K_TAB:
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_ENTER:
	case K_KP_ENTER:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
		return qtrue;
	default:
		return qfalse;
	}
}

/*
================
UI_VR_KeyEvent

First-chance key routing. The engine hands an open keyboard its keys before
the UI sees them; this opens the keyboard when a click lands on a text field
and holds the field's focus while it is open.
================
*/
qboolean UI_VR_KeyEvent( int key, qboolean down ) {
	menucommon_s *item;

	if ( !vrActive || !down ) {
		return qfalse;
	}
	if ( UI_VKeyboardIsActive() ) {
		return UI_VR_KeyboardNavKey( key );
	}
	if ( key == K_MOUSE1 && uis.activemenu ) {
		item = Menu_ItemAtCursor( uis.activemenu );
		if ( item && item->type == MTYPE_FIELD && ( item->flags & QMF_HASMOUSEFOCUS ) &&
			 !( item->flags & ( QMF_GRAYED | QMF_INACTIVE ) ) ) {
			UI_VKeyboardShow();
		}
	}
	return qfalse;
}

qboolean UI_VR_CursorOverride( float *x, float *y ) {
	if ( vrActive && vr->menuCursorActive && vr->pointerMode != VR_POINTER_STICK ) {
		*x = vr->menuCursorX;
		*y = vr->menuCursorY;
		return qtrue;
	}
	return qfalse;
}

qboolean UI_VR_HideCursor( void ) {
	return UI_VKeyboardIsActive() || ( vrActive && vr->pointerMode != VR_POINTER_CURSOR );
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

Sets uis.xscale/yscale/bias/biasY each frame in VR, so the UI_AdjustFrom640
draws and the text painters that apply those fields themselves share one
transform: the centered 4:3 viewable box with the optical-center Y offset.
A flatscreen engine keeps the host's values untouched.
================
*/
void UI_VR_UpdateScale( void )
{
	float vw;
	float vh;
	float scale;
	float safeHeight;

	if ( !vrActive ) {
		return;
	}
	if ( !hostScaleSaved ) {
		hostXscale = uis.xscale;
		hostYscale = uis.yscale;
		hostBias = uis.bias;
		hostBiasY = uis.biasY;
		hostScaleSaved = qtrue;
	}

	if ( vr->sp_intermission_active ) {
		// SP intermission draws to the 1280x960 HUD buffer, even while the console has the virtual screen
		uis.xscale = 2.0f;
		uis.yscale = 2.0f;
		uis.bias = 0.0f;
		uis.biasY = 0.0f;
	} else if ( vr->virtual_screen ) {
		// the 4:3 box scales uniformly (vw/640 == vh/480); bias and biasY center it
		UI_GetViewable4x3Dimensions( &vw, &vh );
		scale = vw / 640.0f;
		uis.bias = ( uis.glconfig.vidWidth - vw ) / 2.0f;
		uis.biasY = ( uis.glconfig.vidHeight - vh ) / 2.0f + UI_GetProjectionCenterYOffset() * scale;

		// VRFM_FIRSTPERSON renders the full framebuffer but shows its centered 4:3 part
		if ( vr->first_person_following ) {
			safeHeight = ( uis.glconfig.vidWidth * 3.0f ) / 4.0f;
			scale = safeHeight / 480.0f;
			uis.biasY = ( uis.glconfig.vidHeight - safeHeight ) / 2.0f + UI_GetProjectionCenterYOffset() * scale;
		}
		uis.xscale = scale;
		uis.yscale = scale;
	} else {
		uis.xscale = hostXscale;
		uis.yscale = hostYscale;
		uis.bias = hostBias;
		uis.biasY = hostBiasY;
	}
}

/*
================
UI_VR_FillScreen

Whole-framebuffer fill for full-bleed screens (connect/loading).
Deliberately bypasses the 640x480 transform: in VR the framebuffer
must be covered edge-to-edge. VR-only - callers gate on vrActive and
keep the stock aspect-preserving 640x480 draw when dormant (stock
pillarboxes wide windows; this fill would paint the bars).
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

#ifndef Q3_VM
// Capability marker for trusted native-module preflight.
DLLEXPORT int TrinityVRAPI( void ) {
	return ( VR_API_MAJOR << 16 ) | VR_API_MINOR;
}
#endif
