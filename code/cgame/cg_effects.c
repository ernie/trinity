// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail( const vec3_t start, const vec3_t end, float spacing ) {
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA( move, i, vec, move );

	VectorScale (vec, spacing, vec);

	for ( ; i < len; i += spacing ) {
		localEntity_t	*le;
		refEntity_t		*re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000 + random() * 250;
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );

		re = &le->refEntity;
		if ( intShaderTime )
			re->u.intShaderTime = cg.time;
		else
			re->u.shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0xff;
		re->shaderRGBA.rgba[2] = 0xff;
		re->shaderRGBA.rgba[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy( move, le->pos.trBase );
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		VectorAdd (move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *CG_SmokePuff( const vec3_t p, const vec3_t vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;

	if ( intShaderTime )
		re->u.intShaderTime = startTime;
	else
		re->u.shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0 / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	}
	le->color[0] = r;
	le->color[1] = g; 
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0xff;
		re->shaderRGBA.rgba[2] = 0xff;
		re->shaderRGBA.rgba[3] = 0xff;
	} else {
		re->shaderRGBA.rgba[0] = le->color[0] * 0xff;
		re->shaderRGBA.rgba[1] = le->color[1] * 0xff;
		re->shaderRGBA.rgba[2] = le->color[2] * 0xff;
		re->shaderRGBA.rgba[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect( const vec3_t origin ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;

	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

#ifndef MISSIONPACK
	re->customShader = cgs.media.teleportEffectShader;
#endif
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear( re->axis );

	VectorCopy( origin, re->origin );

#ifdef MISSIONPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif
}


#ifdef MISSIONPACK
/*
===============
CG_LightningBoltBeam
===============
*/
void CG_LightningBoltBeam( vec3_t start, vec3_t end ) {
	localEntity_t	*le;
	refEntity_t		*beam;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SHOWREFENTITY;
	le->startTime = cg.time;
	le->endTime = cg.time + 50;

	beam = &le->refEntity;

	VectorCopy( start, beam->origin );
	// this is the end point
	VectorCopy( end, beam->oldorigin );

	beam->reType = RT_LIGHTNING;
	beam->customShader = cgs.media.lightningShader;
}


/*
==================
CG_KamikazeEffect
==================
*/
void CG_KamikazeEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_KAMIKAZE;
	le->startTime = cg.time;
	le->endTime = cg.time + 3000;//2250;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	VectorClear(le->angles.trBase);

	re = &le->refEntity;

	re->reType = RT_MODEL;

	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.kamikazeEffectModel;

	VectorCopy( org, re->origin );

}

/*
==================
CG_ObeliskExplode
==================
*/
void CG_ObeliskExplode( vec3_t org, int entityNum ) {
	localEntity_t	*le;
	vec3_t origin;

	// create an explosion
	VectorCopy( org, origin );
	origin[2] += 64;
	le = CG_MakeExplosion( origin, vec3_origin,
						   cgs.media.dishFlashModel,
						   cgs.media.rocketExplosionShader,
						   600, qtrue );
	le->light = 300;
	le->lightColor[0] = 1;
	le->lightColor[1] = 0.75;
	le->lightColor[2] = 0.0;
}

/*
==================
CG_ObeliskPain
==================
*/
void CG_ObeliskPain( vec3_t org ) {
	float r;
	sfxHandle_t sfx;

	// hit sound
	r = rand() & 3;
	if ( r < 2 ) {
		sfx = cgs.media.obeliskHitSound1;
	} else if ( r == 2 ) {
		sfx = cgs.media.obeliskHitSound2;
	} else {
		sfx = cgs.media.obeliskHitSound3;
	}
	trap_S_StartSound ( org, ENTITYNUM_NONE, CHAN_BODY, sfx );
}


/*
==================
CG_InvulnerabilityImpact
==================
*/
void CG_InvulnerabilityImpact( vec3_t org, vec3_t angles ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int				r;
	sfxHandle_t		sfx;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_INVULIMPACT;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;

	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityImpactModel;

	VectorCopy( org, re->origin );
	AnglesToAxis( angles, re->axis );

	r = rand() & 3;
	if ( r < 2 ) {
		sfx = cgs.media.invulnerabilityImpactSound1;
	} else if ( r == 2 ) {
		sfx = cgs.media.invulnerabilityImpactSound2;
	} else {
		sfx = cgs.media.invulnerabilityImpactSound3;
	}
	trap_S_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, sfx );
}

/*
==================
CG_InvulnerabilityJuiced
==================
*/
void CG_InvulnerabilityJuiced( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_INVULJUICED;
	le->startTime = cg.time;
	le->endTime = cg.time + 10000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;

	if ( intShaderTime )
		re->u.intShaderTime = cg.time;
	else
		re->u.shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityJuicedModel;

	VectorCopy( org, re->origin );
	VectorClear(angles);
	AnglesToAxis( angles, re->axis );

	trap_S_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, cgs.media.invulnerabilityJuicedSound );
}
#endif


