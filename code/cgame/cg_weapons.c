// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "../game/bg_mode.h"

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				i;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	for ( i = 0; i < 2; i++ ) {
		float	waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if ( i == 0 ) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis( cent->lerpAngles, v );

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
		VectorCopy( re->origin, le->pos.trBase );
		if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale( xvelocity, waterScale, le->pos.trDelta );

		AxisCopy( axisDefault, re->axis );
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}


#ifdef MISSIONPACK
/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void CG_NailgunEjectBrass( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	smoke = CG_SmokePuff( origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}
#endif


/*
==========================
CG_LaserSight
==========================
*/
void CG_LaserSight( vec3_t start, vec3_t end, byte color[4], float width ) {
  refEntity_t     re;
	memset( &re, 0, sizeof( re ) );

	//Ensure shader is loaded
	cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );

  re.reType = RT_LASERSIGHT;
  re.renderfx = RF_FIRST_PERSON;
  re.customShader = cgs.media.railCoreShader;

  VectorCopy( start, re.origin );
  VectorCopy( end, re.oldorigin );

  //radius is used to store width info
  re.radius = width;

  AxisClear( re.axis );

	re.shaderRGBA.rgba[0] = color[0];
	re.shaderRGBA.rgba[1] = color[1];
	re.shaderRGBA.rgba[2] = color[2];
	re.shaderRGBA.rgba[3] = color[3];

	trap_R_AddRefEntityToScene(&re);
}

/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail( const clientInfo_t *ci, const vec3_t start, const vec3_t end ) {
	vec3_t axis[36], move, move2, vec, temp;
	float  len;
	int    i, j, skip;
 
	localEntity_t *le;
	refEntity_t   *re;
 
	#define RADIUS   4
	#define ROTATION 1
	#define SPACING  5
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA.rgba[0] = ci->color1[0] * 255;
    re->shaderRGBA.rgba[1] = ci->color1[1] * 255;
    re->shaderRGBA.rgba[2] = ci->color1[2] * 255;
    re->shaderRGBA.rgba[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
 
	if ( cg_oldRail.integer != 0 ) {
		// nudge down a bit so it isn't exactly in center
		//re->origin[2] -= 8;
		//re->oldorigin[2] -= 8;
		return;
	}

	//start[2] -= 4;
	VectorCopy( start, move );
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	PerpendicularVector( temp, vec );

	for ( i = 0 ; i < 36; i++ ) {
		RotatePointAroundVector( axis[i], vec, temp, i * 10 ); //banshee 2.4 was 10
	}

	VectorMA( move, 20, vec, move );
	VectorScale( vec, SPACING, vec );

	skip = -1;
 
	j = 18;
	for ( i = 0; i < len; i += SPACING ) {
		if ( i != skip ) {
			skip = i + SPACING;
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
			le->startTime = cg.time;
			le->endTime = cg.time + (i>>1) + 600;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			if ( intShaderTime )
				re->u.intShaderTime = cg.time;
			else
				re->u.shaderTime = cg.time / 1000.0f;

			re->reType = RT_SPRITE;
			re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;

			re->shaderRGBA.rgba[0] = ci->color2[0] * 255;
			re->shaderRGBA.rgba[1] = ci->color2[1] * 255;
			re->shaderRGBA.rgba[2] = ci->color2[2] * 255;
			re->shaderRGBA.rgba[3] = 255;

			le->color[0] = ci->color2[0] * 0.75;
			le->color[1] = ci->color2[1] * 0.75;
			le->color[2] = ci->color2[2] * 0.75;
			le->color[3] = 1.0f;

			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;

			VectorCopy( move, move2 );
			VectorMA( move2, RADIUS , axis[j], move2 );
			VectorCopy( move2, le->pos.trBase );

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		VectorAdd( move, vec, move );

		j = j + ROTATION < 36 ? j + ROTATION : (j + ROTATION) % 36;
	}
}


/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1.0f, 1.0f, 1.0f, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

#ifdef MISSIONPACK
/*
==========================
CG_NailTrail
==========================
*/
static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}
#endif

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail( centity_t *cent, const weaponInfo_t *wi ) {
	localEntity_t	*le;
	refEntity_t		*re;
	entityState_t	*es;
	vec3_t			velocity, xvelocity, origin;
	vec3_t			offset, xoffset;
	vec3_t			v[3];

	float	waterScale = 1.0f;

	if ( cg_noProjectileTrail.integer || cg_oldPlasma.integer ) {
		return;
	}

	es = &cent->currentState;

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime = cg.time;
	le->endTime = le->startTime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	VectorAdd( origin, xoffset, re->origin );
	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	
	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

    re->reType = RT_SPRITE;
    re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bounceFactor = 0.3f;

    re->shaderRGBA.rgba[0] = wi->flashDlightColor[0] * 63;
    re->shaderRGBA.rgba[1] = wi->flashDlightColor[1] * 63;
    re->shaderRGBA.rgba[2] = wi->flashDlightColor[2] * 63;
    re->shaderRGBA.rgba[3] = 63;

    le->color[0] = wi->flashDlightColor[0] * 0.2;
    le->color[1] = wi->flashDlightColor[1] * 0.2;
    le->color[2] = wi->flashDlightColor[2] * 0.2;
    le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;
}


/*
==========================
CG_GrappleTrail
==========================
*/
// spaced over 180, not 360: cull disable already makes each plane two-sided
#define TETHER_PLANES			3
#define TETHER_PLANE_STEP		( 180.0f / TETHER_PLANES )
// all three overlap down the center axis
#define TETHER_PLANE_ALPHA		115

// wider than this spills past the launcher's silhouette
#define TETHER_HALF_WIDTH		1.30f

// world length of one texture repeat
#define TETHER_PULSE_WAVELENGTH	110.0f
// EDIT IN LOCKSTEP with grapplingTether's tcMod scroll, or the arcs drift off
// the crests they ride
#define TETHER_PULSE_HZ			2.5f
#define TETHER_CREST_PERIOD		400		// ms, 1 / TETHER_PULSE_HZ

#define TETHER_TINT_R		46
#define TETHER_TINT_G		122
#define TETHER_TINT_B		255

/*
==========================
CG_GrappleOwnerRGBA

The launcher and the pad carry an rgbGen entity emission stage, so every path
that draws either model has to hand it the shooter's effects color or the lit
hardware renders black.  Stock blue until their info arrives, like the tether.
==========================
*/
void CG_GrappleOwnerRGBA( int clientNum, byte *rgba ) {
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS
			&& cgs.clientinfo[ clientNum ].infoValid ) {
		Byte4Copy( cgs.clientinfo[ clientNum ].c1RGBA, rgba );
		return;
	}
	rgba[0] = TETHER_TINT_R;
	rgba[1] = TETHER_TINT_G;
	rgba[2] = TETHER_TINT_B;
	rgba[3] = 255;
}

/*
==========================
CG_GrappleActivity

What the launcher is doing, which is also which loop is audible on it. The hook
is its own entity, so its state is found in the snapshot, off the hook itself.
==========================
*/
typedef enum {
	GRAPPLE_IDLE,		// weapon out, hook stowed: gridle.wav
	GRAPPLE_FLY,		// hook in flight: grfire.wav
	GRAPPLE_PULL,		// hook anchored, owner reeling in: grpull.wav
	GRAPPLE_RELOAD		// pad re-forming on the dock after a release
} grappleAct_t;

static int CG_GrappleActivity( int clientNum ) {
	const entityState_t	*es;
	int					i;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !cg.snap ) {
		return GRAPPLE_IDLE;
	}

	// pmove only ever CLEARS this flag (on the predicted switch-away), so the
	// playerstate leads the snapshot on release, so reading it here drops the
	// pull's dip on release instead of holding it until the snapshot catches up
	if ( clientNum == cg.predictedPlayerState.clientNum
			&& ( cg.predictedPlayerState.pm_flags & PMF_GRAPPLE_PULL ) ) {
		return GRAPPLE_PULL;
	}

	for ( i = 0; i < cg.snap->numEntities; i++ ) {
		es = &cg.snap->entities[ i ];
		if ( es->weapon != WP_GRAPPLING_HOOK || es->otherEntityNum != clientNum ) {
			continue;
		}
		if ( es->eType == ET_GRAPPLE ) {
			return GRAPPLE_PULL;
		}
		if ( es->eType == ET_MISSILE ) {
			return GRAPPLE_FLY;
		}
	}

	// holding fire with no hook in the snapshot yet: the launch frame, and the
	// stretch after a hook has been freed with the button still down
	if ( clientNum == cg.predictedPlayerState.clientNum ) {
		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			return GRAPPLE_FLY;
		}
	} else if ( cg_entities[ clientNum ].currentState.eFlags & EF_FIRING ) {
		return GRAPPLE_FLY;
	}

	// the charge surges while the pad re-forms, and everything keyed off
	// activity (glow, arc density, brightness, dlight) picks that up for free
	if ( CG_GrappleSeat( clientNum ) < 1.0f ) {
		return GRAPPLE_RELOAD;
	}

	return GRAPPLE_IDLE;
}

/*
==========================
CG_GrappleLaunchCheck

One launch sound per shot.  EV_FIRE_WEAPON repeats every 400ms while the button
is held (which is why the weapon carries no flashSound), so the trigger is the
edge into "a hook is out", not the event.  Called per view, so the first call at
a given cg.time decides and the rest are no-ops.  A gap longer than
PAD_SEAT_RESET means the client was out of PVS rather than idle; replaying the
launch on the frame he reappears would be a phantom.
==========================
*/
typedef struct {
	int		time;		// cg.time of the last update, 0 = never seen
	int		act;
} grappleLaunch_t;

static grappleLaunch_t	cg_grappleLaunch[MAX_CLIENTS];

static void CG_GrappleLaunchCheck( int clientNum, int act ) {
	grappleLaunch_t	*g;
	qboolean		out, wasOut;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	g = &cg_grappleLaunch[ clientNum ];
	if ( g->time == cg.time ) {
		return;			// already decided this frame
	}

	out = ( act == GRAPPLE_FLY || act == GRAPPLE_PULL );
	wasOut = ( g->act == GRAPPLE_FLY || g->act == GRAPPLE_PULL );

	if ( g->time && cg.time - g->time <= PAD_SEAT_RESET && out && !wasOut
			&& cgs.media.sfx_grapplelaunch ) {
		// CHAN_WEAPON cuts a running seat sound short on a re-fire mid-materialize
		trap_S_StartSound( NULL, clientNum, CHAN_WEAPON, cgs.media.sfx_grapplelaunch );
	}
	g->act = act;
	g->time = cg.time;
}

