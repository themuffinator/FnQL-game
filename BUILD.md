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

Output: `build/win32-x86/fnql_qagamex86.dll`. It links the CRT statically and
exports exactly `dllEntry` and `FnQL_GetModuleInfo`.

## Linux (i386)

Use an i686 cross file. FnQL's `misc/meson/linux-x86.ini` works:

```bash
meson setup build/linux-i386 --cross-file ../FnQL/misc/meson/linux-x86.ini
```

Output: `fnql_qagamei386.so`. The Linux build is not yet validated.

## Gates

```bash
meson test -C build/win32-x86
```

- `engine-abi-parity` (G0) compiles a layout probe against these headers and
  against `../FnQL` (override with `-Dfnql-root=<path>`). It fails on any
  shared-structure difference and skips when no FnQL checkout exists. Run it
  from the same x86 developer environment.
- `dedicated-smoke` (G1) runs when `-Dfnql-dedicated=<path to x86
  fnql.ded.exe>` is set. It boots the module on a retail map with a bot. Point
  it at your Quake Live install with `-Dretail-ql=<path>` or
  `FNQL_RETAIL_QL_PATH`. Scratch files go under `.tmp/` and are removed
  afterwards.

- `wire-contract` (G2) compares the shared tables, published cvars, spawn
  table and payload grammars with `docs/contract/`. `retail-tables-current`
  checks that the committed retail tables still match your install, and
  skips when none is found.
- `map-sweep` (suite `sweep`, slow) loads every stock map. It runs with
  `-Dfnql-dedicated`.
- `retail-cgame-probe` (suite `interop`) runs when `-Dfnql-client=<path to
  x86 fnql.exe>` is set. A windowed FnQL client hosts FnQL-game's qagame and
  runs the retail cgame against it.
- `pmove-lockstep` (G3a, suite `parity`) builds `pmove_lockstep.exe` (Windows
  x86 MSVC only) and runs it against the retail `qagamex86.dll` from your
  install's `bin.pk3`. It skips when no install is found or when the module is
  not the 1069 build the harness maps. Run one scenario with
  `python scripts/pmove_lockstep.py --harness build/win32-x86/pmove_lockstep.exe
  --scenario <name> --verbose`.
- `retail-client-join` (G5b, its own suite `retail-client`, with
  `-Dfnql-dedicated`) launches your retail Quake Live through Steam, windowed,
  and connects it to an FnQL dedicated server running FnQL-game. Steam must be
  running. It skips while a retail client is already open, and it restores
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
