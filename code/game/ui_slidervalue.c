#include "ui_slidervalue.h"

static int SliderValue_Round( float x ) {
	return x < 0 ? -(int)( -x + 0.5f ) : (int)( x + 0.5f );
}

int SliderValue_StepCount( float min, float max, float step ) {
	if ( step <= 0 || max <= min )
		return 0;
	return SliderValue_Round( ( max - min ) / step );
}

int SliderValue_Index( float value, float min, float max, float step ) {
	int count = SliderValue_StepCount( min, max, step );
	int index;
	if ( count <= 0 )
		return 0;
	index = SliderValue_Round( ( value - min ) / step );
	if ( index < 0 )
		return 0;
	if ( index > count )
		return count;
	return index;
}

float SliderValue_Snap( float value, float min, float max, float step ) {
	int index;
	if ( step <= 0 ) {
		if ( value < min )
			return min;
		if ( value > max )
			return max;
		return value;
	}
	index = SliderValue_Index( value, min, max, step );
	if ( index == SliderValue_StepCount( min, max, step ) )
		return max;
	return min + index * step;
}

int SliderValue_KeySteps( float base, float step ) {
	int steps;
	if ( step <= 0 )
		return 1;
	steps = SliderValue_Round( base / step );
	return steps < 1 ? 1 : steps;
}

static int SliderValue_Put( char *buf, int size, int at, const char *s ) {
	while ( *s && at < size - 1 )
		buf[at++] = *s++;
	buf[at] = '\0';
	return at;
}

static int SliderValue_PutInt( char *buf, int size, int at, int n, int width ) {
	char digits[12];
	int i = 0;
	do {
		digits[i++] = (char)( '0' + n % 10 );
		n /= 10;
	} while ( ( n || i < width ) && i < (int)sizeof( digits ) - 1 );
	while ( i && at < size - 1 )
		buf[at++] = digits[--i];
	buf[at] = '\0';
	return at;
}

/* scaled is the value in units of 10^-decimals, already rounded */
static int SliderValue_PutFixed( char *buf, int size, int scaled, int decimals, const char *prefix ) {
	int unit = decimals == 2 ? 100 : decimals == 1 ? 10 : 1;
	int at = SliderValue_Put( buf, size, 0, scaled < 0 ? "-" : prefix );
	if ( scaled < 0 )
		scaled = -scaled;
	at = SliderValue_PutInt( buf, size, at, scaled / unit, 1 );
	if ( decimals ) {
		at = SliderValue_Put( buf, size, at, "." );
		at = SliderValue_PutInt( buf, size, at, scaled % unit, decimals );
	}
	return at;
}

void SliderValue_Text( int format, float value, char *buf, int size ) {
	int n, at;
	if ( size <= 0 )
		return;
	buf[0] = '\0';
	switch ( format ) {
	case SLIDER_VALUE_INT:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value ), 0, "" );
		break;
	case SLIDER_VALUE_SIGNED:
		n = SliderValue_Round( value );
		SliderValue_PutFixed( buf, size, n, 0, n > 0 ? "+" : "" );
		break;
	case SLIDER_VALUE_DEC1:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value * 10 ), 1, "" );
		break;
	case SLIDER_VALUE_DEC2:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value * 100 ), 2, "" );
		break;
	case SLIDER_VALUE_PERCENT:
		at = SliderValue_PutFixed( buf, size, SliderValue_Round( value * 100 ), 0, "" );
		SliderValue_Put( buf, size, at, "%" );
		break;
	}
}

void SliderValue_CvarString( int format, float value, char *buf, int size ) {
	if ( size <= 0 )
		return;
	buf[0] = '\0';
	switch ( format ) {
	case SLIDER_VALUE_INT:
	case SLIDER_VALUE_SIGNED:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value ), 0, "" );
		break;
	case SLIDER_VALUE_DEC1:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value * 10 ), 1, "" );
		break;
	case SLIDER_VALUE_DEC2:
	case SLIDER_VALUE_PERCENT:
		SliderValue_PutFixed( buf, size, SliderValue_Round( value * 100 ), 2, "" );
		break;
	}
}
