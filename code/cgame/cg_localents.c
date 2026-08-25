// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define	MAX_LOCAL_ENTITIES	2048
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void	CG_InitLocalEntities( void ) {
	int		i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will always succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t	*CG_AllocLocalEntity( void ) {
	localEntity_t	*le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

// min gib speed (u/s) to keep trailing, so settling gibs don't stack trails
#define GIB_TRAIL_SPEED	200

/*
================
CG_BloodTrail

Leave expanding blood puffs behind gibs
================
*/
void CG_BloodTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;

	// Classic blood: sparse expanding puffs behind the gib (the pre-overhaul
	// trail), with no animated gouts or projected decals.
	if ( cg_blood.integer < 2 ) {
		step = 150;
		t = step * ( ( cg.time - cg.frametime + step ) / step );
		t2 = step * ( cg.time / step );
		for ( ; t <= t2; t += step ) {
			BG_EvaluateTrajectory( &le->pos, t, newOrigin );
			blood = CG_SmokePuff( newOrigin, vec3_origin,
				20, 1, 1, 1, 1, 2000, t, 0, 0,
				cgs.media.bloodTrailShader );
			blood->leType = LE_FALL_SCALE_FADE;
			blood->pos.trDelta[2] = 40;
		}
		return;
	}

	// Modern: gout trail by distance traveled (speed-independent). Accumulate
	// from the last emission point, dropping a gout every cg_bloodTrailStep units.
	{
		vec3_t	dir;
		float	dist;
		int		n;
		int		step;

		step = cg_bloodTrailStep.integer;
		if ( step < 1 ) {
			step = 1;
		}

		BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );
		VectorSubtract( newOrigin, le->trailOrigin, dir );
		dist = VectorNormalize( dir );

		for ( n = 0; dist >= step && n < 64; n++ ) {
			VectorMA( le->trailOrigin, step, dir, le->trailOrigin );
			dist -= step;

			blood = CG_SmokePuff( le->trailOrigin, vec3_origin,
						  ( 24 + random() * 16 ) * cg_bloodGoutScale.value, 1, 1, 1, 1,
						  300 + random() * 66,		// ~333ms: one 30fps animMap cycle
						  cg.time, 0, LEF_PUFF_DONT_SCALE,
						  cgs.media.bloodGoutShader );
			blood->pos.trDelta[2] = -10;	// gentle settle
			if ( animFrame ) {
				blood->refEntity.renderfx |= RF_ANIMFRAME;	// gout plays once across its life
			}
			// slight tumble so trail puffs don't read as identical stamps
			blood->angles.trType = TR_LINEAR;
			blood->angles.trTime = cg.time;
			blood->angles.trBase[0] = blood->refEntity.rotation;
			blood->angles.trDelta[0] = crandom() * 50;	// deg/sec

			if ( cg_bloodTrailDecalStep.integer > 0 &&
				 n % cg_bloodTrailDecalStep.integer == 0 ) {
				CG_BloodDecal( le->trailOrigin, 16 + random() * 16 );
			}
		}
	}
}


/*
================
CG_FragmentBounceMark
================
*/
void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
	int			radius;

	if ( le->leMarkType == LEMT_BLOOD ) {

		// Modern: radial projected decal. Classic: legacy single blood mark.
		if ( cg_blood.integer >= 2 ) {
			CG_BloodDecal( trace->endpos, 16 + ( rand() & 31 ) );
		} else {
			radius = 16 + ( rand() & 31 );
			CG_ImpactMark( cgs.media.bloodMarkShader, trace->endpos, trace->plane.normal,
				random() * 360, 1, 1, 1, 1, qtrue, radius, qfalse );
		}
	} else if ( le->leMarkType == LEMT_BURN ) {

		radius = 8 + (rand()&15);
		CG_ImpactMark( cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360,
			1,1,1,1, qtrue, radius, qfalse );
	}


	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}

