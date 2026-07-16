# Trinity VR Integration Guide

This guide is for the author of a stock Quake III Arena 1.32 mod (baseq3 or
missionpack) who wants their mod to **fully work with VR clients**. That means
three things at once: the mod runs VR-aware under a Trinity VR engine, it renders
VR players correctly for everyone on the server, and it stays byte-for-byte
functional on an ordinary flatscreen engine. You start from your own working
source tree and end with a mod that does all three.

Those three things arrive as three layers, and you want **all of them**; this
is not a menu you pick from:

1. **The vendored VR drop**: a set of self-contained `vr_*` source modules you
   copy into your tree, plus a handful of one-line call-outs you add to your own
   functions. This is what turns the local client into a VR client, and it is
   the subject of this guide.
2. **The head-tracking protocol**: the wire format that lets VR players' head
   movement render for everyone. Its mod side ships inside the drop as vendored
   hooks; what you place is call-outs like everything else (Steps 4 and 5),
   plus one two-line stock addition at your stat enum's tail (Step 4), which
   the drop assumes is in place. The wire format, the bit encoding, and the
   engine-side patches are documented in the sibling document
   [`VR_PROTOCOL.md`](VR_PROTOCOL.md). (A mod that means to stay
   flatscreen-only takes the same path: vendor the drop, `vr_game` included,
   and place the same call-outs. The hooks key on `EF_VR_PLAYER`, not on a VR
   engine, so they run fine on flatscreen clients, and `vr_game`'s server-side
   decode is what flags VR players and feeds their head data, so the mod's
   players still see VR heads on a mixed server.)
3. **The cvar contract**: a body of engine-owned `vr_*` cvars the drop reads and
   writes by name. The drop speaks this contract by construction; you only meet
   it directly if you replace the VR settings screens. Appendix A enumerates it.

The rest of this guide walks the vendored drop (layer 1) in the order you would
actually do the work: files in, build wired, server side, client side, UI, then
verify. Read `VR_PROTOCOL.md` alongside it for layer 2's wire format and
engine side.

## How the call-outs behave

The call-outs you add are **unconditional**: never wrapped in `#ifdef` or a
runtime VR test. Every hook is written so that on an engine with no VR support it
does nothing: predicates return `qfalse` so your stock path runs, event hooks
no-op, and value transforms leave your numbers untouched. On a flatscreen engine
the shared-state mirror the modules carry stays zeroed and the whole drop is
inert. That is the property that lets one source tree serve both worlds. One
family is deliberately different: the head-tracking protocol hooks (Steps 4 and
5) stay live on a flatscreen engine, gated on `EF_VR_PLAYER` instead, so VR
players' heads render for everyone; they too do nothing until a VR player is in
view.

One rule protects that property: **never edit the vendored `vr_*` files.** Every
place you are meant to customize is a documented seam: where you place the
call-out in your own file, your own `CG_Draw2D` body, and the public accessors.
If you find yourself wanting to change a `vr_*` file, you are reaching past the
contract, and you forfeit clean updates when you take the next drop.

---

## Step 1: Copy the files in

Copy these modules into the matching directories of your tree (paths relative to
`code/`). What each one is:

- **`game/vr_shared.h`** and **`game/vr_safe_types.h`**: the mirror ABI
  (`vr_shared_t`) and the QVM-safe enums it uses. Included everywhere; hand-synced
  from the engine; do not edit. The mirror only ever grows at its tail with a
  `VR_API_VERSION` bump, and the engine rejects a module whose mirror size or
  version doesn't match, so a stale copy fails loudly rather than subtly.
- **`game/vr_bg.c` / `vr_bg.h`**: the shared movement and entity-state hooks
  (6DOF view-angle compensation, the dead-view predicate, the 6DOF physics
  mode, head-angle unpack) and the `EF_VR_PLAYER` definition. Named with the
  `bg_` prefix because, like the other `bg_*` files, it compiles into more
  than one module.
- **`game/vr_game.c` / `vr_game.h`**: the server side: bootstrap, the head-bit
  codec and its per-client head state, 6DOF aim/muzzle overrides, the
  config-block seed.
- **`cgame/vr_cgame.c` / `vr_cgame.h`**: the client side: bootstrap, the
  conformance probe, the view pipeline, the view weapon, other players' head
  rendering and the follow head view (with their head-stat interpolation and
  per-entity state), event hooks, the HUD, embodiment, and the cvar accessors.
