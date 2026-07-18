# Trinity VR Integration Guide

This guide is for the author of a stock Quake III Arena 1.32 mod (baseq3 or
missionpack) who wants the mod to **fully work with VR clients**: VR-aware
under a Trinity VR engine, VR players rendered correctly for everyone on the
server, and byte-for-byte functional on an ordinary flatscreen engine.

Three layers deliver that, and you need all of them:

1. **The vendored VR drop**: self-contained `vr_*` modules you copy in, plus
   one-line call-outs in your own functions. The subject of this guide.
2. **The head-tracking protocol**: the wire format that renders VR players'
   head movement for everyone. Its mod side ships inside the drop; you place
   call-outs (Steps 4 and 5) plus one two-line stat-enum addition (Step 4).
   Wire format and engine side: [`VR_PROTOCOL.md`](VR_PROTOCOL.md). A mod
   that means to stay flatscreen-only takes the same path — the hooks key on
   `EF_VR_PLAYER`, not on a VR engine, so its players still see VR heads on a
   mixed server.
3. **The cvar contract**: engine-owned `vr_*` cvars the drop reads and writes
   by name. The drop meets it by construction; you meet it directly only if
   you replace the VR settings screens (Appendix A).

One word is used precisely throughout: **the host** is your mod tree — the
code that hosts the vendored drop. Its obligations are declared in
`vr_host.h` and configured in `vr_host_config.h` (Step 1). The program that
loads your QVMs is always the *engine*, never the host.

## How the call-outs behave

The call-outs you add are **unconditional** — never wrapped in `#ifdef` or a
runtime VR test. Every hook no-ops on an engine with no VR support:
predicates return `qfalse` so your stock path runs, event hooks do nothing,
value transforms return your value untouched. One family is deliberately
different: the head-tracking protocol hooks (Steps 4 and 5) gate on
`EF_VR_PLAYER` instead, so VR players' heads render for everyone even on a
flatscreen engine; with no VR player in view they too do nothing.

A short list of placements is the sanctioned exception: they read the
`vrActive` dormancy signal directly, because an unconditional call there
would double-draw in VR or visibly change flatscreen rendering. They are
the lightning-gun bolt origin, the flatscreen follow crosshair, and the
flatscreen probe draw (Step 5); Team Arena's `ui_shared.c` transform fork
and the connect-screen background fill in both UIs (Step 6). Each is
flagged at its site; every other call-out stays unconditional.

One rule protects this: **never edit the vendored `vr_*` files.** Every
customization point is a documented seam — call-out placement, your own
`CG_Draw2D` body, the public accessors. Editing a `vr_*` file forfeits clean
updates when you take the next drop.

---

## Step 1: Copy the files in

baseq3 is the expected base throughout this guide; the `ui/` modules and the
missionpack halves of every step apply only if your mod builds missionpack
code. Copy these modules into the matching directories of your tree (paths
relative to `code/`):

- **`game/vr_shared.h`** / **`game/vr_safe_types.h`**: the mirror ABI
  (`vr_shared_t`) and its QVM-safe enums. Hand-synced from the engine; do not
  edit. The mirror grows only at its tail with a `VR_API_MINOR` bump (layout
  changes bump `VR_API_MAJOR`), and the engine rejects a size or version it
  cannot meet — but the gate catches skew only; a hand-reordered layout under
  an unchanged version is exactly what the never-edit rule and the Step 7
  probe exist to catch.
- **`game/vr_trap.h`**: the `VR_RESOLVE` binding macro the bootstraps use to
  look traps up by name (Step 3).
- **`cgame/vr_host.h`**: the host contract — everything the drop consumes
  from your tree beyond stock 1.32, declared in one place (Appendix E walks
  it). If your tree satisfies this header, the drop compiles.
- **`cgame/vr_host_config.h`**: the **one file in the drop you are meant to
  edit**. Declares which optional host features exist (`VR_HOST_HAS_TV`,
  `VR_HOST_HAS_WARMUP_EVENTS` — a stock tree sets both to 0), maps small
  contract gaps (`#define Q_sscanf sscanf`), and may set the weapon wheel's
  default set (`VR_WHEEL_DEFAULT_WEAPONS`). Its tokens are
  whitespace-separated — commas are **not** separators. A token is a
  `weapon_t` number or an item classname with or without the `weapon_`
  prefix; `*` expands the default set; `!name` hides a weapon from later
  mentions and expansions; first mention wins. A list containing only
  exclusions means the default set minus those weapons.
- **`game/vr_platform.c` / `.h`**: the platform query (`VR_Platform`; the
  UI modules wrap it as `UI_VR_Platform` — Appendix A). Compiles into cgame
  and both UI targets, not game.
- **`game/vr_bg.c` / `.h`**: shared movement and entity-state hooks, and the
  `EF_VR_PLAYER` definition. Like the other `bg_*` files, it compiles into
  more than one module.
- **`game/vr_game.c` / `.h`**: the server side — bootstrap, head-bit codec,
  6DOF aim/muzzle overrides, config-block seed.
- **`cgame/vr_cgame.c` / `.h`**: the client side — bootstrap, conformance
  probe, view pipeline, view weapon, other players' head rendering, follow
  head view, event hooks, HUD, embodiment, cvar accessors.
- **`q3_ui/vr_ui.c` / `.h`**: the baseq3 UI side (bootstrap, cursor/input
  hooks, menu-scale transform, virtual-keyboard traps).
- **`ui/vr_ui.c` / `.h`**: the same contract, Team Arena implementation.
- **`ui/vr_uishared.c` / `.h`**: the shared VR screen transform and model-FOV
  compensation. Team Arena only; links into two targets (Step 2).

You also need the VR settings screens, for each UI you build. baseq3: five C
menu files (`ui_vroptions.c`, the hub, plus `ui_vrcomfort.c`,
`ui_vrcontrols.c`, `ui_vrhud.c`, `ui_vrmirror.c`); one build serves both
platforms. Team Arena: the VR `.menu` files together with **both** manifests
(`vrmenus_pc.txt`, `vrmenus_quest.txt`) — bundle the pc and quest variants
both, because the same `ui.qvm` may be loaded by either engine and
`UI_VR_LoadMenus` picks the manifest at runtime from the platform. Step 7
explains why the screens are required and what a replacement must reproduce.

---

## Step 2: Wire the build

Add the copied sources to your build's module lists.

