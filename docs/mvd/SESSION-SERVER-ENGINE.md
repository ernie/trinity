# Dedicated Server Engine — MVD Recording Session Prompt

You are working on a **Quake3e-based dedicated server engine**. Your task is to implement server-side Multi-View Demo (MVD) recording that captures the full game state every frame, enabling multi-viewpoint playback on the client.

## Context

The server engine already runs the game module (QVM) via `VM_Call(GAME_RUN_FRAME, levelTime)` each server frame. After this call returns, the full game state is available through pointers established by the game module's `G_LOCATE_GAME_DATA` trap:

- `sv.gentities` — pointer to the game's entity array, stride `sv.gentitySize`
- `sv.gameClients` — pointer to the player state array, stride `sv.gameClientSize`
- `sv.configstrings[]` — all config strings (server info, player info, models, sounds)
- `sv.time` — current server time in milliseconds

The game module does not need any modifications for recording. Recording is entirely engine-side, transparent to the QVM.

## Architecture

The recording system hooks into the server frame loop and captures state after the game module has run. It writes a custom `.mvd` file format with delta compression for efficient storage.

Key engine source areas you will work with:

- `code/server/sv_main.c` — `SV_Frame()`, the main server frame function. Your hook point.
- `code/server/sv_game.c` — `SV_GameSystemCalls()`, implements game module traps. Contains `G_LOCATE_GAME_DATA` handler that sets `sv.gentities`, `sv.gameClients`, etc.
- `code/server/sv_snapshot.c` — existing per-client snapshot building. Reference for how entity state is read. Do NOT modify this for MVD.
- `code/server/server.h` — server state structures (`server_t`, `client_t`). Add MVD recording state here.
- `code/qcommon/msg.c` — `MSG_WriteDeltaEntity`, `MSG_WriteDeltaPlayerstate`, `MSG_ReadDeltaEntity`, `MSG_ReadDeltaPlayerstate`. Reuse these for MVD delta compression.
- `code/qcommon/qcommon.h` — message buffer types (`msg_t`).

Create a new file: `code/server/sv_mvd.c`

## Data Structures

These structures are defined in shared headers that the game module also uses. Do NOT modify them.

entityState_t (from q_shared.h) — per-entity network state:

- number, eType, eFlags
- pos (trajectory_t), apos (trajectory_t) — position and angle trajectories
- origin, origin2, angles, angles2
- otherEntityNum, otherEntityNum2, groundEntityNum
- constantLight, loopSound, modelindex, modelindex2
- clientNum, frame, solid, event, eventParm
- powerups, weapon, legsAnim, torsoAnim, generic1

playerState_t (from q_shared.h) — per-player state:

- commandTime, pm_type, bobCycle, pm_flags, pm_time
- origin, velocity, weaponTime, gravity, speed, delta_angles[3]
- groundEntityNum, legsTimer, legsAnim, torsoTimer, torsoAnim
- movementDir, grapplePoint, eFlags
- eventSequence, events[2], eventParms[2]
- externalEvent, externalEventParm, externalEventTime
- clientNum, weapon, weaponstate, viewangles, viewheight
- damageEvent, damageYaw, damagePitch, damageCount
- stats[16], persistant[16], powerups[16], ammo[16]
- generic1, loopSound, jumppad_ent

Constants:

- MAX_GENTITIES = 1024 (10-bit entity numbers)
- MAX_CLIENTS = 64
- MAX_CONFIGSTRINGS = 1024
- Default sv_fps = 30

## File Format Specification

### Header

Write once when recording starts:

- Magic bytes: "MVD1" (4 bytes)
- Protocol version: uint32
- Server frame rate (sv_fps): uint32
- Max clients: uint32
- Map name: null-terminated string
- Timestamp: null-terminated string (ISO 8601)
- Full configstrings dump: for each non-empty configstring, write (index: uint16, length: uint16, data: bytes). Terminate with index 0xFFFF. This captures player names, models, sounds, game settings — everything the client needs to initialize.

### Frame Blocks

Written every server frame after G_RunFrame returns.

Each frame block:

- Frame header:
  - serverTime: int32 (from sv.time)
  - frameType: uint8 (0 = delta frame, 1 = keyframe)
  - flags: uint8 (reserved)

- Entity section:
  - Active entity bitmask: 128 bytes (1024 bits, one per entity slot). Bit set = entity is active (linked and not SVF_NOCLIENT).
  - For each active entity:
    - If keyframe: write full entityState_t using MSG_WriteDeltaEntity against a zeroed baseline (same pattern the engine uses for initial baselines).
    - If delta frame: write MSG_WriteDeltaEntity against the previous frame's entityState_t for this entity number. If the entity is unchanged, MSG_WriteDeltaEntity writes a minimal "no change" marker.

- Player section:
  - Active player bitmask: 8 bytes (64 bits, one per client slot). Bit set = client is connected and has valid playerState_t.
  - For each active player:
    - If keyframe: write full playerState_t using MSG_WriteDeltaPlayerstate against a zeroed baseline.
    - If delta frame: write MSG_WriteDeltaPlayerstate against previous frame's playerState_t for this client number.

- Configstring changes:
  - Count: uint16 (number of configstrings that changed this frame)
  - For each: (index: uint16, length: uint16, data: bytes)
  - Most frames this is 0. Changes happen on score updates, player connects/disconnects, item pickups.

