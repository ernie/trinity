// cg_headanim.c -- head animation files and playback

#include "cg_headanim.h"

double sin( double x );
double cos( double x );
double sqrt( double x );
double atan2( double y, double x );

static const char *headAnimNames[HEAD_NUM_ANIMS] = {
	"TALK", "TAUNT", "PAIN", "DEATH", "DROWN", "JUMP", "FALL", "FALLING", "GASP", "DEATH1", "DEATH2", "DEATH3"
};

static int CG_HeadUpper( int c ) {
	return ( c >= 'a' && c <= 'z' ) ? c - 'a' + 'A' : c;
}

static int CG_HeadNameIs( const char *a, const char *b ) {
	while ( *a && *b ) {
		if ( CG_HeadUpper( *a ) != CG_HeadUpper( *b ) ) {
			return 0;
		}
		a++;
		b++;
	}
	return *a == *b;
}

static int CG_HeadSpace( int c ) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static const char *CG_HeadToken( const char *p, char *token, int size ) {
	int n = 0;

	for ( ;; ) {
		while ( CG_HeadSpace( *p ) ) {
			p++;
		}
		if ( p[0] != '/' || p[1] != '/' ) {
			break;
		}
		while ( *p && *p != '\n' ) {
			p++;
		}
	}
	while ( *p && !CG_HeadSpace( *p ) ) {
		if ( n < size - 1 ) {
			token[n++] = *p;
		}
		p++;
	}
	token[n] = '\0';
	return p;
}

static float CG_HeadNumber( const char *s ) {
	float	value = 0.0f, scale = 1.0f;
	int		negative = 0, fraction = 0;

	if ( *s == '-' ) {
		negative = 1;
		s++;
	}
	for ( ; *s; s++ ) {
		if ( *s == '.' ) {
			fraction = 1;
		} else if ( *s >= '0' && *s <= '9' ) {
			if ( fraction ) {
				scale *= 0.1f;
				value += ( *s - '0' ) * scale;
			} else {
				value = value * 10.0f + ( *s - '0' );
			}
		} else {
			break;
		}
	}
	return negative ? -value : value;
}

static void CG_HeadWarn( char *warn, int warnSize, const char *problem, const char *name ) {
	int n = 0;

	if ( warn[0] ) {
		return;
	}
	while ( *problem && n < warnSize - 1 ) {
		warn[n++] = *problem++;
	}
	while ( *name && n < warnSize - 1 ) {
		warn[n++] = *name++;
	}
	warn[n] = '\0';
}

static int CG_HeadAnimNamed( const char *name ) {
	int i;

	for ( i = 0; i < HEAD_NUM_ANIMS; i++ ) {
		if ( CG_HeadNameIs( name, headAnimNames[i] ) ) {
			return i;
		}
	}
	return -1;
}

static float CG_HeadClamp( float value, float low, float high, char *warn, int warnSize ) {
	if ( value < low || value > high ) {
		CG_HeadWarn( warn, warnSize, "clamped a value for ", "fx" );
		return value < low ? low : high;
	}
	return value;
}

static const char *CG_HeadParseFx( const char *text, headFx_t fx[HEAD_MAX_FX], int *numFx, char *warn, int warnSize ) {
	char		anim[64], prefix[64], token[64];
	float		v[8];
	headFx_t	*f;
	int			i, k;

	text = CG_HeadToken( text, anim, sizeof( anim ) );
	text = CG_HeadToken( text, prefix, sizeof( prefix ) );
	for ( k = 0; k < 8; k++ ) {
		text = CG_HeadToken( text, token, sizeof( token ) );
		if ( !token[0] ) {
			CG_HeadWarn( warn, warnSize, "missing fields for fx ", anim );
			return text;
		}
		v[k] = CG_HeadNumber( token );
	}
	if ( CG_HeadAnimNamed( anim ) < 0 ) {
		CG_HeadWarn( warn, warnSize, "unknown head animation for fx ", anim );
		return text;
	}
	if ( !fx ) {
		return text;
	}
	if ( *numFx == HEAD_MAX_FX ) {
		CG_HeadWarn( warn, warnSize, "too many fx lines; dropped ", anim );
		return text;
	}
	f = &fx[( *numFx )++];
	f->anim = CG_HeadAnimNamed( anim );
	for ( i = 0; prefix[i]; i++ ) {
		f->prefix[i] = prefix[i];
	}
	f->prefix[i] = '\0';
	f->frames = (int)CG_HeadClamp( v[0], 1, 8, warn, warnSize );
	f->msec = (int)CG_HeadClamp( v[1], 50, 3000, warn, warnSize );
	f->delay = (int)CG_HeadClamp( v[2], 0, 3000, warn, warnSize );
	f->width = CG_HeadClamp( v[3], 0.5f, 64.0f, warn, warnSize );
	f->height = CG_HeadClamp( v[4], 0.5f, 64.0f, warn, warnSize );
	f->pivotX = v[5];
	f->pivotY = v[6];
	f->angle = v[7];
	f->viewed = 0;
	return text;
}

