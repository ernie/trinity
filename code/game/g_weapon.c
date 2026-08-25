// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"
#include "bg_grapple_model.h"
#include "bg_mode.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;
static	vec3_t	muzzle_origin; // for hitscan weapon trace

/*
===============
CalcMuzzlePointOrigin
===============
*/
void CalcMuzzlePointOrigin( gentity_t *ent, vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t muzzlePoint ) {
	if ( !G_VR_MuzzlePoint( ent, forward, right, up, origin, muzzlePoint ) )
	{
		VectorCopy( ent->client->ps.origin, origin );
		origin[2] += ent->client->ps.viewheight;
		VectorMA( origin, 14.0, forward, muzzlePoint );
		// snap to integer coordinates for more efficient network bandwidth usage
		//SnapVector( muzzlePoint );
	}
}


/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


/*
======================================================================

GAUNTLET

======================================================================
*/

void Weapon_Gauntlet( gentity_t *ent ) {

}

/*
===============
CheckGauntletAttack
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	vec3_t		angles;
	qboolean	vrAim;

	// set aiming directions
	vrAim = G_VR_AimAngles( ent, angles );
	if ( !vrAim )
	{
		VectorCopy( ent->client->ps.viewangles, angles );
	}

	AngleVectors (angles, forward, right, up);

	CalcMuzzlePointOrigin( ent, muzzle_origin, forward, right, up, muzzle );

	// the +14 compensates the stock eye->muzzle offset, which doesn't exist
	// when the muzzle IS the 6DOF controller position
	VectorMA( muzzle_origin, vrAim ? 32.0 : ( 32.0 + 14.0 ), forward, end );

	trap_Trace( &tr, muzzle_origin, NULL, NULL, end, ent->s.number, MASK_SHOT );
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( ent->client->noclip ) {
		return qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

	if ( !traceEnt->takedamage ) {
		return qfalse;
	}

	if (ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1.0;
	}
#ifdef MISSIONPACK
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif

	damage = Mode_GetConfig( g_mode.integer )->weapons[WP_GAUNTLET].damage * s_quadFactor;
	G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_GAUNTLET );

	return qtrue;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( v[i] < 0 ) {
			if ( to[i] >= v[i])
			v[i] = (int)v[i];
			else
				v[i] = (int)v[i] - 1;
		} else {
			if ( to[i] <= v[i] )
				v[i] = (int)v[i];
			else
			v[i] = (int)v[i] + 1;
		}
	}
}


static void Bullet_Fire( gentity_t *ent, float spread, int damage, int mod ) {
	trace_t		tr;
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t		impactpoint, bouncedir;
#endif
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

	damage *= s_quadFactor;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;

	VectorMA( muzzle_origin, ( 8192.0 * 16.0 ), forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	passent = ent->s.number;
	for ( i = 0; i < 10; i++ ) {

		// unlagged
		G_DoTimeShiftFor( ent );

		trap_Trace( &tr, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );

		// unlagged
		G_UndoTimeShiftFor( ent );

		if ( tr.surfaceFlags & SURF_NOIMPACT )
			return;

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle_origin );

		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;

			// unlagged
			tent->s.clientNum = ent->s.clientNum;

			if( LogAccuracyHit( traceEnt, ent ) ) {
				ent->client->accuracy_hits++;
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage ) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
#endif
				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, mod );
#ifdef MISSIONPACK
			}
#endif
		}
		break;
	}
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire( gentity_t *ent ) {
	gentity_t *m;

	m = fire_bfg( ent, muzzle, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

SHOTGUN

======================================================================
*/


static qboolean ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *ent ) {
	trace_t		tr;
	int			damage, i, passent;
	gentity_t	*traceEnt;
#ifdef MISSIONPACK
	vec3_t		impactpoint, bouncedir;
#endif
	vec3_t		tr_start, tr_end;
	qboolean	hitClient = qfalse;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );

	for ( i = 0; i < 10; i++ ) {
		trap_Trace( &tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT );
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if (  tr.surfaceFlags & SURF_NOIMPACT ) {
			return qfalse;
		}

		if ( traceEnt->takedamage ) {
			damage = Mode_GetConfig( g_mode.integer )->weapons[WP_SHOTGUN].damage * s_quadFactor;
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( tr_start, impactpoint, bouncedir, tr_end );
					VectorCopy( impactpoint, tr_start );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, tr_start );
					passent = traceEnt->s.number;
				}
				continue;
			}