/*
==========================
CG_GrappleEnvelope

Flat brightness per state, nothing time-varying: brightness modulation near
the glass energy's 3x wave-rate cadence wagon-wheels the motion to a standstill.
==========================
*/
// levels stepped well past the ~10% luminance JND so the glass beats the
// dlit metal around it (grapple.shaderx); fly is near the ceiling already
#define GLOW_IDLE_LEVEL		0.40f
#define GLOW_ACTIVE_LEVEL	0.95f
#define GLOW_PULL_LEVEL		0.75f

static float CG_GrappleEnvelope( int act ) {
	if ( act == GRAPPLE_PULL ) {
		return GLOW_PULL_LEVEL;
	}
	if ( act == GRAPPLE_FLY ) {
		return GLOW_ACTIVE_LEVEL;
	}
	if ( act == GRAPPLE_RELOAD ) {
		return GLOW_ACTIVE_LEVEL;
	}
	return GLOW_IDLE_LEVEL;
}

/*
==========================
CG_GrapplePulse

The envelope for a given wielder, for paths outside this file that draw his
energy (the pad's emission stage). NOT its dlight, which rides its own swell.
==========================
*/
float CG_GrapplePulse( int clientNum ) {
	return CG_GrappleEnvelope( CG_GrappleActivity( clientNum ) );
}

/*
==========================
CG_GrapplePullLevel

The brightness the anchored pad was drawn at (GLOW_PULL_LEVEL). The falling
pad reads this on spawn so its fade starts continuous instead of popping to
full brightness on the release handoff frame.
==========================
*/
float CG_GrapplePullLevel( void ) {
	return GLOW_PULL_LEVEL;
}

/*
==========================
CG_GrappleDlightScale / CG_GrappleDlightRadius

The dim base for both grapple lights plus a shared swell on one clock, so the
muzzle and anchor pulse together. The radius knob sits at zero; it only earns
its keep if the pace drops below ~1 Hz, where color alone stops registering.
==========================
*/
static float CG_GrappleDlightWave( void ) {
	return (float)sin( cg.time * 0.001 * GRAPPLE_DLIGHT_PULSE_HZ * 2.0 * M_PI );
}

float CG_GrappleDlightScale( void ) {
	return GRAPPLE_DLIGHT_SCALE + GRAPPLE_DLIGHT_PULSE_AMP * CG_GrappleDlightWave();
}

float CG_GrappleDlightRadius( float base ) {
	return base * ( 1.0f + GRAPPLE_DLIGHT_RADIUS_AMP * CG_GrappleDlightWave() );
}

/*
==========================
CG_GrappleFade

Scale an emission tint by the envelope.  Alpha is left alone: it selects the
stage, it does not carry brightness.
==========================
*/
void CG_GrappleFade( byte *rgba, float pulse ) {
	rgba[0] = (byte)( rgba[0] * pulse );
	rgba[1] = (byte)( rgba[1] * pulse );
	rgba[2] = (byte)( rgba[2] * pulse );
}

// how far each end buries into what it meets, so neither reads as a gap close
// up. Pad: just past its rear boss, or the cable spears into the puck's middle.
// Launcher: shallow, since the solid emitter face has no bore to sink into
#define TETHER_PAD_OVERLAP	1.20f
#define TETHER_MUZZLE_OVERLAP	0.25f

// below this the muzzle-to-hook vector is too short to normalize into a usable
// direction, and the overlaps above would throw the ends off at random
#define TETHER_MIN_LENGTH		2.0f

// defined with the rest of the arc machinery, below
static void CG_TetherArcs( const vec3_t start, const vec3_t dir, float len,
		int clientNum, int act );

void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi ) {
	static const byte stockTint[3] = { TETHER_TINT_R, TETHER_TINT_G, TETHER_TINT_B };
	vec3_t			origin, start, dir, n, tmp;
	polyVert_t		v[4];
	float			len, s0, s1;
	entityState_t	*es;
	const byte		*tint;
	int				i, k, act;

	es = &ent->currentState;

	// the halo wears the shooter's effects color; the untinted core stage keeps
	// the center white
	tint = stockTint;
	if ( es->otherEntityNum < MAX_CLIENTS
			&& cgs.clientinfo[ es->otherEntityNum ].infoValid ) {
		tint = cgs.clientinfo[ es->otherEntityNum ].c1RGBA;
	}

	VectorCopy( ent->lerpOrigin, origin );	// the lerped pad, not the raw snapshot trajectory
	ent->trailTime = cg.time;

	// the chain meets the back of the hook, not the middle of the model.  the
	// trail runs before CG_Missile builds the render axis, so derive it here
	if ( wi->missileModel ) {
		orientation_t	tether;
		vec3_t			axis[3];

		CG_GrappleHookAxis( ent, axis );
		trap_R_LerpTag( &tether, wi->missileModel, 0, 0, 1.0f, "tag_tether" );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( origin, tether.origin[i], axis[i], origin );
		}
		// bury it along the pad's own axis, which is not the tether's direction
		// once the pad has clamped on at an angle
		VectorMA( origin, TETHER_PAD_OVERLAP, axis[0], origin );
	}

	// chain feed loop plays from the gun while the hook flies
	if ( es->eType == ET_MISSILE ) {
		trap_S_AddLoopingSound( es->otherEntityNum, cg_entities[ es->otherEntityNum ].lerpOrigin,
			vec3_origin, cgs.media.sfx_grapplefire );
	}

	VectorCopy( cg_entities[ es->otherEntityNum ].pe.muzzleOrigin, start );

	// only bail when too short to derive a direction; a bigger guard blanks
	// the tether at the end of every pull, right when it's closest to view
	if ( Distance( start, origin ) < TETHER_MIN_LENGTH )
		return;

	VectorSubtract( origin, start, dir );
	len = VectorNormalize( dir );

	// run the near end back down the bore rather than starting flush with its
	// mouth, which shows an empty barrel behind the tether in first person
	VectorMA( start, -TETHER_MUZZLE_OVERLAP, dir, start );
	len += TETHER_MUZZLE_OVERLAP;

	// no light grid sample: a tether that dimmed in shadow would read as chain

	// S counts texture repeats, so a charge cycle spans the same world distance
	// at any length.  Reversed while reeling, like the pad's rings
	act = CG_GrappleActivity( es->otherEntityNum );
	if ( act == GRAPPLE_PULL ) {
		s0 = len / TETHER_PULSE_WAVELENGTH;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = len / TETHER_PULSE_WAVELENGTH;
	}

	for ( k = 0 ; k < 4 ; k++ ) {
		v[k].modulate[0] = tint[0];
		v[k].modulate[1] = tint[1];
		v[k].modulate[2] = tint[2];
		v[k].modulate[3] = TETHER_PLANE_ALPHA;
	}
	v[0].st[0] = s0;	v[0].st[1] = 0;
	v[1].st[0] = s1;	v[1].st[1] = 0;
	v[2].st[0] = s1;	v[2].st[1] = 1;
	v[3].st[0] = s0;	v[3].st[1] = 1;

	// basis off the cable, not the view: a billboard is wrong in stereo and
	// collapses end-on, where the near end sits
	PerpendicularVector( n, dir );

	for ( i = 0 ; i < TETHER_PLANES ; i++ ) {
		VectorMA( start,  TETHER_HALF_WIDTH, n, v[0].xyz );
		VectorMA( origin, TETHER_HALF_WIDTH, n, v[1].xyz );
		VectorMA( origin, -TETHER_HALF_WIDTH, n, v[2].xyz );
		VectorMA( start,  -TETHER_HALF_WIDTH, n, v[3].xyz );

		trap_R_AddPolyToScene( cgs.media.grappleTetherShader, 4, v );

		RotatePointAroundVector( tmp, dir, n, TETHER_PLANE_STEP );
		VectorCopy( tmp, n );
	}

	CG_TetherArcs( start, dir, len, es->otherEntityNum, act );
}

/*
==========================
CG_GrappleCoreArcs

Charge crawling the launcher's open beam span, so the hardware reads as live
even with the hook stowed. Each filament ignites, travels and fades rather than
flashing; its life derives from a hash of its generation, so every view and
mirror draws it in the same place without syncing state.

Each draws twice along the same nodes: a wide halo in the owner's color and a
thin near-white core inside it, so it reads white-hot rather than as one thin
wash.

No cull, like CG_GrappleTrail, so a segment sighted end-on thins to nothing.
==========================
*/
// the three lit channels and the rail edge over each, measured off the gun model
//   { floor center y, z, the floor's own across direction y, z,
//     center y of the rail edge above it, half its run }
static const float arcChannel[3][6] = {
	{  0.000f, 2.772f, -1.0000f,  0.0000f,  0.00f, 0.50f },	// top flat
	{  0.829f, 2.429f, -0.7071f,  0.7071f,  0.42f, 0.13f },	// +y upper facet
	{ -0.829f, 2.429f, -0.7071f, -0.7071f, -0.42f, 0.13f }	// -y upper facet
};

#define ARC_RAIL_Z			3.30f		// rail beam underside, the shroud above
#define ARC_FOOT_HALF		0.28f		// across the lit floor, inside its 0.30

// x windows the two feet wander in.  The lit floor runs 6.64-11.16 and the
// beam's underside 7.00-10.70; both are pulled a hair inside, so a foot that
// slides the whole way still ends on glass / on metal
#define ARC_FOOT_X0			6.70f
#define ARC_FOOT_X1			11.10f
#define ARC_RAIL_X0			7.10f
#define ARC_RAIL_X1			10.60f

// the head's x pairs to the foot's, or back-to-front diagonals become the
// common case instead of the rare one. Reach is the PRODUCT of four uniform
// hashes, whose tail dies faster than any single hash, keeping full-length
// arcs rare
#define ARC_SPAN_NEAR		0.13f
#define ARC_SPAN_REACH		1.15f

#define ARC_SLOTS			5			// filaments that can be alight at once
#define ARC_SEGMENTS		4			// jag points along one filament
#define ARC_CYCLE			540			// ms before a slot may light again
#define ARC_STAGGER			107			// ...offset per slot, so they never
										//    all come round together
#define ARC_LIFE_MIN		140			// ms a filament lives, hashed per
#define ARC_LIFE_MAX		340			//    instance inside this window
#define ARC_DELAY			190			// latest it may ignite in its cycle;
										//    + LIFE_MAX must stay under CYCLE
#define ARC_TRAVEL			9.0f		// units/sec a foot slides along its edge
#define ARC_TRAVEL_MIN		0.35f		// ...and the slowest share of that
#define ARC_ARCH			0.30f		// how far the first node lifts off the
										//    floor along the facet's own normal
#define ARC_KINK			0.13f		// wander at the interior points
#define ARC_JAG_STEPS		3			// times the jag re-hashes over a life
#define ARC_HALF_WIDTH		0.085f
#define ARC_ALPHA			0.85f

