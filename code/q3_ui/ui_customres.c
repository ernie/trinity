/*
=======================================================================

CUSTOM RESOLUTION

Pick an aspect ratio and type one side; the other fills in.

=======================================================================
*/

#include "ui_local.h"
#include "../game/ui_displaymodes.h"

#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"
#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_ACCEPT0		"menu/art/accept_0"
#define ART_ACCEPT1		"menu/art/accept_1"

#define ID_ASPECT		10
#define ID_ACCEPT		11
#define ID_BACK			12
#define ID_WIDTH		13
#define ID_HEIGHT		14

#define CUSTOMRES_MIN	320
#define CUSTOMRES_MAX	16384

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		aspect;
	menufield_s		width;
	menufield_s		height;

	menubitmap_s	accept;
	menubitmap_s	back;

	void			(*done)( int w, int h );
} customres_t;

static customres_t s_customres;

static void CustomRes_SetField( menufield_s *f, int value ) {
	Com_sprintf( f->field.buffer, sizeof( f->field.buffer ), "%d", value );
	f->field.cursor = strlen( f->field.buffer );
	f->field.scroll = 0;
}

// fills the other side from the one just typed in
static void CustomRes_Autofill( qboolean fromWidth ) {
	int ratio = s_customres.aspect.curvalue;
	int value;

	if ( ratio == 0 ) {
		return;
	}
	if ( !fromWidth ) {
		value = DM_WidthForHeight( ratio, atoi( s_customres.height.field.buffer ) );
		if ( value > 0 ) {
			CustomRes_SetField( &s_customres.width, value );
		}
	} else {
		value = DM_HeightForWidth( ratio, atoi( s_customres.width.field.buffer ) );
		if ( value > 0 ) {
			CustomRes_SetField( &s_customres.height, value );
		}
	}
}

