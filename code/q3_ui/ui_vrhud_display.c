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
#define ART_ACCEPT0				"menu/art/accept_0"
#define ART_ACCEPT1				"menu/art/accept_1"

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
#define ID_APPLY				140
#define ID_FOVEATION			141
#define ID_FOVEATIONSTRENGTH	142

#define ID_BACK					150

#define MAX_REFRESH_RATES		16


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
	menulist_s			foveation;
	menulist_s			foveationstrength;

	menubitmap_s		apply;
	menubitmap_s		back;
} vrhud_display_t;

static vrhud_display_t s_vrhud_display;

static qboolean s_vrhud_display_isPC;
static qboolean s_vrhud_display_isStandalone;

// Display rates the engine's runtime supports, from vr_refreshrates
static int			s_refreshRates[MAX_REFRESH_RATES];
static char			s_refreshRateNames[MAX_REFRESH_RATES][8];
static const char	*s_refreshRateItems[MAX_REFRESH_RATES + 1];
static int			s_numRefreshRates;

// Cut at vr_foveationCaps: the spin control counts items up to the NULL.
static const char	*s_foveationItems[4];
static int			s_numFoveationItems;

static const char	*s_foveationStrengthItems[] = { "Low", "Medium", "High", NULL };

static float		s_vrhud_display_activeSupersampling;


static float VRHudDisplay_SupersamplingFromIndex( int index ) {
	static const float steps[] = { 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.75f, 2.0f };

	return ( index >= 0 && index < ARRAY_LEN( steps ) ) ? steps[index] : 1.0f;
}


static void VRHudDisplay_UpdateApply( void ) {
	float picked = VRHudDisplay_SupersamplingFromIndex( s_vrhud_display.supersampling.curvalue );

	if ( fabs( picked - s_vrhud_display_activeSupersampling ) > 0.001f ) {
		s_vrhud_display.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	} else {
		s_vrhud_display.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	}
}


// No row at all on engines or headsets without foveation
static void VRHudDisplay_ReadFoveationCaps( void ) {
	static const char *names[] = { "Off", "Fixed", "Eye-Tracked" };
	char caps[32];
	int i;

	trap_Cvar_VariableStringBuffer( "vr_foveationCaps", caps, sizeof( caps ) );
	if ( !Q_stricmp( caps, "eyetracked" ) ) {
		s_numFoveationItems = 3;
	} else if ( !Q_stricmp( caps, "fixed" ) ) {
		s_numFoveationItems = 2;
	} else {
		s_numFoveationItems = 0;
	}
	for ( i = 0; i < s_numFoveationItems; i++ ) {
		s_foveationItems[i] = names[i];
	}
	s_foveationItems[s_numFoveationItems] = NULL;
}


// Engines without vr_refreshrates get the classic choices
static void VRHudDisplay_ReadRefreshRates( void ) {
	static const int fallback[] = { 60, 72, 80, 90, 120 };
	char list[256];
	char *p;
	int i;

	s_numRefreshRates = 0;
	trap_Cvar_VariableStringBuffer( "vr_refreshrates", list, sizeof( list ) );
	p = list;
	while ( s_numRefreshRates < MAX_REFRESH_RATES ) {
		const char *token = COM_Parse( &p );
		if ( !token[0] ) {
			break;
		}
		s_refreshRates[s_numRefreshRates++] = (int)( atof( token ) + 0.5f );
	}
	if ( s_numRefreshRates == 0 ) {
		for ( i = 0; i < ARRAY_LEN( fallback ); i++ ) {
			s_refreshRates[i] = fallback[i];
		}
		s_numRefreshRates = ARRAY_LEN( fallback );
	}
	for ( i = 0; i < s_numRefreshRates; i++ ) {
		Com_sprintf( s_refreshRateNames[i], sizeof( s_refreshRateNames[i] ), "%i", s_refreshRates[i] );
		s_refreshRateItems[i] = s_refreshRateNames[i];
	}
	s_refreshRateItems[s_numRefreshRates] = NULL;
}


