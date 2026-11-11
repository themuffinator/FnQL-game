# Native UI

`code/ui/` is the sole UI implementation. The redundant `code/q3_ui/` tree
and the old UI vmMain/syscall entrypoints are removed. Meson builds
`fnql_uix86.dll` on Windows x86 and `fnql_uii386.so` on Linux i386; the Windows
build is validated locally. The module uses installed retail assets and
contains no bundled retail menu files, fonts, models or textures.

## Implemented behavior

- The retail API-8 native boundary: 97 imports, 14 exports, typed calls and
  module identity without an unproved retail-wire capability claim.
- Existing retail main, in-game, callvote, administration, join, controls,
  options and demo menus. Launcher navigation remains owned by the
  engine-hosted WebUI; this module does not add replacement screens.
- Native scalable text, font selection, cursor measurement, display refresh,
  connect/download text, arena enumeration and advertisement-wait exports.
  Live advertising and unavailable online services are not activated.
- Retail 4:3 canvas scaling, widescreen centering, full-screen background
  cropping, font point sizes and shadows. Model lists retain the order in
  `ui/teaminfo.txt`, validate lower skins and exclude color aliases from the
  basic player selector. Slider gradients use the installed `fx_base` asset.
- QL secondary cvar conditions, integer/precision sliders, color sliders,
  declarative presets and preset lists. The options menu's 773 widgets fit
  in the bounded 1,024-item capacity; scripts are bounded at 8 KiB and reject
  overflow instead of silently truncating actions.
  `cvara` registers archived values; preset names persist until a manual edit
  invalidates their values, when the selector becomes `Custom`.
- Thirteen QL game types, bounded arena filtering and map voting; sparse
  player-list rows map back to real client numbers for voting and admin
  commands. Invalid rows and unsafe command arguments are rejected.
  Bot selection uses the installed bot definitions in every game type;
  adding a bot and closing the in-game menu releases the UI and pause state.
- QL bindings including HMG, weapon toggle, ready-up, drops, accuracy, chat
  and voice; saved bindings preserve unrelated keys. Defaults use the
  intended key fields rather than the baseline's unused ID column.
  Clearing a binding removes both assigned keys; closing menus cancels pending
  rebinding, text editing and mouse capture. Quoted empty script values and
  quoted braces survive menu parsing.
- Numbered QL crosshairs, model/skin icons, forced-model previews and color
  updates, server settings, starting weapons, next-map and vote labels.
  Visibility uses the same cgame-published cvars as retail. Demo filenames retain
  case and spaces; empty selections cannot dispatch invalid demo commands.
- The retail cvar table, including its archived proxies (`ui_team*Color`
  default 96, `ui_enemy*Color` 27, `ui_screenDamage*`, `ui_bloomPreset`,
  `cg_enemyCrosshairNames`), the temporary forced-model selectors, and the
  read-only `ui_force*ModelBright`, `ui_priv`, `ui_mainmenu` and `ui_gibs`.
  `cg_drawCrosshair` and `g_warmup` use retail's defaults (2 and 10), and
  `ui_smallFont`/`ui_bigFont` its 0x801 flags. Change callbacks follow retail:
  they run only when a later update changes a registered cvar, so start-up
  never rewrites a `cg_*` color. Forced-model scripts derive only the Bright
  flag, so preset names such as "Keel Bright" persist. Changing the announcer
  previews its voice. Team Arena's first-run volume cvar is gone.
