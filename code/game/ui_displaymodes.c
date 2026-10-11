#include "q_shared.h"
#include "ui_displaymodes.h"

typedef struct { const char *name; int w, h; } dmRatio_t;

// display order after the desktop's own group; 5:4 is a legacy LCD shape
static const dmRatio_t dm_ratios[] = {
	{ "16:9", 16, 9 }, { "16:10", 16, 10 }, { "21:9", 21, 9 }, { "32:9", 32, 9 },
	{ "4:3", 4, 3 }, { "3:2", 3, 2 }, { "5:4", 5, 4 },
};
#define DM_NUM_RATIOS ( (int)( sizeof( dm_ratios ) / sizeof( dm_ratios[0] ) ) )
#define DM_RATIO_21_9 2

// Custom offers these, after Free
static const dmRatio_t dm_customRatios[] = {
	{ "16:9", 16, 9 }, { "16:10", 16, 10 }, { "21:9", 21, 9 }, { "32:9", 32, 9 }, { "4:3", 4, 3 },
};

const char *dm_customRatioNames[] = { "Free", "16:9", "16:10", "21:9", "32:9", "4:3", NULL };

// "21:9" covers the 64:27 and 43:18 panels sold under that name
static int DM_RatioIndex( int w, int h ) {
	float a = (float)w / h;
	int i;

	if ( a > 2.2f && a < 2.5f ) {
		return DM_RATIO_21_9;
	}
	for ( i = 0; i < DM_NUM_RATIOS; i++ ) {
		float r = (float)dm_ratios[i].w / dm_ratios[i].h;

		if ( a > r * 0.98f && a < r * 1.02f ) {
			return i;
		}
	}
	return -1;
}

static int DM_Gcd( int a, int b ) {
	while ( b ) {
		int t = a % b;
		a = b;
		b = t;
	}
	return a;
}

void DM_AspectLabel( int w, int h, char *out, int outSize ) {
	int i = DM_RatioIndex( w, h );

	if ( i >= 0 ) {
		Q_strncpyz( out, dm_ratios[i].name, outSize );
	} else {
		int g = DM_Gcd( w, h );
		Com_sprintf( out, outSize, "%d:%d", w / g, h / g );
	}
}

static int DM_GroupRank( int w, int h, int desktopGroup ) {
	int i = DM_RatioIndex( w, h );

	if ( i < 0 ) {
		return DM_NUM_RATIOS + 1;
	}
	return i == desktopGroup ? 0 : i + 1;
}

static int DM_ReadInt( const char **s ) {
	int v = 0;

	while ( **s >= '0' && **s <= '9' ) {
		v = v * 10 + ( **s - '0' );
		( *s )++;
	}
	return v;
}

void DM_BuildList( dmList_t *list, const char *availableModes, int desktopW, int desktopH ) {
	const char *s = availableModes;
	int desktopGroup = desktopW > 0 && desktopH > 0 ? DM_RatioIndex( desktopW, desktopH ) : -1;
	int i, j;

	list->numModes = 0;
	while ( s && *s && list->numModes < DM_MAX_MODES ) {
		dmMode_t *m = &list->modes[list->numModes];
		char aspect[16];

		Com_Memset( m, 0, sizeof( *m ) );
		while ( *s == ' ' ) {
			s++;
		}
		m->width = DM_ReadInt( &s );
		if ( *s != 'x' ) {
			break;
		}
		s++;
		m->height = DM_ReadInt( &s );
		if ( *s == ':' ) {
			do {
				int hz;

				s++;
				hz = DM_ReadInt( &s );
				if ( m->numRates < DM_MAX_RATES && hz > 0 ) {
					m->rates[m->numRates++] = hz;
				}
			} while ( *s == ',' );
		}
		if ( m->width > 0 && m->height > 0 ) {
			DM_AspectLabel( m->width, m->height, aspect, sizeof( aspect ) );
			Com_sprintf( m->label, sizeof( m->label ), "%dx%d (%s)", m->width, m->height, aspect );
			list->numModes++;
		}
	}

	// insertion sort: group rank, then area
	for ( i = 1; i < list->numModes; i++ ) {
		dmMode_t key = list->modes[i];
		int keyRank = DM_GroupRank( key.width, key.height, desktopGroup );

		for ( j = i - 1; j >= 0; j-- ) {
			const dmMode_t *o = &list->modes[j];
			int rank = DM_GroupRank( o->width, o->height, desktopGroup );

			if ( rank < keyRank || ( rank == keyRank && o->width * o->height <= key.width * key.height ) ) {
				break;
			}
			list->modes[j + 1] = *o;
		}
		list->modes[j + 1] = key;
	}
}

