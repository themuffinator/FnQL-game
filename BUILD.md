# Building FnQL-game

FnQL-game builds 32-bit x86 modules only. Retail Quake Live and FnQL are
32-bit, so a 64-bit module could never load, and the build refuses to
configure for one.

## Windows (MSVC, Win32)

Open an x86 developer environment, for example by running `vcvars32.bat` from
Visual Studio 2022. Then:

```bash
meson setup build/win32-x86
```

```bash
ninja -C build/win32-x86
```

Outputs: `build/win32-x86/fnql_qagamex86.dll`, `fnql_cgamex86.dll` and
`fnql_uix86.dll`. Each links the CRT statically and exports `dllEntry` and
`FnQL_GetModuleInfo`. See [native UI validation](docs/UI.md) for the UI
behavioral and windowed hosting gates.

## Linux (i386)

Use the committed i686 cross file on an x86_64 host with `gcc-multilib`:

```bash
meson setup build/linux-i386 --cross-file meson/linux-i686.ini
```

Outputs: `fnql_qagamei386.so`, `fnql_cgamei386.so` and `fnql_uii386.so`.
The cross file puts `-m32` in the compiler entry so the ABI and wire-contract
probes compile 32-bit too. FnQL's release workflow builds and gates this
target in an Ubuntu 20.04 container (glibc 2.31 baseline). Runtime gates on
Linux are not yet part of the release.

## Gates

```bash
meson test -C build/win32-x86
```

- `engine-abi-parity` (G0) compiles a layout probe against these headers and
  against `../FnQL` (override with `-Dfnql-root=<path>`). It fails on any
  shared-structure difference and skips when no FnQL checkout exists. Run it
  from the same x86 developer environment.
- `game-voting` (suite `unit`) exercises the production public-vote policy,
  ballot lifecycle, moderator pass/veto, input bounds and execution without
  retail assets. The five `server-lockstep-*-voting_*` tests (suite
  `differential`, each with a retail-twice self-check) compare full state and
  messages for policy, actions, midgame/countdown transitions, team shuffles,
  kicks and disconnects. Run the focused gates with:
  `meson test -C build/win32-x86 game-voting 'server-lockstep-*voting*'`.
- `game-drops` (suite `unit`) checks the production drop commands with a
  fake world: policy refusals, blocked drops, weapon ammo transfer and
  selection, short powerup lifetimes, and persistent rune ownership. It
  needs no retail assets. The `tdm_drops`, `tdm_drop_timeout` and
  `ctf_dropflag` G3 scenarios exercise real pickups, expiry, spectators,
  timeout decay, flag passing and throwing against the retail qagame.
- `combat-rules` adds twenty-nine strict retail self-check/differential pairs:
  damaged-health drops enabled/disabled (armor, overkill, self-damage,
  pickup physics and expiry), submerged lightning enabled/disabled and
  infinite-ammo replenishment, and grappling with capsule/box traces
  (world/player latches, ticks, obstruction, weapon changes and restart),
  loadout selection enabled/disabled, and team-kill complaints through
  forgiveness, expiry, admin protection, disconnect and a temporary-ban kick,
  plus thin-ceiling rocket splash with force, probe distance, angle and radius
  dampening controls. Delayed hitscan adds window/depth/trace variants;
  moving trains, live ammo toggles, jump-pad contact and rail headshot/body
  damage have focused pairs. Together they cover 24,600 scenario frames per provider.
  Run `meson test -C build/win32-x86 --suite combat-rules`.
- `game-session` and `game-complaint` (suite `unit`) cover retail session
  serialization, old FnQL-game session migration, malformed state, complaint
  policy boundaries, affirmative response bytes and temporary-ban failures.
- `game-laghax` checks interpolation, stance bounds, rewind limits, reset,
  restoration and invalid settings. FnQL's lockstep engine must support the
  `delay` input field for the delayed-command runtime gates.
- `server-lockstep-nowarmup-*` (suite `differential`) checks `g_doWarmup 0`
  in TDM, Clan Arena and Freeze Tag, including countdown join firing locks.
- `game-flagphysics` (suite `unit`) covers push/teleport trajectories,
  sound throttling, missing destinations and capture callbacks that remove
  a flag. `ctf_flagphysics0` and `ctf_flagphysics1` compare both settings
  on Ironworks, including portal roll and fallback intermission spawns.
  `ctf_capturezone` checks team restrictions and a three-point capture.
  It uses the hand-authored `tests/lockstep/entities/capturezone.ent`
  fixture with installed Campgrounds geometry. The runner's `--entities`
  option mounts it through `sv_altEntDir` and fails if the engine does not
  confirm loading it; no retail map data is copied into the repository.