// two passes on the same polyline: a wide halo in the owner's color and a thin
// near-white core inside it.  ARC_CORE_WIDTH_FRAC must stay <= 1.0 or the core
// pokes out from under the halo
#define ARC_HALO_WIDTH_MULT	1.15f		// halo half-width vs ARC_HALF_WIDTH
#define ARC_CORE_WIDTH_FRAC	0.40f		// core half-width, fraction of the halo's
#define ARC_CORE_WHITE_MIX		0.75f		// how far the core tint pulls toward white
#define ARC_CORE_ALPHA			0.95f		// core alpha cap
#define ARC_CORE_ATTACK			0.15f		// core life fraction ramping up
#define ARC_CORE_RELEASE		0.25f		// ...and fading out at the end
#define ARC_PULSE_COUPLE		0.50f		// share of the glow envelope's dip that
											//    reaches per-filament alpha

#define ARC_SALT_STRIDE		128			// hash room per slot; the jag alone
										//    reaches 55 of it, and two slots
										//    sharing a hash would draw as one
#define ARC_SALT_JAG		16			// ...so 0-15 is the per-instance room,
										//    and it is exactly full

// how busy the gap is per state; idle stays gappy on purpose
#define ARC_DENSITY_IDLE	0.50f
#define ARC_DENSITY_FLY		0.75f
#define ARC_DENSITY_PULL	0.95f
#define ARC_BRIGHT_IDLE		0.55f
#define ARC_BRIGHT_FLY		0.85f
#define ARC_BRIGHT_PULL		1.00f
#define ARC_SPEED_IDLE		0.70f
#define ARC_SPEED_FLY		1.00f
#define ARC_SPEED_PULL		1.25f

// re-judging a lit filament's gate against a NEW density on a state change
// would blink it out mid-alpha; the factors ramp over ARC_FACTOR_TAU and the
// gate has a soft ARC_GATE_BAND edge instead, so density crossings fade, not cut
#define ARC_FACTOR_TAU		150.0f		// ms the factors take to follow a state
#define ARC_GATE_BAND		0.22f		// gate hash room that fades in, not on
#define ARC_FACTOR_RESET	500			// ms gap that means a restart or a seek

/*
==========================
CG_ArcRandom

Hashed, not sequential: every draw of a given frame has to reproduce the same
filaments from the same seed, in any order and any number of times.
==========================
*/
static float CG_ArcRandom( int tick, int salt ) {
	unsigned int	h;

	h = (unsigned int)tick * 2654435761u + (unsigned int)salt * 2246822519u;
	h ^= h >> 15;
	h *= 2246822519u;
	h ^= h >> 13;
	return ( h & 0xffff ) * ( 1.0f / 65535.0f );
}

static float CG_ArcSigned( int tick, int salt ) {
	return 2.0f * CG_ArcRandom( tick, salt ) - 1.0f;
}

/*
==========================
CG_ArcDrift

A foot's travel over one life: direction and speed hashed and offset away from
zero, so a filament's two ends never nearly stall like a plain signed hash would.
==========================
*/
static float CG_ArcDrift( int gen, int salt, float scale ) {
	float	d;

	d = CG_ArcSigned( gen, salt );
	if ( d < 0.0f ) {
		return ( d - ARC_TRAVEL_MIN ) * scale;
	}
	return ( d + ARC_TRAVEL_MIN ) * scale;
}

static float CG_ArcClamp( float x, float lo, float hi ) {
	if ( x < lo ) {
		return lo;
	}
	if ( x > hi ) {
		return hi;
	}
	return x;
}

/*
==========================
CG_ArcCoreEnvelope

The CORE's life curve: fast attack, hold near 1, slow release.  Unlike the
halo's symmetric curve, the core spends most of its life at full presence.
Two smoothstepped ramps multiply, branchless past the clamps.
==========================
*/
static float CG_ArcCoreEnvelope( float u ) {
	float	up, down;

	up = CG_ArcClamp( u / ARC_CORE_ATTACK, 0.0f, 1.0f );
	up = up * up * ( 3.0f - 2.0f * up );
	down = CG_ArcClamp( ( 1.0f - u ) / ARC_CORE_RELEASE, 0.0f, 1.0f );
	down = down * down * ( 3.0f - 2.0f * down );
	return up * down;
}

/*
==========================
CG_ArcFactors

Density, brightness and drift, ramped rather than stepped: the one thing that
can't derive from cg.time alone, so it advances ONCE per frame and every draw shares it.
==========================
*/
typedef struct {
	int		time;			// cg.time this was last advanced
	float	density;
	float	bright;
	float	speed;
} arcFactors_t;

static arcFactors_t	cg_arcFactors[MAX_CLIENTS];

static const arcFactors_t *CG_ArcFactors( int clientNum, int act ) {
	arcFactors_t	*a;
	float			density, bright, speed, k;
	int				dt;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	a = &cg_arcFactors[ clientNum ];
	dt = cg.time - a->time;
	if ( !dt ) {
		return a;			// another draw of this frame already advanced it
	}

	if ( act == GRAPPLE_PULL ) {
		density = ARC_DENSITY_PULL; bright = ARC_BRIGHT_PULL; speed = ARC_SPEED_PULL;
	} else if ( act == GRAPPLE_RELOAD ) {
		density = ARC_DENSITY_PULL * PAD_SEAT_ARC_SPIKE; bright = ARC_BRIGHT_PULL; speed = ARC_SPEED_PULL;
	} else if ( act == GRAPPLE_FLY ) {
		density = ARC_DENSITY_FLY; bright = ARC_BRIGHT_FLY; speed = ARC_SPEED_FLY;
	} else {
		density = ARC_DENSITY_IDLE; bright = ARC_BRIGHT_IDLE; speed = ARC_SPEED_IDLE;
	}

	if ( !a->time || dt < 0 || dt > ARC_FACTOR_RESET ) {
		k = 1.0f;			// first sight of this wielder, a restart, or a seek
	} else {
		k = dt / ( dt + ARC_FACTOR_TAU );
	}
	a->density += ( density - a->density ) * k;
	a->bright += ( bright - a->bright ) * k;
	a->speed += ( speed - a->speed ) * k;
	a->time = cg.time;
	return a;
}

/*
==========================
CG_GrappleSeat

How far through the materialize a wielder's pad is, 0..1. Cannot derive from
cg.time alone, so it advances once per frame and every draw path shares it,
the same shape as cg_arcFactors above.
==========================
*/
typedef struct {
	int		time;			// cg.time this was last advanced
	float	progress;
	int		fired;			// set once the hook has launched: ramp finishes in flight
} padSeat_t;

static padSeat_t	cg_padSeat[MAX_CLIENTS];

float CG_GrappleSeat( int clientNum ) {
	padSeat_t	*p;
	int			delta;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return 1.0f;
	}
	p = &cg_padSeat[ clientNum ];

	delta = cg.time - p->time;
	if ( delta <= 0 ) {
		return p->progress;
	}
	// a demo seek or a restart resets to FULLY FORMED, not to zero: a pad that
	// materializes because someone scrubbed is worse than one that was there.
	// First sight of this client gets the same treatment, since early in a
	// session cg.time can be under PAD_SEAT_RESET
	if ( !p->time || delta > PAD_SEAT_RESET ) {
		p->progress = 1.0f;
	} else if ( p->progress < 1.0f ) {
		p->progress += delta * ( p->fired ? PAD_SEAT_FIRE_BOOST : 1.0f )
			/ (float)PAD_SEAT_TIME;
		if ( p->progress > 1.0f ) {
			p->progress = 1.0f;
		}
	}
	p->time = cg.time;
	return p->progress;
}

void CG_GrappleSeatRestart( int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	cg_padSeat[ clientNum ].progress = 0.0f;
	cg_padSeat[ clientNum ].time = cg.time;
	cg_padSeat[ clientNum ].fired = 0;
}

/*
==========================
CG_GrappleSeatFired

The pad left the dock part-formed. It finishes in flight rather than blocking
the shot, which is in-fiction: this weapon materializes its round at the
emitter rather than storing one.
==========================
*/
void CG_GrappleSeatFired( int clientNum ) {
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		cg_padSeat[ clientNum ].fired = 1;
	}
}

/*
==========================
CG_GrappleSeatSnap

Anchored means seated: the ET_GRAPPLE draw never scales the pad, so a
point-blank hit that beats the ramp must read full size on the very frame
the claws splay.
==========================
*/
void CG_GrappleSeatSnap( int clientNum ) {
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		cg_padSeat[ clientNum ].progress = 1.0f;
		cg_padSeat[ clientNum ].time = cg.time;
	}
}

/*
==========================
CG_ArcStrip

One pass of a filament along an already-built polyline: billboards each segment
toward the view, then draws it as two quads whose
center line carries alpha and whose edges fade to zero, so flat triangles read
as a round filament. Called TWICE per arc from CG_GrappleCoreArcs on the same
pt[] node array - once for the HALO, once for the CORE - so they can never drift apart.
==========================
*/
static void CG_ArcStrip( vec3_t *pt, float hw, const byte *tint, float alpha ) {
	vec3_t		mid, dir, right, viewDir, e1, e2;
	polyVert_t	v[4];
	int			s, k, c;

	for ( s = 0; s < ARC_SEGMENTS; s++ ) {
		VectorAdd( pt[s], pt[s + 1], mid );
		VectorScale( mid, 0.5f, mid );
		VectorSubtract( pt[s + 1], pt[s], dir );
		if ( VectorNormalize( dir ) < 0.001f ) {
			continue;
		}
		// billboard each piece the way the tether billboards a segment
		VectorSubtract( mid, cg.refdef.vieworg, viewDir );
		CrossProduct( dir, viewDir, right );
		if ( VectorNormalize( right ) < 0.001f ) {
			PerpendicularVector( right, dir );
		}

		// center line opaque, edges at zero alpha: a round-looking filament
		// out of flat triangles, one strip either side
		VectorCopy( pt[s], v[0].xyz );
		VectorCopy( pt[s + 1], v[1].xyz );
		v[0].st[0] = 0; v[0].st[1] = 0.5f;
		v[1].st[0] = 1; v[1].st[1] = 0.5f;
		v[2].st[0] = 1; v[2].st[1] = 0;
		v[3].st[0] = 0; v[3].st[1] = 0;
		for ( k = 0; k < 4; k++ ) {
			for ( c = 0; c < 3; c++ ) {
				v[k].modulate[c] = tint[c];
			}
			v[k].modulate[3] = ( k < 2 ) ? (byte)( 255 * alpha ) : 0;
		}
		for ( k = -1; k <= 1; k += 2 ) {
			VectorMA( pt[s], k * hw, right, e1 );
			VectorMA( pt[s + 1], k * hw, right, e2 );
			VectorCopy( e2, v[2].xyz );
			VectorCopy( e1, v[3].xyz );
			trap_R_AddPolyToScene( cgs.media.grappleArcShader, 4, v );
		}
	}
}