void DM_DesktopSize( const char *desktopMode, const char *availableModes, int *w, int *h ) {
	const char *s = desktopMode;
	dmList_t list;
	int i;

	*w = DM_ReadInt( &s );
	*h = 0;
	if ( *s == 'x' ) {
		s++;
		*h = DM_ReadInt( &s );
	}
	if ( *w > 0 && *h > 0 ) {
		return;
	}
	*w = *h = 0;
	DM_BuildList( &list, availableModes, 0, 0 );
	for ( i = 0; i < list.numModes; i++ ) {
		if ( list.modes[i].width * list.modes[i].height > *w * *h ) {
			*w = list.modes[i].width;
			*h = list.modes[i].height;
		}
	}
}

int DM_FindMode( const dmList_t *list, int w, int h ) {
	int i;

	for ( i = 0; i < list->numModes; i++ ) {
		if ( list->modes[i].width == w && list->modes[i].height == h ) {
			return i;
		}
	}
	return -1;
}

int DM_SavedRow( const dmList_t *list, int mode, int *w, int *h, int vidW, int vidH ) {
	int i;

	if ( mode <= -2 ) {
		return 0;
	}
	if ( mode >= 0 ) {
		if ( vidW <= 0 || vidH <= 0 ) {
			return 0;
		}
		*w = vidW;
		*h = vidH;
	}
	i = DM_FindMode( list, *w, *h );
	return i >= 0 ? i + 1 : list->numModes + 1;
}

static const dmRatio_t *DM_CustomRatio( int ratioIndex ) {
	if ( ratioIndex < 1 || ratioIndex > (int)( sizeof( dm_customRatios ) / sizeof( dm_customRatios[0] ) ) ) {
		return NULL;
	}
	return &dm_customRatios[ratioIndex - 1];
}

int DM_HeightForWidth( int ratioIndex, int width ) {
	const dmRatio_t *r = DM_CustomRatio( ratioIndex );

	return r ? ( ( width * r->h / r->w + 1 ) & ~1 ) : 0;
}

int DM_WidthForHeight( int ratioIndex, int height ) {
	const dmRatio_t *r = DM_CustomRatio( ratioIndex );

	return r ? ( ( height * r->w / r->h + 1 ) & ~1 ) : 0;
}

// a desktop size of 0 skips the fit test
static void DM_AddMirror( dmList_t *list, int w, int h, int desktopW, int desktopH, const char *tag ) {
	dmMode_t *m;

	if ( w <= 0 || h <= 0 || ( desktopW && w > desktopW ) || ( desktopH && h > desktopH ) ||
		list->numModes >= DM_MAX_MODES || DM_FindMode( list, w, h ) >= 0 ) {
		return;
	}
	m = &list->modes[list->numModes++];
	Com_Memset( m, 0, sizeof( *m ) );
	m->width = w;
	m->height = h;
	Com_sprintf( m->label, sizeof( m->label ), "%dx%d (%s)", w, h, tag );
}

void DM_BuildMirrorList( dmList_t *list, const char *availableModes, int desktopW, int desktopH, int eyeW, int eyeH ) {
	static const int stream[][2] = { { 1920, 1080 }, { 1280, 720 }, { 2560, 1440 }, { 3840, 2160 } };
	static const int square[] = { 1080, 1440, 2160 };
	static const int frac[][2] = { { 1, 2 }, { 2, 3 }, { 3, 4 } };
	dmList_t detected;
	int i;

	// capture takes the whole window even past the screen edge, so the fixed sizes ignore the desktop's
	list->numModes = 0;
	for ( i = 0; i < 4; i++ ) {
		DM_AddMirror( list, stream[i][0], stream[i][1], 0, 0, "16:9 stream" );
	}
	DM_AddMirror( list, 1080, 1920, 0, 0, "9:16 vertical" );
	DM_AddMirror( list, 720, 1280, 0, 0, "9:16 vertical" );
	// eye buffers are square or close; the headset's own size mirrors best
	if ( eyeW > 0 && eyeH > 0 && eyeW * 10 >= eyeH * 9 && eyeH * 10 >= eyeW * 9 ) {
		DM_AddMirror( list, eyeW & ~1, eyeH & ~1, 0, 0, "eye" );
	}
	for ( i = 0; i < 3; i++ ) {
		DM_AddMirror( list, square[i], square[i], 0, 0, "1:1" );
	}
	for ( i = 0; i < 3; i++ ) {
		char tag[16];

		Com_sprintf( tag, sizeof( tag ), "%d/%d desktop", frac[i][0], frac[i][1] );
		DM_AddMirror( list, ( desktopW * frac[i][0] / frac[i][1] ) & ~1, ( desktopH * frac[i][0] / frac[i][1] ) & ~1,
			desktopW, desktopH, tag );
	}
	DM_BuildList( &detected, availableModes, desktopW, desktopH );
	for ( i = 0; i < detected.numModes && list->numModes < DM_MAX_MODES; i++ ) {
		const dmMode_t *d = &detected.modes[i];

		if ( ( !desktopW || d->width <= desktopW ) && ( !desktopH || d->height <= desktopH ) && DM_FindMode( list, d->width, d->height ) < 0 ) {
			list->modes[list->numModes++] = *d;
		}
	}
}
