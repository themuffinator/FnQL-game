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
| G3a Pmove lockstep | `scripts/pmove_lockstep.py` and `tests/lockstep/`: the retail qagame's `Pmove` and FnQL-game's run the same scripted and fuzzed command streams in one process, and every `playerState_t` and `pmove_t` output matches bit for bit | M3 |
| G3 Lockstep differential | `scripts/server_lockstep.py` and `tests/lockstep/server/` (suite `differential`): FnQL's `sv_lockstep` dedicated server hosts the retail qagame and FnQL-game's qagame in turn with the same map, seed and scripted usercmd streams, and diffs every client-visible entity, scripted playerState, configstring and server command frame by frame; `--self-check` (retail twice) must show zero divergence | M4 |
| G4a Retail prediction | `scripts/prediction_probe.py`: the retail cgame predicts FnQL-game's server-side movement on a stock map with no prediction error at all, VQL and PQL | M3 |
| G4 Prediction | FnQL-game cgame predicts retail and FnQL-game servers with zero prediction errors over recorded demo and live corpora | M7 |
| G5a Retail cgame probe | `scripts/listen_probe.py`: the retail cgame joins an FnQL-game listen server, reaches CS_ACTIVE, no errors | M2 |
| G5b Retail client join | `scripts/retail_client_probe.py` (opt-in suite `retail-client`): the user's retail `quakelive_steam.exe`, launched windowed through Steam, joins an FnQL dedicated server running FnQL-game (pure) and stays in the game | M3 |
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
- Gates G0 and G1 pass. Re-verified 2026-10-29 against the current tree:
  G0 `engine-abi-parity`, G1 `dedicated-smoke` and `map-sweep`, and FnQL's G6
  `game_module_provider` source and runtime tests (6 runtime cases).

Known gaps at the end of M1 (the spawn and arena gaps were closed in M2):

- gameplay is still Quake III;
- QL map entities without spawn functions are skipped;
- `scripts/arenas.txt` exceeds the baseline's 8 KB arena buffer;
- the module does not claim `FNQL_MODULE_CAP_RETAIL_WIRE`;
- builds are x86 only (other targets: M11);
- cgame and ui are not built.

### M2 — Wire contract (complete)

Done (2026-10-29), held by gate G2 `scripts/check_wire_contract.py` against
`docs/contract/`:

- **Tables and enums:** the retail item table, MODs, gametypes and pm types;
  powerups in retail order (unlike QLSRP); holdables, persistant powerups,
  tiers, keys and events; configstring indices; the spawn table; bot item
  indices.
- **Payload grammars:** `CS_PLAYERS` and `CS_WARMUP`, and the first/second
  place names (`CS_FIRST_PLACE`/`CS_SECOND_PLACE`).
- **Published cvars:** the 59 cvars the engine publishes to clients
  (pmove, weapon reload and gravity, appearance, tiered armor), with retail
  names, default strings and flags. The full retail cvar table is in
  `retail_tables.json` for later milestones.
- **Spawning:** `TR_QL_ACCEL`, the eight retail-only spawn functions, and the
  retail spawn filters. `notfree`/`notteam`, the `gametype`/`not_gametype`
  token lists, and the per-gametype exemptions are recovered from the retail
  item row and the Attack and Defend obelisks.
- **Sweep:** the full stock-map sweep (`scripts/sweep_maps.py`).
- **First interoperability gate, G5a (`scripts/listen_probe.py`):** an FnQL
  client runs the retail `cgamex86.dll` against FnQL-game's qagame. It
  reaches `CS_ACTIVE` with a bot, and the retail HUD shows the warmup
  countdown, placements and items.

Moved on:

- the `CS_MATCH_STATE` payload goes to M5, with the match flow that writes
  it;
- the remaining Quake Live `pm_flags` and eFlags bits go to M3, where
  movement parity exercises them.

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

### M3 — Movement and prediction parity (complete)

Done (2026-10-29). `bg_pmove.c` and `bg_slidemove.c` follow the retail
qagame function by function (addresses in the code comments):

- **Arithmetic.** Retail computes in x87 registers at double precision and
  rounds only where it stores, so the rewrite writes every expression in
  double in the retail order, with retail forms of `VectorNormalize`,
  `AngleVectors` (and its float-rounded degree constant), the dot and cross
  products. SSE2 and x87 builds both match retail bit for bit; GCC and Clang
  builds use `-ffp-contract=off`, plus SSE math on x86.