static void VRHudDisplay_SetMenuItems( void ) {
	float superSampling;
	int i;

	s_vrhud_display.hudmode.curvalue			= trap_Cvar_VariableValue( "vr_hudDrawStatus" );
	s_vrhud_display.huddepth.curvalue			= trap_Cvar_VariableValue( "vr_hudDepth" );
	s_vrhud_display.hudscale.curvalue			= trap_Cvar_VariableValue( "vr_hudScale" );
	s_vrhud_display.hudyoffset.curvalue			= trap_Cvar_VariableValue( "vr_hudYOffset" ) + 200;
	s_vrhud_display.virtualscreenmode.curvalue	= trap_Cvar_VariableValue( "vr_virtualScreenMode" );
	s_vrhud_display.selectorwithhud.curvalue	= trap_Cvar_VariableValue( "vr_weaponSelectorWithHud" ) != 0;
	s_vrhud_display.showinhand.curvalue			= trap_Cvar_VariableValue( "vr_showItemInHand" ) != 0;
	s_vrhud_display.showconsole.curvalue		= trap_Cvar_VariableValue( "vr_showConsoleMessages" ) != 0;
	s_vrhud_display.lasersight.curvalue			= trap_Cvar_VariableValue( "vr_lasersight" ) != 0;

	superSampling = trap_Cvar_VariableValue( "vr_superSampling" );
	s_vrhud_display.supersampling.curvalue = 5;   // 1.0, if the cvar names nothing on the list
	for ( i = 0; i < s_vrhud_display.supersampling.numitems; i++ ) {
		if ( fabs( VRHudDisplay_SupersamplingFromIndex( i ) - superSampling ) < 0.001f ) {
			s_vrhud_display.supersampling.curvalue = i;
			break;
		}
	}

	s_vrhud_display.virtualscreenshape.curvalue	= trap_Cvar_VariableValue( "vr_virtualScreenShape" );
	s_vrhud_display.screencurvature.curvalue	= trap_Cvar_VariableValue( "vr_screenCurvature" );

	// nearest, since an archived vr_refreshrate may not be in this headset's list
	{
		int rate = (int)( trap_Cvar_VariableValue( "vr_refreshrate" ) + 0.5f );
		int best = 0;
		int i;

		for ( i = 1; i < s_numRefreshRates; i++ ) {
			if ( abs( s_refreshRates[i] - rate ) < abs( s_refreshRates[best] - rate ) ) {
				best = i;
			}
		}
		s_vrhud_display.refreshrate.curvalue = best;
	}

	if ( s_numFoveationItems > 0 ) {
		int level = (int)trap_Cvar_VariableValue( "vr_foveation" );
		int strength = (int)trap_Cvar_VariableValue( "vr_foveationStrength" );

		if ( level < 0 ) {
			level = 0;
		} else if ( level >= s_numFoveationItems ) {
			level = s_numFoveationItems - 1;
		}
		s_vrhud_display.foveation.curvalue = level;

		// the cvar counts from 1, the spin control from 0
		if ( strength < 1 ) {
			strength = 1;
		} else if ( strength > 3 ) {
			strength = 3;
		}
		s_vrhud_display.foveationstrength.curvalue = strength - 1;
	}
}


