#!/usr/bin/env python3
"""Let retail bots fight on an FnQL dedicated server running FnQL-game's qagame.

This is the milestone 4 runtime gate: the combat code (weapons, missiles,
G_Damage, radius damage, death and respawn) must run for a sustained match
without engine errors, and bots must score kills with several means of death.

It exercises the code; it does not compare it with retail (that is gate G3).
The retail assets come from the user's legitimate Quake Live install, the
engine from an FnQL build, and the module from this repository's build
directory.  Scratch files go under .tmp/ and are removed.
"""

from __future__ import annotations

import argparse
import collections
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from smoke_dedicated import FORBIDDEN, MODULE_NAME, ROOT, find_retail_install  # noqa: E402

KILL_LINE = re.compile(r"^Kill: \d+ \d+ \d+: .* killed .* by (MOD_[A-Z_]+)\s*$", re.MULTILINE)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("FNQL_DEDICATED_EXE"),
                        help="x86 FnQL dedicated server (default: $FNQL_DEDICATED_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--bots", default="sarge,ranger,visor,keel,anarki")
    parser.add_argument("--skill", type=int, default=4)
    parser.add_argument("--seconds", type=float, default=75.0, help="how long the match must run")
    parser.add_argument("--min-kills", type=int, default=6)
    parser.add_argument("--min-mods", type=int, default=2, help="distinct means of death required")
    parser.add_argument("--port", type=int, default=27996)
    parser.add_argument("--keep", action="store_true", help="keep the scratch home directory")
    args = parser.parse_args()

    if not args.engine or not Path(args.engine).is_file():
        raise SystemExit("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE")
    if not args.module.is_file():
        raise SystemExit(f"module not found: {args.module} (build it first)")
    engine = Path(args.engine).resolve()
    retail = find_retail_install(args.retail)
    bots = [name.strip() for name in args.bots.split(",") if name.strip()]

    scratch_root = ROOT / ".tmp"
    scratch_root.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="combat-soak-", dir=scratch_root))
    try:
        (home / "baseq3").mkdir()
        shutil.copy2(args.module, home / "baseq3" / MODULE_NAME)
        log_path = home / "baseq3" / "qconsole.log"
        startup = [f"map {args.map} {args.factory}", "wait 40"]
        startup += [f"addbot {name} {args.skill}" for name in bots]
        command = [
            str(engine),
            "+set", "fs_basepath", str(retail),
            "+set", "fs_homepath", str(home),
            "+set", "logfile", "2",
            "+set", "net_port", str(args.port),
            "+set", "sv_pure", "0",
            "+set", "bot_enable", "1",
            "+set", "vm_gameProvider", "fnql",
            "+set", "fraglimit", "0",
            "+set", "timelimit", "0",
            "+set", "g_warmup", "0",
            # a dedicated server starts its first map through serverstartup
            "+set", "serverstartup", "; ".join(startup),
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
        mods = collections.Counter(KILL_LINE.findall(log))
        kills = sum(mods.values())
        failures = []
        if exited_early:
            failures.append(f"server exited after {time.monotonic() - started:.1f}s (code {process.returncode})")
        failures += [f"forbidden: {line!r}" for line in FORBIDDEN if line in log]
        failures += [f"bot {name} did not join" for name in bots
                     if f"loaded {name} from bots/{name}_t.c" not in log]
        if kills < args.min_kills:
            failures.append(f"only {kills} kills (need {args.min_kills})")
        if len(mods) < args.min_mods:
            failures.append(f"only {len(mods)} means of death (need {args.min_mods})")

        summary = ", ".join(f"{mod} {count}" for mod, count in mods.most_common())
        if failures:
            print("FnQL-game combat soak FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print(f"kills: {summary or 'none'}")
            print("--- log tail ---")
            print(log[-8000:])
            return 1

        print(f"FnQL-game combat soak OK: {len(bots)} bots on {args.map} for {args.seconds:.0f}s, "
              f"{kills} kills: {summary}")
        return 0
    finally:
        if args.keep:
            print(f"kept {home}")
        else:
            shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