// world units, unlike the launcher's, which carry its model scale
#define TETHER_ARC_SLOTS		2
#define TETHER_ARC_SPAN			70.0f		// cable length one filament covers
#define TETHER_ARC_ARCH			5.0f		// how far its middle bows off
#define TETHER_ARC_KINK			1.7f		// wander square to that bow
#define TETHER_ARC_HALF_WIDTH	0.70f
// ARC_DENSITY_FLY/PULL is too narrow to read across two slots: it buys a
// quarter of a filament
#define TETHER_ARC_DENSITY_FLY	0.45f
#define TETHER_ARC_DENSITY_PULL	0.95f

// last generation each slot crackled, so an ignition sounds once, not once per
// frame of the filament's life
static int	cg_tetherArcGen[MAX_CLIENTS][TETHER_ARC_SLOTS];

/*
==========================
CG_TetherArcs

The launcher's slot, gate and life machinery, but each filament pinned to a
charge crest and traveling with it. Both feet sit on the cable, so it reads as
the tether discharging rather than as something drawn alongside it.
==========================
*/
static void CG_TetherArcs( const vec3_t start, const vec3_t dir, float len,
		int clientNum, int act ) {
	vec3_t		pt[ARC_SEGMENTS + 1];
	vec3_t		bowDir, jagDir, p;
	const arcFactors_t	*fac;
	float		gate, u, d, dLit, off, bow, jw, f;
	float		haloAlpha, coreAlpha, ramp, density;
	float		travel, dMin, dMax, bias, x;
	int			i, s, c, m, salt, t, gen, tin, delay, ms, tLit, js, k;
	byte		tint[4], coreTint[4];

	// under two spans there is no room for a filament to sit clear of both ends
	if ( len < TETHER_ARC_SPAN * 2.0f ) {
		return;
	}

	fac = CG_ArcFactors( clientNum, act );

	// remapped, not replaced, so the state change still eases over ARC_FACTOR_TAU
	ramp = CG_ArcClamp( ( fac->density - ARC_DENSITY_FLY )
		/ ( ARC_DENSITY_PULL - ARC_DENSITY_FLY ), 0.0f, 1.0f );
	density = TETHER_ARC_DENSITY_FLY
		+ ( TETHER_ARC_DENSITY_PULL - TETHER_ARC_DENSITY_FLY ) * ramp;

	CG_GrappleOwnerRGBA( clientNum, tint );
	for ( c = 0; c < 3; c++ ) {
		m = tint[c] + (int)( ( 255 - tint[c] ) * ARC_CORE_WHITE_MIX );
		if ( m > 255 ) {
			m = 255;
		}
		coreTint[c] = (byte)m;
	}
	coreTint[3] = 255;

	for ( i = 0; i < TETHER_ARC_SLOTS; i++ ) {
		salt = ( ARC_SLOTS + i ) * ARC_SALT_STRIDE;		// past the launcher's hash room
		t = cg.time + i * ARC_STAGGER;
		gen = t / ARC_CYCLE;
		tin = t - gen * ARC_CYCLE;

		gate = ( density + ARC_GATE_BAND * 0.5f - CG_ArcRandom( gen, salt ) )
			* ( 1.0f / ARC_GATE_BAND );
		if ( gate <= 0.0f ) {
			continue;
		}
		if ( gate > 1.0f ) {
			gate = 1.0f;
		}
		delay = (int)( ARC_DELAY * CG_ArcRandom( gen, salt + 1 ) );
		ms = ARC_LIFE_MIN
			+ (int)( ( ARC_LIFE_MAX - ARC_LIFE_MIN ) * CG_ArcRandom( gen, salt + 2 ) );
		if ( tin < delay || tin >= delay + ms ) {
			continue;
		}
		u = (float)( tin - delay ) / ms;

		// real time, not the staggered slot clock, or the crest lands off where
		// the shader draws it.  modulo one crest keeps a session-long cg.time
		// from eating the precision the position needs
		tLit = cg.time - ( tin - delay );
		off = TETHER_PULSE_WAVELENGTH * ( 0.25f + TETHER_PULSE_HZ
			* ( tLit % TETHER_CREST_PERIOD ) * 0.001f );
		if ( off >= TETHER_PULSE_WAVELENGTH ) {
			off -= TETHER_PULSE_WAVELENGTH;
		}
		// room to clear both ends for a whole life; the crest it rides carries it
		// outward in flight and inward while reeling
		travel = TETHER_PULSE_WAVELENGTH * TETHER_PULSE_HZ * ms * 0.001f;
		if ( act == GRAPPLE_PULL ) {
			dMin = TETHER_ARC_SPAN * 0.5f + travel;
			dMax = len - TETHER_ARC_SPAN * 0.5f;
		} else {
			dMin = TETHER_ARC_SPAN * 0.5f;
			dMax = len - TETHER_ARC_SPAN * 0.5f - travel;
		}
		if ( dMax <= dMin ) {
			continue;			// cable too short to hold one clear of both ends
		}

		// the eye sits all but on the cable, so its far half collapses into a
		// couple of degrees at the pad.  equal screen intervals are equal
		// distance RATIOS: bias toward the muzzle, square for less
		bias = CG_ArcRandom( gen, salt + 4 );
		bias = bias * bias * bias;
		dLit = dMin + ( dMax - dMin ) * bias;

		// x runs from the end the charge starts at, which reeling swaps to the pad
		x = ( act == GRAPPLE_PULL ) ? ( len - dLit ) : dLit;
		k = (int)( ( x - off ) / TETHER_PULSE_WAVELENGTH + 0.5f );
		if ( k < 0 ) {
			k = 0;
		}
		x = off + k * TETHER_PULSE_WAVELENGTH + TETHER_PULSE_WAVELENGTH
			* TETHER_PULSE_HZ * ( tin - delay ) * 0.001f;
		d = ( act == GRAPPLE_PULL ) ? ( len - x ) : x;

		// the snap can land it back over an end
		if ( d - TETHER_ARC_SPAN * 0.5f < 0.0f || d + TETHER_ARC_SPAN * 0.5f > len ) {
			continue;
		}

		// CG_TetherArcs runs once a frame under CG_AddPacketEntities; the core
		// arcs run per view, and firing this from there would double every
		// ignition on a map with a mirror.  The hash that sets the filament's
		// brightness picks the sample, so the loudest crackle is the brightest
		// arc and every witness agrees without syncing.
		if ( clientNum >= 0 && clientNum < MAX_CLIENTS
				&& cg_tetherArcGen[ clientNum ][ i ] != gen ) {
			int	pick = (int)( CG_ArcRandom( gen, salt + 3 ) * 3.0f );

			cg_tetherArcGen[ clientNum ][ i ] = gen;
			if ( pick > 2 ) {
				pick = 2;
			}
			if ( cgs.media.sfx_grapplearc[ pick ] ) {
				vec3_t	arcOrg;

				// at the filament, not the player: it lights at a real point
				// on the cable
				VectorMA( start, d, dir, arcOrg );
				trap_S_StartSound( arcOrg, ENTITYNUM_WORLD, CHAN_AUTO,
					cgs.media.sfx_grapplearc[ pick ] );
			}
		}

		// bow in a hashed direction, wander in the one square to it
		PerpendicularVector( bowDir, dir );
		RotatePointAroundVector( p, dir, bowDir, 360.0f * CG_ArcRandom( gen, salt + 5 ) );
		VectorCopy( p, bowDir );
		CrossProduct( dir, bowDir, jagDir );

		js = (int)( u * ARC_JAG_STEPS );
		VectorMA( start, d - TETHER_ARC_SPAN * 0.5f, dir, pt[0] );
		VectorMA( start, d + TETHER_ARC_SPAN * 0.5f, dir, pt[ARC_SEGMENTS] );
		for ( s = 1; s < ARC_SEGMENTS; s++ ) {
			f = (float)s / ARC_SEGMENTS;
			VectorMA( start, d + ( f - 0.5f ) * TETHER_ARC_SPAN, dir, p );
			// zero at both ends, so the feet stay welded to the cable
			bow = sin( f * M_PI ) * TETHER_ARC_ARCH
				* ( 0.6f + 0.4f * CG_ArcRandom( gen, salt + 6 + s ) );
			VectorMA( p, bow, bowDir, p );
			jw = CG_ArcSigned( gen * ARC_JAG_STEPS + js, salt + ARC_SALT_JAG + s )
				* TETHER_ARC_KINK * sin( f * M_PI );
			VectorMA( p, jw, jagDir, p );
			VectorCopy( p, pt[s] );
		}

		haloAlpha = ARC_ALPHA * gate * fac->bright * sin( u * M_PI );
		coreAlpha = ARC_CORE_ALPHA * gate * fac->bright * CG_ArcCoreEnvelope( u );
		CG_ArcStrip( pt, TETHER_ARC_HALF_WIDTH, tint, haloAlpha );
		CG_ArcStrip( pt, TETHER_ARC_HALF_WIDTH * ARC_CORE_WIDTH_FRAC,
			coreTint, coreAlpha );
	}
}

