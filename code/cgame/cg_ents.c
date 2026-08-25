// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"


/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, const char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, const char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( const centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( const centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if(cent->currentState.constantLight)
	{
		int		cl;
		float		i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float)(( cl >> 0 ) & 255) / 255.0;
		g = (float)(( cl >> 8 ) & 255) / 255.0;
		b = (float)(( cl >> 16 ) & 255) / 255.0;
		i = (float)(( cl >> 24 ) & 255) * 4.0;
		trap_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
	}

}


/*
==================
CG_General
==================
*/
static void CG_General( const centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t	*s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t		ent;
	entityState_t	*es;
	const gitem_t	*item;
	int				msec;
	float			frac;
	float			scale;
	weaponInfo_t	*wi;
	int				modulus;
	itemInfo_t		*itemInfo;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) {
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) || cent->delaySpawn > cg.time ) {
		return;
	}

	itemInfo = &cg_items[ es->modelindex ];
	if ( !itemInfo->registered ) {
		CG_RegisterItemVisuals( es->modelindex );
		if ( !itemInfo->registered ) {
			return;
		}
	}

	item = &bg_itemlist[ es->modelindex ];
	if ( cg_simpleItems.integer && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon_df;
		ent.shaderRGBA.rgba[0] = 255;
		ent.shaderRGBA.rgba[1] = 255;
		ent.shaderRGBA.rgba[2] = 255;
		ent.shaderRGBA.rgba[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// items bob up and down continuously
	scale = 0.005 + cent->currentState.number * 0.00001;
	modulus = 2 * M_PI * 20228 / scale;
	cent->lerpOrigin[2] += 4 + cos( ( ( cg.time + 1000 ) % modulus ) *  scale ) * 4;

	memset (&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if ( item->giType == IT_HEALTH ) {
		VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		VectorCopy( cg.autoAngles, cent->lerpAngles );
		AxisCopy( cg.autoAxis, ent.axis );
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -= 
			wi->weaponMidpoint[0] * ent.axis[0][0] +
			wi->weaponMidpoint[1] * ent.axis[1][0] +
			wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -= 
			wi->weaponMidpoint[0] * ent.axis[0][1] +
			wi->weaponMidpoint[1] * ent.axis[1][1] +
			wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -= 
			wi->weaponMidpoint[0] * ent.axis[0][2] +
			wi->weaponMidpoint[1] * ent.axis[1][2] +
			wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;	// an extra height boost
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if ( msec >= 0 && msec < ITEM_SCALEUP_TIME ) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale( ent.axis[0], frac, ent.axis[0] );
		VectorScale( ent.axis[1], frac, ent.axis[1] );
		VectorScale( ent.axis[2], frac, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ( ( item->giType == IT_WEAPON ) ||
		 ( item->giType == IT_ARMOR ) ) {
		ent.renderfx |= RF_MINLIGHT;
	}

	// increase the size of the weapons when they are presented as items
	if ( item->giType == IT_WEAPON ) {
		VectorScale( ent.axis[0], 1.5, ent.axis[0] );
		VectorScale( ent.axis[1], 1.5, ent.axis[1] );
		VectorScale( ent.axis[2], 1.5, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
#ifdef MISSIONPACK
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.weaponHoverSound );
#endif
		// pickup color from spectaror/own client; the grapple joins the railgun
		// because its emission stage is rgbGen entity too
		if ( item->giTag == WP_RAILGUN || item->giTag == WP_GRAPPLING_HOOK ) {
			const clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
			ent.shaderRGBA.rgba[0] = ci->color1[0] * 255.0f;
			ent.shaderRGBA.rgba[1] = ci->color1[1] * 255.0f;
			ent.shaderRGBA.rgba[2] = ci->color1[2] * 255.0f;
			ent.shaderRGBA.rgba[3] = 255;
		}
	}

#ifdef MISSIONPACK
	if ( item->giType == IT_HOLDABLE && item->giTag == HI_KAMIKAZE ) {
		VectorScale( ent.axis[0], 2, ent.axis[0] );
		VectorScale( ent.axis[1], 2, ent.axis[1] );
		VectorScale( ent.axis[2], 2, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}
#endif

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if ( item->giType == IT_WEAPON && wi && wi->barrelModel ) {
		refEntity_t	barrel;
		vec3_t		angles;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( ent.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = 0;
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene( &barrel );
	}

	// accompanying rings / spheres for powerups
	if ( !cg_simpleItems.integer ) 
	{
		vec3_t spinAngles;

		VectorClear( spinAngles );

		if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
		{
			if ( ( ent.hModel = cg_items[es->modelindex].models[1] ) != 0 )
			{
				if ( item->giType == IT_POWERUP )
				{
					ent.origin[2] += 12;
					spinAngles[1] = ( cg.time & 1023 ) * 360 / -1024.0f;
				}
				AnglesToAxis( spinAngles, ent.axis );
				
				// scale up if respawning
				if ( frac != 1.0 ) {
					VectorScale( ent.axis[0], frac, ent.axis[0] );
					VectorScale( ent.axis[1], frac, ent.axis[1] );
					VectorScale( ent.axis[2], frac, ent.axis[2] );
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene( &ent );
			}
		}
	}
}

//============================================================================

/*
===============
CG_GrapplePadAnchorAxis

The frozen surface normal already points the nose; the roll phase comes off
an ABSOLUTE clock plus a per-hook offset, not elapsed flight time: G_SetOrigin
zeroes pos.trTime at impact, so an anchored pad no longer knows when it was
fired, but it does know when it landed, and both sides resolve the same angle
from that stamp.
===============
*/
void CG_GrapplePadAnchorAxis( const vec3_t angles, int stamp, int num,
		vec3_t axis[3] ) {
	AnglesToAxis( angles, axis );
	// 0 means a game QVM older than the s.time hoist; leave it unrolled
	// rather than resolve a phase from a stamp that was never written
	if ( stamp > 0 ) {
		// same modulus as the flight branch, so the phase does not
		// shift at the instant this hook anchored
		RotateAroundDirection( axis, PAD_FLY_SPIN
			* ( stamp % PAD_FLY_SPIN_MOD ) * 0.001f + num * 137 );
	}
}

/*
===============
CG_GrapplePadMark

The wall-bite scar. The claw layout is not 3-fold symmetric (facets 1/4/6),
so instead of the usual random spin the orientation is solved to land the
mark's texture axes on the pad's rolled axes: the painted gouges sit under
the real claws.
===============
*/
void CG_GrapplePadMark( const vec3_t origin, vec3_t axis[3],
		qboolean temporary ) {
	vec3_t	outward, base, tang;
	float	orient;

	// CG_ImpactMark exempts temporary marks from cg_marks for shadows'
	// sake; this one is a real mark, so gate it here
	if ( !cg_addMarks.integer ) {
		return;
	}
	VectorScale( axis[0], -1, outward );
	PerpendicularVector( base, outward );
	CrossProduct( outward, base, tang );
	orient = atan2( DotProduct( tang, axis[2] ),
		DotProduct( base, axis[2] ) ) * ( 180.0f / M_PI );
	CG_ImpactMark( cgs.media.grappleMarkShader, origin, outward, orient,
		1, 1, 1, 1, qtrue, GRAPPLE_PAD_MARK_RADIUS, temporary );
}

/*
===============
CG_GrappleHookAxis

Rolled about its own line of travel so it reads as gyroscopically held.
===============
*/
void CG_GrappleHookAxis( const centity_t *cent, vec3_t axis[3] ) {
	vec3_t	dir, angles;

	if ( cent->currentState.eType == ET_GRAPPLE ) {
		CG_GrapplePadAnchorAxis( cent->lerpAngles, cent->currentState.time,
			cent->currentState.number, axis );
		return;
	}

	if ( VectorNormalize2( cent->currentState.pos.trDelta, dir ) == 0 ) {
		VectorSet( dir, 0, 0, 1 );
	}
	vectoangles( dir, angles );
	AnglesToAxis( angles, axis );
	// RotateAroundDirection rotates positively-clockwise as viewed looking
	// along axis[0]; axis[0] here is the travel direction receding from the
	// shooter, so a positive angle is clockwise as the shooter sees it.
	// Bound cg.time by the same modulus the anchored branch uses below
	RotateAroundDirection( axis, PAD_FLY_SPIN
		* ( cg.time % PAD_FLY_SPIN_MOD ) * 0.001f
		+ cent->currentState.number * 137 );
}

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t	*weapon;
	const clientInfo_t	*ci;
//	int	col;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS ) {
		s1->weapon = WP_NONE;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

	// add trails
	if ( weapon->missileTrailFunc ) 
	{
		weapon->missileTrailFunc( cent, weapon );
	}
/*
	if ( cent->currentState.modelindex == TEAM_RED ) {
		col = 1;
	}
	else if ( cent->currentState.modelindex == TEAM_BLUE ) {
		col = 2;
	}
	else {
		col = 0;
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	}
*/
	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
	}

	// add missile sound
	if ( weapon->missileSound ) {
		vec3_t	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	if ( cent->currentState.weapon == WP_PLASMAGUN ) {
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		trap_R_AddRefEntityToScene( &ent );
		return;
	}

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

#ifdef MISSIONPACK
	if ( cent->currentState.weapon == WP_PROX_LAUNCHER ) {
		if (s1->generic1 == TEAM_BLUE) {
			ent.hModel = cgs.media.blueProxMine;
		}
	}
#endif

	if ( s1->weapon == WP_GRAPPLING_HOOK ) {
		float	seat;

		CG_GrappleHookAxis( cent, ent.axis );
		// in flight the pad lights with the tether streaming off it,
		// rings running outward at flight pace
		CG_GrappleOwnerRGBA( s1->otherEntityNum, ent.shaderRGBA.rgba );
		CG_GrappleSeatFired( s1->otherEntityNum );

		// calling CG_GrappleSeat is what advances the ramp while the pad is
		// in flight; a pad fired mid-materialize leaves the dock part-formed
		// and must finish growing here, not launch at full size
		seat = CG_GrappleSeat( s1->otherEntityNum );
		if ( seat < 1.0f ) {
			float	e, s, a, roll, d, r;
			vec3_t	t1, t2;

			// same boss-pivoted transform as the stowed-pad draw in
			// cg_weapons.c: seat^1.5 via sqrt since pow() is not in the
			// QVM libc, pivoted on the pad's rear boss so it grows
			// forward instead of swelling from its center
			e = seat * sqrt( seat );
			e = e * e * ( 3.0f - 2.0f * e );
			s = PAD_SEAT_SCALE0 + ( 1.0f - PAD_SEAT_SCALE0 ) * e;

			// unwind PAD_SEAT_SPIN to 0 about ent.axis[0], composing with the
			// PAD_FLY_SPIN roll already on ent.axis rather than replacing it.
			// Must precede the extrude below: RotatePointAroundVector needs a
			// unit axis. Cubic ease-out on raw seat rather than e, so the roll
			// spins briskly off the mark and decelerates into rest instead of
			// following e's symmetric grow
			d = 1.0f - seat;
			r = 1.0f - d * d * d;
			roll = PAD_SEAT_SPIN * ( 1.0f - r );
			VectorCopy( ent.axis[1], t1 );
			VectorCopy( ent.axis[2], t2 );
			RotatePointAroundVector( ent.axis[1], ent.axis[0], t1, roll );
			RotatePointAroundVector( ent.axis[2], ent.axis[0], t2, roll );

			VectorMA( ent.origin, PAD_BOSS_X0 * ( 1.0f - s ), ent.axis[0],
				ent.origin );
			VectorScale( ent.axis[0], s, ent.axis[0] );

			// same alpha ramp as the stowed draw: scale AND alpha together
			// is the materialize, not just growth
			a = seat / PAD_SEAT_ALPHA;
			if ( a > 1.0f ) {
				a = 1.0f;
			}
			ent.shaderRGBA.rgba[0] = (byte)( ent.shaderRGBA.rgba[0] * a );
			ent.shaderRGBA.rgba[1] = (byte)( ent.shaderRGBA.rgba[1] * a );
			ent.shaderRGBA.rgba[2] = (byte)( ent.shaderRGBA.rgba[2] * a );
			ent.shaderRGBA.rgba[3] = a * 0xff;
			ent.customShader = cgs.media.grapplePadFadeShader;
		} else {
			ent.customShader = cgs.media.grapplePadFlyShader;
		}
	} else {
		// convert direction of travel into axis
		if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
			ent.axis[0][2] = 1;
		}

		// spin as it moves
		if ( s1->pos.trType != TR_STATIONARY ) {
			RotateAroundDirection( ent.axis, ( cg.time % TMOD_004 ) / 4.0 );
		} else {
#ifdef MISSIONPACK
			if ( s1->weapon == WP_PROX_LAUNCHER ) {
				AnglesToAxis( cent->lerpAngles, ent.axis );
			}
			else
#endif
			{
				RotateAroundDirection( ent.axis, s1->time );
			}
		}
	}

	// add to refresh list, possibly with quad glow

	s1->powerups &= ~( (1 << PW_INVIS) | (1 << PW_REGEN) );
	ci = &cgs.clientinfo[ s1->clientNum & MAX_CLIENTS ];
	if ( ci->infoValid ) {
		CG_AddRefEntityWithPowerups( &ent, s1, ci->team );
	} else {
		CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE );
	}

}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
#define GRAPPLE_ANCHOR_LIGHT_RADIUS	80

// off-surface standoff for the anchor dlight: the pad's origin is embedded
// in the solid, and a light inside the wall throws nothing onto it
#define GRAPPLE_ANCHOR_LIGHT_LIFT	2.0f

/*
===============
CG_MoverWorldToLocal

No BG equivalent; only this file's mover-axis composition needs world to local.
===============
*/
static void CG_MoverWorldToLocal( vec3_t matrix[3], const vec3_t in, vec3_t out ) {
	out[0] = DotProduct( matrix[0], in );
	out[1] = DotProduct( matrix[1], in );
	out[2] = DotProduct( matrix[2], in );
}

/*
===============
CG_AxisToAngles

Inverse of AnglesToAxis, roll included: yaw/pitch from the forward row,
roll as the signed rotation of the up row away from the roll-zero frame.
===============
*/
static void CG_AxisToAngles( vec3_t axis[3], vec3_t angles ) {
	vec3_t	axis0[3];
	float	sn, cs;

	vectoangles( axis[0], angles );
	AnglesToAxis( angles, axis0 );
	cs = DotProduct( axis[2], axis0[2] );
	// AngleVectors' roll rotates up toward -left (right) for positive roll,
	// so up.left0 = -sin(roll); negate to recover atan2(sin, cos) = roll
	sn = -DotProduct( axis[2], axis0[1] );
	angles[ROLL] = RAD2DEG( atan2( sn, cs ) );
}

static void CG_Grapple( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;
	const clientInfo_t	*ci;
	vec3_t				lightOrigin;
	float				pulse;
	float				dscale;
	float				dradius;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS ) {
		s1->weapon = WP_NONE;
	}
	weapon = &cg_weapons[s1->weapon];

	// re-derive from the mover's rendered trajectory so the pad paths with it
	if ( s1->otherEntityNum2 >= MAX_CLIENTS ) {
		const centity_t *mover = &cg_entities[ s1->otherEntityNum2 ];

		if ( mover->currentValid && mover->currentState.eType == ET_MOVER ) {
			vec3_t	morg, mang, a0, f, loc;
			vec3_t	m[3], m0[3], axis0[3], axis[3];
			int		i;

			BG_EvaluateTrajectory( &mover->currentState.pos, cg.time, morg );
			BG_EvaluateTrajectory( &mover->currentState.apos, cg.time, mang );
			BG_CreateRotationMatrix( mang, m );
			BG_MoverLocalToWorld( m, s1->origin2, cent->lerpOrigin );
			VectorAdd( morg, cent->lerpOrigin, cent->lerpOrigin );

			// claws ride the mover frame; a lone direction cannot carry spin
			BG_EvaluateTrajectory( &mover->currentState.apos, s1->time, a0 );
			BG_CreateRotationMatrix( a0, m0 );
			BG_MoverLocalToWorld( m0, s1->angles2, f );
			vectoangles( f, cent->lerpAngles );
			AnglesToAxis( cent->lerpAngles, axis0 );
			for ( i = 0 ; i < 3 ; i++ ) {
				CG_MoverWorldToLocal( m0, axis0[i], loc );
				BG_MoverLocalToWorld( m, loc, axis[i] );
			}
			CG_AxisToAngles( axis, cent->lerpAngles );
		}
	}

	// the far end of the launcher's cell breathes on its envelope, always the
	// pull loop here
	pulse = CG_GrapplePulse( s1->otherEntityNum );

	// anchored means seated: the ET_GRAPPLE path never scales the pad, so a
	// point-blank hit that beats the ramp snaps to full size on the same frame
	// the claws splay, which hides it
	CG_GrappleSeatSnap( s1->otherEntityNum );

	// the anchored clank loop follows the player, not the anchor point
	if ( cgs.media.sfx_grapplepull ) {
		trap_S_AddLoopingSound( s1->otherEntityNum, cg_entities[ s1->otherEntityNum ].lerpOrigin,
			vec3_origin, cgs.media.sfx_grapplepull );
	}

	// the far end, on the hook's own entity number: loops are one per entnum
	// and last writer wins, so both ends of the tether are audible at once.
	// PVS culls an anchored hook, so around a corner this layer drops and the
	// owner's loop above is what keeps that from being silence.
	if ( cgs.media.sfx_grappletether ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
			vec3_origin, cgs.media.sfx_grappletether );
	}

	// Will draw cable if needed
	CG_GrappleTrail ( cent, weapon );

	// create the render entity
	memset (&ent, 0, sizeof(ent));

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// clamped onto the surface rather than folded for flight
	ent.frame = ent.oldframe = PAD_FRAME_CLAMPED;

	// the server froze the surface normal into angles at impact: +X runs into what it hit
	CG_GrappleHookAxis( cent, ent.axis );

	// temporary scar, re-projected per frame so it never ages under a held
	// pad; the release event stamps the fading copy. No scar on a mover:
	// mark polys are static world geometry and would be left hanging
	if ( s1->otherEntityNum2 < MAX_CLIENTS ) {
		CG_GrapplePadMark( cent->lerpOrigin, ent.axis, qtrue );
	}

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorMA( ent.origin, GRAPPLE_PAD_EMBED, ent.axis[0], ent.origin );
	VectorCopy( ent.origin, ent.oldorigin );

	ci = NULL;
	if ( s1->otherEntityNum < MAX_CLIENTS
			&& cgs.clientinfo[ s1->otherEntityNum ].infoValid ) {
		ci = &cgs.clientinfo[ s1->otherEntityNum ];
	}

	// the pad's emission stage wears the owner's color like the rail core; anchored, rings run inward
	CG_GrappleOwnerRGBA( s1->otherEntityNum, ent.shaderRGBA.rgba );
	CG_GrappleFade( ent.shaderRGBA.rgba, pulse );
	ent.customShader = cgs.media.grapplePadPullShader;

	// quad shell like any missile, but an invisible owner's anchor stays plainly visible
	s1->powerups &= ~( (1 << PW_INVIS) | (1 << PW_REGEN) );
	CG_AddRefEntityWithPowerups( &ent, s1, ci ? ci->team : TEAM_FREE );

	// carries the "still attached" read at a distance; dims with the muzzle's slow breath, never the envelope
	dscale = CG_GrappleDlightScale();
	dradius = CG_GrappleDlightRadius( GRAPPLE_ANCHOR_LIGHT_RADIUS );
	VectorMA( cent->lerpOrigin, -GRAPPLE_ANCHOR_LIGHT_LIFT, ent.axis[0],
		lightOrigin );
	if ( ci ) {
		trap_R_AddLightToScene( lightOrigin, dradius,
			ci->color1[0] * dscale, ci->color1[1] * dscale,
			ci->color1[2] * dscale );
	} else {
		trap_R_AddLightToScene( lightOrigin, dradius,
			0.20f * dscale, 0.50f * dscale, 1.00f * dscale );
	}
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( const centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t	*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[ s1->modelindex2 % MAX_MODELS ];
		trap_R_AddRefEntityToScene(&ent);
	}

}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( const centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t	*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;
	ent.customShader = cgs.media.whiteShader;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( const centity_t *cent ) {
	refEntity_t			ent;
	const entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
================
CG_CreateRotationMatrix
================
*/
void CG_CreateRotationMatrix(vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
CG_TransposeMatrix
================
*/
void CG_TransposeMatrix(vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
CG_RotatePoint
================
*/
void CG_RotatePoint(vec3_t point, vec3_t matrix[3]) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, const vec3_t angles_in, vec3_t angles_out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;
	vec3_t	matrix[3], transpose[3];
	vec3_t	org, org2, move2;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		VectorCopy( angles_in, angles_out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		VectorCopy( angles_in, angles_out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	// origin change when on a rotating object
	CG_CreateRotationMatrix( deltaAngles, transpose );
	CG_TransposeMatrix( transpose, matrix );
	VectorSubtract( in, oldOrigin, org );
	VectorCopy( org, org2 );
	CG_RotatePoint( org2, matrix );
	VectorSubtract( org2, org, move2 );
	VectorAdd( deltaOrigin, move2, deltaOrigin );

	VectorAdd( in, deltaOrigin, out );
	VectorAdd( angles_in, deltaAngles, angles_out );
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {

	// if this player does not want to see extrapolated players
	if ( !cg_smoothClients.integer ) {
		// make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS ) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, 
		cg.snap->serverTime, cg.time, cent->lerpOrigin, cent->lerpAngles, cent->lerpAngles );
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase( centity_t *cent ) {
	refEntity_t model;
#ifdef MISSIONPACK
	vec3_t angles;
	int t, h;
	float c;

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
#else
	if ( cgs.gametype == GT_CTF) {
#endif
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );
		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		trap_R_AddRefEntityToScene( &model );
	}
#ifdef MISSIONPACK
	else if ( cgs.gametype == GT_OBELISK ) {
		// show the obelisk
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		model.hModel = cgs.media.overloadBaseModel;
		trap_R_AddRefEntityToScene( &model );
		// if hit
		if ( cent->currentState.frame == 1) {
			// show hit model
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA.rgba[0] = 0xff;
			model.shaderRGBA.rgba[1] = c;
			model.shaderRGBA.rgba[2] = c;
			model.shaderRGBA.rgba[3] = 0xff;
			//
			model.hModel = cgs.media.overloadEnergyModel;
			trap_R_AddRefEntityToScene( &model );
		}
		// if respawning
		if ( cent->currentState.frame == 2) {
			if ( !cent->miscTime ) {
				cent->miscTime = cg.time;
			}
			t = cg.time - cent->miscTime;
			h = (cg_obeliskRespawnDelay.integer - 5) * 1000;
			//
			if (t > h) {
				c = (float) (t - h) / h;
				if (c > 1)
					c = 1;
			}
			else {
				c = 0;
			}
			// show the lights
			AnglesToAxis( cent->currentState.angles, model.axis );
			//
			model.shaderRGBA.rgba[0] = c * 0xff;
			model.shaderRGBA.rgba[1] = c * 0xff;
			model.shaderRGBA.rgba[2] = c * 0xff;
			model.shaderRGBA.rgba[3] = c * 0xff;

			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene( &model );
			// show the target
			if (t > h) {
				if ( !cent->muzzleFlashTime ) {
					trap_S_StartSound (cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY,  cgs.media.obeliskRespawnSound);
					cent->muzzleFlashTime = 1;
				}
				VectorCopy(cent->currentState.angles, angles);
				angles[YAW] += (float) 16 * acos(1-c) * 180 / M_PI;
				AnglesToAxis( angles, model.axis );

				VectorScale( model.axis[0], c, model.axis[0]);
				VectorScale( model.axis[1], c, model.axis[1]);
				VectorScale( model.axis[2], c, model.axis[2]);

				model.shaderRGBA.rgba[0] = 0xff;
				model.shaderRGBA.rgba[1] = 0xff;
				model.shaderRGBA.rgba[2] = 0xff;
				model.shaderRGBA.rgba[3] = 0xff;
				//
				model.origin[2] += 56;
				model.hModel = cgs.media.overloadTargetModel;
				trap_R_AddRefEntityToScene( &model );
			}
			else {
				//FIXME: show animated smoke
			}
		}
		else {
			cent->miscTime = 0;
			cent->muzzleFlashTime = 0;
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA.rgba[0] = 0xff;
			model.shaderRGBA.rgba[1] = c;
			model.shaderRGBA.rgba[2] = c;
			model.shaderRGBA.rgba[3] = 0xff;
			// show the lights
			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene( &model );
			// show the target
			model.origin[2] += 56;
			model.hModel = cgs.media.overloadTargetModel;
			trap_R_AddRefEntityToScene( &model );
		}
	}
	else if ( cgs.gametype == GT_HARVESTER ) {
		// show harvester model
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterRedSkin;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterBlueSkin;
		}
		else {
			model.hModel = cgs.media.harvesterNeutralModel;
			model.customSkin = 0;
		}
		trap_R_AddRefEntityToScene( &model );
	}
#endif
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_GRAPPLE:
		CG_Grapple( cent );
		break;
	case ET_TEAM:
		CG_TeamBase( cent );
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );

	// Set VR head orientation for local player (mirrors); see vr_cgame.c
	CG_VR_PredictedPlayerHead();

	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	CG_AddViewWeapon( &cg.predictedPlayerState );

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}
}

