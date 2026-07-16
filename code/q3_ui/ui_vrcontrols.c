/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=======================================================================

VR CONTROLS OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"

#define VR_X_POS		330

// Vertical center of the framel/framer oval interior (640x480 space).
#define VR_FRAME_CENTER_Y	242

#define ID_SCOPE				127
#define ID_TWOHANDED			128
#define ID_DIRECTIONMODE		129
#define ID_SNAPTURN				130
#define ID_SENSITIVITY			131
#define ID_UTURN				132
#define ID_RIGHTHANDED			133
#define ID_SWITCHTHUMBSTICKS	134
#define ID_TRIGGERSENSITIVITY	135
#define ID_WEAPONPITCH			136
#define ID_WEAPONSELECTORMODE	137
#define ID_CONTROLSCHEMA		138
#define ID_WEAPONADJUST			139
#define ID_HOLSTER2D			140
#define ID_AUTOSWITCH			141
#define ID_SENDROLL				142

#define ID_BACK					150

#define	NUM_DIRECTIONMODE		2


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

	menuradiobutton_s	scope;
	menulist_s			twohanded;
	menulist_s			directionmode;
	menulist_s			snapturn;
	menuslider_s		sensitivity;
	menuradiobutton_s	uturn;
	menuradiobutton_s	righthanded;
	menuradiobutton_s	switchthumbsticks;
	menuslider_s		triggersensitivity;
	menuslider_s		weaponpitch;
	menulist_s			weaponselectormode;
	menulist_s			controlschema;
	menuradiobutton_s	weaponadjust;
	menuradiobutton_s	holster2d;
	menuradiobutton_s	autoswitch;
	menuradiobutton_s	sendroll;

	menubitmap_s		back;
} vrcontrols_t;

static vrcontrols_t s_vrcontrols;


static void VRControls_SetMenuItems( void ) {
	s_vrcontrols.scope.curvalue				= trap_Cvar_VariableValue( "vr_weaponScope" ) != 0;
	s_vrcontrols.twohanded.curvalue			= trap_Cvar_VariableValue( "vr_twoHandedWeapons" );
	s_vrcontrols.directionmode.curvalue		= (int)trap_Cvar_VariableValue( "vr_directionMode" ) % NUM_DIRECTIONMODE;
	s_vrcontrols.snapturn.curvalue			= (int)trap_Cvar_VariableValue( "vr_snapturn" ) / 45;
	s_vrcontrols.sensitivity.curvalue		= UI_ClampCvar( 50, 150, trap_Cvar_VariableValue( "sensitivity" ) );
	s_vrcontrols.uturn.curvalue				= trap_Cvar_VariableValue( "vr_uturn" ) != 0;
	s_vrcontrols.righthanded.curvalue		= trap_Cvar_VariableValue( "vr_righthanded" ) != 0;
	s_vrcontrols.switchthumbsticks.curvalue	= trap_Cvar_VariableValue( "vr_switchThumbsticks" ) != 0;
	s_vrcontrols.triggersensitivity.curvalue	= trap_Cvar_VariableValue( "vr_triggerSensitivity" );
	s_vrcontrols.weaponpitch.curvalue		= trap_Cvar_VariableValue( "vr_weaponPitch" ) + 25;
	s_vrcontrols.weaponselectormode.curvalue	= (int)trap_Cvar_VariableValue( "vr_weaponSelectorMode" ) % 2;
	s_vrcontrols.controlschema.curvalue		= (int)trap_Cvar_VariableValue( "vr_controlSchema" ) % 3;
	s_vrcontrols.weaponadjust.curvalue		= trap_Cvar_VariableValue( "vr_weaponAdjust" ) != 0;
	s_vrcontrols.holster2d.curvalue			= trap_Cvar_VariableValue( "cg_weaponSelectorSimple2DIcons" ) != 0;
	s_vrcontrols.autoswitch.curvalue		= trap_Cvar_VariableValue( "cg_autoswitch" ) != 0;
	s_vrcontrols.sendroll.curvalue			= trap_Cvar_VariableValue( "vr_sendRollToServer" ) != 0;
}