/*
================
CG_FragmentBounceSound
================
*/
void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace ) {
	if ( le->leBounceSoundType == LEBS_BLOOD ) {
		// half the gibs will make splat sounds
		if ( rand() & 1 ) {
			int r = rand()&3;
			sfxHandle_t	s;

			if ( r == 0 ) {
				s = cgs.media.gibBounce1Sound;
			} else if ( r == 1 ) {
				s = cgs.media.gibBounce2Sound;
			} else {
				s = cgs.media.gibBounce3Sound;
			}
			trap_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}
	} else if ( le->leBounceSoundType == LEBS_BRASS ) {

	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->leBounceSoundType = LEBS_NONE;
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;


	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid || 
		( trace->plane.normal[2] > 0 && 
		( le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) ) {
		le->pos.trType = TR_STATIONARY;
	} else {

	}
}

/*
================
CG_AddFragment
================
*/
static void CG_AddFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	oldZ;
		
		t = le->endTime - cg.time;
		if ( t < SINK_TIME ) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy( le->refEntity.origin, le->refEntity.lightingOrigin );
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * ( 1.0 - (float)t / SINK_TIME );
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.origin[2] = oldZ;
		} else {
			trap_R_AddRefEntityToScene( &le->refEntity );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
		}

		trap_R_AddRefEntityToScene( &le->refEntity );

		// modern trails across bounces while fast; classic stops after one
		if ( le->leFlags & LEF_BLOOD_TRAIL ) {
			if ( VectorLengthSquared( le->pos.trDelta ) > GIB_TRAIL_SPEED * GIB_TRAIL_SPEED ) {
				CG_BloodTrail( le );
			} else {
				// gate closed: keep the marker on the gib so a later speed-up
				// doesn't bridge the gap and restamp the path it already flew
				VectorCopy( newOrigin, le->trailOrigin );
			}
		} else if ( le->leBounceSoundType == LEBS_BLOOD ) {
			CG_BloodTrail( le );
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( CG_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// leave a mark
	CG_FragmentBounceMark( le, &trace );

	// do a bouncy sound
	CG_FragmentBounceSound( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );

	// resume the trail from the impact point so the post-bounce segment
	// hugs the corner instead of drawing a chord across it
	if ( le->leFlags & LEF_BLOOD_TRAIL ) {
		VectorCopy( trace.endpos, le->trailOrigin );
	}

	trap_R_AddRefEntityToScene( &le->refEntity );
}

/*
====================
CG_GrapplePadSettle

Weight-forward nose pull: smoothstep pitch toward straight down, applied over
the raw trajectory angles at draw time. Smoothstep because exp() is not in
the QVM libc, and arriving beats an infinite tail.
====================
*/
static void CG_GrapplePadSettle( float t, vec3_t ang ) {
	float	s;

	if ( PAD_FALL_SETTLE <= 0 ) {
		return;
	}
	s = t * 1000.0f / PAD_FALL_SETTLE;
	if ( s > 1.0f ) {
		s = 1.0f;
	}
	s = s * s * ( 3.0f - 2.0f * s );
	ang[PITCH] += AngleSubtract( 90.0f, ang[PITCH] ) * s;
}

/*
====================
CG_AddGrapplePad

Claws fold back, the nose settles toward straight down, and the whole thing
fades. Settling is a smoothstep approach rather than a rotation: the claws and
contact face make the pad weight-forward, so it converges on an attitude like a
dart instead of turning past it.
====================
*/
static void CG_AddGrapplePad( localEntity_t *le ) {
	refEntity_t	*re = &le->refEntity;
	vec3_t		newOrigin, ang, cleanOrigin, cleanOldOrigin, vel;
	trace_t		trace;
	float		t, u, c, retract, embed;
	float		fpos;
	int		fi, hard;

	t = ( cg.time - le->startTime ) * 0.001f;

	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );
	if ( le->pos.trType != TR_STATIONARY ) {
		CG_Trace( &trace, re->origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
		if ( trace.fraction < 1.0f ) {
			// reflect the CURRENT velocity: the trajectory's initial
			// delta is dwarfed by the gravity term and reads as no bounce.
			// CG_ReflectVelocity parks weak floor rebounds TR_STATIONARY,
			// which ends the hop: heavy machinery does not litter
			// read before the reflect scrubs it
			BG_EvaluateTrajectoryDelta( &le->pos, cg.time, vel );
			hard = VectorLength( vel ) >= PAD_FALL_CLANK_SPEED;
			CG_ReflectVelocity( le, &trace );

			// past the hold the pad is fading out, and a ghost should not
			// clatter
			if ( t < PAD_FALL_TIME * PAD_FALL_HOLD * 0.001f
					&& cgs.media.sfx_grappleclank[ hard ] ) {
				trap_S_StartSound( trace.endpos, ENTITYNUM_WORLD, CHAN_AUTO,
					cgs.media.sfx_grappleclank[ hard ] );
			}

			// ground contact scrubs spin; coming to rest stops it cold
			BG_EvaluateTrajectory( &le->angles, cg.time, ang );
			if ( le->pos.trType == TR_STATIONARY ) {
				// freeze the DISPLAYED attitude: the settle pull overrode
				// pitch all fall while raw tumble integrated underneath, so
				// freezing the raw angles snaps the pad crooked
				CG_GrapplePadSettle( t, ang );
				le->angles.trType = TR_STATIONARY;
				VectorClear( le->angles.trDelta );
			} else {
				VectorScale( le->angles.trDelta, PAD_FALL_SPIN_DAMP,
					le->angles.trDelta );
			}
			VectorCopy( ang, le->angles.trBase );
			le->angles.trTime = cg.time;
			VectorCopy( trace.endpos, re->origin );
		} else {
			VectorCopy( newOrigin, re->origin );
		}
	} else {
		VectorCopy( newOrigin, re->origin );
	}
	VectorCopy( re->origin, re->oldorigin );

	BG_EvaluateTrajectory( &le->angles, cg.time, ang );
	// at rest trBase already holds the frozen, settle-corrected attitude;
	// pulling further would move a pad that has stopped
	if ( le->angles.trType != TR_STATIONARY ) {
		CG_GrapplePadSettle( t, ang );
	}
	AnglesToAxis( ang, re->axis );

	// claws let go before the pad does, so walk the swing ramp frame by frame
	// so the rigid blade never lerps across more than one ~8 deg step.  The
	// ease is baked into the frames, so a linear walk plays the authored curve
	retract = PAD_FALL_RETRACT > 0
		? ( t * 1000.0f > PAD_FALL_RETRACT ? 1.0f : t * 1000.0f / PAD_FALL_RETRACT )
		: 1.0f;
	fpos = ( 1.0f - retract ) * PAD_FRAME_CLAMPED;
	fi = (int)fpos;
	if ( fi >= PAD_FRAME_CLAMPED ) {
		re->oldframe = re->frame = PAD_FRAME_CLAMPED;
		re->backlerp = 0;
	} else {
		re->oldframe = fi;
		re->frame = fi + 1;
		re->backlerp = 1.0f - ( fpos - fi );
	}

	// magnitude of the draw-only embed offset applied just before the draw
	// call below; withdraws to zero over the same window the claws fold back
	embed = GRAPPLE_PAD_EMBED * ( 1.0f - retract );

	// hold, then fade, keeping the owner's tint through it
	u = ( cg.time - le->startTime ) * le->lifeRate;
	c = ( u <= PAD_FALL_HOLD ) ? 1.0f
		: 1.0f - ( u - PAD_FALL_HOLD ) / ( 1.0f - PAD_FALL_HOLD );
	if ( c < 0 ) {
		c = 0;
	}
	re->shaderRGBA.rgba[0] = le->color[0] * 0xff;
	re->shaderRGBA.rgba[1] = le->color[1] * 0xff;
	re->shaderRGBA.rgba[2] = le->color[2] * 0xff;
	re->shaderRGBA.rgba[3] = 0xff;

	// only while the cut is travelling: pad_unform runs hot and washes the
	// whole surface, which would glow the pad the entire way down
	if ( c < 1.0f ) {
		// once, where the cut starts; landings past the hold stay silent, so
		// a long drop's only mark is this
		if ( !( le->leFlags & LEF_SOUND1 ) ) {
			le->leFlags |= LEF_SOUND1;
			if ( cgs.media.sfx_grappleunform ) {
				trap_S_StartSound( re->origin, ENTITYNUM_WORLD, CHAN_AUTO,
					cgs.media.sfx_grappleunform );
			}
		}
		CG_GrapplePadUnform( re->shaderRGBA.rgba, c );
		re->customShader = cgs.media.grapplePadUnformShader;
	} else {
		re->customShader = 0;		// the model's own shader: solid, idle glow
	}

	// re->origin/oldorigin hold the clean, un-embedded physics position
	// everywhere except across this draw call: the same save/offset/draw/restore
	// idiom as CG_AddFragment's ground-sink offset earlier in this file.
	// GRAPPLE_PAD_EMBED is draw-only: it must not be there when CG_Trace
	// above reads re->origin next frame
	VectorCopy( re->origin, cleanOrigin );
	VectorCopy( re->oldorigin, cleanOldOrigin );
	VectorMA( re->origin, embed, re->axis[0], re->origin );
	VectorMA( re->oldorigin, embed, re->axis[0], re->oldorigin );

	trap_R_AddRefEntityToScene( re );

	VectorCopy( cleanOrigin, re->origin );
	VectorCopy( cleanOldOrigin, re->oldorigin );
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
static void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;

	if ( re->reType == RT_RAIL_CORE && cg_railTrailRadius.integer && linearLight ) {
		trap_R_AddLinearLightToScene( re->origin, re->oldorigin, cg_railTrailRadius.value, le->color[0]*c, le->color[1]*c, le->color[2]*c );
	}

	c *= 0xff;

	re->shaderRGBA.rgba[0] = le->color[0] * c;
	re->shaderRGBA.rgba[1] = le->color[1] * c;
	re->shaderRGBA.rgba[2] = le->color[2] * c;
	re->shaderRGBA.rgba[3] = le->color[3] * c;

	if ( intShaderTime )
		trap_R_AddRefEntityToScene2( re );
	else
		trap_R_AddRefEntityToScene( re );
}


/*
==================
CG_UpdateGoutSprite

Per-frame sprite updates for blood gouts: RF_ANIMFRAME maps the animMap frame to
the life fraction (plays once, no loop); a TR_LINEAR angles trajectory spins the
billboard slightly so the sprite tumbles instead of reading as a stamp.
==================
*/
#define	BLOOD_GOUT_FRAMES	10		// frame count of the bloodGout animMap

static void CG_UpdateGoutSprite( localEntity_t *le ) {
	refEntity_t	*re = &le->refEntity;

	if ( re->renderfx & RF_ANIMFRAME ) {
		float frac = 1.0f - ( le->endTime - cg.time ) * le->lifeRate;
		if ( frac < 0.0f ) {
			frac = 0.0f;
		} else if ( frac > 0.999f ) {
			frac = 0.999f;	// stay on the last frame; never wrap to 0
		}
		re->frame = (int)( frac * BLOOD_GOUT_FRAMES );
	}

	if ( le->angles.trType == TR_LINEAR ) {
		re->rotation = le->angles.trBase[0]
			+ le->angles.trDelta[0] * ( cg.time - le->angles.trTime ) * 0.001f;
	}
}

/*
==================
CG_BloodGoutCulled

True when a blood gout is too close to draw this frame: the eye is inside the
sprite, or it covers more than cg_bloodNearCull of the screen height. The caller
skips the draw without freeing, so the gout reappears once it pulls back.
==================
*/
static qboolean CG_BloodGoutCulled( const refEntity_t *re ) {
	vec3_t	delta;
	float	dist;

	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	dist = VectorLength( delta );
	if ( dist <= re->radius ) {
		return qtrue;
	}
	if ( cg_bloodNearCull.value > 0 &&
		 re->radius > cg_bloodNearCull.value * dist * tan( cg.refdef.fov_y * ( M_PI / 360.0f ) ) ) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
CG_AddMoveScaleFade
==================
*/
static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	if ( le->fadeInTime > le->startTime && cg.time < le->fadeInTime ) {
		// fade / grow time
		c = 1.0 - (float) ( le->fadeInTime - cg.time ) / ( le->fadeInTime - le->startTime );
	}
	else {
		// fade / grow time
		c = ( le->endTime - cg.time ) * le->lifeRate;
	}

	re->shaderRGBA.rgba[3] = 0xff * c * le->color[3];

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// Blood trail gouts also cull on screen coverage (see CG_BloodGoutCulled)
	if ( re->customShader == cgs.media.bloodGoutShader && CG_BloodGoutCulled( re ) ) {
		return;
	}

	// if the view would be "inside" the sprite, skip the draw without freeing
	// (overdraw guard); it draws again once the view pulls back
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		return;
	}

	CG_UpdateGoutSprite( le );

	if ( intShaderTime )
		trap_R_AddRefEntityToScene2( re );
	else
		trap_R_AddRefEntityToScene( re );
}


