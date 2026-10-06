// VR button glyphs: maps the engine's glyph names to cells of the gfx/vr/glyphs atlas and lays out strings that mix
// text with key markers. No engine or library calls; each module supplies the I/O and its own text drawing.
#ifndef VR_GLYPH_H
#define VR_GLYPH_H

#define VRG_MARK '\x01'	// a key marker: this byte, then a byte holding the VR key index + 1
#define VRG_KEYS 36		// VR keys, in the engine's K_VR_* order
#define VRG_CELLS 36
#define VRG_GRID 8		// atlas cells per row
#define VRG_CELL_SCALE ( 4.0f / 3.0f )	// a cell's 64 pixels hold the glyph's 48 plus padding against mip bleed

typedef struct {
	int ( *query )( const char *key, char *buf, int size );	// the engine's GetValue: nonzero when it answered
	void ( *cell )( float x, float y, float size, int cell, const float *color );	// one atlas cell, tinted, 640x480 units
} vrgIO_t;

typedef struct {
	float glyph;	// glyph size, 640x480 units
	float glyphY;	// glyph top relative to the y the string is painted at
	float ( *width )( const char *run, void *user );	// a run without markers; color codes take no width
	void ( *text )( float x, float y, const char *run, const float *color, void *user );	// color codes honored
	void *user;
} vrgFont_t;

void VRG_SetIO( const vrgIO_t *io );
// Key glyphs are asked once and kept: Tick lets them lapse every half second, Forget drops them now.
void VRG_Tick( int timeMs );
void VRG_Forget( void );
int VRG_Cell( const char *name );	// the atlas cell of a glyph name, -1 when unknown
void VRG_AppendKey( char *buf, int size, int key );	// appends key's marker when it fits whole
// The VR keys that run command in context's stack as markers, an Alt combo as "<alt>+<key>"; 0 and "" when unbound.
// Drops the cached glyphs first, so a prompt shown now names the hands as they are now.
int VRG_KeysFor( const char *context, const char *command, char *buf, int size );
float VRG_Width( const vrgFont_t *font, const char *text );
void VRG_Paint( const vrgFont_t *font, float x, float y, const float *color, const char *text );

#endif