- **`q3_ui/vr_ui.c` / `vr_ui.h`**: the baseq3 UI side (bootstrap, cursor/input
  hooks, the menu-scale transform, virtual-keyboard traps). baseq3 only.
- **`ui/vr_ui.c` / `vr_ui.h`**: the same contract, Team Arena implementation.
  missionpack only.
- **`ui/vr_uishared.c` / `vr_uishared.h`**: the shared VR screen transform
  (`UI_VR_AdjustFrom640`) and model-FOV compensation, in their own translation
  unit. Team Arena only, and it links into two targets (see Step 2).

You also need the VR settings screens. For baseq3 these are five C menu files
(`ui_vroptions.c`, the hub, plus `ui_vrcomfort.c`, `ui_vrcontrols.c`,
`ui_vrhud.c`, `ui_vrmirror.c`). For Team Arena they are `.menu` files driven by
two manifest lists (`vrmenus_pc.txt`, `vrmenus_quest.txt`). Copy whichever set
matches the UI you build. Step 7 explains why they are required and what a
replacement must reproduce.

---

## Step 2: Wire the build

Add the copied sources to your build's module lists.

| Source file | Compiles into |
|-------------|---------------|
| `vr_bg.c` | game **and** cgame **and** both UI targets (every module) |
| `vr_game.c` | game |
| `vr_cgame.c` | cgame |
| `q3_ui/vr_ui.c` | baseq3 UI |
| `ui/vr_ui.c` | missionpack UI |
| `ui/vr_uishared.c` | missionpack cgame **and** missionpack UI |
| baseq3 VR menu C files | baseq3 UI |

`vr_bg.c` lands in every module because the mirror and the shared movement hooks
are needed everywhere. `vr_shared.h` / `vr_safe_types.h` are headers; they carry
no compile line; just make them reachable on the include path of every module.

**The one dual-link case:** `vr_uishared.c` must compile into *both* the
missionpack cgame and the missionpack UI; it has to link everywhere
`ui_shared.c` does, because both links call its shared transform. This is the
only file that lands in two targets that don't otherwise share code.

Team Arena's `.menu` files and `vrmenus_*` manifests live in the missionpack
asset pak, which only loads under `fs_game missionpack`. A third-party `fs_game`
stacks on baseq3 and never sees them, so ship your own copies with your mod.

> **Build and check.** Compile now, before adding a single call-out. One stock
> addition is needed this early: the two stat enum entries from Step 4, which
> the vendored sources reference by name. With those in, the drop compiles and
> links cleanly with none of the call-outs placed; the modules are just dormant
> code. If it doesn't build here, it's a file-list or include-path problem, not
> an integration problem, and it's far cheaper to fix before the call-outs are
> in.

---

## Step 3: Know how a module wakes up

You will not write this code (it lives in the vendored files), but you should
recognize it, because it is why the call-outs are safe to place unconditionally.

Every VR-aware module finds the engine's extension interface the same way. The
engine advertises its extension entry point through a specially-named cvar,
`//trap_GetValue`. The module reads it as a string:

```c
trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
```

If the string is empty, the engine has no extension interface at all (a stock
1.32 engine, for example), and the module returns early and stays dormant.
Note that having the interface does not make an engine a VR engine: the broker
is a general extension mechanism, and flatscreen Trinity engines carry it too.
The string decodes to the address of a broker through which every VR extension
is resolved by name; there are no fixed VR syscall numbers anywhere in the
drop. What decides VR is the next step: the module asks the broker for
`trap_VR_RegisterState`, and only if that key resolves does it register its
copy of the mirror, stamped with a size and version so the engine can reject a
stale layout:

```c
vr_state.structSize = sizeof( vr_state );
vr_state.apiVersion = VR_API_VERSION;
trap_VR_RegisterState( &vr_state, sizeof( vr_state ), VR_API_VERSION );
```

`VR_API_VERSION` is `1` and the sentinel string is `"TRINITY_VR_API/1"`. The
dormancy signal is **registration**: a module that never resolved
`trap_VR_RegisterState` reports itself inactive (`G_VR_Active()` on the server,
the `vrActive` flag in the client and UI). Appendix B lists the individual trap
keys and which are optional.

---

## Step 4: Wire the server side (`vr_game`, `vr_bg`)

