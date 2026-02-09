# Trinity QVM — MVD Playback UI Session Prompt

You are working on **Trinity**, a Quake 3 QVM mod (baseq3 + missionpack) that runs on the Quake3e engine. Your task is to add client-side UI and camera support for Multi-View Demo (MVD) playback.

## Context

The engine (a separate repository) handles MVD recording and playback. It feeds snapshots to cgame through the standard trap interface — no new traps are needed. The engine signals MVD mode and viewpoint state through existing mechanisms:

- MVD detection: the engine adds `\mvd\1` to `CS_SERVERINFO` during MVD playback. cgame reads this via `trap_GetConfigString(CS_SERVERINFO, ...)` and parses for the `mvd` key.
- Current viewpoint: the engine maintains a cvar `cl_mvdViewpoint` containing the `clientNum` being viewed. cgame reads it via `trap_Cvar_VariableStringBuffer("cl_mvdViewpoint", ...)`.
- Active players: already in configstrings at `CS_PLAYERS + clientNum`. Empty string means inactive slot. The engine populates these from the MVD file header.
- Viewpoint switching: cgame sends commands to the engine via `trap_SendConsoleCommand`. The engine recognizes `mvd_view <clientnum>`, `mvd_view_next`, and `mvd_view_prev` during MVD playback.
- Timeline info: the engine exposes `cl_mvdTime` (current playback time in ms) and `cl_mvdDuration` (total duration in ms) as cvars.
- Demo mode flag: `CG_DrawActiveFrame` receives `demoPlayback = qtrue` during MVD playback, same as regular demos.
- Entity visibility: during MVD playback the engine skips PVS culling and sends all active entities in the snapshot. No camera-position feedback is needed unless entity count exceeds MAX_ENTITIES_IN_SNAPSHOT (256), which is rare.

## Architecture

Trinity is purely QVM code. The relevant directories are:

- `code/cgame/` — client game module (rendering, HUD, camera, input)
- `code/game/` — server game module (gameplay logic) — unlikely to need changes
- `code/q3_ui/` — baseq3 menu UI
- `code/ui/` — missionpack menu UI

Key files you will work with:

- `code/cgame/cg_local.h` — core client state struct (`cg_t`, `cgs_t`). Add `mvdPlayback` flag, free-fly camera state.
- `code/cgame/cg_view.c` — camera and viewpoint logic. Contains the orbit camera (`CG_InitOrbitCamera`, `CG_UpdateOrbitInput`, `CG_OffsetOrbitView`). Extend for free-fly mode.
- `code/cgame/cg_draw.c` — HUD drawing. Add MVD player list overlay and timeline bar.
- `code/cgame/cg_consolecmds.c` — console command registration. Add MVD keybind commands.
- `code/cgame/cg_main.c` — initialization. Detect MVD mode from serverinfo.
- `code/cgame/cg_servercmds.c` — server command and configstring processing.
- `code/cgame/cg_snapshot.c` — snapshot transition logic. May need minor awareness of MVD for viewpoint transitions.
- `code/cgame/cg_predict.c` — prediction is already skipped during demoPlayback.

## What to Build

### 1. MVD Mode Detection

In `CG_Init` or `CG_ConfigStringModified`, parse `CS_SERVERINFO` for `\mvd\1`. Set `cg.mvdPlayback = qtrue` in the `cg_t` struct. This flag gates all MVD-specific UI and behavior.

### 2. Viewpoint Switching Commands

Register cgame commands (in `cg_consolecmds.c`) that proxy to engine console commands:

- `mvd_next` — sends `trap_SendConsoleCommand("mvd_view_next\n")`
- `mvd_prev` — sends `trap_SendConsoleCommand("mvd_view_prev\n")`
- `mvd_player <n>` — sends `trap_SendConsoleCommand("mvd_view <n>\n")`
- `mvd_freefly` — toggles free-fly camera mode (local to cgame, no engine command needed)

### 3. Player List Overlay

When `cg.mvdPlayback` is true and a keybind is held (or toggled), draw a player list showing:

- All active players (iterate `cgs.clientinfo[0..MAX_CLIENTS-1]`, skip empty)
- Current viewpoint highlighted (read `cl_mvdViewpoint` cvar, compare to clientNum)
- Team coloring if applicable (info already in `cgs.clientinfo[i].team`)
- Player can click or press a number to switch viewpoint

Draw this in `CG_DrawMVDOverlay()` called from `CG_Draw2D()` when in MVD mode.

### 4. Free-Fly Camera

Extend the orbit camera infrastructure in `cg_view.c`. The orbit camera already demonstrates the pattern — it reads input from `usercmd_t`, maintains independent camera state, and overrides `cg.refdef.vieworg` / `cg.refdef.viewaxis`.

Free-fly state in `cg_t`:

- `vec3_t freeFlyOrigin` — camera position
- `vec3_t freeFlyAngles` — camera angles (yaw, pitch, roll)
- `qboolean freeFlyActive` — mode toggle
- `float freeFlySpeed` — movement speed (adjustable)

Each frame when free-fly is active:

- Read `usercmd_t` via `trap_GetUserCmd` for movement and angle deltas
- Apply noclip-style movement: forward/right/up vectors from angles, scaled by `forwardmove`/`rightmove`/`upmove` and `freeFlySpeed`
- Update `freeFlyAngles` from mouse input (same delta-tracking pattern as `CG_UpdateOrbitInput`)
- Set `cg.refdef.vieworg = freeFlyOrigin` and build `cg.refdef.viewaxis` from `freeFlyAngles`
- Call `trap_S_Respatialize` with `freeFlyOrigin` so sound is spatialized to camera

When toggling free-fly ON, initialize `freeFlyOrigin` from current camera position. When toggling OFF, return to following the current viewpoint player.

### 5. Timeline / Scrub Bar (Optional)

Read `cl_mvdTime` and `cl_mvdDuration` cvars. Draw a thin progress bar at the bottom of the screen. Optionally allow seeking via `trap_SendConsoleCommand("mvd_seek <time>\n")`.

## Constraints

- Do NOT add new trap enums to `cg_public.h`. All communication with the engine uses existing traps (configstrings, cvars, console commands).
- Do NOT modify `snapshot_t`, `entityState_t`, or `playerState_t` structures.
- Do NOT modify `g_public.h` or any server game code for this feature.
- The orbit camera code is your reference implementation for how cgame can control the camera independently during demo playback. Study `CG_UpdateOrbitInput` and `CG_OffsetOrbitView` before writing free-fly code.
- `demoPlayback` is true during MVD. All existing `cg.demoPlayback` checks (prediction skip, follow-mode enabling, etc.) apply correctly without changes.
- Keep MVD UI drawing behind `cg.mvdPlayback` checks so it never appears during normal play or regular demo playback.

## Testing

- Build QVMs with `make qvms`
- Test with an engine that supports MVD playback feeding `\mvd\1` in serverinfo
- Verify: orbit camera works during MVD (it should with no changes)
- Verify: viewpoint switching via console commands results in smooth snapshot transitions
- Verify: free-fly camera allows full map traversal with mouse-look and WASD movement
- Verify: player list overlay shows correct players and highlights current viewpoint
- Verify: no visual artifacts when switching between follow, orbit, and free-fly modes