/*
===================
CG_EmitPolyVerts
===================
*/
static void CG_EmitPolyVerts( const refEntity_t *re )
{
	polyVert_t	verts[4];
	float		sinR, cosR;
	float		angle;
	vec3_t		left, up;
	int			i;

	if ( spritePolyTrap ) {
		// engine-oriented billboard: same poly batching, orientation
		// resolved by the engine per view (the path below bakes this
		// frame's view axes into the quad at add time)
		trap_R_AddSpritePolyToScene( re->customShader, re->origin,
			re->radius, re->radius, re->rotation, re->shaderRGBA.rgba );
		return;
	}

	if ( re->rotation )
	{
		angle = M_PI * re->rotation / 180.0;
		sinR = sin( angle );
		cosR = cos( angle );

		VectorScale( cg.refdef.viewaxis[1], cosR * re->radius, left );
		VectorMA( left, -sinR * re->radius, cg.refdef.viewaxis[2], left );

		VectorScale( cg.refdef.viewaxis[2], cosR * re->radius, up );
		VectorMA( up, sinR * re->radius, cg.refdef.viewaxis[1], up );
	}
	else
	{
		VectorScale( cg.refdef.viewaxis[1], re->radius, left );
		VectorScale( cg.refdef.viewaxis[2], re->radius, up );
	}

	verts[0].xyz[0] = re->origin[0] + left[0] + up[0];
	verts[0].xyz[1] = re->origin[1] + left[1] + up[1];
	verts[0].xyz[2] = re->origin[2] + left[2] + up[2];
	verts[0].st[0] = 0.0;
	verts[0].st[1] = 0.0;

	verts[1].xyz[0] = re->origin[0] - left[0] + up[0];
	verts[1].xyz[1] = re->origin[1] - left[1] + up[1];
	verts[1].xyz[2] = re->origin[2] - left[2] + up[2];
	verts[1].st[0] = 1.0;
	verts[1].st[1] = 0.0;

	verts[2].xyz[0] = re->origin[0] - left[0] - up[0];
	verts[2].xyz[1] = re->origin[1] - left[1] - up[1];
	verts[2].xyz[2] = re->origin[2] - left[2] - up[2];
	verts[2].st[0] = 1.0;
	verts[2].st[1] = 1.0;

	verts[3].xyz[0] = re->origin[0] + left[0] - up[0];
	verts[3].xyz[1] = re->origin[1] + left[1] - up[1];
	verts[3].xyz[2] = re->origin[2] + left[2] - up[2];
	verts[3].st[0] = 0.0;
	verts[3].st[1] = 1.0;

	for ( i = 0; i < 4; i++ )
	{
		verts[i].modulate[0] = re->shaderRGBA.rgba[0];
		verts[i].modulate[1] = re->shaderRGBA.rgba[1];
		verts[i].modulate[2] = re->shaderRGBA.rgba[2];
		verts[i].modulate[3] = re->shaderRGBA.rgba[3];
	}

	trap_R_AddPolyToScene( re->customShader, 4, verts );
}


