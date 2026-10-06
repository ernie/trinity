#include "vr_glyph.h"

#define VRG_NAMES_MS 500
#define VRG_RUN 256

// Same order as NAMES in tools/generate_vr_glyphs.py; the index is the atlas cell.
static const char *const vrgNames[VRG_CELLS] = {
	"trigger_l", "trigger_r", "grip_l", "grip_r", "gripclick_l", "gripclick_r", "thumbrest_l", "thumbrest_r",
	"bumper_l", "bumper_r", "trackpad_l", "trackpad_r", "a_l", "a_r", "b_l", "b_r",
	"stickclick_l", "stickclick_r", "stick_up_l", "stick_down_l", "stick_left_l", "stick_right_l",
	"stick_up_r", "stick_down_r", "stick_left_r", "stick_right_r",
	"a", "b", "x", "y", "menu", "view", "dpad_up", "dpad_down", "dpad_left", "dpad_right",
};

// Quake's eight text colors (g_color_table), so a glyph after ^3 is tinted like the text around it.
static const float vrgColors[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
									  {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}};

static vrgIO_t io;
static int keyFirst = -1;	// the engine's first VR key code, -1 until it answers
static int cells[VRG_KEYS];	// cell + 2; 0 not asked yet, 1 asked and unnamed
static int cellsTime;

static int VRG_Equals( const char *a, const char *b ) {
	while ( *a && *a == *b ) {
		a++;
		b++;
	}
	return *a == *b;
}

static void VRG_Append( char *buf, int size, const char *text ) {
	int n = 0;
	while ( n < size - 1 && buf[n] )
		n++;
	while ( n < size - 1 && *text )
		buf[n++] = *text++;
	buf[n] = '\0';
}

static void VRG_AppendInt( char *buf, int size, int value ) {
	char digits[12], text[12];
	int n = 0, i;
	if ( value < 0 ) {
		VRG_Append( buf, size, "-" );
		value = -value;
	}
	do {
		digits[n++] = (char)( '0' + value % 10 );
		value /= 10;
	} while ( value && n < 11 );
	for ( i = 0; i < n; i++ )
		text[i] = digits[n - 1 - i];
	text[n] = '\0';
	VRG_Append( buf, size, text );
}

// Digits at s; *end lands on the first character after them.
static int VRG_ParseInt( const char *s, const char **end ) {
	int value = 0;
	while ( *s >= '0' && *s <= '9' )
		value = value * 10 + *s++ - '0';
	*end = s;
	return value;
}

void VRG_SetIO( const vrgIO_t *table ) {
	io = *table;
	VRG_Forget();
}

void VRG_Forget( void ) {
	int i;
	keyFirst = -1;
	for ( i = 0; i < VRG_KEYS; i++ )
		cells[i] = 0;
}

void VRG_Tick( int timeMs ) {
	if ( timeMs - cellsTime >= VRG_NAMES_MS || timeMs < cellsTime ) {
		cellsTime = timeMs;
		VRG_Forget();
	}
}

int VRG_Cell( const char *name ) {
	int i;
	for ( i = 0; i < VRG_CELLS; i++ )
		if ( VRG_Equals( name, vrgNames[i] ) )
			return i;
	return -1;
}

static int VRG_KeyFirst( void ) {
	char value[16];
	const char *end;
	if ( keyFirst < 0 && io.query && io.query( "vr_keyfirst", value, sizeof( value ) ) && value[0] )
		keyFirst = VRG_ParseInt( value, &end );
	return keyFirst;
}

// The atlas cell of a VR key index, -1 when the engine names none.
static int VRG_KeyCell( int key ) {
	char query[32], name[32];
	if ( key < 0 || key >= VRG_KEYS )
		return -1;
	if ( !cells[key] ) {
		cells[key] = 1;
		if ( VRG_KeyFirst() >= 0 ) {
			query[0] = '\0';
			VRG_Append( query, sizeof( query ), "vr_keyglyph " );
			VRG_AppendInt( query, sizeof( query ), keyFirst + key );
			if ( io.query( query, name, sizeof( name ) ) && VRG_Cell( name ) >= 0 )
				cells[key] = VRG_Cell( name ) + 2;
		}
	}
	return cells[key] - 2;
}

