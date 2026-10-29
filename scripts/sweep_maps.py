#!/usr/bin/env python3
"""Load every stock Quake Live map with FnQL-game's qagame in one server.

Robustness gate next to the boot smoke: each map's entities must spawn and the
game must initialize without an engine error, and the only classnames left
without a spawn function must be ones the retail qagame does not spawn either
(docs/contract/retail_tables.json "spawnTable").  The map list comes from the
user's retail baseq3/pak00.pk3; nothing is copied.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import MODULE_NAME, find_retail_install  # noqa: E402


def stock_maps(retail: Path) -> list[str]:
    with zipfile.ZipFile(retail / "baseq3" / "pak00.pk3") as package:
        names = [Path(n).stem for n in package.namelist()
                 if n.lower().startswith("maps/") and n.lower().endswith(".bsp")]
    return sorted(set(names))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("FNQL_DEDICATED_EXE"))
    parser.add_argument("--retail")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--port", type=int, default=27995)
    parser.add_argument("--timeout", type=float, default=900.0)
    args = parser.parse_args()

    if not args.engine or not Path(args.engine).is_file():
        raise SystemExit("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE")
    engine = Path(args.engine).resolve()
    retail = find_retail_install(args.retail)
    maps = stock_maps(retail)
    retail_spawns = set(json.loads((ROOT / "docs" / "contract" / "retail_tables.json")
                                   .read_text(encoding="utf-8"))["spawnTable"])

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="sweep-maps-", dir=scratch))
    try:
        game_dir = home / "baseq3"
        game_dir.mkdir()
        shutil.copy2(args.module, game_dir / MODULE_NAME)
        lines = []
        for name in maps:
            lines += [f"echo sweep: begin {name}", f"map {name} {args.factory}", "wait 5"]
        lines += ["echo sweep: done", "quit"]
        (game_dir / "sweep.cfg").write_text("\n".join(lines) + "\n", encoding="ascii")

        log_path = game_dir / "qconsole.log"
        command = [str(engine), "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
                   "+set", "logfile", "2", "+set", "net_port", str(args.port), "+set", "sv_pure", "0",
                   "+set", "vm_gameProvider", "fnql", "+exec", "sweep.cfg"]
        process = subprocess.Popen(command, cwd=engine.parent, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        deadline = time.monotonic() + args.timeout
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.5)
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
        log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""

        failures = []
        if "sweep: done" not in log:
            failures.append("the sweep did not finish (crash, hang or timeout)")
        sections = re.split(r"sweep: begin (\S+)\n", log)
        loaded = {}
        for i in range(1, len(sections) - 1, 2):
            loaded[sections[i]] = sections[i + 1]
        unknown: dict[str, set[str]] = {}
        declined: list[str] = []
        for name in maps:
            text = loaded.get(name)
            if text is None:
                failures.append(f"{name}: never attempted")
                continue
            if "------ Server Initialization ------" not in text:
                # The engine declined the map before any game code ran (the
                # retail module behaves identically, e.g. the practice maps).
                declined.append(name)
                continue
            if "------- Game Initialization -------" not in text:
                failures.append(f"{name}: game did not initialize")
            for marker in ("ERROR:", "********************", "Rejected DLL", "loading the retail module instead"):
                if marker in text:
                    failures.append(f"{name}: log contains {marker!r}")
            for classname in re.findall(r"^(\S+) doesn't have a spawn function", text, re.M):
                if classname in retail_spawns:
                    failures.append(f"{name}: {classname} has no spawn function but retail spawns it")
                unknown.setdefault(classname, set()).add(name)

        if failures:
            print(f"map sweep FAILED ({len(failures)}):")
            for failure in failures[:60]:
                print(f"  - {failure}")
            return 1
        print(f"map sweep OK: {len(maps) - len(declined)} stock maps initialized")
        if declined:
            print(f"  declined by the engine before game code ran: {', '.join(declined)}")
        for classname, where in sorted(unknown.items()):
            print(f"  not spawned (retail does not spawn it either): {classname} on {len(where)} maps")
        return 0
    finally:
        shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