- **Movement profile.** Every frame the VQL constants, or the PQL set under
  `PMF_AIR_CONTROL`, override `pmove_AirAccel`, `AirStopAccel`,
  `StrafeAccel`, `WalkAccel`, `CircleStrafeFriction`, `WishSpeed` and
  `ChainJump`. Those cvars do not affect retail movement, and now do not
  affect ours.
- **Movement.** PQL air control, strafe acceleration and circle-strafe
  friction; the bunny-hop boost; chain, ramp, step and crouch step jumps; air
  steps and air-step friction; double jump; crouch slide; autohop and required
  jump release; ladders; the flight holdable; the grapple pull; QL water
  levels; no slope speed restore; no velocity snap.
- **Step smoothing.** Steps report `pmove_t.stepUp`/`stepTime`; there are no
  step events.
- **Weapons.** Per-weapon reload times from `weapon_reload_*`, chaingun
  spin-up, haste and scout rates, weapon 15, attack lockout and weapon reset,
  and the medkit latch.
- **Animation and landings.** QL footsteps (walk below 32 ups, scouts silent,
  snow and wood steps) and QL crash-land thresholds.
- **Game side.** `ClientSpawn` sets the per-player flags from
  `pmove_CrouchSlide`, `pmove_DoubleJump` and `pmove_AirControl`, and from
  `cg_autoHop` under `pmove_AutoHop`. `pmove_fixed` is gone, since retail has
  none.
- **Gates.**
  - G3a: 54 scenarios, 45,710 frames, no divergence. They cover running at
    3, 8 and 16 msec, VQL and PQL strafe jumping, stairs, ramps, slopes, ice,
    water, ladders, flight, grapple, weapons, powerups, dead, spectator,
    noclip, frozen, intermission, and fuzzed input and game state.
  - G4a: zero retail-cgame prediction errors on campgrounds and bloodrun,
    and with PQL, crouch slide and double jump on.
  - G5b (2026-10-29): the retail Steam client `quakelive_steam.exe` joined
    an FnQL dedicated server running FnQL-game on campgrounds (FFA,
    `sv_pure 1`). It entered the game, stayed in with a bot, and left no
    errors in either log. A directly started retail client cannot initialize
    Steam, so the probe launches it through Steam.

Open after M3:

- the game logic that sets `PMF_ATTACK_LOCKOUT` during round countdowns
  goes to M5;
- what the game sets `pmove_t.grappleSlow` (retail `+0xe8`) for is not yet
  identified (M4, with the hook);
- the full-server differential G3 moves to M4, where combat makes it worth
  its cost.

M3 plan (original scope):

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

G3 harness (2026-10-29): FnQL's default-off `sv_lockstep` mode (FnQL
`docs/fnql/SERVER_LOCKSTEP.md`) and `scripts/server_lockstep.py`, with the
scenarios `ffa_two_players` and `ffa_bot_restart`. Both self-checks pass:
two retail runs agree on every recorded field except `g_levelStartTime`,
which retail reads from the CRT clock and the differ compares by shape. The
differentials fail, as expected before M4: the first divergence is at
frame 0, where retail puts a connecting player on `TEAM_SPECTATOR`
(`pm_type` 2, `PERS_TEAM` 3) and FnQL-game spawns it into the game.

M4 progress (2026-10-29, in progress). Done, each from the retail
qagame (HLIL/disassembly addresses in the code comments):

- Combat: `G_Damage` (armor save, knockback scales and caps, plums, hit
  tiers, self and team rules, vampiric damage), radius damage with the splash
  and rocket offsets and `g_knockback_z(_self)`, retail `CanDamage` probes,
  `LogAccuracyHit`, the original `SnapVectorTowards`.
- Weapons: one retail muzzle point (5 units, 3 ducked); gauntlet target and
  hit split; bullets (MG/HMG/CG spreads, ironsights); the 20-pellet shotgun;
  railgun (pierce, impact events, `g_railJump`, the headshot bonus on
  contents 0x400); lightning gun (falloff, one invulnerability bounce);
  nailgun (`g_nailcount`/`spread`/`speed`, gravity, bounces); projectile
  launches through one retail spawn helper (one server frame of prestep, a
  +-0.01 box, `g_velocity_*`/`g_damage_*`/`g_splash*`, accelerating
  missiles, `g_guidedRocket`); `G_BounceMissile`, `G_ExplodeMissile`,
  `G_MissileImpact` order (prox sticks first, event before damage).
