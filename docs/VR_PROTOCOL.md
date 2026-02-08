# Quake 3 VR Head Tracking Protocol

This document describes how VR head orientation is networked in Quake 3 mods
that support VR clients. It covers the three layers involved:

1. **VR Client Engine** (e.g. q3vr, ioq3quest) — packs head angles into usercmds
2. **Server Engine** (e.g. [trinity-server](https://github.com/ernie/Quake3e)) — reads extended button bits from VR clients
3. **Game Mod / QVM** (e.g. [Trinity](https://github.com/ernie/trinity)) — unpacks, stores, transmits, and renders head orientation. Current VR clients implement game/cgame in native libs.

The protocol is backward compatible. VR clients on vanilla servers work normally
(head tracking is just not networked). Flatscreen clients on VR-aware servers
see VR players' head movement.

---

## 1. Negotiation

### Server advertises VR support

At startup, the server engine sets a read-only serverinfo cvar:

```c
Cvar_Get("vr_support", "1", CVAR_SERVERINFO | CVAR_ROM);
```

This appears in the serverinfo string that clients receive on connect.

### Client identifies as VR

At init, the VR client engine sets a read-only userinfo cvar:

```c
Cvar_Get("vr", "1", CVAR_USERINFO | CVAR_ROM);
```

### Client detects VR server

On connect, the client reads serverinfo:

```c
clc.serverSupportsVR = (atoi(Info_ValueForKey(serverInfo, "vr_support")) == 1);
```

If the server does not advertise `vr_support`, the client sends standard 16-bit
buttons and does not pack head data. The game works normally.

### Server detects VR client

When userinfo is parsed, the server checks:

```c
val = Info_ValueForKey(cl->userinfo, "vr");
cl->isVR = (atoi(val) == 1);
```

This flag determines how many bits to read from the buttons field.

### Game mod broadcasts VR status

The game QVM includes a `vr` key in the `CS_PLAYERS` configstring so all
clients know which players are VR:

```c
// In ClientUserinfoChanged (g_client.c):
s = va("n\\%s\\...\\vr\\%s", ..., Info_ValueForKey(userinfo, "vr")[0] ? "1" : "0");
trap_SetConfigstring(CS_PLAYERS + clientNum, s);
```

Client-side, this is parsed into `clientInfo_t.vrPlayer` for scoreboard icons
and other UI.

---

## 2. Usercmd Button Encoding

VR clients pack head orientation into the upper bits of `usercmd_t.buttons`:

### Bit Layout

| Bits  | Field            | Range          | Notes                    |
|-------|------------------|----------------|--------------------------|
| 0-11  | Standard buttons | —              | Unchanged                |
| 12-18 | Head pitch       | -90° to +90°   | 7-bit packed (see below) |
| 19-25 | Head yaw offset  | -90° to +90°   | 7-bit packed (see below) |
| 26-31 | Reserved         | —              | For future use           |

### Encoding

```c
// Clamp to ±80° (human range of motion), then pack into 7 bits
float headPitch = Com_Clamp(-80.0f, 80.0f, hmdorientation[PITCH]);
int pitchPacked = ((int)((headPitch + 90.0f) * 127.0f / 180.0f)) & 0x7F;

// Yaw offset is relative to weapon aim direction
float headYawOffset = Com_Clamp(-80.0f, 80.0f,
    AngleSubtract(hmdorientation[YAW], weaponangles[YAW]));
int yawPacked = ((int)((headYawOffset + 90.0f) * 127.0f / 180.0f)) & 0x7F;

cmd->buttons |= (pitchPacked << 12) | (yawPacked << 19);
```

**Quantization:** 127 levels over 180° = ~1.42° per step.

**Pitch** is absolute (how far the player is looking up/down).

**Yaw offset** is relative to weapon aim. If the player turns their head left
while the weapon stays put, yaw offset increases. If only the weapon rotates,
the offset stays the same.

### Decoding (Game QVM)

```c
int pitchPacked = (ucmd->buttons >> 12) & 0x7F;
int yawPacked   = (ucmd->buttons >> 19) & 0x7F;

client->vrHeadPitch     = (pitchPacked * 180.0f / 127.0f) - 90.0f;
client->vrHeadYawOffset = (yawPacked   * 180.0f / 127.0f) - 90.0f;
```

### VR Detection

The game QVM detects VR data by checking the upper button bits:

```c
if (ucmd->buttons & 0x03FFF000) {
    // VR data present — unpack and set EF_VR_PLAYER
    client->ps.eFlags |= EF_VR_PLAYER;
} else {
    client->ps.eFlags &= ~EF_VR_PLAYER;
}
```

### Head Roll

Roll is **not** packed into buttons. It is sent via the standard
`cmd->angles[ROLL]` channel (16-bit SHORT), clamped to ±60°:

```c
angles[ROLL] = Com_Clamp(-60.0f, 60.0f, hmdorientation[ROLL]);
cmd->angles[ROLL] = ANGLE2SHORT(angles[ROLL]);
```

When connected to a VR-aware server, roll is always sent. On vanilla servers
it is gated by the `vr_sendRollToServer` cvar.

---

## 3. Usercmd Serialization (Wire Protocol)

The standard Quake 3 usercmd serialization writes buttons as 16 bits. VR
clients need 32 bits to include head data. Both the client and server msg.c
must agree on the width.

### Function Signature Change

```c
// Original:
void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);
void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);

// VR-aware:
void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to, int buttonBits);
void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to, int buttonBits);
```

### Client (Write Side)

```c
MSG_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd, clc.serverSupportsVR ? 32 : 16);
```

Inside the function, the only change is:

```c
MSG_WriteDeltaKey(msg, key, from->buttons, to->buttons, buttonBits);  // was: 16
```

### Server (Read Side)

```c
MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd, cl->isVR ? 32 : 16);
```

Inside the function:

```c
to->buttons = MSG_ReadDeltaKey(msg, key, from->buttons, buttonBits);  // was: 16
```

The server's `MSG_WriteDeltaUsercmdKey` does not need the `buttonBits` parameter
because the server never writes usercmds — it only reads them.

---

## 4. Server-Side Game Processing

The game QVM (or DLL) runs on the server and processes VR data in two places.

### Unpacking in ClientThink_real (g_active.c)

After standard button processing:

```c
if (ucmd->buttons & 0x03FFF000) {
    int pitchPacked = (ucmd->buttons >> 12) & 0x7F;
    int yawPacked   = (ucmd->buttons >> 19) & 0x7F;

    client->vrHeadPitch     = (pitchPacked * 180.0f / 127.0f) - 90.0f;
    client->vrHeadYawOffset = (yawPacked   * 180.0f / 127.0f) - 90.0f;
    client->ps.eFlags |= EF_VR_PLAYER;
} else {
    client->ps.eFlags &= ~EF_VR_PLAYER;
}
```

Requires adding to `gclient_s` (g_local.h):

```c
float vrHeadPitch;
float vrHeadYawOffset;
```

### Transmitting in ClientEndFrame (g_active.c)

After `BG_PlayerStateToEntityState`, copy VR data to the entity for network
transmission:

```c
if (client->ps.eFlags & EF_VR_PLAYER) {
    ent->s.angles2[PITCH] = client->vrHeadPitch;
    ent->s.angles2[ROLL]  = client->vrHeadYawOffset;  // ROLL slot repurposed

    // Also pack into playerState stats for demo playback
    client->ps.stats[STAT_VR_HEAD_PITCH]      = (short)(client->vrHeadPitch * 182.04f);
    client->ps.stats[STAT_VR_HEAD_YAW_OFFSET] = (short)(client->vrHeadYawOffset * 182.04f);
}
```

The factor `182.04 = 32767 / 180` maps degrees to the short range.

### BG_PlayerStateToEntityState (bg_misc.c)

For the local player's entity (used in demos and first-person follow), derive
`angles2` from stats:

```c
if (ps->eFlags & EF_VR_PLAYER) {
    s->angles2[PITCH] = (float)ps->stats[STAT_VR_HEAD_PITCH] / 182.04f;
    s->angles2[ROLL]  = (float)ps->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f;
}
```

### Required Definitions (bg_public.h)

```c
#define EF_VR_PLAYER  0x00000400  // Shares bit with EF_MOVER_STOP (safe: movers aren't players)

// In statIndex_t enum:
STAT_VR_HEAD_PITCH,           // VR head pitch (packed as short)
STAT_VR_HEAD_YAW_OFFSET       // VR head yaw offset from weapon direction (packed as short)
```

`EF_VR_PLAYER` uses bit 10 (`0x00000400`), which is shared with `EF_MOVER_STOP`.
This is safe because `EF_MOVER_STOP` only applies to mover entities (doors,
platforms), never to players.

---

## 5. EntityState Wire Format

VR head data is carried on existing `entityState_t` fields:

| Field            | EntityState slot   | Semantics                            |
|------------------|--------------------|--------------------------------------|
| Head pitch       | `angles2[PITCH]`   | Absolute pitch angle (float degrees) |
| Head yaw offset  | `angles2[ROLL]`    | Relative to weapon yaw (repurposed)  |
| Movement dir     | `angles2[YAW]`     | Unchanged (legs animation)           |
| VR player flag   | `eFlags`           | `EF_VR_PLAYER` (0x00000400)          |

`angles2[ROLL]` is repurposed for yaw offset because the ROLL slot was unused
on player entities, and head roll comes through the standard viewangles
networking (lerpAngles[ROLL]).

---

## 6. Client-Side Game Rendering

### 3rd Person Model (CG_PlayerAngles in cg_players.c)

For VR players (`EF_VR_PLAYER` set), the cgame:

1. **Interpolates** VR head pitch and yaw from `angles2` using
   `CG_SwingAngles()` as a low-pass filter
2. **Skips torso swing** — torso follows weapon aim 1:1 instead of using the
   standard swing tolerance. This is necessary because the head yaw offset is
   relative to weapon aim, so the torso must track weapon aim exactly for the
   head to appear correct.
3. **Computes head angles** relative to torso using matrix math (inverse torso
   rotation applied to world-space head orientation)
4. **Applies biological limits**: pitch ±80°, yaw ±80°, roll ±60°

Requires adding to `playerEntity_t` (cg_local.h):

```c
float vrHeadPitch;
float vrHeadYawOffset;
```

### 1st Person Follow / Demo Playback (cg_view.c)

When spectating or watching a demo of a VR player, the camera looks through
the player's head (not weapon). The 7-bit quantization (~1.42° steps) causes
visible jitter if used directly, so an exponential moving average smooths it:

```c
float targetPitch = (float)ps->stats[STAT_VR_HEAD_PITCH] / 182.04f;
float targetYaw   = ps->viewangles[YAW]
    + (float)ps->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f;

// EMA: tau ~30ms smooths quantization steps without perceptible lag
float alpha = (float)cg.frametime / ((float)cg.frametime + 30.0f);
vrViewPitch += alpha * AngleSubtract(targetPitch, vrViewPitch);
vrViewYaw   += alpha * AngleSubtract(targetYaw,   vrViewYaw);
```

### Interpolation Between Snapshots (cg_predict.c)

In `CG_InterpolatePlayerState`, VR stats are interpolated the same way as
viewangles — unpack to float, LerpAngle, repack:

```c
out->stats[STAT_VR_HEAD_PITCH] = (int)(LerpAngle(
    (float)prev->ps.stats[STAT_VR_HEAD_PITCH] / 182.04f,
    (float)next->ps.stats[STAT_VR_HEAD_PITCH] / 182.04f, f) * 182.04f);
```

### Local VR Player Mirror Rendering (cg_ents.c, VR client only)

The VR client sets head angles directly on the predicted player entity for
mirror rendering, bypassing the network round-trip:

```c
if (vr && !cg.demoPlayback) {
    cg.predictedPlayerEntity.currentState.angles2[PITCH] = vr->hmdorientation[PITCH];
    cg.predictedPlayerEntity.currentState.angles2[ROLL]  =
        AngleSubtract(vr->hmdorientation[YAW], vr->weaponangles[YAW]);
    cg.predictedPlayerEntity.currentState.eFlags |= EF_VR_PLAYER;
}
```

---

## 7. Data Flow

```
VR Headset (OpenXR)
    |
    v
VR Client Engine (cl_input.c)
    |  Pack pitch/yaw into buttons bits 12-25
    |  Send roll via cmd->angles[ROLL]
    |  Write 32-bit buttons in usercmd
    v
Server Engine (sv_client.c)
    |  Read 32-bit buttons for VR clients, 16-bit for flatscreen
    |  Pass usercmd to game QVM
    v
Game QVM - Server Side (g_active.c)
    |  Unpack buttons → float vrHeadPitch/YawOffset
    |  Set EF_VR_PLAYER on eFlags
    |  Copy to entityState.angles2 and playerState.stats
    v
Server Engine Snapshot
    |  Transmit entityState to all clients
    v
Game QVM - Client Side (cg_players.c, cg_view.c)
    |  3rd person: interpolate + render head model
    |  1st person follow: EMA smooth + set refdef viewangles
    |  Follow HUD: show VR icon (via EF_VR_PLAYER)
    |  Scoreboard: show VR icon (via configstring)
    v
Screen
```

---

## 8. Compatibility Matrix

| Server Engine | Client Engine | Game Mod  | Result                        |
|---------------|---------------|-----------|-------------------------------|
| VR-aware      | VR            | VR-aware  | Full head tracking            |
| VR-aware      | Flatscreen    | VR-aware  | Normal play, sees VR heads    |
| Vanilla       | VR            | Any       | Works, no head tracking sent  |
| Vanilla       | Flatscreen    | Any       | Normal                        |
| VR-aware      | VR            | Vanilla   | VR data in usercmd, but mod ignores it |

---

## 9. Implementation Checklists

### For VR Client Engine Authors

1. Set `vr=1` in userinfo (`CVAR_USERINFO | CVAR_ROM`)
2. Read `vr_support` from serverinfo on connect
3. In `CL_FinishMove`: pack head pitch and yaw offset into `cmd->buttons`
   bits 12-25 (only when server supports VR)
4. In `CL_WritePacket`: call `MSG_WriteDeltaUsercmdKey` with `buttonBits=32`
   when server supports VR, `16` otherwise
5. Update `MSG_WriteDeltaUsercmdKey` and `MSG_ReadDeltaUsercmdKey` signatures
   in msg.c / qcommon.h to accept `buttonBits`
6. Always send roll via `cmd->angles[ROLL]` when server supports VR
7. For mirror rendering: set `angles2` directly on predicted entity from HMD

### For Game Mod (QVM/DLL) Authors

**Server-side game:**
1. Define `EF_VR_PLAYER` (0x00000400) and `STAT_VR_HEAD_PITCH` / `STAT_VR_HEAD_YAW_OFFSET`
2. Add `vrHeadPitch` / `vrHeadYawOffset` floats to client struct
3. In `ClientThink_real`: unpack bits 12-25 from buttons, set/clear `EF_VR_PLAYER`
4. In `ClientEndFrame`: copy to `ent->s.angles2[PITCH/ROLL]` and pack into stats
5. In `BG_PlayerStateToEntityState`: derive `angles2` from stats for local player
6. In `ClientUserinfoChanged`: include `vr` flag in `CS_PLAYERS` configstring

**Client-side game:**
1. In `CG_PlayerAngles`: check `EF_VR_PLAYER` → interpolate head, skip torso swing,
   compute head-relative-to-torso, apply biological limits
2. In `CG_CalcViewValues`: EMA smooth VR stats for 1st-person follow/demo
3. In `CG_InterpolatePlayerState`: LerpAngle on VR stats between snapshots
4. Parse `vr` from configstring for scoreboard icon

### For Server Engine Authors

1. Add `qboolean isVR` to client struct (server.h)
2. Set `vr_support=1` serverinfo at startup (sv_init.c)
3. Parse `vr` from userinfo, set `client->isVR` (sv_client.c)
4. In `SV_UserMove`: read buttons with `cl->isVR ? 32 : 16` (sv_client.c)
5. Add `buttonBits` parameter to `MSG_ReadDeltaUsercmdKey` (msg.c, qcommon.h)

---

## 10. Key Constants

| Constant              | Value          | Purpose                                       |
|-----------------------|----------------|-----------------------------------------------|
| `EF_VR_PLAYER`        | `0x00000400`   | Entity flag identifying VR players            |
| Button mask           | `0x03FFF000`   | Detects VR data in buttons (bits 12-25)       |
| Encoding factor       | `127.0 / 180.0`| Degrees → 7-bit packed value                  |
| Decoding factor       | `180.0 / 127.0`| 7-bit packed value → degrees                  |
| Stats packing factor  | `182.04`       | Degrees → short (32767 / 180)                 |
| Pitch client clamp    | ±80°           | Applied before packing                        |
| Yaw client clamp      | ±80°           | Applied before packing                        |
| Roll client clamp     | ±60°           | Applied to `cmd->angles[ROLL]`                |
| Pitch display clamp   | ±80°           | Applied in cgame 3rd-person rendering         |
| Yaw display clamp     | ±80°           | Applied in cgame 3rd-person rendering         |
| Roll display clamp    | ±60°           | Applied in cgame 3rd-person rendering         |
| EMA tau               | 30 ms          | 1st-person follow smoothing time constant     |
