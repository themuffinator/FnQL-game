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
| G3 Lockstep differential | `scripts/server_lockstep.py` and `tests/lockstep/server/` (suite `differential`): FnQL's `sv_lockstep` dedicated server hosts the retail qagame and FnQL-game's qagame in turn with the same map, seed and scripted usercmd streams, and diffs every client-visible entity, scripted playerState, configstring, server command and engine query answer (FnQL `sv_lockstepQueries`) frame by frame; `--self-check` (retail twice) must show zero divergence | M4 |
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
- `pmove_t.grappleSlow` (retail `+0xe8`) is now identified and wired under
  M4: a hook latched to a client quarters the owner's pull (0x10035190);
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
frames of item pickups and respawns). `ffa_bot_restart` also passes its
400-frame retail self-check and differential with zero divergence. The
remaining 803 field-frames were caused by the inherited bot-input converter
normalizing projected movement by its largest component. Retail preserves
the projected speed instead (qagamex86 0x10021d0a-0x10021d76); matching that
conversion removes both the initial frame-32 velocity error and the later
position, pickup and weapon drift, including after `map_restart`.
The random streams match retail draw for draw (FnQL
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
for bots, minus `g_disableLoadout`; players select through the replicated
`weaponPrimary` byte, consumed before pmove at 0x10034e20), the spawn weapon (the loadout weapon, or
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
per-mode divergences moved to milestone 6: round kits and match state now
have the CA/FT/RR gates below. Harvester's skull toss is now covered by the
objective-mode gates; retail's repeated flag-return prints on a map without
flags remain open. Duel's queue and scoreboard
moved to M5 and pass (below).

The G4a PQL prediction probe missed twice in the crouch segment in 18 runs
on a busy machine (clean on reruns; no movement code changed); it needs a
reproduction under controlled frame timing.

Rename and bullet-impact metadata follow-up (2026-11-04):
`ClientUserinfoChanged` now sends the single broadcast observed at retail
0x1003a7e1. `Bullet_Fire` leaves `clientNum` zero, as 0x1006cecf-0x1006cefe
does; its victim stays in `eventParm` and shooter in `otherEntityNum`.
The `tdm_event_metadata0/1` G3 pairs exercise box/capsule traces, a shooter
in slot 2, active and spectator renames, repeated names and unrelated
userinfo updates. Inspection confirmed four broadcasts at frames 30, 40,
90 and 480, plus three MG, four HMG and three chaingun impacts on each of
flesh and walls, identically in retail and FnQL-game.

The colored-name gap from that probe is also resolved (2026-11-04).
Server-side `G_CleanPlayerName` normalizes into an info-sized buffer before
the final name copy, matching retail 0x1003a0b0 and 0x1003a771. Nonwhite
names get `^7` at the normalization stage (0x1003a1c6), explicit white and
its aliases get no extra reset, and ignored black codes preserve the
active color. The private stored-name capacity is corrected from 35 to
39 bytes plus NUL. Runtime probes confirm that this final byte copy may
clip the appended reset or a color token; preserving that order is needed
for retail-compatible configstrings and rename messages.

`game-names` covers fallback names, whitespace/filtering, color aliases,
small/zero/negative capacities, in-place cleanup and oversized input with
buffer guards. `tdm_colored_names` covers connection, active/spectator
renames, unchanged normalized names, 35-40 byte boundaries and restart;
the metadata scenarios now use implicit resets too. The independent
implementation reserves bounded storage and preserves control-character
filtering even for invalid inputs outside the retail probe. The inherited
shared `BG_CleanName` and its client/UI callers remain unchanged.

Separate userinfo-policy finding: the retail probe drops a client whose
name cleans to `UnnamedPlayer` when both color fields and Steam ID are
absent, under `g_kickBadUserinfo` (0x1003aa03-0x1003aa9d). FnQL-game now
applies that check after retail duplicate-name resolution. Uniqueness uses
the first 39 bytes, removes numeric color codes and compares ASCII case
insensitively (0x1003a350/0x10070b90); collisions receive prefixes `(2)`
through `(99)` before the final copy (0x1003a62a-0x1003a771).
The `ffa_bad_userinfo` pair covers disabled enforcement, each valid color,
duplicate fallback names, an actual drop, color/case collisions, long names,
prefix reuse and restart. Unit checks cover the comparison key's truncation
and extended-byte handling. The earlier name-normalization scenario retains
ordinary color fields so fallback names remain valid there.

Combat-rule follow-up (2026-11-05): damaged-health drops now convert actual
health lost (excluding armor and overkill) into rings of 25- and 5-health
pickups. The rule precaches both sizes and changes ordinary dropped-item
expiry to ten seconds; flags and runes retain their mode-specific lifetime.
Evidence: retail 0x10049a2b, 0x1004f8c0, 0x1004f6ec and 0x1004ffd0.
The strict `combat-damage-health0/1` self-check/differential pairs each cover
1,600 frames, including armor, death, self-damage, pickup physics, weapon
expiry and restart. Missile updates now retain entity order (0x100597d4),
so items created during an impact can run in that frame. Existing collision
rewind is preserved around each missile. Entity exhaustion safely stops
additional health drops instead of terminating the server.

Grapple and discharge follow-up (2026-11-05): hook impacts now place their
event on the hook, replicate its facing/normal, mark a hooked player for
reduced friction and slow the owner's pull. Latched hooks update every
frame, follow their target, damage it at `weapon_reload_gh`, release on
weapon changes/death and reset after three obstruction checks. The
`g_latchedHookOffset` setting adjusts the obstruction trace, not the
replicated pull point. Evidence: 0x1005bf3d-0x1005c323,
0x1006e330-0x1006e7b5 and 0x10035190. Missile traces now honor
`g_playerCylinders`, including start-solid retraces (0x1005c991).
The `ffa_grapple` scenario compares wall/player latches, release, weapon
changes, obstruction and restart with both capsule and box traces.

Underwater lightning discharge now consumes the available cells, publishes
`EV_LIGHTNING_DISCHARGE`, and applies distance-scaled damage to submerged
entities in sight, including the shooter (0x1006eb1d, 0x1004a7a0). With
infinite ammo, a subsequent LG pickup restores its ammo sentinel
(0x1004dac1). `ffa_discharge` compares enabled/disabled discharge and
infinite ammo on Corrosion, including a dry player and dry firing.
These comparisons live in the `combat-rules` suite; the moving-train
follow-up below adds brush collision and crush coverage.

Thin-ceiling splash follow-up (2026-11-05): eligible rockets probe back
through a downward-facing ceiling (0x1005c395), publish
`EV_MISSILE_MISS_DMGTHROUGH`, and use a reduced-radius visibility fallback
from the other side (0x1004a3e0). Ordinary splash keeps its original two
visibility tests. The penetrating path accepts either ordinary test, then
tries the far-side point; damage falloff still measures from the impact,
while knockback starts at the far-side point. Nonpositive or NaN fallback
radii are skipped safely. `ffa_surface_damage` exercises a 16-unit stock
Campgrounds bridge: retail event 86 and 68 health lost above the ceiling
were observed, while the player outside the reduced radius was unaffected.
Six strict pairs cover enabled/disabled forcing, insufficient probe depth,
angular rejection, quarter radius and zero radius, plus wall/floor impacts.
The authored `SURF_DMGTHROUGH` flag follows the observed bit 0x200000;
the fixture activates the same path through the force cvar.

Retail lag compensation now uses an independently written bounded sample
ring (`g_laghax.c`; 0x10051660..0x10051add). The default is four samples
and an 80 ms cap; hitscan rewinds around the whole shot using ps.commandTime,
interpolates origins and uses the newer sample's stance bounds. Projectiles
keep current positions. The inherited compensation remains explicitly
available as `g_unlagged 2` (full) or `0` (frame); `1` selects retail behavior.
History storage has a separate fixed budget and clamps excessive depth to
256 with a diagnostic. Nonpositive depth/window disables safely.

FnQL's dedicated lockstep harness now accepts per-client `delay` timestamps.
The moving/ducking/teleport rail fixture at 200 and 75 ms command age reports
1,744 field/frame differences with the previous DLL and zero with the rewrite.
Six strict pairs cover default, one-sample, extended-window, box traces and
disabled settings; `game-laghax` checks interpolation, stance, caps, reset,
restoration and safety bounds. Retail itself crashes in setviewpos with
history disabled (0x100448cd); disabled-mode runtime fixtures use ordinary
movement, and FnQL safely handles the empty history. A separate live-ammo
pair verifies that changing g_infiniteAmmo updates normal connected players
immediately, including unowned weapons (0x100520e0/0x10059cb0).

The moving-train grapple fixture now covers hook collision/crush, release,
teleport and restart with box and capsule traces. Retail includes hooks in
mover collision (0x1005e395), gives trains the shared crush handler
(0x10060a56), and does not repeatedly free a vacant hook slot (0x1006e330).

The headshot engine boundary is now verified against retail executable
0x4c6bd0/0x4c6d6b/0x4c6da0: a body cylinder plus upper sphere marks an earlier
head hit with contents 0x400. FnQL implements this for point traces while
preserving its existing moving-capsule collision guarantees. The two strict
`ffa_headshot` pairs cover standing/ducked head hits, torso damage and restart;
recording inspection confirms 210 damage for each capsule head hit and 10
for the torso. Engine collision tests also protect unchanged movement and
box-query behavior. Jump-pad touch now sets doubleJumped and event parm zero,
accepts frozen players and ignores hooked players (0x1002d540/0x1006b5d0);
the ordinary-movement `ffa_jumppad` pair reaches the stock campgrounds pad.

Still deferred to
M5/M6: retail's Steam
achievements and ZMQ stats reports (engine
imports 185/186 and 201-204, under the online-services rules).
Freeze Tag thawing, Red Rover infection scoring and CA/FT/RR respawn rules
are implemented and gated under M6 below.

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
  import 206. Implemented through `trap_FactoryExists`, including custom
  host definitions. The `factory-contract` suite requires zero divergence
  for factory-aware map votes and checks identity across transitions (see
  the progress entry below).
- Stats events (`MATCH_STARTED`, `PLAYER_*`, `ROUND_OVER`, `TEAM_WON`, …)
  through imports 185/186, under FnQL's online-services rules (explicit,
  default-off where services are involved).