| Source file | Compiles into |
|-------------|---------------|
| `vr_bg.c` | game **and** cgame **and** both UI targets (every module) |
| `vr_game.c` | game |
| `vr_cgame.c` | cgame |
| `vr_platform.c` | cgame **and** both UI targets (not game) |
| `q3_ui/vr_ui.c` | baseq3 UI |
| `ui/vr_ui.c` | missionpack UI |
| `ui/vr_uishared.c` | missionpack cgame **and** missionpack UI |
| baseq3 VR menu C files | baseq3 UI |

The five contract headers carry no compile line; make them reachable on every
module's include path (this tree includes the host pair from the tail of
`cg_local.h`, just before `vr_cgame.h`).

**The one dual-link case:** `vr_uishared.c` must compile into *both* the
missionpack cgame and the missionpack UI — it links everywhere `ui_shared.c`
does, because both links call its shared transform.

If you build the missionpack UI: its `.menu` files and `vrmenus_*` manifests
are runtime assets. This tree ships them in the missionpack asset pak, which
only loads under `fs_game missionpack`; a mod under its own `fs_game` stacks
on baseq3, so ship the files in your own pak.

> **Build and check.** Compile now, before any call-out. Two stock additions
> are needed this early: the two stat enum entries (Step 4), which the
> vendored sources reference by name, and the `vr_host.h` contract
> (Appendix E). Work through Appendix E first; your first `make` is the
> honest inventory of what remains. Once the drop builds with no call-outs
> placed, the modules are dormant code, and any remaining failure is a
> file-list or include-path problem.

---

## Building the QVMs

Neither VR engine ships a QVM compiler. trinity-vr and trinity-quest load
QVMs; they never produce them — your mod tree is the only source of QVMs,
and this section is about the toolchain that turns it into them.

**What this tree uses.** A vendored lcc 4.2 (`tools/q3lcc`, built as
`q3lcc` / `q3cpp` / `q3rcc`) compiles each module's C into VM bytecode, and
a fast q3asm fork with dead-code elimination and `.jts` jump-table support
links it. Both live under `tools/` as submodules (`git submodule update
--init` after cloning) and build with the host compiler. From the repo
root, `make` builds the tools and then both paks: `dist/pak8t.pk3`
(baseq3) and `dist/pak3t.pk3` (missionpack). The whole chain runs on a
stock Ubuntu box — CI proves it with nothing beyond `build-essential` and
`p7zip-full`. Stock ioq3's `code/tools` also works if you already build
QVMs there; nothing in the drop requires this tree's forks. Adopting the
forks carries one source-tree obligation: this q3asm scopes `static`
symbols per file, so take this tree's `q_math.c` treatment too — the
`Q3_VM` vector math defined out of line there, replacing stock's
`__Q3_VM_MATH` header inlines — or every module's vector-math externs go
unresolved at q3asm link time.

**Gotchas.** Four ways QVM builds bite people who are used to native
ones. q3lcc is a C90 compiler with its own preprocessor, and that
preprocessor can silently diverge from your native compiler's — the same
translation unit can preprocess differently in ways no error reveals.
Always test the QVM and native builds both; the dual-build design exists
partly to force this. Missing runtime support symbols (a libc function
`bg_lib` doesn't provide, a helper that didn't make the file list) surface
at q3asm link time, not compile time — every `.asm` compiles clean and the
failure arrives at the very end, so a clean per-file compile proves
nothing about the link. q3lcc's preprocessor resolves `-I` paths relative
to the main source file's directory, not your make cwd — write include
flags relative to `code/<module>/`. And modern engines JIT their QVMs:
undefined behavior the old interpreter shrugged off for years can crash or
miscompile under a JIT — treat interpreter-era code as suspect, not
proven.

---

## Step 3: Know how a module wakes up

You will not write this code (it lives in the vendored files), but you should
recognize it, because it is why the call-outs are safe to place
unconditionally.

The engine advertises its extension entry point through a specially-named
cvar, `//trap_GetValue`. Each module reads it as a string:

```c
trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
```

If the string is empty, the engine has no extension interface (a stock 1.32
engine) and the module stays dormant. Having the interface does not make an
engine a VR engine — flatscreen Trinity engines carry the broker too, and
every VR extension is resolved from it by name; there are no fixed VR syscall
numbers anywhere in the drop. What decides VR is the next step: the module
asks the broker for `trap_VR_RegisterState`, and only if that key resolves
does it register its copy of the mirror:

```c
vr_state.structSize = sizeof( vr_state );
vr_state.apiVersion = VR_API_MAJOR;
trap_VR_RegisterState( &vr_state, sizeof( vr_state ), VR_API_MAJOR, VR_API_MINOR );
```

Registration advertises the major.minor pair the module was compiled against
(`VR_API_MAJOR` / `VR_API_MINOR`, currently `1.0`). The engine runs a module
whose major matches and whose minor it can meet or exceed, and refuses
anything else with `VR API incompatible`. The same pair builds the sentinel
string `"TRINITY_VR_API/1.0"`, embedded in each module as the
`vr_api_sentinel[]` array; the engine finds it by scanning the raw `.qvm`
image, so a module built from the drop carries it automatically. A
hand-rolled bootstrap must keep the array *and* its self-reference (which
stops dead-stripping) or the engine treats the QVM as flatscreen-only.
Native builds are never sentinel-scanned, so their registration is the only
version gate — advertise the pair you compiled against. The dormancy signal
is **registration**: a module that never resolved `trap_VR_RegisterState`
reports itself inactive (`G_VR_Active()` server-side, `vrActive` in client
and UI). Appendix B lists the trap keys.

Two consequences before you first test. The engine gates each module
**independently**, and a QVM with no sentinel is not an error — it is
skipped with one console line and the engine falls back to its bundled
native module, so forgetting the drop in one module leaves the other two
running alongside someone else's rules. All three modules carry the drop.
And the mirror's write rules are enforced by the engine, not the compiler:
each sync commits back only the blocks your module owns (the block comments
in `vr_shared.h`), so a write to an engine-owned field compiles fine and is
silently reverted at the next sync-in. Code that wants to change engine
state goes through cvars or traps, never the mirror.

---

## Step 4: Wire the server side (`vr_game`, `vr_bg`)

Start server-side; it is small and it is where the head data enters the mod.