static void CG_GrappleCoreArcs( const refEntity_t *gun, int clientNum, int act,
		float pulse ) {
	vec3_t		pt[ARC_SEGMENTS + 1];
	vec3_t		p;
	const float	*chan;
	const arcFactors_t	*fac;
	float		speed;
	float		fx, hx, fy, fz, fs, hy, hz, life, u, jw, jf;
	float		span, sep, sepMax;
	float		ax, ay, az, ny, nz, gate, kink[2];
	float		f, hw, coreHw, haloAlpha, coreAlpha, pulseFactor, peakVar;
	int			t, gen, tin, delay, ms, jag, js, m;
	int			i, s, k, c, salt, ch;
	byte		tint[4], coreTint[4];

	fac = CG_ArcFactors( clientNum, act );
	speed = fac->speed * ARC_TRAVEL;

	// the OWNER's color, not the launcher's already-faded one, or these additive
	// polys would carry the envelope twice (rgb * alpha on top of alphaGen vertex)
	CG_GrappleOwnerRGBA( clientNum, tint );
	// the core reads near-white regardless of fringe color, pulled toward white
	// rather than copied straight; clamped so the mix can't overflow
	for ( c = 0; c < 3; c++ ) {
		m = tint[c] + (int)( ( 255 - tint[c] ) * ARC_CORE_WHITE_MIX );
		if ( m > 255 ) {
			m = 255;
		}
		coreTint[c] = (byte)m;
	}
	coreTint[3] = 255;

	// the axis carries the model's scale, so filaments stay in proportion
	hw = ARC_HALF_WIDTH * ARC_HALO_WIDTH_MULT * VectorLength( gun->axis[0] );
	coreHw = hw * ARC_CORE_WIDTH_FRAC;

	// only brightness softens its coupling to the sound envelope, so pulling
	// still breathes but a lit filament never goes ghostly
	pulseFactor = 1.0f - ( 1.0f - pulse ) * ARC_PULSE_COUPLE;

	for ( i = 0; i < ARC_SLOTS; i++ ) {
		salt = i * ARC_SALT_STRIDE;		// this slot's own stretch of the hash
		t = cg.time + i * ARC_STAGGER;
		gen = t / ARC_CYCLE;			// which filament this slot is on
		tin = t - gen * ARC_CYCLE;		// ...and how far into its cycle

		// the last ARC_GATE_BAND of hash room fades in rather than switching on,
		// so a density change dims a live filament instead of cutting it
		gate = ( fac->density + ARC_GATE_BAND * 0.5f - CG_ArcRandom( gen, salt ) )
			* ( 1.0f / ARC_GATE_BAND );
		if ( gate <= 0.0f ) {
			continue;					// a dark cycle; the gaps are what make it tick
		}
		if ( gate > 1.0f ) {
			gate = 1.0f;
		}
		delay = (int)( ARC_DELAY * CG_ArcRandom( gen, salt + 1 ) );
		ms = ARC_LIFE_MIN
			+ (int)( ( ARC_LIFE_MAX - ARC_LIFE_MIN ) * CG_ArcRandom( gen, salt + 2 ) );
		if ( tin < delay || tin >= delay + ms ) {
			continue;					// not lit yet, or already spent
		}
		u = (float)( tin - delay ) / ms;
		life = ms * 0.001f;

		ch = (int)( 3.0f * CG_ArcRandom( gen, salt + 3 ) );
		if ( ch > 2 ) {
			ch = 2;
		}
		chan = arcChannel[ch];

		// the head is PAIRED to the foot: separation comes from ARC_SPAN_* above
		span = ARC_FOOT_X1 - ARC_FOOT_X0;
		sepMax = span * ( ARC_SPAN_NEAR * CG_ArcRandom( gen, salt + 15 )
			+ ARC_SPAN_REACH * CG_ArcRandom( gen, salt + 11 )
				* CG_ArcRandom( gen, salt + 12 )
				* CG_ArcRandom( gen, salt + 13 )
				* CG_ArcRandom( gen, salt + 14 ) );
		sep = ( CG_ArcRandom( gen, salt + 5 ) < 0.5f ) ? -sepMax : sepMax;

		// both ends then slide from there, each at its own rate.  A foot that
		// runs out of edge just stops there
		fx = ARC_FOOT_X0 + span * CG_ArcRandom( gen, salt + 4 );
		hx = fx + sep;
		fx = CG_ArcClamp( fx + CG_ArcDrift( gen, salt + 6, speed ) * life * u,
			ARC_FOOT_X0, ARC_FOOT_X1 );
		hx = hx + CG_ArcDrift( gen, salt + 7, speed ) * life * u;
		// unequal drift may move the pair but not STRETCH it, or a short filament
		// walks itself out to full length over its life
		hx = CG_ArcClamp( hx, fx - sepMax, fx + sepMax );
		hx = CG_ArcClamp( hx, ARC_RAIL_X0, ARC_RAIL_X1 );

		// ONE hash across the floor, not one per axis: the side channels lie at
		// 45 degrees, so independent y and z would put the foot off the glass
		fs = ARC_FOOT_HALF * CG_ArcSigned( gen, salt + 8 );
		fy = chan[0] + chan[2] * fs;
		fz = chan[1] + chan[3] * fs;
		hy = chan[4] + chan[5] * CG_ArcSigned( gen, salt + 9 );
		hz = ARC_RAIL_Z;

		// per-instance peak variance, shared by both passes so the halo and core swell together
		peakVar = 0.65f + 0.35f * CG_ArcRandom( gen, salt + 10 );

		// HALO: symmetric 4u(1-u), touching peak at the life's midpoint, at the
		// original ARC_ALPHA cap
		haloAlpha = ARC_ALPHA * fac->bright * pulseFactor * gate
			* 4.0f * u * ( 1.0f - u ) * peakVar;

		// CORE: flatter (see CG_ArcCoreEnvelope) and near the byte cap, so the
		// filament spends most of its life white-hot instead of only touching it
		coreAlpha = ARC_CORE_ALPHA * fac->bright * pulseFactor * gate
			* CG_ArcCoreEnvelope( u ) * peakVar;

		// the jag is hashed at ARC_JAG_STEPS points across the life and eased
		// between them, so the kinks crawl instead of snapping
		jw = u * ARC_JAG_STEPS;
		jag = (int)jw;
		jf = jw - jag;
		jf = jf * jf * ( 3.0f - 2.0f * jf );

		// OUT of the trench first, then across: a straight run from a side floor
		// (45-degree facets, no rail above) to the beam buries through the LIP
		// into barrel metal, where the depth test eats it. The first node instead
		// lifts along the facet's own outward normal, clearing the lip entirely
		ny = chan[3];
		nz = -chan[2];
		ax = fx + ( hx - fx ) * ( 1.0f / ARC_SEGMENTS );
		ay = fy + ARC_ARCH * ny;
		az = fz + ARC_ARCH * nz;

		for ( s = 0; s <= ARC_SEGMENTS; s++ ) {
			if ( s == 0 ) {
				p[0] = fx;
				p[1] = fy;
				p[2] = fz;
			} else {
				f = (float)( s - 1 ) / ( ARC_SEGMENTS - 1 );
				p[0] = ax + ( hx - ax ) * f;
				p[1] = ay + ( hy - ay ) * f;
				p[2] = az + ( hz - az ) * f;
			}
			if ( s > 0 && s < ARC_SEGMENTS ) {
				// feet stay welded to their edges; only the middle wanders, ACROSS
				// the channel rather than world y, since the facets sit at 45 degrees
				for ( c = 0; c < 2; c++ ) {
					js = salt + ARC_SALT_JAG + ( jag * ARC_SEGMENTS + s ) * 2 + c;
					kink[c] = ARC_KINK * ( CG_ArcSigned( gen, js ) * ( 1.0f - jf )
						+ CG_ArcSigned( gen, js + ARC_SEGMENTS * 2 ) * jf );
				}
				p[0] += kink[0];
				p[1] += kink[1] * chan[2];
				p[2] += kink[1] * chan[3];
			}
			VectorCopy( gun->origin, pt[s] );
			for ( k = 0; k < 3; k++ ) {
				VectorMA( pt[s], p[k], gun->axis[k], pt[s] );
			}
		}

		// two passes over the SAME pt[] polyline; order doesn't matter under additive blending
		CG_ArcStrip( pt, hw, tint, haloAlpha );
		CG_ArcStrip( pt, coreHw, coreTint, coreAlpha );
	}
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_RocketTrail( ent, wi );
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_barrel.md3" );
	weaponInfo->barrelModel = trap_R_RegisterModel( path );

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	weaponInfo->loopFireSound = qfalse;

	switch ( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/fstrun.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav", qfalse );
		break;

	case WP_LIGHTNING:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/lightning/lg_hum.wav", qfalse );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/lightning/lg_fire.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel( "models/weaphits/crackle.md3" );
		cgs.media.sfx_lghit1 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit.wav", qfalse );
		cgs.media.sfx_lghit2 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit2.wav", qfalse );
		cgs.media.sfx_lghit3 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit3.wav", qfalse );

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		// one model for both states: it rides tag_ammo in the bore while stowed
		// and flies as the missile once away
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons2/grapple/grapple_pad.md3" );
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		// no missileDlight: the glow belongs on the barrel, not the hook
		// no flashSound: the grapple re-fires EV_FIRE_WEAPON while held,
		// which would stack it; the chain feed loops from the gun instead
		// (not missileSound, which the receding hook would doppler down)
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/grapple/gridle.wav", qfalse );
		cgs.media.sfx_grapplefire = trap_S_RegisterSound( "sound/weapons/grapple/grfire.wav", qfalse );
		cgs.media.sfx_grapplehit = trap_S_RegisterSound( "sound/weapons/grapple/grhit.wav", qfalse );
		cgs.media.sfx_grapplepull = trap_S_RegisterSound( "sound/weapons/grapple/grpull.wav", qfalse );
		cgs.media.sfx_grappletether = trap_S_RegisterSound( "sound/weapons/grapple/grtether.wav", qfalse );
		cgs.media.sfx_grapplelaunch = trap_S_RegisterSound( "sound/weapons/grapple/grlaunch.wav", qfalse );
		cgs.media.sfx_grapplebite = trap_S_RegisterSound( "sound/weapons/grapple/grbite.wav", qfalse );
		cgs.media.sfx_grapplefree = trap_S_RegisterSound( "sound/weapons/grapple/grfree.wav", qfalse );
		cgs.media.sfx_grappleseat = trap_S_RegisterSound( "sound/weapons/grapple/grseat.wav", qfalse );
		cgs.media.sfx_grappleclank = trap_S_RegisterSound( "sound/weapons/grapple/grclank.wav", qfalse );
		cgs.media.sfx_grapplesettle = trap_S_RegisterSound( "sound/weapons/grapple/grsettle.wav", qfalse );
		cgs.media.sfx_grapplearc[0] = trap_S_RegisterSound( "sound/weapons/grapple/arc_tick.wav", qfalse );
		cgs.media.sfx_grapplearc[1] = trap_S_RegisterSound( "sound/weapons/grapple/arc_zap.wav", qfalse );
		cgs.media.sfx_grapplearc[2] = trap_S_RegisterSound( "sound/weapons/grapple/arc_tear.wav", qfalse );
		cgs.media.grappleTetherShader = trap_R_RegisterShader( "grapplingTether" );
		cgs.media.grappleArcShader = trap_R_RegisterShader( "grapplingArc" );
		cgs.media.grappleGunFlyShader = trap_R_RegisterShader( "models/weapons2/grapple/gun_fly" );
		cgs.media.grappleGunPullShader = trap_R_RegisterShader( "models/weapons2/grapple/gun_pull" );
		cgs.media.grapplePadFlyShader = trap_R_RegisterShader( "models/weapons2/grapple/pad_fly" );
		cgs.media.grapplePadPullShader = trap_R_RegisterShader( "models/weapons2/grapple/pad_pull" );
		cgs.media.grapplePadFadeShader = trap_R_RegisterShader( "models/weapons2/grapple/pad_fade" );

		break;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/vulcan/wvulfire.wav", qfalse );
		weaponInfo->loopFireSound = qtrue;
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;
#endif

	case WP_MACHINEGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shotgun/sshotf1b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = MISSILE_GLOW_RADIUS;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/proxmine.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/proxmine/wstbfire.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;
#endif

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/grenade1.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/grenade/grenlf1a.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;

#ifdef MISSIONPACK
	case WP_NAILGUN:
		weaponInfo->ejectBrassFunc = CG_NailgunEjectBrass;
		weaponInfo->missileTrailFunc = CG_NailTrail;
//		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/nail.md3" );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/nailgun/wnalfire.wav", qfalse );
		break;
#endif

	case WP_PLASMAGUN:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileTrailFunc = CG_PlasmaTrail;
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/plasma/lasfly.wav", qfalse );

		// plasmagun dlight
		weaponInfo->missileDlight = MISSILE_GLOW_RADIUS;
		MAKERGB( weaponInfo->missileDlightColor, 0.2f, 0.2f, 1.0f );

		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/plasma/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/railgun/rg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.5f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/railgun/railgf1a.wav", qfalse );
		cgs.media.railExplosionShader = trap_R_RegisterShader( "railExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );
		break;

	case WP_BFG:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/bfg/bfg_hum.wav", qfalse );

		// bfg dlight
		weaponInfo->missileDlight = MISSILE_GLOW_RADIUS;
		MAKERGB( weaponInfo->missileDlightColor, 0.2f, 1.0f, 0.2f );

		MAKERGB( weaponInfo->flashDlightColor, 1.0f, 0.7f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/bfg/bfg_fire.wav", qfalse );
		cgs.media.bfgExplosionShader = trap_R_RegisterShader( "bfgExplosion" );
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/bfg.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	// try to register depth-fragment shaders
	if ( cg.clientFrame == 0 && cg.skipDFshaders ) {
		itemInfo->icon_df = 0;
	} else {
		itemInfo->icon_df = trap_R_RegisterShader( va( "%s_df", item->icon ) );
	}

	if ( !itemInfo->icon_df ) {
		itemInfo->icon_df = itemInfo->icon;
		if ( cg.clientFrame == 0 ) {
			cg.skipDFshaders = qtrue; // skip all further tries to avoid shader debug mesages in 1.32c during map loading
		} else {
			cg.skipDFshaders = qfalse;
		}
	} else {
		cg.skipDFshaders = qfalse;
	}

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( const clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );

	// VR follow: weapon points along weapon aim, not head direction
	if ( CG_VR_IsVRFollow() ) {
		VectorCopy( cg.predictedPlayerState.viewangles, angles );
	} else {
		VectorCopy( cg.refdefViewAngles, angles );
	}

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	if ( !vrActive || cg_weaponbob.value != 0 )
	{
		angles[ROLL] += scale * cg.bobfracsin * 0.005;
		angles[YAW] += scale * cg.bobfracsin * 0.01;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;
	}

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( ( cg.time % TMOD_1000 ) * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	trace_t  trace;
	refEntity_t  beam;
	vec3_t   forward;
	vec3_t   muzzlePoint, endPoint;
	int      anim;
	qboolean directView;
	vec3_t   vrAngle;

	if (cent->currentState.weapon != WP_LIGHTNING) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	if ( !cg.renderingThirdPerson && cent->currentState.number == cg.predictedPlayerState.clientNum ) {
		directView = qtrue;
		if ( vrActive )
			CG_CalculateVRWeaponPosition( muzzlePoint, vrAngle );
		else
		VectorCopy( cg.refdef.vieworg, muzzlePoint );
	} else {
		directView = qfalse;
		VectorCopy( cent->lerpOrigin, muzzlePoint );
		anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
		if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
			muzzlePoint[2] += CROUCH_VIEWHEIGHT;
		} else {
			muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
		}
	}

	if ( vrActive && directView ) {
		AngleVectors( vrAngle, forward, NULL, NULL );

		CG_VR_OnWeaponFiring( cent->currentState.weapon );
	} else
	// CPMA  "true" lightning
	if ( directView && cg_trueLightning.value ) {
		//vec3_t	viewangles;
		vec3_t angle;
		int i;

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdefViewAngles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );

	} else {
		// !CPMA
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
	}

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, Mode_GetConfig( cgs.mode )->weapons[WP_LIGHTNING].range, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
}
/*

static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	trace_t		trace;
	refEntity_t		beam;
	vec3_t			forward;
	vec3_t			muzzlePoint, endPoint;

	if ( cent->currentState.weapon != WP_LIGHTNING ) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// find muzzle point for this frame
	VectorCopy( cent->lerpOrigin, muzzlePoint );
	AngleVectors( cent->lerpAngles, forward, NULL, NULL );

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
}
*/

/*
===============
CG_SpawnRailTrail

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
===============
*/
static void CG_SpawnRailTrail( centity_t *cent, vec3_t origin ) {
	clientInfo_t	*ci;

	if ( cent->currentState.weapon != WP_RAILGUN ) {
		return;
	}
	if ( !cent->pe.railgunFlash ) {
		return;
	}
	cent->pe.railgunFlash = qtrue;
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RailTrail( ci, origin, cent->pe.railgunImpact );
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
#ifdef MISSIONPACK
		if ( cent->currentState.weapon == WP_CHAINGUN && !cent->pe.barrelSpinning ) {
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, trap_S_RegisterSound( "sound/weapons/vulcan/wvulwind.wav", qfalse ) );
		}
#endif
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( gun );
	} else {
		trap_R_AddRefEntityToScene( gun );

		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
		if ( powerups & ( 1 << PW_QUAD ) ) {
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	int			grappleAct;
	float		grapplePulse;
//	int	col;
	const	clientInfo_t	*ci;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps ) {
		if ( cg.predictedPlayerState.weapon == WP_RAILGUN 
			&& cg.predictedPlayerState.weaponstate == WEAPON_FIRING ) {
			float	f;
			f = (1.0f - ((float)cg.predictedPlayerState.weaponTime / (float)Mode_GetConfig( cgs.mode )->weapons[WP_RAILGUN].fireTime) );
			gun.shaderRGBA.rgba[0] = 255 * ci->color1[0] * f;
			gun.shaderRGBA.rgba[1] = 255 * ci->color1[1] * f;
			gun.shaderRGBA.rgba[2] = 255 * ci->color1[2] * f;
			//gun.shaderRGBA.rgba[3] = 255;
		} else {
			gun.shaderRGBA.rgba[0] = 255 * ci->color1[0];
			gun.shaderRGBA.rgba[1] = 255 * ci->color1[1];
			gun.shaderRGBA.rgba[2] = 255 * ci->color1[2];
			//gun.shaderRGBA.rgba[3] = 255;
			if ( gun.shaderRGBA.rgba[1] < 64 ) gun.shaderRGBA.rgba[1] = 64;
		}
		gun.shaderRGBA.rgba[3] = 255;
	}

	// the launcher's rgbGen entity emission stage needs the owner's tint in
	// every view, breathed at the cadence of whichever loop is audible on the gun
	grappleAct = GRAPPLE_IDLE;
	grapplePulse = 1.0f;
	if ( weaponNum == WP_GRAPPLING_HOOK ) {
		grappleAct = CG_GrappleActivity( cent->currentState.clientNum );
		CG_GrappleLaunchCheck( cent->currentState.clientNum, grappleAct );
		grapplePulse = CG_GrappleEnvelope( grappleAct );
		CG_GrappleOwnerRGBA( cent->currentState.clientNum, gun.shaderRGBA.rgba );
		CG_GrappleFade( gun.shaderRGBA.rgba, grapplePulse );
		// the state also picks the stream's direction and pace: base shader
		// at idle, forward and faster in flight, reversed while reeling
		if ( grappleAct == GRAPPLE_PULL ) {
			gun.customShader = cgs.media.grappleGunPullShader;
		} else if ( grappleAct == GRAPPLE_FLY ) {
			gun.customShader = cgs.media.grappleGunFlyShader;
		}
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and gauntlet make a different sound when fire is held down
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		} else if ( weaponNum == WP_GRAPPLING_HOOK && ( cent->currentState.eFlags & EF_FIRING ) ) {
			// no firing loop, but still sets the flag so the 400ms refire events
			// skip their one-shots, or the quad sound garbles restarting every cycle
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound ) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	// invis already renders the gun through invisShader, but the arcs are separate
	// polys that shader can't touch, so they need their own gate or an invisible
	// carrier flickers owner-colored arcs as a permanent idle tell. RF_THIRD_PERSON
	// needs the same gate: polys carry no renderfx, so the mirror-only BODY gun
	// was throwing its arcs into the main view with no gun under them
	if ( weaponNum == WP_GRAPPLING_HOOK
			&& !( gun.renderfx & RF_THIRD_PERSON )
			&& !( cent->currentState.powerups & ( 1 << PW_INVIS ) ) ) {
		CG_GrappleCoreArcs( &gun, cent->currentState.clientNum, grappleAct,
			grapplePulse );
	}

	// add the spinning barrel
	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( ( weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET )
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) )
	{
		// continuous flash
	} else {
		// impulse flash; the grapple continues on for its muzzle point and claw
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash
		&& weaponNum != WP_GRAPPLING_HOOK ) {
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	// colorize the railgun blast and the grapple's emitter glow
	if ( weaponNum == WP_RAILGUN || weaponNum == WP_GRAPPLING_HOOK ) {
		flash.shaderRGBA.rgba[0] = 255 * ci->color1[0];
		flash.shaderRGBA.rgba[1] = 255 * ci->color1[1];
		flash.shaderRGBA.rgba[2] = 255 * ci->color1[2];
		flash.shaderRGBA.rgba[3] = 255;
		if ( weaponNum == WP_GRAPPLING_HOOK ) {
			// the emitter is fed by the same cell the channel is
			CG_GrappleFade( flash.shaderRGBA.rgba, grapplePulse );
		}
	}

	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash" );
	if ( weaponNum == WP_GRAPPLING_HOOK && !( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) {
		// the hook sits in the bore on tag_ammo; the flash takes over that spot
		// once it is away
		orientation_t	stow;
		refEntity_t		hook;
		int				i;
		float			seat, a;

		memset( &hook, 0, sizeof( hook ) );
		VectorCopy( parent->lightingOrigin, hook.lightingOrigin );
		hook.shadowPlane = parent->shadowPlane;
		hook.renderfx = parent->renderfx;
		hook.hModel = weapon->missileModel;
		// folded in the bore, same as in flight; it only splays on a clamp
		hook.frame = hook.oldframe = PAD_FRAME_FOLDED;
		// the round in the bore lights off the same charge as the launcher
		Byte4Copy( gun.shaderRGBA.rgba, hook.shaderRGBA.rgba );

		trap_R_LerpTag( &stow, weapon->weaponModel, 0, 0, 1.0f, "tag_ammo" );
		VectorCopy( gun.origin, hook.origin );
		for ( i = 0; i < 3; i++ ) {
			VectorMA( hook.origin, stow.origin[i], gun.axis[i], hook.origin );
		}
		MatrixMultiply( stow.axis, gun.axis, hook.axis );

		// It grows OUT of the emitter: the pivot is the pad's rear boss, not
		// its origin. Pivoting on the origin starts the boss detached and
		// slides it BACK onto the collar, which is the opposite of the pad
		// never letting go
		seat = CG_GrappleSeat( cent->currentState.clientNum );
		if ( seat < 1.0f ) {
			float	e, s, roll, d, r;
			vec3_t	t1, t2;

			// seat^1.5 via sqrt; pow() is not in the QVM libc, and 1.5
			// reads the same as the intended 1.6
			e = seat * sqrt( seat );
			e = e * e * ( 3.0f - 2.0f * e );
			s = PAD_SEAT_SCALE0 + ( 1.0f - PAD_SEAT_SCALE0 ) * e;

			// unwind from PAD_SEAT_SPIN down to 0 about the pad's own
			// forward. axis[0] points away from the viewer (same convention
			// CG_GrappleHookAxis relies on), where a positive angle in
			// RotatePointAroundVector reads CLOCKWISE - so a shrinking
			// positive angle is what unwinds counterclockwise.
			// Must run on the still-unit-length axis[0], before the extrude
			// below scales it: RotatePointAroundVector needs a unit
			// direction, and this is only masked right now because
			// PAD_SEAT_SCALE0 is 1.00. RotatePointAroundVector also can't
			// rotate a vector into itself, hence the temporaries.
			// Own cubic ease-out on raw seat, not e: the roll wants to spin
			// briskly off the mark and decelerate into rest, where e's
			// smoothstep is shaped for a symmetric grow instead.
			d = 1.0f - seat;
			r = 1.0f - d * d * d;
			roll = PAD_SEAT_SPIN * ( 1.0f - r );
			VectorCopy( hook.axis[1], t1 );
			VectorCopy( hook.axis[2], t2 );
			RotatePointAroundVector( hook.axis[1], hook.axis[0], t1, roll );
			RotatePointAroundVector( hook.axis[2], hook.axis[0], t2, roll );

			// PAD_BOSS_X0 back along the pad's own forward, then out again
			// by the scaled amount
			VectorMA( hook.origin, PAD_BOSS_X0 * ( 1.0f - s ), hook.axis[0],
				hook.origin );
			// extrude at full width rather than scaling radially: a radially
			// scaled boss sits INSIDE the emitter collar for the first third
			VectorScale( hook.axis[0], s, hook.axis[0] );

			a = seat / PAD_SEAT_ALPHA;
			if ( a > 1.0f ) {
				a = 1.0f;
			}
			// pad_fade's additive energy stages (glow/nrg1-3) read entity RGB,
			// not entity alpha, for their strength; ramp both or the claws
			// (which those stages cover) show up at full brightness at once
			hook.shaderRGBA.rgba[0] = (byte)( hook.shaderRGBA.rgba[0] * a );
			hook.shaderRGBA.rgba[1] = (byte)( hook.shaderRGBA.rgba[1] * a );
			hook.shaderRGBA.rgba[2] = (byte)( hook.shaderRGBA.rgba[2] * a );
			hook.shaderRGBA.rgba[3] = a * 0xff;

			// condensing out of the collar, not yet seated: pad_fade reads
			// that alpha. Once seated this falls through unset, back to the
			// plain docked shader
			hook.customShader = cgs.media.grapplePadFadeShader;
		}

		// the stowed pad shares the gun's powerup shell, or quad reads as
		// wrapping the launcher but skipping the round in its bore
		CG_AddWeaponWithPowerups( &hook, cent->currentState.powerups );
	} else {
		trap_R_AddRefEntityToScene( &flash );
	}

	// muzzle origin for the tether prefers the view weapon: in VR it IS the
	// tracked controller, so it's correct everywhere; on flatscreen it sits at
	// the camera, the same mirror compromise the lightning bolt and rail trail make
	if ( ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		if ( weaponNum == WP_GRAPPLING_HOOK ) {
			// the tether feeds from tag_chain, not the muzzle
			orientation_t	spool;
			vec3_t			chainOrg;
			int				i;

			trap_R_LerpTag( &spool, weapon->weaponModel, 0, 0, 1.0f, "tag_chain" );
			VectorCopy( gun.origin, chainOrg );
			for ( i = 0; i < 3; i++ ) {
				VectorMA( chainOrg, spool.origin[i], gun.axis[i], chainOrg );
			}
			VectorCopy( chainOrg, nonPredictedCent->pe.muzzleOrigin );
		} else {
			VectorCopy( flash.origin, nonPredictedCent->pe.muzzleOrigin );
		}
	}

	if ( ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		int radius;

		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flash.origin );

		// add rail trail
		CG_SpawnRailTrail( cent, flash.origin );

		// use our own muzzle point as dlight origin 
		// and put it a bit closer to vieworigin to avoid bad normals near walls
		if ( ps && cent->currentState.number == cg.predictedPlayerState.clientNum ) {
			vec3_t	start, end, muzzle, forward, up;
			trace_t	tr;
			AngleVectors( cg.refdefViewAngles, forward, NULL, up );
			VectorMA( cg.refdef.vieworg, 14, forward, muzzle );
			if ( weaponNum == WP_LIGHTNING )
				VectorMA( muzzle, -8, up, muzzle );
			else
				VectorMA( muzzle, -6, up, muzzle );
			VectorMA( cg.refdef.vieworg, 14, forward, start );
			VectorMA( cg.refdef.vieworg, 28, forward, end );
			CG_Trace( &tr, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT | CONTENTS_TRANSLUCENT );
			if ( tr.fraction != 1.0 ) {
				VectorMA( muzzle, -13.0 * ( 1.0 - tr.fraction ), forward, flash.origin );
			} else {
				VectorCopy( muzzle, flash.origin );
			}
		}

		if ( weaponNum == WP_MACHINEGUN ) // make it a bit less annoying
			radius = MG_FLASH_RADIUS + (rand() & WEAPON_FLASH_RADIUS_MOD);
		else if ( weaponNum == WP_GRAPPLING_HOOK ) // breathing; see GRAPPLE_FLASH_RADIUS
			radius = (int)CG_GrappleDlightRadius( GRAPPLE_FLASH_RADIUS );
		else
			radius = WEAPON_FLASH_RADIUS + (rand() & WEAPON_FLASH_RADIUS_MOD);

		// the grapple burns its glow the whole time the hook is out; every
		// other weapon only reaches here inside MUZZLE_FLASH_TIME
		if ( ( weaponNum != WP_GRAPPLING_HOOK || ( nonPredictedCent->currentState.eFlags & EF_FIRING ) )
			&& ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) ) {
			const float *dl = weapon->flashDlightColor;
			vec3_t		grappleDl;

			// the grapple's light is projected energy: the wielder's effects
			// color, like the rail: dim with a slow breath, never the
			// envelope (see GRAPPLE_DLIGHT_SCALE)
			if ( weaponNum == WP_GRAPPLING_HOOK ) {
				VectorScale( ci->color1, CG_GrappleDlightScale(), grappleDl );
				dl = grappleDl;
			}
			trap_R_AddLightToScene( flash.origin, radius, dl[0], dl[1], dl[2] );
		}
	}
}


