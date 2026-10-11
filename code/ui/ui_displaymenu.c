/*
=======================================================================

System menu display rows: Resolution, Custom Size, Color Depth and Refresh Rate

=======================================================================
*/

#include "ui_local.h"
#include "../game/ui_displaymodes.h"

static dmList_t	dm_list;
static int		dm_desktopW, dm_desktopH;
static qboolean	dm_built;
// Custom picked, even when its size is also a list entry; kept while the popup closes and reopens the menu
static qboolean	dm_customPicked;

static vec4_t	dm_grayed = { 0.5f, 0.5f, 0.5f, 1.0f };

void UI_Display_RefreshModes( void ) {
	char modes[MAX_STRING_CHARS], desktop[32];

	trap_Cvar_VariableStringBuffer( "r_availableModes", modes, sizeof( modes ) );
	trap_Cvar_VariableStringBuffer( "r_desktopMode", desktop, sizeof( desktop ) );
	DM_DesktopSize( desktop, modes, &dm_desktopW, &dm_desktopH );
	DM_BuildList( &dm_list, modes, dm_desktopW, dm_desktopH );
	dm_built = qtrue;
}

static void UI_Display_Build( void ) {
	if ( !dm_built ) {
		UI_Display_RefreshModes();
	}
}

// the saved size: r_modeFullscreen when fullscreen reads it, else r_mode
static int UI_Display_SavedMode( void ) {
	char fs[8];

	trap_Cvar_VariableStringBuffer( "r_modeFullscreen", fs, sizeof( fs ) );
	if ( (int)trap_Cvar_VariableValue( "r_fullscreen" ) && fs[0] ) {
		return atoi( fs );
	}
	return (int)trap_Cvar_VariableValue( "r_mode" );
}

// rows: 0 Desktop, 1..numModes the detected sizes, numModes + 1 Custom;
// *w, *h get the size the row shows
static int UI_Display_ResolutionRow( int *w, int *h ) {
	int mode = UI_Display_SavedMode();
	int vidW = uiInfo.uiDC.glconfig.vidWidth, vidH = uiInfo.uiDC.glconfig.vidHeight;

	*w = (int)trap_Cvar_VariableValue( "r_customWidth" );
	*h = (int)trap_Cvar_VariableValue( "r_customHeight" );
	if ( mode == -1 && dm_customPicked ) {
		return dm_list.numModes + 1;
	}
	// Borderless runs at the desktop size, which says nothing about a table index
	if ( (int)trap_Cvar_VariableValue( "r_fullscreen" ) == 2 ) {
		vidW = vidH = 0;
	}
	return DM_SavedRow( &dm_list, mode, w, h, vidW, vidH );
}

static int UI_Display_ResolutionIndex( void ) {
	int w, h;

	return UI_Display_ResolutionRow( &w, &h );
}

// the detected mode the saved size stands for; Desktop uses the desktop's own entry
static const dmMode_t *UI_Display_SelectedMode( void ) {
	int res = UI_Display_ResolutionIndex();
	int i;

	if ( res == 0 ) {
		i = DM_FindMode( &dm_list, dm_desktopW, dm_desktopH );
		return i >= 0 ? &dm_list.modes[i] : NULL;
	}
	return res <= dm_list.numModes ? &dm_list.modes[res - 1] : NULL;
}

static qboolean UI_Display_Borderless( void ) {
	return (int)trap_Cvar_VariableValue( "r_fullscreen" ) == 2;
}

static qboolean UI_Display_RefreshActive( void ) {
	const dmMode_t *m = UI_Display_SelectedMode();

	return (int)trap_Cvar_VariableValue( "r_fullscreen" ) == 1 && m && m->numRates > 0;
}

static void UI_Display_SetMode( const char *mode ) {
	trap_Cvar_Set( "r_mode", mode );
	trap_Cvar_Set( "r_modeFullscreen", mode );
}

// seeds the Custom Resolution editor; the menu that runs it shows the editor its own way
static void UI_Display_OpenCustom( void ) {
	int w = (int)trap_Cvar_VariableValue( "r_customWidth" );
	int h = (int)trap_Cvar_VariableValue( "r_customHeight" );

	trap_Cvar_Set( "ui_customW", va( "%i", w > 0 ? w : 1920 ) );
	trap_Cvar_Set( "ui_customH", va( "%i", h > 0 ? h : 1080 ) );
	trap_Cvar_Set( "ui_customRatio", "0" );
}

