# Client Engine — MVD Playback Session Prompt

You are working on a **Quake3e-based client engine**. Your task is to implement Multi-View Demo (MVD) playback that reads `.mvd` files recorded by the server, reconstructs full game state, and feeds snapshots to the cgame module through the existing trap interface.

## Context

The server records `.mvd` files containing delta-compressed full game state every frame — all entity states, all player states, configstrings, and server commands. The client must read these files, reconstruct the world state, select a viewpoint (which player to follow), build standard `snapshot_t` structures, and deliver them to cgame as if they were live network snapshots.

The cgame module (Trinity QVM) does NOT need to know the internal MVD format. From cgame's perspective, MVD playback looks like a regular demo with extra capabilities signaled through configstrings and cvars:

- `CS_SERVERINFO` contains `\mvd\1`
- `cl_mvdViewpoint` cvar holds the current viewpoint clientNum
- `cl_mvdTime` and `cl_mvdDuration` cvars expose timeline info
- cgame sends `mvd_view`, `mvd_view_next`, `mvd_view_prev`, `mvd_seek` as console commands

## Architecture

MVD playback parallels the existing demo playback path. The engine already has a mode where instead of receiving network packets, it reads from a `.dm_68` file and feeds snapshots to cgame. MVD playback follows the same pattern with a different reader.

Key engine source areas you will work with:

- `code/client/cl_main.c` — demo playback initiation, `CL_PlayDemo_f`. Branch here based on file extension or header magic to enter MVD playback.
- `code/client/cl_cgame.c` — `CL_GetSnapshot` and related functions that implement `CG_GETSNAPSHOT` and `CG_GETCURRENTSNAPSHOTNUMBER` traps. During MVD playback, these return MVD-synthesized snapshots.
- `code/client/cl_parse.c` — standard demo packet parsing. Reference for how snapshots are normally reconstructed from demo files.
- `code/client/client.h` — client state structures (`clientActive_t`, `clientConnection_t`). Add MVD playback state here.
- `code/qcommon/msg.c` — `MSG_ReadDeltaEntity`, `MSG_ReadDeltaPlayerstate`. Reuse these to decode MVD frames.

Create a new file: `code/client/cl_mvd.c`

## MVD File Format (as written by the server)

### Header

- Magic: "MVD1" (4 bytes)
- Protocol version: uint32
- Server frame rate (sv_fps): uint32
- Max clients: uint32
- Map name: null-terminated string
- Timestamp: null-terminated string
- Index table file offset: uint64 (for seeking — read this to find the index at end of file)
- Full configstrings dump: sequence of (index: uint16, length: uint16, data: bytes), terminated by index 0xFFFF.

### Frame Blocks

Each frame:

- serverTime: int32
- frameType: uint8 (0 = delta, 1 = keyframe)
- flags: uint8
- Entity bitmask: 128 bytes (1024 bits)
- Per active entity: delta-encoded entityState_t (MSG_ReadDeltaEntity to decode)
- Player bitmask: 8 bytes (64 bits)
- Per active player: delta-encoded playerState_t (MSG_ReadDeltaPlayerstate to decode)
- Configstring changes: count uint16, then (index: uint16, length: uint16, data: bytes) each
- Server commands: count uint16, then (targetClient: int8, length: uint16, data: bytes) each

Keyframes use zero baselines (all fields written in full). Delta frames delta against the previous frame.

### Index Table (end of file)

- Number of keyframes: uint32
- Per keyframe: (serverTime: int32, fileOffset: uint64)

## What to Build

### 1. MVD Playback State (client.h)

Add to client state (new struct or extend existing):

- MVD file handle
- MVD active flag (distinguishes MVD playback from regular demo playback)
- Server frame rate from header (sv_fps)
- Max clients from header
- Current world state buffers:
  - entityState_t[MAX_GENTITIES] — running entity state (updated each frame by applying deltas)
  - playerState_t[MAX_CLIENTS] — running player state
  - Entity active bitmask: byte[128]
  - Player active bitmask: byte[8]
