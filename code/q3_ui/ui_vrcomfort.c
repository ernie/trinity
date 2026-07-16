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

VR COMFORT OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"

#define VR_X_POS		360

// Vertical center of the framel/framer oval interior (640x480 space).
#define VR_FRAME_CENTER_Y	242

#define ID_COMFORTVIGNETTE		127
#define ID_HEIGHTADJUST			128
#define ID_SIXDOF				129
#define ID_ROLLHIT				130
#define ID_SMOOTHFOLLOW			131
#define ID_HAPTICINTENSITY		132
#define ID_BHAPTICS				133

#define ID_BACK					140


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

	menuslider_s		comfortvignette;
	menuslider_s		heightadjust;
	menuradiobutton_s	sixdof;
	menuradiobutton_s	rollhit;
	menuradiobutton_s	smoothfollow;
	menuslider_s		hapticintensity;
	menuradiobutton_s	bhaptics;

	menubitmap_s		back;
} vrcomfort_t;

static vrcomfort_t s_vrcomfort;


static void VRComfort_SixDofStatusBar( void *self ) {
	UI_DrawString( SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80, "Physical head movement controls in-game movement (SP only)", UI_SMALLFONT|UI_CENTER, colorWhite );
}

static void VRComfort_SetMenuItems( void ) {
	s_vrcomfort.comfortvignette.curvalue	= trap_Cvar_VariableValue( "vr_comfortVignette" );
	s_vrcomfort.heightadjust.curvalue		= trap_Cvar_VariableValue( "vr_heightAdjust" );
	s_vrcomfort.sixdof.curvalue				= trap_Cvar_VariableValue( "vr_6dof" ) != 0;
	s_vrcomfort.rollhit.curvalue			= trap_Cvar_VariableValue( "vr_rollWhenHit" ) != 0;
	s_vrcomfort.smoothfollow.curvalue		= trap_Cvar_VariableValue( "cg_smoothFollow" ) != 0;
	s_vrcomfort.hapticintensity.curvalue	= trap_Cvar_VariableValue( "vr_hapticIntensity" );
	s_vrcomfort.bhaptics.curvalue			= trap_Cvar_VariableValue( "vr_bhaptics" ) != 0;
}


static void VRComfort_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_COMFORTVIGNETTE:
			trap_Cvar_SetValue( "vr_comfortVignette", s_vrcomfort.comfortvignette.curvalue );
			break;

		case ID_HEIGHTADJUST:
			trap_Cvar_SetValue( "vr_heightAdjust", s_vrcomfort.heightadjust.curvalue );
			break;

		case ID_SIXDOF:
			trap_Cvar_SetValue( "vr_6dof", s_vrcomfort.sixdof.curvalue );
			break;

		case ID_ROLLHIT:
			trap_Cvar_SetValue( "vr_rollWhenHit", s_vrcomfort.rollhit.curvalue );
			break;

		case ID_SMOOTHFOLLOW:
			trap_Cvar_SetValue( "cg_smoothFollow", s_vrcomfort.smoothfollow.curvalue );
			break;

		case ID_HAPTICINTENSITY:
			trap_Cvar_SetValue( "vr_hapticIntensity", s_vrcomfort.hapticintensity.curvalue );
			break;

		case ID_BHAPTICS:
			trap_Cvar_SetValue( "vr_bhaptics", s_vrcomfort.bhaptics.curvalue );
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}
}

