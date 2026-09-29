#include "vr_supersample.h"

int VRSS_Tenths( float value ) {
	int tenths = (int)( value * 10.0f + 0.5f );
	if ( value <= 0 || tenths < VRSS_MIN_TENTHS )
		return VRSS_MIN_TENTHS;
	if ( tenths > VRSS_MAX_TENTHS )
		return VRSS_MAX_TENTHS;
	return tenths;
}

float VRSS_Value( int tenths ) {
	return tenths / 10.0f;
}

static const char *VRSS_ParseInt( const char *s, int *out ) {
	int n = 0, any = 0;
	while ( *s == ' ' )
		s++;
	while ( *s >= '0' && *s <= '9' ) {
		n = n * 10 + ( *s - '0' );
		any = 1;
		s++;
	}
	if ( !any )
		return 0;
	*out = n;
	return s;
}

int VRSS_ParseEyeSize( const char *text, int *recW, int *recH, int *maxW, int *maxH ) {
	int *outs[4];
	int i;
	outs[0] = recW;
	outs[1] = recH;
	outs[2] = maxW;
	outs[3] = maxH;
	for ( i = 0; i < 4; i++ ) {
		text = VRSS_ParseInt( text, outs[i] );
		if ( !text )
			return 0;
	}
	return *recW > 0 && *recH > 0;
}

static int VRSS_Put( char *buf, int size, int at, const char *s ) {
	while ( *s && at < size - 1 )
		buf[at++] = *s++;
	buf[at] = '\0';
	return at;
}

static int VRSS_PutInt( char *buf, int size, int at, int n ) {
	char digits[12];
	int i = 0;
	do {
		digits[i++] = (char)( '0' + n % 10 );
		n /= 10;
	} while ( n && i < (int)sizeof( digits ) - 1 );
	while ( i && at < size - 1 )
		buf[at++] = digits[--i];
	buf[at] = '\0';
	return at;
}

void VRSS_Multiplier( int tenths, char *buf, int size ) {
	int at;
	if ( size <= 0 )
		return;
	at = VRSS_PutInt( buf, size, 0, tenths / 10 );
	at = VRSS_Put( buf, size, at, "." );
	VRSS_PutInt( buf, size, at, tenths % 10 );
}

void VRSS_Resolution( int tenths, int recW, int recH, int maxW, int maxH, char *buf, int size ) {
	float factor = VRSS_Value( tenths ), limit;
	int at;
	if ( size <= 0 )
		return;
	buf[0] = 0;
	if ( recW <= 0 || recH <= 0 )
		return;
	/* the runtime's limit scales both axes together, as the engine does */
	if ( maxW > 0 && maxH > 0 ) {
		limit = (float)maxW / recW;
		if ( (float)maxH / recH < limit )
			limit = (float)maxH / recH;
		if ( factor > limit )
			factor = limit;
	}
	at = VRSS_PutInt( buf, size, 0, (int)( recW * factor ) );
	at = VRSS_Put( buf, size, at, "x" );
	VRSS_PutInt( buf, size, at, (int)( recH * factor ) );
}