#endif
			if ( LogAccuracyHit( traceEnt, ent ) ) {
				hitClient = qtrue;
			}
			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN );
			return hitClient;
		}
		return qfalse;
	}
	return qfalse;
}


// this should match CG_ShotgunPattern
static void ShotgunPattern( const vec3_t origin, const vec3_t origin2, int seed, gentity_t *ent ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	qboolean	hitClient = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// unlagged
	G_DoTimeShiftFor( ent );

	{
		const modeConfig_t *gp = Mode_GetConfig( g_mode.integer );
		float angle, radius;
		int ring, ringIndex;

		// generate spread pattern
		for ( i = 0 ; i < gp->weapons[WP_SHOTGUN].count ; i++ ) {
			if ( gp->sgPatternType == 2 ) {
				// CPM dual-ring pattern: 8 inner + 8 outer, offset 22.5° so no pellets at 0/90/180/270
				ring = ( i < 8 ) ? 0 : 1;
				ringIndex = ( i < 8 ) ? i : i - 8;
				radius = ring ? (float)gp->weapons[WP_SHOTGUN].spread * 16.0f : (float)gp->weapons[WP_SHOTGUN].spread * 16.0f * 0.40f;
				angle = 2.0f * M_PI * ringIndex / 8.0f + ( M_PI / 8.0f );	// offset 22.5°
				r = cos( angle ) * radius;
				u = sin( angle ) * radius;
			} else if ( gp->sgPatternType == 1 ) {
				// QL ring pattern: 3 concentric rings (inner 6, middle 6, outer 8)
				if ( i < 6 ) {
					ring = 0; ringIndex = i;			// inner ring: 6 pellets at 0°,60°,...
				} else if ( i < 12 ) {
					ring = 1; ringIndex = i - 6;		// middle ring: 6 pellets, rotated 30°
				} else {
					ring = 2; ringIndex = i - 12;		// outer ring: 8 pellets at 0°,45°,...
				}
				radius = (float)gp->weapons[WP_SHOTGUN].spread * 16.0f * ( ring + 1 ) / 3.0f;
				if ( ring == 0 ) {
					angle = 2.0f * M_PI * ringIndex / 6.0f;				// 0°, 60°, 120°...
				} else if ( ring == 1 ) {
					angle = 2.0f * M_PI * ringIndex / 6.0f - ( 25.0f * M_PI / 180.0f );	// -25° offset
				} else {
					angle = 2.0f * M_PI * ringIndex / 8.0f;				// 0°, 45°, 90°...
				}
				r = cos( angle ) * radius;
				u = sin( angle ) * radius;
			} else {
				// VQ3 random spread
				r = Q_crandom( &seed ) * gp->weapons[WP_SHOTGUN].spread * 16;
				u = Q_crandom( &seed ) * gp->weapons[WP_SHOTGUN].spread * 16;
			}
			VectorMA( origin, ( 8192.0 * 16.0 ), forward, end );
			VectorMA( end, r, right, end );
			VectorMA( end, u, up, end );
			if ( ShotgunPellet( origin, end, ent ) && !hitClient ) {
				hitClient = qtrue;
				ent->client->accuracy_hits++;
			}
		}
	}

	// unlagged
	G_UndoTimeShiftFor( ent );
}


static void weapon_supershotgun_fire( gentity_t *ent ) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096.0, tent->s.origin2 );

	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() & 255;		// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( muzzle_origin, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PLASMA GUN

======================================================================
*/

