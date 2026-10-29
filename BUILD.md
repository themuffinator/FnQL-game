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

You can also run both directly:

```bash
python scripts/check_engine_abi.py --fnql ../FnQL
```

```bash
python scripts/smoke_dedicated.py --engine <fnql.ded.exe> --retail "<Quake Live>"
```
