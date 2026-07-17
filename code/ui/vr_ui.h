// Vendored VR API - Team Arena UI module hooks (vr_ui). Bootstrap, menu-cursor
// lifecycle, haptic wrapper, and virtual-keyboard traps. Dormancy-safe: on a
// flatscreen engine UI_VR_Init leaves the mirror zeroed and vrActive false;
// the haptic/vkeyboard wrappers no-op.
#ifndef VR_UI_H
#define VR_UI_H

// No includes beyond vr_platform.h: this header is included only from the tail
// of ui_local.h, after q_shared.h/bg_public.h/vr_shared.h are already visible
// there (bg_public.h has no include guard, so re-including it here would be
// unsafe if this header were ever pulled in a second time). vr_platform.h and
// the q_shared.h it pulls are both guarded, so they are safe.
#include "../game/vr_platform.h"

vrPlatform_t UI_VR_Platform( void );

void UI_VR_Init( void );
void UI_VR_Shutdown( void );
void UI_VRHaptic( const char *description, int position, int channel, int intensity, float yaw, float height );
void UI_VKeyboardShow( void );
void UI_VKeyboardHide( void );
qboolean UI_VKeyboardIsActive( void );
qboolean UI_VKeyboardHandleKey( int key );
qboolean UI_VR_StickNavActive( void );
qboolean UI_VR_KeyEvent( int key );
qboolean UI_VR_CursorOverride( float *x, float *y );
qboolean UI_VR_HideCursor( void );
void UI_VR_OnMenuMove( void );
void UI_VR_FillScreen( qhandle_t shader );
void UI_VR_LoadMenus( void );
qboolean UI_VR_UpdateSettingsCvar( const char *name, int val );
qboolean UI_VR_RunMenuScript( const char *name );

#endif
