#!/usr/bin/env python3
"""Boot an FnQL dedicated server with FnQL-game's qagame on a retail map.

This is the milestone 1 runtime gate: the module must load through the retail
native ABI, identify itself to the engine, initialize the game and botlib on a
retail map, host a bot, and keep running frames without engine errors.

Nothing here is bundled: the retail assets come from the user's legitimate
Quake Live install, the engine from an FnQL build, and the module from this
repository's build directory.  Scratch files go under .tmp/ and are removed.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_NAME = "fnql_qagamex86.dll" if os.name == "nt" else "fnql_qagamei386.so"

# Each line must appear in the server log for the gate to pass.
REQUIRED = [
    "VM_LoadDll(qagame): FnQL-game ",
    "qagame : native " + MODULE_NAME + " (FnQL-game ",
    "------- Game Initialization -------",
    "\\fnql_game\\",  # identity key reached the serverinfo
    "AAS initialized.",
    "loaded sarge from bots/sarge_t.c",
    "ClientConnect: 0",
    "ClientBegin: 0",
    "FnQL-game smoke: commands done",
]

# Any of these means the module or the ABI is broken.
FORBIDDEN = [
    "is not a usable FnQL-game qagame module",
    "loading the retail module instead",
    "Rejected DLL",
    "********************",  # Com_Error banner
    "ERROR:",
    "Couldn't open logfile: \n",  # garbage cvar strings (vmCvar_t layout drift)
]


def find_retail_install(explicit: str | None) -> Path:
    candidates = [explicit, os.environ.get("FNQL_RETAIL_QL_PATH")]
    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    candidates.append(str(Path(program_files_x86) / "Steam" / "steamapps" / "common" / "Quake Live"))
    for candidate in candidates:
        if candidate and (Path(candidate) / "baseq3" / "pak00.pk3").is_file():
            return Path(candidate)
    raise SystemExit("retail Quake Live not found; pass --retail or set FNQL_RETAIL_QL_PATH")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("FNQL_DEDICATED_EXE"),
                        help="x86 FnQL dedicated server (default: $FNQL_DEDICATED_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--seconds", type=float, default=20.0, help="how long the server must keep running")
    parser.add_argument("--port", type=int, default=27997)
    parser.add_argument("--keep", action="store_true", help="keep the scratch home directory")
    args = parser.parse_args()

    if not args.engine or not Path(args.engine).is_file():
        raise SystemExit("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE")
    if not args.module.is_file():
        raise SystemExit(f"module not found: {args.module} (build it first)")
    engine = Path(args.engine).resolve()
    retail = find_retail_install(args.retail)

    scratch_root = ROOT / ".tmp"
    scratch_root.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="smoke-dedicated-", dir=scratch_root))
    try:
        (home / "baseq3").mkdir()
        shutil.copy2(args.module, home / "baseq3" / MODULE_NAME)
        log_path = home / "baseq3" / "qconsole.log"
        command = [
            str(engine),
            "+set", "fs_basepath", str(retail),
            "+set", "fs_homepath", str(home),
            "+set", "logfile", "2",
            "+set", "net_port", str(args.port),
            "+set", "sv_pure", "0",
            "+set", "bot_enable", "1",
            "+set", "vm_gameProvider", "fnql",
            "+map", args.map, args.factory,
            "+vminfo",
            "+wait", "40",
            "+addbot", "sarge", "3",
            "+wait", "200",
            "+echo", "FnQL-game smoke: commands done",
            "+status",
        ]
        process = subprocess.Popen(command, cwd=engine.parent, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        started = time.monotonic()
        exited_early = False
        while time.monotonic() - started < args.seconds:
            if process.poll() is not None:
                exited_early = True
                break
            time.sleep(0.25)
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)

        log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
        failures = []
        if exited_early:
            failures.append(f"server exited after {time.monotonic() - started:.1f}s (code {process.returncode})")
        failures += [f"missing: {line!r}" for line in REQUIRED if line not in log]
        failures += [f"forbidden: {line!r}" for line in FORBIDDEN if line in log]

        if failures:
            print("FnQL-game dedicated smoke FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- log tail ---")
            print(log[-8000:])
            return 1

        print(f"FnQL-game dedicated smoke OK: {args.map} ({args.factory}) ran {args.seconds:.0f}s "
              f"with a bot on {engine.name}")
        return 0
    finally:
        if args.keep:
            print(f"kept {home}")
        else:
            shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