/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA.rgba[3] = 0xff * c * le->color[3];
	re->radius = le->radius * ( 1.0 - c ) + 8;

	// if the view would be "inside" the sprite, skip the draw without freeing
	// (overdraw guard); it draws again once the view pulls back
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLengthSquared( delta );
	if ( len < le->radius * le->radius ) {
		return;
	}
#if 1
	CG_EmitPolyVerts( re );
#else
	trap_R_AddRefEntityToScene( re );
#endif
}


/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA.rgba[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - ( 1.0 - c ) * le->pos.trDelta[2];

	re->radius = le->radius * ( 1.0 - c ) + 16;

	// if the view would be "inside" the sprite, skip the draw without freeing
	// (overdraw guard); it draws again once the view pulls back
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLengthSquared( delta );
	if ( len < le->radius * le->radius ) {
		return;
	}
#if 1
	CG_EmitPolyVerts( re );
#else
	trap_R_AddRefEntityToScene( re );
#endif
}


/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t	*ent;

	ent = &ex->refEntity;

	// add the entity
	if ( intShaderTime )
		trap_R_AddRefEntityToScene2( ent );
	else
		trap_R_AddRefEntityToScene( ent );

	// add the dlight
	if ( ex->light ) {
		float		light;

		light = (float)( cg.time - ex->startTime ) / ( ex->endTime - ex->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2] );
	}
}


/*
================
CG_AddSpriteExplosion
================
*/
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float c;

	re = le->refEntity;

	c = ( le->endTime - cg.time ) / ( float ) ( le->endTime - le->startTime );
	if ( c > 1 ) {
		c = 1.0;	// can happen during connection problems
	}

	re.shaderRGBA.rgba[0] = 0xff;
	re.shaderRGBA.rgba[1] = 0xff;
	re.shaderRGBA.rgba[2] = 0xff;
	re.shaderRGBA.rgba[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * ( 1.0 - c ) + 30;

	if ( intShaderTime )
		trap_R_AddRefEntityToScene2( &re );
	else
		trap_R_AddRefEntityToScene( &re );

	// add the dlight
	if ( le->light ) {
		float		light;

		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2] );
	}
}


