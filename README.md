# FnQL-game

Clean-room game modules for [FnQL](https://github.com/themuffinator/FnQL)
(Fappin' Quake Live). FnQL-game builds replacements for the retail Quake Live
`qagame`, `cgame` and `ui` modules. They interoperate with retail Quake Live
in both directions: FnQL servers running FnQL-game accept retail clients, and
FnQL clients running FnQL-game join retail servers.

- **Baseline:** [baseq3a](https://github.com/ec-/baseq3a), an improved
  Quake III 1.32 game code. See [`docs/BASELINE.md`](docs/BASELINE.md).
- **Quake Live behavior** is rewritten from retail evidence, with the QLSRP
  reconstruction as a behavioral reference. Nothing is ported line by line.
- **Retail assets** come from your legitimate Steam install of Quake Live.
  This repository ships none.

## Status

Milestone 1 of [`docs/PLAN.md`](docs/PLAN.md) is complete:

- `fnql_qagamex86.dll` builds for 32-bit x86, and since milestone 11 the
  modules also build for x86_64 on Windows and Linux (`fnql_qagamex86_64`)
  and for aarch64, armv7l, ppc64le and ppc64 on Linux, where they move
  players bit for bit as the retail `Pmove` does;
- it speaks the retail native module ABI and identifies itself to FnQL;
- it boots on retail maps in an FnQL dedicated server and hosts bots.

Milestone 2 (the wire contract) is complete. The item table, enums,
configstring layout, published cvars, spawn table and spawn filters match
retail, checked against the retail binaries. Every loadable stock map boots,
and the retail cgame runs against FnQL-game's qagame and reaches the game
with the retail HUD.

Milestone 3 (movement) is complete. Player movement and weapon timing match
the retail qagame bit for bit:

- the lockstep harness runs the retail `Pmove` next to FnQL-game's on 54
  scenarios, including fuzzed input, with no divergence;
- the retail cgame predicts FnQL-game's server, VQL and PQL, with no
  prediction error at all.

Combat and items (milestone 4) and the core match flow (milestone 5) are
rewritten from the retail qagame. The full-server lockstep differential runs
the retail qagame and FnQL-game side by side and finds no difference in FFA,
duel, TDM and CTF scenarios: kills and respawns, items and item timers, the
scoreboards, duel queueing and forfeits, ready-up, the end-of-match vote and
awards, CTF captures and defense bonuses, spectator following, timeouts, and
the stats and auto-action commands. Manual weapon, flag and timed-powerup
transfers also match retail, including powerup decay across timeouts.
Optional damaged-health drops, underwater lightning discharge and grappling
now have strict retail comparisons for their combat and item behavior.
Loadout choices and team-kill complaints also match the retail scenarios;
session persistence uses retail's thirteen-field format and reads older
FnQL-game records. One Flag CTF now has strict comparisons for captures,
carrier visibility and pings, drops, expiry, disconnect and capture limits.
Retail lag compensation now passes delayed moving-target comparisons; moving
brush grapple collision and live infinite-ammo changes have focused gates too.
Railgun headshot bonuses are exercised through FnQL's retail-shaped point
traces, with player movement collision protected by separate engine tests.
Thin-ceiling rocket splash and warmup-disabled team matches have retail
comparison gates as well.
Optional dropped-flag jump-pad and teleporter physics, capture zones,
portal camera roll and fallback intermission spawns have retail comparison
gates, including the default-off flag-physics path.
Scout, Guard, Damage and Armor Regen now have retail comparison gates for
spawning, effects, combat, transfers, timeouts and relocation, including
disconnects with powerup drops disabled. Race checkpoint timing, map-local
personal records and spectator progress now have strict retail comparisons,
as do Red Rover joins and shuffles during warmup, countdown and live play.
Clan Arena, Freeze Tag and Red Rover now implement their round lifecycles,
elimination/thawing/conversion rules and scoring, with strict server
comparisons covering their VCA, Instagib Freeze Tag and Infected variants.
Attack & Defend now runs alternating attack turns, elimination and flag
scoring. Domination has capture points, contested ownership, point-based
respawns and regeneration. Harvester skull collection/drops/captures and
Overload damage/destruction/regeneration/reset have strict retail server gates.
See [server command compatibility](docs/SERVER_COMMANDS.md) for coverage.
Broader bot behavior and special-mode parity remain in progress.

The [native UI](docs/UI.md) replaces the old Q3 UI entirely, and milestone 8
is complete: every menu, tab page and scripted interaction matches retail
captures, and its cvar and binding tables match retail's row for row.
Milestone 10 is complete: FnQL selects the cgame and UI too, a pure server
accepts them only from the FnQL-game client package, FnQL-game peers
negotiate extensions that retail peers never see, and the same binaries run
as drop-ins in the retail engine. The modules now claim retail wire parity
(`FNQL_MODULE_CAP_RETAIL_WIRE`), so FnQL's `auto` provider takes them.

Milestone 12 (release) is complete: FnQL releases build these modules in CI,
bundle them with the client package, and load them by default.

## Using it

### With an FnQL release

Every FnQL release carries the modules in `baseq3/`: `fnql_qagamex86.dll`,
`fnql_cgamex86.dll` and `fnql_uix86.dll` on Windows, the `i386.so` set on
Linux, and the `x86_64` sets in the 64-bit packages. It also carries
`fnql_game_client.pk3`, the client package (below). Extract the package into
your Quake Live folder as FnQL's own README describes, and FnQL uses
FnQL-game with no further setup:

```bash
fnql.ded.exe +map campgrounds ffa
```

hosts FnQL-game's qagame, and `fnql.exe` runs its cgame and UI. Run `vminfo`
in the console to see which modules are active.

FnQL picks each module with a provider cvar: `vm_gameProvider` for the
server, `vm_cgameProvider` and `vm_uiProvider` for a player. Each takes one of:

| Value | Module |
|---|---|
| `auto` (default) | FnQL-game's, if it declares retail wire parity, as released builds do; otherwise retail's |
| `fnql` | FnQL-game's, whatever it declares |
| `retail` | the retail module, exactly as retail Quake Live loads it |

If an FnQL-game module is missing or invalid, FnQL falls back to the retail
module and says so in the console. To go back to retail, set the provider:

```bash
fnql.ded.exe +set vm_gameProvider retail +map campgrounds ffa
fnql.exe +set vm_cgameProvider retail +set vm_uiProvider retail
```

A configuration written by an FnQL release from before milestone 12 may still
hold `vm_gameProvider "retail"`, because those releases saved the old default.
Run `reset vm_gameProvider` once to take the new default.

### Pure servers and the client package

Loose modules serve unpure servers, local play and demos. On a pure server
the client keeps FnQL-game's cgame and UI only if the server has loaded the
same client package, `fnql_game_client.pk3`, and lists it in
`sv_fnqlClientPaks`. A pure FnQL server from a release does that on its own.
On any other pure server, retail ones included, the client uses the retail
cgame and UI, and reports what it ran. A server operator can stop offering
the package with `sv_fnqlClientModules 0`.

### Playing with retail Quake Live

- Retail clients join FnQL servers running FnQL-game and need nothing extra:
  no package, no download, no difference on the wire.
- FnQL clients join retail servers. Retail servers require a Steam ticket,
  so the FnQL client must be able to reach Steam. A pure retail server always
  gets the retail cgame and UI.
- FnQL-game peers negotiate extensions only with each other, never with a
  retail peer. See [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md).

### With your own build

Build as [`BUILD.md`](BUILD.md) describes. Then put the modules, or the
`fnql_game_client.pk3` the build makes beside them, in your FnQL home
`baseq3` directory. FnQL-game modules carry the `fnql_` prefix, so they never
shadow a retail module; the providers above select them. The build also makes `fnql_game_server.pk3`,
the qagame in a package. Servers never reference it to clients, so it is
optional.

## Documents

- [`docs/PLAN.md`](docs/PLAN.md): milestones, evidence rules and validation
  gates.
- [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md): the interoperability
  contract with retail peers and the FnQL engine.
- [`docs/BASELINE.md`](docs/BASELINE.md): what was imported from baseq3a, and
  what was left out.
- [`docs/LICENSING.md`](docs/LICENSING.md): the two licenses the code and
  the released modules are under, and one open question.
- [`BUILD.md`](BUILD.md): building and running the gates.
- [`CREDITS.md`](CREDITS.md): lineage and acknowledgements.
