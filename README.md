# Trinity

Unified Quake 3 mod codebase that builds both baseq3 and missionpack (Team Arena) QVMs from a single source tree.

## Features

### From baseq3a

- new toolchain used (optimized q3lcc and q3asm)
- upstream security fixes
- floatfix
- fixed vote system
- fixed spawn system
- fixed in-game crosshair proportions
- fixed UI mouse sensitivity for high-resolution
- fixed not being able to gib after match end (right before showing the scores)
- fixed shotgun not gibbing unless aiming at the feet
- fixed server browser + faster scanning
- fixed grappling hook muzzle position visuals
- new demo UI (subfolders,filtering,sorting)
- updated serverinfo UI
- map rotation system
- unlagged weapons
- improved prediction
- damage-based hitsounds
- colored skins
- high-quality proportional font renderer
- single-line cvar declaration, improved cvar code readability and development efficiency
- single-line event (EV\_\*) declaration
- single-line mean of death (MOD\_\*) declaration

### From missionpackplus

- Fixed UI scaling
- CPMA-style fullbright skins

### New for Trinity

**Client**

All of these settings are available in the options menu.

- `cg_damagePlums`: Quake Live style colored damage numbers on hit (requires server support for clients)
- `cg_damageEffect`: Modern red-border damage indication, directionally-weighted
- `com_blood`: `0` off, `1` classic blood, `2` modern (default) — animated blood gouts, gib blood spray, dense trails, and richer surface splats color-matched to Q3's dark blood. On Trinity servers, modern blood is aggregated per victim and scaled by damage; on vanilla servers it falls back to per-hit. Set in the Game Options menu.
- `cg_followMode`: Orbit camera for spectating and demo playback. Mouse orbits around the followed player, forward/back keys zoom in and out. When following, weapon next/prev and zoom keys adjust distance and toggle follow mode, respectively, toggling between first and third-person.
- Playback of TV demos (TrinityVision, of course :wink:). These `.tvd` files are server-side recorded, and work like other MVD systems. Requires the client engine support. Currently, [specific to trinity-engine](https://github.com/ernie/trinity-engine)

You can also set your own bindings. The default ones behave like this, but only in follow mode, so they don't override normal gameplay:

```
bind mouse2 followcam
bind mwheelup followzoomin
bind mwheeldown followzoomout
```

TVD playback supports timeline scrubbing. Hold the bound key and move the mouse left/right to select a position on the timeline. A vertical indicator and time label show the target position. Release the key to seek there, or press ESC to cancel.

```
bind mouse3 +tv_scrub
```

Other TVD playback binds:

```
bind leftarrow tv_backward   # skip backward (cg_tvSkip seconds, default 10)
bind rightarrow tv_forward   # skip forward
bind uparrow tv_next         # next player
bind downarrow tv_prev       # previous player
```

**VR Support**

- Independent head and torso tracking for VR clients, backwards-compatible with flatscreen players
- VR player head movement captured in demos for playback
- VR player icon displayed on the scoreboard

**Server**

- Support for clients with `cg_damagePlums`
- `g_mode`: Selectable game mode — a single latched profile that picks both the movement physics and the combat ruleset, synchronized to clients for prediction. Voteable (can use `vq3` / `cpm` / `ql` / `qlt` there). Accepts numeric or string values:
  - `0` — Vanilla Quake 3 (default)
  - `1` — CPMA/promode (higher ground accel/friction, strafe air control, double-jump; 3-tiered armor, instant weapon switch, damage adjustments)
  - `2` — Quake Live (auto-hop, jump velocity 275; damage adjustments, cg_trueShotgun)
  - `3` — Quake Live Turbo (Quake Live combat with CPMA-style strafe air control + ramp jump)
- `g_teamDMSpawnThreshold`: Set > 0 to include Team (CTF) spawns in maps with fewer deathmatch spawns than the value. Allows use of maps that are otherwise a telefrag-fest in Team DM.

The per-mode movement and combat values were determined by dumping cvars where possible, followed by community resources, then trial and error until things felt right. Bug reports of differences in behavior from the _default_ settings for CPMA/Quake Live are greatly appreciated.

## Prerequisites

- GCC or compatible C compiler
- GNU Make
- 7-Zip (for pk3 packaging)

### Windows

Install [MSYS2](https://www.msys2.org/) and from the MSYS2 terminal:

```bash
pacman -S mingw-w64-x86_64-gcc make p7zip
```

### Linux

```bash
# Debian/Ubuntu
sudo apt install build-essential p7zip-full

# Arch
sudo pacman -S base-devel p7zip
```

## Building

### 1. Clone with submodules

```bash
git clone --recursive git@github.com:ernie/trinity.git
cd trinity
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

### 2. Build

```bash
make
```

This builds the compiler tools and both pk3 files. Output goes to `dist/`:

- `dist/pak8t.pk3` (baseq3)
- `dist/pak3t.pk3` (missionpack)

### Building individually

```bash
make tools        # Build compiler tools only
make qvms         # Build QVMs only (requires tools)
```

## Cleaning

```bash
make clean        # Clean QVM build artifacts
make clean-tools  # Clean compiled tools
make clean-all    # Clean everything
```

## Installation

Copy the pk3 files from `dist/` to your Quake 3 installation:

- `pak8t.pk3` → `baseq3/`
- `pak3t.pk3` → `missionpack/`

## Project Structure

```
trinity/
├── Makefile        # Top-level build
├── code/
│   ├── cgame/      # Client (rendering, effects, HUD)
│   ├── game/       # Server (gameplay logic, AI)
│   ├── q3_ui/      # baseq3 UI
│   └── ui/         # missionpack (Team Arena) UI
├── assets/         # Game assets (menus, shaders, etc.)
├── build/          # QVM build system
├── tools/
│   ├── q3asm/      # Assembler (submodule)
│   └── q3lcc/      # Compiler (submodule)
├── dist/           # Build output (pk3 files)
└── ui/             # Menu definitions
```

## Credits

Trinity was initially based on ec-/baseq3a with some additions from Kr3m/missionpackplus.

This mod would not be possible without their amazing work.

Trinity bundles HD asset improvements derived from **High Quality Quake 3 Team Arena (HQQ3)** by **ZerTerO** ([ModDB profile](https://www.moddb.com/members/zertero)) — specifically HD UI graphics, weapon and player model textures, additional player model geometry (fritzkrieg, pi, james, janet, and head variants), HD HUD elements, and HD levelshots.

## Documentation

- [Color schemes reference](docs/COLOR_SCHEMES.md) — chat `^N`, `color1`/`color2`, `cg_enemyColors`, the UI slider, and how they all map to each other
- [VR protocol](docs/VR_PROTOCOL.md) — how VR head tracking is networked