- `game-runes` (suite `unit`) checks spawn eligibility, reserved spawn
  points, team distribution, bounded selection, relocation and malformed
  maps without retail assets. The `tdm_runes`, `tdm_runes_tiered`,
  `tdm_rune_combat`, `tdm_rune_timeout`, `ctf_runes`, `ctf_rune_nodrop`
  and `tdm_rune_disconnect0/1` G3 pairs cover all four rune effects,
  Haste/Quad precedence, tiered armor, pickup ownership, manual/death drops,
  expiry, no-drop volumes, timeouts, team changes, disconnect policy and
  map restarts. Each pair includes a retail-twice determinism check.
- `game-names` checks color resets, black/white aliases, fallback names,
  capacity boundaries, overlong input and in-place cleanup without retail
  assets. `tdm_colored_names` compares connection/rename updates, spectator
  names, repeated updates, the 39-byte stored-name limit and map restarts
  against retail, with a separate retail-twice determinism check.
- `tdm_event_metadata0/1` G3 pairs check rename broadcasts and MG/HMG/CG
  flesh/wall impact metadata with box and capsule traces. A nonzero-slot
  shooter exposes incorrect `clientNum` values; active and spectator name
  changes, unchanged names and unrelated userinfo updates cover broadcast
  routing. Each pair includes a retail-twice determinism check.
- `dedicated-smoke` (G1) runs when `-Dfnql-dedicated=<path to x86
  fnql.ded.exe>` is set. It boots the module on a retail map with a bot. Point
  it at your Quake Live install with `-Dretail-ql=<path>` or
  `FNQL_RETAIL_QL_PATH`. Scratch files go under `.tmp/` and are removed
  afterwards.

- `wire-contract` (G2) compares the shared tables, published cvars, spawn
  table and payload grammars with `docs/contract/`. `retail-tables-current`
  checks that the committed retail tables still match your install, and
  skips when none is found.
- `factory-contract` (suite, with `-Dfnql-dedicated`) runs strict G3
  differentials and retail determinism checks for factory-aware map votes:
  missing/invalid names, vote-disable flags, stock and custom factories,
  map-only retention, restart retention, and restoration of stock settings.
  Run `meson test -C build/win32-x86 --suite factory-contract`.
  `server_lockstep.py --factories <file>` mounts hand-authored definitions
  in each disposable home and requires the engine to confirm loading them.
  `--expect-factory <frame> <id>` checks the published identity in both runs,
  so two failed transitions cannot produce a false pass. The older
  `factories` suite checks only for fatal errors across special modes; it
  does not establish gameplay parity for those modes.
- `factory-gameplay` adds strict retail self-checks and differentials for
  Instagib combat and CTF objectives, Quad Hog pickup/drop/expiry/locators,
  live rule-mask updates, and factory health/handicap/vampiric healing.
  Run `meson test -C build/win32-x86 --suite factory-gameplay` from the
  x86 compiler environment. `game-quadhog` in the `unit` suite also checks
  missing spawn points, objective ownership and timer overflow bounds.
  The health probe isolates VCA's shared rules in FFA; the separate
  `round-modes` suite tests the complete VCA factory.
- `round-modes` contains 21 strict server scenarios, each run as a
  retail-twice self-check and a retail-versus-FnQL comparison. It covers
  Clan Arena elimination/following/countdowns/timeouts/results/limits,
  Freeze Tag thawing/environmental respawns/reset controls/continuous play,
  Red Rover conversion/scoring/limits, and the VCA, IFT and Infected
  factories. Run `meson test -C build/win32-x86 --suite round-modes` from
  the x86 compiler environment. These are zero-divergence gates; the
  separate `factories` suite remains only a hosting smoke check.