/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	const clientInfo_t *ci;
	const weaponInfo_t *weapon;
	vec3_t		fovOffset;
	vec3_t		angles;
	qboolean	vrPosed;
	float		vrScale;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}


	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( ( cg.predictedPlayerState.eFlags & EF_FIRING )
		|| ( !CG_VR_OwnsHiddenGunMuzzle() && ps->weapon == WP_GRAPPLING_HOOK ) ) {
			// special hack for lightning gun and grappling hook...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			if ( !CG_VR_OwnsHiddenGunMuzzle() ) {
				// head-derived muzzle would clobber the controller muzzle in VR
				VectorCopy( origin, cg_entities[ps->clientNum].pe.muzzleOrigin );
			}
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	if ( CG_VR_WeaponWheel() ) {
		return;		// selector drew instead of the gun
	}

	if ( CG_VR_HideViewWeapon() ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cgs.fov > 90.0 ) {
		fovOffset[0] = 0;
		fovOffset[2] = -0.2 * ( cgs.fov - 90.0 );
	} else {
		// move gun forward at lowerer fov
		fovOffset[0] = -0.2 * ( cgs.fov - 90.0 );
		fovOffset[2] = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	vrScale = 1.0f;
	vrPosed = CG_VR_WeaponHandPose( hand.origin, angles, &vrScale );
	if ( !vrPosed ) {
		CG_CalculateWeaponPosition( hand.origin, angles );

		// VR follow: offset weapon along weapon-aim axes, not head-view axes
		if ( CG_VR_IsVRFollow() ) {
			vec3_t	weaponAxis[3];
			AnglesToAxis( cg.predictedPlayerState.viewangles, weaponAxis );
			VectorMA( hand.origin, (cg_gun_x.value+fovOffset[0]), weaponAxis[0], hand.origin );
			VectorMA( hand.origin, cg_gun_y.value, weaponAxis[1], hand.origin );
			VectorMA( hand.origin, (cg_gun_z.value+fovOffset[2]), weaponAxis[2], hand.origin );
		} else {
			VectorMA( hand.origin, (cg_gun_x.value+fovOffset[0]), cg.refdef.viewaxis[0], hand.origin );
			VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
			VectorMA( hand.origin, (cg_gun_z.value+fovOffset[2]), cg.refdef.viewaxis[2], hand.origin );
		}
	}

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	if ( vrPosed ) {
		CG_VR_WeaponHandFinish( &hand, vrScale );
	} else {
		// the arc filaments carry no renderfx and draw at true depth, so a
		// depth-hacked gun would hide them; leave the hack off, same as VR always does
		hand.renderfx = RF_FIRST_PERSON | RF_MINLIGHT;
		if ( ps->weapon != WP_GRAPPLING_HOOK ) {
			hand.renderfx |= RF_DEPTHHACK;
		}
	}

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/


/*
===================
CG_DrawWeaponSelect
===================
*/
#define AMMO_FONT_SIZE 12
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y;
	int		dx, dy;
	int		weaponSelect;
	const char *name;
	float	*color;
	char	buf[16];

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 || cg_drawWeaponSelect.integer == 0 ) {
		return;
	}

	// don't display while the 3D weapon wheel is open (reads zero when VR inactive)
	if ( vr->weapon_select ) {
		return;
	}

	if ( cg_drawWeaponSelect.integer < 0 ) {
		color = colorWhite;
	} else {
		color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
		if ( !color ) {
			return;
		}
	}
	trap_R_SetColor( color );

	weaponSelect = abs( cg_drawWeaponSelect.integer );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = WP_GAUNTLET ; i < MAX_WEAPONS ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	if ( weaponSelect < 3 ) {
		x = 320 - count * 20;
		y = cgs.screenYmax + 1 - 100; // - STATUSBAR_HEIGHT - 40
		dx = 40;
		dy = 0;
	} else {
		x = cgs.screenXmin + 6;
		y = 240 - count * 20;
		dx = 0;
		dy = 40;
	}

	for ( i = WP_GAUNTLET ; i < MAX_WEAPONS ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeapon( i );

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 32+8, 32+8, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		} else if ( weaponSelect > 1 && cg.snap->ps.ammo[ i ] > 0 ) {
			// ammo counter
			BG_sprintf( buf, "%i", cg.snap->ps.ammo[ i ] );
			if ( weaponSelect == 2 ) {
				// horizontal ammo counters
				CG_DrawString( x + 32/2, y - 20, buf, color, AMMO_FONT_SIZE, AMMO_FONT_SIZE, 0, DS_CENTER | DS_PROPORTIONAL );
			} else {
				// vectical ammo counters
				CG_DrawString( x + 39 + (3*AMMO_FONT_SIZE), y + (32-AMMO_FONT_SIZE)/2, buf, color, AMMO_FONT_SIZE, AMMO_FONT_SIZE, 0, DS_RIGHT );
			}
		}

		x += dx;
		y += dy;
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item && weaponSelect == 1 ) {
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
			CG_DrawString( 320, y - 22, name, color, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_PROPORTIONAL | DS_CENTER | DS_FORCE_COLOR );
		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
qboolean CG_WeaponSelectable( int i ) {
	if ( !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW || cg.demoPlayback ) {
		CG_FollowZoomIn_f();
		return;
	}

	original = cg.weaponSelect;

	for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == MAX_WEAPONS ) {
			cg.weaponSelect = 0;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == MAX_WEAPONS ) {
		cg.weaponSelect = original;
	}
}