- Item grammar: retail's `cvarTest3/4`, `showCvar3/4`, `hideCvar3/4` and
  `outlineimage`; script verbs `toggle` (retail's per-menu toggle flag) and an
  inert `activateAdvert`; menu scripts `clearComError`, `setFullScreen`,
  `setWindowed`, `toggleFullscreen` (`vid_restart fast`), `playerModelChanged`
  and `openWebGameSettings`.
- The binding table in retail order with retail defaults (WASD, mouse attack
  and zoom, F3 ready-up, arrow-key drops, keypad `follow 0..9`, and the
  `forfeit`, `droprune`, `screenshot`, `togglechathistory` and `+pstats`
  rows). `+strafe`, `+mlook`, `+button4` and `messagemode3/4` stay as
  extensions; the installed controls menu binds the last two.
- Menu activation as retail: an opened menu owns input (`KEYCATCH_UI`), the
  main menu plays `music/fla_mp05` and always shows a pending error, the
  in-game menu resyncs `ui_mousePitch`, and NEED_CD, BAD_CD_KEY and POSTGAME
  do nothing. `Leave` keeps the UI on the main menu; `closeingame` also
  clears held keys.
- Mouse input is absolute, as the engine delivers it: drawable pixels map
  onto the centered 4:3 canvas, and focus moves only inside 640x480.
- The connect screen reads download progress through import 96 for the
  `cl_downloadItem` id, with 64-bit sizes, so workshop downloads show
  progress; the classic download cvars remain the fallback. Init sets
  `ui_version` to the FnQL-game version and `ui_gibs` from import 93
  (Steam app 343500). Export 9 reports a unique CD key, as retail does.
- Menu parsing and arena loading are silent unless `ui_debug` is set.
- Player-model previews as retail: the camera frames 1.1 times the 56-unit
  box; every model but orbb is scaled to that idle height; tints are doubled
  unless the engine reports `r_colorCorrectActive`. Team and enemy previews
  use `cg_force*Model` (a skin alone selects sarge), red and blue use
  `cg_forceRed/BlueTeamModel` with the team colors, and an unset model draws
  nothing. The player preview uses `model`/`headmodel`, pose 5/210 and the
  machinegun, or under loadouts the queued loadout primary (heavy machinegun
  otherwise).
- No cursor sprite is drawn: the engine shows the system pointer while a
  menu owns input, as retail's menus expect. `Controls` takes input with
  `KEYCATCH_UI` alone, and game types read "Attack and Defend". Console
  command `listPlayerModels` lists the selectable models.
- The 4:3 canvas uses retail's exact arithmetic. A wide display centres it
  with the float constants 4/3 and 1/480, so the 960x600 bias is
  79.99999, not 80, and truncated pixel edges fall where retail's do. Other
  displays stretch with 1/640 and 1/480. Text point size is the item scale
  times vidHeight / 768 * 96.
- List rows paint their background before their text, as retail does: the
  selected row's highlight (or `outlineimage`) and the odd-row fill span
  (x + 2, y + 6, width - 22, element height), so selected text is no longer
  dimmed by the highlight drawn over it.
- A cvar caption with no text that is aligned CENTER or RIGHT is not
  painted. Retail measures its centring width from the missing text and
  gets its display-context pointer back from an uninitialised local, so such
  a caption lands far off-screen; the controls pages' slider-value captions
  are never visible in retail. LEFT captions and autowrapped items are
  unaffected.
- Autowrapped text advances 1.3 line heights per line (`\r`-wrapped text
  keeps height + 5), and the connect screen ends every connecting,
  challenging or awaiting-gamestate frame with "Press ESC to cancel" at
  (320, 440).
- The UI no longer registers `cg_drawCrosshairNames` (default 1). Retail's
  UI and cgame register no such cvar, so the HUD page's "Target Names"
  shows "No" as in retail.
- Sliders store values as retail does: a click on the track always writes
  `%f`, and a thumb drag writes a rounded-up `%d` for `cvarInt` items, `%.Nf`
  for an item with a `precision`, and `%f` otherwise. A slider's precision
  defaults to 0, not 3.
- Binding a key clears only the waiting state: the bound item keeps the red
  focus tint while it stays focused. Starting to edit a field, by click or
  Enter, selects insert mode, so the cursor is `|`.
- The edit-field cursor is painted as retail paints it: the text is drawn
  once, unshadowed, and while bit 8 of the UI time is set the cursor follows
  at the measured width of the text before it. A space just before the
  cursor is measured as `_`, a cursor past the end of the text keeps its
  raw position (the engine measures that range as 2 px wide), and the
  insert-mode `|` is pulled left by 0.8 of its own width.
- Applying a preset stores a value made only of digits and at most one `.`
  as `%f` of its parsed number, and any other value as written. The menus
  then test the stored text: after the "No" impact-sparks preset,
  `cg_impactSparks` is `0.000000`, which a test for `0` does not match, so
  "Sparks Velocity" stays enabled, as it does in retail.
- Menus use retail's grammar and script set only. The menu-script
  dispatcher has no command retail's lacks: `updateSPMenu`, `resetScores`,
  `nextSkirmish`, `SkirmishStart`, `voteLeader`, `glCustom` and
  `setPbClStatus` are gone, as are the `r_lodbias` and `ui_glCustom` cases
  of `uiScript update` (the latter forced `r_fullScreen 1`). The installed
  menus still call `uiScript glCustom`; it now prints "unknown UI script
  glCustom" as retail's does (0x1000d2c0). Items have no `enableCvar2` or
  `disableCvar2` (neither retail uix86 nor cgamex86 has them), and menus
  accept `ownerdrawFlag2`, which retail's menu table maps to the item handler
  0x1001f5b0. Menu_Paint tests both owner-draw flag words before yielding to
  the web UI, in retail's order (0x1001d7fb, 0x1001d82e).

## Evidence and scope

Observed ABI facts come from retail `uix86.dll`'s `dllEntry` at `0x10003970`;
the refresh-display and arena exports at `0x10003920`/`0x10003930`; init at
`0x1000fab0`; active-menu dispatch at `0x100100d0`; and advertisement wait at
`0x10011130`. The QLSRP symbol map and retail HLIL corpus locate these
observations; no QLSRP implementation text was incorporated.

The retail menu-script dispatcher at `0x1000b0e0`, map vote at `0x1000c35b`,
bot definition selection and command at `0x1000c4ba`/`0x1000c4c9`,
and player actions at `0x1000cd91` through `0x1000d300` establish command
names and arguments. Slot 20 receives one text argument. FnQL's optimized
x86 fixed-arity adapter could overwrite the caller's return address when
tail-calling a two-argument function; its variadic replacement preserves
both the retail lane and the older source adapter.

The slider callback at `0x10011240` and palette at `0x1002bb88` establish
the packed color representation and interpolation. Bounds checks around
out-of-range values are a deliberate robustness improvement. The checked
retail `menudef.h` supplies widget, ownerdraw and feeder numbers. Installed
`ui/menus.txt`, `ui/ingame.txt`, options includes and `ui/teaminfo.txt`
establish menu grammar, controls and model/skin pairs. Numeric crosshair
paths were verified in the installed pk3 files.

The menu files are authoritative for layout and actions; there are no injected
buttons or custom replacement menus. Runtime comparisons use the same engine
and installed assets with either the retail DLL or FnQL-game's DLL. The main
menu captures are pixel-identical at 640x480 and 960x600. In-game panels were
compared visually; moving backgrounds and focus animation prevent a blanket
pixel-identity claim for those captures.

Specific rendering observations include `Text_GetDimensions`/`Text_Paint`
at `0x10003d90`/`0x10003ec0` (host bounds and height/8 font points), display
refresh at `0x1000f9f0` (centered canvas), `Item_SliderColor_Paint` at
`0x1001b5c0` (uncolored gradient), character parsing and filtering at
`0x1000f140`/`0x1000d490`/`0x1000d530`, server-settings rows at `0x10007030`,
and starting-weapon bit positions/spacing at `0x10008730`. Native UI leaves
the optional widescreen-mode callback unset in retail init; mode metadata
is parsed without introducing cgame's separate anchoring behavior into UI.

The 2026-11-05 gap pass compared the installed menus and the retail DLL's
string, keyword, script and cvar tables with this module. Observed sources:
cvar table 0x1002afd8 (130 rows of cvar, name, default, callback and flags)
with registration 0x10011730 and updates 0x100118a0; callbacks 0x10011240
(slider colors), 0x10011510/0x10011630/0x10011660 (Bright flags) and
0x10011690 (announcer); binding table 0x1002a0d0 (81 rows of 0x18 bytes);
item keyword table around 0x1002a868; `toggle` 0x10016580; script dispatch
0x1000c059..0x1000cd31; menu activation 0x100100d0; mouse export 0x10010000;
export 9 0x10003910; init tail 0x1000fe9d..0x1000ff33; download panel
0x100108d0. Inferred, not observed: retail's toggle flag sits at menu offset
0x4c; this module keeps an equivalent private field. Deliberate differences:
retail stores 0 and -2 as default keys for `weapon 14` and `forfeit`, which
name no key, so they are stored as no default; sizes above 4 GiB format
correctly; UIMENU_TEAM falls back to `ingame_join` because the installed
menus define no `joingame_menu`; ESC still releases input at once. Retail
calls import 85 when closing menus; FnQL implements it as a no-op, so it is
not called. The advert imports stay unused while advertising is off.

A second pass rendered the model ownerdraws with both UIs and read the
retail code behind the differences: previews 0x10005690 (player),
0x10005850/0x10005c20 (team/enemy) and 0x10005ff0/0x100062a0 (red/blue);
`UI_DrawPlayer` 0x10012d90 (camera constant 61.6 at 0x10029488, height
constants 24/56/1.1, tint scale test 0x10012e0d, orbb test 0x100142c3);
`animation_t` is 24 bytes, so the height uses the first LEGS_IDLE and
TORSO_STAND frames; the loadout-primary flag in the weapon table at
0x1002c1c4; game type names 0x1002c0f4; refresh 0x10004390, which draws no
cursor; `Controls` 0x1000bff6; console commands 0x10002ac0. The `qz*` map
alias table at 0x1002ba00 has no code references and is not reproduced.
Retail's 125-frame FPS average and its server-browser column labels are
not reproduced; the installed menus use neither.

A third pass compared every text draw and measurement by wrapping each UI
in an investigation proxy that logged imports 94 and 95; both UIs then
issued the same 72 distinct draws. Observed sources: display refresh
0x1000f9f0 (canvas constants 0x10029280, 0x10029270, 0x10029250,
0x10029248), Text_Paint 0x10003ec0, Item_SetTextExtents 0x10019f10,
Item_Text_Paint 0x1001a7e0, Menu_UpdatePosition 0x100159a0 and the
text-list loop in Item_ListBox_Paint 0x1001bfb0. Retail also measures some
labels with font 0 that it then draws with their item font; those
measurements do not affect any drawn position and are not reproduced.

A fourth pass rendered every tab page and the out-of-game screens. Observed
sources: Item_Paint 0x1001ced0 (TEXT and BUTTON items paint text unless
they are ownerdrawn), the `cvar` keyword at item+0x158 (0x1001ed80),
Item_SetTextExtents 0x10019f73 with textWidth 0x10003e60 (the NULL-text
measure returns the caller's saved ECX, the display context), the
autowrapped painter 0x1001a3d0 (advance constant 1.3 at 0x10029508), the
`\r`-wrapped painter 0x1001a630 (height + 5), the connect screen
0x10010e30..0x1001112a (overlay mode returns at once) and PC_Script_Parse
0x10014cf0 with Item_RunScript 0x10016d70 (2048-byte buffers, silent
truncation). Deliberate difference: this module keeps its 8 KiB
reject-on-overflow script bound instead of truncating a script mid-command;
no installed script approaches either limit once tokenized.

A fifth pass drove both UIs with identical scripted input through the
test-only input proxy and compared hover, slider, list, tab, edit-field and
binding states. Observed sources: Scroll_Slider_ThumbFunc
0x10018e60..0x10018f8f (drag formatting), Item_Slider_HandleKey
0x10019180..0x10019315 (click formatting), item initialisation 0x1002001a
(precision 0), Item_Bind_HandleKey 0x1001b9e0..0x1001bbbe (only the
waiting flag at 0x106b40d4 is cleared), Menu_HandleKey 0x10019dd5 and
0x10019e26 (insert mode on edit start), Item_TextField_Paint 0x1001a8e0
(cursor x = text rect x + width + 8, raw cursor position) and
Text_PaintWithCursor 0x10004070..0x10004271 (blink bit 0x100 at
0x10004162, `_` substitution at 0x10004182, insert shift constant 0.8 at
0x10029388). An investigation proxy that logged imports 94 and 95 showed the
cursor-range measurement retail makes past the end of an empty field.

A sixth pass also checked what the interactions store. Observed: FnQL's
engine refused every UI write to a cvar registered with flag 0x800 ("Restricted
source tried to set"), with both UIs alike. That covered nearly every option
the in-game menus edit, from `cg_railStyle` to the weapon-config fields, so
those items were inert and the pixel gates could not tell. The engine fix is
FnQL's `Cvar_SetFromUI`. With writes landing, one UI difference remained: the
preset list stored numeric values as written, where retail's preset apply
(0x100187b2..0x1001881d, numeric test 0x10001670, format `%f` at 0x100290e8)
stores them as `%f`. Retail's multi writer (0x1001847d..0x100184f4, `%i` at
0x10025920 for whole values, else `%f`) already matched.

Client provider
selection and honest pure-server packaging remain the separate M10 engine
integration contract. Do not replace the installed retail DLL or advertise
this development module as a retail-authenticated pure package.

## Gates

From the x86 developer environment:

```text
meson test -C build/win32-x86 --suite ui --suite ui-hosting --print-errorlogs
```

Configure `-Dfnql-client=<x86 fnql.exe>` and `-Dretail-ql=<Steam install>` to
enable hosting tests. `tests/ui_native.c` links production UI code and tests
the ABI, native text, secondary visibility, presets, colors, binding defaults,
map filtering/previews, bot cycling, binding edits, command bounds and sparse
player selections without OS input. It also checks font bounds/point sizes,
background cropping, archive registration, server-setting rows, and weapon
icon bit positions and spacing.

Every UI probe launches the windowless client (`+set com_headless 2`): it
renders and screenshots normally but never shows a window, takes focus,
enters fullscreen or touches desktop gamma. Steam and the web UI run at their
defaults (on), as retail does; no probe switches them off.

`scripts/ui_probe.py` runs with mouse/joystick handling disabled,
using a private unpure home and an independent DLL copy. It validates module
identity, active gameplay, all requested menus, advanced options actions,
an added bot in server status, normal shutdown and ten engine-generated
screenshots. Both retail and FnQL-game qagame are exercised.
`--keep` retains evidence under `.tmp/ui/`;
otherwise the verified private directory is removed.

`scripts/ui_legacy_probe.py` opens menus through retail `menu_open`,
`menu_close` and engine `togglemenu`, including `ingame_about`, at 4:3 and
widescreen resolutions. This exercises ordinary menu activation and the
server-settings ownerdraw that the earlier diagnostic shortcut missed.
Omit `--module` to capture the installed retail UI for comparison. Both
paths disable input and use engine screenshots only.

`scripts/ui_model_probe.py` (`fnql-ui-model-previews`) renders one private
test menu with the five model ownerdraws in each UI, through a homepath
`ui/ingame.txt` in an unpure probe home, and requires every preview's
silhouette bounds (4 px) and mean colour (2 levels) to match retail.
Retail-against-retail runs differ by at most 2 px and 1 level; the earlier
baseline camera missed by 31-47 px.

`scripts/ui_menu_parity_probe.py` (`fnql-ui-menu-parity-960`, `-1024`)
compares three capture sets in both UIs and requires no pixel to differ by
more than 16 levels; animated menu shaders vary by at most 4 levels between
two retail runs. `ingame` opens the ten installed in-game menus on a
cheat-enabled local map with the world, entities and HUD hidden. `tabs`
reaches all 18 tab pages of `ingame_options` (including each advanced
sub-page) and `ingame_controls` through private variant copies whose
`onOpen` runs the tab button's own installed action; three variants load per
run through a homepath `ui/ingame.txt`, which keeps the copies inside the UI
memory pool and each script inside retail's 2048-byte buffer. `outgame`
covers `main`, `main_options`, `quit`, `demo`, `error_popmenu` with a
pending error and the connect screen. Out of game the Awesomium web UI owns
the screen, as in retail, and uix86 yields to it (retail Menu_Paint
0x1001d82e), so those captures show the web UI in both runs; each first
requires `web_browserActive 1` in both, so a web UI that failed to start
cannot pass as parity. The connect screen is uix86's even then
(`web_browserActive 0`). Both 960x600 and 1024x768 run all three sets. A
set whose retail capture misses its screens is retried once.

`scripts/ui_interaction_probe.py` (`fnql-ui-interaction-960`, `-1024`)
installs the test-only input proxy (`tests/ui_input_proxy.c`, built as
`ui_input_proxy.dll` and never installed) as the probe home's `uix86.dll`.
The proxy loads the UI named by `FNQL_UI_PROXY_TARGET`: the retail
`uix86.dll` read from `baseq3/bin.pk3` into the probe's private scratch, or
FnQL-game's module. Its `pxmouse`, `pxkey` and `pxchar` console commands
reach the UI's own key and mouse exports, and `pxclock 8` gives both UIs the
same synthetic time, so focus pulses and the cursor blink match. Once a
probe command has run, the proxy drops the engine's own key and mouse
events, so a real pointer crossing the probe window cannot move the UI's
focus. The gate covers:

- in-game menu: hovering a navigation button;
- basic options: hovering and dragging the volume slider, clicking a player
  head, stepping a multi with left and right clicks, clicking and dragging a
  colour slider, and scrolling the model list with the wheel;
- the Advanced Game page: a yes/no, a multi, down-arrow focus and Enter, a
  preset list, and a labelled slider;
- the weapon-config page: typing in insert mode (both blink phases), Home,
  Right, Delete, End and backspace edits, overstrike via Insert (both
  phases), and committing with Enter;
- controls: waiting for and binding a key;
- call vote: clicking and arrowing through the map list;
- out of game, with the web UI up: hovering a main-menu button, clicking
  it and pressing Escape, which must leave the web UI on screen in both UIs
  (`web_browserActive` is required to be 1 there and 0 in game).

All 31 captures must match retail under the same 16-level threshold. The
values of 12 cvars the scenarios write, printed at the start and end of each
run, must match retail's. Six of them must differ from their starting value,
so a refused write fails the gate even though both UIs would then render the
same unchanged state. A failing run is retried once. The gate needs an FnQL
engine whose UI cvar imports use `Cvar_SetFromUI` (FnQL
`docs/fnql/GAME_MODULES.md`); an older engine refuses the protected writes and
fails it.

`ui_openmenu <menu>` is a diagnostic console command. With `developer 1`,
`ui_menuaction <menu> <item>` invokes an existing visible menu's action
directly for these probes; it does not synthesize keyboard or mouse input.
An optional third argument names a continuation config to queue after the
action's commands, so the probe verifies their effects before proceeding.
The engine's `ui_native_bridge_source_tests.py` guards the command adapter,
and its x86 runtime hosting test covers the formerly crashing controls path.

Run the earlier ABI, contract, movement and server gates with `meson test`.
Distribution also remains subject to the preexisting [baseline licensing
question](LICENSING.md).

Gap-pass validation on 2026-11-05: the x86 DLL and `ui-native` built; all
five UI gates and all 35 `unit`, `abi` and `contract` tests passed. The unit
test now covers the slot 3/4 gates, `outlineimage`, `toggle`, binding order
and defaults, callback timing, Bright derivation, the display scripts,
`Leave`, the announcer preview, imports 93/96, export 9 and absolute mouse
mapping. After the second pass all six UI gates (with the model-preview
gate) and the 35 `unit`, `abi` and `contract` tests passed. A kept 960x600
capture (`.tmp/ui/legacy-fnql-k7rrlyum/`) has a main menu identical to
retail and no UI cvar-default warnings. In-game captures differ from a
fresh retail run (`legacy-retail-d1btpvt7`) by 2k-20k pixels, the same
range as two retail runs differ from each other.

Third pass: with the world hidden, the ten in-game menus were pixel-identical
to retail (no pixel over 16 levels; the residual is under 4 levels, as
between retail runs) at 960x600, 1280x720 and 1024x768. Before it, six
menus differed by 197 to 3,589 pixels. All 43 tests in the `ui`,
`ui-hosting`, `unit`, `abi` and `contract` suites passed.

Fourth pass: all 18 tab pages and the six out-of-game screens are
pixel-identical to retail at 960x600, 1280x720 and 1024x768 as well. Before it, the
controls Mouse and Gamepad pages showed slider-value captions retail never
shows, the HUD page showed "Target Names: Yes" where retail shows "No", the
wrapped help text sat 3 px low, and the connect screen lacked "Press ESC to
cancel". All 44 tests in the same suites passed.

Fifth pass: all 13 interaction captures match retail at 960x600 and
1024x768, and the 1024x768 parity gate now runs every set (34 captures).
Before it, the slider drag stored three-decimal values, the bound item lost
its tint, editing started in whatever overstrike mode was last used, the
cursor drew 2-4 px off and the edit text was shadowed. All 47 tests in the
`ui`, `ui-hosting`, `unit`, `abi` and `contract` suites passed.

Sixth pass: on an FnQL build with `Cvar_SetFromUI`, all 31 interaction
captures and all 12 cvar results match retail at 960x600 and 1024x768. All
nine `ui-hosting` gates and the 54 `ui`, `unit`, `abi` and `contract` tests
passed, as did 76 of FnQL's 77 engine tests (the retail factory probe
skipped for want of its environment). Before it, every protected option
write was refused in both UIs, and presets stored `0` where retail stores
`0.000000`.

Seventh pass: menus follow retail's grammar and script set only, and every
UI probe runs headless (`com_headless 2`) with Steam and the web UI at their
defaults. All nine `ui-hosting` gates and the 55 `ui`, `unit`, `abi` and
`contract` tests passed. With the web UI up, both UIs yield the out-of-game
screens to it identically, and the connect screen still matches retail. A
rerun with the FnQL-Steam provider beside the shared `fnql.exe` and a
signed-in Steam client (`com_steamProviderStatus active`) passed all nine
`ui-hosting` gates too; the probe homes kept every profile write
(`qzconfig.cfg`, `repconfig.cfg`, the web UI's data) away from the install.

Earlier local validation on 2026-11-05:

- All five UI gates passed (`ui-legacy-gates.txt` in the Meson log directory).
- All 31 unit tests and both cgame hosting checks passed after shared-menu
  changes (`ui-shared-unit.txt`, `ui-shared-hosting.txt`).
- 220 engine ABI comparisons and all 32 engine UI bridge tests passed.
- The broader run passed 417/419 tests (`ui-followup-regression.txt`). The
  unrelated failures were `retail-client-join-unpure` (client never entered;
  later traced to a stale `profile.pid` safe-mode prompt, see CGAME.md)
  and `fnql-cgame-fnql-combat` (no client-observed kill); that combat probe
  loaded the retail UI.

Final engine captures and runtime logs under `.tmp/ui/`:

| Resolution | FnQL-game | Retail reference |
| --- | --- | --- |
| 960x600 | `legacy-fnql-iwv3huwl/baseq3/` | `legacy-retail-ib23letp/baseq3/` |
| 640x480 | `legacy-fnql-7snh4isu/baseq3/` | `legacy-retail-5rb2z0xg/baseq3/` |

The obsolete probe homes under `.tmp/ui/` were removed in the gap pass; only
the captures cited above remain. The obsolete source files and empty
`code/q3_ui/` directory are removed.
