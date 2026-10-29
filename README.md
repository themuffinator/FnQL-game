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

Milestone 2 (the wire contract) is in progress. The item table, enums,
configstring layout and spawn table now match retail, checked against the
retail binaries. Every loadable stock map boots.

Gameplay is still Quake III, and the module does not yet claim retail wire
parity, so FnQL keeps loading the retail modules unless told otherwise. cgame
and ui arrive in later milestones.

## Using it

Build it (see [`BUILD.md`](BUILD.md)) and place `fnql_qagamex86.dll` in your
FnQL home `baseq3` directory. Then start an FnQL server with:

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
- [`docs/LICENSING.md`](docs/LICENSING.md): licensing status, including one
  open question.
- [`BUILD.md`](BUILD.md): building and running the gates.
- [`CREDITS.md`](CREDITS.md): lineage and acknowledgements.
