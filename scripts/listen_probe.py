#!/usr/bin/env python3
"""Run the retail cgame against FnQL-game's qagame in one FnQL client.

First interoperability gate (G5a): an FnQL listen server loads FnQL-game's
qagame while the client half runs the retail cgamex86.dll from the user's
install.  The retail cgame is exactly what a retail Quake Live client runs,
so everything it receives -- gamestate, configstrings, snapshots, server
commands -- crosses the same contract a retail peer sees.  The probe adds a
bot, takes a screenshot (kept with --keep for inspection) and requires:

  - qagame provided by FnQL-game and cgame by the retail module;
  - the retail cgame initialized and the client reached CS_ACTIVE;
  - no engine or cgame error.

The client runs windowless (com_headless 2) with sound off, and with Steam
and the WebUI at their defaults.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import MODULE_NAME, find_retail_install  # noqa: E402
import headless  # noqa: E402

REQUIRED = [
    "qagame : native " + MODULE_NAME + " (FnQL-game ",
    "cgame : native cgamex86.dll (unidentified)",
    "CL_InitCGame:",
    "Going from CS_PRIMED to CS_ACTIVE",
    "Sarge^7 connected",  # retail announces connects only after the first five seconds
    "probe: done",
]
FORBIDDEN = ["ERROR:", "********************", "Rejected DLL", "loading the retail module instead"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--client", default=os.environ.get("FNQL_CLIENT_EXE"),
                        help="x86 FnQL client (default: $FNQL_CLIENT_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--factories", type=Path, help="additional hand-authored factory fixture")
    parser.add_argument("--expect-gametype", type=int, choices=range(13),
                        help="require the selected retail gametype value")
    parser.add_argument("--cheats", action="store_true",
                        help="use devmap for fixtures outside retail arena mode metadata")
    parser.add_argument("--port", type=int, default=27993)
    parser.add_argument("--timeout", type=float, default=150.0)
    parser.add_argument("--keep", action="store_true", help="keep the scratch home (log, screenshot)")
    args = parser.parse_args()

    if not args.client or not Path(args.client).is_file():
        raise SystemExit("pass --client <fnql.exe> or set FNQL_CLIENT_EXE")
    client = Path(args.client).resolve()
    retail = find_retail_install(args.retail)
    if args.factories and not args.factories.is_file():
        parser.error("--factories needs an existing file")

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="listen-probe-", dir=scratch))
    try:
        game_dir = home / "baseq3"
        game_dir.mkdir()
        shutil.copy2(args.module, game_dir / MODULE_NAME)
        if args.factories:
            scripts_dir = game_dir / "scripts"
            scripts_dir.mkdir()
            shutil.copy2(args.factories, scripts_dir / "listen.factories")
        (game_dir / "probe.cfg").write_text("\n".join([
            f"{'devmap' if args.cheats else 'map'} {args.map} {args.factory}",
            "wait 1000", "addbot sarge 3", "wait 400",
            "screenshotJPEG", "wait 60", "vminfo", "g_gametype", "echo probe: done", "quit", ""]), encoding="ascii")
        # Windowless client with Steam and the WebUI at their defaults (on),
        # as in retail (AGENTS.md rules 13 and 14).
        command = [str(client), *headless.CLIENT,
                   "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
                   "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                   "+set", "r_customwidth", "1024", "+set", "r_customheight", "640",
                   "+set", "in_mouse", "0", "+set", "in_nograb", "1", "+set", "in_joystick", "0",
                   "+set", "s_initsound", "0", "+set", "logfile", "2", "+set", "developer", "1",
                   "+set", "cl_allowDownload", "0", "+set", "net_port", str(args.port),
                   "+set", "vm_gameProvider", "fnql", "+exec", "probe.cfg"]
        process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   **headless.popen_options())
        deadline = time.monotonic() + args.timeout
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.5)
        timed_out = process.poll() is None
        if timed_out:
            process.kill()
        process.wait(timeout=10)

        log_path = game_dir / "qconsole.log"
        log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
        failures = ["client did not quit before the timeout"] if timed_out else []
        failures += [f"missing: {line!r}" for line in REQUIRED if line not in log]
        failures += [f"forbidden: {line!r}" for line in FORBIDDEN if line in log]
        if args.expect_gametype is not None:
            selected = re.search(r'"g_gametype" is:"([^"\^]*)', log)
            if not selected or selected.group(1) != str(args.expect_gametype):
                failures.append(f"expected g_gametype {args.expect_gametype}, got "
                                + (selected.group(1) if selected else "no cvar report"))
        shots = sorted((game_dir / "screenshots").glob("*.jpg")) if (game_dir / "screenshots").is_dir() else []
        if not shots:
            failures.append("no screenshot was written")

        if failures:
            print("listen probe FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- log tail ---")
            print(log[-6000:])
            return 1
        print(f"listen probe OK: retail cgame ran against FnQL-game qagame on {args.map} ({args.factory})")
        if args.keep:
            print(f"  screenshot: {shots[-1]}")
        return 0
    finally:
        if args.keep:
            print(f"kept {home}")
        else:
            shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
