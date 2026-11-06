#!/usr/bin/env python3
"""A retail Quake Live client joins an FnQL server running FnQL-game (gate G5b).

The server is an FnQL dedicated server hosting FnQL-game's qagame on a
retail map.  The client is the user's legitimate retail quakelive_steam.exe,
launched windowed through Steam (`steam -applaunch 282440 ... +connect`); a
directly started client cannot initialize Steam.  The gate passes when the
retail client connects, enters the game and stays in it for the observation
window without being dropped. It also requires the requested bot to appear
and the retail client to remain in the final authenticated-rcon status query.
--require-auth additionally requires successful Steam session validation.

The retail client keeps its settings and console log under
<install>/<steamid>/.  The probe stops the client instead of letting it exit
(which would write its config), and still backs up those files before the run
and restores any that changed, so the user's retail settings and logs are
never altered.  That includes profile.pid: retail writes its PID there and
rewrites it to "0" on a clean exit; at start-up a leftover PID sets
com_crashed and, unless com_ignoreCrash is set, blocks on an "Enable Safe
Mode?" message box (0x4c93e0, 0x4cc1f9).  A stopped client leaves its PID
behind, so the probe restores the file and launches with com_ignoreCrash 1;
otherwise every run after the first waited on that box and never connected.  It skips (exit 77) without a retail install, without Steam
running, or while a retail client is already running, which it never touches.

--cgame <cgamex86.dll> makes the retail client run FnQL-game's cgame without
altering the retail install.  Observed in quakelive_steam.exe: a module is
looked up in the paks first (0x4d2750), newest search path first, and only in
paks named "bin" (0x4d1770 flags them for code); on a pure server only paks in
the server's pure list qualify.  A hit is extracted to
<fs_homepath>/baseq3/<name>x86.dll and LoadLibrary'd; nothing else checks it
(0x4eceb0 requires only dllEntry).  The home path never mounts paks: it is
added with the directory-only flag (0x4d32d8, 0x4d2f38), which is why a
bin.pk3 there is ignored.  fs_basepath is CVAR_INIT (0x4d31c6), so the client
gets a scratch base path instead: baseq3/pak00.pk3 and web.pak hard-linked
(or copied) from the install, and a bin.pk3 rebuilt from the retail one with
only cgamex86.dll replaced, so uix86.dll and the qagames stay retail.  The
server's home gets the same bin.pk3, so it is in the pure list.  The gate then
also requires the retail process to have loaded cgamex86.dll from the client
home with the module's SHA-256.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import secrets
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import zipfile
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
    candidates.append(retail / "awesomium.log")  # rewritten by every launch
    candidates.append(retail / "baseq3" / "profile.pid")
    candidates.append(retail / "baseq3" / "qconsole.log")
    for account in retail.iterdir():
        if account.is_dir() and re.fullmatch(r"-?\d{17}", account.name):
            candidates += list(account.glob("*.cfg")) + list(account.glob("baseq3/*.cfg"))
            candidates.append(account / "baseq3" / "profile.pid")  # a stopped client leaves its PID
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


def loaded_modules(pid: int, name: str) -> list[Path]:
    """Paths of the modules called `name` that process `pid` has loaded."""
    script = (f"(Get-Process -Id {pid} -ErrorAction SilentlyContinue).Modules | "
              f"Where-Object {{ $_.ModuleName -ieq '{name}' }} | ForEach-Object {{ $_.FileName }}")
    # A 64-bit process sees only the WOW64 thunks of the 32-bit client, so
    # enumerate with the 32-bit PowerShell where Windows provides one.
    windir = Path(os.environ.get("SystemRoot", r"C:\Windows"))
    wow = windir / "SysWOW64" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
    shell = str(wow) if wow.is_file() else "powershell"
    out = subprocess.run([shell, "-NoProfile", "-Command", script],
                         capture_output=True, text=True).stdout
    return [Path(line.strip()) for line in out.splitlines() if line.strip()]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_cgame_pak(path: Path, retail_bin: Path, module: Path) -> None:
    """The retail bin.pk3 with cgamex86.dll replaced by `module`."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(retail_bin) as source, zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as pak:
        names = source.namelist()
        if "cgamex86.dll" not in names:
            raise SystemExit(f"{retail_bin} carries no cgamex86.dll")
        for item in source.infolist():
            data = module.read_bytes() if item.filename == "cgamex86.dll" else source.read(item)
            info = zipfile.ZipInfo(item.filename, date_time=item.date_time)
            info.compress_type = zipfile.ZIP_DEFLATED
            pak.writestr(info, data)


def mirror_file(source: Path, target: Path) -> None:
    """Hard-link `source` to `target` (read-only use), copying across volumes."""
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)


