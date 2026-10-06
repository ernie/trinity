// VR bindings menu model shared by both UIs: rows, the bind rules, the focused column and the waiting state. No engine
// or library calls; the UI supplies all I/O.
#ifndef VR_BINDMENU_H
#define VR_BINDMENU_H

#define VRBM_KEYS 36
#define VRBM_ROW_COUNT 32
#define VRBM_BINDING 256
#define VRBM_CELL 16	// a cell's text: two key markers and " / " between them

typedef enum { VRBM_TAB_MOVE, VRBM_TAB_ACTIONS, VRBM_TAB_FOLLOW, VRBM_TAB_SYSTEM, VRBM_TABS } vrbmTab_t;
typedef enum { VRBM_COL_PLAIN, VRBM_COL_ALT, VRBM_COL_CLEAR } vrbmColumn_t;

#define VRBM_HEADER 1	// a section heading, no binding
#define VRBM_REQUIRED 2	// never left unbound: its clear restores the controller's default
#define VRBM_NOALT 4	// no Alt binding: its Alt cell shows a dash and takes no focus

typedef struct {
	int tab;
	const char *label;
	const char *command;
	const char *context;
	int flags;
	const char *command2;	// a second (layer, command) pair bound to the same key; NULL for one pair
	const char *context2;
} vrbmRow_t;

typedef struct {
	void ( *read )( const char *context, int key, char *buf, int size );	// empty when unbound
	void ( *bind )( const char *context, int key, const char *command );
	void ( *unbind )( const char *context, int key );
	void ( *cancelKeys )( char *buf, int size );							// the Menu button as key markers, "" for none
	void ( *capture )( void );												// arms the engine's one-shot capture
	int ( *defaults )( const char *context, const char *command, int keys[2] );	// this controller's default keys; their count
} vrbmIO_t;

typedef enum { VRBM_BOUND, VRBM_UNCHANGED, VRBM_REFUSED } vrbmResult_t;

typedef struct {
	int waiting;	// row being bound, -1 when idle
	int waitingAlt;	// the capture fills the Alt column
	int refused;	// VR key of the last refused press, -1 for none
	int column;		// the focused row's cell, a vrbmColumn_t
} vrbmState_t;

extern const vrbmRow_t vrbmRows[VRBM_ROW_COUNT];
extern vrbmState_t vrbm;

void VRBM_SetIO( const vrbmIO_t *io );
int VRBM_Editable( const vrbmRow_t *row, int alt );
int VRBM_Keys( const vrbmRow_t *row, int alt, int keys[2] );
// A cell's keys as vr_glyph markers, "" when unbound.
void VRBM_Cell( const vrbmRow_t *row, int alt, char *buf, int size );
// Cells are read once and kept: Tick lets them lapse every half second so outside changes show, Forget drops them now.
void VRBM_Tick( int timeMs );
void VRBM_Forget( void );

// The menu opened: nothing waits and the Plain column has focus.
void VRBM_Open( void );
// The cell row shows focused: Plain stands in for an Alt cell the row lacks.
int VRBM_Column( const vrbmRow_t *row );
// Left (-1) or right (+1) along row's cells, skipping an Alt cell the row lacks; stops at the ends.
void VRBM_Step( const vrbmRow_t *row, int dir );
// The pointer is over column of row: it takes focus unless it is an Alt cell the row lacks.
void VRBM_Point( const vrbmRow_t *row, int column );
// Enter or a click on row's focused cell: binds that column or clears the row.
void VRBM_Activate( int row );

int VRBM_Waiting( void );
void VRBM_Cancel( void );
vrbmResult_t VRBM_Capture( int key );
// Any other key while waiting: the engine dropped its capture (that key, focus loss), so arm it again.
void VRBM_OtherKey( void );
// The status line, with key markers; focusedRow is the row with focus, -1 for none.
void VRBM_Status( int focusedRow, char *buf, int size );

#endif
