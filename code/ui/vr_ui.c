// VR API bootstrap for the Team Arena UI module. Name-resolves the extension
// traps and registers the vr_shared_t mirror so the UI runs VR-aware under the
// VR engine, and degrades to normal mouse mode on a flatscreen host. Mirrors the
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
void	(*trap_VR_RegisterState)( void *state, int stateSize, int apiVersion );
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
	trap_VR_RegisterState( &vr_state, sizeof( vr_state ), VR_API_MAJOR );
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
UI_VR_FillScreen

Whole-framebuffer fill for full-bleed screens (connect/loading).
Deliberately bypasses the 640x480 transform: in VR the framebuffer
must be covered edge-to-edge; on flatscreen this matches the stock
full-window stretch.
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
dormant mirror (flatscreen host) or an unrecognized vr_platform value both
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
	manifest = ( platform == VRP_QUEST ) ? "ui/vrmenus_quest.txt" : "ui/vrmenus_pc.txt";

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
	} else if ( Q_stricmp( name, "vr_uturn" ) == 0 ) {
		if ( UI_VR_Platform() != VRP_NONE ) {
			int controlSchema = (int)trap_Cvar_VariableValue( "vr_controlSchema" ) % 3;
			if (val) {
				if (controlSchema == 1) {
					trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "uturn");
				} else {
					trap_Cvar_Set("vr_button_map_RTHUMBBACK", "uturn");
				}
			} else {
				if (controlSchema == 1) {
					trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "blank");
				} else if (controlSchema == 2) {
					trap_Cvar_Set("vr_button_map_RTHUMBBACK", "weapprev");
				} else {
					trap_Cvar_Set("vr_button_map_RTHUMBBACK", "blank");
				}
			}
		}
		return qtrue;
	} else if ( Q_stricmp( name, "vr_switchThumbsticks" ) == 0 ) {
		if ( UI_VR_Platform() != VRP_NONE ) {
			// Swap-in-place: exchange the current A<->X and B<->Y button
			// bindings, preserving user overrides. Involutive: toggling
			// twice restores the original mapping.
			char a[64], b[64], x[64], yb[64];
			trap_Cvar_VariableStringBuffer( "vr_button_map_A", a, sizeof(a) );
			trap_Cvar_VariableStringBuffer( "vr_button_map_B", b, sizeof(b) );
			trap_Cvar_VariableStringBuffer( "vr_button_map_X", x, sizeof(x) );
			trap_Cvar_VariableStringBuffer( "vr_button_map_Y", yb, sizeof(yb) );
			trap_Cvar_Set( "vr_button_map_A", x );
			trap_Cvar_Set( "vr_button_map_X", a );
			trap_Cvar_Set( "vr_button_map_B", yb );
			trap_Cvar_Set( "vr_button_map_Y", b );
		}
		return qtrue;
	} else if ( Q_stricmp( name, "vr_controlSchema" ) == 0 ) {
		if ( UI_VR_Platform() != VRP_NONE ) {
			qboolean uturn = trap_Cvar_VariableValue( "vr_uturn" ) != 0;
			switch (val)
			{
				case 0: // Default schema (weapon wheel on grip)
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT", "turnleft"); // turn left
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT", "turnright"); // turn right
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD", ""); // unmapped
					if (uturn) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "uturn"); // u-turn
					} else {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", ""); // unmapped
					}
					trap_Cvar_Set("vr_button_map_PRIMARYGRIP", "+weapon_select"); // weapon selector
					trap_Cvar_Set("vr_button_map_PRIMARYTHUMBSTICK", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT_ALT", ""); // unmapped
					break;
				case 1: // Weapon wheel on thumbstick - all directions as weapon select (useful for HMD wheel)
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD", "+weapon_select");
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT", "+weapon_select");
					trap_Cvar_Set("vr_button_map_RTHUMBBACK", "+weapon_select");
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT", "+weapon_select");
					trap_Cvar_Set("vr_button_map_PRIMARYTHUMBSTICK", "+weapon_select");
					trap_Cvar_Set("vr_button_map_PRIMARYGRIP", "+alt"); // switch to alt layout
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT_ALT", "turnleft"); // turn left
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT_ALT", "turnright"); // turn right
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD_ALT", "blank");
					if (uturn) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "uturn");
					} else {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "blank");
					}
					break;
				default: // Weapon wheel disabled - only prev/next weapon switch is active
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT", "turnleft"); // turn left
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT", "turnright"); // turn right
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD", "weapnext"); // next weapon
					if (uturn) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "uturn"); // u-turn
					} else {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "weapprev"); // previous weapon
					}
					trap_Cvar_Set("vr_button_map_PRIMARYGRIP", "+alt"); // switch to alt layout
					trap_Cvar_Set("vr_button_map_PRIMARYTHUMBSTICK", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBFORWARD_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBRIGHT_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", ""); // unmapped
					trap_Cvar_Set("vr_button_map_RTHUMBLEFT_ALT", ""); // unmapped
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

VR desktop-mirror script commands, dispatched from UI_RunMenuScript's
else-if chain. Returns qtrue when name matched one of the vrMirror*
commands (regardless of whether the platform gate let the body run), so
the caller's chain continues exactly as before for unmatched names.
===============
*/
qboolean UI_VR_RunMenuScript( const char *name ) {
	if ( Q_stricmp( name, "vrMirrorSetup" ) == 0 ) {
		// Stage the restart-class desktop-mirror values into ui_ cvars.
		// Mode: 0=Off, 1=Windowed, 2=Fullscreen from vr_desktopMode + r_fullscreen.
		if ( UI_VR_Platform() != VRP_NONE ) {
			int mirror, fullscreen, w, h;
			mirror = (int)trap_Cvar_VariableValue( "vr_desktopMode" );
			fullscreen = (int)trap_Cvar_VariableValue( "r_fullscreen" );
			if ( mirror == 0 ) {
				trap_Cvar_Set( "ui_vrDesktopMode", "0" );
			} else if ( fullscreen == 0 ) {
				trap_Cvar_Set( "ui_vrDesktopMode", "1" );
			} else {
				trap_Cvar_Set( "ui_vrDesktopMode", "2" );
			}
			w = (int)trap_Cvar_VariableValue( "r_customdesktopwidth" );
			h = (int)trap_Cvar_VariableValue( "r_customdesktopheight" );
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
		// Write the staged desktop-mirror values and vid_restart (R5
		// staged-Apply pattern). No-op when nothing changed.
		if ( UI_VR_Platform() != VRP_NONE ) {
			int stagedMode, curMode, mirror, fullscreen, dirty;
			char res[32];
			char *xp;
			dirty = 0;
			mirror = (int)trap_Cvar_VariableValue( "vr_desktopMode" );
			fullscreen = (int)trap_Cvar_VariableValue( "r_fullscreen" );
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
					trap_Cvar_SetValue( "vr_desktopMode", 0 );
				} else if ( stagedMode == 1 ) {
					trap_Cvar_SetValue( "vr_desktopMode", 1 );
					trap_Cvar_SetValue( "r_fullscreen", 0 );
				} else {
					trap_Cvar_SetValue( "vr_desktopMode", 1 );
					trap_Cvar_SetValue( "r_fullscreen", 1 );
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
				curW = (int)trap_Cvar_VariableValue( "r_customdesktopwidth" );
				curH = (int)trap_Cvar_VariableValue( "r_customdesktopheight" );
				if ( stagedW > 0 && stagedH > 0 && ( stagedW != curW || stagedH != curH ) ) {
					trap_Cvar_Set( "r_customdesktopwidth", va( "%d", stagedW ) );
					trap_Cvar_Set( "r_customdesktopheight", va( "%d", stagedH ) );
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