Start server-side; it is small and it is where the head data enters the mod.

**Bootstrap.** `G_VR_Init()` goes at the top of `G_InitGame`, alongside the
other init prints. It discovers the extension interface and seeds the config
block:

```diff
 	G_Printf ("------- Game Initialization -------\n");
 	G_Printf ("gamename: %s\n", GAMEVERSION);
 	G_Printf ("gamedate: %s\n", __DATE__);
+
+	G_VR_Init();
```

**Decode head data.** `G_VR_ClientThink` unpacks the head bits out of the button
field and sets or clears `EF_VR_PLAYER`. It is self-gating (a non-VR client
never sets those bits), so it needs no guard. Place it in `ClientThink_real`
after the button latch:

```diff
 	client->oldbuttons = client->buttons;
 	client->buttons = ucmd->buttons;
 	client->latched_buttons |= client->buttons & ~client->oldbuttons;
+
+	G_VR_ClientThink( client, ucmd );
```

The remaining server call-outs are one line each, at their obvious sites:

- `ClientEndFrame` (g_active.c) → `G_VR_ClientEndFrame( client, ent )` copies the
  head angles onto the entity and packs the head stats.
- `ClientUserinfoChanged` (g_client.c) → `G_VR_ClientIsVR( userinfo )`
  value-gates the `vr\` configstring field (it reads the value with `atoi`, not
  mere presence).
- `CalcMuzzlePointOrigin` (g_weapon.c) → `if ( !G_VR_MuzzlePoint( ent, ... ) )`
  wraps the stock muzzle math; the hook returns `qtrue` when it wrote a 6DOF
  muzzle.
- `FireWeapon` and `CheckGauntletAttack` (g_weapon.c) →
  `G_VR_AimAngles( ent, angles )` applies the 6DOF aim override when it returns
  `qtrue`.

One stock addition remains, because it cannot live in a vendored file: append
the two head stats at the tail of your `statIndex_t` enum in `bg_public.h`:

```c
	STAT_VR_HEAD_PITCH,				// VR head pitch angle (packed as short for demo playback)
	STAT_VR_HEAD_YAW_OFFSET,		// VR head yaw offset from weapon direction (packed as short)
```

The values are per-game, which is why no vendored header can carry them: the
stock enum has a conditional entry (`STAT_PERSISTANT_POWERUP` under
`MISSIONPACK`), so the same two names take different values in baseq3 and
missionpack builds. That is fine, because the engine never interprets stat
indices; the entire contract is that your game and cgame pack and unpack the
same slots, and one shared enum guarantees it. Appending at the tail is what
keeps the addition harmless across versions: an old demo or a version-skewed
client reads zeros out of the new slots, a missing feature, instead of
misreading some other stat. The vendored sources reference the two names, so
this addition is part of making the drop compile. Everything else the protocol
once asked of stock files ships in the drop: `EF_VR_PLAYER` is defined in
`vr_bg.h`, and the per-client head angles live in module state inside
`vr_game.c`.

**Shared movement (`vr_bg`).** The physics hooks sit in `bg_pmove.c`. Two of
them land in `PM_UpdateViewAngles`: `BG_VR_DeadViewLocked` replaces the stock
dead-player early-out (VR players keep looking around while dead; for everyone
else it is the stock test verbatim), and the view-angle compensation returns
`qtrue` to skip the stock viewangle update, placed just above the stock clamp
loop:

```diff
-	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
+	if ( BG_VR_DeadViewLocked( ps ) ) {
 		return;		// no view changes at all
 	}
 