**Bootstrap.** `G_VR_Init()` at the top of `G_InitGame`, alongside the other
init prints:

```diff
 	G_Printf ("------- Game Initialization -------\n");
 	G_Printf ("gamename: %s\n", GAMEVERSION);
 	G_Printf ("gamedate: %s\n", __DATE__);
+
+	G_VR_Init();
```

**Decode head data.** `G_VR_ClientThink` unpacks the head bits out of the
button field and sets or clears `EF_VR_PLAYER`. Self-gating (a non-VR client
never sets those bits); place it in `ClientThink_real` after the button
latch:

```diff
 	client->oldbuttons = client->buttons;
 	client->buttons = ucmd->buttons;
 	client->latched_buttons |= client->buttons & ~client->oldbuttons;
+
+	G_VR_ClientThink( client, ucmd );
```

The remaining server call-outs are one line each, at their obvious sites:

- `ClientEndFrame` (g_active.c) → `G_VR_ClientEndFrame( client, ent )` copies
  the head angles onto the entity and packs the head stats.
- `ClientUserinfoChanged` (g_client.c) → append `\vr\%s` with
  `G_VR_ClientIsVR( userinfo ) ? "1" : "0"` to the **player** configstring
  format only. Bots and vanilla servers simply omit the key; every reader
  treats a missing key as not-VR.
- `CalcMuzzlePointOrigin` (g_weapon.c) → `if ( !G_VR_MuzzlePoint( ent, ... ) )`
  wraps the stock muzzle math; `qtrue` means it wrote a 6DOF muzzle.
- `FireWeapon` and `CheckGauntletAttack` (g_weapon.c) →
  `G_VR_AimAngles( ent, angles )` applies the 6DOF aim override when it
  returns `qtrue`.

**Who is in VR.** The `vr\` field gives every client the answer, and the
drop wraps the read: `CG_VR_ClientIsVR( clientNum )` in cgame reports what
the server recorded, no clientinfo parsing required. Surfacing it is
recommended: VR players aim and move differently, and a marker on the
scoreboard or HUD tells everyone else what they are looking at. This tree
parses the flag once into clientinfo through `CG_VR_ClientIsVR`
(cg_players.c, `CG_NewClientInfo`) and draws the scoreboard and follow-bar
icons from the cached `ci->vrPlayer`; a host without clientinfo caching
can call the accessor directly at its draw sites.

One stock addition remains, because it cannot live in a vendored file: append
the two head stats at the tail of your `statIndex_t` enum in `bg_public.h`:

```c
	STAT_VR_HEAD_PITCH,				// VR head pitch angle (packed as short for demo playback)
	STAT_VR_HEAD_YAW_OFFSET,		// VR head yaw offset from weapon direction (packed as short)
```

The values are per-game — the stock enum has a `MISSIONPACK`-conditional
entry, so the same names take different values in baseq3 and missionpack
builds. That is fine: the engine never interprets stat indices; the contract
is that game and cgame share one enum. Tail-appending is what keeps the
addition harmless across versions — old demos and version-skewed clients
read zeros out of the new slots (a missing feature) instead of misreading
another stat. Two cautions: "tail" means append-only, so entries your mod
adds later go after these two and inherit the same property; and `ps->stats`
caps at 16 slots (`MAX_STATS`), so count before you append. Everything else
the protocol needs ships in the drop (`EF_VR_PLAYER` in `vr_bg.h`,
per-client head state in `vr_game.c`).

**Shared movement (`vr_bg`).** The physics hooks sit in `bg_pmove.c`. Two
land in `PM_UpdateViewAngles`: `BG_VR_DeadViewLocked` replaces the stock
dead-player early-out (VR players keep looking around while dead; for
everyone else it is the stock test verbatim), and the view-angle
compensation returns `qtrue` to skip the stock viewangle update:

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

Run your ground friction and acceleration through the two value transforms
— `BG_VR_PmoveFriction( pm->ps, friction )` and
`BG_VR_PmoveAccelerate( pm->ps, accelerate )` — wherever your movement
constants are consumed (stock: the `pm_friction` use in `PM_Friction` and
the ground `pm_accelerate` use in `PM_WalkMove`). Dormant they return your
value untouched; for the 6DOF client they return the tracking coefficients.

In `PM_Footsteps`, keep stock's not-trying-to-move branch untouched and add
a VR-only clause after it:

```c
	// VR players: analog sticks and roomscale drift produce tiny velocities
	// that must not advance the walk cycle
	if ( BG_VR_DriftIdle( pm->ps ) && xyspeed < 10 ) {
		pm->ps->bobCycle = 0;	// start at beginning of cycle again
		if ( pm->ps->pm_flags & PMF_DUCKED ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		} else {
			PM_ContinueLegsAnim( LEGS_IDLE );
		}
		return;
	}
```

`BG_VR_DriftIdle` keys on `EF_VR_PLAYER`, so flatscreen behavior stays
byte-for-byte stock and server and prediction agree per player.

And in `BG_PlayerStateToEntityState` and
`BG_PlayerStateToEntityStateExtraPolate` (one call each, after the
`s->angles2[YAW]` assignment), call `BG_VR_HeadToEntityState( ps, s )` to
derive the head orientation for demos and first-person follow.

> **Build and check.** Rebuild game and cgame. Nothing visible changes on a
> flatscreen engine, but a VR client connecting to a server running this
> game is now flagged `EF_VR_PLAYER` and its head data flows. The visible
> result is verified by Step 7's probe.

---

## Step 5: Wire the client side (`vr_cgame`)

The largest surface, but it decomposes into a bootstrap, one whole-frame
fork, one scaling keystone, the head-tracking protocol hooks, and a long
list of one-line call-outs in five shapes.

**Bootstrap.** `CG_VR_Init()` at the top of `CG_Init`:

```diff
+	CG_VR_Init();
+
 	// load a few needed things before we do any screen updates
```

Pair it with `CG_VR_RegisterMedia()` in `CG_RegisterGraphics`,
`CG_VR_Frame()` near the top of `CG_DrawActiveFrame` — after
`CG_UpdateCvars()`, before the loading-screen early-return and
`CG_ProcessSnapshots()` (see placement note 1) — and `CG_VR_Shutdown()` in
`CG_Shutdown`.

**Console commands.** The engine drives the weapon selector and the
weapon-adjust tool through cgame console commands; register the four drop
handlers in your `cg_consolecmds.c` command table:

```c
	{ "weapon_select", CG_WeaponSelectorSelect_f },
	{ "weapon_adjust", CG_WeaponAdjust_f },
	{ "weapon_adjust_reset", CG_WeaponAdjustReset_f },
	{ "weapon_adjust_reset_all", CG_WeaponAdjustResetAll_f },
