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

VR HUD & DISPLAY OPTIONS MENU

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

#define ID_HUDMODE				127
#define ID_HUDDEPTH				128
#define ID_HUDSCALE				129
#define ID_HUDYOFFSET			130
#define ID_VIRTUALSCREENMODE	131
#define ID_SELECTORWITHHUD		132
#define ID_SHOWINHAND			133
#define ID_SHOWCONSOLE			134
#define ID_LASERSIGHT			135
#define ID_SUPERSAMPLING		136
#define ID_VIRTUALSCREENSHAPE	137
#define ID_SCREENCURVATURE		138
#define ID_REFRESHRATE			139

#define ID_BACK					150


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

	menulist_s			hudmode;
	menuslider_s		huddepth;
	menuslider_s		hudscale;
	menuslider_s		hudyoffset;
	menulist_s			virtualscreenmode;
	menuradiobutton_s	selectorwithhud;
	menuradiobutton_s	showinhand;
	menuradiobutton_s	showconsole;
	menuradiobutton_s	lasersight;
	menulist_s			supersampling;
	menulist_s			virtualscreenshape;
	menuslider_s		screencurvature;
	menulist_s			refreshrate;

	menubitmap_s		back;
} vrhud_t;

static vrhud_t s_vrhud;

static qboolean s_vrhud_isPC;
static qboolean s_vrhud_isQuest;


static void VRHud_SetMenuItems( void ) {
	float superSampling;

	s_vrhud.hudmode.curvalue			= trap_Cvar_VariableValue( "vr_hudDrawStatus" );
	s_vrhud.huddepth.curvalue			= trap_Cvar_VariableValue( "vr_hudDepth" );
	s_vrhud.hudscale.curvalue			= trap_Cvar_VariableValue( "vr_hudScale" );
	s_vrhud.hudyoffset.curvalue			= trap_Cvar_VariableValue( "vr_hudYOffset" ) + 200;
	s_vrhud.virtualscreenmode.curvalue	= trap_Cvar_VariableValue( "vr_virtualScreenMode" );
	s_vrhud.selectorwithhud.curvalue	= trap_Cvar_VariableValue( "vr_weaponSelectorWithHud" ) != 0;
	s_vrhud.showinhand.curvalue			= trap_Cvar_VariableValue( "vr_showItemInHand" ) != 0;
	s_vrhud.showconsole.curvalue		= trap_Cvar_VariableValue( "vr_showConsoleMessages" ) != 0;
	s_vrhud.lasersight.curvalue			= trap_Cvar_VariableValue( "vr_lasersight" ) != 0;

	superSampling = trap_Cvar_VariableValue( "vr_superSampling" );
	if ( s_vrhud_isQuest ) {
		// Quest: 6-step {0.8..1.3}
		if (superSampling == 0.8f) {
			s_vrhud.supersampling.curvalue = 0;
		} else if (superSampling == 0.9f) {
			s_vrhud.supersampling.curvalue = 1;
		} else if (superSampling == 1.0f) {
			s_vrhud.supersampling.curvalue = 2;
		} else if (superSampling == 1.1f) {
			s_vrhud.supersampling.curvalue = 3;
		} else if (superSampling == 1.2f) {
			s_vrhud.supersampling.curvalue = 4;
		} else if (superSampling == 1.3f) {
			s_vrhud.supersampling.curvalue = 5;
		} else {
			s_vrhud.supersampling.curvalue = 3;
		}
	} else {
		// PC: 13-step {0.5..2.0}
		if (superSampling <= 0.5f) {
			s_vrhud.supersampling.curvalue = 0;
		} else if (superSampling == 0.6f) {
			s_vrhud.supersampling.curvalue = 1;
		} else if (superSampling == 0.7f) {
			s_vrhud.supersampling.curvalue = 2;
		} else if (superSampling == 0.8f) {
			s_vrhud.supersampling.curvalue = 3;
		} else if (superSampling == 0.9f) {
			s_vrhud.supersampling.curvalue = 4;
		} else if (superSampling == 1.0f) {
			s_vrhud.supersampling.curvalue = 5;
		} else if (superSampling == 1.1f) {
			s_vrhud.supersampling.curvalue = 6;
		} else if (superSampling == 1.2f) {
			s_vrhud.supersampling.curvalue = 7;
		} else if (superSampling == 1.3f) {
			s_vrhud.supersampling.curvalue = 8;
		} else if (superSampling == 1.4f) {
			s_vrhud.supersampling.curvalue = 9;
		} else if (superSampling == 1.5f) {
			s_vrhud.supersampling.curvalue = 10;
		} else if (superSampling == 1.75f) {
			s_vrhud.supersampling.curvalue = 11;
		} else if (superSampling >= 2.0f) {
			s_vrhud.supersampling.curvalue = 12;
		} else {
			s_vrhud.supersampling.curvalue = 5;
		}
	}

	s_vrhud.virtualscreenshape.curvalue	= trap_Cvar_VariableValue( "vr_virtualScreenShape" );
	s_vrhud.screencurvature.curvalue	= trap_Cvar_VariableValue( "vr_screenCurvature" );

	switch ( (int) trap_Cvar_VariableValue( "vr_refreshrate" ) )
	{
		case 60:  s_vrhud.refreshrate.curvalue = 0; break;
		case 72:  s_vrhud.refreshrate.curvalue = 1; break;
		case 80:  s_vrhud.refreshrate.curvalue = 2; break;
		case 90:  s_vrhud.refreshrate.curvalue = 3; break;
		case 120: s_vrhud.refreshrate.curvalue = 4; break;
		default:  s_vrhud.refreshrate.curvalue = 1; break;
	}
}