+	if ( BG_VR_UpdateViewAngles( ps, cmd ) ) {
+		return;		// 6DOF client: YAW-only delta compensation applied
+	}
+
 	// circularly clamp the angles with deltas
 	for (i=0 ; i<3 ; i++) {
```

The other two `vr_bg` call-outs: in `PmoveSingle`, pass the physics mode through
`BG_VR_PmovePhysics( pm->ps, pm_mode )` (it returns the mode unchanged when
dormant, the 6DOF mode when active); and in `BG_PlayerStateToEntityState` (two
sites), call `BG_VR_HeadToEntityState( ps, s )` to derive the head orientation
for demos and first-person follow.

> **Build and check.** Rebuild game and cgame. Nothing visible changes yet on a
> flatscreen host (the protocol hooks idle until an entity carries
> `EF_VR_PLAYER`; every other hook is dormant), but a VR client connecting to a
> server running this game will now be flagged `EF_VR_PLAYER` and its head data
> will flow. You verify the visible result in Step 8's probe.

---

## Step 5: Wire the client side (`vr_cgame`)

This is the largest surface, but it decomposes into a bootstrap, one whole-frame
fork, one scaling keystone, the head-tracking protocol hooks, and then a long
list of one-line predicate and event call-outs that all follow the same shape.

**Bootstrap.** `CG_VR_Init()` at the top of `CG_Init` does trap discovery and
seeds the singleplayer/6DOF flags:

```diff
+	CG_VR_Init();
+
 	// load a few needed things before we do any screen updates
```

Pair it with `CG_VR_RegisterMedia()` in `CG_RegisterGraphics` (reticle and
HUD-sprite media, Appendix C), `CG_VR_Frame()` at the very top of
`CG_DrawActiveFrame` (per-frame mirror reads), and `CG_VR_Shutdown()` in
`CG_Shutdown`.

**The one whole-frame fork.** `CG_VR_DrawFrame` is the only place the drop takes
over an entire subsystem. It returns `qtrue` when it has fully drawn the VR frame
(scene submit plus 2D/HUD inside the post-bloom and HUD-buffer brackets) and the
caller returns; on a flatscreen host it returns `qfalse` and your stock tail
runs. Place it in `CG_DrawActive`, just before the stock "draw 3D view":

```diff
 	// clear around the rendered view if sized down
 	CG_TileClear();
+
+	if ( CG_VR_DrawFrame( stereoView ) ) {
+		return;
+	}
 
 	// draw 3D view
 	trap_R_RenderScene( &cg.refdef );
```

Everything *above* that line runs in VR too, so pre-scene additions belong there
as usual. Screen-space 2D belongs in `CG_Draw2D`, which the VR tail calls with
the correct bracketing, so your own HUD work in `CG_Draw2D` applies in VR
automatically, without touching a vendored file. The fork is kept whole because
its stateful bracketing (post-bloom, HUD-buffer, anchor push/pop, viewheight
fold/restore) is fragile enough that a host ordering mistake would break it.

**The scaling keystone.** `CG_VR_AdjustFrom640` carries the entire VR scaling
body (widescreen anchors, 4:3 fit, optical-center offset). It goes at the very
top of `CG_AdjustFrom640`; `qtrue` means it scaled the rect for VR and the caller
returns, `qfalse` runs the stock flatscreen scale:

```diff
 void CG_AdjustFrom640( float *x, float *y, float *w, float *h )
 {
+	if ( CG_VR_AdjustFrom640( x, y, w, h ) ) {
+		return;
+	}
 	// scale for screen sizes
```

**The head-tracking protocol hooks.** These are the mod side of the
head-tracking protocol (`VR_PROTOCOL.md`): they render other players' VR heads
and look through a followed VR player's head. Unlike the hooks above, they do
not key on VR registration; they gate on `EF_VR_PLAYER` (or the follow/demo
state) inside, because a flatscreen client on a mixed server renders VR
players' heads through these same call-outs. With no VR players in view they
do nothing, so they are just as safe to place unconditionally.

Five of them rebuild `CG_PlayerAngles` (cg_players.c) around the VR head.
Declare one local alongside the function's own:

```diff
 	int			dir, clientNum;
 	clientInfo_t	*ci;
+	vec3_t		absoluteTorsoAngles;
```

`CG_VR_PlayerHeadLerp` caches the entity's interpolated head orientation,
right after the angle setup:

```diff
 	VectorClear( legsAngles );
 	VectorClear( torsoAngles );
 
+	// VR player - interpolate head pitch and yaw offset (applied after AnglesSubtract below)
+	CG_VR_PlayerHeadLerp( cent );
+
 	// --------- yaw -------------
```

The two torso hooks wrap the stock swing arms; each returns `qtrue` when the
VR arm ran (the torso tracks weapon aim exactly, because the networked head
yaw is an offset from weapon aim), `qfalse` to run your stock swing path
unchanged:

```diff
 	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
-	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];
-
-	// torso
-	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
+	// VR: torso follows weapon aim 1:1 (no movement offset);
+	// flatscreen: torso turns 25% toward movement direction
+	if ( !CG_VR_PlayerTorsoYaw( cent, headAngles, torsoAngles ) ) {
+		torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];
+		CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
+	}
 	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
