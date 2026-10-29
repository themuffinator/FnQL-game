# AGENTS.md

## Mission

FnQL-game builds clean-room replacements for the retail Quake Live game
modules (qagame, cgame, ui) for the FnQL engine. The baseline is baseq3a. The
Quake Live behavior is rewritten from retail evidence. The modules must be
fully functional and invisible to retail peers in both directions:

- FnQL servers running FnQL-game accept retail clients;
- FnQL clients running FnQL-game join retail servers.

There must be no glitch either way. [`docs/PLAN.md`](docs/PLAN.md) holds the
milestones; [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) holds the
contract.

## Related repositories

- `E:\Repositories\FnQL`: the engine (`https://github.com/themuffinator/FnQL.git`).
  - It owns the wire, the module loader, providers, purity and packaging.
  - Its `docs/fnql/GAME_MODULES.md` is the engine half of the contract.
  - Its `AGENTS.md` rules apply to any engine change made for this project.
- `E:\Repositories\QuakeLive-SRP`: QLSRP, the Quake Live reconstruction.
  - A behavioral reference only.
  - The retail HLIL corpus under `references/hlil/quakelive/` is the evidence
    for compatibility constants.
- Baseline: `https://github.com/ec-/baseq3a` at
  `a3085f7aa6a9242383795b2806b0124401d8a409` (see `docs/BASELINE.md`).
- Retail runtime: the user's legitimate Steam install of Quake Live.

## Rules

1. **Clean room.** Never copy QLSRP implementation text or mirror its
   structure. Read it to learn what retail does, then write the behavior fresh
   for this codebase. The same applies to any other derived source.
2. **Retail is authoritative.** Every compatibility-sensitive constant (enum
   value, table order, configstring index or payload, server command grammar,
   pmove result) needs a retail citation: HLIL address or line, retail
   string, or a documented retail probe. When QLSRP and retail disagree,
   retail wins. `docs/PLAN.md` lists the known QLSRP divergences.
3. **Observed versus inferred.** Keep facts and inferences apart in comments,
   docs and commits.
4. **One ABI.** Modules use the retail structured native ABI and only retail
   import slots. FnQL-only services go through the `//trap_GetValue`
   handshake (`G_NativeHasEngineExtensions`) and must degrade cleanly on the
   retail engine. Never export `vmMain`.
5. **Shared layouts move together.** Structures shared with the engine by
   pointer change only together with FnQL. `scripts/check_engine_abi.py`
   (gate G0) must pass.
6. **Retail profile by default.** Nothing reaches a peer that has not
   negotiated it. Extensions are per client, only after both sides announce
   (`fnql_game` serverinfo, the cgame userinfo answer), and never through
   broadcast channels retail clients parse.
7. **Honest identity.** Claim `FNQL_MODULE_CAP_RETAIL_WIRE` only when the
   module's parity gates pass. Never synthesize authentication, purity or
   capability success.
8. **Gates are part of done.** A milestone slice is complete only when its
   gates pass and earlier gates still pass (`meson test`). Add a gate for any
   new compatibility surface.
9. **32-bit x86 for now.** Build, test and package x86 artifacts only, and
   never validate with other architectures, until a target is enabled through
   FnQL's Project Plan step 7 (FnQ3's full architecture and platform range).
   Then enable each target only behind its build, gate and FnQL hosting test.
   Retail-module interoperability stays x86 regardless.
10. **Cheap, windowed probes.** Use dedicated-server probes when the question
    is server-side. Client probes always run with `+set r_fullscreen 0`.
11. **Provenance.** Baseline files keep their original headers. New files use
    the FnQL-game GPL header. Record baseline drift in `docs/BASELINE.md`.
12. **No retail content.** Never commit Quake Live or Quake III assets,
    binaries, or extracted files and data dumps. The one exception is the
    interoperability facts in `docs/contract/`: the names, numbers and asset
    paths that the protocol and retail peers reference. They are regenerated
    and checked against the user's install by
    `scripts/extract_retail_contract.py`. Scratch output goes to `.tmp/`.

## Layout

- `code/game/`: qagame.
  - `g_native.[ch]` is the retail ABI boundary.
  - `fnql_module.h` is the identity contract shared with FnQL.
- `code/cgame/`, `code/ui/`, `code/q3_ui/`: baseline client modules, not yet
  ported (milestones 7 and 8).
- `version/fnql_game_version.h`: the canonical version.
- `scripts/`: gates and tooling.
- `docs/`: plan, contract, baseline, licensing.