Exit: G3 over full matches; first G5 runs with retail clients on an FnQL
server.

Progress (G3, both with no divergence):

- Factory integration (`factory-contract` suite): the native wrapper uses
  retail import 206 directly and fails closed if the host supplies no
  callback. The voting implementation validates optional factory IDs
  against that registry, after the map/gametype vote-disable flags and BSP
  existence check. Retail qagamex86 0x100428bf-0x10042a1b does not parse
  factory files or reject arena metadata in the module; the engine owns
  those decisions. `factory_votes` covers missing maps/factories, invalid
  token characters, and accepted stock IDs under vote flags 0, 1 and 8.
  `factory_custom` passes a vote for a hand-authored host definition,
  observes speed 333/gravity 777, retains it through map-only and restart
  commands, then restores stock duel. `factory_map_only` confirms flag 8
  still allows a map vote without a factory. Each has a retail self-check;
  explicit `g_factory` assertions guard against matching failed transitions.
  These gates establish the factory contract, not M6 special-mode gameplay.

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
  baseq3a's in-place reset; a duelist leaving mid-match forfeits — retail
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
- `ctf_match` (spidercrossings): flag pickup, capture, drop and return
  follow 0x10069800/0x10069e30 (yellow "got"/"returned" prints, the
  "CAPTURED the flag! (... captured in m:ss.sss)" print from a carry clock
  that ClientTimerActions runs, 0x100343f3; 100 for the capture, 25 for
  teammates, 10 for a return, a pickup from base and an assist; the
  capture sound reaches everyone but the capturer, `SVF_NOTSINGLECLIENT`;
  the taken call is throttled to two seconds and names the taker in
  `otherEntityNum`, 0x10069532; nothing after the match ends), the flag
  spawn positions go out in `CS_RED_FLAG_ORIGIN`/`CS_BLUE_FLAG_ORIGIN`
  (0x100509b7), and the CTF scoreboard's unidentified team stat is the
  flags taken from base (0x10069fbf). The last header difference was a
  retail quirk: the team scoreboards hide the other team's item control
  only partly, leaving invisibility (and in the flag modes medkits and
  flag time) visible (0x1003e029, 0x1003ea93, 0x1003f0bb).
- `ctf_defense` (spidercrossings, three players, `give` and `setviewpos`):
  retail `Team_FragBonuses` (0x10068970): 20 for a carrier frag announced
  with the carrier's powerup ("fragged BLUE's Haste flag carrier!"), 5
  for fragging whoever hurt the carrier within 8 s, 10 for a target within
  1000 units of the attacker's flag and in its PVS (the target's distance
  only), and 2 near the attacker's carrier (the attacker's distance; Quake
  III measured the wrong vector). A flag that is neither at its base,
  dropped nor carried goes home every CTF frame (0x100692b0, from
  G_RunFrame 0x10059a1e), which returns a carried flag at the
  intermission and, on a map without flags, prints retail's flag return
  every frame (the CTF factory test now matches retail).
