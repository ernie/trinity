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

VR DESKTOP MIRROR OPTIONS MENU (PC-only)

=======================================================================
*/


#include "ui_local.h"
#include "../game/ui_displaymodes.h"


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"
#define ART_ACCEPT0				"menu/art/accept_0"
#define ART_ACCEPT1				"menu/art/accept_1"

#define VR_X_POS		320

// Vertical center of the framel/framer oval interior (640x480 space).
#define VR_FRAME_CENTER_Y	242

#define ID_DESKTOPMODE           150
#define ID_DESKTOPRESOLUTION     151
#define ID_DESKTOPCONTENTTYPE    152
#define ID_DESKTOPCONTENTFIT     153
#define ID_DESKTOPMENUSTYLE      154
#define ID_APPLY                 155
#define ID_BACK                  156


typedef struct {
	menuframework_s menu;

	menutext_s banner;
	menubitmap_s framel;
	menubitmap_s framer;

	menulist_s mode;
	menulist_s resolution;
	menulist_s contenttype;
	menulist_s contentfit;
	menulist_s menustyle;

	menubitmap_s apply;
	menubitmap_s back;
} vrmirror_t;

static vrmirror_t s_vrmirror;

static int s_ivo_desktopmode;
static int s_ivo_desktopresolution;

// Window Resolution rows: the mirror list, then Custom when the saved size is in none of it
static dmList_t		s_mirrorList;
static const char	*s_mirrorNames[DM_MAX_MODES + 2];
static char			s_mirrorCustom[40];
static int			s_mirrorCustomRow;

static void VRMirror_GetResolutions( void )
{
	char modes[MAX_STRING_CHARS], desktop[32];
	int dw, dh, w, h, i, n = 0;

	trap_Cvar_VariableStringBuffer( "r_availableModes", modes, sizeof( modes ) );
	trap_Cvar_VariableStringBuffer( "r_desktopMode", desktop, sizeof( desktop ) );
	DM_DesktopSize( desktop, modes, &dw, &dh );
	// under VR the renderer's size is the eye buffer
	DM_BuildMirrorList( &s_mirrorList, modes, dw, dh, uis.glconfig.vidWidth, uis.glconfig.vidHeight );
	for ( i = 0; i < s_mirrorList.numModes; i++ )
		s_mirrorNames[n++] = s_mirrorList.modes[i].label;

	w = trap_Cvar_VariableValue( "vr_mirrorWidth" );
	h = trap_Cvar_VariableValue( "vr_mirrorHeight" );
	i = DM_FindMode( &s_mirrorList, w, h );
	s_mirrorCustomRow = -1;
	if ( i < 0 && w > 0 && h > 0 ) {
		Com_sprintf( s_mirrorCustom, sizeof( s_mirrorCustom ), "Custom (%dx%d)", w, h );
		i = s_mirrorCustomRow = n;
		s_mirrorNames[n++] = s_mirrorCustom;
	}
	s_mirrorNames[n] = NULL;
	s_vrmirror.resolution.curvalue = s_ivo_desktopresolution = i < 0 ? 0 : i;
}

static void VRMirror_SetMenuItems( void ) {
	int mirror;
	int fullscreen;

	// s_vrmirror.resolution and s_ivo_resolution are handled elsewhere
	// Desktop mirror UI: 0=off, 1=windowed, 2=fullscreen
	// Reads vr_mirrorEnabled (0=off, 1=on) and vr_mirrorFullscreen (0=windowed, 1=fullscreen)
	mirror = trap_Cvar_VariableValue( "vr_mirrorEnabled" );
	fullscreen = trap_Cvar_VariableValue( "vr_mirrorFullscreen" );
	if (mirror == 0) {
		s_vrmirror.mode.curvalue = 0; // Off
	} else if (fullscreen == 0) {
		s_vrmirror.mode.curvalue = 1; // Windowed (mirror=1, fullscreen=0)
	} else {
		s_vrmirror.mode.curvalue = 2; // Fullscreen (mirror=1, fullscreen=1)
	}
	s_ivo_desktopmode = s_vrmirror.mode.curvalue;
	s_vrmirror.contenttype.curvalue = trap_Cvar_VariableValue( "vr_desktopContentType" );
	s_vrmirror.contentfit.curvalue = trap_Cvar_VariableValue( "vr_desktopContentFit" );
	s_vrmirror.menustyle.curvalue = trap_Cvar_VariableValue( "vr_desktopMenuStyle" );
}