void Weapon_Plasmagun_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define	MAX_RAIL_HITS	4
void weapon_railgun_fire( gentity_t *ent ) {
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	damage = Mode_GetConfig( g_mode.integer )->weapons[WP_RAILGUN].damage * s_quadFactor;

	VectorMA( muzzle_origin, 8192.0, forward, end );

	// unlagged
	G_DoTimeShiftFor( ent );

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace( &trace, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );
		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if ( traceEnt->takedamage ) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if ( G_InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					SnapVectorTowards( trace.endpos, muzzle );
					// send railgun beam effect
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					// set player number for custom colors on the railtrail
					tent->s.clientNum = ent->s.clientNum;
					VectorCopy( muzzle, tent->s.origin2 );
					// move origin a bit to come closer to the drawn gun muzzle
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;	// don't make the explosion at the end
					//
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			}
			else {
				if ( LogAccuracyHit( traceEnt, ent ) ) {
					hits++;
				}
				G_Damage( traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN );
			}
#else
			if ( LogAccuracyHit( traceEnt, ent ) ) {
				hits++;
			}
			G_Damage( traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN );
#endif
		}
		if ( trace.contents & CONTENTS_SOLID ) {
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

	// unlagged
	G_UndoTimeShiftFor( ent );


	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle_origin );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
	VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );

	SnapVector( tent->s.origin2 );

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if ( hits == 0 ) {
		// complete miss
		ent->client->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->client->accurateCount += hits;
		if ( ent->client->accurateCount >= 2 ) {
			ent->client->accurateCount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;

			G_LogPrintf( "Award: %d impressive: %s\n",
				ent->client->ps.clientNum, ent->client->pers.netname );

			// add the sprite over the player's head
			ent->client->ps.eFlags &= ~EF_AWARDS;
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
		ent->client->accuracy_hits++;
	}

}


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
	gentity_t	*hook;

	// a bot's press throws exactly one hook: a held trigger re-throwing the
	// instant a miss frees it would keep botlib's "hook out" state alive
	// forever. A player's held trigger still re-throws
	if ( !ent->client->hook
			&& !( ( ent->r.svFlags & SVF_BOT ) && ent->client->fireHeld ) ) {
		hook = fire_grapple (ent, muzzle, forward);
		hook->damage *= s_quadFactor;
		if ( ( ent->r.svFlags & SVF_BOT ) && trap_Cvar_VariableIntegerValue( "bot_grapple" ) >= 2 ) {
			G_Printf( "GRAPPLE-LAUNCH c%d t %d yaw %.2f pitch %.2f\n", ent->s.number, level.time,
					ent->client->ps.viewangles[YAW], ent->client->ps.viewangles[PITCH] );
		}
	}
	ent->client->fireHeld = qtrue;
}


