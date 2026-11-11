#!/usr/bin/env python3
"""Require scripted menu interaction to behave as the retail UI behaves.

The test-only input proxy (tests/ui_input_proxy.c) is installed as the probe
home's uix86.dll and loads the UI named by FNQL_UI_PROXY_TARGET: the retail
uix86.dll read from the install's baseq3/bin.pk3 into the probe's private
scratch, or FnQL-game's module. Its pxmouse, pxkey and pxchar console commands
feed both UIs the same frame-scheduled input through their own key and mouse
exports, and pxclock gives both the same synthetic UI time, so pulsing focus
colours and the edit-field cursor blink land on the same phase.

Scenarios, on a cheat-enabled local map with the world, entities and HUD
hidden, then out of game:

- hover a navigation button of the in-game menu (focus highlight);
- basic options: hover, then drag, the master-volume slider; click a player
  head; step the rail-style multi forward (left click) and back (right
  click); click, then drag, the crosshair-colour slider; scroll the model
  list with the mouse wheel;
- the ADVANCED tab's Game page: toggle a yes/no item, step a multi, move
  focus with the down arrow and step the newly focused item with Enter,
  apply a preset list, click a labelled slider's track;
- its WEAPON CONFIG page: type into an edit field in insert mode (both
  cursor-blink phases), edit with Home, Right, Delete, End and backspace,
  switch to overstrike with Insert (both phases) and commit with Enter;
- click a controls binding (waiting for a key), then bind a key;
- click the call-vote map list, then move its selection with an arrow key;
- disconnect and, with the web UI up, hover a main-menu button, click it and
  press Escape. Steam and the web UI run at their defaults, as in retail, so
  out of game the Awesomium web UI owns the screen and both UIs must yield
  to it (retail Menu_Paint 0x1001d82e) whatever input they receive; the gate
  requires web_browserActive 1 in both runs there, and 0 in game.

Every capture must match retail: a pixel counts as different only above the
parity gate's THRESHOLD. The menus' cvar writes must also land: the values of
WATCHED cvars, printed at the start and end of each run, must match between
the two UIs, and every CHANGED cvar must differ from its starting value, so a
refused write (an engine cvar policy, say) fails the gate even though both
UIs would then render the same unchanged state. Captures are engine
screenshots from the windowless client (com_headless 2), with mouse and
joystick input disabled, in private
unpure probe homes; installed files are never changed. A failing run is
retried once.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from smoke_dedicated import find_retail_install
from ui_menu_parity_probe import THRESHOLD, compare, remove

ROOT = Path(__file__).resolve().parents[1]
CAPTURES = ('ix_nav_hover', 'ix_slider_hover', 'ix_slider_drag', 'ix_head_click', 'ix_multi_next',
            'ix_multi_prev', 'ix_color_click', 'ix_color_drag', 'ix_list_wheel', 'ix_tab_advanced',
            'ix_yesno_click', 'ix_multi_click', 'ix_key_down', 'ix_key_enter', 'ix_presetlist',
            'ix_label_slider', 'ix_tab_weapconfig', 'ix_edit_typing', 'ix_edit_typing2', 'ix_edit_keys',
            'ix_edit_backspace', 'ix_edit_overstrike', 'ix_edit_overstrike2', 'ix_edit_commit',
            'ix_bind_waiting', 'ix_bind_done', 'ix_list_click', 'ix_list_arrow', 'ix_main_hover',
            'ix_main_options', 'ix_main_escape')
# Cvars the scenarios write; each must end equal in both UIs.
WATCHED = ('s_volume', 'cg_railStyle', 'cg_crosshairColor', 'cg_autoswitch', 'cg_kickScale', 'cg_bob',
           'cg_impactSparks', 'cg_impactSparksLifetime', 'ui_impactSparks', 'cg_impactSparksVelocity',
           'cg_zoomSensitivity', 'cg_weaponConfig')
# Of those, the writes that must change the starting value.
CHANGED = ('s_volume', 'cg_crosshairColor', 'cg_autoswitch', 'cg_kickScale', 'cg_impactSparks', 'cg_weaponConfig')
K_TAB, K_ENTER, K_ESCAPE, K_BACKSPACE = 9, 13, 27, 127
K_DOWNARROW, K_RIGHTARROW, K_INS, K_DEL, K_HOME, K_END = 133, 135, 139, 140, 143, 144
K_MOUSE1, K_MOUSE2, K_MWHEELDOWN = 178, 179, 183


def canvas(width: int, height: int) -> tuple[float, float, float]:
    """(x scale, y scale, x bias) of the retail 640x480 canvas (UI_ComputeCanvas)."""
    if width * 480 > height * 640:
        return height / 480, height / 480, (width - height * 4 / 3) / 2
    return width / 640, height / 480, 0.0


def script(width: int, height: int) -> list[str]:
    xs, ys, bias = canvas(width, height)
    at = lambda x, y: f'pxmouse {round(bias + xs * x)} {round(ys * y)}'
    press = lambda key: [f'pxkey {key} 1', f'pxkey {key} 0']
    click = lambda key=K_MOUSE1: [f'pxkey {key} 1', 'wait 2', f'pxkey {key} 0']
    shot = lambda name: ['wait 40', f'screenshotPNG {name}', 'wait 5']
    # 33 frames of 8 ms move the cursor blink (UI time bit 8) to its other phase.
    blink = lambda name: ['wait 33', f'screenshotPNG {name}', 'wait 5']
    snapshot = lambda label: [f'echo PXCV {label}'] + [f'cvarlist {name}' for name in WATCHED + ('web_browserActive',)]
    c = ['wait 40', 'devmap campgrounds ffa', 'wait 140', 'r_drawworld 0', 'r_drawentities 0', 'cg_draw2D 0',
         'wait 20', 'togglemenu', 'wait 20', 'pxclock 8'] + snapshot('start')
    c += [at(228, 72), 'wait 2'] + shot('ix_nav_hover')
    # Options, basic page (menu origin 39, 99).
    c += ['menu_close ingame_about', 'menu_open ingame_options', 'wait 20', at(241, 177), 'wait 2']
    c += shot('ix_slider_hover')
    c += [at(200, 177), 'wait 2', f'pxkey {K_MOUSE1} 1', 'wait 2', at(260, 177), 'wait 4', at(275, 177), 'wait 4',
          f'pxkey {K_MOUSE1} 0'] + shot('ix_slider_drag')
    c += [at(340, 199), 'wait 2'] + click() + shot('ix_head_click')
    c += [at(462, 326), 'wait 2'] + click() + shot('ix_multi_next')
    c += click(K_MOUSE2) + shot('ix_multi_prev')
    c += [at(477, 286), 'wait 2'] + click() + shot('ix_color_click')
    c += [f'pxkey {K_MOUSE1} 1', 'wait 2', at(490, 286), 'wait 4', at(500, 286), 'wait 4', f'pxkey {K_MOUSE1} 0']
    c += shot('ix_color_drag')
    c += [at(420, 199), 'wait 2'] + press(K_MWHEELDOWN) + shot('ix_list_wheel')
    # ADVANCED tab, Game page.
    c += [at(167, 127), 'wait 2'] + click() + shot('ix_tab_advanced')
    c += [at(300, 186), 'wait 2'] + click() + shot('ix_yesno_click')
    c += [at(300, 156), 'wait 2'] + click() + shot('ix_multi_click')
    c += press(K_DOWNARROW) + shot('ix_key_down')
    c += press(K_ENTER) + shot('ix_key_enter')
    c += [at(300, 261), 'wait 2'] + click() + shot('ix_presetlist')
    c += [at(440, 206), 'wait 2'] + click() + shot('ix_label_slider')
    # WEAPON CONFIG page: editing starts in insert mode.
    c += [at(101, 236), 'wait 2'] + click() + shot('ix_tab_weapconfig')
    c += [at(345, 177), 'wait 2'] + click() + ['wait 2']
    for character in 'cg_fov 90':
        c += [f'pxchar {ord(character)}', 'wait 1']
    c += shot('ix_edit_typing') + blink('ix_edit_typing2')
    c += press(K_HOME) + press(K_RIGHTARROW) + press(K_RIGHTARROW) + press(K_DEL) + shot('ix_edit_keys')
    c += press(K_END) + press(K_BACKSPACE) + ['pxchar 8', 'wait 1'] + shot('ix_edit_backspace')
    c += press(K_INS) + press(K_HOME) + ['pxchar 88', 'wait 1'] + shot('ix_edit_overstrike')
    c += blink('ix_edit_overstrike2')
    c += press(K_ENTER) + shot('ix_edit_commit') + ['menu_close ingame_options']
    c += ['menu_open ingame_controls', 'wait 20', at(230, 160), 'wait 2'] + click() + shot('ix_bind_waiting')
    c += ['pxkey 120 1', 'pxkey 120 0'] + shot('ix_bind_done') + ['menu_close ingame_controls']
    c += ['menu_open ingame_callvote', 'wait 20', at(330, 260), 'wait 2'] + click() + shot('ix_list_click')
    c += press(K_DOWNARROW) + shot('ix_list_arrow') + ['menu_close ingame_callvote'] + snapshot('end')
    # Out of game: the main menu.
    c += ['disconnect', 'wait 200', 'echo PXCV web', 'cvarlist web_browserActive', at(100, 335), 'wait 2']
    c += shot('ix_main_hover')
    c += click() + ['wait 20'] + shot('ix_main_options')
    c += press(K_ESCAPE) + shot('ix_main_escape')
    return c + ['quit', '']


def capture(label: str, client: Path, retail: Path, proxy: Path, target: Path, scratch: Path,
            width: int, height: int) -> Path:
    root = Path(tempfile.mkdtemp(prefix=f'interaction-{label}-', dir=scratch)).resolve()
    home = root / 'run'
    base = home / 'baseq3'
    base.mkdir(parents=True)
    shutil.copy2(proxy, base / 'uix86.dll')
    (base / 'probe.cfg').write_text('\n'.join(script(width, height)), encoding='ascii')
    settings = {
        'com_headless': '2', 'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(width), 'r_customheight': str(height),
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1',
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
        deadline = time.monotonic() + 240
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


def cvar_snapshots(root: Path) -> dict[str, dict[str, str]]:
    """{label: {lower-case cvar name: value}} from the run's console log."""
    log = root / 'run' / 'baseq3' / 'qconsole.log'
    snapshots: dict[str, dict[str, str]] = {}
    current = None
    watched = {name.lower() for name in WATCHED + ('web_browserActive',)}
    if not log.is_file():
        return snapshots
    for line in log.read_text(encoding='latin-1').splitlines():
        marker = re.match(r'PXCV (\w+)\s*$', line)
        if marker:
            current = snapshots.setdefault(marker.group(1), {})
            continue
        row = re.match(r'\s.*?\s(\S+) "(.*)"\s*$', line)
        if current is not None and row and row.group(1).lower() in watched:
            current[row.group(1).lower()] = row.group(2)
    return snapshots


