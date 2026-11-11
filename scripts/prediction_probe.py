#!/usr/bin/env python3
"""Count client prediction misses with retail or FnQL-game modules (gates G4a/G4).

The chosen cgame predicts the local player and compares
each prediction with the server's playerState; with cg_showmiss 1 it prints
" Prediction miss: <distance> { dx dy dz }" for every disagreement and
"prediction error" when the command stream itself diverges.  On a listen
server there is no injected network loss or lag. A miss is evidence of a
prediction discrepancy; it still needs diagnosis before attributing it to
movement arithmetic, caching, item timing or the engine's command stream.

The probe drives the local player through a scripted movement course
(running, strafing, jumping, crouching, turning) with console button
commands and reports the misses.  With --max-misses (default 0) it is a
gate; --report prints every miss for parity work.  The client runs windowed
with sound off.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import MODULE_NAME, find_retail_install  # noqa: E402
import headless  # noqa: E402

# Movement course, in client frames (wait N).  Each segment exercises a
# different part of Pmove; the names tag the misses in the report.
COURSE = [
    ("run", ["+forward", "wait 120", "-forward"]),
    ("strafe", ["+moveleft", "wait 60", "-moveleft", "+moveright", "wait 60", "-moveright"]),
    ("jump", ["+moveup", "wait 4", "-moveup", "wait 80"]),
    ("run-jump", ["+forward", "wait 20", "+moveup", "wait 4", "-moveup", "wait 60", "-forward"]),
    ("crouch", ["+movedown", "wait 40", "+forward", "wait 60", "-forward", "-movedown"]),
    ("turn-run", ["+right", "+forward", "wait 90", "-forward", "-right"]),
    ("back", ["+back", "wait 80", "-back"]),
    ("idle", ["wait 60"]),
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--client", default=os.environ.get("FNQL_CLIENT_EXE"))
    parser.add_argument("--retail")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--cgame", type=Path, help="load this cgame inside the private unpure profile")
    parser.add_argument("--retail-qagame", action="store_true", help="use installed retail qagame")
    parser.add_argument("--keep", action="store_true", help="retain the private profile and evidence")
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--port", type=int, default=27991)
    parser.add_argument("--max-misses", type=int, default=0)
    parser.add_argument("--report", action="store_true", help="print every miss")
    parser.add_argument("--timeout", type=float, default=200.0)
    parser.add_argument("--set", nargs=2, action="append", default=[], metavar=("CVAR", "VALUE"),
                        help="server cvar to set before the map loads (repeatable)")
    args = parser.parse_args()

    if not args.client or not Path(args.client).is_file():
        raise SystemExit("pass --client <fnql.exe> or set FNQL_CLIENT_EXE")
    client = Path(args.client).resolve()
    retail = find_retail_install(args.retail)

    scratch = (ROOT / ".tmp" / "prediction").resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="probe-", dir=scratch)).resolve()
    process = None
    try:
        game_dir = home / "baseq3"
        game_dir.mkdir()
        if not args.retail_qagame:
            shutil.copy2(args.module.resolve(strict=True), game_dir / MODULE_NAME)
        if args.cgame:
            shutil.copy2(args.cgame.resolve(strict=True), game_dir / MODULE_NAME.replace("fnql_qagame", "cgame"))
        lines = [f"set {name} {value}" for name, value in args.set]
        # Quake Live puts a connecting player on the spectators; join the game,
        # then ask for the team so the log proves the course runs in play
        lines += [f"map {args.map} {args.factory}", "wait 300", "team f", "wait 100", "team",
                  *(name for name, _ in args.set), "set cg_showmiss 1"]
        for name, steps in COURSE:
            lines += [f"echo course: {name}", *steps]
        lines += ["set cg_showmiss 0", "vminfo", "echo course: done", "quit"]
        (game_dir / "course.cfg").write_text("\n".join(lines) + "\n", encoding="ascii")

        command = [str(client), *headless.CLIENT,
                   "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
                   "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                   "+set", "r_customwidth", "800", "+set", "r_customheight", "500",
                   "+set", "s_initsound", "0", "+set", "logfile", "2",
                   "+set", "sv_pure", "0", "+set", "developer", "1",
                   "+set", "in_mouse", "0", "+set", "in_joystick", "0", "+set", "in_nograb", "1",
                   "+set", "cl_allowDownload", "0", "+set", "net_port", str(args.port),
                   "+set", "com_maxfps", "125", "+set", "vm_gameProvider",
                   "retail" if args.retail_qagame else "fnql", "+exec", "course.cfg"]
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
        if timed_out or "course: done" not in log:
            print("prediction probe FAILED: the course did not finish")
            print(log[-4000:])
            return 1
        if "Team Free" not in log:
            print("prediction probe FAILED: the player did not join the game")
            print(log[-4000:])
            return 1
        if process.returncode or "Going from CS_PRIMED to CS_ACTIVE" not in log or any(
                text in log for text in ("ERROR:", "********************", "Rejected DLL")):
            print("prediction probe FAILED: client/module did not complete normally")
            print(log[-4000:])
            return 1
        if args.cgame and "VM_LoadDll(cgame): FnQL-game" not in log:
            print("prediction probe FAILED: FnQL-game cgame was not loaded")
            return 1

        misses: dict[str, list[str]] = {}
        segment = "setup"
        for line in log.splitlines():
            marker = re.match(r"course: (\S+)", line)
            if marker:
                segment = marker.group(1)
            elif "Prediction miss" in line or "prediction error" in line:
                misses.setdefault(segment, []).append(line.strip())
        total = sum(len(v) for v in misses.values())
        summary = ", ".join(f"{name} {len(v)}" for name, v in misses.items()) or "none"
        print(f"prediction probe: {total} misses ({summary}) on {args.map} ({args.factory})")
        settings_match = True
        for name, wanted in args.set:
            seen = re.search(rf'"{re.escape(name)}" is:"([^"]*)"', log)
            value = re.sub(r"\^\d", "", seen.group(1)) if seen else "?"
            print(f"   {name} = {value}" + ("" if value == wanted else f" (asked for {wanted})"))
            settings_match = settings_match and value == wanted
        if args.report:
            for name, entries in misses.items():
                print(f"== {name}")
                for entry in entries[:40]:
                    print(f"   {entry}")
        return 0 if total <= args.max_misses and settings_match else 1
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait(timeout=10)
        if args.keep:
            print(f"Evidence: {home}")
        else:
            # Delete only this resolved private profile, never through links.
            if home.parent != scratch:
                raise RuntimeError(f"unsafe prediction profile: {home}")
            for path in [home, *home.rglob('*')]:
                attributes = getattr(path.lstat(), 'st_file_attributes', 0)
                if path.is_symlink() or attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                    raise RuntimeError(f"linked prediction profile entry: {path}")
            shutil.rmtree(home)


if __name__ == "__main__":
    sys.exit(main())
