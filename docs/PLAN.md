# FnQL-game Plan

FnQL-game builds clean-room replacements for the retail Quake Live game
modules (`qagamex86.dll`, `cgamex86.dll`, `uix86.dll`) for the FnQL engine.
The baseline is [baseq3a](https://github.com/ec-/baseq3a) (Quake III 1.32
lineage). Quake Live behavior is rewritten here from evidence: retail
observation and the retail HLIL corpus first, the QLSRP reconstruction
(`../QuakeLive-SRP`) as a behavioral reference. Nothing is ported line by line.

The finish line is full functionality and full interoperability:

- an FnQL server running FnQL-game accepts retail Quake Live clients;
- an FnQL client running FnQL-game joins retail servers;
- in both directions nobody can tell, because nothing on the wire differs
  from retail unless both peers negotiated it.

[`COMPATIBILITY.md`](COMPATIBILITY.md) defines that contract. FnQL's
`docs/fnql/GAME_MODULES.md` is the engine half of it.

## Evidence rules

1. **Retail wins.** Enum values, table orders, configstring indices and
   payloads, server command grammar and movement results come from retail
   evidence: the retail binaries, the HLIL dumps under
   `QuakeLive-SRP/references/hlil/quakelive/`, and retail runtime probes.
2. **QLSRP is a behavior map, not a source.** Its game code shows what retail
   does and where. Its implementation text is not copied. Known divergences
   from retail (verified against HLIL, 2026-10-29):
   - QLSRP's low configstring range uses Quake III numbers. Retail uses
     `CS_GAME_VERSION` 12, `CS_LEVEL_START_TIME` 13, `CS_INTERMISSION` 14,
     `CS_ITEMS` 15, `CS_MODELS` 17, `CS_SOUNDS` 273, `CS_PLAYERS` 529,
     `CS_LOCATIONS` 593 and `CS_FLAGSTATUS` 658.
   - Retail publishes pmove settings as an engine-built infostring at CS
     `0x2A9` from cvars flagged `0x4000`. QLSRP uses a positional stream at
     `0x2BE`.
   - QLSRP's match-state keys `ot*`, `to*`, `sd*`, `shuffle*` and `pct` and
     its forced-cosmetics configstring have no retail trace.
   - The retail grappling-hook reload default looks like 200 ms; QLSRP has
     100 ms.
3. **Facts and inferences stay separate** in code comments, docs and commit
   messages. Every compatibility constant cites what it is based on.
4. **The FnQL engine stays engine-only.** Game logic lands here. Engine hooks
   land in FnQL behind the provider and capability rules.

## Validation gates

Every milestone exits through gates, and each gate stays in the suite once it
exists.

| Gate | What it proves | First milestone |
|---|---|---|
| G0 ABI layout | `scripts/check_engine_abi.py`: every structure, offset and constant shared by pointer with the engine matches FnQL | M1 |
| G1 Boot smoke | `scripts/smoke_dedicated.py`: the module loads through the retail ABI, identifies itself, initializes on a retail map, hosts a bot, keeps running | M1 |
| G2 Wire manifest | Machine-readable manifest of retail enums and tables (gametypes, weapons, 60-row item list, powerups, stats, events, MODs, CS indices and payload keys, server commands) compared with FnQL-game's tables | M2 |
| G3 Lockstep differential | FnQL hosts the retail qagame and FnQL-game's qagame in turn with the same map, seed and scripted usercmd streams, and diffs every snapshot entity, playerState, configstring and server command frame by frame | M3 |
| G4 Prediction | FnQL-game cgame predicts retail and FnQL-game servers with zero prediction errors over recorded demo and live corpora | M7 |
| G5 Interop matrix | Retail client ↔ FnQL server, and FnQL client ↔ retail server, both pure and unpure, scripted where possible and manual (Steam) otherwise | M5, M10 |
| G6 Engine non-regression | FnQL's suites, including the retail-module provider tests, stay green on every engine change | M1 |

## Milestones

### M1 — Foundation and qagame bring-up (complete)

- Companion repository registered with FnQL: `AGENTS.md`, `TECHNICAL.md`, and
  the contract `docs/fnql/GAME_MODULES.md`.
- Baseline imported: baseq3a `a3085f7aa6a9242383795b2806b0124401d8a409`, with
  exclusions recorded in [`BASELINE.md`](BASELINE.md).
- Meson build for 32-bit x86 producing `fnql_qagamex86.dll`: static CRT,
  exports `dllEntry` and `FnQL_GetModuleInfo` only, no warnings.
