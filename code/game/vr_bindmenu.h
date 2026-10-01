// VR bindings menu model shared by both UIs: rows, the bind rules and the waiting state. No engine or library calls;
// the UI supplies all I/O.
#ifndef VR_BINDMENU_H
#define VR_BINDMENU_H

#define VRBM_KEYS 36
#define VRBM_ROW_COUNT 32
#define VRBM_BINDING 256

typedef enum { VRBM_TAB_MOVE, VRBM_TAB_ACTIONS, VRBM_TAB_FOLLOW, VRBM_TAB_SYSTEM, VRBM_TABS } vrbmTab_t;

#define VRBM_HEADER 1	// a section heading, no binding
#define VRBM_REQUIRED 2	// never left unbound: its X restores the controller's default
#define VRBM_NOALT 4	// shown grayed, not editable, while the Alt view is on

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
	void ( *keyName )( int key, char *buf, int size );						// display name
	void ( *cancelName )( char *buf, int size );							// the Menu button's display name
	void ( *capture )( void );												// arms the engine's one-shot capture
	int ( *defaults )( const char *context, const char *command, int keys[2] );	// this controller's default keys; their count
} vrbmIO_t;

typedef enum { VRBM_BOUND, VRBM_UNCHANGED, VRBM_REFUSED } vrbmResult_t;

typedef struct {
	int waiting;	// row being bound, -1 when idle
	int altView;
	int refused;	// VR key of the last refused press, -1 for none
} vrbmState_t;

extern const vrbmRow_t vrbmRows[VRBM_ROW_COUNT];
extern vrbmState_t vrbm;

void VRBM_SetIO( const vrbmIO_t *io );
const char *VRBM_Context( const vrbmRow_t *row, int altView );
int VRBM_Editable( const vrbmRow_t *row, int altView );
int VRBM_Keys( const vrbmRow_t *row, int altView, int keys[2] );
vrbmResult_t VRBM_Bind( const vrbmRow_t *row, int altView, int key );
void VRBM_Clear( const vrbmRow_t *row, int altView );
void VRBM_Names( const vrbmRow_t *row, int altView, char *buf, int size );
// Names are read once and kept: Tick lets them lapse every half second so outside changes show, Forget drops them now.
void VRBM_Tick( int timeMs );
void VRBM_Forget( void );

int VRBM_Waiting( void );
void VRBM_Start( int row );
void VRBM_Cancel( void );
vrbmResult_t VRBM_Capture( int key );
// Any other key while waiting: the engine dropped its capture (that key, focus loss), so arm it again.
void VRBM_OtherKey( void );
void VRBM_Status( char *buf, int size );

#endif