- Current viewpoint: int (clientNum to follow, default 0)
- Configstrings: stored in existing `cl.gameState` configstring storage
- Current serverTime (from last read frame)
- Next frame serverTime (for interpolation timing)
- Keyframe index (loaded from end of file for seeking)
- Playback speed multiplier (for timescale)

Memory: same as server recording buffers, ~192 KB for entity+player state.

### 2. File Reader (cl_mvd.c)

`CL_MVD_Open(filename)`:

- Open file, read and validate header (check magic "MVD1", protocol version)
- Read sv_fps, maxclients, map name
- Read index table offset from header, seek to end, load keyframe index into memory
- Read configstrings dump, populate `cl.gameState`
- Insert `\mvd\1` into the CS_SERVERINFO configstring
- Initialize world state buffers to zero
- Read first frame (will be a keyframe) to populate initial state
- Set cl.mvdActive = true

`CL_MVD_ReadFrame()`:

- Read frame header (serverTime, frameType)
- If keyframe: zero all previous state buffers (keyframe deltas against zero baseline)
- Read entity bitmask. For each entity slot:
  - If bit set in current bitmask: read MSG_ReadDeltaEntity. For keyframes, delta against zeroed entityState_t. For delta frames, delta against the stored entityState_t for this slot. Store result in running entity state buffer.
  - If bit was set in previous bitmask but not current: entity was removed. Zero it in the buffer.
- Read player bitmask. For each client slot:
  - Same pattern as entities using MSG_ReadDeltaPlayerstate.
- Read configstring changes. Apply to `cl.gameState`.
- Read server commands. Queue them for delivery to cgame via the existing `CG_GETSERVERCOMMAND` trap path.
- Update previous bitmasks to current.
- Store serverTime.

`CL_MVD_Seek(targetTime)`:

- Binary search the keyframe index for the largest keyframe serverTime <= targetTime
- Seek file to that offset
- Read the keyframe (populates full world state)
- Fast-forward: read delta frames until serverTime >= targetTime
- Resume normal playback

### 3. Snapshot Synthesis

This is the core integration point. When cgame calls `trap_GetSnapshot`, the engine must return a valid `snapshot_t` built from the MVD world state.

`CL_MVD_BuildSnapshot(snapshotNumber, snapshot_t *dest)`:

- Set `dest->serverTime` from the current MVD frame serverTime
- Set `dest->ps` from `playerStateBuffer[currentViewpoint]`. This is the playerState_t of the player being followed.
- Set `dest->snapFlags` appropriately (clear SNAPFLAG_NOT_ACTIVE)
- Set `dest->ping = 0` (not meaningful in demo playback)
- Build entity list — iterate all 1024 entity slots:
  - Skip the viewpoint player's own entity (entity number == currentViewpoint) since their state is already in `dest->ps`
  - Skip inactive entities (bit not set in entity bitmask)
  - Add to `dest->entities[dest->numEntities++]`
  - Do NOT apply PVS culling — include all active entities. This enables free-fly camera in cgame.
  - If numEntities reaches MAX_ENTITIES_IN_SNAPSHOT (256), stop adding. In practice this rarely triggers.
- Set `dest->areamask` to all 1s (0xFF for all 32 bytes) — no portal area culling during MVD playback.
- Set `dest->numServerCommands` and `dest->serverCommandSequence` from queued server commands.

Hook into `CL_GetSnapshot` (the function behind the CG_GETSNAPSHOT trap): when `cl.mvdActive`, call `CL_MVD_BuildSnapshot` instead of the normal snapshot lookup.

Hook into `CL_GetCurrentSnapshotNumber` (behind CG_GETCURRENTSNAPSHOTNUMBER): when `cl.mvdActive`, return the MVD frame number and serverTime.

### 4. Frame Pacing

The engine's demo playback loop must advance MVD frames at the correct rate.

Each render frame:

