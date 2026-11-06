#!/usr/bin/env python3
"""Require scripted menu interaction to render as the retail UI renders it.

The test-only input proxy (tests/ui_input_proxy.c) is installed as the probe
home's uix86.dll and loads the UI named by FNQL_UI_PROXY_TARGET: the retail
uix86.dll read from the install's baseq3/bin.pk3 into the probe's private
scratch, or FnQL-game's module. Its pxmouse, pxkey and pxchar console commands
feed both UIs the same frame-scheduled input through their own key and mouse
exports, and pxclock gives both the same synthetic UI time, so pulsing focus
colours and the edit-field cursor blink land on the same phase.

Scenarios, on a cheat-enabled local map with the world, entities and HUD
hidden:

- hover a navigation button of the in-game menu (focus highlight);
- hover, then drag, the options page's master-volume slider;
- click a player head in the options page's model list;
- click the ADVANCED tab and its WEAPON CONFIG sub-tab;
- focus an edit field, toggle insert mode, type, capture both cursor-blink
  phases, then commit with Enter;
- click a controls binding (waiting for a key), then bind a key;
- click the call-vote map list, then move its selection with an arrow key.

Captures are engine screenshots, windowed, with mouse and joystick input
disabled, in private unpure probe homes; installed files are never changed.
A pixel counts as different only above the parity gate's THRESHOLD; every
capture must have none. A failing comparison is retried once.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from smoke_dedicated import find_retail_install
from ui_menu_parity_probe import THRESHOLD, compare, remove

ROOT = Path(__file__).resolve().parents[1]
CAPTURES = ('ix_nav_hover', 'ix_slider_hover', 'ix_slider_drag', 'ix_head_click', 'ix_tab_advanced',
            'ix_tab_weapconfig', 'ix_edit_typing', 'ix_edit_typing2', 'ix_edit_commit',
            'ix_bind_waiting', 'ix_bind_done', 'ix_list_click', 'ix_list_arrow')
K_MOUSE1, K_INS, K_ENTER, K_DOWNARROW = 178, 135, 13, 133


def canvas(width: int, height: int) -> tuple[float, float, float]:
    """(x scale, y scale, x bias) of the retail 640x480 canvas (UI_ComputeCanvas)."""
    if width * 480 > height * 640:
        return height / 480, height / 480, (width - height * 4 / 3) / 2
    return width / 640, height / 480, 0.0


def script(width: int, height: int) -> list[str]:
    xs, ys, bias = canvas(width, height)
    at = lambda x, y: f'pxmouse {round(bias + xs * x)} {round(ys * y)}'
    click = [f'pxkey {K_MOUSE1} 1', 'wait 2', f'pxkey {K_MOUSE1} 0']
    shot = lambda name: ['wait 40', f'screenshotPNG {name}', 'wait 5']
    c = ['wait 40', 'devmap campgrounds ffa', 'wait 140', 'r_drawworld 0', 'r_drawentities 0', 'cg_draw2D 0',
         'wait 20', 'togglemenu', 'wait 20', 'pxclock 8']
    c += [at(228, 72), 'wait 2'] + shot('ix_nav_hover')
    c += ['menu_close ingame_about', 'menu_open ingame_options', 'wait 20', at(241, 177), 'wait 2']
    c += shot('ix_slider_hover')
    c += [at(200, 177), 'wait 2', f'pxkey {K_MOUSE1} 1', 'wait 2', at(260, 177), 'wait 4', at(275, 177), 'wait 4',
          f'pxkey {K_MOUSE1} 0'] + shot('ix_slider_drag')
    c += [at(340, 199), 'wait 2'] + click + shot('ix_head_click')
    c += [at(167, 127), 'wait 2'] + click + shot('ix_tab_advanced')
    c += [at(101, 236), 'wait 2'] + click + shot('ix_tab_weapconfig')
    # 33 frames of 8 ms move the cursor blink (UI time bit 8) to its other phase.
    c += [at(345, 177), 'wait 2'] + click + ['wait 2', f'pxkey {K_INS} 1', f'pxkey {K_INS} 0', 'pxchar 120',
                                            'pxchar 121', 'wait 2'] + shot('ix_edit_typing')
    c += ['wait 33', 'screenshotPNG ix_edit_typing2', 'wait 5']
    c += [f'pxkey {K_ENTER} 1', f'pxkey {K_ENTER} 0'] + shot('ix_edit_commit') + ['menu_close ingame_options']
    c += ['menu_open ingame_controls', 'wait 20', at(230, 160), 'wait 2'] + click + shot('ix_bind_waiting')
    c += ['pxkey 120 1', 'pxkey 120 0'] + shot('ix_bind_done') + ['menu_close ingame_controls']
    c += ['menu_open ingame_callvote', 'wait 20', at(330, 260), 'wait 2'] + click + shot('ix_list_click')
    c += [f'pxkey {K_DOWNARROW} 1', f'pxkey {K_DOWNARROW} 0'] + shot('ix_list_arrow')
    return c + ['menu_close ingame_callvote', 'quit', '']


def capture(label: str, client: Path, retail: Path, proxy: Path, target: Path, scratch: Path,
            width: int, height: int) -> Path:
    root = Path(tempfile.mkdtemp(prefix=f'interaction-{label}-', dir=scratch)).resolve()
    home = root / 'run'
    base = home / 'baseq3'
    base.mkdir(parents=True)
    shutil.copy2(proxy, base / 'uix86.dll')
    (base / 'probe.cfg').write_text('\n'.join(script(width, height)), encoding='ascii')
    settings = {
        'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(width), 'r_customheight': str(height),
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1', 'cl_webuiEnable': '0', 'web_browserActive': '0',
        'cl_allowDownload': '0', 'vm_gameProvider': 'retail', 'net_port': '28126', 'com_maxfps': '125',
    }
    command = [str(client)]
    for key, value in settings.items():
        command += ['+set', key, value]
    command += ['+exec', 'probe.cfg']
    startup = subprocess.STARTUPINFO()
    startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 4
    env = dict(os.environ, FNQL_UI_PROXY_TARGET=str(target))
    process = subprocess.Popen(command, cwd=home, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, startupinfo=startup)
    try:
        deadline = time.monotonic() + 180
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(.25)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
    shots = root / 'shots'
    shots.mkdir()
    captured = base / 'screenshots'
    if captured.is_dir():
        for png in captured.glob('*.png'):
            shutil.copy2(png, shots / png.name)
    return root


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--proxy', required=True, type=Path)
    parser.add_argument('--retail')
    parser.add_argument('--width', type=int, default=960)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'win32':
        print('The interaction gate hosts the retail Windows x86 UI')
        return 77
    try:
        import PIL  # noqa: F401
    except ImportError:
        print('Pillow is required for the interaction comparison')
        return 77
    client = args.client.resolve(strict=True)
    module = args.module.resolve(strict=True)
    proxy = args.proxy.resolve(strict=True)
    retail = find_retail_install(args.retail)
    scratch = ROOT / '.tmp/ui'
    scratch.mkdir(parents=True, exist_ok=True)
    unpack = Path(tempfile.mkdtemp(prefix='interaction-retail-ui-', dir=scratch)).resolve()
    try:
        with zipfile.ZipFile(retail / 'baseq3' / 'bin.pk3') as package:
            (unpack / 'uix86.dll').write_bytes(package.read('uix86.dll'))
        for attempt in range(2):
            roots = [capture('retail', client, retail, proxy, unpack / 'uix86.dll', scratch, args.width, args.height),
                     capture('fnql', client, retail, proxy, module, scratch, args.width, args.height)]
            result = compare(*roots)
            for name in CAPTURES:
                result.setdefault(f'{name}.png', None)
            clean = all(count == 0 for count in result.values())
            for root in roots:
                if args.keep or (not clean and attempt == 1):
                    print(f'Evidence: {root}')
                else:
                    remove(root, scratch)
            if clean:
                break
    finally:
        remove(unpack, scratch)
    if not clean:
        failures = [f'{name}: {"missing" if count is None else f"{count} px"}'
                    for name, count in sorted(result.items()) if count != 0]
        print(f'Interaction differs from retail at {args.width}x{args.height} (> {THRESHOLD} levels): '
              + ', '.join(failures))
        return 1
    print(f'{len(result)} interaction captures match retail at {args.width}x{args.height}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
