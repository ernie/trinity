// VR UI registration and input helpers.
#include "ui_local.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"
#include "../game/vr_bindmenu.h"
#include "../game/vr_glyph.h"
#include "../game/vr_supersample.h"

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

typedef struct {
	float scale;
	int style;
} uiVRGlyphText_t;

static float UI_VRGlyph_Width( const char *run, void *user ) {
	return uiInfo.uiDC.textWidth( run, ( (uiVRGlyphText_t *)user )->scale, 0 );
}

static void UI_VRGlyph_Text( float x, float y, const char *run, const float *color, void *user ) {
	const uiVRGlyphText_t *text = (const uiVRGlyphText_t *)user;
	uiInfo.uiDC.drawText( x, y, text->scale, (float *)color, run, 0, 0, text->style );
}

// Glyphs stand 1.4 cap heights tall above the baseline.
static void UI_VRGlyph_Font( vrgFont_t *font, uiVRGlyphText_t *text, float scale, int style ) {
	text->scale = scale;
	text->style = style;
	font->glyph = uiInfo.uiDC.textHeight( "A", scale, 0 ) * 1.4f;
	font->glyphY = -font->glyph * 0.85f;
	font->width = UI_VRGlyph_Width;
	font->text = UI_VRGlyph_Text;
	font->user = text;
}

float UI_VR_GlyphWidth( const char *text, float scale ) {
	vrgFont_t font;
	uiVRGlyphText_t run;
	UI_VRGlyph_Font( &font, &run, scale, 0 );
	return VRG_Width( &font, text );
}

void UI_VR_GlyphPaint( float x, float y, float scale, vec4_t color, const char *text, int style ) {
	vrgFont_t font;
	uiVRGlyphText_t run;
	UI_VRGlyph_Font( &font, &run, scale, style );
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

static void UI_VR_Attach( void ) {
	char ext[64];
#ifdef Q3_VM
	trap_GetValue = NULL;
#else
	dll_com_trapGetValue = 0;
#endif
	vrActive = qfalse;
	bindCaptureAvailable = qfalse;
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

// Menus gate their VR rows on these with cvarTest; a flatscreen engine reads 0.
void UI_VR_Init( void ) {
	trap_Cvar_Set( "ui_vrModeSwitchAvailable", UI_VR_CanSwitchMode() ? "1" : "0" );
	VRG_SetIO( &uiVRGlyphIO );
	UI_VR_Attach();
	trap_Cvar_Register( NULL, "ui_vrActive", "0", CVAR_ROM );
	trap_Cvar_Set( "ui_vrActive", vrActive ? "1" : "0" );
	trap_Cvar_Set( "ui_vrBindingsAvailable", UI_VR_BindingsAvailable() ? "1" : "0" );
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
		return qtrue;
	default:
		return qfalse;
	}
}

static itemDef_t *UI_VR_FocusedItem( void ) {
	menuDef_t *menu = Menu_GetFocused();
	return menu ? Menu_GetFocusedItem( menu ) : NULL;
}

// Stick navigation steps the focused slider by a fortieth of its range, as stock has no key for it.
static qboolean UI_VR_StepSlider( int key ) {
	itemDef_t *item = UI_VR_FocusedItem();
	editFieldDef_t *editDef;
	float value;

	if ( key != K_LEFTARROW && key != K_KP_LEFTARROW && key != K_RIGHTARROW && key != K_KP_RIGHTARROW ) {
		return qfalse;
	}
	if ( !item || item->type != ITEM_TYPE_SLIDER || !item->cvar || !item->typeData ) {
		return qfalse;
	}
	editDef = (editFieldDef_t *)item->typeData;
	value = trap_Cvar_VariableValue( item->cvar );
	value += ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ? 1 : -1 ) * ( editDef->maxVal - editDef->minVal ) / 40.0f;
	if ( value < editDef->minVal ) {
		value = editDef->minVal;
	} else if ( value > editDef->maxVal ) {
		value = editDef->maxVal;
	}
	trap_Cvar_Set( item->cvar, va( "%f", value ) );
	return qtrue;
}

