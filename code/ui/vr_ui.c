// VR UI registration and input helpers.
#include "ui_local.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"
#include "../game/vr_bindmenu.h"

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

void UI_VR_Init( void ) {
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
UI_VR_UpdateSettingsCvar

VR settings-menu cvar handlers, dispatched from UI_Update's else-if chain.
Returns qtrue when name matched one of the VR settings cvars (regardless of
whether the platform gate let the body run), so the caller's chain continues
exactly as before for unmatched names.
===============
*/
qboolean UI_VR_UpdateSettingsCvar( const char *name, int val ) {
	if ( Q_stricmp( name, "vr_hudDrawStatus" ) == 0 ) {
		if ( UI_VR_Platform() != VRP_NONE ) {
			switch (val) {
				case 2:
					trap_Cvar_SetValue( "cg_draw3dIcons", 0 );
					break;
				default:
					trap_Cvar_SetValue( "cg_draw3dIcons", 1 );
					break;
			}
		}
		return qtrue;
	}
	return qfalse;
}

/*
===============
UI_VR_RunMenuScript
===============
*/
qboolean UI_VR_RunMenuScript( const char *name ) {
	if ( Q_stricmp( name, "vrBindCancel" ) == 0 ) {
		VRBM_Cancel();
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

#ifndef Q3_VM
// Capability marker for trusted native-module preflight.
DLLEXPORT int TrinityVRAPI( void ) {
	return ( VR_API_MAJOR << 16 ) | VR_API_MINOR;
}
#endif