- The end-of-match awards (0x100559e0, after every intermission scoreboard
  broadcast, 0x10055e50): accuracy (most shots break ties), damage (fewest
  shots), item control (100 a red armor or mega, 50 a yellow, 25 a green,
  480 a quad or battle suit; not in race, CA, domination, AD or RR) and,
  in the objective modes, offense (100 a capture, 10 an assist), defense
  (10 a defend or assist) and the most valuable player (25 a capture plus
  kills, the longest streak and defense, less deaths). The match stats
  gained captures, assists and defends (+0x7f8, +0x7fc, +0x800).
- Smaller retail details found by the CTF scenarios: the powerup pickup
  print puts the colour after the name (0x1004dd97); entities carry only
  the low six bits of `generic1` and a frozen player is never `EF_DEAD`
  (0x1002d5b0); trajectories round each component once, as retail's x87
  code does, and the sine phase has no modulo (0x1002d210); items think
  before they move and bounce with the velocity at `level.time` less the
  trace's fraction of a frame (0x10050de0, 0x10050c00); `give` sets every
  weapon bit but WP_NONE's, keeps infinite ammo and sets the entity's
  armor (0x1003fb50).
- `duel_commands`: `acc` (with the scoreboard) and `pstats` send every
  weapon slot's accuracy, the followed player's for a following
  spectator (0x1003f4d0, 0x1003f660); `ragequit` hands the client to
  engine import 192 (0x1004600e); `g_autoAction` and the `cg_autoAction`
  userinfo record demos from the countdown and stop them, with
  screenshots, four seconds into the intermission, under retail's file
  names (0x100632d0, 0x10062fd0). Following is retail's (0x10035470,
  0x10040f30, 0x10041130): a follower stays `PM_SPECTATOR` with its own
  ping, cannot follow a follower, follows only its own team in team modes,
  gets the scoreboard when it starts following, moves on or floats free
  when its player goes (0x1003c87b), and resumes after a respawn such as
  the restart into the match (0x1003c675, 0x10033e5a).
- `tdm_timeout`: retail looks every client command up in a command table
  first (0x10062e20, table 0x10080750: privilege, name, handler, help);
  `?` lists what the caller may use (0x10062d60), `players` prints slot,
  Steam id, privilege mark and name (0x10060ee0), and an unprivileged
  player is refused the moderator and administrator commands with
  retail's message (FnQL-game grants no privileges yet, so their handlers
  wait for the access lists). `timeout` (0x100621c0) is refused to
  spectators, off without `g_timeoutCount`, counted per team in team modes
  (the counts left go out in `CS_RED/BLUE_TIMEOUTS_LEFT`) or per player,
  and counted before the match state is checked (0x10061090 refuses it in
  the warmup and at the intermission); it stops the match for
  `g_timeoutLen` (0x10061db0). During it ClientThink only moves the command
  time on and sets `PMF_NO_MOVE` (0x10034d66), and G_RunFrame holds every
  entity's think, event and missile or mover flight and the match clock
  (0x100596a5, 0x100598f6). `timein` (0x10062330), from the caller or a
  moderator, ends it five seconds later; then respawn times, powerups,
  external events and item timers move on by the pause (0x10061ee0).
- Cvar changes are announced as retail does: only with cheats on, and
  only for the cvars whose retail change callback announces
  (0x10051ae0 and 48 specific callbacks; `retail_tables.json` now records
  `announce` per cvar and G2 holds the registrations to it).
- One flag CTF and `g_droppedFlagBonus` remain baseq3a.
- Exit rules follow 0x10058030: the timelimit with `g_overtime` (an
  `EV_OVERTIME` broadcast per extension while tied), the frag limit in FFA,
  duel and TDM, the capture limit in the flag modes, the score limit in
  domination and the mercy limit after `g_mercytime` in team modes; the
  round modes end by their rounds (M6). Checks run once a frame, as in
  retail; baseq3a's after-command `CheckExitRulesLater` is gone.
- Found by running into a new map: `target_location` names stop at 63
  (0x10067c03); FnQL-game had no limit and overwrote the match-state
  configstrings from 661 on maps with more.

- Manual drops (2026-11-03): `g_dropCmds` gates `dropflag` (bit 1),
  `droppowerup` (bit 2), and `dropweapon` (bit 4); `g_runes` gates
  `droprune`. Mode, spectator, timeout and instagib restrictions follow
  retail qagamex86 0x10044910, 0x10044ae0, 0x10044f10 and 0x10045250.
  Drops require a clear solid-world box trace, transfer ownership and
  expire after ten seconds. Weapon ammo is limited to the item's pickup
  quantity; the next lower owned weapon is selected with `EV_DROP_WEAPON`.
  Flag passes and `g_throwFlagVelocity`/`g_throwFlagForwardMult` use
  0x100513b0. `g_droppedPowerupsDecay` marks only manual timed-powerup
  drops (0x80000), caps their lifetime at the carried expiry, subtracts
  elapsed whole seconds on pickup (0x1004dfe0), and shifts their decay
  timestamp on timein (0x10061ee0). Death drops keep their existing rules.
  G3 scenarios `tdm_drops` (1,800 frames), `tdm_drop_timeout` (1,500) and
  `ctf_dropflag` (1,000) compare inventory, pickups, expiry, messages,
  spectator timers and flag throws with retail. All three pass with zero
  divergence, including their retail-twice determinism checks.
  `game-drops` independently
  gates policy, blocked drops, ammo bounds, and rune release.
  The tests also identified and fixed dropped-pickup announcements
  (0x1004dcbc) and dynamic item spectator notifications (0x10050ba1).

