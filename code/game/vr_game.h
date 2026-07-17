// Vendored VR API - game-module hooks (vr_game). Bootstrap, head-bit codec,
// 6DOF aim/muzzle overrides, and config-mirror writes. Every hook is
// dormancy-safe: on a flatscreen engine G_VR_Init leaves the mirror zeroed and
// g_vrActive false, predicates return qfalse, transforms leave stock values.
#ifndef VR_GAME_H
#define VR_GAME_H

// No includes: this header is included only from the tail of g_local.h,
// after q_shared.h/bg_public.h/vr_shared.h are already visible there
// (bg_public.h has no include guard, so re-including it here would be
// unsafe if this header were ever pulled in a second time).

// forward decls (full types in g_local.h; this header is included from there)
struct gentity_s;
struct gclient_s;

void G_VR_Init( void );
qboolean G_VR_Active( void );
void G_VR_ClientThink( struct gclient_s *client, const usercmd_t *ucmd );
void G_VR_ClientEndFrame( struct gclient_s *client, struct gentity_s *ent );
qboolean G_VR_ClientIsVR( const char *userinfo );
qboolean G_VR_MuzzlePoint( struct gentity_s *ent, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t origin, vec3_t muzzlePoint );
qboolean G_VR_AimAngles( struct gentity_s *ent, vec3_t angles );

#endif