static void VRHudDisplay_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_HUDMODE:
			trap_Cvar_SetValue( "vr_hudDrawStatus", s_vrhud_display.hudmode.curvalue );
			trap_Cvar_SetValue( "cg_draw3dIcons", (s_vrhud_display.hudmode.curvalue == 2) ? 0 : 1 );
			break;

		case ID_HUDDEPTH:
			trap_Cvar_SetValue( "vr_hudDepth", s_vrhud_display.huddepth.curvalue );
			break;

		case ID_HUDSCALE:
			trap_Cvar_SetValue( "vr_hudScale", s_vrhud_display.hudscale.curvalue );
			break;

		case ID_HUDYOFFSET:
			trap_Cvar_SetValue( "vr_hudYOffset", s_vrhud_display.hudyoffset.curvalue - 200 );
			break;

		case ID_VIRTUALSCREENMODE:
			trap_Cvar_SetValue( "vr_virtualScreenMode", s_vrhud_display.virtualscreenmode.curvalue );
			break;

		case ID_SELECTORWITHHUD:
			trap_Cvar_SetValue( "vr_weaponSelectorWithHud", s_vrhud_display.selectorwithhud.curvalue );
			break;

		case ID_SHOWINHAND:
			trap_Cvar_SetValue( "vr_showItemInHand", s_vrhud_display.showinhand.curvalue );
			break;

		case ID_SHOWCONSOLE:
			trap_Cvar_SetValue( "vr_showConsoleMessages", s_vrhud_display.showconsole.curvalue );
			break;

		case ID_LASERSIGHT:
			trap_Cvar_SetValue( "vr_lasersight", s_vrhud_display.lasersight.curvalue );
			break;

		case ID_SUPERSAMPLING:
			trap_Cvar_SetValue( "vr_superSampling", VRHudDisplay_SupersamplingFromIndex( s_vrhud_display.supersampling.curvalue ) );
			VRHudDisplay_UpdateApply();
			break;

		case ID_APPLY:
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
			break;

		case ID_VIRTUALSCREENSHAPE:
			trap_Cvar_SetValue( "vr_virtualScreenShape", s_vrhud_display.virtualscreenshape.curvalue );
			break;

		case ID_SCREENCURVATURE:
			trap_Cvar_SetValue( "vr_screenCurvature", s_vrhud_display.screencurvature.curvalue );
			break;

		case ID_REFRESHRATE:
			// the engine applies this live and writes back the rate it actually got
			trap_Cvar_SetValue( "vr_refreshrate", s_refreshRates[s_vrhud_display.refreshrate.curvalue] );
			break;

		case ID_FOVEATION:
			// applies live too; the engine falls back if the runtime refuses a mode
			trap_Cvar_SetValue( "vr_foveation", s_vrhud_display.foveation.curvalue );
			break;

		case ID_FOVEATIONSTRENGTH:
			trap_Cvar_SetValue( "vr_foveationStrength", s_vrhud_display.foveationstrength.curvalue + 1 );
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}
}