- Flag physics and capture zones (2026-11-04): `g_flagPhysics` defaults off
  and attaches `frameThink` only when a flag is manually dropped
  (0x100515f9). The callback queries a 30-unit cube, applies push velocity
  with a 1,500 ms sound debounce, teleports to the target with a snapped
  400-unit exit velocity plus 96 upward, and touches capture zones
  (0x10050f80). Missing destinations stop that frame without a crash.
  Capture zones use their `team` and `count`, credit the opposing player
  or dropped flag's owner, and share the existing capture rules
  (0x1006bda0 / 0x10069800). CTF still requires the player to carry the
  flag; the thrown neutral-flag branch awaits full 1FCTF validation.
  G3 `ctf_flagphysics0` and `ctf_flagphysics1` each pass 1,000 frames on
  Ironworks. `ctf_capturezone` passes 600 frames with a hand-authored
  entity fixture, including non-scoring empty/own-team touches and a
  three-point opposing-team capture. Retail determinism checks also pass.
  Unit coverage includes malformed area-query entries, missing targets,
  sound timing and a capture that frees the flag.
  These probes also exposed and fixed portal roll replication through
  `angles2` (0x1005a77f / 0x1005a919), the fallback intermission spawn's
  nine-unit offset (0x10054f4b), and early rounding of sine phases on
  bobbing movers (0x1002d297 / 0x1002d42b). Sine position and velocity
  are additionally compared directly with retail in the Pmove harness.

- Public voting (2026-11-04): `g_vote.c` owns the retail callvote grammar,
  vote flags, delay/limit policy, spectator eligibility, moderator pass/veto,
  and the frozen electorate (0x100423a0, 0x10043920, 0x10044270,
  0x100588f0). It executes approved votes after three seconds through either
  the engine command buffer or the module's toggle/random/shuffle handlers
  (0x10058ab0, 0x100591b0). Public voting leaves playerState flags unchanged;
  map-vote intermission behavior remains separate. The retail delay cvar is
  an absolute level-time threshold in milliseconds, not a between-vote
  cooldown. `g_allowSpecVote`, `g_allowVoteMidGame`, `g_voteDelay` and
  `g_voteLimit` have their observed default zero and flags zero.
  `game-voting` gates policy, malformed input, overflow, electorate changes,
  moderator overrides and execution once. Five `voting_*` G3 scenarios
  cover 5,250 frames: accepted/refused calls, majority/failure/expiry,
  ready countdown and midgame restrictions, spectators, team changes,
  disconnects, coin/random results, scalar/toggle actions, map restarts,
  shuffle and kicks. Their retail-twice self-checks and differentials pass
  with zero divergence. Factory voting has its separate `factory-contract`
  gates. The disconnect probe also corrected EV_PLAYER_TELEPORT_OUT's
  clientNum to retail's default zero (0x1003c9ef).
  Overlong tokens and overflowing integer arguments fail closed instead of
  truncating into a different vote target. The inherited multi-command Q3
  voting grammar and non-retail options (`rotate`, `g_gametype`,
  `g_unlagged`, `g_warmup`, `capturelimit`) are replaced by the observed QL
  allowlist; `map <name> <factory>` is the retail gametype-change path.
  These gates use the x86 retail 1069 module hosted by FnQL; live retail
  clients and other-platform runtime validation remain separate gates.

- Server commands (2026-11-04): the full shared administrative table and
  console route are implemented in `g_admin.c`/`g_access.c`, including
  privileges, persistent/temporary bans, locks, forced moves, pause/abort,
  ready and score/time controls. Chat/mute formatting, retail voice-command
  suppression, spectator selectors, bot chat, shuffle and diagnostic routing
  have corresponding fixes. Five strict G3 scenarios cover 2,590 frames;
  all self-checks and comparisons pass. `game-admin` covers malformed input,
  privilege boundaries, persistence failures and bounded editor operations.
  [Server command audit](SERVER_COMMANDS.md) records the full inventory,
  retail evidence, safety deviations and remaining special-mode dependencies.
  Race checkpoint scoring and map-local records are now implemented, with
  strict 40-frame editor and 280-frame records gates plus `game-race` bounds
  tests. Red Rover command shuffles respawn unchanged-team players and
  preserve match stats; `rr_shuffle` and `rr_shuffle_live` cover the mode.

Session/loadout/complaint follow-up (2026-11-05): the session writer and
reader now use retail's **thirteen** fields (0x10065880/0x10065930), including
the selected weapon before wins/losses and the reserved final word. The
earlier twelve-field description was incorrect. Old FnQL-game seven- to
twelve-field records remain readable; malformed records become safe
spectator sessions. `game-session` covers ordering, migration, bounds and
round trips. The otherwise-unused reserved word is preserved without
inventing behavior for it.

ClientThink now records the previous pmove's `weaponPrimary` as the next
spawn's loadout (0x10034e20). `ffa_loadout` compares enabled/disabled
loadouts over 1,100 frames each, including spectator/dead choices, invalid
bytes, disabled weapons and restart.

Team-kill complaints are implemented from 0x1004756e, 0x10043ab0 and
0x1003c943: 15.5-second prompts, `vote` responses, admin immunity, warnings,
temporary bans at the limit and disconnect cancellation. The retail
affirmative-byte quirk is preserved and gated. Nonfatal damage does **not**
prompt, even with `g_complaintDamageThreshold` enabled: disassembly at
0x10049363..0x1004936f proves its condition impossible (`sete` followed by
testing bit 8); the live fixture confirms it. This intentionally differs
from the reconstructed QLSRP behavior. `game-complaint` covers policy and
failure bounds; `tdm_complaints` and `tdm_complaint_kick` compare 6,200 frames.

The inherited forced/read-only `g_doWarmup` override is removed: retail
registers it as writable/archive (0x1008e0b8). TDM, CA and FT strict pairs
now cover warmup disabled. CA/FT players joining the match countdown
retain the firing lock (0x100381f1/0x1004bea9), just as at round countdown.

One Flag CTF now passes seven strict pairs (17,100 frames/provider): both
teams' captures, own-stand rejection, manual/death drops, pickup bonuses,
expiry, disconnect, pause, restart and capturelimit. Retail neutral statuses
are dropped=2, red carrier=3, blue carrier=4 (0x10069eaf/0x10071aa0).
The carrier snapshot predicate reads entity-state powerups, not server flags
(0x10052dc0). Pickup and periodic pings broadcast the carrier's team and
`g_neutralFlagPingRate`, sharing the Quad Hog residual (0x1004f293/0x10034777).
Zero ping rate emits every command, as retail does. `g_droppedFlagBonus`
enables a fixed ten points when a death drop's previous owner differs from
the picker (0x10069f51); manually thrown flags do not get that bonus.
The capturelimit fixture also proves that `map_restart` preserves
CS_INTERMISSION: the inherited initialization clear is removed.

The initial pure/unpure G5 runs with retail clients now pass. Spectators also honour a
0x800 button retail checks in SpectatorThink (0x10033f93: it frees a
follower); which client key sends it is not yet identified.

