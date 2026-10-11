#ifndef UI_DISPLAYMODES_H
#define UI_DISPLAYMODES_H

#define DM_MAX_MODES	40
#define DM_MAX_RATES	8

typedef struct {
	int		width, height;
	int		rates[DM_MAX_RATES];	// Hz, highest first
	int		numRates;
	char	label[40];				// "3440x1440 (21:9)"
} dmMode_t;

typedef struct {
	dmMode_t	modes[DM_MAX_MODES];	// grouped and ordered for display
	int			numModes;
} dmList_t;

// parses r_availableModes ("WxH:hz,hz ...") and orders it: the desktop's aspect
// group first, then 16:9, 16:10, 21:9, 32:9, 4:3, 3:2, 5:4, then unnamed ratios;
// each group smallest to largest
void DM_BuildList( dmList_t *list, const char *availableModes, int desktopW, int desktopH );

// desktop mirror sizes: 16:9 streaming, 9:16 vertical, the eye buffer and 1:1
// squares, desktop fractions, then DM_BuildList's sizes no larger than the
// desktop; no size twice
void DM_BuildMirrorList( dmList_t *list, const char *availableModes, int desktopW, int desktopH, int eyeW, int eyeH );

// "21:9" for a named ratio within 2%, else the reduced ratio
void DM_AspectLabel( int w, int h, char *out, int outSize );

// the desktop size from r_desktopMode ("WxH"), else the largest size in
// r_availableModes; 0x0 when neither has one
void DM_DesktopSize( const char *desktopMode, const char *availableModes, int *w, int *h );

// index of WxH in the list, or -1
int DM_FindMode( const dmList_t *list, int w, int h );

// the Resolution row for a saved r_mode: 0 Desktop, 1..numModes a size,
// numModes + 1 Custom. *w, *h carry r_customWidth/Height in and the size shown
// out; a table index (mode >= 0) shows vidW x vidH, the size it runs at.
int DM_SavedRow( const dmList_t *list, int mode, int *w, int *h, int vidW, int vidH );

// Custom's ratios; index 0 is Free
extern const char *dm_customRatioNames[];

// the other dimension for a Custom ratio, rounded to even pixels; 0 for Free
int DM_HeightForWidth( int ratioIndex, int width );
int DM_WidthForHeight( int ratioIndex, int height );

#endif