typedef struct {
	int		anim;
	float	below, distance, width, swing;
} headFxView_t;

static const char *CG_HeadParseFxView( const char *text, headFxView_t views[HEAD_MAX_FX], int *numViews, char *warn,
		int warnSize ) {
	char			anim[64], token[64];
	float			v[4];
	headFxView_t	*view;
	int				k;

	text = CG_HeadToken( text, anim, sizeof( anim ) );
	for ( k = 0; k < 4; k++ ) {
		text = CG_HeadToken( text, token, sizeof( token ) );
		if ( !token[0] ) {
			CG_HeadWarn( warn, warnSize, "missing fields for fxview ", anim );
			return text;
		}
		v[k] = CG_HeadNumber( token );
	}
	if ( CG_HeadAnimNamed( anim ) < 0 ) {
		CG_HeadWarn( warn, warnSize, "unknown head animation for fxview ", anim );
		return text;
	}
	if ( *numViews == HEAD_MAX_FX ) {
		CG_HeadWarn( warn, warnSize, "too many fxview lines; dropped ", anim );
		return text;
	}
	view = &views[( *numViews )++];
	view->anim = CG_HeadAnimNamed( anim );
	view->below = CG_HeadClamp( v[0], -90.0f, 90.0f, warn, warnSize );
	view->distance = CG_HeadClamp( v[1], 1.0f, 256.0f, warn, warnSize );
	view->width = CG_HeadClamp( v[2], 1.0f, 90.0f, warn, warnSize );
	view->swing = CG_HeadClamp( v[3], 0.0f, 2.0f, warn, warnSize );
	return text;
}

static void CG_HeadApplyFxViews( const headFxView_t *views, int numViews, headFx_t *fx, int numFx, char *warn,
		int warnSize ) {
	int i, k, matched;

	for ( i = 0; i < numViews; i++ ) {
		matched = 0;
		for ( k = 0; k < numFx; k++ ) {
			if ( fx[k].anim == views[i].anim ) {
				fx[k].viewed = 1;
				fx[k].viewBelow = views[i].below;
				fx[k].viewDistance = views[i].distance;
				fx[k].viewWidth = views[i].width;
				fx[k].viewSwing = views[i].swing;
				matched = 1;
			}
		}
		if ( !matched ) {
			CG_HeadWarn( warn, warnSize, "no fx line for fxview ", headAnimNames[views[i].anim] );
		}
	}
}