static void UI_Display_DrawResolution( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	const char *s;
	int res, w, h;

	UI_Display_Build();
	if ( UI_Display_Borderless() ) {
		uiInfo.uiDC.drawText( rect->x, rect->y, scale, dm_grayed, "Desktop", 0, 0, textStyle );
		return;
	}
	res = UI_Display_ResolutionRow( &w, &h );
	if ( res == 0 ) {
		s = "Desktop";
	} else if ( res <= dm_list.numModes ) {
		s = dm_list.modes[res - 1].label;
	} else {
		s = va( "Custom (%ix%i)", w, h );
	}
	uiInfo.uiDC.drawText( rect->x, rect->y, scale, color, s, 0, 0, textStyle );
}

static qboolean UI_Display_ResolutionKey( int key ) {
	int select = UI_SelectForKey( key );
	int res, count;

	UI_Display_Build();
	if ( select == 0 || UI_Display_Borderless() ) {
		return qfalse;
	}
	count = dm_list.numModes + 2;
	res = ( UI_Display_ResolutionIndex() + select + count ) % count;
	dm_customPicked = ( res == count - 1 );
	if ( res == 0 ) {
		UI_Display_SetMode( "-2" );
	} else if ( res <= dm_list.numModes ) {
		trap_Cvar_SetValue( "r_customWidth", dm_list.modes[res - 1].width );
		trap_Cvar_SetValue( "r_customHeight", dm_list.modes[res - 1].height );
		UI_Display_SetMode( "-1" );
	} else {
		int w, h;

		// a table index becomes the size it runs at; Custom Size edits it
		UI_Display_ResolutionRow( &w, &h );
		if ( w > 0 && h > 0 ) {
			trap_Cvar_SetValue( "r_customWidth", w );
			trap_Cvar_SetValue( "r_customHeight", h );
		}
		UI_Display_SetMode( "-1" );
	}
	// a rate belongs to its size
	trap_Cvar_Set( "r_displayRefresh", "0" );
	return qtrue;
}

static qboolean UI_Display_CustomActive( void ) {
	int w, h;

	return !UI_Display_Borderless() && UI_Display_ResolutionRow( &w, &h ) == dm_list.numModes + 1;
}

static void UI_Display_DrawCustomSize( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	int w = (int)trap_Cvar_VariableValue( "r_customWidth" );
	int h = (int)trap_Cvar_VariableValue( "r_customHeight" );

	UI_Display_Build();
	uiInfo.uiDC.drawText( rect->x, rect->y, scale, UI_Display_CustomActive() ? color : dm_grayed,
		w > 0 && h > 0 ? va( "%ix%i", w, h ) : "Set...", 0, 0, textStyle );
}

// only Enter or a click runs the row's action, which opens the editor; stepping through the menu never does
static qboolean UI_Display_CustomSizeKey( int key ) {
	UI_Display_Build();
	return UI_Display_CustomActive() && ( key == K_MOUSE1 || key == K_ENTER || key == K_KP_ENTER );
}

// only exclusive fullscreen sets a display mode, so only it picks a color depth
static const int	dm_colorBits[] = { 0, 16, 32 };
static const char	*dm_colorNames[] = { "Desktop Default", "16-bit", "32-bit" };

static int UI_Display_ColorIndex( void ) {
	int bits = (int)trap_Cvar_VariableValue( "r_colorbits" );
	int i;

	for ( i = 0; i < (int)ARRAY_LEN( dm_colorBits ); i++ ) {
		if ( dm_colorBits[i] == bits ) {
			return i;
		}
	}
	return 0;
}

static void UI_Display_DrawColorDepth( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	qboolean active = (int)trap_Cvar_VariableValue( "r_fullscreen" ) == 1;

	uiInfo.uiDC.drawText( rect->x, rect->y, scale, active ? color : dm_grayed,
		dm_colorNames[active ? UI_Display_ColorIndex() : 0], 0, 0, textStyle );
}

static qboolean UI_Display_ColorDepthKey( int key ) {
	int select = UI_SelectForKey( key );
	int count = ARRAY_LEN( dm_colorBits );

	if ( select == 0 || (int)trap_Cvar_VariableValue( "r_fullscreen" ) != 1 ) {
		return qfalse;
	}
	trap_Cvar_SetValue( "r_colorbits", dm_colorBits[( UI_Display_ColorIndex() + select + count ) % count] );
	return qtrue;
}

