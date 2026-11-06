#!/usr/bin/env python3
"""Compare the UI player-model previews of the installed retail UI and FnQL-game.

Each UI renders one private test menu holding the five model ownerdraws
(team, enemy, red, blue and player previews) over a flat background. The test
menu exists only in a private, unpure probe home and is added through a
homepath ui/ingame.txt; installed files are never changed. Captures are engine
screenshots, windowed, with mouse and joystick input disabled.

For every preview the silhouette bounding box and mean colour must agree with
the retail capture. Idle animation phase differs between runs, so the check
uses tolerances measured from retail-against-retail captures rather than
pixel identity.
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
# Ownerdraw ids from the installed ui/menudef.h.
PREVIEWS = (('team', 553, 10, 40), ('enemy', 554, 170, 40), ('red', 555, 330, 40),
            ('blue', 556, 490, 40), ('player', 514, 10, 250))
SIZE = (140, 200)
BACKGROUND = (0.2, 0.2, 0.3)
# Retail-against-retail captures differ by at most 2 px in bounds and under
# 1 level in mean colour; FnQL-game's earlier baseline camera missed by tens.
BOUNDS_TOLERANCE = 4
MEAN_TOLERANCE = 2.0
MODELS = {'cg_forceTeamModel': 'keel/bright', 'cg_forceTeamSkin': '',
          'cg_forceEnemyModel': 'tankjr/sport', 'cg_forceEnemySkin': '',
          'cg_forceRedTeamModel': 'orbb', 'cg_forceBlueTeamModel': 'ranger',
          'model': 'visor', 'headmodel': 'visor', 'cg_loadout': '0'}


def capture(client: Path, retail: Path, module: Path | None, scratch: Path, keep: bool) -> tuple[Path, Path]:
    home = Path(tempfile.mkdtemp(prefix='models-' + ('fnql-' if module else 'retail-'), dir=scratch)).resolve()
    base = home / 'baseq3'
    (base / 'ui').mkdir(parents=True)
    if module:
        shutil.copy2(module, base / 'uix86.dll')
    (base / 'ui' / 'ingame.txt').write_text(
        '{\n loadMenu { "ui/ingame.menu" }\n loadMenu { "ui/ingame_about.menu" }\n'
        ' loadMenu { "ui/fnqlmodels.menu" }\n}\n', encoding='ascii')
    items = ''.join(
        f'  itemDef {{ name {name} rect {x} {y} {SIZE[0]} {SIZE[1]} visible 1 decoration ownerdraw {owner} }}\n'
        for name, owner, x, y in PREVIEWS)
    (base / 'ui' / 'fnqlmodels.menu').write_text(
        '{\n menuDef {\n  name fnqlmodels\n  fullscreen 1\n  rect 0 0 640 480\n  visible 0\n'
        '  itemDef { name bg rect 0 0 640 480 style 1 backcolor %g %g %g 1 visible 1 decoration }\n'
        % BACKGROUND + items + ' }\n}\n', encoding='ascii')
    commands = ['wait 40', 'map campgrounds ffa', 'wait 140', 'togglemenu', 'wait 10',
                'menu_close ingame_about', 'menu_close ingame', 'menu_open fnqlmodels', 'wait 60',
                'screenshotPNG models', 'wait 5', 'echo model-probe-done', 'quit', '']
    (base / 'probe.cfg').write_text('\n'.join(commands), encoding='ascii')
    settings = {
        'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': '960', 'r_customheight': '600',
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1', 'cl_webuiEnable': '0', 'web_browserActive': '0',
        'cl_allowDownload': '0', 'vm_gameProvider': 'retail', 'net_port': '28119',
        'com_maxfps': '125', **MODELS,
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
    process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, startupinfo=startup)
    try:
        deadline = time.monotonic() + 90
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(.25)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
    return home, base / 'screenshots' / 'models.png'


def measure(image, name: str, x: int, y: int) -> tuple[tuple[int, int, int, int] | None, tuple[float, ...]]:
    from PIL import ImageChops, ImageStat
    scale = image.height / 480.0
    bias = (image.width - 640 * scale) / 2
    box = (round(bias + x * scale), round(y * scale), round(bias + (x + SIZE[0]) * scale),
           round((y + SIZE[1]) * scale))
    crop = image.crop(box)
    flat = tuple(round(channel * 255) for channel in BACKGROUND)
    ground = crop.copy()
    ground.paste(flat, (0, 0) + crop.size)
    mask = ImageChops.difference(crop, ground).convert('L').point(lambda value: 255 if value > 24 else 0)
    return mask.getbbox(), tuple(ImageStat.Stat(crop).mean)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--retail')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    try:
        from PIL import Image
    except ImportError:
        print('Pillow is required for the model preview comparison')
        return 77
    client = args.client.resolve(strict=True)
    module = args.module.resolve(strict=True)
    retail = find_retail_install(args.retail)
    scratch = ROOT / '.tmp/ui'
    scratch.mkdir(parents=True, exist_ok=True)
    homes = []
    try:
        shots = {}
        for label, dll in (('retail', None), ('fnql', module)):
            home, shot = capture(client, retail, dll, scratch, args.keep)
            homes.append(home)
            if not shot.is_file():
                print(f'Model preview capture failed for {label}: {home}')
                return 1
            shots[label] = Image.open(shot).convert('RGB')
        failures = []
        for name, _, x, y in PREVIEWS:
            expected = measure(shots['retail'], name, x, y)
            actual = measure(shots['fnql'], name, x, y)
            if expected[0] is None or actual[0] is None:
                failures.append(f'{name}: retail {expected[0]}, FnQL-game {actual[0]}')
                continue
            bounds = max(abs(a - b) for a, b in zip(expected[0], actual[0]))
            mean = max(abs(a - b) for a, b in zip(expected[1], actual[1]))
            if bounds > BOUNDS_TOLERANCE or mean > MEAN_TOLERANCE:
                failures.append(f'{name}: bounds {expected[0]} vs {actual[0]}, mean '
                                f'{[round(v, 1) for v in expected[1]]} vs {[round(v, 1) for v in actual[1]]}')
        if failures:
            print('Model previews differ from retail:\n  ' + '\n  '.join(failures))
            return 1
        print('Model previews match retail: ' + ', '.join(name for name, *_ in PREVIEWS))
        return 0
    finally:
        for home in homes:
            if args.keep:
                print(f'Evidence: {home}')
            elif home.parent == scratch.resolve() and not linked(home) and not any(linked(p) for p in home.rglob('*')):
                shutil.rmtree(home)
            else:
                print(f'Preserved unexpected probe output: {home}')


if __name__ == '__main__':
    sys.exit(main())