/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW || cg.demoPlayback ) {
		CG_FollowZoomOut_f();
		return;
	}

	original = cg.weaponSelect;

	for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = MAX_WEAPONS - 1;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == MAX_WEAPONS ) {
		cg.weaponSelect = original;
	}
}


/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW || cg.demoPlayback ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > MAX_WEAPONS-1 ) {
		return;
	}

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	cg.weaponSelect = num;
}


/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;

	cg.weaponSelectTime = cg.time;

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW || cg.demoPlayback ) {
		return;
	}

	for ( i = MAX_WEAPONS-1 ; i > 0 ; i-- ) {
		// utility, not firepower, and never the answer to an empty gun
		if ( i == WP_GRAPPLING_HOOK ) {
			continue;
		}
		if ( CG_WeaponSelectable( i ) ) {
			cg.weaponSelect = i;
			break;
		}
	}
}


/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	if ( ent->number >= 0 && ent->number < MAX_CLIENTS && cent != &cg.predictedPlayerEntity ) {
		// point from external event to client entity
		cent = &cg_entities[ ent->number ];
	}

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// only on initial press: both keep re-firing EV_FIRE_WEAPON while held,
	// which would restart the quad sound every cycle
	if ( ent->weapon == WP_LIGHTNING || ent->weapon == WP_GRAPPLING_HOOK ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
		// In TVD playback, the followed player's entity may appear in both the
		// entity list and the playerstate, causing duplicate CG_FireWeapon
		// calls on different centities.  lightningFiring is only maintained
		// on predictedPlayerEntity, so without this check we hear nonstop
		// initial firing sounds.
		if ( cent != &cg.predictedPlayerEntity
				&& ent->number == cg.snap->ps.clientNum
				&& cg.predictedPlayerEntity.pe.lightningFiring ) {
			return;
		}
	}

	if( ent->weapon == WP_RAILGUN ) {
		cent->pe.railFireTime = cg.time;
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

	// play a sound
	for ( c = 0 ; c < ARRAY_LEN( weap->flashSound ) ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		weap->ejectBrassFunc( cent );
	}

	//Are we the player?
	if (cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		CG_VR_OnWeaponFired( ent->weapon );
	}
}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( weapon_t weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec3_t			sprOrg;
	vec3_t			sprVel;

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
#ifdef MISSIONPACK
	case WP_NAILGUN:
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_nghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_nghitmetal;
		} else {
			sfx = cgs.media.sfx_nghit;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#endif
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
#endif
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = GL_EXPLOSION_RADIUS;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = RL_EXPLOSION_RADIUS;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1.0;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		if (cg_oldRocket.integer == 0) {
			// explosion sprite animation
			VectorMA( origin, 24, dir, sprOrg );
			VectorScale( dir, 64, sprVel );

			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 20, 30 );
		}
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		light = BFG_EXPLOSION_RADIUS;
		lightColor[0] = 0.2f;
		lightColor[1] = 1.0f;
		lightColor[2] = 0.2f;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

	case WP_GRAPPLING_HOOK:
		// the hook bites in: no mark, no explosion
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.sfx_grapplehit );
		return;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_chghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_chghitmetal;
		} else {
			sfx = cgs.media.sfx_chghit;
		}
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
#endif

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, mod, shader, duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
		if ( weapon == WP_RAILGUN ) {
			// colorize with client color
			VectorCopy( cgs.clientinfo[clientNum].color1, le->color );
			le->refEntity.shaderRGBA.rgba[0] = le->color[0] * 255;
			le->refEntity.shaderRGBA.rgba[1] = le->color[1] * 255;
			le->refEntity.shaderRGBA.rgba[2] = le->color[2] * 255;
			le->refEntity.shaderRGBA.rgba[3] = 255;
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if ( weapon == WP_RAILGUN ) {
		float	*color;

		// colorize with client color
		color = cgs.clientinfo[ clientNum ].color1; // was color2

		CG_ImpactMark( mark, origin, dir, random()*360, color[0], color[1], color[2], 1.0, alphaFade, radius, qfalse );
	} else {
		CG_ImpactMark( mark, origin, dir, random()*360, 1.0, 1.0, 1.0, 1.0, alphaFade, radius, qfalse );
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	vec3_t bleedDir;
	// The dir from EV_MISSILE_HIT is the surface normal (pointing outward from
	// the player toward the shooter). Negate it to get the projectile travel
	// direction for the blood spray.
	VectorNegate( dir, bleedDir );
	// Trinity servers send aggregated, damage-scaled blood via EV_BLOOD; bleed
	// per-hit only as the vanilla-server fallback (no real damage here).
	if ( !cgs.trinity ) {
		CG_Bleed( origin, bleedDir, entityNum, 30, qtrue );
	}

	CG_VR_OnHitByMissile( entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_PLASMAGUN:
	case WP_BFG:
#ifdef MISSIONPACK
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:
#endif
		CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		break;
	case WP_GRAPPLING_HOOK:
		// on the victim's entity, so it travels with them under tow
		if ( cgs.media.sfx_grapplebite ) {
			trap_S_StartSound( NULL, entityNum, CHAN_AUTO, cgs.media.sfx_grapplebite );
		}
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum ) {
	trace_t		tr;
	int sourceContentType, destContentType;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	sourceContentType = CG_PointContents( start, 0 );
	destContentType = CG_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		vec3_t	pelletDir;
		// Direction from impact back toward the shooter, matching the
		// EV_MISSILE_HIT convention (surface normal toward shooter); it gets
		// negated in CG_MissileHitPlayer for the blood spray.
		VectorSubtract( start, end, pelletDir );
		VectorNormalize( pelletDir );
		CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, pelletDir, tr.entityNum );
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
		} else {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	{
		const modeConfig_t *gp = Mode_GetConfig( cgs.mode );
		float angle, radius;
		int ring, ringIndex;
		int trueSG = cg_trueShotgun.integer;

		// generate spread pattern
		for ( i = 0 ; i < gp->weapons[WP_SHOTGUN].count ; i++ ) {
			if ( gp->sgPatternType == 2 ) {
				// CPM dual-ring pattern: 8 inner + 8 outer, offset 22.5°
				ring = ( i < 8 ) ? 0 : 1;
				ringIndex = ( i < 8 ) ? i : i - 8;
				radius = ring ? (float)gp->weapons[WP_SHOTGUN].spread * 16.0f : (float)gp->weapons[WP_SHOTGUN].spread * 16.0f * 0.40f;
				angle = 2.0f * M_PI * ringIndex / 8.0f + ( M_PI / 8.0f );
				r = cos( angle ) * radius;
				u = sin( angle ) * radius;
			} else if ( gp->sgPatternType == 1 && trueSG > 0 ) {
				// QL ring pattern: 3 concentric rings (inner 6, middle 6, outer 8)
				if ( i < 6 ) {
					ring = 0; ringIndex = i;
				} else if ( i < 12 ) {
					ring = 1; ringIndex = i - 6;
				} else {
					ring = 2; ringIndex = i - 12;
				}
				radius = (float)gp->weapons[WP_SHOTGUN].spread * 16.0f * ( ring + 1 ) / 3.0f;
				if ( ring == 0 ) {
					angle = 2.0f * M_PI * ringIndex / 6.0f;
				} else if ( ring == 1 ) {
					angle = 2.0f * M_PI * ringIndex / 6.0f - ( 25.0f * M_PI / 180.0f );
				} else {
					angle = 2.0f * M_PI * ringIndex / 8.0f;
				}
				r = cos( angle ) * radius;
				u = sin( angle ) * radius;
			} else {
				// VQ3 random spread, or QL with cg_trueShotgun 0 (cosmetic random)
				r = Q_crandom( &seed ) * gp->weapons[WP_SHOTGUN].spread * 16;
				u = Q_crandom( &seed ) * gp->weapons[WP_SHOTGUN].spread * 16;
			}
			VectorMA( origin, 8192 * 16, forward, end);
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);

			CG_ShotgunPellet( origin, end, otherEntNum );
		}
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es ) {
	vec3_t	v;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t			up;

		contents = CG_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 8 );
			CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
		}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;
	qboolean	hasStart = qfalse;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
		hasStart = qtrue;

		if ( cg_tracerChance.value > 0 ) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		vec3_t	dir;
		if ( hasStart ) {
			// Calculate direction from shooter to impact point
			VectorSubtract( end, start, dir );
			VectorNormalize( dir );
		} else {
			// No valid shooter position, use zero vector for omnidirectional spray
			VectorClear( dir );
		}
		if ( !cgs.trinity ) {
			CG_Bleed( end, dir, fleshEntityNum, 3, qtrue );	// vanilla fallback (no real damage)
		}
	} else {
		CG_MissileHitWall( WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT );
	}

}