static void VRHud_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_HUDMODE:
			trap_Cvar_SetValue( "vr_hudDrawStatus", s_vrhud.hudmode.curvalue );
			trap_Cvar_SetValue( "cg_draw3dIcons", (s_vrhud.hudmode.curvalue == 2) ? 0 : 1 );
			break;

		case ID_HUDDEPTH:
			trap_Cvar_SetValue( "vr_hudDepth", s_vrhud.huddepth.curvalue );
			break;

		case ID_HUDSCALE:
			trap_Cvar_SetValue( "vr_hudScale", s_vrhud.hudscale.curvalue );
			break;

		case ID_HUDYOFFSET:
			trap_Cvar_SetValue( "vr_hudYOffset", s_vrhud.hudyoffset.curvalue - 200 );
			break;

		case ID_VIRTUALSCREENMODE:
			trap_Cvar_SetValue( "vr_virtualScreenMode", s_vrhud.virtualscreenmode.curvalue );
			break;

		case ID_SELECTORWITHHUD:
			trap_Cvar_SetValue( "vr_weaponSelectorWithHud", s_vrhud.selectorwithhud.curvalue );
			break;

		case ID_SHOWINHAND:
			trap_Cvar_SetValue( "vr_showItemInHand", s_vrhud.showinhand.curvalue );
			break;

		case ID_SHOWCONSOLE:
			trap_Cvar_SetValue( "vr_showConsoleMessages", s_vrhud.showconsole.curvalue );
			break;

		case ID_LASERSIGHT:
			trap_Cvar_SetValue( "vr_lasersight", s_vrhud.lasersight.curvalue );
			break;

		case ID_SUPERSAMPLING:
			{
				float supersampling;
				if ( s_vrhud_isQuest ) {
					switch (s_vrhud.supersampling.curvalue) {
						case 0: supersampling = 0.8f; break;
						case 1: supersampling = 0.9f; break;
						case 2: supersampling = 1.0f; break;
						case 3: supersampling = 1.1f; break;
						case 4: supersampling = 1.2f; break;
						case 5: supersampling = 1.3f; break;
						default: supersampling = 1.1f; break;
					}
				} else {
					switch (s_vrhud.supersampling.curvalue) {
						case 0: supersampling = 0.5f; break;
						case 1: supersampling = 0.6f; break;
						case 2: supersampling = 0.7f; break;
						case 3: supersampling = 0.8f; break;
						case 4: supersampling = 0.9f; break;
						case 5: supersampling = 1.0f; break;
						case 6: supersampling = 1.1f; break;
						case 7: supersampling = 1.2f; break;
						case 8: supersampling = 1.3f; break;
						case 9: supersampling = 1.4f; break;
						case 10: supersampling = 1.5f; break;
						case 11: supersampling = 1.75f; break;
						case 12: supersampling = 2.0f; break;
						default: supersampling = 1.0f; break;
					}
				}
				trap_Cvar_SetValue( "vr_superSampling", supersampling );
			}
			break;

		case ID_VIRTUALSCREENSHAPE:
			trap_Cvar_SetValue( "vr_virtualScreenShape", s_vrhud.virtualscreenshape.curvalue );
			break;

		case ID_SCREENCURVATURE:
			trap_Cvar_SetValue( "vr_screenCurvature", s_vrhud.screencurvature.curvalue );
			break;

		case ID_REFRESHRATE:
			{
				int refresh;
				switch (s_vrhud.refreshrate.curvalue) {
					case 0:  refresh = 60;  break;
					case 1:  refresh = 72;  break;
					case 2:  refresh = 80;  break;
					case 3:  refresh = 90;  break;
					case 4:  refresh = 120; break;
					default: refresh = 72;  break;
				}
				trap_Cvar_SetValue( "vr_refreshrate", refresh );
			}
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}
}