#ifdef MISSIONPACK
/*
====================
CG_AddKamikaze
====================
*/
void CG_AddKamikaze( localEntity_t *le ) {
	refEntity_t	*re;
	refEntity_t shockwave;
	float		c;
	vec3_t		test, axis[3];
	int			t;

	re = &le->refEntity;

	t = cg.time - le->startTime;
	VectorClear( test );
	AnglesToAxis( test, axis );

	if (t > KAMI_SHOCKWAVE_STARTTIME && t < KAMI_SHOCKWAVE_ENDTIME) {

		if (!(le->leFlags & LEF_SOUND1)) {
//			trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeExplodeSound );
			trap_S_StartLocalSound(cgs.media.kamikazeExplodeSound, CHAN_AUTO);
			le->leFlags |= LEF_SOUND1;
		}
		// 1st kamikaze shockwave
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.u.shaderTime = re->u.shaderTime;
		VectorCopy(re->origin, shockwave.origin);

		c = (float)(t - KAMI_SHOCKWAVE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME);
		VectorScale( axis[0], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0] );
		VectorScale( axis[1], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1] );
		VectorScale( axis[2], c * KAMI_SHOCKWAVE_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2] );
		shockwave.nonNormalizedAxes = qtrue;

		if (t > KAMI_SHOCKWAVEFADE_STARTTIME) {
			c = (float)(t - KAMI_SHOCKWAVEFADE_STARTTIME) / (float)(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVEFADE_STARTTIME);
		}
		else {
			c = 0;
		}
		c *= 0xff;
		shockwave.shaderRGBA.rgba[0] = 0xff - c;
		shockwave.shaderRGBA.rgba[1] = 0xff - c;
		shockwave.shaderRGBA.rgba[2] = 0xff - c;
		shockwave.shaderRGBA.rgba[3] = 0xff - c;

		trap_R_AddRefEntityToScene( &shockwave );
	}

	if (t > KAMI_EXPLODE_STARTTIME && t < KAMI_IMPLODE_ENDTIME) {
		// explosion and implosion
		c = ( le->endTime - cg.time ) * le->lifeRate;
		c *= 0xff;
		re->shaderRGBA.rgba[0] = le->color[0] * c;
		re->shaderRGBA.rgba[1] = le->color[1] * c;
		re->shaderRGBA.rgba[2] = le->color[2] * c;
		re->shaderRGBA.rgba[3] = le->color[3] * c;

		if( t < KAMI_IMPLODE_STARTTIME ) {
			c = (float)(t - KAMI_EXPLODE_STARTTIME) / (float)(KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME);
		}
		else {
			if (!(le->leFlags & LEF_SOUND2)) {
//				trap_S_StartSound (re->origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.kamikazeImplodeSound );
				trap_S_StartLocalSound(cgs.media.kamikazeImplodeSound, CHAN_AUTO);
				le->leFlags |= LEF_SOUND2;
			}
			c = (float)(KAMI_IMPLODE_ENDTIME - t) / (float) (KAMI_IMPLODE_ENDTIME - KAMI_IMPLODE_STARTTIME);
		}
		VectorScale( axis[0], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[0] );
		VectorScale( axis[1], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[1] );
		VectorScale( axis[2], c * KAMI_BOOMSPHERE_MAXRADIUS / KAMI_BOOMSPHEREMODEL_RADIUS, re->axis[2] );
		re->nonNormalizedAxes = qtrue;

		trap_R_AddRefEntityToScene( re );
		// add the dlight
		trap_R_AddLightToScene( re->origin, c * 1000.0, 1.0, 1.0, c );
	}

	if (t > KAMI_SHOCKWAVE2_STARTTIME && t < KAMI_SHOCKWAVE2_ENDTIME) {
		// 2nd kamikaze shockwave
		if (le->angles.trBase[0] == 0 &&
			le->angles.trBase[1] == 0 &&
			le->angles.trBase[2] == 0) {
			le->angles.trBase[0] = random() * 360;
			le->angles.trBase[1] = random() * 360;
			le->angles.trBase[2] = random() * 360;
		}
		else {
			c = 0;
		}
		memset(&shockwave, 0, sizeof(shockwave));
		shockwave.hModel = cgs.media.kamikazeShockWave;
		shockwave.reType = RT_MODEL;
		shockwave.u.shaderTime = re->u.shaderTime;
		VectorCopy(re->origin, shockwave.origin);

		test[0] = le->angles.trBase[0];
		test[1] = le->angles.trBase[1];
		test[2] = le->angles.trBase[2];
		AnglesToAxis( test, axis );

		c = (float)(t - KAMI_SHOCKWAVE2_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2_STARTTIME);
		VectorScale( axis[0], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[0] );
		VectorScale( axis[1], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[1] );
		VectorScale( axis[2], c * KAMI_SHOCKWAVE2_MAXRADIUS / KAMI_SHOCKWAVEMODEL_RADIUS, shockwave.axis[2] );
		shockwave.nonNormalizedAxes = qtrue;

		if (t > KAMI_SHOCKWAVE2FADE_STARTTIME) {
			c = (float)(t - KAMI_SHOCKWAVE2FADE_STARTTIME) / (float)(KAMI_SHOCKWAVE2_ENDTIME - KAMI_SHOCKWAVE2FADE_STARTTIME);
		}
		else {
			c = 0;
		}
		c *= 0xff;
		shockwave.shaderRGBA.rgba[0] = 0xff - c;
		shockwave.shaderRGBA.rgba[1] = 0xff - c;
		shockwave.shaderRGBA.rgba[2] = 0xff - c;
		shockwave.shaderRGBA.rgba[3] = 0xff - c;

		trap_R_AddRefEntityToScene( &shockwave );
	}
}

/*
===================
CG_AddInvulnerabilityImpact
===================
*/
void CG_AddInvulnerabilityImpact( localEntity_t *le ) {
	trap_R_AddRefEntityToScene( &le->refEntity );
}