- Check if `cl.serverTime` (the cgame's requested time) has advanced past the current MVD frame's serverTime
- If so, read the next MVD frame(s) until caught up
- The engine already manages `cl.serverTime` progression for regular demos — MVD should follow the same timescale mechanism (`cl_timescale` cvar)

Maintain two MVD frames in memory (current and next) for cgame's snapshot interpolation. cgame calls `trap_GetSnapshot` for both `snap` and `nextSnap` — the engine must be able to return both:

- Snapshot N = current frame's world state
- Snapshot N+1 = next frame's world state
- cgame interpolates entities between them based on serverTime

### 5. Viewpoint Switching

Register console commands during MVD playback:

- `mvd_view <clientnum>` — validate clientnum is active (bit set in player bitmask), set `cl.mvdViewpoint`. Update `cl_mvdViewpoint` cvar.
- `mvd_view_next` — find next active player after current viewpoint (wrap around). Set viewpoint.
- `mvd_view_prev` — find previous active player.
- `mvd_seek <time_ms>` — call CL_MVD_Seek.

When viewpoint changes, the next snapshot returned to cgame will have a different `snapshot.ps`. cgame handles this gracefully — it already handles followed-player changes during regular spectating and demo playback.

### 6. Cvar Exposure

Maintain these cvars for cgame to read:

- `cl_mvdViewpoint` — current viewpoint clientNum (int)
- `cl_mvdTime` — current playback serverTime (int, ms)
- `cl_mvdDuration` — total demo duration (int, ms — read from last keyframe index entry or last frame serverTime)

### 7. Demo Command Integration

Integrate MVD into the existing `demo` command:

- In `CL_PlayDemo_f`, after determining the filename, check the file extension or read the first 4 bytes for the "MVD1" magic.
- If MVD: call `CL_MVD_Open` and enter MVD playback mode.
- If standard .dm_XX: use existing demo playback path.
- Set `clc.demoplaying = qtrue` in both cases so `demoPlayback` is passed as true to `CG_DrawActiveFrame`.

This means users type `demo myfile` regardless of format — the engine auto-detects.

### 8. Cleanup

When MVD playback ends (user types `disconnect`, file ends, or `stopdemo`):

- Close MVD file handle
- Free keyframe index
- Clear MVD state
- Unregister MVD-specific console commands

## Constraints

- Do NOT modify cgame trap enum values in cg_public.h or game trap enum values in g_public.h.
- Do NOT modify snapshot_t, entityState_t, or playerState_t structures.
- Reuse MSG_ReadDeltaEntity and MSG_ReadDeltaPlayerstate from msg.c. Do NOT reimplement delta decoding.
- The snapshot_t you build must be identical in structure to what the normal demo path produces. cgame must not need to distinguish MVD from regular demo at the trap level.
- The areamask should be all 1s (all areas visible) during MVD playback since there is no meaningful PVS.
- Entity list should skip the viewpoint player entity (its data is in snapshot.ps, not in entities array — this matches normal Q3 behavior).
- Server commands with targetClient != -1 and targetClient != currentViewpoint can be optionally filtered out (they were directed at another player). Broadcast commands (targetClient == -1) should always be delivered.

## Testing

- Play back an MVD recorded by the server engine. Verify cgame initializes correctly (map loads, player models appear, HUD works).
- Switch viewpoints via console. Verify camera jumps to new player's position, first-person weapon model changes, HUD updates (health, ammo, armor reflect new player).
- Test orbit camera (cg_followMode 1) during MVD playback — should orbit current viewpoint player.
- Test seeking: jump to middle of demo, verify world state is correct (compare entity positions against a full sequential playback to the same timestamp).
- Test timescale: `timescale 2` should play at double speed, `timescale 0.5` at half speed.
- Test edge cases: player disconnects mid-demo (player bitmask bit clears), map items respawning (entities appearing/disappearing), match restart (configstring changes).
- Verify memory usage stays constant regardless of demo length (no leaks from frame reading).