static void VRMirror_UpdateMenuItems( void )
{
	// the size only shapes a windowed mirror
	if ( s_vrmirror.mode.curvalue == 1 )
		s_vrmirror.resolution.generic.flags &= ~QMF_GRAYED;
	else
		s_vrmirror.resolution.generic.flags |= QMF_GRAYED;

	s_vrmirror.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( s_ivo_desktopmode != s_vrmirror.mode.curvalue )
	{
		s_vrmirror.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	if ( s_ivo_desktopresolution != s_vrmirror.resolution.curvalue )
	{
		s_vrmirror.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
}

static void VRMirror_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	if (s_ivo_desktopmode != s_vrmirror.mode.curvalue)
	{
		// UI curvalue: 0=off, 1=windowed, 2=fullscreen
		// Set vr_mirrorEnabled (0=off, 1=on) and vr_mirrorFullscreen (0=windowed, 1=fullscreen)
		if (s_vrmirror.mode.curvalue == 0) {
			// Off
			trap_Cvar_SetValue( "vr_mirrorEnabled", 0 );
		} else if (s_vrmirror.mode.curvalue == 1) {
			// Windowed
			trap_Cvar_SetValue( "vr_mirrorEnabled", 1 );
			trap_Cvar_SetValue( "vr_mirrorFullscreen", 0 );
		} else {
			// Fullscreen
			trap_Cvar_SetValue( "vr_mirrorEnabled", 1 );
			trap_Cvar_SetValue( "vr_mirrorFullscreen", 1 );
		}
	}

	if ( s_ivo_desktopresolution != s_vrmirror.resolution.curvalue &&
		s_vrmirror.resolution.curvalue < s_mirrorList.numModes )
	{
		const dmMode_t *m = &s_mirrorList.modes[s_vrmirror.resolution.curvalue];

		trap_Cvar_SetValue( "vr_mirrorWidth", m->width );
		trap_Cvar_SetValue( "vr_mirrorHeight", m->height );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

static void VRMirror_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_DESKTOPMODE:
			// Will be applied in VRMirror_ApplyChanges
			break;

		case ID_DESKTOPRESOLUTION:
			// Will be applied in VRMirror_ApplyChanges
			break;

		case ID_DESKTOPCONTENTTYPE:
			trap_Cvar_SetValue( "vr_desktopContentType", s_vrmirror.contenttype.curvalue );
			break;

		case ID_DESKTOPCONTENTFIT:
			trap_Cvar_SetValue( "vr_desktopContentFit", s_vrmirror.contentfit.curvalue );
			break;

		case ID_DESKTOPMENUSTYLE:
			trap_Cvar_SetValue( "vr_desktopMenuStyle", s_vrmirror.menustyle.curvalue );
			break;

		case ID_APPLY:
			VRMirror_ApplyChanges( ptr, notification );
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}

	VRMirror_UpdateMenuItems();
}

static void VRMirror_MenuInit( void ) {
	int y;

	static const char *s_desktopModes[] =
	{
		"Off",
		"Windowed",
		"Fullscreen",
		NULL,
	};

	static const char *s_desktopContentTypes[] =
	{
		"Left eye",
		"Right eye",
		"Both eyes",
		NULL,
	};

	static const char *s_desktopContentFits[] =
	{
		"Fit / Contain",
		"Fill / Crop",
		NULL,
	};

	static const char *s_desktopMenuStyles[] =
	{
		"Desktop view",
		"VR view",
		NULL,
	};

	memset( &s_vrmirror, 0, sizeof(vrmirror_t) );

	VRMirror_GetResolutions();
	UI_VRMirror_Cache();

	s_vrmirror.menu.wrapAround = qtrue;
	s_vrmirror.menu.fullscreen = qtrue;

	s_vrmirror.banner.generic.type  = MTYPE_BTEXT;
	s_vrmirror.banner.generic.x     = 320;
	s_vrmirror.banner.generic.y     = 16;
	s_vrmirror.banner.string        = "DESKTOP MIRROR";
	s_vrmirror.banner.color         = color_white;
	s_vrmirror.banner.style         = UI_CENTER;

	s_vrmirror.framel.generic.type  = MTYPE_BITMAP;
	s_vrmirror.framel.generic.name  = ART_FRAMEL;
	s_vrmirror.framel.generic.flags = QMF_INACTIVE;
	s_vrmirror.framel.generic.x     = 0;
	s_vrmirror.framel.generic.y     = 78;
	s_vrmirror.framel.width         = 256;
	s_vrmirror.framel.height        = 329;

	s_vrmirror.framer.generic.type  = MTYPE_BITMAP;
	s_vrmirror.framer.generic.name  = ART_FRAMER;
	s_vrmirror.framer.generic.flags = QMF_INACTIVE;
	s_vrmirror.framer.generic.x     = 376;
	s_vrmirror.framer.generic.y     = 76;
	s_vrmirror.framer.width         = 256;
	s_vrmirror.framer.height        = 334;

	// Center the 5-row block (small-font rows) in the frame interior.
	y = VR_FRAME_CENTER_Y - ( 4 * (BIGCHAR_HEIGHT+2) + SMALLCHAR_HEIGHT ) / 2;
	s_vrmirror.mode.generic.type     = MTYPE_SPINCONTROL;
	s_vrmirror.mode.generic.x        = VR_X_POS;
	s_vrmirror.mode.generic.y        = y;
	s_vrmirror.mode.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrmirror.mode.generic.name     = "Mode:";
	s_vrmirror.mode.generic.id       = ID_DESKTOPMODE;
	s_vrmirror.mode.generic.callback = VRMirror_MenuEvent;
	s_vrmirror.mode.itemnames        = s_desktopModes;
	s_vrmirror.mode.numitems         = 3;

	y += BIGCHAR_HEIGHT+2;
	s_vrmirror.resolution.generic.type     = MTYPE_SPINCONTROL;
	s_vrmirror.resolution.generic.name     = "Window Resolution:";
	s_vrmirror.resolution.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_WRAP;
	s_vrmirror.resolution.generic.x        = VR_X_POS;
	s_vrmirror.resolution.generic.y        = y;
	s_vrmirror.resolution.itemnames        = s_mirrorNames;
	s_vrmirror.resolution.generic.callback = VRMirror_MenuEvent;
	s_vrmirror.resolution.generic.id       = ID_DESKTOPRESOLUTION;

	y += BIGCHAR_HEIGHT+2;
	s_vrmirror.contenttype.generic.type     = MTYPE_SPINCONTROL;
	s_vrmirror.contenttype.generic.x        = VR_X_POS;
	s_vrmirror.contenttype.generic.y        = y;
	s_vrmirror.contenttype.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrmirror.contenttype.generic.name     = "Content type:";
	s_vrmirror.contenttype.generic.id       = ID_DESKTOPCONTENTTYPE;
	s_vrmirror.contenttype.generic.callback = VRMirror_MenuEvent;
	s_vrmirror.contenttype.itemnames        = s_desktopContentTypes;
	s_vrmirror.contenttype.numitems         = 3;

	y += BIGCHAR_HEIGHT+2;
	s_vrmirror.contentfit.generic.type      = MTYPE_SPINCONTROL;
	s_vrmirror.contentfit.generic.x         = VR_X_POS;
	s_vrmirror.contentfit.generic.y         = y;
	s_vrmirror.contentfit.generic.flags     = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrmirror.contentfit.generic.name      = "Content fit:";
	s_vrmirror.contentfit.generic.id        = ID_DESKTOPCONTENTFIT;
	s_vrmirror.contentfit.generic.callback  = VRMirror_MenuEvent;
	s_vrmirror.contentfit.itemnames         = s_desktopContentFits;
	s_vrmirror.contentfit.numitems          = 2;

	y += BIGCHAR_HEIGHT+2;
	s_vrmirror.menustyle.generic.type     = MTYPE_SPINCONTROL;
	s_vrmirror.menustyle.generic.x        = VR_X_POS;
	s_vrmirror.menustyle.generic.y        = y;
	s_vrmirror.menustyle.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_vrmirror.menustyle.generic.name     = "Menu style:";
	s_vrmirror.menustyle.generic.id       = ID_DESKTOPMENUSTYLE;
	s_vrmirror.menustyle.generic.callback = VRMirror_MenuEvent;
	s_vrmirror.menustyle.itemnames        = s_desktopMenuStyles;
	s_vrmirror.menustyle.numitems         = 2;

	s_vrmirror.apply.generic.type     = MTYPE_BITMAP;
	s_vrmirror.apply.generic.name     = ART_ACCEPT0;
	s_vrmirror.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_vrmirror.apply.generic.callback = VRMirror_ApplyChanges;
	s_vrmirror.apply.generic.id       = ID_APPLY;
	s_vrmirror.apply.generic.x        = 640;
	s_vrmirror.apply.generic.y        = 480-64;
	s_vrmirror.apply.width            = 128;
	s_vrmirror.apply.height           = 64;
	s_vrmirror.apply.focuspic         = ART_ACCEPT1;

	s_vrmirror.back.generic.type      = MTYPE_BITMAP;
	s_vrmirror.back.generic.name      = ART_BACK0;
	s_vrmirror.back.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_vrmirror.back.generic.callback  = VRMirror_MenuEvent;
	s_vrmirror.back.generic.id        = ID_BACK;
	s_vrmirror.back.generic.x         = 0;
	s_vrmirror.back.generic.y         = 480-64;
	s_vrmirror.back.width             = 128;
	s_vrmirror.back.height            = 64;
	s_vrmirror.back.focuspic          = ART_BACK1;

	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.banner );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.framel );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.framer );

	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.mode  );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.resolution );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.contenttype );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.contentfit );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.menustyle );

	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.apply );
	Menu_AddItem( &s_vrmirror.menu, &s_vrmirror.back );

	VRMirror_SetMenuItems();
	VRMirror_UpdateMenuItems();
}


/*
===============
UI_VRMirror_Cache
===============
*/
void UI_VRMirror_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT0 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT1 );
}


/*
===============
UI_VRMirrorMenu
===============
*/
void UI_VRMirrorMenu( void ) {
	// a headset has no desktop to mirror to
	if ( UI_VR_Platform() != VRP_PC ) {
		return;
	}
	VRMirror_MenuInit();
	UI_PushMenu( &s_vrmirror.menu );
}
