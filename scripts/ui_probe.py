#!/usr/bin/env python3
"""Windowed, input-free native ui hosting probe using installed retail assets.

The module is copied under the retail filename only inside a disposable,
explicitly unpure home until FnQL's client provider/purity work is enabled.
This is a hosting gate, not a claim of pure-server or WebUI-service compatibility.
"""

from __future__ import annotations
import argparse
import shutil
import stat
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from smoke_dedicated import find_retail_install

ROOT = Path(__file__).resolve().parents[1]

def linked(path: Path) -> bool:
    info = path.lstat()
    return stat.S_ISLNK(info.st_mode) or bool(getattr(info, 'st_file_attributes', 0) & 0x400)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', type=Path, default=ROOT/'build/win32-x86/fnql_uix86.dll')
    parser.add_argument('--retail')
    parser.add_argument('--qagame', type=Path)
    parser.add_argument('--map', default='campgrounds')
    parser.add_argument('--factory', default='ffa')
    parser.add_argument('--timeout', type=float, default=90)
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    client = args.client.resolve(strict=True)
    module = args.module.resolve(strict=True)
    for token in (args.map, args.factory):
        if not re.fullmatch(r'[A-Za-z0-9_/-]+', token):
            parser.error('map and factory must be command-safe names')
    for binary in (client, module):
        data = binary.read_bytes()
        if data[:2] != b'MZ': parser.error('this hosting probe currently requires Windows x86 PE files')
        pe = int.from_bytes(data[60:64], 'little')
        if data[pe:pe+6] != b'PE\0\0\x4c\x01': parser.error(f'not an x86 PE file: {binary}')
    retail = find_retail_install(args.retail)
    scratch = ROOT/'.tmp/ui'
    scratch.mkdir(parents=True, exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix='probe-', dir=scratch)).resolve()
    process = None
    try:
        base = home/'baseq3'; base.mkdir()
        shutil.copy2(module, base/'uix86.dll')
        if args.qagame:
            shutil.copy2(args.qagame.resolve(strict=True), base/'fnql_qagamex86.dll')
        (base/'probe.cfg').write_text('\n'.join([
            'wait 30', 'vminfo', 'ui_openmenu main', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_load', 'wait 10', 'ui_openmenu main',
            f'map {args.map} {args.factory}', 'wait 120',
            'ui_openmenu ingame', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_openmenu ingame_callvote', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_openmenu ingame_addbot', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_menuaction ingame_addbot botsettings after-bot.cfg', '']), encoding='ascii')
        (base/'after-bot.cfg').write_text('\n'.join([
            'wait 60', 'echo ui-bot-added', 'status',
            'ui_openmenu ingame_admin', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_openmenu ingame_join', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_openmenu ingame_controls', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_openmenu ingame_options', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_menuaction ingame_options btn2', 'wait 10', 'screenshotJPEG', 'wait 10',
            'ui_menuaction ingame_options advTeamButton', 'wait 10', 'screenshotJPEG', 'wait 10',
            'echo ui-probe-done', 'quit', '']), encoding='ascii')
        command = [str(client)]
        settings = {
            'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
            'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': '960', 'r_customheight': '600',
            'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0',
            's_initsound': '0', 'logfile': '2', 'developer': '1',
            'cl_allowDownload': '0', 'cl_webuiEnable': '0', 'net_port': '28097', 'com_maxfps': '125',
            'vm_gameProvider': 'fnql' if args.qagame else 'retail',
        }
        for key, value in settings.items(): command += ['+set', key, value]
        command += ['+exec', 'probe.cfg']
        startup = None
        if sys.platform == 'win32':
            startup = subprocess.STARTUPINFO()
            startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = 4  # SW_SHOWNOACTIVATE; never control the user's input.
        process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   startupinfo=startup)
        deadline = time.monotonic() + args.timeout
        log = ''
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.25)
            path = base/'qconsole.log'
            if path.is_file():
                log = path.read_text(encoding='utf-8', errors='replace')
                if 'Server fatal crashed:' in log or '********************' in log:
                    break
        timed_out = process.poll() is None
        if timed_out: process.kill()
        process.wait(timeout=10)
        path = base/'qconsole.log'
        if path.is_file(): log = path.read_text(encoding='utf-8', errors='replace')
        required = ['VM_LoadDll(ui): FnQL-game', 'Going from CS_PRIMED to CS_ACTIVE',
                    'ui-probe-done', 'UI: opened main',
                    'UI: action ingame_addbot/botsettings', 'UI: opened ingame_callvote',
                    'UI: opened ingame_admin', 'UI: opened ingame_join',
                    'UI: opened ingame_controls', 'UI: opened ingame_options',
                    'UI: action ingame_options/btn2', 'UI: action ingame_options/advTeamButton']
        failures = [f'missing {s}' for s in required if s not in log]
        bot_status = log.partition('ui-bot-added')[2].partition('UI: opened ingame_admin')[0]
        if not re.search(r'\b(?:bot|BOT)\b', bot_status):
            failures.append('add-bot action did not create a bot')
        if timed_out: failures.append('client did not exit normally')
        if process.returncode: failures.append(f'client exit {process.returncode}')
        if any(s in log for s in ['ERROR:', '********************', 'Rejected DLL', 'unknown menu keyword', 'unknown menu item keyword']): failures.append('engine/module error')
        shots = list((base/'screenshots').glob('*.jpg'))
        if len(shots) < 10: failures.append(f'expected ten engine screenshots, got {len(shots)}')
        if failures:
            print('ui hosting FAILED: '+', '.join(failures))
            print(log[-6500:])
            return 1
        print(f'ui hosting OK: {args.map}/{args.factory}; '+('FnQL-game' if args.qagame else 'retail')+' qagame')
        return 0
    finally:
        if process and process.poll() is None:
            process.kill(); process.wait(timeout=10)
        if args.keep:
            print(f'Evidence: {home}')
        else:
            # Only this task's fresh, private directory; reject linked descendants.
            if home.parent != scratch.resolve() or linked(home):
                raise RuntimeError('unsafe probe cleanup path')
            if any(linked(p) for p in home.rglob('*')):
                raise RuntimeError('linked probe output; preserving for inspection')
            shutil.rmtree(home)

if __name__ == '__main__':
    sys.exit(main())