static void CustomRes_Event( void *ptr, int event ) {
	int w, h;
	int id = ((menucommon_s *)ptr)->id;

	// a field's typing is done when it loses focus; Enter moves focus on
	if ( event == QM_LOSTFOCUS && ( id == ID_WIDTH || id == ID_HEIGHT ) ) {
		CustomRes_Autofill( id == ID_WIDTH );
		return;
	}
	if ( event != QM_ACTIVATED ) {
		return;
	}
	switch ( id ) {
	case ID_ASPECT:
		CustomRes_Autofill( qtrue );
		break;

	case ID_ACCEPT:
		w = Com_Clamp( CUSTOMRES_MIN, CUSTOMRES_MAX, atoi( s_customres.width.field.buffer ) );
		h = Com_Clamp( CUSTOMRES_MIN, CUSTOMRES_MAX, atoi( s_customres.height.field.buffer ) );
		UI_PopMenu();
		if ( s_customres.done ) {
			s_customres.done( w, h );
		}
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void CustomRes_MenuInit( int w, int h, void (*done)( int w, int h ) ) {
	static const char *ratio_names[8];
	int i;

	memset( &s_customres, 0, sizeof( s_customres ) );
	s_customres.done = done;
	for ( i = 0; dm_customRatioNames[i] && i < (int)ARRAY_LEN( ratio_names ) - 1; i++ ) {
		ratio_names[i] = dm_customRatioNames[i];
	}
	ratio_names[i] = NULL;

	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT0 );
	trap_R_RegisterShaderNoMip( ART_ACCEPT1 );

	s_customres.menu.wrapAround = qtrue;
	s_customres.menu.fullscreen = qtrue;

	s_customres.banner.generic.type	= MTYPE_BTEXT;
	s_customres.banner.generic.x	= 320;
	s_customres.banner.generic.y	= 16;
	s_customres.banner.string		= "CUSTOM RESOLUTION";
	s_customres.banner.color		= color_white;
	s_customres.banner.style		= UI_CENTER;

	s_customres.framel.generic.type		= MTYPE_BITMAP;
	s_customres.framel.generic.name		= ART_FRAMEL;
	s_customres.framel.generic.flags	= QMF_INACTIVE;
	s_customres.framel.generic.x		= 0;
	s_customres.framel.generic.y		= 78;
	s_customres.framel.width			= 256;
	s_customres.framel.height			= 329;

	s_customres.framer.generic.type		= MTYPE_BITMAP;
	s_customres.framer.generic.name		= ART_FRAMER;
	s_customres.framer.generic.flags	= QMF_INACTIVE;
	s_customres.framer.generic.x		= 376;
	s_customres.framer.generic.y		= 76;
	s_customres.framer.width			= 256;
	s_customres.framer.height			= 334;

	s_customres.aspect.generic.type		= MTYPE_SPINCONTROL;
	s_customres.aspect.generic.name		= "Aspect:";
	s_customres.aspect.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_customres.aspect.generic.callback	= CustomRes_Event;
	s_customres.aspect.generic.id		= ID_ASPECT;
	s_customres.aspect.generic.x		= 330;
	s_customres.aspect.generic.y		= 200;
	s_customres.aspect.itemnames		= ratio_names;

	s_customres.width.generic.type		= MTYPE_FIELD;
	s_customres.width.generic.name		= "Width:";
	s_customres.width.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_customres.width.generic.callback	= CustomRes_Event;
	s_customres.width.generic.id		= ID_WIDTH;
	s_customres.width.generic.x			= 330;
	s_customres.width.generic.y			= 224;
	s_customres.width.field.widthInChars	= 6;
	s_customres.width.field.maxchars		= 5;

	s_customres.height.generic.type		= MTYPE_FIELD;
	s_customres.height.generic.name		= "Height:";
	s_customres.height.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_customres.height.generic.callback	= CustomRes_Event;
	s_customres.height.generic.id		= ID_HEIGHT;
	s_customres.height.generic.x		= 330;
	s_customres.height.generic.y		= 248;
	s_customres.height.field.widthInChars	= 6;
	s_customres.height.field.maxchars		= 5;

	s_customres.accept.generic.type		= MTYPE_BITMAP;
	s_customres.accept.generic.name		= ART_ACCEPT0;
	s_customres.accept.generic.flags	= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_customres.accept.generic.callback	= CustomRes_Event;
	s_customres.accept.generic.id		= ID_ACCEPT;
	s_customres.accept.generic.x		= 640;
	s_customres.accept.generic.y		= 480-64;
	s_customres.accept.width			= 128;
	s_customres.accept.height			= 64;
	s_customres.accept.focuspic			= ART_ACCEPT1;

	s_customres.back.generic.type		= MTYPE_BITMAP;
	s_customres.back.generic.name		= ART_BACK0;
	s_customres.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_customres.back.generic.callback	= CustomRes_Event;
	s_customres.back.generic.id			= ID_BACK;
	s_customres.back.generic.x			= 0;
	s_customres.back.generic.y			= 480-64;
	s_customres.back.width				= 128;
	s_customres.back.height				= 64;
	s_customres.back.focuspic			= ART_BACK1;

	Menu_AddItem( &s_customres.menu, &s_customres.banner );
	Menu_AddItem( &s_customres.menu, &s_customres.framel );
	Menu_AddItem( &s_customres.menu, &s_customres.framer );
	Menu_AddItem( &s_customres.menu, &s_customres.aspect );
	Menu_AddItem( &s_customres.menu, &s_customres.width );
	Menu_AddItem( &s_customres.menu, &s_customres.height );
	Menu_AddItem( &s_customres.menu, &s_customres.accept );
	Menu_AddItem( &s_customres.menu, &s_customres.back );

	// Free keeps whatever both sides say
	s_customres.aspect.curvalue = 0;
	CustomRes_SetField( &s_customres.width, w > 0 ? w : 1920 );
	CustomRes_SetField( &s_customres.height, h > 0 ? h : 1080 );
}

/*
=================
UI_CustomResolutionMenu
=================
*/
void UI_CustomResolutionMenu( int w, int h, void (*done)( int w, int h ) ) {
	CustomRes_MenuInit( w, h, done );
	UI_PushMenu( &s_customres.menu );
	Menu_SetCursorToItem( &s_customres.menu, &s_customres.width );
}