/*
======================
CG_HeadParseAnims
======================
*/
int CG_HeadParseAnims( const char *text, int headFrames, headAnim_t anims[HEAD_NUM_ANIMS], headFx_t fx[HEAD_MAX_FX],
		int *numFx, char *warn, int warnSize ) {
	char			name[64], token[64];
	float			numbers[4], first, count, fps;
	headAnim_t		*a;
	headFxView_t	views[HEAD_MAX_FX];
	int				i, k, defined = 0, numViews = 0;

	for ( i = 0; i < HEAD_NUM_ANIMS; i++ ) {
		anims[i].firstFrame = anims[i].numFrames = anims[i].reversed = 0;
		anims[i].frameLerp = 1000;
	}
	if ( numFx ) {
		*numFx = 0;
	}
	if ( warnSize > 0 ) {
		warn[0] = '\0';
	}

	for ( ;; ) {
		text = CG_HeadToken( text, name, sizeof( name ) );
		if ( !name[0] ) {
			break;
		}
		if ( CG_HeadNameIs( name, "fx" ) ) {
			text = CG_HeadParseFx( text, numFx ? fx : 0, numFx, warn, warnSize );
			continue;
		}
		if ( CG_HeadNameIs( name, "fxview" ) ) {
			text = CG_HeadParseFxView( text, views, &numViews, warn, warnSize );
			continue;
		}
		for ( k = 0; k < 4; k++ ) {
			text = CG_HeadToken( text, token, sizeof( token ) );
			if ( !token[0] ) {
				CG_HeadWarn( warn, warnSize, "missing numbers for ", name );
				if ( numFx ) {
					CG_HeadApplyFxViews( views, numViews, fx, *numFx, warn, warnSize );
				}
				return defined;
			}
			numbers[k] = CG_HeadNumber( token );
		}
		i = CG_HeadAnimNamed( name );
		if ( i < 0 ) {
			CG_HeadWarn( warn, warnSize, "unknown head animation ", name );
			continue;
		}

		a = &anims[i];
		// range checks stay in float so absurd counts can't overflow an int
		first = numbers[0];
		count = numbers[1];
		a->reversed = count < 0.0f;
		if ( count < 0.0f ) {
			count = -count;
		}
		a->numFrames = 0;
		if ( first < 0.0f || count < 1.0f || first >= headFrames ) {
			if ( count >= 1.0f ) {
				CG_HeadWarn( warn, warnSize, "frames out of range for ", name );
			}
			continue;
		}
		if ( first + count > headFrames ) {
			CG_HeadWarn( warn, warnSize, "frames past the head's last frame for ", name );
			count = headFrames - first;
		}
		a->firstFrame = (int)first;
		a->numFrames = (int)count;
		fps = numbers[3];
		if ( fps <= 0.0f ) {
			fps = 1.0f;
		}
		a->frameLerp = (int)( 1000.0f / fps );
		if ( a->frameLerp < 1 ) {
			a->frameLerp = 1;
		}
		defined++;
	}
	if ( numFx ) {
		CG_HeadApplyFxViews( views, numViews, fx, *numFx, warn, warnSize );
	}
	return defined;
}

#define HEAD_RISE_MSEC		40
#define HEAD_FALL_MSEC		120

static const float headTalkTargets[] = { 0.0f, 0.4f, 0.6f, 0.8f, 1.0f };

static const struct {
	const char	*name;
	int			anim;
} headSounds[] = {
	{ "*death1", HEAD_DEATH1 }, { "*death2", HEAD_DEATH2 }, { "*death3", HEAD_DEATH3 },
	{ "*drown", HEAD_DROWN },
	{ "*pain25_1", HEAD_PAIN }, { "*pain50_1", HEAD_PAIN }, { "*pain75_1", HEAD_PAIN }, { "*pain100_1", HEAD_PAIN },
	{ "sound/player/gurp1", HEAD_PAIN }, { "sound/player/gurp2", HEAD_PAIN },
	{ "*jump1", HEAD_JUMP },
	{ "*fall1", HEAD_FALL },
	{ "*falling1", HEAD_FALLING },
	{ "*gasp", HEAD_GASP },
	{ "*taunt", HEAD_TAUNT }
};

/*
======================
CG_HeadMd3FrameCount
======================
*/
int CG_HeadMd3FrameCount( const unsigned char *header, int length ) {
	int frames;

	if ( length < 80 || header[0] != 'I' || header[1] != 'D' || header[2] != 'P' || header[3] != '3'
			|| header[4] != 15 || header[5] || header[6] || header[7] ) {
		return -1;
	}
	frames = header[76] | ( header[77] << 8 ) | ( header[78] << 16 ) | ( header[79] << 24 );
	return frames < 0 ? -1 : frames;
}

/*
======================
CG_HeadTalkTarget
======================
*/
float CG_HeadTalkTarget( int voipLevel ) {
	if ( voipLevel <= 0 ) {
		return 0.0f;
	}
	if ( voipLevel >= 4 ) {
		return 1.0f;
	}
	return headTalkTargets[voipLevel];
}