static void UI_Display_DrawRefresh( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	int hz;

	UI_Display_Build();
	hz = (int)trap_Cvar_VariableValue( "r_displayRefresh" );
	uiInfo.uiDC.drawText( rect->x, rect->y, scale, UI_Display_RefreshActive() ? color : dm_grayed,
		hz > 0 ? va( "%i Hz", hz ) : "Default", 0, 0, textStyle );
}

static qboolean UI_Display_RefreshKey( int key ) {
	const dmMode_t *m;
	int select = UI_SelectForKey( key );
	int hz, i, cur = 0;

	UI_Display_Build();
	if ( select == 0 || !UI_Display_RefreshActive() ) {
		return qfalse;
	}
	m = UI_Display_SelectedMode();
	hz = (int)trap_Cvar_VariableValue( "r_displayRefresh" );
	for ( i = 0; i < m->numRates; i++ ) {
		if ( m->rates[i] == hz ) {
			cur = i + 1;
		}
	}
	// rows: 0 Default, then the rates
	cur = ( cur + select + m->numRates + 1 ) % ( m->numRates + 1 );
	trap_Cvar_SetValue( "r_displayRefresh", cur ? m->rates[cur - 1] : 0 );
	return qtrue;
}

// fills one side of the Custom popup from the other once its typing ends
static void UI_Display_FillCustom( qboolean fromWidth ) {
	int ratio = (int)trap_Cvar_VariableValue( "ui_customRatio" );
	int other;

	if ( fromWidth ) {
		other = DM_HeightForWidth( ratio, (int)trap_Cvar_VariableValue( "ui_customW" ) );
		if ( other > 0 ) {
			trap_Cvar_Set( "ui_customH", va( "%i", other ) );
		}
	} else {
		other = DM_WidthForHeight( ratio, (int)trap_Cvar_VariableValue( "ui_customH" ) );
		if ( other > 0 ) {
			trap_Cvar_Set( "ui_customW", va( "%i", other ) );
		}
	}
}

qboolean UI_Display_OwnerDraw( rectDef_t *rect, int ownerDraw, float scale, vec4_t color, int textStyle ) {
	switch ( ownerDraw ) {
	case UI_RESOLUTION:
		UI_Display_DrawResolution( rect, scale, color, textStyle );
		return qtrue;
	case UI_DISPLAYREFRESH:
		UI_Display_DrawRefresh( rect, scale, color, textStyle );
		return qtrue;
	case UI_CUSTOMSIZE:
		UI_Display_DrawCustomSize( rect, scale, color, textStyle );
		return qtrue;
	case UI_COLORDEPTH:
		UI_Display_DrawColorDepth( rect, scale, color, textStyle );
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean UI_Display_OwnerDrawHandleKey( int ownerDraw, int key ) {
	switch ( ownerDraw ) {
	case UI_RESOLUTION:
		return UI_Display_ResolutionKey( key );
	case UI_DISPLAYREFRESH:
		return UI_Display_RefreshKey( key );
	case UI_CUSTOMSIZE:
		return UI_Display_CustomSizeKey( key );
	case UI_COLORDEPTH:
		return UI_Display_ColorDepthKey( key );
	default:
		return qfalse;
	}
}

qboolean UI_Display_RunMenuScript( const char *name ) {
	if ( Q_stricmp( name, "refreshModes" ) == 0 ) {
		UI_Display_RefreshModes();
		return qtrue;
	}
	if ( Q_stricmp( name, "customResOpen" ) == 0 ) {
		UI_Display_OpenCustom();
		return qtrue;
	}
	if ( Q_stricmp( name, "customResFillWidth" ) == 0 || Q_stricmp( name, "customResFillHeight" ) == 0 ) {
		UI_Display_FillCustom( Q_stricmp( name, "customResFillWidth" ) == 0 );
		return qtrue;
	}
	if ( Q_stricmp( name, "customResAccept" ) == 0 ) {
		int w = Com_Clamp( 320, 16384, (int)trap_Cvar_VariableValue( "ui_customW" ) );
		int h = Com_Clamp( 320, 16384, (int)trap_Cvar_VariableValue( "ui_customH" ) );

		trap_Cvar_SetValue( "r_customWidth", w );
		trap_Cvar_SetValue( "r_customHeight", h );
		UI_Display_SetMode( "-1" );
		dm_customPicked = qtrue;
		return qtrue;
	}
	return qfalse;
}