- `objective-modes` contains 37 strict server scenarios, each paired with a
  retail-twice self-check. Attack & Defend covers alternating turns, first
  touch/capture/elimination scoring, dropped flags, deferred spawns, timeouts,
  limits and rolling score history. Domination covers capture contention,
  assists/defense, regeneration, owned-point spawns and limits. Harvester
  covers skull caps, collection/drops/expiry, captures and restart. Overload
  covers friendly/enemy damage, destruction, respawn, regeneration, limits
  and timeout/restart. One Flag CTF adds carrier visibility/pings, captures,
  death/manual drops and bonuses, expiry, disconnect, timeout and capturelimit.
  Run `meson test -C build/win32-x86 --suite objective-modes`.
  Fixtures are hand-authored entities on the installed campgrounds BSP;
  Overload and One Flag use small custom factories. No retail map data is distributed.
  `game-domination` separately tests malformed capture settings, timer overflow,
  the five-point registry and bounded owned-point spawn selection.
  The four score-limit scenarios continue through automatic stock-map rotation.
- `stock-maps` checks spectator startup on Brimstone Abbey and Devilish,
  including door trajectory precision and sound registration. A hand-authored
  fixture checks both default and bounce-pad push sounds. All three scenarios
  have retail-twice self-checks and strict differential gates. Run
  `meson test -C build/win32-x86 --suite stock-maps`.
- `map-sweep` (suite `sweep`, slow) loads every stock map. It runs with
  `-Dfnql-dedicated`.
- `retail-cgame-probe` (suite `interop`) runs when `-Dfnql-client=<path to
  x86 fnql.exe>` is set. A windowed FnQL client hosts FnQL-game's qagame and
  runs the retail cgame against it.
- `interop-modes` extends that startup/render gate to the other twelve base
  gametypes, using stock maps with the required objectives. One Flag CTF and
  Overload use hand-authored factory fixtures; Overload explicitly uses
  `devmap` because retail arena metadata excludes that mode. Each probe checks
  the selected gametype, module identity, CS_ACTIVE, a connected bot, no engine/module errors, and an engine
  screenshot. These are startup interoperability checks, not full-match or
  authenticated retail-client tests. They run windowed without input grabs.
- `pmove-lockstep` (G3a, suite `parity`) builds `pmove_lockstep.exe` (Windows
  x86 MSVC only) and runs it against the retail `qagamex86.dll` from your
  install's `bin.pk3`. It skips when no install is found or when the module is
  not the 1069 build the harness maps. It also checks sine trajectory
  position and velocity against retail over 3,408 evaluations. Run one scenario with
  `python scripts/pmove_lockstep.py --harness build/win32-x86/pmove_lockstep.exe
  --scenario <name> --verbose`.
- `retail-client-join` and `retail-client-join-unpure` (G5b, suite `retail-client`, with
  `-Dfnql-dedicated`) launches your retail Quake Live through Steam, windowed,
  and connect it to pure and unpure FnQL dedicated servers running FnQL-game.
  They require successful Steam session validation, a bot and a final status
  query showing the retail client still connected. Steam must be signed in.
  The probe skips while a retail client is already open, and it restores
  any retail config or log the client rewrote.
- `retail-cgame-prediction` and `retail-cgame-prediction-pql` (G4a, suite
  `interop`, with `-Dfnql-client`) drive a scripted movement course. They fail
  on any retail-cgame prediction error, with VQL and with PQL movement.

You can also run the main gates directly:

```bash
python scripts/check_engine_abi.py --fnql ../FnQL
```

```bash
python scripts/smoke_dedicated.py --engine <fnql.ded.exe> --retail "<Quake Live>"
```

### Tracing the random stream

`meson configure build/win32-x86 -Dtrace-rand=true` makes the qagame log
every `rand()` with its source line. Together with FnQL's
`+set sv_lockstepTraceRand 1` (which traces the retail module) it shows
where a G3 differential's random streams part. Switch it back off for normal
builds and gates.

### Server command gates

`meson test -C build/win32-x86 --suite unit` includes `game-admin`, which
exercises the production admin/access, spectator, bot-chat and Race-editor
handlers without retail assets. The `differential` suite includes both
retail-twice and retail-versus-FnQL runs for `tdm_admin`, `tdm_access`,
`tdm_commands`, `tdm_shuffle` and `tdm_shuffle_live` (2,590 frames total, strict zero divergence).
`game-race` covers production checkpoint order, records and bounds.
Additional strict G3 pairs are `race_editor` (40 frames), `race_records`
(280 frames, with a hand-authored course), `rr_shuffle` and `rr_shuffle_odd`
(100 frames each), `rr_join` (40 frames, two players), and `rr_shuffle_live`
(1,600 frames, five players across countdown and live play).
See [the command audit](docs/SERVER_COMMANDS.md) for evidence and coverage.
