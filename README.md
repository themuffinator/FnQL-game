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

- `fnql_qagamex86.dll` builds for 32-bit x86;
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

The module does not yet claim retail wire parity, so FnQL keeps loading the
retail modules unless told otherwise. Native cgame and UI builds now have
local hosting gates. The [native UI](docs/UI.md) replaces the old Q3 UI
entirely; client provider selection and pure-server packaging remain M10.

## Using it

FnQL releases include the modules in `baseq3/` (`fnql_qagamex86.dll` on
Windows, `fnql_qagamei386.so` on Linux, plus the cgame and UI modules that
FnQL cannot select yet). To use your own build instead (see
[`BUILD.md`](BUILD.md)), place the module in your FnQL home `baseq3`
directory. Then start an FnQL server with:

```bash
fnql.ded.exe +set vm_gameProvider fnql +map campgrounds ffa
```

If the module is missing or invalid, FnQL falls back to the retail module and
says so. Run `vminfo` to see which module is active.

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
