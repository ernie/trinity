// cg_headanim.h -- head animation files and playback; free of game types so it compiles on its own

#ifndef CG_HEADANIM_H
#define CG_HEADANIM_H

enum {
	HEAD_TALK,
	HEAD_TAUNT,
	HEAD_PAIN,
	HEAD_DEATH,
	HEAD_DROWN,
	HEAD_JUMP,
	HEAD_FALL,
	HEAD_FALLING,
	HEAD_GASP,
	HEAD_DEATH1,
	HEAD_DEATH2,
	HEAD_DEATH3,
	HEAD_NUM_ANIMS
};

#define HEAD_MAX_FX		4

typedef struct {
	int		firstFrame;
	int		numFrames;		// 0 when the head lacks this animation
	int		frameLerp;		// msec per frame
	int		reversed;
} headAnim_t;

typedef struct {
	int		oldFrame;
	int		frame;
	float	backlerp;		// as refEntity_t: 1 shows oldFrame, 0 shows frame
} headPose_t;

typedef struct {
	int			anim;			// HEAD_* playing on a timer, or -1
	int			startTime;
	float		openness;		// how far talking has the mouth open, 0 to 1
	int			blendFrom;		// when the last timed animation ended, 0 once blended back
	headPose_t	fromPose;
	int			fxTime[HEAD_MAX_FX];	// when each effect's sound last started, 0 when none
	int			fxPlaced[HEAD_MAX_FX];	// set once fxCard holds the first drawing's angle
	float		fxCard[HEAD_MAX_FX][6];	// right, up, then center from the beak, in screen-plane coordinates
} headState_t;

// a card of drawings shown at the beak when the animation's sound plays
typedef struct {
	int		anim;			// HEAD_* whose sound fires it
	char	prefix[64];		// drawing n is the shader <prefix>_<n>
	int		frames;
	int		msec;
	int		delay;			// msec from the sound to the first drawing
	float	width, height;
	float	pivotX, pivotY;	// the point placed at the beak, in card sizes from its center
	float	angle;			// degrees from the beak's direction on screen
	int		viewed;			// an fxview line shows the card in its own player's first person
	float	viewBelow;		// degrees below the view's center
	float	viewDistance;	// game units from the eye
	float	viewWidth;		// degrees of view the card's width spans
	float	viewSwing;		// share of the beak's swing from its rest direction the card follows
} headFx_t;

int CG_HeadParseAnims( const char *text, int headFrames, headAnim_t anims[HEAD_NUM_ANIMS], headFx_t fx[HEAD_MAX_FX],
	int *numFx, char *warn, int warnSize );
int CG_HeadFxFrame( const headFx_t *fx, int msecSinceSound );
// beakDepth is the beak's direction along the view, positive pointing away from the camera
void CG_HeadFxCard( const headFx_t *fx, const float beakOnScreen[2], float beakDepth, float right[2], float up[2],
	float center[2] );
// axis: view forward, left, up; swing in degrees, yaw right, pitch up; returns the card's scale
float CG_HeadFxView( const headFx_t *fx, const float eye[3], const float axis[3][3], float swingYaw, float swingPitch,
	float point[3], float fan[2] );
void CG_HeadFxSwing( const float rest[3], const float now[3], const float axis[3][3], float *yaw, float *pitch );
int CG_HeadMd3FrameCount( const unsigned char *header, int length );

float CG_HeadTalkTarget( int voipLevel );
float CG_HeadOpenness( float current, float target, int msec );
void CG_HeadTalkPose( const headAnim_t *talk, float openness, headPose_t *pose );
int CG_HeadTimedPose( const headAnim_t *anim, int msecSinceStart, int hold, int pingpong, headPose_t *pose );
int CG_HeadAnimForSound( const char *soundName );
void CG_HeadBlendPose( const headPose_t *from, const headPose_t *to, float t, headPose_t *out );
int CG_HeadSoundStarts( const headAnim_t anims[HEAD_NUM_ANIMS], int currentAnim, int dead, int soundAnim );
void CG_HeadPoseFor( const headAnim_t anims[HEAD_NUM_ANIMS], headState_t *hs, int corpse, int deathNum, int time,
	headPose_t *pose );

#endif