```

```diff
 	} else {
 		dest = headAngles[PITCH] * 0.75f;
 	}
-	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
+	// VR: torso follows weapon pitch directly; flatscreen uses swing tolerance
+	if ( !CG_VR_PlayerTorsoPitch( cent, dest ) ) {
+		CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
+	}
 	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
```

The last two bracket the hierarchy unwind: `CG_VR_PlayerSaveAbsoluteTorso`
snapshots the torso angles while they are still absolute, and
`CG_VR_PlayerHeadAngles` consumes the cache after `AnglesSubtract` has made
everything relative, remapping the world-space head pose into torso-local
angles under biological limits:

```diff
 	// pain twitch
 	CG_AddPainTwitch( cent, torsoAngles );
 
+	// Save absolute torso angles for VR head calculation
+	CG_VR_PlayerSaveAbsoluteTorso( cent, torsoAngles, absoluteTorsoAngles );
+
 	// pull the angles back out of the hierarchial chain
 	AnglesSubtract( headAngles, torsoAngles, headAngles );
 	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
+
+	// VR: compute head orientation relative to torso using matrix math
+	CG_VR_PlayerHeadAngles( cent, absoluteTorsoAngles, headAngles );
+
 	AnglesToAxis( legsAngles, legs );
 	AnglesToAxis( torsoAngles, torso );
```

`CG_VR_FollowHeadView` looks through a followed VR player's head instead of
the weapon, smoothing the 7-bit head stats with an EMA. It goes in
`CG_CalcViewValues` (cg_view.c) right after the view angles are copied in:

```diff
 	VectorCopy( ps->origin, cg.refdef.vieworg );
 	VectorCopy( ps->viewangles, cg.refdefViewAngles );
 
+	CG_VR_FollowHeadView( ps );
+
 	if (cg_cameraOrbit.integer) {
```

`CG_VR_InterpolateHeadStats` gives the head stats the same between-snapshot
treatment as viewangles, at the end of `CG_InterpolatePlayerState`
(cg_predict.c):

```diff
 		out->velocity[i] = prev->ps.velocity[i] +
 			f * (next->ps.velocity[i] - prev->ps.velocity[i] );
 	}
 
+	CG_VR_InterpolateHeadStats( out, &prev->ps, &next->ps, f );
+
 }
