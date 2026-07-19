// Vendored VR API - cgame module hooks (vr_cgame). Bootstrap, config mirror,
// haptic wrapper, and the conformance probe overlay. Dormancy-safe: on a
// flatscreen engine CG_VR_Init leaves the mirror zeroed and vrActive false;
// the haptic wrapper and probe draw no-op.
#ifndef VR_CGAME_H
#define VR_CGAME_H

// No includes: this header is included only from the tail of cg_local.h,
// after q_shared.h/bg_public.h/vr_shared.h are already visible there
// (bg_public.h has no include guard, so re-including it here would be
// unsafe if this header were ever pulled in a second time).

void CG_VR_Init( void );
void CG_VR_Shutdown( void );
void CG_VR_RegisterMedia( void );
void CG_VR_Frame( void );
void CG_VRHaptic( const char *description, int position, int channel, int intensity, float yaw, float height );
void CG_VRProbe_Draw( void );

// cg_view.c per-frame view-pipeline hooks: FOV override, forced third-person
// camera modes, view-kick suppression, and damage view-punch scale. Each is
// a verbatim gate-for-hook swap at its original call site.
void CG_VR_Fov( float *fov_x, float *fov_y );
qboolean CG_VR_ForceThirdPerson( void );
qboolean CG_VR_OwnsViewKick( void );
float CG_VR_DamageRollScale( void );

// VR camera queries used across cg_view.c and the hooks below (moved from
// cg_local.h; the definitions and their static helpers/CG_CalcViewValues
// fork moved here too - see vr_cgame.c).
qboolean CG_VR_IsThirdPersonFollow( VR_FollowMode followMode );
qboolean CG_VR_IsDeathCam( void );

// CG_CalcViewValues view-pipeline hooks: the VR fork's body, split
// at its VR-only regions so stock CG_CalcViewValues can call through them.
qboolean CG_VR_MenuViewFreeze( void );
qboolean CG_VR_IntermissionView( void );
qboolean CG_VR_OffsetView( void );
void CG_VR_ComputeWeaponAngles( void );
qboolean CG_VR_ViewAxis( void );

// VR-protocol head rendering (cg_predict.c/cg_view.c/cg_draw.c call sites).
// EF-gated or self-gating, NOT vrActive-gated: flatscreen engines render VR
// players' heads too. Reset re-seeds the follow EMA (tv_seek_sync).
qboolean CG_VR_IsVRFollow( void );
void CG_VR_InterpolateHeadStats( playerState_t *out, const playerState_t *prev, const playerState_t *next, float f );
void CG_VR_FollowHeadView( const playerState_t *ps );
void CG_VR_FollowHeadViewReset( void );

// Game-event hooks: each wraps the haptic dispatch (and, for OnTeleport, the
// fake-6DoF realign) that used to live inline at the event's call site. Call
// sites keep whatever local-player gate they already had; hooks that were
// passed enough context to re-derive that gate themselves (OnTeleport,
// OnHitByMissile, OnDamageTaken) keep it inside instead.
void CG_VR_OnUseItem( void );
void CG_VR_OnFall( int severity );
void CG_VR_OnJump( qboolean pad );
void CG_VR_OnItemPickup( const gitem_t *item );
void CG_VR_OnWeaponSwitch( void );
void CG_VR_OnTeleport( int clientNum );
void CG_VR_OnDeath( void );
void CG_VR_OnPowerup( void );
void CG_VR_OnGibbed( void );
void CG_VR_OnHitByMissile( int entityNum );
void CG_VR_OnDamageTaken( int damage, float yaw );
void CG_VR_OnWeaponFired( int weapon );
void CG_VR_OnWeaponFiring( int weapon );

// ui_shared.c Item_SetFocus menu-hover haptic (wired via DC->vrMenuMove;
// the ui link's counterpart is UI_VR_OnMenuMove in code/ui/vr_ui.c).
void CG_VR_OnMenuMove( void );