/*
================
UI_VR_KeyEvent

First-chance key routing. While a bindings row waits for a button, every key
belongs to the bindings model; closing the menu ends the wait. The engine
hands an open keyboard its keys before the UI sees them; the edit field keeps
focus while it is open. Stick navigation steps the focused slider.
================
*/
qboolean UI_VR_KeyEvent( int key, qboolean down ) {
	if ( VRBM_Waiting() ) {
		if ( down ) {
			if ( key == K_ESCAPE ) {
				VRBM_Cancel();
			} else if ( UI_VR_KeyIndex( key ) >= 0 ) {
				VRBM_Capture( UI_VR_KeyIndex( key ) );
			} else {
				VRBM_OtherKey();
			}
		}
		return qtrue;
	}
	if ( !down ) {
		return qfalse;
	}
	if ( UI_VKeyboardIsActive() ) {
		return UI_VR_KeyboardNavKey( key );
	}
	if ( UI_VR_StickNavActive() ) {
		return UI_VR_StepSlider( key );
	}
	return qfalse;
}

/*
================
UI_VR_CursorOverride

Called every frame from _UI_Refresh and from _UI_MouseEvent. The laser
pointer sets the cursor; under stick navigation the cursor rests on the
focused item (a slider's thumb), so the stock key paths that test the
cursor against that item act on it. Also drives the menu-hover haptic.
================
*/
qboolean UI_VR_CursorOverride( float *x, float *y ) {
	itemDef_t *item;

	if ( UI_VR_MenuFocusMoved() ) {
		UI_VR_OnMenuMove();
	}
	if ( !vrActive ) {
		return qfalse;
	}
	if ( vr->pointerMode == VR_POINTER_STICK ) {
		item = UI_VR_FocusedItem();
		if ( !item ) {
			return qfalse;
		}
		*x = item->type == ITEM_TYPE_SLIDER ? Item_Slider_ThumbPosition( item ) : item->window.rect.x + item->window.rect.w / 2;
		*y = item->window.rect.y + item->window.rect.h / 2;
		return qtrue;
	}
	if ( vr->menuCursorActive ) {
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

// displayContextDef_t.vrEditField: an edit field started editing.
void UI_VR_OnEditField( void ) {
	UI_VKeyboardShow();
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
	trap_R_DrawStretchPic( 0, 0, uiInfo.uiDC.glconfig.vidWidth, uiInfo.uiDC.glconfig.vidHeight, 0, 0, 1, 1, shader );
}

/*
===============
UI_VR_LoadMenus

Loads the VR options pages when running under a VR engine. Platform-variant
pages are selected by UI_VR_Platform(), which applies the two-gate rule: a
dormant mirror (flatscreen engine) or an unrecognized vr_platform value both
collapse to VRP_NONE, and no VR pages are parsed either way.
===============
*/
void UI_VR_LoadMenus( void ) {
	vrPlatform_t platform = UI_VR_Platform();
	const char *manifest;
	fileHandle_t f;
	int len;

	if ( platform == VRP_NONE ) {
		return;
	}
	manifest = ( platform == VRP_STANDALONE ) ? "ui/vrmenus_standalone.txt" : "ui/vrmenus_pc.txt";

	len = trap_FS_FOpenFile( manifest, &f, FS_READ );
	if ( f ) {
		trap_FS_FCloseFile( f );
	}
	if ( len <= 0 ) {
		return;
	}

	UI_LoadMenus( manifest, qfalse );
}

/*
===============
UI_VR_RunMenuScript
===============
*/
qboolean UI_VR_RunMenuScript( const char *name ) {
	// the Display Mode row binds ui_vrEnabled; vr_enabled is latched, so the choice goes through the console
	if ( Q_stricmp( name, "vrModeSetup" ) == 0 ) {
		trap_Cvar_Set( "ui_vrEnabled", UI_VR_ModeChoice() ? "1" : "0" );
		return qtrue;
	} else if ( Q_stricmp( name, "vrModeChanged" ) == 0 ) {
		UI_VR_ChooseMode( trap_Cvar_VariableValue( "ui_vrEnabled" ) != 0 );
		return qtrue;
	} else if ( Q_stricmp( name, "vrBindCancel" ) == 0 ) {
		VRBM_Cancel();
		return qtrue;
	} else if ( Q_stricmp( name, "vrBindOpen" ) == 0 ) {
		VRBM_Open();
		return qtrue;
	} else if ( Q_stricmp( name, "vrBindReset" ) == 0 ) {
		// run now and drop the cached cells, so the rows show the defaults at once
		trap_Cmd_ExecuteText( EXEC_NOW, "vr_bindreset\n" );
		VRBM_Forget();
		return qtrue;
	} else if ( Q_stricmp( name, "vrHudDrawStatusChanged" ) == 0 ) {
		// the HUD mode without a status bar draws no 3D icons
		if ( UI_VR_Platform() != VRP_NONE ) {
			trap_Cvar_SetValue( "cg_draw3dIcons", (int)trap_Cvar_VariableValue( "vr_hudDrawStatus" ) == 2 ? 0 : 1 );
		}
		return qtrue;
	} else if ( Q_stricmp( name, "vrMirrorSetup" ) == 0 ) {
		// Stage the restart-class desktop-mirror values into ui_ cvars.
		// Mode: 0=Off, 1=Windowed, 2=Fullscreen from vr_mirrorEnabled + vr_mirrorFullscreen.
		if ( UI_VR_Platform() != VRP_NONE ) {
			int mirror, fullscreen, w, h;
			mirror = (int)trap_Cvar_VariableValue( "vr_mirrorEnabled" );
			fullscreen = (int)trap_Cvar_VariableValue( "vr_mirrorFullscreen" );
			if ( mirror == 0 ) {
				trap_Cvar_Set( "ui_vrDesktopMode", "0" );
			} else if ( fullscreen == 0 ) {
				trap_Cvar_Set( "ui_vrDesktopMode", "1" );
			} else {
				trap_Cvar_Set( "ui_vrDesktopMode", "2" );
			}
			w = (int)trap_Cvar_VariableValue( "vr_mirrorWidth" );
			h = (int)trap_Cvar_VariableValue( "vr_mirrorHeight" );
			if ( w > 0 && h > 0 ) {
				trap_Cvar_Set( "ui_vrDesktopRes", va( "%dx%d", w, h ) );
			} else {
				trap_Cvar_Set( "ui_vrDesktopRes", "default" );
			}
		}
		return qtrue;
	} else if ( Q_stricmp( name, "vrMirrorNextRes" ) == 0 ) {
		// Cycle the staged resolution through the engine-detected mode list.
		if ( UI_VR_Platform() != VRP_NONE ) {
			char modes[MAX_STRING_CHARS];
			char cur[32];
			char *s, *first, *pick;
			trap_Cvar_VariableStringBuffer( "r_availableModes", modes, sizeof( modes ) );
			if ( modes[0] != '\0' ) {
				trap_Cvar_VariableStringBuffer( "ui_vrDesktopRes", cur, sizeof( cur ) );
				// split the space-separated list in place; pick the entry
				// after the staged one, wrapping to the first
				first = modes;
				pick = NULL;
				s = modes;
				while ( s && *s ) {
					char *next = strchr( s, ' ' );
					if ( next ) {
						*next++ = '\0';
					}
					if ( pick == NULL && Q_stricmp( s, cur ) == 0 ) {
						pick = next; // may be NULL/empty -> wrap below
					}
					s = next;
				}
				if ( pick == NULL || *pick == '\0' ) {
					pick = first;
				}
				trap_Cvar_Set( "ui_vrDesktopRes", pick );
			}
		}
		return qtrue;
	} else if ( Q_stricmp( name, "vrMirrorApply" ) == 0 ) {
		// Restart only when staged mirror settings changed.
		if ( UI_VR_Platform() != VRP_NONE ) {
			int stagedMode, curMode, mirror, fullscreen, dirty;
			char res[32];
			char *xp;
			dirty = 0;
			mirror = (int)trap_Cvar_VariableValue( "vr_mirrorEnabled" );
			fullscreen = (int)trap_Cvar_VariableValue( "vr_mirrorFullscreen" );
			if ( mirror == 0 ) {
				curMode = 0;
			} else if ( fullscreen == 0 ) {
				curMode = 1;
			} else {
				curMode = 2;
			}
			stagedMode = (int)trap_Cvar_VariableValue( "ui_vrDesktopMode" );
			if ( stagedMode != curMode ) {
				if ( stagedMode == 0 ) {
					trap_Cvar_SetValue( "vr_mirrorEnabled", 0 );
				} else if ( stagedMode == 1 ) {
					trap_Cvar_SetValue( "vr_mirrorEnabled", 1 );
					trap_Cvar_SetValue( "vr_mirrorFullscreen", 0 );
				} else {
					trap_Cvar_SetValue( "vr_mirrorEnabled", 1 );
					trap_Cvar_SetValue( "vr_mirrorFullscreen", 1 );
				}
				dirty = 1;
			}
			trap_Cvar_VariableStringBuffer( "ui_vrDesktopRes", res, sizeof( res ) );
			xp = strchr( res, 'x' );
			if ( xp ) {
				int stagedW, stagedH, curW, curH;
				*xp = '\0';
				stagedW = atoi( res );
				stagedH = atoi( xp + 1 );
				curW = (int)trap_Cvar_VariableValue( "vr_mirrorWidth" );
				curH = (int)trap_Cvar_VariableValue( "vr_mirrorHeight" );
				if ( stagedW > 0 && stagedH > 0 && ( stagedW != curW || stagedH != curH ) ) {
					trap_Cvar_Set( "vr_mirrorWidth", va( "%d", stagedW ) );
					trap_Cvar_Set( "vr_mirrorHeight", va( "%d", stagedH ) );
					dirty = 1;
				}
			}
			if ( dirty ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
			}
		}
		return qtrue;
	}
	return qfalse;
}

/*
===============
VR settings and bindings owner-draws

The host's UI_OwnerDraw, UI_OwnerDrawHandleKey and UI_OwnerDrawWidth call the
entry points below from their default cases; the IDs are in ui/menudef.h.
===============
*/
#define MAX_REFRESH_RATES 16
static int uiRefreshRates[MAX_REFRESH_RATES];
static int uiNumRefreshRates;

static void UI_ReadRefreshRates(void) {
	static const int fallback[] = { 60, 72, 80, 90, 120 };
	char list[256];
	char *p;
	int i;

	uiNumRefreshRates = 0;
	trap_Cvar_VariableStringBuffer("vr_refreshrates", list, sizeof(list));
	p = list;
	while (uiNumRefreshRates < MAX_REFRESH_RATES) {
		const char *token = COM_Parse(&p);
		if (!token[0]) {
			break;
		}
		uiRefreshRates[uiNumRefreshRates++] = (int)(atof(token) + 0.5f);
	}
	if (uiNumRefreshRates == 0) {
		for (i = 0; i < ARRAY_LEN(fallback); i++) {
			uiRefreshRates[i] = fallback[i];
		}
		uiNumRefreshRates = ARRAY_LEN(fallback);
	}
}

// nearest, since an archived vr_refreshrate may not be in this headset's list
static int UI_RefreshRateIndex(void) {
	int rate = (int)(trap_Cvar_VariableValue("vr_refreshrate") + 0.5f);
	int best = 0;
	int i;

	UI_ReadRefreshRates();
	for (i = 1; i < uiNumRefreshRates; i++) {
		if (abs(uiRefreshRates[i] - rate) < abs(uiRefreshRates[best] - rate)) {
			best = i;
		}
	}
	return best;
}

static void UI_DrawRefreshRate(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	uiInfo.uiDC.drawText(rect->x, rect->y, scale, color, va("%i Hz", uiRefreshRates[UI_RefreshRateIndex()]), 0, 0, textStyle);
}

static qboolean UI_RefreshRate_HandleKey(int flags, float *special, int key) {
	int select = UI_SelectForKey(key);
	if (select != 0) {
		int i = UI_RefreshRateIndex() + select;

		if (i >= uiNumRefreshRates) {
			i = 0;
		} else if (i < 0) {
			i = uiNumRefreshRates - 1;
		}

		// the engine applies this live and writes back the rate it actually got
		trap_Cvar_SetValue("vr_refreshrate", uiRefreshRates[i]);
		return qtrue;
	}
	return qfalse;
}

// vr_foveation: 0 off, 1 fixed, 2 eye tracked. vr_foveationStrength: 1 low, 2 medium, 3 high.
// vr_foveationCaps (none / fixed / eyetracked) caps the mode row at what the headset has.
static const char *uiFoveationNames[] = { "Off", "Fixed", "Eye-Tracked" };
static const char *uiFoveationStrengthNames[] = { "Low", "Medium", "High" };

// highest vr_foveation value this headset supports, -1 when it has none
static int UI_FoveationMax(void) {
	char caps[32];

	trap_Cvar_VariableStringBuffer("vr_foveationCaps", caps, sizeof(caps));
	if (!Q_stricmp(caps, "eyetracked")) {
		return 2;
	}
	if (!Q_stricmp(caps, "fixed")) {
		return 1;
	}
	return -1;
}

static int UI_FoveationLevel(int max) {
	int level = (int)trap_Cvar_VariableValue("vr_foveation");

	if (level < 0) {
		level = 0;
	} else if (level > max) {
		level = max;
	}
	return level;
}

static const char *UI_FoveationText(void) {
	int max = UI_FoveationMax();

	if (max < 0) {
		return "Not Supported";
	}
	return uiFoveationNames[UI_FoveationLevel(max)];
}

static void UI_DrawFoveation(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	uiInfo.uiDC.drawText(rect->x, rect->y, scale, color, UI_FoveationText(), 0, 0, textStyle);
}

static qboolean UI_Foveation_HandleKey(int flags, float *special, int key) {
	int select = UI_SelectForKey(key);
	if (select != 0) {
		int max = UI_FoveationMax();
		int level;

		if (max < 0) {
			return qtrue;
		}
		level = UI_FoveationLevel(max) + select;
		if (level > max) {
			level = 0;
		} else if (level < 0) {
			level = max;
		}

		// the engine applies this live and falls back if the runtime refuses
		trap_Cvar_SetValue("vr_foveation", level);
		return qtrue;
	}
	return qfalse;
}

static int UI_FoveationStrength(void) {
	int strength = (int)trap_Cvar_VariableValue("vr_foveationStrength");

	if (strength < 1) {
		strength = 1;
	} else if (strength > 3) {
		strength = 3;
	}
	return strength;
}

static const char *UI_FoveationStrengthText(void) {
	if (UI_FoveationMax() < 0) {
		return "Not Supported";
	}
	return uiFoveationStrengthNames[UI_FoveationStrength() - 1];
}

static void UI_DrawFoveationStrength(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	uiInfo.uiDC.drawText(rect->x, rect->y, scale, color, UI_FoveationStrengthText(), 0, 0, textStyle);
}

static qboolean UI_FoveationStrength_HandleKey(int flags, float *special, int key) {
	int select = UI_SelectForKey(key);
	if (select != 0) {
		int strength;

		if (UI_FoveationMax() < 0) {
			return qtrue;
		}
		strength = UI_FoveationStrength() + select;
		if (strength > 3) {
			strength = 1;
		} else if (strength < 1) {
			strength = 3;
		}

		trap_Cvar_SetValue("vr_foveationStrength", strength);
		return qtrue;
	}
	return qfalse;
}

// Only the player's own tilt: the engine applies the grip-to-aim correction itself, so zero reads as none.
static const char *UI_WeaponPitchText(void) {
	int offset = (int)trap_Cvar_VariableValue("vr_weaponPitch");

	// Kept terse: this sits in the narrow strip beside the slider bar
	return va("%s%i", offset > 0 ? "+" : "", offset);
}

static void UI_DrawWeaponPitch(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	uiInfo.uiDC.drawText(rect->x, rect->y, scale, color, UI_WeaponPitchText(), 0, 0, textStyle);
}

// vr_superSampling as a 1.0..2.0 slider in tenths, with the eye size each step renders at beside it.
static rectDef_t uiSuperSamplingBar;   // the bar's window from its last paint, so a click can land on it
static int uiEyeSize[4];               // recommended w h, maximum w h; zeros outside VR
static int uiEyeSizeTime = -1;
static qboolean uiSuperSamplingDrag;   // a press on the bar follows the pointer until the button lets go

static void UI_ReadEyeSize(void) {
	char eyeSize[64];
	if (uiEyeSizeTime >= 0 && uiInfo.uiDC.realTime - uiEyeSizeTime < 1000) {
		return;
	}
	uiEyeSizeTime = uiInfo.uiDC.realTime;
	if (!trap_GetValue(eyeSize, sizeof(eyeSize), "vr_eyesize") ||
		!VRSS_ParseEyeSize(eyeSize, &uiEyeSize[0], &uiEyeSize[1], &uiEyeSize[2], &uiEyeSize[3])) {
		memset(uiEyeSize, 0, sizeof(uiEyeSize));
	}
}

// the step under the pointer, with the pointer held to the bar
static int UI_SuperSampling_TenthsAt(float cursorx) {
	float t = (cursorx - uiSuperSamplingBar.x) / uiSuperSamplingBar.w;
	if (t < 0) {
		t = 0;
	} else if (t > 1) {
		t = 1;
	}
	return VRSS_MIN_TENTHS + (int)(t * (VRSS_MAX_TENTHS - VRSS_MIN_TENTHS) + 0.5f);
}

static void UI_DrawSuperSampling(rectDef_t *rect, float span, float scale, vec4_t color, int textStyle) {
	int tenths = VRSS_Tenths(trap_Cvar_VariableValue("vr_superSampling"));
	// rect->y is the row's text baseline; the bar sits on it like the stock slider sits in its row
	const float top = rect->y - SLIDER_HEIGHT, gap = 4, bar = SLIDER_WIDTH;
	const int textHeight = uiInfo.uiDC.textHeight("0", scale, 0);
	float cx, y;
	char mult[8], res[24];

	if (uiSuperSamplingDrag) {
		if (!trap_Key_IsDown(K_MOUSE1)) {
			uiSuperSamplingDrag = qfalse;
		} else if (uiSuperSamplingBar.w > 0 && UI_SuperSampling_TenthsAt(uiInfo.uiDC.cursorx) != tenths) {
			tenths = UI_SuperSampling_TenthsAt(uiInfo.uiDC.cursorx);
			trap_Cvar_SetValue("vr_superSampling", VRSS_Value(tenths));
		}
	}
	UI_ReadEyeSize();
	VRSS_Multiplier(tenths, mult, sizeof(mult));
	VRSS_Resolution(tenths, uiEyeSize[0], uiEyeSize[1], uiEyeSize[2], uiEyeSize[3], res, sizeof(res));
	if (span <= bar + gap) {
		span = bar + gap + bar;
	}
	uiSuperSamplingBar.x = rect->x;
	uiSuperSamplingBar.y = top;
	uiSuperSamplingBar.w = bar;
	uiSuperSamplingBar.h = SLIDER_HEIGHT;
	trap_R_SetColor(color);
	UI_DrawHandlePic(rect->x, top, bar, SLIDER_HEIGHT, uiInfo.uiDC.Assets.sliderBar);
	UI_DrawHandlePic(rect->x + (tenths - VRSS_MIN_TENTHS) * bar / (VRSS_MAX_TENTHS - VRSS_MIN_TENTHS) - SLIDER_THUMB_WIDTH / 2,
					 top - 2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, uiInfo.uiDC.Assets.sliderThumb);
	trap_R_SetColor(NULL);
	// two lines, each centered in what is left of the span, the pair centered on the bar
	cx = rect->x + bar + gap + (span - bar - gap) / 2;
	y = top + SLIDER_HEIGHT / 2 - (textHeight + 3) / 2.0f + textHeight / 2.0f;
	uiInfo.uiDC.drawText(cx - uiInfo.uiDC.textWidth(mult, scale, 0) / 2, y, scale, color, mult, 0, 0, textStyle);
	uiInfo.uiDC.drawText(cx - uiInfo.uiDC.textWidth(res, scale, 0) / 2, y + textHeight + 3, scale, color, res, 0, 0, textStyle);
}

static qboolean UI_SuperSampling_HandleKey(int flags, float *special, int key) {
	int tenths = VRSS_Tenths(trap_Cvar_VariableValue("vr_superSampling"));
	const float cx = uiInfo.uiDC.cursorx, cy = uiInfo.uiDC.cursory;
	int select;

	if (key == K_MOUSE1 && cx >= uiSuperSamplingBar.x && cx < uiSuperSamplingBar.x + uiSuperSamplingBar.w &&
		cy >= uiSuperSamplingBar.y - 2 && cy < uiSuperSamplingBar.y + uiSuperSamplingBar.h + 2) {
		tenths = UI_SuperSampling_TenthsAt(cx);
		uiSuperSamplingDrag = qtrue;
	} else {
		select = UI_SelectForKey(key);
		if (select == 0) {
			return qfalse;
		}
		tenths += select;
	}
	trap_Cvar_SetValue("vr_superSampling", VRSS_Value(VRSS_Tenths(tenths / 10.0f)));
	return qtrue;
}

// Each row's cell edges from its last paint (Plain, Alt, the clear glyph) and its window: focus outlives hover, so a
// click must land inside its row and on a cell.
#define VRB_LABEL_W 150
#define VRB_CELL_W 110
static float vrBindCellX[VRBM_ROW_COUNT][3];
static rectDef_t vrBindHit[VRBM_ROW_COUNT];
static const vec4_t vrBindWhite = {1, 1, 1, 1};
static const vec4_t vrBindRed = {1, .31f, .31f, 1};
static const vec4_t vrBindDivider = {.5f, .5f, .5f, .6f};

static void UI_VRBind_SetHit( rectDef_t *hit, float x, float y, float w, float h ) {
	hit->x = x;
	hit->y = y;
	hit->w = w;
	hit->h = h;
}

static qboolean UI_VRBind_Inside( const rectDef_t *hit ) {
	const float x = uiInfo.uiDC.cursorx, y = uiInfo.uiDC.cursory;
	return x >= hit->x && x < hit->x + hit->w && y >= hit->y && y < hit->y + hit->h;
}

// The cell under the pointer in row index; the label counts as Plain.
static int UI_VRBind_ColumnAt( int index ) {
	const float x = uiInfo.uiDC.cursorx;
	if ( x >= vrBindCellX[index][VRBM_COL_CLEAR] - 6 ) {
		return VRBM_COL_CLEAR;
	}
	if ( x >= vrBindCellX[index][VRBM_COL_ALT] - 4 ) {
		return VRBM_COL_ALT;
	}
	return VRBM_COL_PLAIN;
}

static int UI_VRBind_FocusedRow( void ) {
	itemDef_t *item = UI_VR_FocusedItem();
	return item && item->window.ownerDraw == UI_VRBIND ? (int)item->special : -1;
}

static void UI_DrawVRBind( float top, rectDef_t *rect, int index, float scale, vec4_t color, int textStyle ) {
	const vrbmRow_t *row;
	char cell[VRBM_CELL];
	vec4_t fill;
	qboolean focused;
	int c, column;
	float x;

	VRBM_Tick( uiInfo.uiDC.realTime );
	VRG_Tick( uiInfo.uiDC.realTime );
	if ( index < 0 || index >= VRBM_ROW_COUNT ) {
		return;
	}
	row = &vrbmRows[index];
	if ( row->flags & VRBM_HEADER ) {
		uiInfo.uiDC.drawText( rect->x, rect->y, scale, color, row->label, 0, 0, textStyle );
		return;
	}
	vrBindCellX[index][VRBM_COL_PLAIN] = rect->x + VRB_LABEL_W;
	vrBindCellX[index][VRBM_COL_ALT] = rect->x + VRB_LABEL_W + VRB_CELL_W;
	vrBindCellX[index][VRBM_COL_CLEAR] = rect->x + rect->w - 20;
	focused = UI_VRBind_FocusedRow() == index;
	if ( focused && !UI_VR_StickNavActive() && !VRBM_Waiting() && UI_VRBind_Inside( &vrBindHit[index] ) ) {
		c = UI_VRBind_ColumnAt( index );
		if ( c >= 0 ) {
			VRBM_Point( row, c );
		}
	}
	column = vrbm.waiting == index ? ( vrbm.waitingAlt ? VRBM_COL_ALT : VRBM_COL_PLAIN ) : focused ? VRBM_Column( row ) : -1;
	uiInfo.uiDC.drawText( rect->x, rect->y, scale, column >= 0 ? color : (float *)vrBindWhite, row->label, 0, 0, textStyle );
	// one row pitch tall (20 + the 2px gap), so the rows' dividers butt into one line without overlapping
	uiInfo.uiDC.fillRect( vrBindCellX[index][VRBM_COL_ALT] - 6, top - 1, 1, rect->h + 2, vrBindDivider );
	fill[0] = color[0];
	fill[1] = color[1];
	fill[2] = color[2];
	fill[3] = .18f;
	for ( c = VRBM_COL_PLAIN; c <= VRBM_COL_ALT; c++ ) {
		x = vrBindCellX[index][c];
		if ( c == VRBM_COL_ALT && !VRBM_Editable( row, 1 ) ) {
			// centered in the cell's highlight span, and as thin as the divider
			uiInfo.uiDC.fillRect( x - 4 + ( VRB_CELL_W - 8 ) / 2 - 12, top + rect->h / 2, 24, 1, vrBindDivider );
			continue;
		}
		if ( c == column ) {
			uiInfo.uiDC.fillRect( x - 4, top + 1, VRB_CELL_W - 8, rect->h - 2, fill );
		}
		if ( vrbm.waiting == index && c == column ) {
			uiInfo.uiDC.drawText( x, rect->y, scale, color, "...", 0, 0, textStyle );
			continue;
		}
		VRBM_Cell( row, c == VRBM_COL_ALT, cell, sizeof( cell ) );
		UI_VR_GlyphPaint( x, rect->y, scale, c == column ? color : (float *)vrBindWhite, cell, textStyle );
	}
	if ( focused ) {
		if ( column == VRBM_COL_CLEAR ) {
			uiInfo.uiDC.fillRect( vrBindCellX[index][VRBM_COL_CLEAR] - 4, top + 1, 16, rect->h - 2, fill );
		}
		uiInfo.uiDC.drawText( vrBindCellX[index][VRBM_COL_CLEAR], rect->y, scale, (float *)vrBindRed, "X", 0, 0, textStyle );
	}
}

static qboolean UI_VRBind_HandleKey( float *special, int key ) {
	const int index = (int)*special;
	const vrbmRow_t *row;
	int c;

	if ( index < 0 || index >= VRBM_ROW_COUNT || VRBM_Waiting() ) {
		return key == K_MOUSE1 || key == K_ENTER || key == K_KP_ENTER;
	}
	row = &vrbmRows[index];
	switch ( key ) {
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		VRBM_Step( row, -1 );
		return qtrue;
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		VRBM_Step( row, 1 );
		return qtrue;
	case K_MOUSE1:
		if ( !UI_VRBind_Inside( &vrBindHit[index] ) ) {
			return qtrue;
		}
		if ( !UI_VR_StickNavActive() ) {
			c = UI_VRBind_ColumnAt( index );
			if ( c < 0 ) {
				return qtrue;
			}
			VRBM_Point( row, c );
			if ( VRBM_Column( row ) != c ) {
				return qtrue;
			}
		}
		VRBM_Activate( index );
		return qtrue;
	case K_ENTER:
	case K_KP_ENTER:
		VRBM_Activate( index );
		return qtrue;
	default:
		return qfalse;
	}
}

static void UI_DrawVRBindStatus( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	char status[256];
	VRBM_Status( UI_VRBind_FocusedRow(), status, sizeof( status ) );
	UI_VR_GlyphPaint( rect->x, rect->y, scale, color, status, textStyle );
}

void UI_VR_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case UI_REFRESHRATE:
		UI_DrawRefreshRate( &rect, scale, color, textStyle );
		break;
	case UI_FOVEATION:
		UI_DrawFoveation( &rect, scale, color, textStyle );
		break;
	case UI_FOVEATION_STRENGTH:
		UI_DrawFoveationStrength( &rect, scale, color, textStyle );
		break;
	case UI_WEAPONPITCH:
		UI_DrawWeaponPitch( &rect, scale, color, textStyle );
		break;
	case UI_VRSUPERSAMPLING:
		UI_DrawSuperSampling( &rect, special, scale, color, textStyle );
		break;
	case UI_VRBIND:
		if ( (int)special >= 0 && (int)special < VRBM_ROW_COUNT ) {
			UI_VRBind_SetHit( &vrBindHit[(int)special], x, y, w, h );
		}
		UI_DrawVRBind( y, &rect, (int)special, scale, color, textStyle );
		break;
	case UI_VRBIND_STATUS:
		UI_DrawVRBindStatus( &rect, scale, color, textStyle );
		break;
	default:
		break;
	}
}

qboolean UI_VR_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key ) {
	switch ( ownerDraw ) {
	case UI_REFRESHRATE:
		return UI_RefreshRate_HandleKey( flags, special, key );
	case UI_FOVEATION:
		return UI_Foveation_HandleKey( flags, special, key );
	case UI_FOVEATION_STRENGTH:
		return UI_FoveationStrength_HandleKey( flags, special, key );
	case UI_VRSUPERSAMPLING:
		return UI_SuperSampling_HandleKey( flags, special, key );
	case UI_VRBIND:
		return UI_VRBind_HandleKey( special, key );
	default:
		break;
	}
	return qfalse;
}

int UI_VR_OwnerDrawWidth( int ownerDraw, float scale ) {
	if ( ownerDraw == UI_WEAPONPITCH ) {
		return uiInfo.uiDC.textWidth( UI_WeaponPitchText(), scale, 0 );
	}
	return 0;
}

#ifndef Q3_VM
// Capability marker for trusted native-module preflight.
DLLEXPORT int TrinityVRAPI( void ) {
	return ( VR_API_MAJOR << 16 ) | VR_API_MINOR;
}
#endif
