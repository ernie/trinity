// Copyright (C) 1999-2000 Id Software, Inc.
//
// HDR CALIBRATION MENU
#include "ui_local.h"

#define ID_PEAK			20	// fine slider (5-nit steps)
#define ID_HIGHLIGHT	21
#define ID_BACK			22

// highlight presets: cvar value ×10
static const int hdr_highlightValues[]	= { 5, 10, 15, 20, 25, 30, 35, 40 };
static const char* hdr_highlightNames[]	= { "0.5", "1.0", "1.5", "2.0", "2.5", "3.0", "3.5", "4.0", NULL };

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menuslider_s	peak;
	menulist_s		highlight;
	menubitmap_s	back;
} hdrCalibrationInfo_t;

static hdrCalibrationInfo_t	hdrInfo;

static void UI_HDR_Status( char *status, int size ) {
	trap_Cvar_VariableStringBuffer( "r_hdrStatus", status, size );
}

// nofbo still counts, so switching Frame Buffer on unlocks HDR before the restart
qboolean UI_HDR_Available( void ) {
	char status[32];
	UI_HDR_Status( status, sizeof( status ) );
	return (qboolean)( !Q_stricmp( status, "active" ) || !Q_stricmp( status, "available" ) ||
		!Q_stricmp( status, "nofbo" ) || !Q_stricmp( status, "ossetting" ) );
}

// calibration needs live HDR output, or output that only the OS HDR switch holds back
qboolean UI_HDR_CalibrationAvailable( void ) {
	char status[32];
	UI_HDR_Status( status, sizeof( status ) );
	return (qboolean)( !Q_stricmp( status, "active" ) || !Q_stricmp( status, "ossetting" ) );
}

// HDR output, bloom and MSAA need the frame buffer path; without it those cvars do nothing
qboolean UI_HDR_FBOOff( void ) {
	char buf[8];
	trap_Cvar_VariableStringBuffer( "r_fbo", buf, sizeof( buf ) );
	return (qboolean)( buf[0] != '\0' && atoi( buf ) == 0 );
}

// QVMs have no platform define; the engine's version string carries its OS ("win_mingw64-x86_64")
static qboolean UI_HDR_OnWindows( void ) {
	char buf[MAX_CVAR_VALUE_STRING];
	trap_Cvar_VariableStringBuffer( "version", buf, sizeof( buf ) );
	return (qboolean)( strstr( buf, " win_" ) != NULL );
}

static int UI_HDR_NearestIndex( int value, const int* table, int count ) {
	int i, best = 0, bestDiff = abs( value - table[0] );
	for ( i = 1; i < count; i++ ) {
		int diff = abs( value - table[i] );
		if ( diff < bestDiff ) { bestDiff = diff; best = i; }
	}
	return best;
}

static void UI_HDRCalibration_Event( void* ptr, int event ) {
	if ( event != QM_ACTIVATED ) {
		return;
	}
	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_PEAK:
		hdrInfo.peak.curvalue = (int)( hdrInfo.peak.curvalue + 0.5f );  // snap to a whole 5-nit step (a click can land mid-step)
		trap_Cvar_SetValue( "r_hdrPeak", hdrInfo.peak.curvalue * 5 );
		trap_Cvar_SetValue( "r_hdrPaperWhite", 0 );  // keep paper-white on auto; experts set it via console
		break;
	case ID_HIGHLIGHT:
		trap_Cvar_SetValue( "r_hdrHighlight", hdr_highlightValues[ hdrInfo.highlight.curvalue ] / 10.0f );
		break;
	case ID_BACK:
		trap_Cvar_SetValue( "r_hdrCalibrate", 0 );
		UI_PopMenu();
		break;
	}
}

static sfxHandle_t UI_HDRCalibration_Key( int key ) {
	if ( key == K_ESCAPE || key == K_MOUSE2 ) {
		trap_Cvar_SetValue( "r_hdrCalibrate", 0 );
	}
	return Menu_DefaultKey( &hdrInfo.menu, key );
}

static void UI_HDRCalibration_Draw( void ) {
	char nitsStr[32], status[32];
	vec4_t savedNormal, savedHigh;
	static vec4_t dimFocus = { 0.5f, 0.4f, 0.0f, 1.0f };

	// dark full-screen backdrop so the engine test pattern reads cleanly
	UI_FillRect( 0, 0, 640, 480, color_black );

	// Dim the framework control labels (Slider/SpinControl read these globals) so
	// only the test pattern is bright; restored right after: q3_ui draws serially.
	Vector4Copy( text_color_normal, savedNormal );
	Vector4Copy( text_color_highlight, savedHigh );
	Vector4Copy( text_color_disabled, text_color_normal );
	Vector4Copy( dimFocus, text_color_highlight );
	Menu_Draw( &hdrInfo.menu );
	Vector4Copy( savedNormal, text_color_normal );
	Vector4Copy( savedHigh, text_color_highlight );

	// display-only Peak readout; curvalue is float, so cast before %i
	Com_sprintf( nitsStr, sizeof( nitsStr ), "%i nits", (int)( hdrInfo.peak.curvalue * 5 ) );
	UI_DrawString( hdrInfo.peak.generic.x + 120, hdrInfo.peak.generic.y,
		nitsStr, UI_LEFT|UI_SMALLFONT, text_color_disabled );

	// focus-driven tooltip for Highlight (q3_ui has no hover tooltips)
	if ( Menu_ItemAtCursor( &hdrInfo.menu ) == (void*)&hdrInfo.highlight ) {
		UI_DrawString( 320, 288,
			"Extra pop for the brightest highlights. 1.0 = natural, higher is punchier.",
			UI_CENTER|UI_SMALLFONT, text_color_disabled );
	}

	// status + instructions below the test pattern
	UI_HDR_Status( status, sizeof( status ) );
	if ( !Q_stricmp( status, "active" ) ) {
		// The HDR test pattern renders only on the desktop mirror (it is meaningless
		// through the headset's SDR tonemap), so where the box would be, cue the
		// player to look at the monitor. This sits inside the box footprint, so on
		// the mirror it is hidden under the opaque pattern; only the headset sees it.
		UI_DrawString( 320, 168, "Test pattern is on the",   UI_CENTER|UI_SMALLFONT, text_color_disabled );
		UI_DrawString( 320, 184, "desktop mirror - view it",  UI_CENTER|UI_SMALLFONT, text_color_disabled );
		UI_DrawString( 320, 200, "on your monitor",           UI_CENTER|UI_SMALLFONT, text_color_disabled );

		UI_DrawString( 320, 312, "Raise Peak until the inner rectangle's edge vanishes into the outer.",
			UI_CENTER|UI_SMALLFONT, text_color_disabled );
		UI_DrawString( 320, 328, "HDR: Active", UI_CENTER|UI_SMALLFONT, text_color_disabled );
	} else if ( !Q_stricmp( status, "ossetting" ) ) {
		UI_DrawString( 320, 328, UI_HDR_OnWindows()
			? "HDR is off in Windows display settings - turn it on there, then restart video."
			: "HDR is off in your display settings - turn it on there, then restart video.",
			UI_CENTER|UI_SMALLFONT, color_red );
	}
}

