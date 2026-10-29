#!/usr/bin/env python3
"""A retail Quake Live client joins an FnQL server running FnQL-game (gate G5b).

The server is an FnQL dedicated server hosting FnQL-game's qagame on a
retail map.  The client is the user's legitimate retail quakelive_steam.exe,
launched windowed through Steam (`steam -applaunch 282440 ... +connect`); a
directly started client cannot initialize Steam.  The gate passes when the
retail client connects, enters the game and stays in it for the observation
window without being dropped.

The retail client keeps its settings and console log under
<install>/<steamid>/.  The probe stops the client instead of letting it exit
(which would write its config), and still backs up those files before the run
and restores any that changed, so the user's retail settings and logs are
never altered.  It skips (exit 77) without a retail install, without Steam
running, or while a retail client is already running, which it never touches.
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

SKIP = 77
RETAIL_CLIENT = "quakelive_steam.exe"
RETAIL_APPID = "282440"

FORBIDDEN = [
    "is not a usable FnQL-game qagame module",
    "loading the retail module instead",
    "********************",  # Com_Error banner
    "ERROR:",
]


def steam_exe() -> Path | None:
    out = subprocess.run(["powershell", "-NoProfile", "-Command",
                          "(Get-Process steam -ErrorAction SilentlyContinue | Select-Object -First 1).Path"],
                         capture_output=True, text=True).stdout.strip()
    return Path(out) if out and Path(out).is_file() else None


def client_pids() -> set[int]:
    out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {RETAIL_CLIENT}", "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    pids = set()
    for row in out.splitlines():
        fields = [f.strip('"') for f in row.split('","')]
        if len(fields) > 1 and fields[0].lower() == RETAIL_CLIENT:
            pids.add(int(fields[1]))
    return pids


def retail_files(retail: Path) -> dict[Path, bytes]:
    """Everything the retail client may rewrite: configs and each account's console log."""
    files: dict[Path, bytes] = {}
    candidates = list(retail.glob("*.cfg")) + list(retail.glob("baseq3/*.cfg"))
    candidates.append(retail / "baseq3" / "qconsole.log")
    for account in retail.iterdir():
        if account.is_dir() and re.fullmatch(r"-?\d{17}", account.name):
            candidates += list(account.glob("*.cfg")) + list(account.glob("baseq3/*.cfg"))
            candidates.append(account / "baseq3" / "qconsole.log")
    for path in candidates:
        if path.is_file() and not path.is_symlink():
            files[path] = path.read_bytes()
    return files


def restore_files(saved: dict[Path, bytes]) -> list[Path]:
    restored = []
    for path, data in saved.items():
        if not path.is_file() or path.read_bytes() != data:
            path.write_bytes(data)
            restored.append(path)
    return restored