/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum( int client, const vec3_t origin, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlum.integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	
	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;
	
	VectorCopy( origin, le->pos.trBase );
	if ( origin[2] >= lastPos[2] - 20 && origin[2] <= lastPos[2] + 20 ) {
		le->pos.trBase[2] -= 20;
	}

	//CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(origin, lastPos);

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}

/*
==================
CG_DamagePlum
==================
*/
void CG_DamagePlum( vec3_t org, int damage ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;
	float			random_x, random_y;

	if ( cg_damagePlums.integer == 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_DAMAGEPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = 1.0;
	le->color[1] = 0.5;
	le->color[2] = 0.0;
	le->color[3] = 1.0;
	le->radius = damage;

	VectorCopy( org, le->pos.trBase );

	random_x = (random() * 2.0 - 1.0);
	random_y = (random() * 2.0 - 1.0);

	le->pos.trDelta[0] = random_x;
	le->pos.trDelta[1] = random_y;
	le->pos.trDelta[2] = 0.5 + random() * 0.5;

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion( const vec3_t origin, const vec3_t dir,
								qhandle_t hModel, qhandle_t shader,
								int msec, qboolean isSprite ) {
	float			ang;
	localEntity_t	*ex;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CG_MakeExplosion: msec = %i", msec );
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale( dir, 16, tmpVec );
		VectorAdd( tmpVec, origin, newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, newOrigin );

		// set axis with random rotate
		if ( !dir ) {
			AxisClear( ex->refEntity.axis );
		} else {
			ang = rand() % 360;
			VectorCopy( dir, ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	if ( intShaderTime )
		ex->refEntity.u.intShaderTime = ex->startTime;
	else
		ex->refEntity.u.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( newOrigin, ex->refEntity.origin );
	VectorCopy( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit.
Spawns cosmetic floating gouts (which never mark) plus instant surface decals,
both counts scaled by damage. dir = damage direction. damage = the hit's damage
(supplied per victim per frame by the server via EV_BLOOD).
=================
*/
// Splat count scales with damage (~damage/3), capped.
#define BLOOD_DMG_PER_SPLAT	3
#define BLOOD_SPLAT_CAP		8

void CG_Bleed( vec3_t origin, vec3_t dir, int entityNum, int damage, qboolean directional ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int				i;
	int				count;
	vec3_t			baseDir;
	vec3_t			traceStart, traceDir, splatEnd;
	trace_t			trace;
	qboolean		isPlayer;
	float			puffRadius;
	int				puffDuration;

	if ( !cg_blood.integer ) {
		return;
	}

	isPlayer = ( entityNum == cg.snap->ps.clientNum );

	// com_blood 1 = classic (legacy sprite); 2+ = enhanced (gouts + decals below)
	if ( cg_blood.integer < 2 ) {
		le = CG_AllocLocalEntity();
		le->leType = LE_EXPLOSION;
		le->startTime = cg.time;
		le->endTime = le->startTime + 500;

		VectorCopy( origin, le->refEntity.origin );
		le->refEntity.reType = RT_SPRITE;
		le->refEntity.rotation = rand() % 360;
		le->refEntity.radius = 24;
		le->refEntity.customShader = cgs.media.bloodExplosionShader;

		if ( isPlayer ) {
			le->refEntity.renderfx |= RF_THIRD_PERSON;
		}
		return;
	}

	// direction basis (fall back to up)
	if ( dir && ( dir[0] != 0 || dir[1] != 0 || dir[2] != 0 ) ) {
		VectorNormalize2( dir, baseDir );
	} else {
		VectorSet( baseDir, 0, 0, 1 );
	}


	// underwater: a single rising puff, no spray/decals
	if ( CG_PointContents( origin, -1 ) & MASK_WATER ) {
		le = CG_SmokePuff( origin, vec3_origin,
			2 + random() * 3, 1, 1, 1, 1,
			300 + random() * 200, cg.time, 0, 0,
			cgs.media.bloodTrailShader );
		le->leType = LE_FALL_SCALE_FADE;
		le->pos.trDelta[2] = -2;
		if ( isPlayer ) {
			le->refEntity.renderfx |= RF_THIRD_PERSON;
		}
		return;
	}

	// damage -> splat count (~damage/3, capped)
	count = ( damage + BLOOD_DMG_PER_SPLAT - 1 ) / BLOOD_DMG_PER_SPLAT;
	if ( count < 1 ) {
		count = 1;
	}
	if ( count > BLOOD_SPLAT_CAP ) {
		count = BLOOD_SPLAT_CAP;
	}

	// Cosmetic floating gouts (never mark; the decals below are the surface blood).
	for ( i = 0; i < count; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leFlags = LEF_PUFF_DONT_SCALE | LEF_NO_MARK;
		le->leType = LE_BLOOD_PARTICLE;
		le->startTime = cg.time;
		le->endTime = cg.time + 700 + random() * 300;
		le->lifeRate = 1.0f / ( le->endTime - le->startTime );

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;
		VectorMA( origin, random() * 24, baseDir, le->pos.trBase );
		VectorScale( baseDir, 60 + random() * 80, le->pos.trDelta );
		le->pos.trDelta[0] += crandom() * 40;
		le->pos.trDelta[1] += crandom() * 40;
		le->pos.trDelta[2] += 40 + random() * 50;

		VectorCopy( le->pos.trBase, re->origin );
		re->reType = RT_SPRITE;
		re->rotation = rand() % 360;
		re->radius = 18 + random() * 18;
		re->customShader = cgs.media.bloodGoutShader;
		re->u.shaderTime = cg.time / 1000.0f;
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0xff;
		re->shaderRGBA.rgba[2] = 0xff;
		re->shaderRGBA.rgba[3] = 0xff;

		le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0f;
		le->radius = re->radius;

		if ( isPlayer ) {
			re->renderfx |= RF_THIRD_PERSON;
		}
	}

	// Instant surface decals: per decal jitter the start point across a small
	// disc (so marks spread instead of stacking), then trace outward and stamp
	// where it hits. A direct hit sprays along the impact momentum (painting
	// the surface behind the victim) with a downward pull so open hits still
	// pool; splash has no real impact vector, so it sprays omnidirectionally.
	for ( i = 0; i < count; i++ ) {
		traceStart[0] = origin[0] + crandom() * 24;
		traceStart[1] = origin[1] + crandom() * 24;
		traceStart[2] = origin[2] + crandom() * 8;
		if ( directional ) {
			traceDir[0] = baseDir[0] + crandom() * 0.5f;
			traceDir[1] = baseDir[1] + crandom() * 0.5f;
			traceDir[2] = baseDir[2] + crandom() * 0.4f - 0.5f;	// forward, pulled down
		} else {
			traceDir[0] = crandom();
			traceDir[1] = crandom();
			traceDir[2] = crandom() - 1.0f;		// omnidirectional, biased to the floor
		}
		VectorNormalize( traceDir );
		VectorMA( traceStart, 96, traceDir, splatEnd );
		CG_Trace( &trace, traceStart, NULL, NULL, splatEnd, -1, CONTENTS_SOLID );
		if ( trace.fraction < 1.0f ) {
			CG_ImpactMark( cgs.media.bloodSplatShader[ rand() & 3 ], trace.endpos,
				trace.plane.normal, random() * 360, 1, 1, 1, 1, qtrue,
				30 + random() * 30, qfalse );
		}
	}

	// small blood mist at the wound
	puffRadius = 2 + random() * 3;
	puffDuration = 300 + random() * 200;
	le = CG_SmokePuff( origin, vec3_origin,
		puffRadius, 1, 1, 1, 1, puffDuration,
		cg.time, 0, 0, cgs.media.bloodTrailShader );
	le->leType = LE_FALL_SCALE_FADE;
	le->pos.trDelta[2] = 4;
	if ( isPlayer ) {
		le->refEntity.renderfx |= RF_THIRD_PERSON;
	}
}



/*
==================
CG_LaunchGib
==================
*/
static void CG_LaunchGib( const vec3_t origin, const vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	// tumble in flight; CG_AddFragment evaluates le->angles
	le->leFlags |= LEF_TUMBLE;
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand() % 360;
	le->angles.trBase[1] = rand() % 360;
	le->angles.trBase[2] = rand() % 360;
	le->angles.trDelta[0] = crandom() * 400;
	le->angles.trDelta[1] = crandom() * 400;
	le->angles.trDelta[2] = crandom() * 400;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

/*
===================
CG_GibBloodSpray

Enhanced gib blood: several streams shot outward from the gib origin, each
ray-traced so it stops at walls, emitting a line of animated gout sprites, plus
one large ground splat.
===================
*/
#define	GIB_STREAM_NUM		12		// number of streams
#define	GIB_STREAM_COUNT	12		// sprites per stream
#define	GIB_STREAM_DIST		220.0f	// max stream length

static void CG_GibBloodSpray( const vec3_t org ) {
	int				i, j;
	vec3_t			o, v, tmp, end;
	trace_t			tr;
	float			dist;
	localEntity_t	*le;
	refEntity_t		*re;

	for ( i = 0; i < GIB_STREAM_NUM; i++ ) {
		// start point jittered around the origin, biased slightly upward
		o[0] = org[0] + crandom() * 8;
		o[1] = org[1] + crandom() * 8;
		o[2] = org[2] + 8 + crandom() * 12;

		// outward direction, mostly upward
		v[0] = crandom();
		v[1] = crandom();
		v[2] = 0.2f + random();

		// trace to a wall so the stream doesn't punch through geometry
		VectorMA( o, GIB_STREAM_DIST, v, tmp );
		CG_Trace( &tr, o, NULL, NULL, tmp, -1, CONTENTS_SOLID );
		dist = GIB_STREAM_DIST * tr.fraction;

		for ( j = 1; j < GIB_STREAM_COUNT; j++ ) {
			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leFlags = LEF_PUFF_DONT_SCALE | LEF_NO_MARK;
			le->leType = LE_BLOOD_PARTICLE;
			le->startTime = cg.time;
			le->endTime = cg.time + 500 + random() * 400;
			le->lifeRate = 1.0f / ( le->endTime - le->startTime );

			le->pos.trType = TR_GRAVITY;
			le->pos.trTime = cg.time;
			VectorCopy( o, le->pos.trBase );
			// velocity grows along the stream; add upward kick and jitter
			le->pos.trDelta[0] = v[0] * dist * ( (float)j / GIB_STREAM_COUNT ) + crandom() * 2;
			le->pos.trDelta[1] = v[1] * dist * ( (float)j / GIB_STREAM_COUNT ) + crandom() * 2;
			le->pos.trDelta[2] = v[2] * dist * ( (float)j / GIB_STREAM_COUNT ) + crandom() * 2 + 100;

			VectorCopy( o, re->origin );
			re->reType = RT_SPRITE;
			re->rotation = rand() % 360;
			re->radius = 18 + random() * 22;	// 18-40, matches on-hit gout scale
			re->customShader = cgs.media.bloodGoutShader;
			re->u.shaderTime = cg.time / 1000.0f;
			re->shaderRGBA.rgba[0] = 0xff;
			re->shaderRGBA.rgba[1] = 0xff;
			re->shaderRGBA.rgba[2] = 0xff;
			re->shaderRGBA.rgba[3] = 0xff;

			le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0f;
			le->radius = re->radius;
		}
	}

	// large ground splat under the gib origin
	VectorCopy( org, end );
	end[2] -= 64;
	CG_Trace( &tr, org, NULL, NULL, end, -1, CONTENTS_SOLID );
	if ( tr.fraction < 1.0f ) {
		CG_ImpactMark( cgs.media.bloodSplatShader[ rand() & 3 ], tr.endpos,
			tr.plane.normal, random() * 360, 1, 1, 1, 1, qtrue,
			60 + random() * 60, qfalse );	// 60-120, scale-corrected gib ground splat
	}
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define	GIB_VELOCITY	250
#define	GIB_JUMP		250
void CG_GibPlayer( const vec3_t playerOrigin ) {
	vec3_t	origin, velocity;

	if ( !cg_blood.integer ) {
		return;
	}

	CG_GibBloodSpray( playerOrigin );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	if ( rand() & 1 ) {
		CG_LaunchGib( origin, velocity, cgs.media.gibSkull );
	} else {
		CG_LaunchGib( origin, velocity, cgs.media.gibBrain );
	}

	// allow gibs to be turned off for speed
	if ( !cg_gibs.integer ) {
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibAbdomen );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibArm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibChest );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFist );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFoot );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibForearm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibIntestine );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );
}

/*
==================
CG_LaunchExplode
==================
*/
void CG_LaunchExplode( vec3_t origin, vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 10000 + random() * 6000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

#define	EXP_VELOCITY	100
#define	EXP_JUMP		150
/*
===================
CG_BigExplode

Generated a bunch of gibs launching out from the bodies location
===================
*/
void CG_BigExplode( vec3_t playerOrigin ) {
	vec3_t	origin, velocity;

	if ( !cg_blood.integer ) {
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*1.5;
	velocity[1] = crandom()*EXP_VELOCITY*1.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.0;
	velocity[1] = crandom()*EXP_VELOCITY*2.0;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.5;
	velocity[1] = crandom()*EXP_VELOCITY*2.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );
}