static void UI_HDRCalibration_Init( void ) {
	int peak;

	memset( &hdrInfo, 0, sizeof( hdrInfo ) );
	UI_HDRCalibrationMenu_Cache();

	hdrInfo.menu.wrapAround = qtrue;
	hdrInfo.menu.fullscreen = qtrue;
	hdrInfo.menu.draw = UI_HDRCalibration_Draw;
	hdrInfo.menu.key = UI_HDRCalibration_Key;

	hdrInfo.banner.generic.type	= MTYPE_BTEXT;
	hdrInfo.banner.generic.flags = QMF_CENTER_JUSTIFY;
	hdrInfo.banner.generic.x	= 320;
	hdrInfo.banner.generic.y	= 16;
	hdrInfo.banner.string		= "HDR CALIBRATION";
	hdrInfo.banner.color		= text_color_disabled;
	hdrInfo.banner.style		= UI_CENTER;

	// fine peak slider: 50..400 in units of 5 nits (250..2000)
	hdrInfo.peak.generic.type		= MTYPE_SLIDER;
	hdrInfo.peak.generic.name		= "Peak Brightness (nits):";
	hdrInfo.peak.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	hdrInfo.peak.generic.callback	= UI_HDRCalibration_Event;
	hdrInfo.peak.generic.id			= ID_PEAK;
	hdrInfo.peak.generic.x			= 320;
	hdrInfo.peak.generic.y			= 235;
	hdrInfo.peak.minvalue			= 50;
	hdrInfo.peak.maxvalue			= 400;

	// stepped highlight: 0.5..4.0 in 0.5 steps
	hdrInfo.highlight.generic.type		= MTYPE_SPINCONTROL;
	hdrInfo.highlight.generic.name		= "Highlight:";
	hdrInfo.highlight.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	hdrInfo.highlight.generic.callback	= UI_HDRCalibration_Event;
	hdrInfo.highlight.generic.id		= ID_HIGHLIGHT;
	hdrInfo.highlight.generic.x			= 320;
	hdrInfo.highlight.generic.y			= 262;
	hdrInfo.highlight.itemnames			= hdr_highlightNames;

	hdrInfo.back.generic.type		= MTYPE_BITMAP;
	hdrInfo.back.generic.name		= "menu/art/back_0";
	hdrInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	hdrInfo.back.generic.callback	= UI_HDRCalibration_Event;
	hdrInfo.back.generic.id			= ID_BACK;
	hdrInfo.back.generic.x			= 0;
	hdrInfo.back.generic.y			= 480-64;
	hdrInfo.back.width				= 128;
	hdrInfo.back.height				= 64;
	hdrInfo.back.focuspic			= "menu/art/back_1";

	peak = (int)trap_Cvar_VariableValue( "r_hdrPeak" );
	hdrInfo.peak.curvalue = peak / 5;
	if ( hdrInfo.peak.curvalue < 50 )	hdrInfo.peak.curvalue = 50;
	if ( hdrInfo.peak.curvalue > 400 )	hdrInfo.peak.curvalue = 400;

	hdrInfo.highlight.curvalue = UI_HDR_NearestIndex( (int)( trap_Cvar_VariableValue( "r_hdrHighlight" ) * 10.0f ),
		hdr_highlightValues, ARRAY_LEN( hdr_highlightValues ) );

	Menu_AddItem( &hdrInfo.menu, &hdrInfo.banner );
	Menu_AddItem( &hdrInfo.menu, &hdrInfo.peak );
	Menu_AddItem( &hdrInfo.menu, &hdrInfo.highlight );
	Menu_AddItem( &hdrInfo.menu, &hdrInfo.back );
}

void UI_HDRCalibrationMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( "menu/art/back_0" );
	trap_R_RegisterShaderNoMip( "menu/art/back_1" );
}

void UI_HDRCalibrationMenu( void ) {
	trap_Cvar_SetValue( "r_hdrCalibrate", 1 );
	UI_HDRCalibration_Init();
	UI_PushMenu( &hdrInfo.menu );
}