// cg_weapons.c CG_AddViewWeapon hand-pose hooks (fork's former CG_AddViewWeaponVR)
// plus the matrix/position helpers and weapon selector that moved with them.
qboolean CG_VR_WeaponWheel( void );
qboolean CG_VR_HideViewWeapon( void );
qboolean CG_VR_WeaponHandPose( vec3_t origin, vec3_t angles, float *scale );
void CG_VR_WeaponHandFinish( refEntity_t *hand, float scale );
void CG_WeaponSelectorSelect_f( void );
void CG_DrawWeaponSelector( void );
void CG_ConvertFromVR(vec3_t in, vec3_t offset, vec3_t out);
void rotateAboutOrigin(float x, float y, float rotation, vec2_t out);
void CG_CalculateVROffHandPosition( vec3_t origin, vec3_t angles );
void CG_CalculateVRWeaponPosition( vec3_t origin, vec3_t angles );

// CG_DrawActive frame-tail dispatch (cg_draw.c's former CG_DrawActiveVR):
// scene submit plus 2D/HUD drawing inside the post-bloom / HUD-buffer
// brackets. qtrue: VR frame fully drawn, caller returns; qfalse: flatscreen
// engine, caller runs the stock tail. stereoView is unused (the body hardcodes
// STEREO_CENTER - pre-existing).
qboolean CG_VR_DrawFrame( stereoFrame_t stereoView );

// suppression predicates / HUD visibility (cg_draw.c/cg_view.c/cg_weapons.c/
// cg_newdraw.c/cg_scoreboard.c 2D-HUD and suppression gates - see vr_cgame.c)
qboolean CG_VR_OwnsHudVisibility( void );
qboolean CG_VR_HudVisible( void );
qboolean CG_VR_Owns2DCrosshair( void );
qboolean CG_VR_OwnsWeaponSelect( void );
qboolean CG_VR_OwnsDamageBorder( void );
qboolean CG_VR_OwnsViewBob( void );
qboolean CG_VR_OwnsHiddenGunMuzzle( void );
qboolean CG_VR_SuppressDeadScoreboard( void );
qboolean CG_VR_OwnsHoldableIcon( void );

// cg_drawtools.c CG_AdjustFrom640 keystone: the entire vrActive scaling body,
// moved verbatim. qtrue: *x/*y/*w/*h scaled for VR, caller returns; qfalse:
// caller runs the stock flatscreen scale.
qboolean CG_VR_AdjustFrom640( float *x, float *y, float *w, float *h );

// cg_draw.c CG_ScanForCrosshairEntity aim ray: qtrue when the VR weapon-axis
// ray was written to start/end; qfalse: caller runs the stock view ray.
qboolean CG_VR_CrosshairScanRay( vec3_t start, vec3_t end );

// cg_event.c EV_RAILTRAIL muzzle origin: qtrue when the VR weapon position
// was written to origin/angles; qfalse: caller runs the stock gun-offset math.
qboolean CG_VR_WeaponMuzzleOrigin( vec3_t origin, vec3_t angles );

// cg_newdraw.c chat paint offsets (bodies are the pre-existing expressions
// verbatim; dormant/flatscreen values fold to 0).
int CG_VR_ChatOffsetX( void );
int CG_VR_ChatOffsetY( void );

// cg_draw.c / cg_newdraw.c HUD portrait head angles for VR players. EF-gated
// inside, NOT vrActive-gated: flatscreen engines still need to render a VR
// player's real head orientation (spectating/demo playback).
qboolean CG_VR_PortraitHeadAngles( vec3_t angles );

// Standardized single-value cvar read/write accessors.
float CG_VR_CvarValue( const char *name );
void CG_VR_CvarSet( const char *name, float value );

// Drop-state accessors and reset call-outs (the VR state these touch lives
// inside vr_cgame.c; hosts read and re-arm it only through these).
void CG_VR_DeathCamReset( void );          // local player (re)spawn / new gamestate
void CG_VR_PortraitReset( void );          // HUD portrait subject changed
qboolean CG_VR_DrawingZoomedHUD( void );   // inside the zoom minimal-HUD pass
qhandle_t CG_VR_ReticleShader( void );     // zoom scope mask, 0 until media loads
qboolean CG_VR_ClientIsVR( int clientNum ); // server-flagged VR player (vr\ configstring field)