static void VRHud_MenuInit( void ) {
	int y;

	static const char *hud_names[] =
	{
		"Off",
		"Floating",
		"Fixed to View",
		NULL,
	};

	static const char *s_virtualScreenModes[] =
	{
		"Fixed",
		"Follow",
		NULL,
	};

	static const char *s_virtualScreenShapes[] =
	{
		"Curved",
		"Flat",
		NULL,
	};

	static const char *s_supersampling_pc[] =
	{
		"0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1",
		"1.2", "1.3", "1.4", "1.5", "1.75", "2.0",
		NULL,
	};

	static const char *s_supersampling_quest[] =
	{
		"0.8", "0.9", "1.0", "1.1", "1.2", "1.3",
		NULL,
	};

	static const char *s_refreshrate[] =
	{
		"60", "72", "80", "90", "120",
		NULL,
	};

	s_vrhud_isPC    = ( UI_VR_Platform() == VRP_PC );
	s_vrhud_isQuest = ( UI_VR_Platform() == VRP_QUEST );

	memset( &s_vrhud, 0, sizeof(vrhud_t) );

	UI_VRHud_Cache();

	s_vrhud.menu.wrapAround = qtrue;
	s_vrhud.menu.fullscreen = qtrue;

	s_vrhud.banner.generic.type		= MTYPE_BTEXT;
	s_vrhud.banner.generic.x		= 320;
	s_vrhud.banner.generic.y		= 16;
	s_vrhud.banner.string			= "HUD & DISPLAY";
	s_vrhud.banner.color			= color_white;
	s_vrhud.banner.style			= UI_CENTER;

	s_vrhud.framel.generic.type		= MTYPE_BITMAP;
	s_vrhud.framel.generic.name		= ART_FRAMEL;
	s_vrhud.framel.generic.flags	= QMF_INACTIVE;
	s_vrhud.framel.generic.x		= 0;
	s_vrhud.framel.generic.y		= 78;
	s_vrhud.framel.width			= 256;
	s_vrhud.framel.height			= 329;

	s_vrhud.framer.generic.type		= MTYPE_BITMAP;
	s_vrhud.framer.generic.name		= ART_FRAMER;
	s_vrhud.framer.generic.flags	= QMF_INACTIVE;
	s_vrhud.framer.generic.x		= 376;
	s_vrhud.framer.generic.y		= 76;
	s_vrhud.framer.width			= 256;
	s_vrhud.framer.height			= 334;

	// Center the small-font row block in the frame interior. 10 base rows,
	// +1 on PC (virtual screen shape), +2 on Quest (curvature, refresh rate).
	y = VR_FRAME_CENTER_Y - ( ( (10 + (s_vrhud_isPC ? 1 : 0) + (s_vrhud_isQuest ? 2 : 0)) - 1 ) * (BIGCHAR_HEIGHT+2) + SMALLCHAR_HEIGHT ) / 2;
	s_vrhud.hudmode.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud.hudmode.generic.name		= "HUD Mode:";
	s_vrhud.hudmode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.hudmode.generic.callback	= VRHud_MenuEvent;
	s_vrhud.hudmode.generic.id			= ID_HUDMODE;
	s_vrhud.hudmode.generic.x			= VR_X_POS;
	s_vrhud.hudmode.generic.y			= y;
	s_vrhud.hudmode.itemnames			= hud_names;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.huddepth.generic.type		= MTYPE_SLIDER;
	s_vrhud.huddepth.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.huddepth.generic.x			= VR_X_POS;
	s_vrhud.huddepth.generic.y			= y;
	s_vrhud.huddepth.generic.name		= "HUD Depth:";
	s_vrhud.huddepth.generic.callback	= VRHud_MenuEvent;
	s_vrhud.huddepth.generic.id			= ID_HUDDEPTH;
	s_vrhud.huddepth.minvalue			= 0;
	s_vrhud.huddepth.maxvalue			= 5;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.hudscale.generic.type		= MTYPE_SLIDER;
	s_vrhud.hudscale.generic.x			= VR_X_POS;
	s_vrhud.hudscale.generic.y			= y;
	s_vrhud.hudscale.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.hudscale.generic.name		= "HUD Scale:";
	s_vrhud.hudscale.generic.id			= ID_HUDSCALE;
	s_vrhud.hudscale.generic.callback	= VRHud_MenuEvent;
	s_vrhud.hudscale.minvalue			= 0.5f;
	s_vrhud.hudscale.maxvalue			= 2.0f;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.hudyoffset.generic.type		= MTYPE_SLIDER;
	s_vrhud.hudyoffset.generic.x		= VR_X_POS;
	s_vrhud.hudyoffset.generic.y		= y;
	s_vrhud.hudyoffset.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.hudyoffset.generic.name		= "HUD Vertical Position:";
	s_vrhud.hudyoffset.generic.id		= ID_HUDYOFFSET;
	s_vrhud.hudyoffset.generic.callback	= VRHud_MenuEvent;
	s_vrhud.hudyoffset.minvalue			= 0;
	s_vrhud.hudyoffset.maxvalue			= 400;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.virtualscreenmode.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud.virtualscreenmode.generic.x			= VR_X_POS;
	s_vrhud.virtualscreenmode.generic.y			= y;
	s_vrhud.virtualscreenmode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.virtualscreenmode.generic.name		= "Virtual screen mode:";
	s_vrhud.virtualscreenmode.generic.id		= ID_VIRTUALSCREENMODE;
	s_vrhud.virtualscreenmode.generic.callback	= VRHud_MenuEvent;
	s_vrhud.virtualscreenmode.itemnames			= s_virtualScreenModes;
	s_vrhud.virtualscreenmode.numitems			= 2;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.selectorwithhud.generic.type		= MTYPE_RADIOBUTTON;
	s_vrhud.selectorwithhud.generic.name		= "Draw HUD On Weapon Wheel:";
	s_vrhud.selectorwithhud.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.selectorwithhud.generic.callback	= VRHud_MenuEvent;
	s_vrhud.selectorwithhud.generic.id			= ID_SELECTORWITHHUD;
	s_vrhud.selectorwithhud.generic.x			= VR_X_POS;
	s_vrhud.selectorwithhud.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.showinhand.generic.type			= MTYPE_RADIOBUTTON;
	s_vrhud.showinhand.generic.name			= "Show Item In Hand:";
	s_vrhud.showinhand.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.showinhand.generic.callback		= VRHud_MenuEvent;
	s_vrhud.showinhand.generic.id			= ID_SHOWINHAND;
	s_vrhud.showinhand.generic.x			= VR_X_POS;
	s_vrhud.showinhand.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.showconsole.generic.type		= MTYPE_RADIOBUTTON;
	s_vrhud.showconsole.generic.name		= "Show Console Messages:";
	s_vrhud.showconsole.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.showconsole.generic.callback	= VRHud_MenuEvent;
	s_vrhud.showconsole.generic.id			= ID_SHOWCONSOLE;
	s_vrhud.showconsole.generic.x			= VR_X_POS;
	s_vrhud.showconsole.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.lasersight.generic.type			= MTYPE_RADIOBUTTON;
	s_vrhud.lasersight.generic.name			= "Laser Sight:";
	s_vrhud.lasersight.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.lasersight.generic.callback		= VRHud_MenuEvent;
	s_vrhud.lasersight.generic.id			= ID_LASERSIGHT;
	s_vrhud.lasersight.generic.x			= VR_X_POS;
	s_vrhud.lasersight.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud.supersampling.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud.supersampling.generic.name		= "Supersampling:";
	s_vrhud.supersampling.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud.supersampling.generic.x			= VR_X_POS;
	s_vrhud.supersampling.generic.y			= y;
	s_vrhud.supersampling.generic.callback	= VRHud_MenuEvent;
	s_vrhud.supersampling.generic.id		= ID_SUPERSAMPLING;
	if ( s_vrhud_isQuest ) {
		s_vrhud.supersampling.itemnames		= s_supersampling_quest;
		s_vrhud.supersampling.numitems		= 6;
	} else {
		s_vrhud.supersampling.itemnames		= s_supersampling_pc;
		s_vrhud.supersampling.numitems		= 13;
	}

	// PC-only tail: Virtual screen shape
	if ( s_vrhud_isPC ) {
		y += BIGCHAR_HEIGHT+2;
		s_vrhud.virtualscreenshape.generic.type		= MTYPE_SPINCONTROL;
		s_vrhud.virtualscreenshape.generic.x		= VR_X_POS;
		s_vrhud.virtualscreenshape.generic.y		= y;
		s_vrhud.virtualscreenshape.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud.virtualscreenshape.generic.name		= "Virtual screen shape:";
		s_vrhud.virtualscreenshape.generic.id		= ID_VIRTUALSCREENSHAPE;
		s_vrhud.virtualscreenshape.generic.callback	= VRHud_MenuEvent;
		s_vrhud.virtualscreenshape.itemnames		= s_virtualScreenShapes;
		s_vrhud.virtualscreenshape.numitems			= 2;
	}

	// Quest-only tail: Screen Curvature, Refresh Rate
	if ( s_vrhud_isQuest ) {
		y += BIGCHAR_HEIGHT+2;
		s_vrhud.screencurvature.generic.type		= MTYPE_SLIDER;
		s_vrhud.screencurvature.generic.x			= VR_X_POS;
		s_vrhud.screencurvature.generic.y			= y;
		s_vrhud.screencurvature.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud.screencurvature.generic.name		= "Screen Curvature:";
		s_vrhud.screencurvature.generic.id			= ID_SCREENCURVATURE;
		s_vrhud.screencurvature.generic.callback	= VRHud_MenuEvent;
		s_vrhud.screencurvature.minvalue			= 0.0f;
		s_vrhud.screencurvature.maxvalue			= 1.0f;

		y += BIGCHAR_HEIGHT+2;
		s_vrhud.refreshrate.generic.type		= MTYPE_SPINCONTROL;
		s_vrhud.refreshrate.generic.name		= "Refresh Rate:";
		s_vrhud.refreshrate.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud.refreshrate.generic.x			= VR_X_POS;
		s_vrhud.refreshrate.generic.y			= y;
		s_vrhud.refreshrate.itemnames			= s_refreshrate;
		s_vrhud.refreshrate.generic.callback	= VRHud_MenuEvent;
		s_vrhud.refreshrate.generic.id			= ID_REFRESHRATE;
		s_vrhud.refreshrate.numitems			= 5;
	}

	s_vrhud.back.generic.type		= MTYPE_BITMAP;
	s_vrhud.back.generic.name		= ART_BACK0;
	s_vrhud.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vrhud.back.generic.callback	= VRHud_MenuEvent;
	s_vrhud.back.generic.id			= ID_BACK;
	s_vrhud.back.generic.x			= 0;
	s_vrhud.back.generic.y			= 480-64;
	s_vrhud.back.width				= 128;
	s_vrhud.back.height				= 64;
	s_vrhud.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_vrhud.menu, &s_vrhud.banner );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.framel );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.framer );

	Menu_AddItem( &s_vrhud.menu, &s_vrhud.hudmode );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.huddepth );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.hudscale );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.hudyoffset );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.virtualscreenmode );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.selectorwithhud );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.showinhand );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.showconsole );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.lasersight );
	Menu_AddItem( &s_vrhud.menu, &s_vrhud.supersampling );
	if ( s_vrhud_isPC ) {
		Menu_AddItem( &s_vrhud.menu, &s_vrhud.virtualscreenshape );
	}
	if ( s_vrhud_isQuest ) {
		Menu_AddItem( &s_vrhud.menu, &s_vrhud.screencurvature );
		Menu_AddItem( &s_vrhud.menu, &s_vrhud.refreshrate );
	}

	Menu_AddItem( &s_vrhud.menu, &s_vrhud.back );

	VRHud_SetMenuItems();
}


/*
===============
UI_VRHud_Cache
===============
*/
void UI_VRHud_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_VRHudMenu
===============
*/
void UI_VRHudMenu( void ) {
	VRHud_MenuInit();
	UI_PushMenu( &s_vrhud.menu );
}