static void VRHudDisplay_MenuInit( void ) {
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

	static const char *s_supersampling_names[] =
	{
		"0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1",
		"1.2", "1.3", "1.4", "1.5", "1.75", "2.0",
		NULL,
	};

	s_vrhud_display_isPC         = ( UI_VR_Platform() == VRP_PC );
	s_vrhud_display_isStandalone = ( UI_VR_Platform() == VRP_STANDALONE );

	memset( &s_vrhud_display, 0, sizeof(vrhud_display_t) );

	UI_VRHudDisplay_Cache();
	VRHudDisplay_ReadRefreshRates();
	VRHudDisplay_ReadFoveationCaps();
	s_vrhud_display_activeSupersampling = trap_Cvar_VariableValue( "vr_superSampling" );

	s_vrhud_display.menu.wrapAround = qtrue;
	s_vrhud_display.menu.fullscreen = qtrue;

	s_vrhud_display.banner.generic.type		= MTYPE_BTEXT;
	s_vrhud_display.banner.generic.x		= 320;
	s_vrhud_display.banner.generic.y		= 16;
	s_vrhud_display.banner.string			= "HUD & DISPLAY";
	s_vrhud_display.banner.color			= color_white;
	s_vrhud_display.banner.style			= UI_CENTER;

	s_vrhud_display.framel.generic.type		= MTYPE_BITMAP;
	s_vrhud_display.framel.generic.name		= ART_FRAMEL;
	s_vrhud_display.framel.generic.flags	= QMF_INACTIVE;
	s_vrhud_display.framel.generic.x		= 0;
	s_vrhud_display.framel.generic.y		= 78;
	s_vrhud_display.framel.width			= 256;
	s_vrhud_display.framel.height			= 329;

	s_vrhud_display.framer.generic.type		= MTYPE_BITMAP;
	s_vrhud_display.framer.generic.name		= ART_FRAMER;
	s_vrhud_display.framer.generic.flags	= QMF_INACTIVE;
	s_vrhud_display.framer.generic.x		= 376;
	s_vrhud_display.framer.generic.y		= 76;
	s_vrhud_display.framer.width			= 256;
	s_vrhud_display.framer.height			= 334;

	// Center the small-font row block in the frame interior. 10 base rows,
	// +1 on PC (virtual screen shape), +1 on standalone (screen curvature),
	// +1 for refresh rate (every platform), +2 with foveation.
	y = VR_FRAME_CENTER_Y - ( ( (10 + (s_vrhud_display_isPC ? 1 : 0) + (s_vrhud_display_isStandalone ? 1 : 0) + 1 + (s_numFoveationItems ? 2 : 0)) - 1 ) * (BIGCHAR_HEIGHT+2) + SMALLCHAR_HEIGHT ) / 2;
	s_vrhud_display.hudmode.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud_display.hudmode.generic.name		= "HUD Mode:";
	s_vrhud_display.hudmode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.hudmode.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.hudmode.generic.id			= ID_HUDMODE;
	s_vrhud_display.hudmode.generic.x			= VR_X_POS;
	s_vrhud_display.hudmode.generic.y			= y;
	s_vrhud_display.hudmode.itemnames			= hud_names;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.huddepth.generic.type		= MTYPE_SLIDER;
	s_vrhud_display.huddepth.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.huddepth.generic.x			= VR_X_POS;
	s_vrhud_display.huddepth.generic.y			= y;
	s_vrhud_display.huddepth.generic.name		= "HUD Depth:";
	s_vrhud_display.huddepth.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.huddepth.generic.id			= ID_HUDDEPTH;
	s_vrhud_display.huddepth.minvalue			= 0;
	s_vrhud_display.huddepth.maxvalue			= 5;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.hudscale.generic.type		= MTYPE_SLIDER;
	s_vrhud_display.hudscale.generic.x			= VR_X_POS;
	s_vrhud_display.hudscale.generic.y			= y;
	s_vrhud_display.hudscale.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.hudscale.generic.name		= "HUD Scale:";
	s_vrhud_display.hudscale.generic.id			= ID_HUDSCALE;
	s_vrhud_display.hudscale.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.hudscale.minvalue			= 0.5f;
	s_vrhud_display.hudscale.maxvalue			= 2.0f;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.hudyoffset.generic.type		= MTYPE_SLIDER;
	s_vrhud_display.hudyoffset.generic.x		= VR_X_POS;
	s_vrhud_display.hudyoffset.generic.y		= y;
	s_vrhud_display.hudyoffset.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.hudyoffset.generic.name		= "HUD Vertical Position:";
	s_vrhud_display.hudyoffset.generic.id		= ID_HUDYOFFSET;
	s_vrhud_display.hudyoffset.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.hudyoffset.minvalue			= 0;
	s_vrhud_display.hudyoffset.maxvalue			= 400;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.virtualscreenmode.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud_display.virtualscreenmode.generic.x			= VR_X_POS;
	s_vrhud_display.virtualscreenmode.generic.y			= y;
	s_vrhud_display.virtualscreenmode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.virtualscreenmode.generic.name		= "Virtual screen mode:";
	s_vrhud_display.virtualscreenmode.generic.id		= ID_VIRTUALSCREENMODE;
	s_vrhud_display.virtualscreenmode.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.virtualscreenmode.itemnames			= s_virtualScreenModes;
	s_vrhud_display.virtualscreenmode.numitems			= 2;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.selectorwithhud.generic.type		= MTYPE_RADIOBUTTON;
	s_vrhud_display.selectorwithhud.generic.name		= "Draw HUD On Weapon Wheel:";
	s_vrhud_display.selectorwithhud.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.selectorwithhud.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.selectorwithhud.generic.id			= ID_SELECTORWITHHUD;
	s_vrhud_display.selectorwithhud.generic.x			= VR_X_POS;
	s_vrhud_display.selectorwithhud.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.showinhand.generic.type			= MTYPE_RADIOBUTTON;
	s_vrhud_display.showinhand.generic.name			= "Show Item In Hand:";
	s_vrhud_display.showinhand.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.showinhand.generic.callback		= VRHudDisplay_MenuEvent;
	s_vrhud_display.showinhand.generic.id			= ID_SHOWINHAND;
	s_vrhud_display.showinhand.generic.x			= VR_X_POS;
	s_vrhud_display.showinhand.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.showconsole.generic.type		= MTYPE_RADIOBUTTON;
	s_vrhud_display.showconsole.generic.name		= "Show Console Messages:";
	s_vrhud_display.showconsole.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.showconsole.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.showconsole.generic.id			= ID_SHOWCONSOLE;
	s_vrhud_display.showconsole.generic.x			= VR_X_POS;
	s_vrhud_display.showconsole.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.lasersight.generic.type			= MTYPE_RADIOBUTTON;
	s_vrhud_display.lasersight.generic.name			= "Laser Sight:";
	s_vrhud_display.lasersight.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.lasersight.generic.callback		= VRHudDisplay_MenuEvent;
	s_vrhud_display.lasersight.generic.id			= ID_LASERSIGHT;
	s_vrhud_display.lasersight.generic.x			= VR_X_POS;
	s_vrhud_display.lasersight.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.supersampling.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud_display.supersampling.generic.name		= "Supersampling:";
	s_vrhud_display.supersampling.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.supersampling.generic.x			= VR_X_POS;
	s_vrhud_display.supersampling.generic.y			= y;
	s_vrhud_display.supersampling.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.supersampling.generic.id		= ID_SUPERSAMPLING;
	s_vrhud_display.supersampling.itemnames		= s_supersampling_names;
	s_vrhud_display.supersampling.numitems		= 13;

	// PC-only tail: Virtual screen shape
	if ( s_vrhud_display_isPC ) {
		y += BIGCHAR_HEIGHT+2;
		s_vrhud_display.virtualscreenshape.generic.type		= MTYPE_SPINCONTROL;
		s_vrhud_display.virtualscreenshape.generic.x		= VR_X_POS;
		s_vrhud_display.virtualscreenshape.generic.y		= y;
		s_vrhud_display.virtualscreenshape.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud_display.virtualscreenshape.generic.name		= "Virtual screen shape:";
		s_vrhud_display.virtualscreenshape.generic.id		= ID_VIRTUALSCREENSHAPE;
		s_vrhud_display.virtualscreenshape.generic.callback	= VRHudDisplay_MenuEvent;
		s_vrhud_display.virtualscreenshape.itemnames		= s_virtualScreenShapes;
		s_vrhud_display.virtualscreenshape.numitems			= 2;
	}

	if ( s_vrhud_display_isStandalone ) {
		y += BIGCHAR_HEIGHT+2;
		s_vrhud_display.screencurvature.generic.type		= MTYPE_SLIDER;
		s_vrhud_display.screencurvature.generic.x			= VR_X_POS;
		s_vrhud_display.screencurvature.generic.y			= y;
		s_vrhud_display.screencurvature.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud_display.screencurvature.generic.name		= "Screen Curvature:";
		s_vrhud_display.screencurvature.generic.id			= ID_SCREENCURVATURE;
		s_vrhud_display.screencurvature.generic.callback	= VRHudDisplay_MenuEvent;
		s_vrhud_display.screencurvature.minvalue			= 0.0f;
		s_vrhud_display.screencurvature.maxvalue			= 1.0f;
	}

	y += BIGCHAR_HEIGHT+2;
	s_vrhud_display.refreshrate.generic.type		= MTYPE_SPINCONTROL;
	s_vrhud_display.refreshrate.generic.name		= "Refresh Rate:";
	s_vrhud_display.refreshrate.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrhud_display.refreshrate.generic.x			= VR_X_POS;
	s_vrhud_display.refreshrate.generic.y			= y;
	s_vrhud_display.refreshrate.itemnames			= s_refreshRateItems;
	s_vrhud_display.refreshrate.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.refreshrate.generic.id			= ID_REFRESHRATE;
	s_vrhud_display.refreshrate.numitems			= s_numRefreshRates;

	if ( s_numFoveationItems > 0 ) {
		y += BIGCHAR_HEIGHT+2;
		s_vrhud_display.foveation.generic.type		= MTYPE_SPINCONTROL;
		s_vrhud_display.foveation.generic.name		= "Foveated Rendering:";
		s_vrhud_display.foveation.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud_display.foveation.generic.x			= VR_X_POS;
		s_vrhud_display.foveation.generic.y			= y;
		s_vrhud_display.foveation.itemnames			= s_foveationItems;
		s_vrhud_display.foveation.generic.callback	= VRHudDisplay_MenuEvent;
		s_vrhud_display.foveation.generic.id		= ID_FOVEATION;
		s_vrhud_display.foveation.numitems			= s_numFoveationItems;

		y += BIGCHAR_HEIGHT+2;
		s_vrhud_display.foveationstrength.generic.type		= MTYPE_SPINCONTROL;
		s_vrhud_display.foveationstrength.generic.name		= "Foveation Strength:";
		s_vrhud_display.foveationstrength.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_vrhud_display.foveationstrength.generic.x			= VR_X_POS;
		s_vrhud_display.foveationstrength.generic.y			= y;
		s_vrhud_display.foveationstrength.itemnames			= s_foveationStrengthItems;
		s_vrhud_display.foveationstrength.generic.callback	= VRHudDisplay_MenuEvent;
		s_vrhud_display.foveationstrength.generic.id		= ID_FOVEATIONSTRENGTH;
		s_vrhud_display.foveationstrength.numitems			= 3;
	}

	// shown only while the picked supersampling differs from the active one
	s_vrhud_display.apply.generic.type		= MTYPE_BITMAP;
	s_vrhud_display.apply.generic.name		= ART_ACCEPT0;
	s_vrhud_display.apply.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_vrhud_display.apply.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.apply.generic.id		= ID_APPLY;
	s_vrhud_display.apply.generic.x			= 640;
	s_vrhud_display.apply.generic.y			= 480-64;
	s_vrhud_display.apply.width				= 128;
	s_vrhud_display.apply.height			= 64;
	s_vrhud_display.apply.focuspic			= ART_ACCEPT1;

	s_vrhud_display.back.generic.type		= MTYPE_BITMAP;
	s_vrhud_display.back.generic.name		= ART_BACK0;
	s_vrhud_display.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vrhud_display.back.generic.callback	= VRHudDisplay_MenuEvent;
	s_vrhud_display.back.generic.id			= ID_BACK;
	s_vrhud_display.back.generic.x			= 0;
	s_vrhud_display.back.generic.y			= 480-64;
	s_vrhud_display.back.width				= 128;
	s_vrhud_display.back.height				= 64;
	s_vrhud_display.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.banner );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.framel );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.framer );

	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.hudmode );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.huddepth );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.hudscale );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.hudyoffset );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.virtualscreenmode );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.selectorwithhud );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.showinhand );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.showconsole );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.lasersight );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.supersampling );
	if ( s_vrhud_display_isPC ) {
		Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.virtualscreenshape );
	}
	if ( s_vrhud_display_isStandalone ) {
		Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.screencurvature );
	}
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.refreshrate );
	if ( s_numFoveationItems > 0 ) {
		Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.foveation );
		Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.foveationstrength );
	}

	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.apply );
	Menu_AddItem( &s_vrhud_display.menu, &s_vrhud_display.back );

	VRHudDisplay_SetMenuItems();
	VRHudDisplay_UpdateApply();
}


/*
===============
UI_VRHudDisplay_Cache
===============
*/
void UI_VRHudDisplay_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT0 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT1 );
}


/*
===============
UI_VRHudDisplayMenu
===============
*/
void UI_VRHudDisplayMenu( void ) {
	VRHudDisplay_MenuInit();
	UI_PushMenu( &s_vrhud_display.menu );
}