def cvar_failures(retail_root: Path, fnql_root: Path) -> list[str]:
    retail, fnql = cvar_snapshots(retail_root), cvar_snapshots(fnql_root)
    failures = []
    for which, snapshot in (('retail', retail), ('FnQL-game', fnql)):
        for label, expected in (('start', '0'), ('end', '0'), ('web', '1')):
            value = snapshot.get(label, {}).get('web_browseractive')
            if value != expected:
                failures.append(f'{which} web_browserActive at {label}: {value or "missing"}, expected {expected}')
    for label in ('start', 'end'):
        for name in WATCHED:
            key = name.lower()
            values = [snapshot.get(label, {}).get(key) for snapshot in (retail, fnql)]
            if None in values:
                failures.append(f'{name} missing at {label}')
            elif values[0] != values[1]:
                failures.append(f'{name} at {label}: retail "{values[0]}", FnQL-game "{values[1]}"')
    for which, snapshot in (('retail', retail), ('FnQL-game', fnql)):
        for name in CHANGED:
            key = name.lower()
            start, end = snapshot.get('start', {}).get(key), snapshot.get('end', {}).get(key)
            if start is not None and start == end:
                failures.append(f'{which} UI write to {name} did not land (still "{end}")')
    return failures


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
            failures = [f'{name}: {"missing" if count is None else f"{count} px"}'
                        for name, count in sorted(result.items()) if count != 0]
            failures += cvar_failures(*roots)
            for root in roots:
                if args.keep or (failures and attempt == 1):
                    print(f'Evidence: {root}')
                else:
                    remove(root, scratch)
            if not failures:
                break
    finally:
        remove(unpack, scratch)
    if failures:
        print(f'Interaction differs from retail at {args.width}x{args.height} (> {THRESHOLD} levels): '
              + ', '.join(failures))
        return 1
    print(f'{len(result)} interaction captures and {len(WATCHED)} cvar results match retail at '
          f'{args.width}x{args.height}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
