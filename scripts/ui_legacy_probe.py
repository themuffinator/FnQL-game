#!/usr/bin/env python3
"""Capture the same installed menus through retail console commands in either UI.

Uses engine screenshots only. No keyboard/mouse events or retail-file writes.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from smoke_dedicated import find_retail_install
from ui_probe import linked

ROOT = Path(__file__).resolve().parents[1]
MENUS = ('ingame_about', 'ingame', 'ingame_callvote', 'ingame_addbot', 'ingame_admin',
         'ingame_join', 'ingame_controls', 'ingame_options', 'demo')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', type=Path, help='omit to run the installed retail UI')
    parser.add_argument('--retail')
    parser.add_argument('--width', type=int, default=960)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    client = args.client.resolve(strict=True)
    module = args.module.resolve(strict=True) if args.module else None
    for binary in (client, module):
        if binary is None:
            continue
        data = binary.read_bytes()
        pe = int.from_bytes(data[60:64], 'little')
        if data[:2] != b'MZ' or data[pe:pe + 6] != b'PE\0\0\x4c\x01':
            parser.error(f'expected Windows x86 artifact: {binary}')
    retail = find_retail_install(args.retail)
    scratch = ROOT / '.tmp/ui'
    scratch.mkdir(parents=True, exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix='legacy-' + ('fnql-' if module else 'retail-'), dir=scratch)).resolve()
    process = None
    try:
        base = home / 'baseq3'
        base.mkdir()
        if module:
            shutil.copy2(module, base / 'uix86.dll')
        commands = ['wait 40', 'menu_open main', 'wait 15', 'screenshotPNG main',
                    'wait 5', 'menu_close main', 'map campgrounds ffa', 'wait 140',
                    'togglemenu', 'wait 20', 'menu_close ingame_about', 'menu_close ingame']
        for menu in MENUS:
            commands += [f'menu_open {menu}', 'wait 15', f'screenshotPNG {menu}',
                         'wait 5', f'menu_close {menu}']
            if module and menu == 'ingame_about':
                commands += ['ui_report ingame_about']
        commands += ['echo legacy-ui-probe-done', 'quit', '']
        (base / 'probe.cfg').write_text('\n'.join(commands), encoding='ascii')
        settings = {
            'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
            'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(args.width),
            'r_customheight': str(args.height), 'in_mouse': '0', 'in_nograb': '1',
            'in_joystick': '0', 's_initsound': '0', 'logfile': '2', 'developer': '1',
            'cl_webuiEnable': '0', 'web_browserActive': '0', 'cl_allowDownload': '0',
            'vm_gameProvider': 'retail', 'net_port': '28117', 'com_maxfps': '125',
        }
        command = [str(client)]
        for key, value in settings.items():
            command += ['+set', key, value]
        command += ['+exec', 'probe.cfg']
        startup = None
        if sys.platform == 'win32':
            startup = subprocess.STARTUPINFO()
            startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = 4
        process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   startupinfo=startup)
        deadline = time.monotonic() + 90
        while process.poll() is None and time.monotonic() < deadline:
            log_path = base / 'qconsole.log'
            if log_path.exists():
                pending = log_path.read_text(encoding='utf-8', errors='replace')
                if any(marker in pending for marker in ('Unhandled exception', 'Server fatal crashed:',
                                                        '********************', 'VM_Create on UI failed')):
                    break
            time.sleep(.25)
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
        log = (base / 'qconsole.log').read_text(encoding='utf-8', errors='replace')
        missing = [name for name in ('main',) + MENUS if not (base / 'screenshots' / (name + '.png')).is_file()]
        required = ['legacy-ui-probe-done', 'Going from CS_PRIMED to CS_ACTIVE']
        if module:
            required += ['VM_LoadDll(ui): FnQL-game', 'UI: menu ingame_about items=89 catcher=2',
                         'UI: item match_details type=8 owner=557', 'UI: item startingweapons type=8 owner=558']
        failures = [f'missing {marker}' for marker in required if marker not in log]
        if any(marker in log for marker in ('ERROR:', '********************', 'Rejected DLL',
                                            'unknown menu keyword', 'unknown menu item keyword')):
            failures.append('engine or menu parser error')
        if process.returncode or missing or failures:
            print(f'Legacy capture failed: exit={process.returncode}, missing={missing}, errors={failures}\n{log[-4000:]}')
            return 1
        print(f'Legacy capture OK: {"FnQL-game" if module else "retail"}, {args.width}x{args.height}')
        return 0
    finally:
        if process and process.poll() is None:
            process.kill()
            process.wait(timeout=10)
        if args.keep:
            print(f'Evidence: {home}')
        else:
            if home.parent != scratch.resolve() or linked(home) or any(linked(p) for p in home.rglob('*')):
                raise RuntimeError('linked or unexpected probe output; preserving for inspection')
            shutil.rmtree(home)


if __name__ == '__main__':
    sys.exit(main())