- Retail native ABI for qagame (`code/game/g_native.[ch]`):
  - a typed 207-slot import table pinned to the retail layout;
  - the 19-slot export table in retail order, every entry funnelled through
    the one dispatcher;
  - FnQL extension discovery gated on `//trap_GetValue`.
- Shared structures (`playerState_t`, `entityState_t`, `usercmd_t`,
  `trajectory_t`, `vmCvar_t`, `entityShared_t`) moved to the Quake Live layout
  and held there by G0.
- FnQL engine: `vm_gameProvider`, the module identity probe, `vminfo`
  provenance, the server-module reference exemption, and the slot-173 bot
  import fix, with source and runtime tests (G6).
- Gates G0 and G1 pass.

Known gaps at the end of M1 (the spawn and arena gaps were closed in M2):

- gameplay is still Quake III;
- QL map entities without spawn functions are skipped;
- `scripts/arenas.txt` exceeds the baseline's 8 KB arena buffer;
- the module does not claim `FNQL_MODULE_CAP_RETAIL_WIRE`;
- builds are x86 only (other targets: M11);
- cgame and ui are not built.

### M2 — Wire contract (in progress)

Done (2026-10-29), gate G2 `scripts/check_wire_contract.py`:

- retail item table, MODs, gametypes, pm types, powerups (retail order,
  unlike QLSRP), holdables, persistant powerups, tiers, keys, events,
  configstring indices, spawn table, bot item indices;
- `CS_PLAYERS` and `CS_WARMUP` payload grammars;
- `TR_QL_ACCEL`;
- the 8 retail-only spawn functions;
- the full stock-map sweep (`scripts/sweep_maps.py`).

Left:

- register the retail cvar table, starting with the 33 `pmove_*` cvars
  (flag `0x4000`, published by the engine in CS `0x2A9`), plus
  `weapon_reload_*` and `armor_tiered`. The defaults are already extracted;
- a first G5 run: the retail cgame on an FnQL client joining an FnQL-game
  server.

### M2 plan (original scope)

Rewrite the shared `bg_` layer and the spawn table to the retail contract:

- `gametype_t`: FFA, Duel, Race, TDM, CA, CTF, 1FCTF, Overload, Harvester,
  FT, Domination, AD, RR.
- `weapon_t` with HMG (`WP_NUM_WEAPONS` 15).
- The 60-row `bg_itemlist`, retail order, ending with spawn armor, HMG, HMG
  ammo, ammo pack and the three keys.
- `powerup_t`, `holdable_t`, and the stats and persistant indices.
- `entity_event_t` 0–99 with the retail removals and additions.
- Means of death, eFlags, and `pm_flags` (24 wire bits; `TIME_WATERJUMP` is
  128).
- Configstring indices and payload grammars from HLIL, and the `CS_MATCH_STATE`
  keys `time`, `round`, `turn`, `state`.
- `TR_QL_ACCEL` trajectory evaluation.
- Spawn functions for every classname in the stock map set (149 maps), plus
  the `gametype`/`not_gametype` spawn filters.
- Arena and bot parsing sized for retail script files.

Exit: G2 manifest identical; every stock map spawns with no unknown classname.

### M3 — Movement and prediction parity

Rewrite `bg_pmove`/`bg_slidemove` to retail behavior:

- The 33 pmove settings arrive through the engine-published CS `0x2A9`
  infostring. qagame registers them with the `0x4000` flag.
- Per-player profile bits travel in `pm_flags`.
- Movement: air control, chain/ramp/step jumps, crouch slide and crouch step
  jump, air steps, double jump, autohop versus required jump release.
- Weapons: drop/raise times, the per-weapon reload table, chaingun spin-up.
- No velocity snap.

Build the G3 lockstep harness in FnQL: deterministic scripted clients, a
per-frame recorder, and a differ. The harness is engine tooling, default-off.

Exit: zero divergence against the retail qagame across a scripted movement
corpus on stock maps.

### M4 — Combat, items and spawning

- Weapons: HMG, nailgun, prox, chaingun; railgun headshots; LG discharge; the
  deterministic 20-pellet shotgun ring.
- Damage, knockback, splash and velocity tunables (`g_damage_*`,
  `g_knockback_*`, `g_splash*`, `g_velocity_*`).
- Starting kit and loadouts.
- Item timers (`time`/`time2`/`generic1`), spawn delays, tiered armor, ammo
  pack, quad hog, keys, regen and vampiric rules.
- The QL spawn system.

Exit: G3 differential over combat and item scenarios.

### M5 — Match flow and core gametypes