def scratch_basepath(base: Path, retail: Path, module: Path) -> None:
    """A base path the retail client mounts in place of the install's."""
    for name in ("web.pak",):
        if (retail / name).is_file():
            mirror_file(retail / name, base / name)
    for pak in sorted((retail / "baseq3").glob("*.pk3")):
        if pak.name.lower() != "bin.pk3":
            mirror_file(pak, base / "baseq3" / pak.name)
    write_cgame_pak(base / "baseq3" / "bin.pk3", retail / "baseq3" / "bin.pk3", module)


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
    parser.add_argument("--require-auth", action="store_true",
                        help="also require the engine's successful Steam session validation")
    parser.add_argument("--cgame", type=Path,
                        help="FnQL-game cgamex86.dll for the retail client, served from a scratch home pak")
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
    if args.cgame is not None and not args.cgame.is_file():
        raise SystemExit(f"no cgame module at {args.cgame}")

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="retail-client-", dir=scratch))
    saved = retail_files(retail)
    started_at = time.time()
    server = None
    clients: set[int] = set()
    rcon_password = secrets.token_hex(16)

    def server_command(command: str) -> str:
        # Windows headless servers do not necessarily consume redirected
        # stdin. Use the engine's authenticated console protocol on loopback.
        payload = b"\xff\xff\xff\xff" + f"rcon {rcon_password} {command}\n".encode("ascii")
        replies = []
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(0.5)
            sock.connect(("127.0.0.1", args.port))
            sock.send(payload)
            while True:
                try:
                    replies.append(sock.recv(65535).decode("latin-1"))
                except (socket.timeout, ConnectionResetError):
                    break
        return "\n".join(replies)

    try:
        (home / "baseq3").mkdir()
        shutil.copy2(args.module, home / "baseq3" / MODULE_NAME)
        log_path = home / "baseq3" / "qconsole.log"
        client_home = home / "client"
        client_args: list[str] = []
        if args.cgame is not None:
            client_base = home / "client-base"
            scratch_basepath(client_base, retail, args.cgame)
            (client_home / "baseq3").mkdir(parents=True)
            shutil.copy2(client_base / "baseq3" / "bin.pk3", home / "baseq3" / "bin.pk3")
            client_args = ["+set", "fs_basepath", str(client_base), "+set", "fs_homepath", str(client_home)]
        server = subprocess.Popen(
            [str(engine), "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
             "+set", "logfile", "2", "+set", "net_port", str(args.port), "+set", "sv_pure", args.pure,
             "+set", "bot_enable", "1", "+set", "vm_gameProvider", "fnql",
             "+set", "rconPassword", rcon_password,
             "+set", "sv_hostname", "FnQL-game retail client probe",
             "+set", "developer", "1" if args.developer or args.require_auth else "0",
             "+set", "serverstartup", f"map {args.map} {args.factory}"],
            cwd=home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
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

        subprocess.run([str(steam), "-applaunch", RETAIL_APPID, *client_args,
                        "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                        "+set", "in_mouse", "0", "+set", "in_nograb", "1",
                        "+set", "com_ignoreCrash", "1",
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
                server_command("addbot sarge 2")
            stay = time.monotonic() + args.seconds
            while time.monotonic() < stay and alive():
                time.sleep(0.5)
            status = server_command("status")
            (home / "server-status.txt").write_text(status, encoding="utf-8")
            time.sleep(2)
            text = log()
            if not alive():
                failures.append("the retail client exited during the game")
            if re.search(rf"ClientDisconnect: {began}\b", text):
                failures.append(f"client {began} disconnected during the observation window")
            if re.search(r"(dropped|timed out|was kicked)", text, re.IGNORECASE):
                failures.append("the server dropped a client")
            if args.bots and not re.search(r"ClientUserinfoChanged: \d+ .*\\skill\\", text):
                failures.append("the server did not add the requested bot")
            if not re.search(rf"(?m)^\s*{began}\s+", status):
                failures.append("the retail client was absent from the final server status")
            if args.require_auth and f"Platform authentication validated client {began}." not in text:
                failures.append("the engine did not confirm Steam session validation")
            if args.cgame is not None and alive():
                wanted = sha256(args.cgame)
                loaded = [p for pid in clients for p in loaded_modules(pid, "cgamex86.dll")]
                (home / "client-cgame-modules.txt").write_text(chr(10).join(map(str, loaded)), encoding="utf-8")
                ours = [p for p in loaded if p.is_file() and p.resolve().is_relative_to(client_home.resolve())
                        and sha256(p) == wanted]
                if not ours:
                    failures.append("the retail client did not load FnQL-game's cgame from the client home"
                                    + (f" (loaded: {', '.join(map(str, loaded))})" if loaded else ""))
        text = log()
        failures += [f"forbidden in server log: {line!r}" for line in FORBIDDEN if line in text]

        client_log = ""
        if (client_home / "baseq3" / "qconsole.log").is_file():
            client_log = (client_home / "baseq3" / "qconsole.log").read_text(encoding="utf-8", errors="replace")
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
              + (f" ({name.group(1)})" if name else "") + f" and stayed {args.seconds:.0f}s"
              + (" running FnQL-game's cgame" if args.cgame is not None else ""))
        return 0
    finally:
        if server is not None and server.poll() is None:
            try:
                server_command("quit")
                server.wait(timeout=10)
            except (OSError, subprocess.TimeoutExpired):
                server.kill()
                server.wait(timeout=10)
        # End the server session before cancelling the client's Steam ticket;
        # otherwise teardown can leave a misleading auth rejection in the log.
        for pid in clients:
            kill_tree(pid)
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