/*
======================
G_GrappleCoMoverMember

The anchor or a member of its wired co-mover set
======================
*/
qboolean G_GrappleCoMoverMember( gentity_t *hook, int entityNum ) {
	int		k, n;

	if ( hook->target_ent && hook->target_ent->s.number == entityNum ) {
		return qtrue;
	}
	for ( k = 0 ; k < 3 ; k++ ) {
		n = ( hook->s.time2 >> ( k * 10 ) ) & 0x3FF;
		if ( n && n == entityNum ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
G_GrappleDamage / G_GrappleKnockback

Overrides, empty meaning the mode table decides, the same path every other
weapon takes. Seeding these as cvar defaults cannot work: Cvar_Get keeps an
existing cvar's value, so a mode change between maps never reaches them.
==================
*/
int G_GrappleDamage( void ) {
	if ( g_damage_gh.string[0] ) {
		return g_damage_gh.integer;
	}
	return Mode_GetConfig( g_mode.integer )->weapons[WP_GRAPPLING_HOOK].damage;
}

float G_GrappleKnockback( void ) {
	if ( g_knockback_gh.string[0] ) {
		return g_knockback_gh.value;
	}
	return Mode_GetConfig( g_mode.integer )->weapons[WP_GRAPPLING_HOOK].knockback;
}

void Weapon_HookFree (gentity_t *ent)
{
	// the brush runs a frame ahead of the hold on approach strokes, so a
	// captured wielder can part overlapping it: step them clear first
	if ( ent->s.generic1 && ent->parent->inuse && ent->parent->client
			&& ent->parent->client->hook == ent
			&& ent->target_ent && ent->target_ent->inuse ) {
		trace_t		tr;
		gentity_t	*w = ent->parent;

		trap_Trace( &tr, w->r.currentOrigin, w->r.mins, w->r.maxs,
			w->r.currentOrigin, w->s.number, MASK_PLAYERSOLID );
		if ( tr.startsolid ) {
			vec3_t	mang, n, p;
			vec3_t	m[3], mt[3];
			int		k;

			BG_EvaluateTrajectory( &ent->target_ent->s.apos, level.time, mang );
			G_CreateRotationMatrix( mang, mt );
			G_TransposeMatrix( mt, m );
			VectorScale( ent->pos2, -1, n );
			G_RotatePoint( n, m );
			for ( k = 8 ; k <= 64 ; k += 8 ) {
				VectorMA( w->r.currentOrigin, k, n, p );
				trap_Trace( &tr, p, w->r.mins, w->r.maxs, p, w->s.number, MASK_PLAYERSOLID );
				if ( !tr.startsolid ) {
					VectorCopy( p, w->client->ps.origin );
					VectorCopy( p, w->r.currentOrigin );
					trap_LinkEntity( w );
					break;
				}
			}
		}
	}

	// a reused client slot must not lose its own live hook: only clear the
	// owner's pointer and pull bit if they still name this hook
	if ( ent->parent && ent->parent->client && ent->parent->client->hook == ent ) {
		ent->parent->client->hook = NULL;
		ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	}

	// PVS culling can drop an anchored pad from a client's snapshot without it
	// having been freed, so absence is not a release. The event is, and it is
	// broadcast, so PVS cannot swallow it either. Fires on EVERY
	// release, anchored or not: the client's seat ramp restart lives on this
	// same event, and a miss (the common case, firing into open air) is still
	// a release that must re-materialize the pad
	{
		gentity_t	*tent;

		tent = G_TempEntity( ent->r.currentOrigin, EV_GRAPPLE_RELEASE );
		VectorCopy( ent->s.angles, tent->s.angles );
		tent->s.otherEntityNum = ent->parent->s.number;
		// only an anchored release drops fall debris; a miss carries no
		// frozen impact normal for it to fall away from
		tent->s.eventParm = ( ent->s.eType == ET_GRAPPLE ) ? 1 : 0;
		// the release scar re-derives the pad's spin from these; a witness
		// who never had the anchored pad in a snapshot has no other source.
		// otherEntityNum2 flags a mover anchor (no scar)
		tent->s.time = ent->s.time;
		// 32 bits; generic1 is 8 on the wire. This is the release tent, not the
		// hook: the hook entity's own time2 is the packed co-mover set instead,
		// same field name, different eType, different meaning
		tent->s.time2 = ent->s.number;
		tent->s.otherEntityNum2 = ent->s.otherEntityNum2;
		tent->s.modelindex2 = ent->s.modelindex2;	// a body anchor leaves no scar
		tent->r.svFlags |= SVF_BROADCAST;	// a release is a release for everyone, PVS or not
	}

	G_FreeEntity( ent );
}


void Weapon_HookThink (gentity_t *ent)
{
	// owner gone: drop the orphan before it writes into a reused client slot
	if ( !ent->parent->inuse || ent->parent->client->hook != ent ) {
		G_FreeEntity( ent );
		return;
	}

	// no wielder, no projection (also catches a mid-latch switch to spectator)
	if ( ent->parent->health <= 0
			|| ent->parent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		Weapon_HookFree( ent );
		return;
	}

	if (ent->enemy) {
		vec3_t v, oldorigin, dir;
		int dmg;

		// a dead, vanished, or turned-teammate target frees the hook rather
		// than dragging the owner to it
		if ( !ent->enemy->inuse || !ent->enemy->r.linked
				|| ent->enemy->health <= 0
				|| OnSameTeam( ent->parent, ent->enemy ) ) {
			Weapon_HookFree( ent );
			return;
		}

		// re-ticks impact damage on the model's own tick; skip at 0 since G_Damage still counts a hit
		if ( G_GrappleDamage() > 0 && level.time >= ent->timestamp ) {
			// a tick spans seconds, so read the wielder's LIVE quad here rather
			// than the fire-time snapshot
			dmg = G_GrappleDamage();
			if ( ent->parent->client && ent->parent->client->ps.powerups[PW_QUAD] > level.time ) {
				dmg *= g_quadfactor.value;
			}
#ifdef MISSIONPACK
			if ( ent->parent->client && ent->parent->client->persistantPowerup
					&& ent->parent->client->persistantPowerup->item
					&& ent->parent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
				dmg *= 2;
			}
#endif
			VectorSubtract( ent->enemy->r.currentOrigin, ent->parent->r.currentOrigin, dir );
			VectorNormalize( dir );
			G_Damage( ent->enemy, ent, ent->parent, dir, ent->r.currentOrigin,
				dmg, 0, MOD_GRAPPLE );
			// the tick can kill the body it is anchored in, and the death frees
			// this hook out from under it
			if ( !ent->inuse || !ent->parent || !ent->parent->client || ent->parent->client->hook != ent ) {
				return;
			}
			ent->timestamp += GRAPPLE_MODEL_TICK_MS;
		}

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		SnapVectorTowards( v, oldorigin );	// save net bandwidth

		G_SetOrigin( ent, v );
		ent->s.pos.trType = TR_INTERPOLATE;	// client lerps the tracked pad
		trap_LinkEntity( ent );
	}
	else if ( ent->target_ent ) {
		gentity_t	*mover = ent->target_ent;
		vec3_t		morg, mang, f, p, oldorigin;
		vec3_t		m[3], mt[3];
		trace_t		tr;

		// anchor entity gone (a killed shoot-door): nothing left to grip
		if ( !mover->inuse || mover->s.eType != ET_MOVER ) {
			Weapon_HookFree( ent );
			return;
		}

		BG_EvaluateTrajectory( &mover->s.pos, level.time, morg );
		BG_EvaluateTrajectory( &mover->s.apos, level.time, mang );

		// mover-frame anchor back out to world
		G_CreateRotationMatrix( mang, mt );
		G_TransposeMatrix( mt, m );
		VectorCopy( ent->pos1, p );
		G_RotatePoint( p, m );
		VectorAdd( morg, p, p );

		VectorCopy( ent->r.currentOrigin, oldorigin );
		SnapVectorTowards( p, oldorigin );	// save net bandwidth

		// rides the mover frame like the offset; vectoangles zeroing roll matches the pad's own anchor
		VectorCopy( ent->pos2, f );
		G_RotatePoint( f, m );
		vectoangles( f, ent->s.angles );

		G_SetOrigin( ent, p );
		ent->s.pos.trType = TR_INTERPOLATE;	// client lerps the tracked pad
		VectorCopy( ent->s.angles, ent->s.apos.trBase );	// angles ride the same lerp
		trap_LinkEntity( ent );

		// a sweep overrunning a grounded wielder can't be captured out of
		// the way: snap the link; airborne overlap is an arrival instead
		trap_Trace( &tr, ent->parent->r.currentOrigin, ent->parent->r.mins,
			ent->parent->r.maxs, ent->parent->r.currentOrigin,
			ent->parent->s.number, MASK_PLAYERSOLID );
		if ( tr.startsolid && tr.entityNum == mover->s.number
				&& !ent->s.generic1
				&& ent->parent->client->ps.groundEntityNum != ENTITYNUM_NONE ) {
			Weapon_HookFree( ent );
			return;
		}

		// within reach the wielder is captured: perpendicular off the face,
		// body support clear along the normal (rotators keep the full
		// margin, since their spinning normal sweeps the box)
		if ( !ent->s.generic1
				&& ent->parent->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& Distance( ent->parent->r.currentOrigin, ent->r.currentOrigin ) <= GRAPPLE_LATCH_REACH ) {
			vec3_t		loc, out;
			float		standoff;

			VectorScale( ent->pos2, -1, out );
			G_RotatePoint( out, m );

			if ( mover->s.apos.trType == TR_STATIONARY ) {
				standoff = 8.0f
					+ ent->parent->r.maxs[0] * fabs( out[0] )
					+ ent->parent->r.maxs[1] * fabs( out[1] )
					+ ( out[2] < 0 ? ent->parent->r.maxs[2] : -ent->parent->r.mins[2] ) * fabs( out[2] );
			} else {
				standoff = GRAPPLE_LATCH_STANDOFF;
			}

			VectorScale( ent->pos2, -standoff, loc );
			VectorAdd( ent->pos1, loc, loc );
			G_RotatePoint( loc, m );
			VectorAdd( morg, loc, loc );

			// tight sockets: the tether sets longer, out to where the body
			// fits clear of everything but the anchor; no fit = a pinned
			// capture, and the squeeze check resolves the lethal pins
			{
				vec3_t		probe;
				int			k;
				qboolean	linked;

				linked = mover->r.linked ? qtrue : qfalse;
				if ( linked ) {
					trap_UnlinkEntity( mover );
				}
				for ( k = 0; k <= 64; k += 8 ) {
					VectorMA( loc, (float)k, out, probe );
					trap_Trace( &tr, probe, ent->parent->r.mins, ent->parent->r.maxs,
						probe, ent->parent->s.number, MASK_PLAYERSOLID );
					if ( tr.startsolid ) {
						continue;
					}
					if ( k > 0 ) {
						// margin holds the brush frame-lead off the foreign
						// blocker; bare fit when even that is blocked
						VectorMA( loc, (float)k + GRAPPLE_FIT_MARGIN, out, probe );
						trap_Trace( &tr, probe, ent->parent->r.mins, ent->parent->r.maxs,
							probe, ent->parent->s.number, MASK_PLAYERSOLID );
						if ( tr.startsolid ) {
							VectorMA( loc, (float)k, out, probe );
						}
					}
					VectorCopy( probe, loc );
					break;
				}
				if ( linked ) {
					trap_LinkEntity( mover );
				}
			}

			trap_Trace( &tr, ent->parent->r.currentOrigin, ent->parent->r.mins,
				ent->parent->r.maxs, loc, ent->parent->s.number, MASK_PLAYERSOLID );
			if ( !tr.startsolid ) {
				VectorCopy( tr.endpos, loc );
			}
			VectorSubtract( loc, morg, loc );
			G_RotatePoint( loc, mt );

			VectorCopy( loc, ent->movedir );
			ent->s.generic1 = 1;

			// the whole co-mover set rides with the anchor: resolve
			// the set once and wire it, so both prediction sides agree
			ent->s.time2 = 0;
			{
				int		n, k, itmp, slot;
				int		cand[3];
				float	cdist[3];
				float	d, ftmp;
				vec3_t	cp;

				slot = 0;
				for ( n = MAX_CLIENTS ; n < level.num_entities ; n++ ) {
					gentity_t *co = &g_entities[n];

					if ( !co->inuse || co->s.eType != ET_MOVER || co == mover
							|| !BG_MoverCoMoves( &mover->s, &co->s ) ) {
						continue;
					}
					if ( co->r.absmin[0] > mover->r.absmax[0] + 128
							|| co->r.absmax[0] < mover->r.absmin[0] - 128
							|| co->r.absmin[1] > mover->r.absmax[1] + 128
							|| co->r.absmax[1] < mover->r.absmin[1] - 128
							|| co->r.absmin[2] > mover->r.absmax[2] + 128
							|| co->r.absmax[2] < mover->r.absmin[2] - 128 ) {
						continue;
					}
					// bigger movers can exceed the three slots: keep the
					// parts nearest the pad, they contact the captive first
					for ( k = 0 ; k < 3 ; k++ ) {
						cp[k] = ent->r.currentOrigin[k];
						if ( cp[k] < co->r.absmin[k] ) cp[k] = co->r.absmin[k];
						else if ( cp[k] > co->r.absmax[k] ) cp[k] = co->r.absmax[k];
					}
					d = DistanceSquared( ent->r.currentOrigin, cp );
					if ( slot < 3 ) {
						cand[slot] = co->s.number;
						cdist[slot] = d;
						slot++;
					} else if ( d < cdist[2] ) {
						cand[2] = co->s.number;
						cdist[2] = d;
					} else {
						continue;
					}
					for ( k = slot - 1 ; k > 0 && cdist[k] < cdist[k-1] ; k-- ) {
						itmp = cand[k]; cand[k] = cand[k-1]; cand[k-1] = itmp;
						ftmp = cdist[k]; cdist[k] = cdist[k-1]; cdist[k-1] = ftmp;
					}
				}
				for ( k = 0 ; k < slot ; k++ ) {
					ent->s.time2 |= cand[k] << ( k * 10 );
				}
			}
		}

		// G_SetOrigin wipes trDelta every think, so re-stamp the wire copy
		// of the captured pose
		if ( ent->s.generic1 ) {
			VectorCopy( ent->movedir, ent->s.pos.trDelta );
		}

		// the mover cannot see its captive to push or reverse: when the
		// world pins the wielder and the brush overruns them, crush is here
		if ( ent->s.generic1 ) {
			vec3_t	pose, o2, a2;
			vec3_t	m2[3], mt2[3];
			int		t;

			// both operands at the same instant, or lag reads as strain
			t = ent->parent->client->ps.commandTime;
			BG_EvaluateTrajectory( &mover->s.pos, t, o2 );
			BG_EvaluateTrajectory( &mover->s.apos, t, a2 );
			G_CreateRotationMatrix( a2, mt2 );
			G_TransposeMatrix( mt2, m2 );
			VectorCopy( ent->movedir, pose );
			G_RotatePoint( pose, m2 );
			VectorAdd( o2, pose, pose );
			if ( Distance( pose, ent->parent->r.currentOrigin ) > GRAPPLE_CRUSH_STRAIN ) {
				trap_Trace( &tr, ent->parent->r.currentOrigin, ent->parent->r.mins,
					ent->parent->r.maxs, ent->parent->r.currentOrigin,
					ent->parent->s.number, MASK_PLAYERSOLID );
				if ( tr.startsolid ) {
					G_Damage( ent->parent, mover, mover, NULL, NULL, 99999, 0, MOD_CRUSH );
				}
			}
		}
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
	ent->nextthink = level.time + 25;	// every frame: smooth follow, on-schedule ticks
}


/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void Weapon_LightningFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	gentity_t	*traceEnt, *tent;
	int			damage, i, passent;

	damage = Mode_GetConfig( g_mode.integer )->weapons[WP_LIGHTNING].damage * s_quadFactor;

	passent = ent->s.number;

	for (i = 0; i < 10; i++) {
		VectorMA( muzzle_origin, Mode_GetConfig( g_mode.integer )->weapons[WP_LIGHTNING].range, forward, end );

		// unlagged
		G_DoTimeShiftFor( ent );

		trap_Trace( &tr, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );

		// unlagged
		G_UndoTimeShiftFor( ent );

#ifdef MISSIONPACK
		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if (i) {
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			//
			tent = G_TempEntity( muzzle, EV_LIGHTNINGBOLT );
			VectorCopy( tr.endpos, end );
			SnapVector( end );
			VectorCopy( end, tent->s.origin2 );
		}
#endif
		if ( tr.entityNum == ENTITYNUM_NONE ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if ( traceEnt->takedamage ) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					VectorSubtract( end, impactpoint, forward );
					VectorNormalize(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
#endif
			if ( LogAccuracyHit( traceEnt, ent ) ) {
				ent->client->accuracy_hits++;
			}
			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_LIGHTNING );
		}

		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
		} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
		}

		break;
	}
}

#ifdef MISSIONPACK
/*
======================================================================

NAILGUN

======================================================================
*/

void Weapon_Nailgun_Fire (gentity_t *ent) {
	gentity_t	*m;
	int			count;
	int			nailCount = Mode_GetConfig( g_mode.integer )->weapons[WP_NAILGUN].count;

	for( count = 0; count < nailCount; count++ ) {
		m = fire_nail (ent, muzzle, forward, right, up );
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
	}

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void weapon_proxlauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_prox (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

#endif

//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	vec3_t viewang;

	if ( ent->client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1.0;
	}
#ifdef MISSIONPACK
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET ) {
#ifdef MISSIONPACK
		if( ent->s.weapon == WP_NAILGUN ) {
			int nc = Mode_GetConfig( g_mode.integer )->weapons[WP_NAILGUN].count;
			ent->client->accuracy_shots += nc;
		} else {
			ent->client->accuracy_shots++;
		}
#else
		ent->client->accuracy_shots++;
#endif
	}

	// set aiming directions
	if ( !G_VR_AimAngles( ent, viewang ) )
	{
		VectorCopy( ent->client->ps.viewangles, viewang );
	}

	AngleVectors( viewang, forward, right, up );

	CalcMuzzlePointOrigin( ent, muzzle_origin, forward, right, up, muzzle );

	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire( ent );
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		break;
	case WP_MACHINEGUN:
		{
			const modeConfig_t *gp = Mode_GetConfig( g_mode.integer );
			if ( g_gametype.integer != GT_TEAM ) {
				Bullet_Fire( ent, gp->weapons[WP_MACHINEGUN].spread, gp->weapons[WP_MACHINEGUN].damage, MOD_MACHINEGUN );
			} else {
				Bullet_Fire( ent, gp->weapons[WP_MACHINEGUN].spread, gp->weapons[WP_MACHINEGUN].teamDamage, MOD_MACHINEGUN );
			}
		}
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire( ent );
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire( ent );
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire( ent );
		break;
	case WP_RAILGUN:
		weapon_railgun_fire( ent );
		break;
	case WP_BFG:
		BFG_Fire( ent );
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		Weapon_Nailgun_Fire( ent );
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire( ent );
		break;
	case WP_CHAINGUN:
		{
			const modeConfig_t *gp = Mode_GetConfig( g_mode.integer );
			Bullet_Fire( ent, gp->weapons[WP_CHAINGUN].spread, gp->weapons[WP_CHAINGUN].damage, MOD_CHAINGUN );
		}
		break;
#endif
	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
}


#ifdef MISSIONPACK

/*
===============
KamikazeRadiusDamage
===============
*/
static void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (!ent->takedamage) {
			continue;
		}

		// dont hit things we have already hit
		if( ent->kamikazeTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += Mode_GetConfig( g_mode.integer )->splashZKnockback;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		// dont hit things we have already hit
		if( ent->kamikazeShockTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			dir[2] += Mode_GetConfig( g_mode.integer )->splashZKnockback;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			//
			dir[2] = 0;
			VectorNormalize(dir);
			if ( ent->client ) {
				ent->client->ps.velocity[0] = dir[0] * push;
				ent->client->ps.velocity[1] = dir[1] * push;
				ent->client->ps.velocity[2] = 100;
			}
			ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void KamikazeDamage( gentity_t *self ) {
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if (self->count >= KAMI_SHOCKWAVE_STARTTIME) {
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400,	(int) (float) t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME) );
	}
	//
	if (self->count >= KAMI_EXPLODE_STARTTIME) {
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage( self->s.pos.trBase, self->activator, 400,	(int) (float) t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME) );
	}

	// either cycle or kill self
	if( self->count >= KAMI_SHOCKWAVE_ENDTIME ) {
		G_FreeEntity( self );
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE) {
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void G_StartKamikaze( gentity_t *ent ) {
	gentity_t	*explosion;
	gentity_t	*te;
	vec3_t		snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if ( ent->client ) {
		VectorCopy( ent->s.pos.trBase, snapped );
	}
	else {
		VectorCopy( ent->activator->s.pos.trBase, snapped );
	}
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( explosion, snapped );

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	trap_LinkEntity( explosion );

	if (ent->client) {
		//
		explosion->activator = ent;
		//
		ent->s.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		G_Damage( ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE );
	}
	else {
		if ( !strcmp(ent->activator->classname, "bodyque") ) {
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		}
		else {
			explosion->activator = ent->activator;
		}
	}

	// play global sound at all clients
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}
#endif