- Gametypes: FFA, Duel (queue and forfeit), TDM, CTF, 1FCTF.
- Match flow: warmup, ready-up, timeouts, overtime, sudden death,
  intermission and the end-of-match map vote.
- Scoreboards: the `scores_*`/`*stats`/`pstats`/`tinfo` server command
  grammars.
- Voting and autoshuffle.
- Admin tiers from `access.txt`.
- Factory integration: the engine parses factories, the game checks them with
  import 206.
- Stats events (`MATCH_STARTED`, `PLAYER_*`, `ROUND_OVER`, `TEAM_WON`, …)
  through imports 185/186, under FnQL's online-services rules (explicit,
  default-off where services are involved).

Exit: G3 over full matches; first G5 runs with retail clients on an FnQL
server.

### M6 — Round-based and special modes

Clan Arena, Freeze Tag (with the thaw-event export), Attack & Defend, Red
Rover, Race (checkpoints, records), Domination, Harvester, Overload, plus the
instagib, quad hog and infected factory variants. This milestone also fills in
the QL engine queries: visibility, objectives, voice suppression, admin,
enemy relations and score.

Exit: G3 per mode; G5 per mode.

### M7 — cgame

A retail-ABI `fnql_cgamex86.dll`:

- 128 import slots; 21 exports including the reserved NULL slot 16.
- Snapshots and prediction (G4).
- Events, effects, players and weapons.
- The script-driven HUD: ownerdraw IDs match the pak's `ui/menudef.h`.
- Per-gametype scoreboards, server command parsing.
- Chat-field and speaking-state exports.

Exit: G4 on retail and FnQL-game servers; visual parity checks against retail
captures.

### M8 — ui

A retail-ABI `fnql_uix86.dll`:

- 97 import slots; 14 exports.
- The Team Arena menu system: in-game, callvote, admin, join and options
  menus; the connect and advert screens; the offline main-menu fallback.
- Coexists with the engine-hosted WebUI.

### M9 — Bots

Quake Live bot behavior over FnQL's botlib:

- item and weapon weights for QL items;
- gametype goals for every mode;
- chat.

### M10 — Engine integration completion (FnQL)

- `vm_cgameProvider` and `vm_uiProvider` with honest purity. On a pure server,
  an FnQL-game client module is used only when the server lists the FnQL-game
  client package. The client then reports that package's checksum and FnQL
  servers accept it. Retail pure servers always get retail client modules.
- Separate server and client packages:
  - server packages are never referenced to clients (done in M1 for the
    module file);
  - client packages are never required of retail clients.
- Capability negotiation:
  - `fnql_game` serverinfo key (server side);
  - an FnQL-game cgame userinfo key (client side);
  - per-client extension channels;
  - the first FnQL extensions.
- An `auto` provider value that selects FnQL-game only for modules declaring
  `FNQL_MODULE_CAP_RETAIL_WIRE`.
- Verification that the same binaries also run as drop-ins in the retail
  engine.

Exit: G5 matrix complete, pure and unpure.

### M11 — Architecture and platform range

FnQL's Project Plan step 7 takes the engine to FnQ3's full range:

- architectures: x86_64, aarch64, armv7l, ppc64 and ppc64le;
- platforms: Windows, Linux, macOS and the BSDs.

Retail modules exist only for 32-bit x86, so on every other target FnQL-game
is the only game. For each target, in step with the engine:

- **Native modules per architecture.** The structured ABI and the shared
  structures carry no pointer-width assumptions. Each target adds its arch
  suffix, its build and its own G0/G1 run.
- **Optionally, one QVM build for all architectures.** FnQL would first have
  to expose the Quake Live-only imports (float cvar values, bounded cvars,
  match reports, Steam stats, factory checks) through the legacy syscall
  numbering, so a bytecode module can reach them.

The rules for every target:

- The wire contract is architecture-independent (G2), so x86 and non-x86 peers
  interoperate.
- x86 retail interoperability is never regressed.
- A target counts as supported only when its gates pass.

### M12 — Release

- CI builds for Win32 and Linux i386.
- Packaging inside FnQL releases.
- Defaults flipped to `auto` once G2–G5 pass.
- User docs.

Prerequisite: the baseline licensing question in
[`LICENSING.md`](LICENSING.md) is resolved.

## Working conventions

- Small compatibility slices, each with its gate.
- Baseline files keep their headers. New files carry the FnQL-game GPL header.
- 32-bit x86 only. Never validate with 64-bit artifacts.
- Probes run windowed (`+set r_fullscreen 0`) and as cheaply as the question
  allows. Dedicated server first.