```

Two notes ride along. The hooks keep their own state (the per-entity head
cache, the follow EMA), so if your mod can jump its timeline (a demo seek, a
TV scrub), re-seed at the jump: `CG_VR_FollowHeadViewReset()` re-arms the
follow EMA and `CG_VR_PlayerHeadReset()` zeroes the per-entity cache; a mod
that never seeks has no call site for either. And the follow test is public as
`CG_VR_IsVRFollow()`, qtrue while following a VR player in first or third
person, for your own drawing that should behave differently then (this tree
uses it to aim the view weapon and a 3D crosshair along the followed player's
weapon direction rather than their gaze).

**Everything else** is a one-line call-out at its original site, all following
one of four shapes:

- **View, weapon, and embodiment gates.** The VR-only regions of the view code
  (`CG_VR_Fov`, `CG_VR_OffsetView`, `CG_VR_ViewAxis`, the intermission and
  menu-freeze regions, the weapon-angle compute) and the view-weapon hooks
  (`CG_VR_WeaponWheel`, `CG_VR_WeaponHandPose`, `CG_VR_HideViewWeapon`, …) are
  gate-for-hook swaps: stock `CG_CalcViewValues` and `CG_AddViewWeapon` now call
  *through* them, so your own view and weapon customizations stay intact. The
  embodiment hooks (`CG_VR_FirstPersonBody`, `CG_VR_OffHandItemPose`, …) do the
  same for `cg_players.c` / `cg_ents.c`.
- **Suppression predicates.** Each of `CG_VR_OwnsHudVisibility`,
  `CG_VR_Owns2DCrosshair`, `CG_VR_OwnsWeaponSelect`, `CG_VR_OwnsViewBob`,
  `CG_VR_SuppressDeadScoreboard`, and their siblings is a `!CG_VR_Owns...()` term
  you AND into an existing draw condition. Dormant, they return `qfalse`, so the
  term vanishes and your condition is unchanged.
- **Event hooks.** `CG_VR_OnFall`, `CG_VR_OnJump`, `CG_VR_OnTeleport`,
  `CG_VR_OnHitByMissile`, `CG_VR_OnWeaponFired`, and the rest wrap the haptic
  dispatch that used to sit inline at each game event. Dormant, they no-op.
- **Server-interaction accessors.** Trinity servers surface interactions a VR
  client joins through public accessors, and these engines record TVD, so a VR
  client on your mod will meet all of them; wire all three families. Voting:
  `CG_VR_SetVoteActive(active)` and `CG_VR_VoteHolding()` in the vote-draw path
  let a controller hold to confirm instead of needing a keyboard `F1`/`F2`.
  Spectating: `CG_VR_SetScoreboardCursor(active)` and
  `CG_VR_ScoreboardCursor(&x, &y)` feed the engine-computed pointer into the
  scoreboard so a spectator can click a player to follow. TV playback:
  `CG_VR_MenuYaw()`, `CG_VR_LockMenuYaw()`, `CG_VR_UnlockMenuYaw()`, and
  `CG_VR_MenuPointerYaw()` coordinate the scrub pointer's yaw with the virtual
  screen (console-command path, draw path, and shutdown unwind). All of them are
  accessors, never raw `vr->` reads, and all are dormancy-safe like every other
  call-out.

Two placement details are worth stating outright:

1. **Keep the weapon-adjust call out of `CG_VR_Frame`.** `CG_VR_Frame()` runs at
   the top of `CG_DrawActiveFrame`, *before* `CG_ProcessSnapshots()` refreshes
   `cg.snap`. The per-frame weapon-adjust step stays down in `CG_DrawActiveFrame`
   after `CG_ProcessSnapshots`, right before `CG_AddViewWeapon`, so it reads a
   fresh snapshot and its cvar writes still precede the same-frame weapon draw.
   Folding it into `CG_VR_Frame` would feed it a stale snapshot.
2. **Most event hooks keep their call-site identity gate.** A `void` hook like
   `CG_VR_OnFall(severity)` cannot re-derive which client the event belongs to,
   so it keeps the local-player gate that was already at the call site (for
   example `es->clientNum == cg.snap->ps.clientNum` around `CG_VR_OnDeath`). Only
   `CG_VR_OnTeleport(clientNum)` and `CG_VR_OnHitByMissile(entityNum)` receive an
   identity argument and re-derive the check inside the hook; those two are where
   you have a client number to filter on if you ever need per-client behavior.

---

## Step 6: Wire the UI for your game

The UI call-outs differ by which UI you build. Do the one that matches your mod;
the settings-screen requirement in Step 7 applies to both.

**baseq3 (`q3_ui`).** `UI_VR_Init()` at the top of `UI_Init`:

```diff
 void UI_Init( void ) {
+	UI_VR_Init();
+
 	UI_RegisterCvars();
```

The rest are one-liners at their sites: `UI_VR_Shutdown()` in `UI_Shutdown`;
`UI_VR_KeyEvent(key)` for first-chance virtual-keyboard keys in `UI_KeyEvent`;
the stick-nav / cursor-override / hover-haptic trio in `UI_MouseEvent`; and
`UI_VR_UpdateScale()` once per frame in `UI_Refresh`. The baseq3 variant
deliberately leaves stock `UI_AdjustFrom640` untouched and bakes the VR transform
into `uis.scale`/`biasX`/`biasY` in `UI_VR_UpdateScale` instead.

**Team Arena (`ui`).** Team Arena hooks `UI_AdjustFrom640` directly, at *two*
sites, one in `ui_atoms.c` and one in `ui_shared.c`, both routed through
`vr_uishared` so text and models transform identically in both links. The
`ui_atoms.c` site:

```diff
 void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
+	if ( UI_VR_AdjustFrom640( x, y, w, h ) ) {
+		return;
+	}
```

Team Arena's other call-outs live in `ui_main.c`: `UI_VR_Init()` /
`UI_VR_Shutdown()` in `_UI_Init` / `_UI_Shutdown`; the keyboard traps and the
hover haptic wired into the display context; `UI_VR_LoadMenus()` to load the
`vrmenus` manifest; and the two settings dispatchers
`UI_VR_UpdateSettingsCvar(name, val)` and `UI_VR_RunMenuScript(name)` in the
`Cvar_SetValue` and `RunMenuScript` handlers. The model-FOV compensation
(`UI_VR_CompensateModelFov`) is called from `Item_Model_Paint` in both links.

Appendix D lists the deliberate differences between the two UIs so you are not
surprised when the two `vr_ui.c` files do the "same" thing differently.

---

## Step 7: The VR settings screens

The settings screens are **required** for proper VR support, not optional
polish. Comfort and control options (turning style, control schema, thumbstick
swap, HUD mode, world scale, the desktop mirror) must be reachable from inside
the headset, and a mod's own `ui.qvm` replaces the engine's wholesale. If your
mod ships no VR settings UI, VR players have no settings UI at all; there is no
pak inheritance for compiled menus.

What is replaceable is the *implementation*. A mod with its own settings system
may present these options its own way, provided it does two things: covers the
full cvar surface in Appendix A, and reproduces the couplings that live behind
the screens. Those couplings are:

- **`vr_hudDrawStatus` → `cg_draw3dIcons`.** Setting HUD status to the value `2`
  (no status bar) also sets `cg_draw3dIcons 0`; **every other value** sets
  `cg_draw3dIcons 1`. Get this direction right: 2 disables, anything else
  enables.
- **`vr_uturn` ↔ `vr_controlSchema`.** These interlock; changing one recomputes
  the other so the button map stays consistent.
- **`vr_switchThumbsticks`.** A swap-in-place edit of the affected button
  mappings.
- **The desktop-mirror Apply.** `vr_desktopMode` together with
  `r_customdesktopwidth` / `r_customdesktopheight` are staged, then applied with a
  `vid_restart` on confirm, mirroring the menu's staged Apply.

The straightforward path is to take the shipped screens as-is; then the couplings
come along for free.

> **Build, run, and read the probe.** Set `cg_vrApiProbe 1` and connect a VR
> client. The probe draws an overlay at the top-left: the first line reads
> `VR API ACTIVE` (or `ABSENT` on a flatscreen host), and the second line is the
> sync round-trip check: it prints `PASS` while the engine is echoing the
> mirror back correctly each frame and `FAIL` if a sync is dropped. Below those,
> it prints live HMD position/orientation, weapon angles, FOV, thumbsticks, and
> the state flags. `ACTIVE` plus a steady `PASS` with sensible, moving numbers is
> the sign the drop is wired end-to-end. Turn the probe off with
> `cg_vrApiProbe 0`.

---

# Appendices (reference)

## Appendix A: The cvar contract

Alongside the `vr_shared_t` mirror, the drop reads and writes a set of
engine-owned `vr_*` cvars by name. This is a second, unversioned ABI: the
contract is the *names*. Single-value reads and writes from the vendored files go
through `CG_VR_CvarValue(name)` / `CG_VR_CvarSet(name, value)` (a string-buffer
idiom, because the QVM has no float-returning variable trap). A mod that rolls its
own settings UI still writes these names.

| Cvar(s) | Class | Role |
|---------|-------|------|
| `vr_worldscale`, `vr_worldscaleScaler` | archived | World scale; read each frame in `CG_VR_Frame` and server-side in `vr_game.c` |
| `vr_hudDrawStatus`, `vr_hudDepth`, `vr_hudScale`, `vr_hudYOffset` | archived | HUD visibility preference and in-world placement |
| `vr_currentHudDrawStatus`, `vr_currentHudDepth` | transient | Written by `CG_VR_Frame` for the renderer to read |
| `vr_thirdPersonSpectator` | transient | Written each frame so the renderer drops sky in spectator views |
| `vr_platform` | archived | `pc` / `quest`; read **only** through `UI_VR_Platform()` (see below) |
| `vr_6dof` | archived | Seeds `use_6dof` (singleplayer only) |
| `vr_desktopMode` | archived | Desktop-mirror mode, staged/applied by the menu scripts |
| `vr_lasersight`, `vr_twoHandedWeapons`, `vr_showItemInHand`, `vr_rollWhenHit`, `vr_weaponAdjust`, `vr_weaponSelectorMode`, `vr_weaponSelectorWithHud` | archived | Gameplay/comfort toggles read by the client hooks |
| `vr_uturn`, `vr_controlSchema`, `vr_switchThumbsticks` | archived | Control-scheme handlers (see Step 7) |
| `vr_button_map_*` family | archived | Per-button remaps: `A`, `B`, `X`, `Y`, `PRIMARYGRIP`, `PRIMARYTHUMBSTICK`, and the `RTHUMB{FORWARD,BACK,LEFT,RIGHT}` set with their `_ALT` variants |

There is no `vr_stabilised` cvar; weapon stabilization is a mirror flag, not a
cvar.

**Menu content must never read `vr_platform` raw.** It is an archived cvar, so a
user who once ran a VR build and later launches a flatscreen build still has
`vr_platform "pc"` on disk. Menu code keys VR rows on `UI_VR_Platform()` instead,
which returns `VRP_NONE` whenever the mirror is dormant regardless of the cvar,
and otherwise maps the cvar string to `VRP_PC` / `VRP_QUEST`:

```c
vrPlatform_t UI_VR_Platform( void ) {
    char buf[16];
    if ( !vrActive )
        return VRP_NONE;
    trap_Cvar_VariableStringBuffer( "vr_platform", buf, sizeof( buf ) );
    if ( !Q_stricmp( buf, "pc" ) )    return VRP_PC;
    if ( !Q_stricmp( buf, "quest" ) ) return VRP_QUEST;
    return VRP_NONE;
}
```

Requiring live registration first is what keeps VR-only menu rows off a
flatscreen host. Team Arena's `UI_VR_LoadMenus()` is double-gated the same way.

## Appendix B: Bootstrap trap keys

`trap_GetValue(value, valueSize, key)` returns non-zero and writes the resolved
address into `value` when the engine supports `key`.

| Key | Modules | Required? | Capability on success |
|-----|---------|-----------|-----------------------|
| `trap_VR_RegisterState` | all | **Required** (no key, no VR) | sets `vrActive` / `g_vrActive` |
| `trap_R_BeginPostBloom2D` / `trap_R_EndPostBloom2D` | cgame | optional | `hasPostBloom2D` (needs both) |
| `trap_R_HUDBufferStart` / `trap_R_HUDBufferEnd` | cgame | optional | `hasHUDBuffer` (needs both) |
| `trap_HapticEvent` | cgame, both UI | optional | `hasHapticEvent` |
| `trap_VKeyboard_Show` / `_Hide` / `_IsActive` / `_HandleKey` | both UI | optional | `hasVKeyboard` (needs **all four**) |

The paired and grouped keys are all-or-nothing: a partial keyboard interface is
treated as no keyboard, and one HUD-buffer key without the other is treated as no
HUD buffer. A feature whose trap is unresolved simply never fires.

## Appendix C: Asset dependencies

Each asset degrades cleanly when absent: a missing asset drops its feature, it
never crashes.

| Asset | Used by | Without it |
|-------|---------|-----------|
| `gfx/weapon/scope` | `CG_VR_RegisterMedia`, zoom scope mask | Zoom still works, no reticle |
| `sprites/vr/hud` | `CG_VR_RegisterMedia`, in-world HUD panel (HUD mode 1) | Mode 1 also needs `hasHUDBuffer`; HUD mode 2 unaffected |
| laser-sight beam | drawn via `CG_LaserSight`, gated by `vr_lasersight` | No beam renders |
| VR menu files / manifests | the settings UI | Console-only settings (every toggle is still a cvar) |

Those two shaders are the only assets `CG_VR_RegisterMedia` registers directly;
the laser beam draws through the stock `CG_LaserSight` helper. Comfort vignettes
are cvar-driven (`vr_comfortVignette`) and rendered engine-side.

## Appendix D: Differences between the two UIs

Deliberate differences, noted so the two `vr_ui.c` files don't surprise you:

- **Virtual-keyboard key routing.** baseq3 intercepts in `UI_KeyEvent` via
  `UI_VR_KeyEvent(key)`. Team Arena intercepts deeper, inside `Menu_HandleKey`'s
  edit-field path, because the `.menu` parser owns key dispatch. Same behavior,
  different insertion point.
- **On-screen model transform.** baseq3 bakes the VR scale into
  `uis.scale/biasX/biasY` once per frame and leaves `UI_AdjustFrom640` stock.
  Team Arena hooks `UI_AdjustFrom640` directly, at two sites unified through
  `vr_uishared`.
- **Event-hook identity.** Only `CG_VR_OnTeleport` and `CG_VR_OnHitByMissile`
  carry a client/entity number; the others rely on the local-player gate already
  at their call site (Step 5, note 2).