Qagame validation, 2026-11-05: the x86 build and 325 selected ABI, contract,
unit, runtime, differential, factory, round, objective, combat and stock-map
tests pass. FnQL's x86 client/dedicated builds, three collision gates and 620
engine source checks pass. G5a now passes on all thirteen base gametypes
with the current x86 client: the retail cgame reaches CS_ACTIVE, the requested
gametype is confirmed, a bot connects, and an engine screenshot is written.
The `interop-modes` suite adds twelve modes to the original FFA gate. Overload
uses an explicit `devmap` fixture because retail arena metadata excludes it;
the stock Dueling Keeps BSP nevertheless contains the required obelisks.
These are startup interoperability gates, not full-match client validation.
After Steam sign-in, G5b passes with the legitimate retail executable against
both pure and unpure FnQL servers. Each run confirms successful Steam session
validation, adds a bot, and verifies that the retail client remains in the
final server status after thirty seconds. The probe uses authenticated
loopback rcon because redirected stdin did not reach the Windows headless
console; it stops the server before cancelling the client's ticket and
restores the retail configuration and logs. Both settings are registered in
the opt-in `retail-client` suite. This closes the live-join validation blocker
for the qagame fixes; it does not claim M10's complete full-match matrix.

### M6 — Round-based and special modes

Clan Arena, Freeze Tag (with the thaw-event export), Attack & Defend, Red
Rover, Race (checkpoints, records), Domination, Harvester, Overload, plus the
instagib, quad hog and infected factory variants. This milestone also fills in
the QL engine queries: visibility, objectives, voice suppression, admin,
enemy relations and score.

Factory gameplay progress (2026-11-04):

- Instagib's map-item suppression preserves flag objectives; the starting
  weapon remains the kit's railgun when retail skips normal weapon selection
  (0x1005084d, 0x1003b4f9, 0x1003ba11). Strict G3 probes cover combat,
  infinite ammo, disabled suicide, forced item giving, restart, and a played
  Instagib CTF match with capture/drop/return.
- Quad Hog now registers and creates its world quad, announces pickup,
  selects respawns with retail's fixed sixteen-step random walk, replaces
  expired/dropped objectives, and sends periodic carrier locator events.
  Evidence: 0x100500ba, 0x1004d290-0x1004d500, 0x1004f889,
  0x10034725 and 0x1004f21a. `IsObjectiveEntity` recognizes its world quad
  (0x10052f40). Missing spawn points return safely instead of hanging.
  Unit checks cover ownership and bounded timers; strict G3 probes cover
  pickup, death drops, idle/powerup expiry, locators, combat restrictions,
  forced giving and restart.
- Factory starting health scales handicap rather than retaining a fixed
  100-point maximum (0x1003a878). A strict FFA probe isolates the shared VCA
  health and vampiric-healing rules, including invalid handicap and restart.
- HUD rule bits reflect factory settings and live changes, with serverinfo
  following the next frame (0x10053400-0x10054915 and the live-rules probe).
  Map-item rules retain the explicit Race inclusion and cheat force-spawn
  bypass (0x100507e0). All six scenarios have retail-twice self-checks and
  zero-divergence gates in the `factory-gameplay` suite.

Base-mode implementation (2026-11-04):

- **Clan Arena / VCA:** `g_rounds.c` now controls countdown, attack lockout,
  elimination, automatic teammate following, round results/reset, damage
  scoring, last-player warnings, accuracy/perfect awards, draws and match
  limits. Evidence: 0x10037fd0, 0x10038160, 0x10038230, 0x100389f0 and
  0x10035960. Warmup spawn selection, kit registration and waiting-player
  entity conversion follow 0x1003b4f9 and 0x1003c59f. Full VCA combat also
  verifies health/healing, turbo movement and dmflags footstep suppression
  (0x1003517e). The round-state queries now report actual state.
- **Freeze Tag / IFT:** frozen players retain their collision shell and
  movement state. Teammates thaw by proximity with optional line-of-sight;
  the original nearby helper receives the assist. Auto-thaw, warmup thaw,
  environmental removal/respawn, spawn protection and thaw tick visibility
  follow 0x1004bc80, 0x1004cd40, 0x10049659 and 0x10053020. Round transitions
  preserve living positions and honor health/armor/weapon reset controls,
  winning-team thaw policy, powerup removal and continuous rounds
  (0x1004c1b0). The thaw delay pauses during timeouts.
- **Red Rover / Infected:** deaths convert teams, retain per-round scores
  across respawns and update `scores_rr`. Match-score ties use round score;
  round winner lists use their own round-score order. Rounds shuffle/reset,
  award bonuses and end on round/time limits. Infected selects the initial
  infected player, remembers the last survivor, applies the infected
  gauntlet/health/speed/movement kit, converts killed/timed-out survivors,
  and awards timed or death-triggered survivor bonuses. Evidence:
  0x100643e0-0x10065700 and 0x10055f80. The existing survivor movement
  threshold configstring is retained; no server-side locator event is
  invented for the unreferenced `g_rrInfectedSurvivorPingRate` cvar.
- Related retail differences exposed by these modes are corrected: CA/RR
  ammo pickups use normal caps (0x1004dab0), and enemy powerup-carrier kills
  announce their powerup (0x10059d40). Damage without protection bypasses
  the round score/protection hook, as at 0x10049577.

The `round-modes` suite contains 21 strict G3 scenarios, each paired with a
retail-twice self-check. It covers CA combat/following/timeouts/draws/limits,
FT combat/assisted and automatic thaw/warmup/environmental deaths/reset
options/continuous rounds/timeouts, RR conversion/scoring/limits, and full
VCA, IFT and Infected factories. Hosting smoke tests remain separate and do
not establish gameplay parity. These server gates do not replace
retail-client interoperability gates; the engine queries are covered below.

Objective base modes (2026-11-04):

- **Attack & Defend:** the round controller alternates attack roles, freezes
  countdown movement, scores the first base touch once, scores captures and
  attacker eliminations, and ends matches only after the second attack turn.
  It publishes the retail turn/round state and rolling twenty-turn `scores_ad`
  history. Flags persist when dropped and reset between turns. Defensive
  bonuses use the current defending team; perfect-round awards include self
  and friendly damage. Deferred spawns retain the round-aware dispatch and
  attack lockout. Evidence: 0x10035780-0x100367bf, 0x10034dd0, 0x10034ea9,
  0x100398ee, 0x1004999f, 0x1004f6b7, 0x10069df2 and 0x1006a008-0x1006a276.