/*
===================
CG_AddInvulnerabilityJuiced
===================
*/
void CG_AddInvulnerabilityJuiced( localEntity_t *le ) {
	int t;

	t = cg.time - le->startTime;
	if ( t > 3000 ) {
		le->refEntity.axis[0][0] = (float) 1.0 + 0.3 * (t - 3000) / 2000;
		le->refEntity.axis[1][1] = (float) 1.0 + 0.3 * (t - 3000) / 2000;
		le->refEntity.axis[2][2] = (float) 0.7 + 0.3 * (2000 - (t - 3000)) / 2000;
	}
	if ( t > 5000 ) {
		le->endTime = 0;
		CG_GibPlayer( le->refEntity.origin, vec3_origin );
	}
	else {
		trap_R_AddRefEntityToScene( &le->refEntity );
	}
}
#endif


/*
===================
CG_AddRefEntity
===================
*/
static void CG_AddRefEntity( localEntity_t *le ) {
	if ( le->endTime < cg.time ) {
		CG_FreeLocalEntity( le );
		return;
	}
	trap_R_AddRefEntityToScene( &le->refEntity );
}


/*
===================
CG_AddScorePlum
===================
*/
#define NUMBER_SIZE				8
#define DAMAGE_DIGIT_SPACING	1.7
#define DAMAGE_NARROW_SPACING	1.2  // tighter spacing for '1'