- Server commands:
  - Count: uint16 (number of server commands issued this frame)
  - For each: (targetClient: int8 where -1 means broadcast, length: uint16, data: bytes)
  - Captures chat messages, print events, centerprint, etc.

### Keyframes

Write a keyframe every 150 frames (5 seconds at sv_fps 30). Keyframes are identical in structure to delta frames but all entity and player states are written in full (delta'd against zero baselines). This enables seeking.

### Index Table (end of file)

Written when recording stops:

- Number of keyframes: uint32
- For each keyframe: (serverTime: int32, fileOffset: uint64)
- Index table offset: uint64 (written at a known position, e.g., bytes 8-15 of the file header, so the reader can find the table without scanning)

## What to Build

### 1. Recording State (server.h)

Add to the server state or a new struct:

- File handle for the MVD file
- Recording active flag
- Previous frame entity state buffer: entityState_t[MAX_GENTITIES] — the "last written" state for delta encoding
- Previous frame player state buffer: playerState_t[MAX_CLIENTS]
- Previous frame entity active bitmask: byte[128]
- Previous frame player active bitmask: byte[8]
- Frame counter (for keyframe interval)
- Keyframe index list (growable array of serverTime + fileOffset pairs)

Memory cost: about 192 KB for the two previous-frame buffers. Negligible.

### 2. Recording Functions (sv_mvd.c)

`SV_MVD_StartRecord(filename)`:

- Open file via FS_FOpenFileWrite
- Write header (magic, protocol, sv_fps, maxclients, map name)
- Dump all non-empty configstrings
- Zero-initialize previous frame buffers
- Set recording flag

`SV_MVD_WriteFrame()`:

- Called at the end of SV_Frame, after VM_Call(GAME_RUN_FRAME) and before or after SV_SendClientMessages — order relative to SV_SendClientMessages does not matter since recording is independent of the per-client snapshot pipeline.
- Determine if this frame is a keyframe (frameCounter % 150 == 0). If keyframe, record file offset for the index.
- Build current entity bitmask by iterating sv.gentities[0..sv.num_entities-1]. An entity is active if its linked field is true (in entityShared_t) and SVF_NOCLIENT is not set in svFlags.
- For each active entity: read entityState_t from the gentity (at offset 0 of each entity in the sv.gentities array — entityState_t is the first member of sharedEntity_t). Delta-encode against previous frame buffer (or zero baseline for keyframes) using MSG_WriteDeltaEntity into a msg_t buffer.
- Build current player bitmask by checking which client slots are active (client->state >= CS_CONNECTED or equivalent).
- For each active player: read playerState_t from sv.gameClients (offset by sv.gameClientSize per slot — playerState_t is the first member). Delta-encode using MSG_WriteDeltaPlayerstate.
- Write changed configstrings (compare against a cached copy or use a dirty flag if available).
- Write server commands (hook SV_SendServerCommand to capture commands issued this frame).
- Flush the msg_t buffer to file via FS_Write.
- Copy current state into previous frame buffers.
- Increment frame counter.

`SV_MVD_StopRecord()`:

- Write the index table (keyframe offsets)
- Patch the file header with the index table offset
- Close the file handle
- Clear recording state

### 3. Console Commands

- `sv_mvdrecord <filename>` — calls SV_MVD_StartRecord. Defaults to a timestamped filename if none given.
- `sv_mvdstop` — calls SV_MVD_StopRecord.
- `sv_mvdauto <0|1>` — cvar to auto-record every match. Hook into map loading / G_InitGame to auto-start, and into map change / shutdown to auto-stop.

### 4. Server Command Capture

To record server commands (chat, prints), hook into `SV_SendServerCommand`. When MVD recording is active, copy each command into a per-frame buffer before it goes out to clients. Write the buffer as part of the frame block.

## Constraints

- Do NOT modify the game module (QVM) or any shared headers (q_shared.h, bg_public.h, g_public.h, cg_public.h).
- Do NOT modify the per-client snapshot pipeline in sv_snapshot.c. MVD recording is a parallel, independent system.
- Reuse MSG_WriteDeltaEntity and MSG_WriteDeltaPlayerstate from msg.c. Do NOT reimplement delta compression.
- Entity state is read from sv.gentities at the stride sv.gentitySize. The entityState_t is at offset 0 (first member of sharedEntity_t). The entityShared_t follows immediately. Access the `linked` and `svFlags` fields from entityShared_t to determine active status.
- Player state is read from sv.gameClients at the stride sv.gameClientSize. The playerState_t is at offset 0.
- Keep file I/O batched — write one msg_t buffer per frame, not field-by-field.
- Memory budget for recording is small: two full-world buffers (~192 KB) plus a per-frame write buffer (~32 KB msg_t).

## Testing

- Record a match with bots. Verify file size is in expected range (~1.5-2 MB/min for a 4v4).
- Write a standalone verification tool (or test command) that reads the MVD file, parses all frames, and validates: frame count matches expected duration, all delta decodes succeed without errors, keyframes appear at correct intervals, index table entries match actual file offsets.
- Verify recording does not impact server frame timing — measure SV_Frame duration with and without recording active.
- Verify recording across map changes: stop recording on map change, optionally auto-start on new map.
- Test with maximum clients (64) to ensure bitmask and buffer sizes are correct.