/*
======================
CG_HeadOpenness
======================
*/
float CG_HeadOpenness( float current, float target, int msec ) {
	int tau = target > current ? HEAD_RISE_MSEC : HEAD_FALL_MSEC;

	if ( msec <= 0 ) {
		return current;
	}
	// dt / ( tau + dt ): steady across frame rates; the QVM lacks exp
	return current + ( target - current ) * msec / (float)( tau + msec );
}

static int CG_HeadLastFrame( const headAnim_t *anim ) {
	return anim->firstFrame + anim->numFrames - 1;
}

/*
======================
CG_HeadTalkPose
======================
*/
void CG_HeadTalkPose( const headAnim_t *talk, float openness, headPose_t *pose ) {
	float	position;
	int		step, last = talk->numFrames - 1;

	if ( openness < 0.0f ) {
		openness = 0.0f;
	} else if ( openness > 1.0f ) {
		openness = 1.0f;
	}
	position = openness * last;
	step = (int)position;
	if ( step > last ) {
		step = last;
	}
	pose->backlerp = 1.0f - ( position - step );
	if ( talk->reversed ) {
		pose->oldFrame = CG_HeadLastFrame( talk ) - step;
		pose->frame = step < last ? pose->oldFrame - 1 : pose->oldFrame;
	} else {
		pose->oldFrame = talk->firstFrame + step;
		pose->frame = step < last ? pose->oldFrame + 1 : pose->oldFrame;
	}
}

static int CG_HeadSequenceFrame( const headAnim_t *anim, int index, int pingpong ) {
	if ( pingpong && index >= anim->numFrames ) {
		index = 2 * anim->numFrames - 2 - index;
	}
	return anim->reversed ? CG_HeadLastFrame( anim ) - index : anim->firstFrame + index;
}

/*
======================
CG_HeadTimedPose
======================
*/
int CG_HeadTimedPose( const headAnim_t *anim, int msecSinceStart, int hold, int pingpong, headPose_t *pose ) {
	int length = pingpong ? 2 * anim->numFrames - 1 : anim->numFrames;
	int index, next;

	if ( msecSinceStart < 0 ) {
		msecSinceStart = 0;
	}
	index = msecSinceStart / anim->frameLerp;
	if ( index >= length ) {
		if ( !hold ) {
			return 0;
		}
		index = length - 1;
		msecSinceStart = index * anim->frameLerp;
	}
	next = index + 1 < length ? index + 1 : index;
	pose->oldFrame = CG_HeadSequenceFrame( anim, index, pingpong );
	pose->frame = CG_HeadSequenceFrame( anim, next, pingpong );
	pose->backlerp = 1.0f - (float)( msecSinceStart - index * anim->frameLerp ) / anim->frameLerp;
	return 1;
}

static int CG_HeadSoundIs( const char *soundName, const char *name ) {
	while ( *name && CG_HeadUpper( *soundName ) == CG_HeadUpper( *name ) ) {
		soundName++;
		name++;
	}
	return !*name && ( !*soundName || CG_HeadNameIs( soundName, ".wav" ) );
}

/*
======================
CG_HeadAnimForSound
======================
*/
int CG_HeadAnimForSound( const char *soundName ) {
	int i;

	for ( i = 0; i < (int)( sizeof( headSounds ) / sizeof( headSounds[0] ) ); i++ ) {
		if ( CG_HeadSoundIs( soundName, headSounds[i].name ) ) {
			return headSounds[i].anim;
		}
	}
	return -1;
}

static int CG_HeadVisibleFrame( const headPose_t *pose ) {
	return pose->backlerp >= 0.5f ? pose->oldFrame : pose->frame;
}

/*
======================
CG_HeadBlendPose
======================
*/
void CG_HeadBlendPose( const headPose_t *from, const headPose_t *to, float t, headPose_t *out ) {
	if ( t < 0.0f ) {
		t = 0.0f;
	} else if ( t > 1.0f ) {
		t = 1.0f;
	}
	out->oldFrame = CG_HeadVisibleFrame( from );
	out->frame = CG_HeadVisibleFrame( to );
	out->backlerp = 1.0f - t;
}

#define HEAD_BLEND_MSEC		100
#define HEAD_FX_BLEND		0.5f	// the beak's screen length, sin of its angle off the view, where blending starts