def kill_tree(pid: int) -> None:
    subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("FNQL_DEDICATED_EXE"),
                        help="x86 FnQL dedicated server (default: $FNQL_DEDICATED_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--port", type=int, default=27983)
    parser.add_argument("--pure", default="1")
    parser.add_argument("--bots", type=int, default=1, help="bots to add once the client is in")
    parser.add_argument("--connect-timeout", type=float, default=120.0)
    parser.add_argument("--seconds", type=float, default=30.0, help="how long the client must stay in")
    parser.add_argument("--keep", action="store_true", help="keep the scratch home directory")
    parser.add_argument("--developer", action="store_true", help="log connectionless traffic on the server")
    args = parser.parse_args()

    if not args.engine or not Path(args.engine).is_file():
        raise SystemExit("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE")
    if os.name != "nt":
        print("the retail client is Windows only; skipping")
        return SKIP
    try:
        retail = find_retail_install(args.retail)
    except SystemExit:
        print("no retail Quake Live install; skipping")
        return SKIP
    if not (retail / RETAIL_CLIENT).is_file():
        print(f"no {RETAIL_CLIENT} in {retail}; skipping")
        return SKIP
    steam = steam_exe()
    if steam is None:
        print("Steam is not running; skipping")
        return SKIP
    if client_pids():
        print("a retail client is already running; skipping so it is left alone")
        return SKIP
    engine = Path(args.engine).resolve()

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="retail-client-", dir=scratch))
    saved = retail_files(retail)
    started_at = time.time()
    server = None
    clients: set[int] = set()
    try:
        (home / "baseq3").mkdir()
        shutil.copy2(args.module, home / "baseq3" / MODULE_NAME)
        log_path = home / "baseq3" / "qconsole.log"
        server = subprocess.Popen(
            [str(engine), "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
             "+set", "logfile", "2", "+set", "net_port", str(args.port), "+set", "sv_pure", args.pure,
             "+set", "bot_enable", "1", "+set", "vm_gameProvider", "fnql",
             "+set", "sv_hostname", "FnQL-game retail client probe",
             "+set", "developer", "1" if args.developer else "0",
             "+set", "serverstartup", f"map {args.map} {args.factory}"],
            cwd=engine.parent, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))

        def log() -> str:
            return log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""

        def alive() -> bool:
            return bool(clients & client_pids())

        deadline = time.monotonic() + 30
        while "------- Game Initialization -------" not in log() and time.monotonic() < deadline:
            time.sleep(0.5)
        if "------- Game Initialization -------" not in log():
            print("retail client probe FAILED: the server did not start")
            print(log()[-4000:])
            return 1

        subprocess.run([str(steam), "-applaunch", RETAIL_APPID,
                        "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                        "+set", "r_customwidth", "960", "+set", "r_customheight", "540",
                        "+set", "logfile", "2", "+connect", f"127.0.0.1:{args.port}"],
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=60)
        deadline = time.monotonic() + 60
        while not clients and time.monotonic() < deadline:
            clients = client_pids()
            time.sleep(0.5)

        failures = []
        began = None
        if not clients:
            failures.append("Steam did not start the retail client")
        else:
            deadline = time.monotonic() + args.connect_timeout
            while time.monotonic() < deadline and alive():
                m = re.search(r"ClientBegin: (\d+)", log())
                if m:
                    began = m.group(1)
                    break
                time.sleep(0.5)
            if began is None:
                failures.append("the retail client never entered the game"
                                + ("" if alive() else " (it exited)"))
        if began is not None:
            for _ in range(args.bots):
                server.stdin.write(b"addbot sarge 2\n")
            server.stdin.flush()
            stay = time.monotonic() + args.seconds
            while time.monotonic() < stay and alive():
                time.sleep(0.5)
            server.stdin.write(b"status\n")
            server.stdin.flush()
            time.sleep(2)
            text = log()
            if not alive():
                failures.append("the retail client exited during the game")
            if re.search(rf"ClientDisconnect: {began}\b", text):
                failures.append(f"client {began} disconnected during the observation window")
            if re.search(r"(dropped|timed out|was kicked)", text, re.IGNORECASE):
                failures.append("the server dropped a client")
        text = log()
        failures += [f"forbidden in server log: {line!r}" for line in FORBIDDEN if line in text]

        client_log = ""
        for path in saved.keys() | {retail / "baseq3" / "qconsole.log"} | {
                a / "baseq3" / "qconsole.log" for a in retail.iterdir()
                if a.is_dir() and re.fullmatch(r"\d{17}", a.name)}:
            if path.name == "qconsole.log" and path.is_file() and path.stat().st_mtime >= started_at:
                client_log = path.read_text(encoding="utf-8", errors="replace")
                if args.keep:
                    shutil.copy2(path, home / "retail-client-qconsole.log")

        name = re.search(r'ClientUserinfoChanged: \d+ n\\([^\\]*)', text)
        if failures:
            print("retail client probe FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- server log tail ---")
            print(text[-6000:])
            if client_log:
                print("--- retail client log tail ---")
                print(client_log[-6000:])
            return 1
        print(f"retail client probe OK: {RETAIL_CLIENT} joined {args.map} ({args.factory}, sv_pure "
              f"{args.pure}) on {engine.name} as client {began}"
              + (f" ({name.group(1)})" if name else "") + f" and stayed {args.seconds:.0f}s")
        return 0
    finally:
        for pid in clients:
            kill_tree(pid)
        if server is not None and server.poll() is None:
            try:
                server.stdin.write(b"quit\n")
                server.stdin.flush()
                server.wait(timeout=10)
            except (OSError, subprocess.TimeoutExpired):
                server.kill()
        time.sleep(1)
        # a console log the client created where there was none is the run's own
        logs = [retail / "baseq3" / "qconsole.log"] + [
            a / "baseq3" / "qconsole.log" for a in retail.iterdir() if re.fullmatch(r"\d{17}", a.name)]
        for created in logs:
            if created not in saved and created.is_file():
                created.unlink()
        restored = restore_files(saved)
        if restored:
            print("restored retail files: " + ", ".join(str(p.relative_to(retail)) for p in restored))
        if args.keep:
            print(f"kept {home}")
        else:
            shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