void CG_AddScorePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len;
	int			i, score, digits[10], numdigits, negative;

	re = &le->refEntity;
	re->renderfx |= RF_CROSSHAIR;	// draw undimmed inside fog volumes

	c = ( le->endTime - cg.time ) * le->lifeRate;

	score = le->radius;
	if (score < 0) {
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0x11;
		re->shaderRGBA.rgba[2] = 0x11;
	}
	else {
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0xff;
		re->shaderRGBA.rgba[2] = 0xff;
		if (score >= 50) {
			re->shaderRGBA.rgba[1] = 0;
		} else if (score >= 20) {
			re->shaderRGBA.rgba[0] = re->shaderRGBA.rgba[1] = 0;
		} else if (score >= 10) {
			re->shaderRGBA.rgba[2] = 0;
		} else if (score >= 2) {
			re->shaderRGBA.rgba[0] = re->shaderRGBA.rgba[2] = 0;
		}

	}
	if (c < 0.25f)
		re->shaderRGBA.rgba[3] = 0xff * 4.0f * c;
	else
		re->shaderRGBA.rgba[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	VectorCopy(le->pos.trBase, origin);
	origin[2] += 110.0f - c * 100.0f;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);

	VectorMA(origin, -10.0f + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, skip the draw without freeing
	// (overdraw guard); it draws again once the view pulls back
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLengthSquared( delta );
	if ( len < 20*20 ) {
		return;
	}

	negative = qfalse;
	if (score < 0) {
		negative = qtrue;
		score = -score;
	}

	for (numdigits = 0; !(numdigits && !score); numdigits++) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if (negative) {
		digits[numdigits] = 10;
		numdigits++;
	}

	for (i = 0; i < numdigits; i++) {
		VectorMA(origin, (float) (((float) numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene( re );
	}
}

/*
===================
CG_AddDamagePlum
===================
*/
void CG_AddDamagePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len, distance;
	int			i, damage, digits[10], numdigits, negative;
	float		progress, fade, spread_x, spread_y, vertical_offset, peak_height;

	re = &le->refEntity;
	re->renderfx |= RF_CROSSHAIR;	// draw undimmed inside fog volumes

	c = ( le->endTime - cg.time ) * le->lifeRate;

	damage = le->radius;

	// Color based on damage amount: gradient from blue to red
	if (damage > 75) {
		// Red
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0x00;
		re->shaderRGBA.rgba[2] = 0x00;
	} else if (damage > 50) {
		// Orange
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0x80;
		re->shaderRGBA.rgba[2] = 0x00;
	} else if (damage > 25) {
		// Yellow
		re->shaderRGBA.rgba[0] = 0xff;
		re->shaderRGBA.rgba[1] = 0xff;
		re->shaderRGBA.rgba[2] = 0x00;
	} else {
		// Blue
		re->shaderRGBA.rgba[0] = 0x00;
		re->shaderRGBA.rgba[1] = 0x80;
		re->shaderRGBA.rgba[2] = 0xff;
	}

	// Fade out after 75% of arc (750ms)
	progress = 1.0 - c;  // 0.0 at start, 1.0 at end
	if (progress < 0.75f) {
		fade = 1.0f;  // Full opacity for first 750ms
	} else {
		fade = 1.0f - ((progress - 0.75f) / 0.25f);  // Fade out over remaining 250ms
	}
	re->shaderRGBA.rgba[3] = 0xff * fade;

	VectorCopy(le->pos.trBase, origin);

	// Calculate distance to base origin for scaling sprite and arc
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLengthSquared( delta );
	if ( len < 20*20 ) {
		// view inside the sprite: skip the draw without freeing (overdraw
		// guard); it draws again once the view pulls back
		return;
	}

	distance = sqrt(len);
	re->radius = cg_damagePlumScale.value * (NUMBER_SIZE / 1280.0f) * distance * tan(cg.refdef.fov_x * M_PI / 360.0f);

	// Horizontal spread
	spread_x = le->pos.trDelta[0] * 20.0 * re->radius * progress;
	spread_y = le->pos.trDelta[1] * 20.0 * re->radius * progress;
	origin[0] += spread_x;
	origin[1] += spread_y;

	// Vertical arc: symmetric rise and fall over the full duration
	// Uses sine wave for smooth, even arc that peaks at 50% progress
	peak_height = 15.0 * le->pos.trDelta[2] * re->radius;
	vertical_offset = peak_height * sin(progress * M_PI);
	origin[2] += vertical_offset;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	vec[2] = 0;  // Keep digits on same horizontal plane
	VectorNormalize(vec);

	negative = qfalse;
	if (damage < 0) {
		negative = qtrue;
		damage = -damage;
	}

	for (numdigits = 0; !(numdigits && !damage); numdigits++) {
		digits[numdigits] = damage % 10;
		damage = damage / 10;
	}

	if (negative) {
		digits[numdigits] = 10;
		numdigits++;
	}

	{
		float total_width = 0;
		float pos;
		int digit, next_digit;

		// First pass: calculate total width
		for (i = 0; i < numdigits; i++) {
			digit = digits[numdigits - 1 - i];
			next_digit = (i + 1 < numdigits) ? digits[numdigits - 2 - i] : -1;
			if (digit == 1 || next_digit == 1) {
				total_width += re->radius * DAMAGE_NARROW_SPACING;
			} else {
				total_width += re->radius * DAMAGE_DIGIT_SPACING;
			}
		}

		// Second pass: render digits from left to right, centered
		pos = total_width / 2;
		for (i = 0; i < numdigits; i++) {
			digit = digits[numdigits - 1 - i];
			next_digit = (i + 1 < numdigits) ? digits[numdigits - 2 - i] : -1;

			VectorMA(origin, pos, vec, re->origin);
			re->customShader = cgs.media.damagePlumShaders[digit];
			trap_R_AddRefEntityToScene(re);

			if (digit == 1 || next_digit == 1) {
				pos -= re->radius * DAMAGE_NARROW_SPACING;
			} else {
				pos -= re->radius * DAMAGE_DIGIT_SPACING;
			}
		}
	}
}


/*
===================
CG_AddBloodParticle

Blood gout/droplet: moves under gravity (mist also drags), traces for collision,
leaves a mark on impact, then fades out on the surface.
===================
*/
#define	BLOOD_DRAG	6.0f	// mist air resistance (1/s); terminal fall ~= gravity/BLOOD_DRAG

static void CG_AddBloodParticle( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		newOrigin;
	trace_t		trace;
	float		c;

	re = &le->refEntity;

	// Staggered births: a scheduled gout doesn't integrate or draw until born.
	if ( cg.time < le->startTime ) {
		return;
	}

	// Calculate fade
	c = ( le->endTime - cg.time ) * le->lifeRate;
	if ( c < 0 ) c = 0;
	re->shaderRGBA.rgba[3] = 0xff * c * le->color[3];

	// Calculate new position. Mist (LEF_NO_MARK) integrates gravity + air drag so
	// the fine spray decelerates and hangs like aerosol rather than arcing like a
	// solid; heavier droplets keep the closed-form ballistic path.
	if ( le->leFlags & LEF_NO_MARK ) {
		float dt = ( cg.time - le->pos.trTime ) * 0.001f;
		if ( dt > 0.0f ) {
			float damp = 1.0f - BLOOD_DRAG * dt;
			if ( damp < 0.0f ) damp = 0.0f;
			le->pos.trDelta[2] -= DEFAULT_GRAVITY * dt;
			VectorScale( le->pos.trDelta, damp, le->pos.trDelta );
			VectorMA( le->pos.trBase, dt, le->pos.trDelta, le->pos.trBase );
			le->pos.trTime = cg.time;
		}
		VectorCopy( le->pos.trBase, newOrigin );
	} else {
		BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );
	}

	// Particle entered water: spawn sinking blood cloud
	if ( CG_PointContents( newOrigin, -1 ) & MASK_WATER ) {
		localEntity_t *cloud;

		cloud = CG_SmokePuff( newOrigin, vec3_origin,
			1 + random() * 2, 1, 1, 1, 0.4f,
			300 + random() * 200, cg.time, 0, 0,
			cgs.media.bloodTrailShader );
		cloud->leType = LE_FALL_SCALE_FADE;
		cloud->pos.trDelta[2] = 5 + random() * 10;
		CG_FreeLocalEntity( le );
		return;
	}

	// Trace for collision
	CG_Trace( &trace, re->origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );

	if ( trace.fraction < 1.0f ) {
		// Hit a surface. Gouts/spray (LEF_NO_MARK) splat via their own dedicated
		// decals, so they just disappear here; small droplets leave a mark.
		if ( !( le->leFlags & LEF_NO_MARK ) ) {
			CG_ImpactMark( cgs.media.bloodSplatShader[ rand() & 3 ], trace.endpos, trace.plane.normal,
				random() * 360, 1, 1, 1, 1, qtrue, le->radius, qfalse );
		}
		CG_FreeLocalEntity( le );
		return;
	} else {
		// Still in flight
		VectorCopy( newOrigin, re->origin );
	}

	// Near-eye cull: skip drawing a gout the camera is on top of (see
	// CG_BloodGoutCulled). Non-freeing: it draws again once it pulls back.
	if ( CG_BloodGoutCulled( re ) ) {
		return;
	}

	CG_UpdateGoutSprite( le );

	trap_R_AddRefEntityToScene( re );
}


//==============================================================================

/*
====================
CG_GrapplePadFall

The anchor let go. Kick the pad off the surface it was gripping, scatter it in
the contact plane, and let it tumble down.

Three rolls with three different shapes, because they fail differently: axial
spin needs a floor (a pad that barely turns reads as still magnetically held),
while tumble and scatter need only a ceiling: zero tumble is correct on a floor,
and the kick already carries the pad clear without any scatter at all.
====================
*/
void CG_GrapplePadFall( const vec3_t origin, const vec3_t angles, int ownerClientNum, int stamp, int num ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			normal, right, up;
	float			a, lever, spin, pullLevel;
	byte			rgba[4];

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_GRAPPLE_PAD;
	le->startTime = cg.time;
	le->endTime = cg.time + PAD_FALL_TIME;
	le->lifeRate = 1.0f / ( le->endTime - le->startTime );
	le->bounceFactor = PAD_FALL_BOUNCE;

	// the server froze the impact normal into angles; +X runs into what it hit
	AngleVectors( angles, normal, NULL, NULL );

	VectorNegate( normal, normal );

	// physics starts at the true impact point, NOT embedded like the anchored
	// draw: an embedded trBase starts every CG_Trace in CG_AddGrapplePad
	// solid, and the pad never leaves the wall. The embedded look on the
	// handoff frame is drawn-only, applied in CG_AddGrapplePad
	VectorCopy( origin, le->pos.trBase );
	le->pos.trType = TR_GRAVITY;	// BG_EvaluateTrajectory hardcodes DEFAULT_GRAVITY, not a per-trajectory value
	le->pos.trTime = cg.time;
	VectorScale( normal, PAD_FALL_KICK, le->pos.trDelta );

	// scatter belongs in the CONTACT PLANE. Along a world axis it would scatter
	// on a floor but merely stiffen the kick on a wall
	MakeNormalVectors( normal, right, up );
	a = random() * M_PI * 2;
	VectorMA( le->pos.trDelta, PAD_FALL_SCATTER * random() * cos( a ), right,
		le->pos.trDelta );
	VectorMA( le->pos.trDelta, PAD_FALL_SCATTER * random() * sin( a ), up,
		le->pos.trDelta );

	// sideways slide: the claws release across a few frames, and the last one
	// gripping drags the pad along the surface. World horizontal, projected
	// into the contact plane so it never pushes into the wall
	{
		vec3_t	drift;
		float	d;

		a = random() * M_PI * 2;
		VectorSet( drift, cos( a ), sin( a ), 0 );
		d = DotProduct( drift, normal );
		VectorMA( drift, -d, normal, drift );
		// a pick near the wall normal projects to ~nothing: that release
		// just slid less
		if ( VectorNormalize( drift ) > 0.1f ) {
			VectorMA( le->pos.trDelta, PAD_FALL_DRIFT_MIN + random()
				* ( PAD_FALL_DRIFT_MAX - PAD_FALL_DRIFT_MIN ), drift,
				le->pos.trDelta );
		}
	}

	// tumble comes from gravity's lever arm on the offset mass, so it vanishes
	// when the normal is vertical: face down on a floor or hung off a ceiling,
	// gravity runs through the grip and there is no torque to have
	lever = sqrt( normal[0] * normal[0] + normal[1] * normal[1] );

	// the pad was seated against a surface the player watched it grip, so unlike
	// CG_LaunchGib the base angles are the clamped pose, not a random one
	{
		vec3_t	axis[3], rolled;

		// start from the pose the anchored draw showed, roll included
		CG_GrapplePadAnchorAxis( angles, stamp, num, axis );
		CG_AxisToAngles( axis, rolled );
		VectorCopy( rolled, le->angles.trBase );
	}
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trDelta[PITCH] = crandom() * PAD_FALL_TUMBLE * lever;
	le->angles.trDelta[YAW] = 0;
	spin = PAD_FALL_SPIN_MIN
		+ random() * ( PAD_FALL_SPIN_MAX - PAD_FALL_SPIN_MIN );
	if ( rand() & 1 ) {
		spin = -spin;
	}
	le->angles.trDelta[ROLL] = spin;

	memset( re, 0, sizeof( *re ) );
	re->hModel = cg_weapons[ WP_GRAPPLING_HOOK ].missileModel;
	re->renderfx = RF_NOSHADOW;
	VectorCopy( le->pos.trBase, re->origin );
	VectorCopy( le->pos.trBase, re->oldorigin );

	// the pad carries an rgbGen entity stage, so a local entity that never sets
	// shaderRGBA renders it black
	CG_GrappleOwnerRGBA( ownerClientNum, rgba );

	// start from the same brightness the anchored draw ended at
	// (CG_GrappleFade with GLOW_PULL_LEVEL), or the handoff frame pops
	// ~33% brighter
	pullLevel = CG_GrapplePullLevel();
	le->color[0] = rgba[0] * ( 1.0f / 255.0f ) * pullLevel;
	le->color[1] = rgba[1] * ( 1.0f / 255.0f ) * pullLevel;
	le->color[2] = rgba[2] * ( 1.0f / 255.0f ) * pullLevel;
	le->color[3] = 1.0f;
}

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t	*le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FRAGMENT:			// gibs and brass
			CG_AddFragment( le );
			break;

		case LE_GRAPPLE_PAD:
			CG_AddGrapplePad( le );
			break;

		case LE_MOVE_SCALE_FADE:	// water bubbles, plasma trails, smoke puff
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:			// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE:	// gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:			// rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum( le );
			break;

		case LE_DAMAGEPLUM:
			CG_AddDamagePlum( le );
			break;

		case LE_BLOOD_PARTICLE:
			CG_AddBloodParticle( le );
			break;

#ifdef MISSIONPACK
		case LE_KAMIKAZE:
			CG_AddKamikaze( le );
			break;
		case LE_INVULIMPACT:
			CG_AddInvulnerabilityImpact( le );
			break;
		case LE_INVULJUICED:
			CG_AddInvulnerabilityJuiced( le );
			break;
#endif
		case LE_SHOWREFENTITY:
			CG_AddRefEntity( le );
			break;
		}
	}
}