static int CG_HeadIsDeath( int anim ) {
	return anim == HEAD_DEATH || anim == HEAD_DEATH1 || anim == HEAD_DEATH2 || anim == HEAD_DEATH3;
}

static int CG_HeadHolds( int anim ) {
	return CG_HeadIsDeath( anim ) || anim == HEAD_DROWN;
}

/*
======================
CG_HeadSoundStarts
======================
*/
int CG_HeadSoundStarts( const headAnim_t anims[HEAD_NUM_ANIMS], int currentAnim, int dead, int soundAnim ) {
	// a dead player still lands and surfaces, and those sounds must not break the death hold
	if ( soundAnim < 0 || ( dead && CG_HeadHolds( currentAnim ) && !CG_HeadHolds( soundAnim ) ) ) {
		return -1;
	}
	if ( anims[soundAnim].numFrames ) {
		return soundAnim;
	}
	if ( CG_HeadIsDeath( soundAnim ) && anims[HEAD_DEATH].numFrames ) {
		return HEAD_DEATH;
	}
	return anims[HEAD_TALK].numFrames ? HEAD_TALK : -1;
}

static void CG_HeadDeathPose( const headAnim_t anims[HEAD_NUM_ANIMS], int deathNum, headPose_t *pose ) {
	const headAnim_t *death = &anims[HEAD_DEATH1 + deathNum - 1];

	if ( !death->numFrames ) {
		death = &anims[HEAD_DEATH];
	}
	if ( death->numFrames ) {
		CG_HeadTimedPose( death, death->numFrames * death->frameLerp, 1, 0, pose );
	}
}

/*
======================
CG_HeadPoseFor
======================
*/
void CG_HeadPoseFor( const headAnim_t anims[HEAD_NUM_ANIMS], headState_t *hs, int corpse, int deathNum, int time,
		headPose_t *pose ) {
	const headAnim_t	*anim;
	headPose_t			talkPose;
	int					pingpong, length;
	float				t;

	pose->oldFrame = pose->frame = 0;
	pose->backlerp = 0.0f;

	// corpses never hear their death sound, so they hold the death pose directly
	if ( corpse ) {
		if ( deathNum ) {
			CG_HeadDeathPose( anims, deathNum, pose );
		}
		return;
	}

	if ( hs->anim >= 0 && ( !anims[hs->anim].numFrames || ( CG_HeadHolds( hs->anim ) && !deathNum ) ) ) {
		hs->anim = -1;
		hs->blendFrom = 0;
	}
	if ( hs->anim >= 0 ) {
		anim = &anims[hs->anim];
		pingpong = hs->anim == HEAD_TALK;
		if ( CG_HeadTimedPose( anim, time - hs->startTime, CG_HeadHolds( hs->anim ), pingpong, pose ) ) {
			return;
		}
		length = ( pingpong ? 2 * anim->numFrames - 1 : anim->numFrames ) * anim->frameLerp;
		CG_HeadTimedPose( anim, length, 1, pingpong, &hs->fromPose );
		hs->anim = -1;
		hs->blendFrom = hs->startTime + length;
	}

	if ( deathNum ) {
		CG_HeadDeathPose( anims, deathNum, pose );
		return;
	}

	talkPose.oldFrame = talkPose.frame = 0;
	talkPose.backlerp = 0.0f;
	if ( anims[HEAD_TALK].numFrames ) {
		CG_HeadTalkPose( &anims[HEAD_TALK], hs->openness, &talkPose );
	}
	if ( hs->blendFrom ) {
		t = (float)( time - hs->blendFrom ) / HEAD_BLEND_MSEC;
		if ( t < 1.0f ) {
			CG_HeadBlendPose( &hs->fromPose, &talkPose, t, pose );
			return;
		}
		hs->blendFrom = 0;
	}
	*pose = talkPose;
}

/*
======================
CG_HeadFxFrame
======================
*/
int CG_HeadFxFrame( const headFx_t *fx, int msecSinceSound ) {
	int age = msecSinceSound - fx->delay;

	if ( age < 0 || age >= fx->msec ) {
		return -1;
	}
	return age * fx->frames / fx->msec;
}