- **Domination:** `g_domination.c` owns bounded map-point registration,
  floor placement, signed capture progress, contention/neutralization,
  capture assists, defensive awards, batched point scoring and HUD ownership
  counts. Respawns use owned points' target associations, with retail's
  deathmatch fallback. Factory health/armor regeneration uses independent
  damage delays and residual clocks, including armor-after-health and armor
  tiers. Evidence: 0x1004aa80-0x1004bb10, 0x10038b60-0x1003994f,
  0x10059b08, 0x10034260-0x100343e0, 0x1003c319 and 0x10049789.
- **Harvester:** death creates a team skull at the neutral generator and
  optionally drops carried enemy skulls at the death position. Collection
  caps the six-bit carry count at 62, preserves upper state bits, removes
  friendly skulls and emits the retail pickup event. Captures use only those
  six bits. Toss speed/random order, expiry and capture announcements match
  retail. Evidence: 0x10046970-0x10046d65, 0x1006a0c6 and
  0x1006a800-0x1006a980. Map restart clears the cached neutral generator.
- **Overload:** the inherited Team Arena controller's damage, friendly-fire
  rejection, attack warnings, regeneration, destruction, scoring and respawn
  match retail in played probes (0x1006a570-0x1006b109). Percentage arithmetic
  now tolerates zero health settings and large values, and destruction does
  not dereference a missing player attacker.

The `objective-modes` suite adds 30 strict scenarios and 30 retail-twice
self-checks. It covers all four played modes, including score limits,
timeouts, restarts, long AD score history, Domination owned-point spawning,
Harvester's carry cap and Overload's complete damage/reset cycle. The
`game-domination` unit gate exercises registry/spawn bounds, invalid cap
times, extreme scaling and timer saturation. Malformed settings use safe
bounded fallbacks; online achievement/stat-service work remains separate.

Stock-map rotation follow-up (2026-11-04): both recorded discrepancies are
resolved. Brimstone Abbey's 360-degree doors now retain double intermediates
through the projected brush extent minus lip, then round to float as retail
does (0x1005fbe5-0x1005fbf5). This preserves the small transverse component
and yields the observed 23200.001953125 velocity instead of 23200. The change
is local to door travel-distance calculation; shared vector math is unchanged.
`target_push` now registers `jumppad.wav` only with its bounce-pad flag, with
no default wind sound (0x1006b900-0x1006b912). This preserves Devilish's
subsequent `sound/world/electro.wav` index.

All four objective limit scenarios now run for 3,600 frames, through automatic
rotation into the next stock map, and compare client states as well as entities
and configstrings. They reproduce the original failures before the fixes and
match retail afterward. The `stock-maps` suite additionally checks server
startup on Brimstone Abbey and Devilish, and a hand-authored fixture containing
both default and bounce-pad `target_push` entities. Each has a retail-twice
self-check and a zero-tolerance differential gate.

Devilish spectator-height follow-up (2026-11-04): resolved by removing the
inherited spawn-floor adjustment. Retail's spawn registration (0x10055000)
preserves map origins; its missing-intermission fallback (0x10054f4b) selects
an authored deathmatch spawn. Lowering that spawn had changed the initial
spectator Z from 1 to -5. The stock-map gates now include two spectators and
compare their complete player states, covering the fallback as well as maps
with explicit intermission points. Retail-client rendering and online services
remain outside these dedicated-server replays.

The rune gameplay slice is implemented and gated against the retail x86
qagame (2026-11-04). Static evidence and observed behavior are recorded
separately from the broader special-mode work:

- Initialization (0x1004d950, called at 0x100555c1) creates all four runes
  outside Race/intermission. Item registration (0x1005011f) handles
  `g_runes` and `com_build`. Spawn selection (0x1004d650) needs five
  eligible spots, considers at most sixteen available candidates and
  reserves a spot until pickup (0x1004e4e5). With five spawns on each
  team, initial runes alternate sides. The rewrite bounds malformed-map
  searches and logs missing spots/items rather than hanging or crashing.
- Manual/death drops relocate after ten/thirty seconds (0x1004f6ce;
  the manual command overrides the deadline). No-drop volumes relocate
  immediately (0x10050f27). Relocation chooses a fresh eligible side and
  frees the old entity before spawning its replacement (0x1004da10).
  The Space CTF recording confirms a throw at frame 23 and relocation
  at frame 57, in both modules, before its timer expires.
- Scout speed is 1.25, with Haste taking precedence (0x10034ee5); its
  separate 500 ms timer drains armor to 50 outside Clan Arena
  (0x100347d6). Guard does not regenerate health (0x100344d3), and a
  userinfo probe with handicap 100 preserves maximum health 100.
  Armor Regen adds ten armor each second, allows the last increment
  past the soft cap with tiered armor and does not regenerate ammo
  (0x10034601). Ordinary over-armor decay follows 0x100346b6.
- The combat probe exposed unrelated shared combat differences that
  affected rune gameplay: pain events need retail health bands and a
  damage sequence independent of sound debounce (0x100338dd-0x10033923);
  knockback needs the existing retail-precision vector normalization
  (0x10048d9d). Both are corrected and covered by the normal-mode gates.
- A disconnect probe with `g_dropPowerups 0` exposed Team Arena's obsolete
  pickup-relink path. Retail uses only the ordinary drop path at
  0x1003c9f7; removing the extra relink avoids resurrecting a freed item
  and preserves disabled-drop policy. Enabled/disabled death drops,
  team changes and disconnects have separate comparison gates.

`game-runes` checks eligibility, bounds, reservations, team distribution
and malformed maps. Eight full-server scenarios, each with a retail-twice
self-check, cover lifecycle, combat, tiered armor, timeouts, no-drop
relocation and disconnect policy. These establish the tested qagame
behavior; the separate round-mode gates above cover CA/FT/RR. Remaining
special modes, cgame presentation and G5 retail-client interoperability
remain their own milestones.

Engine queries (2026-11-04): the seven export-tail predicates the engine
asks (native slots 12-18) are rewritten from retail 0x10052dc0-0x100530f0
and its team helpers 0x10067ec0-0x10067f70, replacing the neutral stubs.

- Visibility (slot 12): teammates in the team modes, everyone for a
  spectator, neutral-flag carriers in One Flag CTF and everyone under Red Rover infection.
  The old stub showed every player to everyone regardless of PVS.
- Thaw tick (slot 13): only a linked viewer is filtered, so unlinked
  spectators keep seeing the tick; retail checks `r.linked`, not `inuse`.