```

**The one whole-frame fork.** `CG_VR_DrawFrame` is the only place the drop
takes over an entire subsystem: `qtrue` means it fully drew the VR frame and
the caller returns; on a flatscreen engine it returns `qfalse` and your
stock tail runs. Place it in `CG_DrawActive`, immediately after
`CG_TileClear()` and before the stock stereo-crosshair block:

```diff
 	// clear around the rendered view if sized down
 	CG_TileClear();
+
+	if ( CG_VR_DrawFrame( stereoView ) ) {
+		return;
+	}
 
 	if(stereoView != STEREO_CENTER)
 		CG_DrawCrosshair3D();
```

Everything *above* the fork runs in VR too, so pre-scene additions go there
as usual; screen-space 2D belongs in `CG_Draw2D`, which the VR tail calls
with the correct bracketing, so your own HUD work applies in VR without
touching a vendored file. The fork is kept whole because its stateful
bracketing (post-bloom, HUD-buffer, anchor push/pop, viewheight
fold/restore) would break under a host ordering mistake.

**The scaling keystone.** `CG_VR_AdjustFrom640` carries the entire VR
scaling body. At the very top of `CG_AdjustFrom640`; `qtrue` means it scaled
the rect for VR, `qfalse` runs the stock scale:

```diff
 void CG_AdjustFrom640( float *x, float *y, float *w, float *h )
 {
+	if ( CG_VR_AdjustFrom640( x, y, w, h ) ) {
+		return;
+	}
 	// scale for screen sizes
```

**The head-tracking protocol hooks.** The mod side of `VR_PROTOCOL.md`: they
render other players' VR heads and look through a followed VR player's head.
They gate on `EF_VR_PLAYER` (or follow/demo state) rather than VR
registration, because a flatscreen client on a mixed server renders VR heads
through these same call-outs; with no VR players in view they do nothing.

Five of them rebuild `CG_PlayerAngles` (cg_players.c) around the VR head.
Declare one local alongside the function's own:

```diff
 	int			dir, clientNum;
 	clientInfo_t	*ci;
+	vec3_t		absoluteTorsoAngles;
```

`CG_VR_PlayerHeadLerp` caches the entity's interpolated head orientation:

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
yaw is an offset from weapon aim):

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
`CG_VR_PlayerHeadAngles` consumes the cache after `AnglesSubtract`,
remapping the world-space head pose into torso-local angles under biological
limits:

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

`CG_VR_FollowHeadView` looks through a followed VR player's head, smoothing
the 7-bit head stats with an EMA. In `CG_CalcViewValues` (cg_view.c) right
after the view angles are copied in:

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

`CG_VR_PredictedPlayerHead` feeds the local player's HMD orientation into
the predicted player entity, so mirror and follow rendering see your own
head. In `CG_AddPacketEntities` (cg_ents.c), immediately after the
predicted player's `BG_PlayerStateToEntityState` call:

```diff
 	// generate and add the entity from the playerstate
 	ps = &cg.predictedPlayerState;
 	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
+
+	// Set VR head orientation for local player (mirrors) - see vr_cgame.c
+	CG_VR_PredictedPlayerHead();
+
 	CG_AddCEntity( &cg.predictedPlayerEntity );
```

The hooks keep their own state (per-entity head cache, follow EMA); if your
mod can jump its timeline (demo seek, TV scrub), re-seed at the jump with
`CG_VR_FollowHeadViewReset()` and `CG_VR_PlayerHeadReset()` — a mod that
never seeks has no call site for either. The follow test is public as
`CG_VR_IsVRFollow()`, qtrue while following a VR player, for your own
drawing that should behave differently then (this tree aims the view weapon
and 3D crosshair along the followed player's weapon direction). One of its
placements is a sanctioned `vrActive` gate: the flatscreen follow crosshair
in `CG_DrawActiveFrame` —
`if ( !vrActive && !cg.renderingThirdPerson && CG_VR_IsVRFollow() )
CG_DrawCrosshair3D();` after `CG_AddViewWeapon` — because in VR the draw
tail's crosshair site already covers follow mode, and an unconditional call
would double-blend.

**Everything else** is a one-line call-out at its original site, in one of
five shapes:

- **View, weapon, and embodiment gates.** The VR-only regions of the view
  code (`CG_VR_Fov`, `CG_VR_OffsetView`, `CG_VR_ViewAxis`, the intermission
  and menu-freeze regions, the weapon-angle compute), the view-weapon hooks
  (`CG_VR_WeaponWheel`, `CG_VR_WeaponHandPose`, `CG_VR_HideViewWeapon`, …),
  and the embodiment hooks (`CG_VR_FirstPersonBody`, `CG_VR_OffHandItemPose`,
  …) are gate-for-hook swaps: stock code calls *through* them, so your own
  customizations stay intact. The same shape covers the scattered
  aim and orientation call-outs: `CG_VR_CrosshairScanRay` (the
  `CG_ScanForCrosshairEntity` aim ray), `CG_VR_WeaponMuzzleOrigin` (the
  `EV_RAILTRAIL` muzzle), `CG_VR_FollowedPlayerSprite` (the followed-player marker in
  `CG_PlayerSprites`), `CG_VR_ForceThirdPerson` (forces
  `cg.renderingThirdPerson` in `CG_DrawActiveFrame`), and the value reads
  `CG_VR_DamageRollScale` (the `CG_OffsetFirstPersonView` damage-kick roll)
  and `CG_VR_ChatOffsetX/Y` (the missionpack chat paints in cg_newdraw.c).
- **Suppression predicates.** `CG_VR_OwnsHudVisibility`,
  `CG_VR_Owns2DCrosshair`, `CG_VR_OwnsWeaponSelect`, `CG_VR_OwnsViewBob`,
  `CG_VR_SuppressDeadScoreboard`, and their siblings are `!CG_VR_Owns...()`
  terms you AND into an existing draw condition. Dormant they return
  `qfalse` and your condition is unchanged. `CG_VR_IsDeathCam` takes the
  same shape as an OR-in early-out in `CG_DrawCrosshair3D`. The missionpack
  HUD adds a positive twin, `CG_VR_HudVisible()` — the `vr_hudDrawStatus`
  ladder's visibility answer, qtrue when dormant — at three sites:
  `CG_OwnerDraw` and `CG_Draw2DMinimal` early-return when it is qfalse, and
  `CG_Draw2D` composes it into the `Menu_PaintAll` condition,
  `CG_VR_HudVisible() && ( CG_VR_OwnsHudVisibility() || cg_drawStatus.integer )`.
- **Event hooks.** `CG_VR_OnFall`, `CG_VR_OnJump`, `CG_VR_OnTeleport`,
  `CG_VR_OnHitByMissile`, `CG_VR_OnWeaponFired`, and the rest wrap the
  haptic dispatch at each game event. Dormant they no-op.
- **Server-interaction accessors.** Wire the families your tree has
  features for. Voting (stock): `CG_VR_SetVoteActive(active)` and
  `CG_VR_VoteHolding()` in the vote-draw path (controller hold-to-confirm).
  The hold machinery itself is host code: take this tree's
  `CG_ProcessVoteHold` (cg_draw.c) — it reads `CG_VR_VoteHolding()` each
  frame, runs the hold timer, and dispatches the vote/teamvote commands —
  and call it beside `CG_VR_SetVoteActive` from the vote-overlay block
  `CG_Draw2D` reaches (this tree: `CG_DrawTVOverlay`, after
  `CG_DrawVote`/`CG_DrawTeamVote`). Spectating: the scoreboard-cursor pair
  — `CG_VR_SetScoreboardCursor(active)` arms the engine pointer,
  `CG_VR_ScoreboardCursor(&x, &y)` reads it back into the missionpack
  scoreboard — belongs to a clickable scoreboard; this tree carries one, a
  stock mod has no call site for either. TV playback: `CG_VR_MenuYaw()`,
  `CG_VR_LockMenuYaw(yaw)`, `CG_VR_UnlockMenuYaw(yaw)`, and
  `CG_VR_MenuPointerYaw()` coordinate a scrub pointer with the virtual
  screen — call sites exist only in a mod with TV/demo scrubbing (this
  tree). All are accessors, never raw `vr->` reads, and all are
  dormancy-safe.
- **Drop-state resets and reads.** Call `CG_VR_DeathCamReset()` wherever the
  local player (re)spawns or a new gamestate begins (this tree: the `CG_Init`
  seed and `CG_Respawn`), and `CG_VR_PortraitReset()` where your HUD
  portrait's subject changes. For your own drawing,
  `CG_VR_DrawingZoomedHUD()` is qtrue inside the zoom minimal-HUD pass and
  `CG_VR_ReticleShader()` returns the zoom scope mask shader. Dormant, the
  resets no-op and the reads return qfalse/0.

Two sites are bigger than one line; take their shape from this tree:

- **The lightning gun** (`CG_LightningBolt`, cg_weapons.c). Compute
  `directView` as stock does, then fork on `vrActive && directView` — one
  of the sanctioned `vrActive` gates: the bolt originates at the
  controller-held weapon (`CG_CalculateVRWeaponPosition` supplies the
  muzzle point and angles) and `CG_VR_OnWeaponFiring( weapon )` drives the
  continuous-fire haptic each frame; the stock true-lightning math runs in
  the other branch.
- **The HUD portrait** (`CG_DrawStatusBarHead`, cg_draw.c; missionpack
  twin `CG_DrawPlayerHead`, cg_newdraw.c).
  `vr = CG_VR_PortraitHeadAngles( angles );` right after the function's
  `VectorClear( angles )`: qtrue fills the real head orientation — gated on
  `EF_VR_PLAYER` inside, not `vrActive`, so a flatscreen client still
  portrays a followed VR player — and the damage kick becomes additive on
  top (`angles[YAW] += cg.damageX * 45`); the stock random idle-bob
  machinery runs only under `!vr`. Pair it with the `CG_VR_PortraitReset()`
  call-out above.

One more call-out pairs with Step 7: the probe's flatscreen half. The VR
draw tail renders the probe inside the protected 2D bracket, so the host
site is a sanctioned `vrActive` gate at the end of `CG_DrawActiveFrame`:

```diff
 	// actually issue the rendering calls
 	CG_DrawActive( stereoView );
+
+	// VR API conformance probe overlay; when VR is active the draw tail
+	// renders it inside the protected 2D bracket instead
+	if ( !vrActive )
+		CG_VRProbe_Draw();
```

The families above are the reading guide, not the inventory; the
authoritative call-out list is the diff itself — this tree's `cg_*` delta
against stock, or the mod template's Step 5 commit.

Two placement details:

1. **Keep the weapon-adjust call out of `CG_VR_Frame`.** `CG_VR_Frame()`
   runs before `CG_ProcessSnapshots()` refreshes `cg.snap`. The per-frame
   weapon-adjust step, `CG_WeaponAdjustFrame()`, stays in
   `CG_DrawActiveFrame` after `CG_ProcessSnapshots`, right before
   `CG_AddViewWeapon`, so it reads a fresh snapshot; folding it into
   `CG_VR_Frame` would feed it a stale one. The draw half,
   `CG_WeaponAdjustDraw()`, sits at the tail of `CG_DrawScreen2D`.
2. **Most event hooks keep their call-site identity gate.** A `void` hook
   like `CG_VR_OnFall(severity)` cannot re-derive which client the event
   belongs to, so it keeps the local-player gate already at the call site.
   Only `CG_VR_OnTeleport(clientNum)` and `CG_VR_OnHitByMissile(entityNum)`
   receive an identity argument and check inside.

---

## Step 6: Wire the UI for your game

Do the UI (or UIs) your mod builds; Step 7's settings-screen requirement
applies to both.

**baseq3 (`q3_ui`).** `UI_VR_Init()` at the top of `UI_Init`:

```diff
 void UI_Init( void ) {
+	UI_VR_Init();
+
 	UI_RegisterCvars();
```

The rest are one-liners at their sites: `UI_VR_Shutdown()` beside
`UI_Shutdown` in vmMain's `UI_SHUTDOWN` case; `UI_VR_KeyEvent(key)`
first-chance in `UI_KeyEvent`; in `UI_MouseEvent`, the
`UI_VR_StickNavActive()` early-out,
`UI_VR_CursorOverride( &uis.cursorx, &uis.cursory )`, the
`UI_VKeyboardIsActive()` guard before the region test, and the
`UI_VR_OnMenuMove()` hover haptic on focus change; `UI_VR_UpdateScale()`
and the same cursor override once per frame in `UI_Refresh`; the cursor
draw gated with `!UI_VR_HideCursor()`; and `UI_VR_CompensateModelFov` on
every menu-model refdef — the ui_menu.c models and `UI_DrawPlayer` —
keeping the origin math on the desired fov. The connect-screen background
is a sanctioned `vrActive` gate in both UIs; baseq3's form:

```c
	if ( vrActive ) {
		UI_VR_FillScreen( uis.menuBackShader );
	} else {
		UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
	}
```

VR covers the physical framebuffer edge-to-edge so the letterbox outside
the centered 4:3 box shows no stale eye-buffer content; flatscreen keeps
the stock aspect-preserving draw, because the fill would visibly paint the
pillarbox bars. baseq3 keeps `UI_AdjustFrom640` free of any VR branch — it
bakes the VR transform into the `uis.scale`/`biasX`/`biasY` fields the
function already applies (Appendix E).

**Team Arena (`ui`).** Team Arena hooks the 640-space transform directly,
at *two* sites, both routed through `vr_uishared` so text and models
transform identically in both links. `ui_atoms.c`'s `UI_AdjustFrom640`
takes the simple early-out:

```diff
 void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
+	if ( UI_VR_AdjustFrom640( x, y, w, h ) ) {
+		return;
+	}
```

`ui_shared.c`'s `AdjustFrom640` is more involved — one of the sanctioned
`vrActive` forks: declare `extern qboolean vrActive;` at the top of the
file, keep your stock body for `!vrActive`, try `UI_VR_AdjustFrom640`
first, and keep a live in-world transform tail (`xscale`/`yscale` ÷ 2.75,
centered) for the non-virtual-screen case — the missionpack cgame paints
the VR HUD's `.menu` widgets through it during gameplay. Take this tree's
`AdjustFrom640` fork; the `!vrActive` branch stays your stock body.

The other call-outs live in `ui_main.c`: `UI_VR_Init()` at the top of
`_UI_Init`; `UI_VR_Shutdown()` beside `_UI_Shutdown` in vmMain's
`UI_SHUTDOWN` case;
`UI_VR_CursorOverride( &uiInfo.uiDC.cursorx, &uiInfo.uiDC.cursory )` once
per frame in `_UI_Refresh`, with the cursor draw gated behind
`!UI_VKeyboardIsActive()` and `!UI_VR_HideCursor()`; in `_UI_MouseEvent`,
the `UI_VR_StickNavActive()` early-out and the same cursor override; the
hover haptic wired into the display context
(`uiInfo.uiDC.vrMenuMove = &UI_VR_OnMenuMove`) alongside the four
`vkeyboard*` members and `Menu_HandleKey`'s edit-field interception — a
deeper insertion this tree carries for in-headset text entry (Appendix D);
`UI_VR_LoadMenus()` after every `UI_LoadMenus` wave — both `_UI_Init` and
`UI_Load`; the connect-screen background gated the same way as baseq3's
(`if ( vrActive && menu->window.background )
UI_VR_FillScreen( menu->window.background );` before the stock
`Menu_Paint`); and the two settings dispatchers
`UI_VR_UpdateSettingsCvar(name, val)` and `UI_VR_RunMenuScript(name)` in
`UI_Update`'s else-if chain and `UI_RunMenuScript`'s command chain.
`UI_VR_CompensateModelFov` is called from `Item_Model_Paint` in both links
and from `UI_DrawPlayer`.

Appendix D lists the deliberate differences between the two UIs.

---

## Step 7: The VR settings screens

The settings screens are **required**: comfort and control options must be
reachable from inside the headset, and a mod's own `ui.qvm` replaces the
engine's wholesale — ship no VR settings UI and VR players have none. What
is replaceable is the implementation: a mod may present these options its
own way provided it covers the full cvar surface (Appendix A) and reproduces
the couplings behind the screens:

- **`vr_hudDrawStatus` → `cg_draw3dIcons`.** The value `2` (no status bar)
  also sets `cg_draw3dIcons 0`; **every other value** sets
  `cg_draw3dIcons 1`.
- **`vr_uturn` ↔ `vr_controlSchema`.** These interlock; changing one
  recomputes the other so the button map stays consistent.
- **`vr_switchThumbsticks`.** A swap-in-place edit of the affected button
  mappings.
- **The desktop-mirror Apply.** `vr_desktopMode` with
  `r_customdesktopwidth` / `r_customdesktopheight` are staged, then applied
  with a `vid_restart` on confirm.
- **Reachable VR menus are fullscreen.** The engine's laser pointer can land
  a click anywhere on the virtual screen; a non-fullscreen menu treats a
  click outside its own rect as out-of-bounds and dismisses. Every shipped
  standalone VR screen sets `fullscreen 1` — the five q3_ui screens
  (`menu.fullscreen = qtrue`) and Team Arena's `vroptions_*.menu`. The
  in-game `ingame_vroptions_*.menu` panels are the deliberate exception:
  `fullscreen 0` with `outOfBoundsClick`, so a click outside the panel
  closes it back to the game. A replacement full-screen menu that omits
  `fullscreen 1` will misbehave under laser input with no error anywhere.

The straightforward path is to take the shipped screens as-is; the couplings
come along for free.

**Making them reachable.** baseq3: in `ui_setup.c`, the SETUP menu swaps
its flatscreen CONTROLS entry for a VR OPTIONS entry when
`UI_VR_Platform() != VRP_NONE` (VR binds live in VR OPTIONS; the keyboard
binds page is flatscreen-only). Dormancy-safe: on a flatscreen engine the
menu is stock. Team Arena needs no wiring — the `vrmenus` manifests packed
into the missionpack pak are the only load path, and `UI_VR_LoadMenus`
(Step 6) reads them.

> **Build, run, and read the probe.** Set `cg_vrApiProbe 1` and connect a VR
> client. The probe draws an overlay at the top-left: `VR API ACTIVE` (or
> `ABSENT` on a flatscreen engine), then the sync round-trip check — `PASS`
> while the engine echoes the mirror back correctly each frame, `FAIL` if a
> sync is dropped — then live HMD position/orientation, weapon angles, FOV,
> thumbsticks, and state flags. `ACTIVE` plus a steady `PASS` with sensible,
> moving numbers means the drop is wired end-to-end. Turn it off with
> `cg_vrApiProbe 0`.

---

# Appendices (reference)

## Appendix A: The cvar contract

Alongside the mirror, the drop reads and writes engine-owned `vr_*` cvars by
name — a second, unversioned ABI whose contract is the *names*. Single-value
reads and writes go through `CG_VR_CvarValue(name)` /
`CG_VR_CvarSet(name, value)` (a string-buffer idiom; the cgame QVM has no
float-returning cvar trap — the UI modules read through stock
`trap_Cvar_VariableValue`). A mod that rolls its own settings UI still
writes these names.

| Cvar(s) | Class | Role |
|---------|-------|------|
| `vr_worldscale` | archived | World scale; read each frame in `CG_VR_Frame` and server-side in `vr_game.c` |
| `vr_worldscaleScaler` | archived | Spectator/zoom scale factor; **written** each frame by the drop's view code (spectator multipliers, zoom coefficient) and read back for HUD-panel placement; the engine's renderers read it too |
| `vr_hudDrawStatus`, `vr_hudDepth`, `vr_hudScale`, `vr_hudYOffset` | archived | HUD visibility preference and in-world placement |
| `vr_currentHudDrawStatus`, `vr_currentHudDepth` | transient | Written by `CG_VR_Frame` for the renderer to read |
| `vr_thirdPersonSpectator` | transient | Written each frame so the renderer drops sky in spectator views |
| `vr_platform` | ROM (engine-set) | `pc` / `quest`; read **only** through `UI_VR_Platform()` (see below) |
| `vr_6dof` | archived | Seeds `use_6dof` (singleplayer only) |
| `vr_desktopMode` | archived | Desktop-mirror mode, staged/applied by the menu scripts |
| `r_customdesktopwidth`, `r_customdesktopheight` | archived (PC engine only) | Desktop-mirror resolution, staged and written by the mirror Apply (Step 7); unregistered on other engines, where the writes are inert |
| `vr_lasersight`, `vr_twoHandedWeapons`, `vr_showItemInHand`, `vr_rollWhenHit`, `vr_weaponAdjust`, `vr_weaponSelectorMode`, `vr_weaponSelectorWithHud` | archived | Gameplay/comfort toggles read by the client hooks |
| `vr_uturn`, `vr_controlSchema`, `vr_switchThumbsticks` | archived | Control-scheme handlers (see Step 7) |
| `vr_button_map_*` family | archived | Per-button remaps: `A`, `B`, `X`, `Y`, `PRIMARYGRIP`, `PRIMARYTHUMBSTICK`, and the `RTHUMB{FORWARD,BACK,LEFT,RIGHT}` set with their `_ALT` variants |

There is no `vr_stabilised` cvar; weapon stabilization is a mirror flag, not
a cvar.

**Menu content must never read `vr_platform` raw.** The flatscreen engine
never registers the cvar, so a stale value lingering in a user config (an
older build, a hand-`seta`) must not impersonate a headset. Key VR rows on
`UI_VR_Platform()`, which returns `VRP_NONE` whenever the mirror is dormant
regardless of the cvar. The two-gate body lives in `VR_Platform`
(`game/vr_platform.c`); each UI module wraps it:

```c
vrPlatform_t VR_Platform( qboolean vrActive ) {
	char buf[16];

	if ( !vrActive ) {
		return VRP_NONE;
	}
	trap_Cvar_VariableStringBuffer( "vr_platform", buf, sizeof( buf ) );
	if ( !Q_stricmp( buf, "pc" ) ) {
		return VRP_PC;
	}
	if ( !Q_stricmp( buf, "quest" ) ) {
		return VRP_QUEST;
	}
	return VRP_NONE;
}

vrPlatform_t UI_VR_Platform( void ) {
	return VR_Platform( vrActive );
}
```

Team Arena's `UI_VR_LoadMenus()` is double-gated the same way.

## Appendix B: Bootstrap trap keys

`trap_GetValue(value, valueSize, key)` returns non-zero and writes the
resolved trap number into `value` when the engine answers for `key`. One key
is the gate: if `trap_VR_RegisterState` does not resolve, the module stays
dormant and nothing else is attempted. Every other key is part of the
**required v1 contract** — an engine that answers the handshake provides all
of them — so the bootstraps bind them unconditionally and the hooks call
them without guards; there are no per-feature capability flags. Growing the
set with new traps is what a `VR_API_MINOR` bump is for.

| Key | Modules |
|-----|---------|
| `trap_VR_RegisterState` | all (**the gate**: its resolve sets `vrActive` / `g_vrActive`) |
| `trap_R_BeginPostBloom2D` / `trap_R_EndPostBloom2D` | cgame |
| `trap_R_HUDBufferStart` / `trap_R_HUDBufferEnd` | cgame |
| `trap_HapticEvent` | cgame, both UI |
| `trap_VKeyboard_Show` / `_Hide` / `_IsActive` / `_HandleKey` | both UI |

## Appendix C: Asset dependencies

Each asset degrades cleanly when absent: a missing asset drops its feature,
it never crashes.

| Asset | Used by | Without it |
|-------|---------|-----------|
| `gfx/weapon/scope` | `CG_VR_RegisterMedia`, zoom scope mask | Zoom still works, no reticle |
| `sprites/vr/hud` | `CG_VR_RegisterMedia`, in-world HUD panel (HUD mode 1) | Mode 1's panel doesn't draw; HUD mode 2 unaffected |
| laser-sight beam | drawn via `CG_LaserSight`, gated by `vr_lasersight` | No beam renders |
| VR menu files / manifests | the settings UI | Console-only settings (every toggle is still a cvar) |

`CG_VR_RegisterMedia` registers those two shaders (plus the weapon-wheel
hover sphere, `models/powerups/health/small_sphere.md3`) itself. The laser
beam draws through the host's `CG_LaserSight` (`vr_host.h`). Comfort
vignettes are cvar-driven (`vr_comfortVignette`) and rendered engine-side.

## Appendix D: Differences between the two UIs

Deliberate differences, noted so the two `vr_ui.c` files don't surprise you:

- **Virtual-keyboard key routing.** baseq3 intercepts in `UI_KeyEvent` via
  `UI_VR_KeyEvent(key)`. Team Arena intercepts deeper, inside
  `Menu_HandleKey`'s edit-field path, because the `.menu` parser owns key
  dispatch.
- **On-screen model transform.** baseq3 bakes the VR scale into
  `uis.scale/biasX/biasY` once per frame and keeps `UI_AdjustFrom640` free
  of any VR branch — its body simply applies those fields (rewritten from
  stock's `xscale`/`yscale`/`bias` to the uniform-scale fields; see
  Appendix E). Team Arena hooks the transform directly, at two sites
  unified through `vr_uishared`.
- **Event-hook identity.** Only `CG_VR_OnTeleport` and
  `CG_VR_OnHitByMissile` carry a client/entity number; the others rely on
  the local-player gate at their call site (Step 5, note 2).

## Appendix E: The host contract (`vr_host.h`)

Everything the drop consumes from your tree beyond stock 1.32 is declared in
`vr_host.h`; this appendix is its narrative. The drop carries its own VR
state, cvars, and media internally — no `cg_t`/`cgs_t` fields, no cvar-table
entries, and no media-struct entries are required of the host. What remains:

**Seven host functions.** Declared with their contracts in `vr_host.h`:
`CG_DrawScreen2D`, `CG_Draw2DMinimal`, `CG_PushHUDAnchors` /
`CG_PopHUDAnchors`, `CG_GetViewable4x3Dimensions`, `CG_GetProjectionCenter`,
and `CG_LaserSight`. A plain-4:3, no-frills host can implement the geometry
pair as `640/480` and `320/240` and draw nothing in `CG_DrawScreen2D`; the
anchor pair may be empty on a host with no widescreen anchoring. Take this
tree's implementations if you want the full behavior.

**Six stock exports.** Functions that are `static` in stock 1.32 and must
lose it: `CG_Draw2D`, `CG_DrawCrosshair3D`, `CG_WeaponSelectable`,
`CG_CalculateWeaponPosition`, `CG_OffsetFirstPersonView`, and `CG_TrailItem`
(which also gains offset/scale parameters — see `vr_host.h`). Two of them
want this tree's bodies, not just the export. `CG_Draw2D`: this tree
restructured it into an exported, stereo-aware form split from
`CG_DrawScreen2D` and `CG_Draw2DMinimal`, and that split is what lets the
whole-frame fork bracket your 2D correctly. `CG_DrawCrosshair3D`: take this
tree's implementation — it aims the trace from
`CG_CalculateVRWeaponPosition` rather than the view axis, and sets the
sprite color explicitly (the sprite is vertex-modulated; map this tree's
crosshair-color cvar to your own scheme, or plain white).

**Renderer contract (`cgame/tr_types.h`).** `RF_OVERBRIGHT`,
`RF_WORLD_ORIENTED`, `RT_LASERSIGHT`, `refEntity_t.invert`, and
`refdef_t.isHUD`. The two struct fields are **tail-appended and must stay
last**: the stock prefix is what keeps the layout compatible with engines
that do not know the field. A mid-struct insertion compiles clean and
corrupts rendering at run time.

`isHUD` is not just a declaration: set `refdef.isHUD = qtrue` on every
mini-scene refdef your 2D pass renders. In stock that is one site —
`CG_Draw3DModel` — and it carries the scoreboard heads, the status-bar
face, and every 3D icon. Menu model previews in the UI modules render
into the virtual screen and do not take the flag.

**Syscall plumbing.** The QVM/native trampolines for every Appendix B trap
live in all four `*_syscalls.c`, with declarations at the tail of each
`*_local.h`; the `dll_*` trap-number variables and QVM function-pointer
storage live in `cg_main.c` and `g_main.c` (the rest ships inside the
vendored files — `vr_game.c`, `vr_cgame.c`, and the UI modules' storage
entirely in `vr_ui.c`). Mechanical — copy the blocks from this tree's
files. Float arguments cross the DLL boundary through `PASSFLOAT`; miss it
and native builds pass garbage.

**Small pieces.** `Q_sscanf` (stock trees: `#define Q_sscanf sscanf` in
`vr_host_config.h`); the two stat enum entries (Step 4); the tunables
`PLAYER_HEIGHT` / `SPECTATOR_WORLDSCALE_MULTIPLIER` /
`SPECTATOR2_WORLDSCALE_MULTIPLIER`, which default in `vr_host.h` and may be
predefined by the host; `cgs.cursorX` / `cgs.cursorY` become `float`
(stock: `int`) — `CG_VR_ScoreboardCursor` writes the cursor through
`float *`, the cgame twin of the UI-substrate cursor fields below; and
`cgs.media.friendShader` (`sprites/foe`) must be registered in every
gametype, not just `GT_TEAM` — the weapon wheel's selection marker draws
through it.

**UI substrate.** baseq3: the `uis.scale` / `uis.biasX` / `uis.biasY` /
`uis.cursorScaleR` / `uis.screenXmin…Ymax` uniform-scale fields on
`uiStatic_t` **replace** stock's `xscale` / `yscale` / `bias`, and every
consumer converts to them: `UI_AdjustFrom640`, `UI_MouseEvent`'s cursor
scaling and clamps, and the three text painters (`UI_DrawBannerString2`,
`UI_DrawProportionalString2`, `UI_DrawString2`). `UI_VideoCheck`
recomputes the fields on resolution change — take this tree's function
(`ui_main.c`) and its call sites in `UI_Init`, `UI_KeyEvent`, `UI_Refresh`,
and the connect screen; under VR, `UI_VR_UpdateScale` overwrites the same
fields each frame. `UI_DrawPlayer` (ui_players.c) recomputes its fov from
the transformed rect, keeping the origin math on the desired fov. And
`uis.cursorx` / `uis.cursory` become `float` (stock: `int`), because
`UI_VR_CursorOverride` writes the cursor through `float *`. Team Arena: the
`vrMenuMove` member on `displayContextDef_t`, wired to the drop's menu-hover
hook (the virtual-keyboard wiring adds the four `vkeyboard*` members —
Step 6) — and that struct's `cursorx` / `cursory` become `float` for the
same cursor-override reason.

If a symbol is still unresolved after `vr_host.h` is satisfied, it is a bug
in the contract — report it; Step 2's failing build output is otherwise your
complete work list.