/*
======================
CG_HeadFxCard
======================
*/
void CG_HeadFxCard( const headFx_t *fx, const float beakOnScreen[2], float beakDepth, float right[2], float up[2],
		float center[2] ) {
	float	length = (float)sqrt( beakOnScreen[0] * beakOnScreen[0] + beakOnScreen[1] * beakOnScreen[1] );
	float	d[2], u[2], a = fx->angle * 0.017453292f, c = (float)cos( a ), s = (float)sin( a );
	float	along = beakDepth > 0.0f ? 1.0f : -1.0f, w = length / HEAD_FX_BLEND;

	// a beak near the view line has no reliable screen direction, so the card eases toward vertical
	if ( w > 1.0f ) {
		w = 1.0f;
	}
	d[0] = length > 1e-6f ? w * beakOnScreen[0] / length : 0.0f;
	d[1] = ( length > 1e-6f ? w * beakOnScreen[1] / length : 0.0f ) + ( 1.0f - w ) * along;
	length = (float)sqrt( d[0] * d[0] + d[1] * d[1] );
	if ( length < 1e-3f ) {
		d[0] = 0.0f;
		d[1] = along;
	} else {
		d[0] /= length;
		d[1] /= length;
	}
	// a beak pointing left mirrors the card, so the drawing's up stays up
	if ( d[0] >= 0.0f ) {
		u[0] = -d[1];
		u[1] = d[0];
	} else {
		u[0] = d[1];
		u[1] = -d[0];
	}
	right[0] = c * d[0] + s * u[0];
	right[1] = c * d[1] + s * u[1];
	up[0] = c * u[0] - s * d[0];
	up[1] = c * u[1] - s * d[1];
	center[0] = -( fx->pivotX * fx->width * right[0] + fx->pivotY * fx->height * up[0] );
	center[1] = -( fx->pivotX * fx->width * right[1] + fx->pivotY * fx->height * up[1] );
}

/*
======================
CG_HeadFxView
======================
*/
float CG_HeadFxView( const headFx_t *fx, const float eye[3], const float axis[3][3], float swingYaw, float swingPitch,
		float point[3], float fan[2] ) {
	float	e = ( fx->viewSwing * swingPitch - fx->viewBelow ) * 0.017453292f, y = fx->viewSwing * swingYaw * 0.017453292f;
	float	ce = (float)cos( e ), se = (float)sin( e ), cy = (float)cos( y ), sy = (float)sin( y );
	float	half = fx->viewWidth * 0.5f * 0.017453292f;
	int		i;

	for ( i = 0; i < 3; i++ ) {
		point[i] = eye[i] + fx->viewDistance * ( ce * ( cy * axis[0][i] - sy * axis[1][i] ) + se * axis[2][i] );
	}
	fan[0] = sy;
	fan[1] = cy;
	return 2.0f * fx->viewDistance * (float)( sin( half ) / cos( half ) ) / fx->width;
}

static void CG_HeadViewAngles( const float v[3], const float axis[3][3], float *yaw, float *pitch ) {
	float	f = v[0] * axis[0][0] + v[1] * axis[0][1] + v[2] * axis[0][2];
	float	r = -( v[0] * axis[1][0] + v[1] * axis[1][1] + v[2] * axis[1][2] );
	float	u = v[0] * axis[2][0] + v[1] * axis[2][1] + v[2] * axis[2][2];

	*yaw = (float)atan2( r, f ) * 57.29578f;
	*pitch = (float)atan2( u, sqrt( f * f + r * r ) ) * 57.29578f;
}

/*
======================
CG_HeadFxSwing
======================
*/
void CG_HeadFxSwing( const float rest[3], const float now[3], const float axis[3][3], float *yaw, float *pitch ) {
	float	restYaw, restPitch, nowYaw, nowPitch;

	CG_HeadViewAngles( rest, axis, &restYaw, &restPitch );
	CG_HeadViewAngles( now, axis, &nowYaw, &nowPitch );
	*yaw = nowYaw - restYaw;
	if ( *yaw > 180.0f ) {
		*yaw -= 360.0f;
	} else if ( *yaw < -180.0f ) {
		*yaw += 360.0f;
	}
	*pitch = nowPitch - restPitch;
}