- Objectives (slot 14): flags in CTF, One Flag CTF and Attack & Defend,
  Harvester's obelisks (case-insensitive classname), Quad Hog's quad, and
  with `g_itemTimers` every item whose retail row timer flag is set. Quad
  Hog skips the item-timer rule.
- Voice (slot 15): bots never receive and muted players never send;
  self-echo needs `g_allTalk 2`; `g_allTalk`, the intermission, waiting for
  players and spectators outside team modes are open; otherwise
  team-only. `g_allTalk` (default 0, retail flags 0) is now registered;
  retail reads it nowhere else.
- Admin (slot 16): privilege tier 2. The FnQL engine's in-band `rcon`
  client command requires it, as retail's does.
- Enemies (slot 17) and score (slot 18): retail's spectator and team rules,
  without a connection check; the score comes from `ent->client`.

Retail's own team predicate treats two spectators as teammates outside the
team modes; FnQL-game's shared `OnSameTeam` does not, so the exports use a
local retail-exact helper and the shared function is unchanged. Arguments
are bounded to the arrays they index, which retail does not do.

FnQL's opt-in `sv_lockstepQueries` record carries every answer, and
`server_lockstep.py` enables it unless given `--no-queries`, so every G3
scenario compares them. All 247 differential, self-check, round, objective,
factory, stock-map, unit, ABI and contract tests pass with zero divergence.
The previous stubs diverged 305 times on `ctf_match`. The FT thaw and
`tdm_admin` scenarios record non-zero thaw-suppression and admin answers.

Remaining retail cvars (2026-11-06): a table pass found 42 of retail's 393
qagame cvars unregistered. All are now registered with retail's defaults,
flags and change announcements, and G2 fails on any retail cvar missing
again (`gamedate` carries the module's own build date, as retail's does).
The rules behind them, rewritten from retail:

