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

VR OPTIONS HUB MENU

=======================================================================
*/


#include "ui_local.h"


#define VR_MENU_VERTICAL_SPACING	34

// Vertical center of the framel/framer oval interior (640x480 space).
// Menu row blocks are centered on this so they sit inside the frame art.
#define VR_FRAME_CENTER_Y			242

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_COMFORT			10
#define ID_CONTROLS			11
#define ID_HUD				12
#define ID_MIRROR			13
#define ID_CONSOLE			14
#define ID_BACK				15


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		comfort;
	menutext_s		controls;
	menutext_s		hud;
	menutext_s		mirror;
	menutext_s		console;

	menubitmap_s	back;
} vroptions_t;

static vroptions_t	s_vroptions;


static void VROptions_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_COMFORT:
		UI_VRComfortMenu();
		break;

	case ID_CONTROLS:
		UI_VRControlsMenu();
		break;

	case ID_HUD:
		UI_VRHudMenu();
		break;

	case ID_MIRROR:
		UI_VRMirrorMenu();
		break;

	case ID_CONSOLE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "toggleconsole\n" );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


static void VROptions_MenuInit( void ) {
	int		y;
	qboolean isPC;

	isPC = ( UI_VR_Platform() == VRP_PC );

	UI_VROptions_Cache();

	memset( &s_vroptions, 0, sizeof(vroptions_t) );
	s_vroptions.menu.wrapAround = qtrue;
	s_vroptions.menu.fullscreen = qtrue;

	s_vroptions.banner.generic.type		= MTYPE_BTEXT;
	s_vroptions.banner.generic.x		= 320;
	s_vroptions.banner.generic.y		= 16;
	s_vroptions.banner.string			= "VR OPTIONS";
	s_vroptions.banner.color			= color_white;
	s_vroptions.banner.style			= UI_CENTER;

	s_vroptions.framel.generic.type		= MTYPE_BITMAP;
	s_vroptions.framel.generic.name		= ART_FRAMEL;
	s_vroptions.framel.generic.flags	= QMF_INACTIVE;
	s_vroptions.framel.generic.x		= 0;
	s_vroptions.framel.generic.y		= 78;
	s_vroptions.framel.width			= 256;
	s_vroptions.framel.height			= 329;

	s_vroptions.framer.generic.type		= MTYPE_BITMAP;
	s_vroptions.framer.generic.name		= ART_FRAMER;
	s_vroptions.framer.generic.flags	= QMF_INACTIVE;
	s_vroptions.framer.generic.x		= 376;
	s_vroptions.framer.generic.y		= 76;
	s_vroptions.framer.width			= 256;
	s_vroptions.framer.height			= 334;

	// Center the spoke block (rows use PROP text, PROP_HEIGHT tall) in the
	// frame: PC has 5 rows (adds DESKTOP MIRROR), non-PC has 4.
	y = VR_FRAME_CENTER_Y - ( ( (isPC ? 5 : 4) - 1 ) * VR_MENU_VERTICAL_SPACING + PROP_HEIGHT ) / 2;
	s_vroptions.comfort.generic.type		= MTYPE_PTEXT;
	s_vroptions.comfort.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vroptions.comfort.generic.x			= 320;
	s_vroptions.comfort.generic.y			= y;
	s_vroptions.comfort.generic.id			= ID_COMFORT;
	s_vroptions.comfort.generic.callback	= VROptions_Event;
	s_vroptions.comfort.string				= "COMFORT";
	s_vroptions.comfort.color				= color_red;
	s_vroptions.comfort.style				= UI_CENTER;

	y += VR_MENU_VERTICAL_SPACING;
	s_vroptions.controls.generic.type		= MTYPE_PTEXT;
	s_vroptions.controls.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vroptions.controls.generic.x			= 320;
	s_vroptions.controls.generic.y			= y;
	s_vroptions.controls.generic.id			= ID_CONTROLS;
	s_vroptions.controls.generic.callback	= VROptions_Event;
	s_vroptions.controls.string				= "CONTROLS";
	s_vroptions.controls.color				= color_red;
	s_vroptions.controls.style				= UI_CENTER;

	y += VR_MENU_VERTICAL_SPACING;
	s_vroptions.hud.generic.type			= MTYPE_PTEXT;
	s_vroptions.hud.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vroptions.hud.generic.x				= 320;
	s_vroptions.hud.generic.y				= y;
	s_vroptions.hud.generic.id				= ID_HUD;
	s_vroptions.hud.generic.callback		= VROptions_Event;
	s_vroptions.hud.string					= "HUD & DISPLAY";
	s_vroptions.hud.color					= color_red;
	s_vroptions.hud.style					= UI_CENTER;

	if ( isPC ) {
		y += VR_MENU_VERTICAL_SPACING;
		s_vroptions.mirror.generic.type			= MTYPE_PTEXT;
		s_vroptions.mirror.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_vroptions.mirror.generic.x			= 320;
		s_vroptions.mirror.generic.y			= y;
		s_vroptions.mirror.generic.id			= ID_MIRROR;
		s_vroptions.mirror.generic.callback		= VROptions_Event;
		s_vroptions.mirror.string				= "DESKTOP MIRROR";
		s_vroptions.mirror.color				= color_red;
		s_vroptions.mirror.style				= UI_CENTER;
	}

	y += VR_MENU_VERTICAL_SPACING;
	s_vroptions.console.generic.type		= MTYPE_PTEXT;
	s_vroptions.console.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vroptions.console.generic.x			= 320;
	s_vroptions.console.generic.y			= y;
	s_vroptions.console.generic.id			= ID_CONSOLE;
	s_vroptions.console.generic.callback	= VROptions_Event;
	s_vroptions.console.string				= "CONSOLE";
	s_vroptions.console.color				= color_red;
	s_vroptions.console.style				= UI_CENTER;

	s_vroptions.back.generic.type		= MTYPE_BITMAP;
	s_vroptions.back.generic.name		= ART_BACK0;
	s_vroptions.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vroptions.back.generic.id			= ID_BACK;
	s_vroptions.back.generic.callback	= VROptions_Event;
	s_vroptions.back.generic.x			= 0;
	s_vroptions.back.generic.y			= 480-64;
	s_vroptions.back.width				= 128;
	s_vroptions.back.height				= 64;
	s_vroptions.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_vroptions.menu, &s_vroptions.banner );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.framel );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.framer );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.comfort );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.controls );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.hud );
	if ( isPC ) {
		Menu_AddItem( &s_vroptions.menu, &s_vroptions.mirror );
	}
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.console );
	Menu_AddItem( &s_vroptions.menu, &s_vroptions.back );
}


/*
=================
UI_VROptions_Cache
=================
*/
void UI_VROptions_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_VROptionsMenu
===============
*/
void UI_VROptionsMenu( void ) {
	VROptions_MenuInit();
	UI_PushMenu( &s_vroptions.menu );
}