// cg_players.c embodiment hooks: first-person body pose/visibility, off-hand
// item suppression/pose (CG_TrailItem's VR branch), the off-hand holdable
// draw (former CG_PlayerPowerups block), flag-carry routing, the local-
// client-entity query, and the followed-player head sprite.
// CG_VR_FirstPersonBody gates the pose (cg_firstPersonBodyScale.value > 0);
// CG_VR_ShowFirstPersonBody gates the renderfx (!= 0) - the two conditions
// are NOT interchangeable (a negative scale still draws the normal body).
qboolean CG_VR_FirstPersonBody( centity_t *cent );
qboolean CG_VR_ShowFirstPersonBody( void );
void CG_VR_FirstPersonBodyAxes( refEntity_t *legs, refEntity_t *torso );
qboolean CG_VR_IsLocalClientEntity( const centity_t *cent );
qboolean CG_VR_SuppressOffHandItem( centity_t *cent );
qboolean CG_VR_OffHandItemPose( centity_t *cent, refEntity_t *ent, vec3_t offset, float scale, float horizontalScale );
void CG_VR_DrawOffHandHoldable( centity_t *cent );
qboolean CG_VR_OffHandCarriesFlag( centity_t *cent );
qboolean CG_VR_FollowedPlayerSprite( centity_t *cent );

// cg_players.c CG_PlayerAngles hooks: the EF_VR_PLAYER head/torso orientation
// regions. EF-gated inside, NOT vrActive-gated: flatscreen engines render VR
// players' heads too. PlayerHeadLerp caches the interpolated head pitch/yaw
// offset in per-entity module storage (formerly playerEntity_t.vrHead*);
// PlayerHeadAngles consumes it. TorsoYaw/TorsoPitch return qtrue when the VR
// arm ran, qfalse to run the stock swing path. PlayerHeadReset zeroes the
// storage (cg_snapshot.c's TV backward-seek entity wipe).
void CG_VR_PlayerHeadLerp( centity_t *cent );
qboolean CG_VR_PlayerTorsoYaw( centity_t *cent, const vec3_t headAngles, vec3_t torsoAngles );
qboolean CG_VR_PlayerTorsoPitch( centity_t *cent, float dest );
void CG_VR_PlayerSaveAbsoluteTorso( const centity_t *cent, const vec3_t torsoAngles, vec3_t absoluteTorsoAngles );
void CG_VR_PlayerHeadAngles( centity_t *cent, const vec3_t absoluteTorsoAngles, vec3_t headAngles );
void CG_VR_PlayerHeadReset( void );

// cg_ents.c: local VR player's HMD orientation feeding the predicted-player
// entity state (mirror/follow rendering).
void CG_VR_PredictedPlayerHead( void );

// cg_consolecmds.c/cg_main.c/cg_draw.c/cg_newdraw.c trinity-feature VR state:
// TV-scrub menu-yaw lock/restore, the spectator scoreboard cursor, hold-to-
// vote, and the scrub pointer's controller-yaw source. All dormancy-safe:
// writes no-op and reads return 0/0.0f/qfalse when !vrActive.
float    CG_VR_MenuYaw( void );
void     CG_VR_LockMenuYaw( float yaw );
void     CG_VR_UnlockMenuYaw( float yaw );
void     CG_VR_SetScoreboardCursor( qboolean active );
qboolean CG_VR_ScoreboardCursor( float *x, float *y );
void     CG_VR_SetVoteActive( qboolean active );
int      CG_VR_VoteHolding( void );
float    CG_VR_MenuPointerYaw( void );

// weapon adjustment mode
void CG_WeaponAdjust_f( void );
void CG_WeaponAdjust_Enter( void );
void CG_WeaponAdjust_Exit( void );
void CG_WeaponAdjustReset_f( void );
void CG_WeaponAdjustResetAll_f( void );
void CG_WeaponAdjustFrame( void );
void CG_WeaponAdjustDraw( void );

#endif