- Inactivity (0x10033fd0, called at 0x10034df4 for every human before the
  spectator step; bots skip it): idle command msec accumulate while
  `g_inactivity` is set, unless the player moves, attacks, spectates or
  holds admin privilege; a timeout or, in the round modes, a player out of
  normal play pauses the count. Speaking restarts the count but not the
  warning flag (0x100417a5). The warning reads "%i second%s until dropped
  for inactivity!" `g_inactivityWarning` seconds before; `g_dropInactive
  0` sends the player to the spectators to follow someone instead of
  dropping it. `g_debugInactivity` prints the count.
- Flood protection (0x100341e0): observed in the retail DLL, nothing ever
  increments the counter at gclient +0x338 (a full disassembly scan finds
  only the decay's writes), so its drop cannot happen. FnQL-game keeps the
  check and its decay and invents no counting rule.
- Chat (0x10041760, 0x10041330): team chat uses retail's team test
  (0x10067f30), so outside the team modes a player's team chat becomes
  public and a spectator's stays among the spectators. Duel spectators may
  chat to the duelists during the warmup and the intermission.
  `g_teamSpecSayEnable 0` keeps team-mode spectators' team chat and tells
  among spectators.
- Team joins (0x10040921-0x10040b65, 0x10068100): a colour can be chosen
  only while the match waits for players; during the countdown and the
  match the joiner goes to the smaller team. `teamsize` refuses a full
  team. With `g_teamForceBalance`, teams two apart refuse the larger side,
  and even teams refuse a switch between them; the locks are tested last.
  The `tdm_autoshuffle` probe exposed this: the inherited Q3 rule let a
  2-2 switch through. Forced moves (shuffle, admin `put`) are unchanged.
- Sudden-death respawn (0x1005952f, 0x10048289): with
  `g_suddenDeathRespawn`, overtime respawn delays start at
  `g_suddenDeathRespawnStart` seconds and grow by `...Increment` every
  `...Tick` seconds up to `...Max`, each change centerprinted.
  `g_suddenDeathRespawnPrint` is registered but never read by retail.
- Automatic shuffle (0x1005992b, 0x1004fd30, 0x1006a2d0): team modes only;
  armed at map start and below `g_shuffle_automatic_minplayers`, it counts
  `g_shuffle_timedelay` down while the match waits and then shuffles; any
  shuffle disarms it. The shuffle's own minimum now counts connected
  players, as retail's does.
- Duel ready delay (0x10058580, counts from 0x10057890): once one of two
  duelists has readied, `CS_READY_UP` carries a `g_warmupReadyDelay`
  deadline; then action 1 unreadies the ready player and benches the other
  as spectate-only through the team-change core (0x10040440, now
  `G_ExecuteTeamChange`), and action 2 readies both.
- `g_teamSpawnAsSpec` (0x10065a03): a team-mode map starting in its warmup
  restores everyone as a spectator.
- `g_tackleFlag` (0x1004983a, through the flag-drop helper 0x100513b0): a
  damaging gauntlet hit knocks the victim's flag out 64 units above it,
  tossed straight up at `g_tackleFlag`.
- Kamikaze (0x1006fc9f): the earthquake scales per player by
  `1 - distance / g_kamiAttenuate`, floored at `g_kamiMinRatio`, skips a
  player at exactly zero and draws each player's jolt separately.
- Debug output: `g_debugThawTime` (0x1004d1d9) tells the thawing helper
  the time left, `g_debugVampiricDamage` (0x10049b80) logs each leech, and
  `g_enableDebugTrace` (0x10049d20) announces a blocked splash probe that
  stopped on a spectating player. `g_botSpawnList` and
  `g_forceAtmosphericEffects` are registered only; retail reads them
  nowhere else.

The 15 missing `bot_*` cvars are registered for retail parity; the retail
bot behaviour behind them remains milestone M9.

The new `server-rules` suite has 18 strict pairs (retail-twice self-check
and zero-divergence differential): both inactivity outcomes, FFA, TDM
(with and without `g_teamSpecSayEnable`) and duel chat, team joins, sudden
death, the shuffle, both ready-delay actions, spawn as spectator and its
off control, the tackle and its off control, two kamikaze attenuations and
the thaw print. Built from the previous commit, the module diverges on
every one of them except the controls (spawn and tackle off, and TDM chat
with the default `g_teamSpecSayEnable`, which already matched). Two scenarios deliberately keep
a single fresh spectator or queued player: retail stamps spectator order
with the CRT wall clock (`_time64`, 0x100404ed), which lockstep cannot pin,
so two such players would order by real time. Unit gates cover the
tackle (`game-drops`) and `g_teamSpawnAsSpec` (`game-session`).

Exit: G3 per mode; G5 per mode.

### M7 — cgame

Native hosting bring-up is implemented; M7 remains open. The x86 DLL builds,
passes its adapter gate, and reaches active Campgrounds/FFA play with both
qagame providers. See [CGAME.md](CGAME.md) for evidence and outstanding work.
Capability bits remain zero; hosting does not establish G4 or visual parity.

The retail score command family now has bounded parsers and main per-mode
player rows. FFA/TDM scoreboards pass with both qagame providers and repeated
HUD reloads; retail Duel, Race, CA and CTF also pass command/menu bring-up.
End tables and remaining ownerdraws still prevent scoreboard visual parity.
Retail team-index lists and accuracy replies now parse, snapshot vitals feed
the HUD, and `+acc` displays and refreshes weapon/overall accuracy. The four
`cgame-stats` probes pass with both providers; `+pstats` presentation remains.
Overtime, match results and red/blue round-win announcements now have
retail-backed handlers and unit coverage. Full FFA match-end probes pass
with both providers; the optional `cgame-match` suite also gates overtime.
End-screen ownerdraws, remaining event variants and audio/visual parity
remain open; see the evidence and probe limitations in [CGAME.md](CGAME.md).
Thaw fragments/trails/marks and damage-through impact debris now use retail
event evidence and have focused unit coverage plus passing runtime event
gates with both providers. Frozen-player body materials and animation speed
now follow retail, with regression tests for existing animation/powerup
paths. Round draws, last-standing voice and Red Rover winner/survivor
messages now have retail-backed handlers and unit coverage; paused-bot round
probes exercise CA draws and infected RR survivors. Teammate markers,
POI and remaining objective/team-sound variants still need work. Race start,
checkpoint and finish events, reliable race state, timeout compensation and
elapsed/last-time HUD are implemented. Gravity-only race probes exercise
both server providers. Checkpoint model progression, floor projectors,
navigation icons and the bound/unbound respawn hint also have production
tests and a gravity-only presentation fixture. The shared native font bridge
now uses retail pixel sizing and visual bounds/ascent, with production
coverage across HUD transforms. Full HUD visual parity remains open. No-player-clipping body/weapon materials, ghost view weapons
and selected-weapon ammo warnings now have production regression coverage.
Status-value anchors, unlimited ammo, health/armor bar cropping and
team-tinted HUD images now have production tests and matched retail-module
captures. Weapon lists now have retail persistent layouts, timed strip,
ammo/full-list/follow policies, production tests and matched retail-module
captures. Snapshot-based cycle/direct/toggle selection, empty switching,
frame history, spawn preferences, queued primary loadouts and weapon config
dispatch now have production tests. Input-free hosting gates and retail
comparison captures exercise spawn/loadout/config behavior with both qagame
providers. Match/round clocks, round labels, overtime and local time now
have production tests; matched retail captures and hosting gates cover
active clocks and indefinite pauses. Pause/time-in banners, server `cp`/`pcp`
notices and timeout countdown/resume cues now follow retail evidence, with
production tests and matched time-in captures. Top-left player/team standings,
mode icons, win limits and remaining-player/owned-point counters now have
production tests and input-free hosting gates. Matched retail-module captures
cover FFA long-name clipping and CA default/custom team labels. Normal-match
warmup waiting/ready-up/countdown presentation and its announcer transitions
now have production tests and FFA/CA hosting gates with both providers.
Broader mixed visibility flags, training/tutorial presentation and remaining
menu-capture behavior remain open.
Retail FOV presets, command FOV,
spectator/demo overrides, zoom scaling/toggle/sensitivity and death resets
now have production regression coverage. Broader camera parity and live
zoom/follow/demo captures remain open.
An offline production-predictor gate now covers delayed acknowledgements,
cache reuse, QL server-state corrections, missing history and interpolation.
The cache no longer discards corrected jump/crouch timing or QL sidecars.
The live prediction probe accepts FnQL cgame with both qagame providers;
the full G4 live/demo corpus is still required.

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

Implemented in the sole `code/ui/` native module; `code/q3_ui/` is removed.
The installed legacy menu definitions supply all screens and actions. The
runtime gate includes normal menu activation and retail screenshot comparisons
at 640x480 and 960x600; no custom navigation is injected. [UI.md](UI.md)
records the recovered behavior, validation evidence and integration boundaries.
A table-by-table gap pass against the retail DLL also brought in retail's cvar
table and change callbacks, its binding table, the slot 3/4 item gates,
`toggle`, the display and model scripts, retail menu activation, absolute
mouse input and workshop download progress. Pixel gates now compare every
in-game menu, tab page and out-of-game screen with retail at 960x600 and
1024x768, and an input-proxy gate compares hover, slider, multi, yes/no,
preset, list, tab, keyboard, edit-field, key-binding and main-menu
interaction at both sizes, including the cvar values those writes store.
That gate needs FnQL's `Cvar_SetFromUI`: earlier engines refused the UI's
writes to retail-protected settings cvars, leaving the options menus inert.

A retail-ABI `fnql_uix86.dll`:

- 97 import slots; 14 exports.
- The Team Arena menu system: in-game, callvote, admin, join and options
  menus; the existing connect/download and advert screens.
- Coexists with the engine-hosted WebUI.

### M9 — Bots

The FFA movement/restart slice passes G3: `BotInputToUserCommand` now
preserves retail's projected speed, wide arithmetic before command-byte
truncation, and integer-absolute vertical projection. This focused gate
does not establish complete bot AI parity across modes or maps.

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

- CI builds for Win32 and Linux i386. Done: FnQL's release workflow builds a
  pinned commit of this repository (FnQL `version/fnql_game.json`) with MSVC
  Win32 and in an Ubuntu 20.04 i686 container, and runs `meson test` on both.
- Packaging inside FnQL releases. Done: the modules ship in `baseq3/` of
  every FnQL package, with this repository's licensing documents.
- Defaults flipped to `auto` once G2–G5 pass.
- User docs.

The baseline licensing question in [`LICENSING.md`](LICENSING.md) is still
open. Until it is settled, releases distribute the modules under both
licenses as that document describes.

## Working conventions

- Small compatibility slices, each with its gate.
- Baseline files keep their headers. New files carry the FnQL-game GPL header.
- 32-bit x86 only. Never validate with 64-bit artifacts.
- Probes run windowed (`+set r_fullscreen 0`) and as cheaply as the question
  allows. Dedicated server first.