static void VRComfort_MenuInit( void ) {
	int y;

	memset( &s_vrcomfort, 0, sizeof(vrcomfort_t) );

	UI_VRComfort_Cache();

	s_vrcomfort.menu.wrapAround = qtrue;
	s_vrcomfort.menu.fullscreen = qtrue;

	s_vrcomfort.banner.generic.type		= MTYPE_BTEXT;
	s_vrcomfort.banner.generic.x		= 320;
	s_vrcomfort.banner.generic.y		= 16;
	s_vrcomfort.banner.string			= "COMFORT";
	s_vrcomfort.banner.color			= color_white;
	s_vrcomfort.banner.style			= UI_CENTER;

	s_vrcomfort.framel.generic.type		= MTYPE_BITMAP;
	s_vrcomfort.framel.generic.name		= ART_FRAMEL;
	s_vrcomfort.framel.generic.flags	= QMF_INACTIVE;
	s_vrcomfort.framel.generic.x		= 0;
	s_vrcomfort.framel.generic.y		= 78;
	s_vrcomfort.framel.width			= 256;
	s_vrcomfort.framel.height			= 329;

	s_vrcomfort.framer.generic.type		= MTYPE_BITMAP;
	s_vrcomfort.framer.generic.name		= ART_FRAMER;
	s_vrcomfort.framer.generic.flags	= QMF_INACTIVE;
	s_vrcomfort.framer.generic.x		= 376;
	s_vrcomfort.framer.generic.y		= 76;
	s_vrcomfort.framer.width			= 256;
	s_vrcomfort.framer.height			= 334;

	// Center the 7-row block (small-font rows) in the frame interior.
	y = VR_FRAME_CENTER_Y - ( 6 * (BIGCHAR_HEIGHT+2) + SMALLCHAR_HEIGHT ) / 2;
	s_vrcomfort.comfortvignette.generic.type		= MTYPE_SLIDER;
	s_vrcomfort.comfortvignette.generic.x			= VR_X_POS;
	s_vrcomfort.comfortvignette.generic.y			= y;
	s_vrcomfort.comfortvignette.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.comfortvignette.generic.name		= "Comfort Vignette:";
	s_vrcomfort.comfortvignette.generic.id			= ID_COMFORTVIGNETTE;
	s_vrcomfort.comfortvignette.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.comfortvignette.minvalue			= 0.0f;
	s_vrcomfort.comfortvignette.maxvalue			= 1.0f;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.heightadjust.generic.type		= MTYPE_SLIDER;
	s_vrcomfort.heightadjust.generic.x			= VR_X_POS;
	s_vrcomfort.heightadjust.generic.y			= y;
	s_vrcomfort.heightadjust.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.heightadjust.generic.name		= "Height Adjust:";
	s_vrcomfort.heightadjust.generic.id			= ID_HEIGHTADJUST;
	s_vrcomfort.heightadjust.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.heightadjust.minvalue			= 0.0f;
	s_vrcomfort.heightadjust.maxvalue			= 1.0f;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.sixdof.generic.type			= MTYPE_RADIOBUTTON;
	s_vrcomfort.sixdof.generic.name			= "SP 6DoF Movement:";
	s_vrcomfort.sixdof.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.sixdof.generic.callback		= VRComfort_MenuEvent;
	s_vrcomfort.sixdof.generic.id			= ID_SIXDOF;
	s_vrcomfort.sixdof.generic.x			= VR_X_POS;
	s_vrcomfort.sixdof.generic.y			= y;
	s_vrcomfort.sixdof.generic.statusbar	= VRComfort_SixDofStatusBar;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.rollhit.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcomfort.rollhit.generic.name		= "Roll When Hit:";
	s_vrcomfort.rollhit.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.rollhit.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.rollhit.generic.id			= ID_ROLLHIT;
	s_vrcomfort.rollhit.generic.x			= VR_X_POS;
	s_vrcomfort.rollhit.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.smoothfollow.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcomfort.smoothfollow.generic.name		= "Smooth Follow:";
	s_vrcomfort.smoothfollow.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.smoothfollow.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.smoothfollow.generic.id			= ID_SMOOTHFOLLOW;
	s_vrcomfort.smoothfollow.generic.x			= VR_X_POS;
	s_vrcomfort.smoothfollow.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.hapticintensity.generic.type		= MTYPE_SLIDER;
	s_vrcomfort.hapticintensity.generic.x			= VR_X_POS;
	s_vrcomfort.hapticintensity.generic.y			= y;
	s_vrcomfort.hapticintensity.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.hapticintensity.generic.name		= "Haptic Intensity:";
	s_vrcomfort.hapticintensity.generic.id			= ID_HAPTICINTENSITY;
	s_vrcomfort.hapticintensity.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.hapticintensity.minvalue			= 0;
	s_vrcomfort.hapticintensity.maxvalue			= 1.0;

	y += BIGCHAR_HEIGHT+2;
	s_vrcomfort.bhaptics.generic.type		= MTYPE_RADIOBUTTON;
	s_vrcomfort.bhaptics.generic.name		= "bHaptics Support:";
	s_vrcomfort.bhaptics.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrcomfort.bhaptics.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.bhaptics.generic.id			= ID_BHAPTICS;
	s_vrcomfort.bhaptics.generic.x			= VR_X_POS;
	s_vrcomfort.bhaptics.generic.y			= y;

	s_vrcomfort.back.generic.type		= MTYPE_BITMAP;
	s_vrcomfort.back.generic.name		= ART_BACK0;
	s_vrcomfort.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vrcomfort.back.generic.callback	= VRComfort_MenuEvent;
	s_vrcomfort.back.generic.id			= ID_BACK;
	s_vrcomfort.back.generic.x			= 0;
	s_vrcomfort.back.generic.y			= 480-64;
	s_vrcomfort.back.width				= 128;
	s_vrcomfort.back.height				= 64;
	s_vrcomfort.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.banner );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.framel );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.framer );

	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.comfortvignette );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.heightadjust );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.sixdof );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.rollhit );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.smoothfollow );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.hapticintensity );
	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.bhaptics );

	Menu_AddItem( &s_vrcomfort.menu, &s_vrcomfort.back );

	VRComfort_SetMenuItems();
}


/*
===============
UI_VRComfort_Cache
===============
*/
void UI_VRComfort_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_VRComfortMenu
===============
*/
void UI_VRComfortMenu( void ) {
	VRComfort_MenuInit();
	UI_PushMenu( &s_vrcomfort.menu );
}