static void VRControls_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_SCOPE:
			trap_Cvar_SetValue( "vr_weaponScope", s_vrcontrols.scope.curvalue );
			break;

		case ID_TWOHANDED:
			trap_Cvar_SetValue( "vr_twoHandedWeapons", s_vrcontrols.twohanded.curvalue );
			break;

		case ID_DIRECTIONMODE:
			trap_Cvar_SetValue( "vr_directionMode", s_vrcontrols.directionmode.curvalue );
			break;

		case ID_SNAPTURN:
			trap_Cvar_SetValue( "vr_snapturn", s_vrcontrols.snapturn.curvalue * 45 );
			break;

		case ID_SENSITIVITY:
			trap_Cvar_SetValue( "sensitivity", s_vrcontrols.sensitivity.curvalue );
			break;

		case ID_UTURN:
			{
				if (s_vrcontrols.uturn.curvalue) {
					if (s_vrcontrols.controlschema.curvalue == 1) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "uturn");
					} else {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "uturn");
					}
				} else {
					if (s_vrcontrols.controlschema.curvalue == 1) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "blank");
					} else if (s_vrcontrols.controlschema.curvalue == 2) {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "weapprev");
					} else {
						trap_Cvar_Set("vr_button_map_RTHUMBBACK", "blank");
					}
				}
			}
			trap_Cvar_SetValue( "vr_uturn", s_vrcontrols.uturn.curvalue );
			break;

		case ID_RIGHTHANDED:
			trap_Cvar_SetValue( "vr_righthanded", s_vrcontrols.righthanded.curvalue );
			break;

		case ID_SWITCHTHUMBSTICKS:
			{
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
			trap_Cvar_SetValue( "vr_switchThumbsticks", s_vrcontrols.switchthumbsticks.curvalue );
			break;

		case ID_TRIGGERSENSITIVITY:
			trap_Cvar_SetValue( "vr_triggerSensitivity", s_vrcontrols.triggersensitivity.curvalue );
			break;

		case ID_WEAPONPITCH:
			trap_Cvar_SetValue( "vr_weaponPitch", s_vrcontrols.weaponpitch.curvalue - 25 );
			break;

		case ID_WEAPONSELECTORMODE:
			trap_Cvar_SetValue( "vr_weaponSelectorMode", s_vrcontrols.weaponselectormode.curvalue );
			break;

		case ID_CONTROLSCHEMA:
			{
				switch (s_vrcontrols.controlschema.curvalue)
				{
					case 0: // Default schema (weapon wheel on grip)
						trap_Cvar_Set("vr_button_map_RTHUMBLEFT", "turnleft"); // turn left
						trap_Cvar_Set("vr_button_map_RTHUMBRIGHT", "turnright"); // turn right
						trap_Cvar_Set("vr_button_map_RTHUMBFORWARD", ""); // unmapped
						if (s_vrcontrols.uturn.curvalue) {
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
						if (s_vrcontrols.uturn.curvalue) {
							trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "uturn");
						} else {
							trap_Cvar_Set("vr_button_map_RTHUMBBACK_ALT", "blank");
						}
						break;
					default: // Weapon wheel disabled - only prev/next weapon switch is active
						trap_Cvar_Set("vr_button_map_RTHUMBLEFT", "turnleft"); // turn left
						trap_Cvar_Set("vr_button_map_RTHUMBRIGHT", "turnright"); // turn right
						trap_Cvar_Set("vr_button_map_RTHUMBFORWARD", "weapnext"); // next weapon
						if (s_vrcontrols.uturn.curvalue) {
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
			trap_Cvar_SetValue( "vr_controlSchema", s_vrcontrols.controlschema.curvalue );
			break;

		case ID_WEAPONADJUST:
			trap_Cvar_SetValue( "vr_weaponAdjust", s_vrcontrols.weaponadjust.curvalue );
			break;

		case ID_HOLSTER2D:
			trap_Cvar_SetValue( "cg_weaponSelectorSimple2DIcons", s_vrcontrols.holster2d.curvalue );
			break;

		case ID_AUTOSWITCH:
			trap_Cvar_SetValue( "cg_autoswitch", s_vrcontrols.autoswitch.curvalue );
			break;

		case ID_SENDROLL:
			trap_Cvar_SetValue( "vr_sendRollToServer", s_vrcontrols.sendroll.curvalue );
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}

	if ((int)trap_Cvar_VariableValue( "vr_snapturn" ) == 0)
	{
		s_vrcontrols.sensitivity.generic.flags &= ~QMF_GRAYED;
	}
	else
	{
		s_vrcontrols.sensitivity.generic.flags |= QMF_GRAYED;
	}
}

static void VRControls_SensitivityStatusBar( void *self )
{
	const int currentValue = (int)UI_ClampCvar( 50, 150, trap_Cvar_VariableValue( "sensitivity" ) );

	char buf[128] = { 0 };
	Com_sprintf( buf, sizeof(buf), "Current value: %d (default: 100)", currentValue );

	UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.75, buf, UI_SMALLFONT|UI_CENTER, colorWhite );
}

static void VRControls_WeaponAdjustStatusBar( void *self ) {
	UI_DrawString( SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Hold both grips for 1s to enter weapon adjustment mode", UI_SMALLFONT|UI_CENTER, colorWhite );
}

static void VRControls_MenuInit( void ) {
	int				y;

	static const char *s_directionmode[] =
			{
					"HMD (Default)",
					"Off-hand Controller",
					NULL
			};

	static const char *s_snapturn[] =
			{
					"Smooth Turning",
					"45 Degrees",
					"90 Degrees",
					NULL
			};

	static const char *s_weaponselectormode[] =
			{
					"VR Style / Pointing",
					"Classic / Thumbstick",
					NULL
			};

	static const char *s_controlschema[] =
			{
					"Weapon Wheel on Grip",
					"Weapon Wheel on Stick",
					"Weapon Wheel Disabled",
					NULL
			};

	static const char *s_twohandedmode[] =
			{
					"Disabled",
					"Enabled (Basic)",
					"Enabled (VR Gun Stock)",
					NULL
			};

	memset( &s_vrcontrols, 0, sizeof(vrcontrols_t) );

	UI_VRControls_Cache();

	s_vrcontrols.menu.wrapAround = qtrue;
	s_vrcontrols.menu.fullscreen = qtrue;

	s_vrcontrols.banner.generic.type	= MTYPE_BTEXT;
	s_vrcontrols.banner.generic.x		= 320;
	s_vrcontrols.banner.generic.y		= 16;
	s_vrcontrols.banner.string			= "CONTROLS";
	s_vrcontrols.banner.color			= color_white;
	s_vrcontrols.banner.style			= UI_CENTER;

	s_vrcontrols.framel.generic.type	= MTYPE_BITMAP;
	s_vrcontrols.framel.generic.name	= ART_FRAMEL;
	s_vrcontrols.framel.generic.flags	= QMF_INACTIVE;
	s_vrcontrols.framel.generic.x		= 0;
	s_vrcontrols.framel.generic.y		= 78;
	s_vrcontrols.framel.width			= 256;
	s_vrcontrols.framel.height			= 329;

	s_vrcontrols.framer.generic.type	= MTYPE_BITMAP;
	s_vrcontrols.framer.generic.name	= ART_FRAMER;
	s_vrcontrols.framer.generic.flags	= QMF_INACTIVE;
	s_vrcontrols.framer.generic.x		= 376;
	s_vrcontrols.framer.generic.y		= 76;
	s_vrcontrols.framer.width			= 256;
	s_vrcontrols.framer.height			= 334;

	// Center the 16-row block (small-font rows) in the frame interior. With
	// this many rows the block nearly fills the frame, so centering just
	// yields minimal, equal top/bottom margins.
	y = VR_FRAME_CENTER_Y - ( 15 * (BIGCHAR_HEIGHT+2) + SMALLCHAR_HEIGHT ) / 2;
	s_vrcontrols.scope.generic.type			= MTYPE_RADIOBUTTON;
	s_vrcontrols.scope.generic.name			= "Railgun Scope:";
	s_vrcontrols.scope.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.scope.generic.callback		= VRControls_MenuEvent;
	s_vrcontrols.scope.generic.id			= ID_SCOPE;
	s_vrcontrols.scope.generic.x			= VR_X_POS;
	s_vrcontrols.scope.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.twohanded.generic.type		= MTYPE_SPINCONTROL;
	s_vrcontrols.twohanded.generic.name		= "Two-Handed Weapons:";
	s_vrcontrols.twohanded.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.twohanded.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.twohanded.generic.id		= ID_TWOHANDED;
	s_vrcontrols.twohanded.generic.x		= VR_X_POS;
	s_vrcontrols.twohanded.generic.y		= y;
	s_vrcontrols.twohanded.itemnames		= s_twohandedmode;
	s_vrcontrols.twohanded.numitems			= 3;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.directionmode.generic.type		= MTYPE_SPINCONTROL;
	s_vrcontrols.directionmode.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.directionmode.generic.x		= VR_X_POS;
	s_vrcontrols.directionmode.generic.y		= y;
	s_vrcontrols.directionmode.generic.name		= "Direction Mode:";
	s_vrcontrols.directionmode.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.directionmode.generic.id		= ID_DIRECTIONMODE;
	s_vrcontrols.directionmode.itemnames		= s_directionmode;
	s_vrcontrols.directionmode.numitems			= NUM_DIRECTIONMODE;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.snapturn.generic.type		= MTYPE_SPINCONTROL;
	s_vrcontrols.snapturn.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.snapturn.generic.x			= VR_X_POS;
	s_vrcontrols.snapturn.generic.y			= y;
	s_vrcontrols.snapturn.generic.name		= "Turning Mode:";
	s_vrcontrols.snapturn.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.snapturn.generic.id		= ID_SNAPTURN;
	s_vrcontrols.snapturn.itemnames			= s_snapturn;
	s_vrcontrols.snapturn.numitems			= 3;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.sensitivity.generic.type		= MTYPE_SLIDER;
	s_vrcontrols.sensitivity.generic.x			= VR_X_POS;
	s_vrcontrols.sensitivity.generic.y			= y;
	s_vrcontrols.sensitivity.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.sensitivity.generic.name		= "Smooth turn speed:";
	s_vrcontrols.sensitivity.generic.id			= ID_SENSITIVITY;
	s_vrcontrols.sensitivity.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.sensitivity.minvalue			= 50;
	s_vrcontrols.sensitivity.maxvalue			= 150;
	s_vrcontrols.sensitivity.generic.statusbar	= VRControls_SensitivityStatusBar;

	if ((int)trap_Cvar_VariableValue( "vr_snapturn" ) > 0)
	{
		s_vrcontrols.sensitivity.generic.flags |= QMF_GRAYED;
	}

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.uturn.generic.type			= MTYPE_RADIOBUTTON;
	s_vrcontrols.uturn.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.uturn.generic.x			= VR_X_POS;
	s_vrcontrols.uturn.generic.y			= y;
	s_vrcontrols.uturn.generic.name			= "Quick U-Turn:";
	s_vrcontrols.uturn.generic.callback		= VRControls_MenuEvent;
	s_vrcontrols.uturn.generic.id			= ID_UTURN;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.righthanded.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.righthanded.generic.name		= "Right-Handed:";
	s_vrcontrols.righthanded.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.righthanded.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.righthanded.generic.id			= ID_RIGHTHANDED;
	s_vrcontrols.righthanded.generic.x			= VR_X_POS;
	s_vrcontrols.righthanded.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.switchthumbsticks.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.switchthumbsticks.generic.name		= "Switch Thumbsticks:";
	s_vrcontrols.switchthumbsticks.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.switchthumbsticks.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.switchthumbsticks.generic.id		= ID_SWITCHTHUMBSTICKS;
	s_vrcontrols.switchthumbsticks.generic.x		= VR_X_POS;
	s_vrcontrols.switchthumbsticks.generic.y		= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.triggersensitivity.generic.type		= MTYPE_SLIDER;
	s_vrcontrols.triggersensitivity.generic.x			= VR_X_POS;
	s_vrcontrols.triggersensitivity.generic.y			= y;
	s_vrcontrols.triggersensitivity.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.triggersensitivity.generic.name		= "Trigger Sensitivity:";
	s_vrcontrols.triggersensitivity.generic.id			= ID_TRIGGERSENSITIVITY;
	s_vrcontrols.triggersensitivity.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.triggersensitivity.minvalue			= 0.1f;
	s_vrcontrols.triggersensitivity.maxvalue			= 0.9f;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.weaponpitch.generic.type		= MTYPE_SLIDER;
	s_vrcontrols.weaponpitch.generic.x			= VR_X_POS;
	s_vrcontrols.weaponpitch.generic.y			= y;
	s_vrcontrols.weaponpitch.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.weaponpitch.generic.name		= "Weapon Pitch:";
	s_vrcontrols.weaponpitch.generic.id			= ID_WEAPONPITCH;
	s_vrcontrols.weaponpitch.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.weaponpitch.minvalue			= 0;
	s_vrcontrols.weaponpitch.maxvalue			= 30;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.weaponselectormode.generic.type		= MTYPE_SPINCONTROL;
	s_vrcontrols.weaponselectormode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.weaponselectormode.generic.x			= VR_X_POS;
	s_vrcontrols.weaponselectormode.generic.y			= y;
	s_vrcontrols.weaponselectormode.generic.name		= "Weapon Wheel Mode:";
	s_vrcontrols.weaponselectormode.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.weaponselectormode.generic.id			= ID_WEAPONSELECTORMODE;
	s_vrcontrols.weaponselectormode.itemnames			= s_weaponselectormode;
	s_vrcontrols.weaponselectormode.numitems			= 2;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.controlschema.generic.type		= MTYPE_SPINCONTROL;
	s_vrcontrols.controlschema.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.controlschema.generic.x		= VR_X_POS;
	s_vrcontrols.controlschema.generic.y		= y;
	s_vrcontrols.controlschema.generic.name		= "Control Schema:";
	s_vrcontrols.controlschema.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.controlschema.generic.id		= ID_CONTROLSCHEMA;
	s_vrcontrols.controlschema.itemnames		= s_controlschema;
	s_vrcontrols.controlschema.numitems			= 3;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.weaponadjust.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.weaponadjust.generic.name		= "Weapon Adjustment:";
	s_vrcontrols.weaponadjust.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.weaponadjust.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.weaponadjust.generic.id		= ID_WEAPONADJUST;
	s_vrcontrols.weaponadjust.generic.x			= VR_X_POS;
	s_vrcontrols.weaponadjust.generic.y			= y;
	s_vrcontrols.weaponadjust.generic.statusbar	= VRControls_WeaponAdjustStatusBar;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.holster2d.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.holster2d.generic.name		= "Simple Items (Weapon Wheel):";
	s_vrcontrols.holster2d.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.holster2d.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.holster2d.generic.id		= ID_HOLSTER2D;
	s_vrcontrols.holster2d.generic.x		= VR_X_POS;
	s_vrcontrols.holster2d.generic.y		= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.autoswitch.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.autoswitch.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.autoswitch.generic.name		= "Autoswitch Weapons:";
	s_vrcontrols.autoswitch.generic.id			= ID_AUTOSWITCH;
	s_vrcontrols.autoswitch.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.autoswitch.generic.x			= VR_X_POS;
	s_vrcontrols.autoswitch.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcontrols.sendroll.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcontrols.sendroll.generic.name		= "Send Roll Angle:";
	s_vrcontrols.sendroll.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcontrols.sendroll.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.sendroll.generic.id		= ID_SENDROLL;
	s_vrcontrols.sendroll.generic.x			= VR_X_POS;
	s_vrcontrols.sendroll.generic.y			= y;

	s_vrcontrols.back.generic.type		= MTYPE_BITMAP;
	s_vrcontrols.back.generic.name		= ART_BACK0;
	s_vrcontrols.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vrcontrols.back.generic.callback	= VRControls_MenuEvent;
	s_vrcontrols.back.generic.id		= ID_BACK;
	s_vrcontrols.back.generic.x			= 0;
	s_vrcontrols.back.generic.y			= 480-64;
	s_vrcontrols.back.width				= 128;
	s_vrcontrols.back.height			= 64;
	s_vrcontrols.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.banner );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.framel );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.framer );

	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.scope );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.twohanded );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.directionmode );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.snapturn );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.sensitivity );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.uturn );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.righthanded );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.switchthumbsticks );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.triggersensitivity );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.weaponpitch );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.weaponselectormode );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.controlschema );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.weaponadjust );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.holster2d );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.autoswitch );
	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.sendroll );

	Menu_AddItem( &s_vrcontrols.menu, &s_vrcontrols.back );

	VRControls_SetMenuItems();
}


/*
===============
UI_VRControls_Cache
===============
*/
void UI_VRControls_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_VRControlsMenu
===============
*/
void UI_VRControlsMenu( void ) {
	VRControls_MenuInit();
	UI_PushMenu( &s_vrcontrols.menu );
}
