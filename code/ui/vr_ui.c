// VR UI registration and input helpers.
#include "ui_local.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"
#include "../game/vr_bindmenu.h"
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

static void UI_VRBind_KeyName( int key, char *buf, int size ) {
	if ( !trap_GetValue( buf, size, va( "vr_keyname %i", vrKeyFirst + key ) ) )
		Q_strncpyz( buf, "???", size );
}

static void UI_VRBind_CancelName( char *buf, int size ) {
	if ( !trap_GetValue( buf, size, "vr_menu_cancel_button" ) )
		Q_strncpyz( buf, "Menu", size );
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
									UI_VRBind_KeyName, UI_VRBind_CancelName, UI_VRBind_Capture, UI_VRBind_Defaults};

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
		trap_Cvar_Set( "ui_vrBindAlt", "0" );
	}
	vr->menuYawLocked = qfalse;
	vr->menuCursorActive = vrActive;
}

// Menus gate their VR rows on these with cvarTest; a flatscreen engine reads 0.
void UI_VR_Init( void ) {
	trap_Cvar_Set( "ui_vrModeSwitchAvailable", UI_VR_CanSwitchMode() ? "1" : "0" );
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
	return vrActive && vr->pointerMode == VR_POINTER_STICK;
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

First-chance key routing: while a bindings row waits for a button, every key
belongs to the bindings model; closing the menu ends the wait. Stick
navigation steps the focused slider.
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

// Where each row's clear glyph starts, from its last paint; a click right of it clears instead of binding.
static float vrBindGlyphX[VRBM_ROW_COUNT];
// Each row's and the Alt switch's window, from their last paint: focus outlives hover, so clicks must land inside.
static rectDef_t vrBindHit[VRBM_ROW_COUNT];
static rectDef_t vrBindAltHit;

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

static qboolean UI_VRBind_IsArrow( int c ) {
	return c == 134 || c == 135 || c == 136 || c == 141;
}

// drawText, with the arrow bytes drawn from the character sheet as the on-screen keyboard draws them: the font pages
// have no glyphs there.
static void UI_VRBind_Paint( float x, float y, float scale, vec4_t color, const char *text, int style ) {
	static qhandle_t charset;
	char run[256];
	const float size = uiInfo.uiDC.textHeight( "A", scale, 0 ) * 1.4f;
	int n = 0;

	if ( !charset ) {
		charset = trap_R_RegisterShaderNoMip( "gfx/2d/bigchars" );
	}
	for ( ;; text++ ) {
		const int c = *text & 255;
		if ( c && !UI_VRBind_IsArrow( c ) && n < (int)sizeof( run ) - 1 ) {
			run[n++] = (char)c;
			continue;
		}
		run[n] = '\0';
		if ( n ) {
			uiInfo.uiDC.drawText( x, y, scale, color, run, 0, 0, style );
			x += uiInfo.uiDC.textWidth( run, scale, 0 );
			n = 0;
		}
		if ( !c ) {
			break;
		}
		if ( UI_VRBind_IsArrow( c ) ) {
			float ax = x, ay = y - size * 0.85f, aw = size, ah = size;
			const float s = ( c & 15 ) * 0.0625f, t = ( c >> 4 ) * 0.0625f;
			UI_AdjustFrom640( &ax, &ay, &aw, &ah );
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( ax, ay, aw, ah, s, t, s + 0.0625f, t + 0.0625f, charset );
			trap_R_SetColor( NULL );
			x += size;
		}
	}
}

static void UI_DrawVRBind( rectDef_t *rect, int index, float scale, vec4_t color, int textStyle ) {
	const vrbmRow_t *row;
	char names[128];

	VRBM_Tick( uiInfo.uiDC.realTime );
	if ( index < 0 || index >= VRBM_ROW_COUNT ) {
		return;
	}
	row = &vrbmRows[index];
	if ( row->flags & VRBM_HEADER ) {
		uiInfo.uiDC.drawText( rect->x, rect->y, scale, color, row->label, 0, 0, textStyle );
		return;
	}
	if ( !VRBM_Editable( row, vrbm.altView ) ) {
		VRBM_Names( row, 0, names, sizeof( names ) );
		uiInfo.uiDC.drawText( rect->x, rect->y, scale, color, row->label, 0, 0, textStyle );
		UI_VRBind_Paint( rect->x + 170, rect->y, scale, color, names, textStyle );
		return;
	}
	VRBM_Names( row, vrbm.altView, names, sizeof( names ) );
	uiInfo.uiDC.drawText( rect->x, rect->y, scale, color, row->label, 0, 0, textStyle );
	UI_VRBind_Paint( rect->x + 170, rect->y, scale, color, vrbm.waiting == index ? "..." : names, textStyle );
	vrBindGlyphX[index] = rect->x + rect->w - 20;
	uiInfo.uiDC.drawText( vrBindGlyphX[index], rect->y, scale, color, "X", 0, 0, textStyle );
}

static qboolean UI_VRBind_HandleKey( float *special, int key ) {
	const int index = (int)*special;

	if ( key != K_MOUSE1 && key != K_ENTER && key != K_KP_ENTER ) {
		return qfalse;
	}
	if ( index < 0 || index >= VRBM_ROW_COUNT || VRBM_Waiting() || !VRBM_Editable( &vrbmRows[index], vrbm.altView ) ) {
		return qtrue;
	}
	if ( key == K_MOUSE1 && !UI_VRBind_Inside( &vrBindHit[index] ) ) {
		return qtrue;
	}
	if ( key == K_MOUSE1 && !UI_VR_StickNavActive() && uiInfo.uiDC.cursorx >= vrBindGlyphX[index] ) {
		VRBM_Clear( &vrbmRows[index], vrbm.altView );
	} else {
		VRBM_Start( index );
	}
	return qtrue;
}

static void UI_DrawVRBindStatus( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	char status[256];
	VRBM_Status( status, sizeof( status ) );
	UI_VRBind_Paint( rect->x, rect->y, scale, color, status, textStyle );
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
		UI_DrawVRBind( &rect, (int)special, scale, color, textStyle );
		break;
	case UI_VRBIND_ALT:
		UI_VRBind_SetHit( &vrBindAltHit, x, y, w, h );
		uiInfo.uiDC.drawText( rect.x, rect.y, scale, color, vrbm.altView ? "Alt held: Yes" : "Alt held: No", 0, 0, textStyle );
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
	case UI_VRBIND_ALT:
		if ( ( key == K_MOUSE1 && UI_VRBind_Inside( &vrBindAltHit ) ) || key == K_ENTER || key == K_KP_ENTER ) {
			vrbm.altView = !vrbm.altView;
			trap_Cvar_Set( "ui_vrBindAlt", vrbm.altView ? "1" : "0" );
			return qtrue;
		}
		break;
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