- Init and joining (driven by G3): serverinfo/systeminfo carry exactly
  retail's keys (the contract now checks both directions and the spelling),
  the init-time configstrings, sound order, match GUID, `SP_worldspawn`
  (PRE_GAME warmup, map-only gravity), everyone starting as a spectator
  (bots join through their AI), `priv`, retail team-change broadcasts, the
  spawn kit (`g_startingWeapons`/`Ammo`/`Health`/`Armor`) and the warmup
  loadout, spectator state.
- Gates: `combat-soak` (runtime): five bots fight on a retail map and must
  score kills with several means of death.

- Death and kill credit (driven by the `ffa_kill_restart` scenario):
  retail `player_die` (0x10047200): the death event without a parameter, the
  body and view turned towards the killer instead of Quake III's dead yaw
  (the slot is retail's battle suit frag count), `g_respawn_delay_min` plus
  `g_allowKill` after a suicide, which always gibs, the gib direction from
  the last damage, gibbed players' invisible bodies kept in the body queue,
  `EV_SHOTGUN_KILL`, the score plum's client, `AddScore`'s first and latest
  scorers, `g_quadHog` scoring, `g_returnFlagOnSuicide`, keys going home,
  `g_allowKill`/`MOD_SWITCHTEAM` in the kill and team commands, and the
  powerups-first `TossClientItems` with retail's `Drop_Item` toss.
- QL medals and rewards: `EV_AWARD` (first frag, revenge, combo kill,
  midair over `g_midAirMinHeight`, headshot, perforated, quad god, rampage),
  excellent and humiliation, from the damage history `G_Damage` now keeps.
- Match statistics (retail gclient +0x568): kills, deaths, suicides, team
  kills, streaks, per-weapon shots/hits/kills/deaths/damage, damage given and
  taken; kept across respawns and `map_restart`, restarted on a new session
  and on team changes.
- Items (driven by the `ffa_items` scenario, which `setviewpos` under
  `--cheats` drives over shards, weapons, ammo and both armors through a
  25-second respawn): retail `BG_CanItemBeGrabbed` (0x1002ced0: the
  dropper's one-second lockout, weapon stay, domination ammo, tiered armor,
  each mode's flag rules), `Touch_Item` (0x1004f020: no pickups during the
  countdown, respawn delays in seconds, `g_itemTimers` keeping taken timed
  items visible with their respawn time, `bot_itemDelayTime`), every
  `Pickup_*` (`g_weaponRespawn`/`g_ammoRespawn`/`g_powerupRespawn`, tiered
  armor, `g_quadHogTime`, the flight holdable's `g_flight*`, runes, keys),
  the pickup announcements, `RespawnItem`, the spectators' item timers
  (`EV_ITEM_PICKUP_SPEC`, `g_specItemTimers`, the `specresp` command, CTF's
  item sides), the item-control statistics per player and per team, and
  `TeleportPlayer` (0x1005a420).
- The retail scoreboards (0x1003cbf0): `scores_ffa` (0x1003d090, 18 fields
  a player, best weapon by damage), `scores_race`, `scores_tdm`/`scores_ft`
  /`scores_ca`/`scores_ctf` (the flag modes and domination) with the team
  item-control header each side sees only for its own team until the
  intermission, Overload's `scores`, the `smscores` fallback with
  `g_forceSmallScoreboardMessage`, and the intermission's `tdmstats`/
  `castats`/`ctfstats`, and `scores_duel` (0x1003d2b0: the duelists in
  client order, each with per-weapon hits, shots, accuracy, damage and
  kills and the armor and mega item control, which a duelist sees for its
  opponent only from the spectators; the intermission shows the last full
  entries even after a duelist has gone). `scores_rr` and `pstats` remain.

G3 status: `ffa_two_players`, `ffa_kill_restart` and `ffa_items` pass with
no divergence (two scripted players: movement, firing, a suicide, the
respawn delay, the body queue, the scoreboard, `map_restart`, and 1,400
frames of item pickups and respawns). `ffa_bot_restart` diverges
only on the bot's own entity (803 field-frames, from 77,773 and then 2,505):
its movement drifts by a unit from frame 32, so bot AI parity (M9) is all
that is left there. The random streams match retail draw for draw (FnQL
`sv_lockstepTraceRand` traces the retail module's `rand()` and re-arms after
a `map_restart` reloads it; FnQL-game's `-Dtrace-rand=true` traces its own):
the Microsoft runtime's generator on every platform, the retail spawn picker
(qagamex86 0x10039080), item spawning (`g_ammoPack`, `g_spawnItem*`, the
one-unit-up snap), the per-frame `trigger_push` aim (QL's frame think,
retail 0x10059370), the intermission pick at init, and the stats reset on a
new session and on team changes. Also matched: the spawn teleport event,
the whole-unit player `trDelta`, ammo pack and ammo box grabbing and pickup
(retail weapon table maxima), the armor mirror, and the item touch box
(`g_itemHeight`).

`tdm_items` (team deathmatch under `--cheats`) passes with no divergence
since the team-mode slice: `g_loadout`'s kit (the session's loadout weapon,
gclient +0x358: the heavy machinegun for players, a random loadout weapon
for bots, minus `g_disableLoadout`; how a player picks another is not yet
identified in the retail qagame), the spawn weapon (the loadout weapon, or
the first held from `g_bestStartingWeapons`, 0x1003b5a0), which spawns
fire their spot's targets, team locations every frame (0x100356de) with
retail's nearest-location test, the `tinfo` overlay (0x10068490: up to
eight teammates' numbers a second after a team spawn or a disconnect),
retail respawning (0x10039f20: attack or use once the time has come,
forced `g_respawn_delay_max` later, at once during a round mode's
countdown), and no new-team-leader print.

The `factories` suite runs that scenario under every other retail factory
on campgrounds and fails only on a fatal error. It found two crashes, both
fixed: a mode whose spawns the map lacks now defers the spawn as retail
does (0x1003bcfa: 600 msec as a spectator, then after `g_maxDeferredSpawns`
any deathmatch spot) instead of `G_Error`, and Harvester registers QL's
"Red Skull"/"Blue Skull" rather than Team Arena's cubes. The remaining
per-mode divergences are milestone 6: race's untimed score, the round
modes' kits and match state, Harvester's skull toss, and retail's repeated
flag-return prints on a map without flags. Duel's queue and scoreboard
moved to M5 and pass (below).

The G4a PQL prediction probe missed twice in the crouch segment in 18 runs
on a busy machine (clean on reruns; no movement code changed); it needs a
reproduction under controlled frame timing.

Open in M4: grapple impact and hook behaviour, rocket damage through thin
ceilings (`g_forceDmgThroughSurface`, radius damage 0x1004a3e0), LG
discharge (0x1004a7a0), the `g_loadout` choices, retail lag compensation
(`g_lagHaxMs`; baseq3a's unlagged stays meanwhile), `g_dropDamagedHealth`,
quad hog's quad respawn, and whether the engine marks capsule head hits with
contents 0x400 (FnQL does not; retail's engine is unverified). Deferred to
M5/M6 with their modes: team-kill complaints (`g_complaintLimit`), freeze
tag's freezing and thawing, red rover's infection scoring, the round modes'
respawn rules, and retail's Steam achievements and ZMQ stats reports (engine
imports 185/186 and 201-204, under the online-services rules).

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

Progress (G3, both with no divergence):

- `duel_queue`: the duel queue. Retail keeps `so` (spectate-only) and `pq`
  (queue position) in the session at gclient +0x36c/+0x370. `so` starts at
  1 in duel (0x10065acd); the spectate team commands set it with "You are
  set to spectate only" (0x100406d0) and "team f" clears it with "You are
  in the queue to play" (0x10040bd1); a duel bot's ClientBegin clears it
  (0x1003b188). 0x1003a270 numbers the queue by spectator time and
  0x10058410 refreshes the players' keys by flipping `teamtask`. SetTeam
  returns the team or -1 and re-runs a spectator's repeated request
  (0x10040cbe); a free spectator starts on client 0 (0x10040715), so it
  hears client 0's deaths like a follower; `Cmd_Team_f` names the team
  ("Team Free"), honours `g_switchTeamDelay` (3 s) and books a loss for a
  duelist leaving a match (0x10040d90). ClientBegin restarts the whole
  player state (0x1003b030: the spawn count too). `scores_duel`
  (0x1003d2b0) as under M4.
- `duel_match` (1,827 frames): ready-up and the countdown, during which
  every connected client carries `PMF_ATTACK_LOCKOUT` (0x10057a9b); the
  match starts by `map_restart 0` with `g_restarted` set (0x10058860), not
  baseq3a's in-place reset; a duelist leaving mid-match forfeits � retail
  tests it in CalculateRanks before the sort (0x1005612b), in the warmup
  check and in CheckExitRules (0x10057cd0/0x10057f10: -999 for the missing
  duelist or empty team, "Game has been forfeited."); `LogExit`
  (0x10057510: queues the intermission, logs the scores, and broadcasts
  `EV_GAMEOVER` or the winning team's sound); the intermission 200 msec
  later (0x10058030), `BeginIntermission` (0x10056cb0: the duel result in
  the `w`/`l` keys, the three `nextmaps` choices in `CS_ROTATION_TITLES`
  and `CS_ROTATION_VOTES`, `CS_VOTE_TIME`), `MoveClientToIntermission`
  (0x10056b30: only a follower is unlinked; the player state, command time
  included, freezes), the map vote (`vote 1-3`, 0x10043ff0: two-second
  spacing, `disable_vote_ui`, and `enable_vote_ui` 2.25 s later from
  0x10059603), an attack press toggling ready (0x10034d1f),
  `CS_INTERMISSION_EXIT` after 20 s and `ExitLevel` 3 s later
  (0x10057ad0/0x10057000: the most-voted map, a tie drawn at random, and
  `map_restart` when it is the same map). From the match's end only
  `readyup`, `vote` and the chat and score commands are taken
  (0x10045fe3). The run stops at frame 1,827: frame 1,828 loads the voted
  clan arena map, whose kit and rounds are M6.
- `duel_forfeit`: the `forfeit` command (0x100456b0, 0x10057cd0): not
  during a timeout or a round countdown, nor in free-for-all, race or red
  rover; under `g_allowForfeit`, outside warmup, and in duel only for the
  trailing player (score -999), in team games only for the last player of
  the losing team.
- `tdm_match`: a dedicated server holds a team game for `g_warmupDelay`
  before counting down; a cancelled countdown releases the attack lockout
  (0x10058796); `AddTeamScore` (0x100681b0: no origin, nothing after the
  match ends, the tie, scored and took-the-lead calls at the world origin,
  none for the score reaching the capture limit); red forfeits when its
  last player leaves; `tdmstats` at the intermission. ClientConnect greets
  only a first connection outside training (0x1003aede: `priv`,
  "connected", and the team announcement, which a `map_restart` no longer
  repeats). Items first think two server frames after spawning
  (0x10050b0a: `2 * 1000 / sv_fps` msec, not baseq3a's 200), which moved
  the first powerup spawns by one 100 msec settle frame.
- `ctf_match` (spidercrossings, not yet gated): flag pickup, capture,
  drop and return follow 0x10069800/0x10069e30 (yellow "got"/"returned"
  prints, the "CAPTURED the flag! (... captured in m:ss.sss)" print from a
  carry clock that ClientTimerActions runs, 0x100343f3; 100 for the
  capture, 25 for teammates, 10 for a return, a pickup from base and an
  assist; the capture sound reaches everyone but the capturer,
  `SVF_NOTSINGLECLIENT`; the taken call is throttled to two seconds and
  names the taker; nothing after the match ends), the flag spawn positions
  go out in `CS_RED_FLAG_ORIGIN`/`CS_BLUE_FLAG_ORIGIN` (0x100509b7), and
  the CTF scoreboard's unidentified team stat is the flags taken from base
  (0x10069fbf). Still diverging: one `scores_ctf` team-stat header after a
  drop (which team's block a viewer sees, or when the flag time counts)
  and the end-of-match awards; one flag CTF and `g_droppedFlagBonus`
  remain baseq3a.
- Exit rules follow 0x10058030: the timelimit with `g_overtime` (an
  `EV_OVERTIME` broadcast per extension while tied), the frag limit in FFA,
  duel and TDM, the capture limit in the flag modes, the score limit in
  domination and the mercy limit after `g_mercytime` in team modes; the
  round modes end by their rounds (M6). Checks run once a frame, as in
  retail; baseq3a's after-command `CheckExitRulesLater` is gone.
- Found by running into a new map: `target_location` names stop at 63
  (0x10067c03); FnQL-game had no limit and overwrote the match-state
  configstrings from 661 on maps with more.

Open in M5: the end-of-match awards (`CS_AWARD_*`, 0x100559e0, which read
match statistics FnQL-game does not keep yet), `g_autoAction`'s demo and
screenshot commands (0x100632d0), callvote's retail rules, timeouts,
`pstats`/`acc`/`ragequit`, the retail session string (twelve fields,
0x10065880), team-kill complaints, TDM/CTF/1FCTF match flow scenarios,
and G5 runs with retail clients.

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