void VRG_AppendKey( char *buf, int size, int key ) {
	int n = 0;
	if ( key < 0 || key >= VRG_KEYS )
		return;
	while ( n < size - 1 && buf[n] )
		n++;
	if ( n + 2 > size - 1 )
		return;
	buf[n] = VRG_MARK;
	buf[n + 1] = (char)( key + 1 );
	buf[n + 2] = '\0';
}

int VRG_KeysFor( const char *context, const char *command, char *buf, int size ) {
	char query[160], value[32];
	const char *p;
	int key, count = 0, ok = 1;
	buf[0] = '\0';
	VRG_Forget();
	if ( VRG_KeyFirst() < 0 )
		return 0;
	query[0] = '\0';
	VRG_Append( query, sizeof( query ), "vr_bindkeys " );
	VRG_Append( query, sizeof( query ), context );
	VRG_Append( query, sizeof( query ), " " );
	VRG_Append( query, sizeof( query ), command );
	if ( !io.query( query, value, sizeof( value ) ) )
		return 0;
	for ( p = value; *p && ok; ) {
		if ( *p < '0' || *p > '9' ) {
			ok = 0;
			break;
		}
		key = VRG_ParseInt( p, &p ) - keyFirst;
		if ( key < 0 || key >= VRG_KEYS ) {
			ok = 0;
			break;
		}
		if ( count++ )
			VRG_Append( buf, size, "+" );
		VRG_AppendKey( buf, size, key );
		if ( *p == '+' && p[1] )
			p++;
		else if ( *p )
			ok = 0;
	}
	if ( !ok || !count ) {	// a reply we can't read names nothing, so the caller keeps its own text
		buf[0] = '\0';
		return 0;
	}
	return count;
}

// Walks text's runs and markers, measuring and, when paint is set, drawing; returns the width.
static float VRG_Layout( const vrgFont_t *font, float x, float y, const float *color, const char *text, int paint ) {
	char run[VRG_RUN];
	float tint[4];
	const float start = x;
	int n = 0, code = -1, c, key, cell;
	for ( ;; text++ ) {
		c = *text & 255;
		if ( c && c != VRG_MARK ) {
			if ( c == '^' && text[1] && text[1] != '^' )
				code = ( text[1] - '0' ) & 7;
			if ( n < VRG_RUN - 1 )
				run[n++] = (char)c;
			continue;
		}
		run[n] = '\0';
		if ( n ) {
			if ( paint )
				font->text( x, y, run, color, font->user );
			x += font->width( run, font->user );
		}
		n = 0;
		if ( !c || !text[1] )
			break;
		key = ( text[1] & 255 ) - 1;
		cell = VRG_KeyCell( key );
		text++;
		if ( cell < 0 )
			continue;
		if ( paint ) {
			const float quad = font->glyph * VRG_CELL_SCALE, inset = ( quad - font->glyph ) / 2;
			if ( code >= 0 ) {
				tint[0] = vrgColors[code][0];
				tint[1] = vrgColors[code][1];
				tint[2] = vrgColors[code][2];
				tint[3] = color[3];
			}
			io.cell( x - inset, y + font->glyphY - inset, quad, cell, code >= 0 ? tint : color );
		}
		x += font->glyph;
		// the next run restates the active color, since each run's draw starts from the caller's
		if ( code >= 0 ) {
			run[n++] = '^';
			run[n++] = (char)( '0' + code );
		}
	}
	return x - start;
}

float VRG_Width( const vrgFont_t *font, const char *text ) {
	return VRG_Layout( font, 0, 0, 0, text, 0 );
}

void VRG_Paint( const vrgFont_t *font, float x, float y, const float *color, const char *text ) {
	VRG_Layout( font, x, y, color, text, 1 );
}
