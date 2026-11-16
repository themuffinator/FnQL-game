project('fnql-game', 'c',
  version: '0.1.0',  # canonical value lives in version/fnql_game_version.h
  meson_version: '>=1.1.0',
  default_options: [
    'buildtype=release',
    'warning_level=1',
    'b_vscrt=static_from_buildtype',
  ])

# FnQL-game builds drop-in replacements for the retail Quake Live game modules.
# Targets are enabled one at a time, in step with FnQL's Project Plan step 7
# (milestone M11), each behind its own gates: 32-bit x86 (the retail ABI) and
# x86_64 on Windows and Linux, where FnQL-game is the only game.

cc = meson.get_compiler('c')
fs = import('fs')

x86_target_probe = '''
#if !defined(_M_IX86) && !defined(__i386__)
#error FnQL-game requires an x86 target
#endif
typedef char fnql_game_pointer_width_must_be_32_bits[(sizeof(void *) == 4) ? 1 : -1];
int main(void) { return 0; }
'''
x86_64_target_probe = '''
#if !defined(_M_AMD64) && !defined(__x86_64__)
#error FnQL-game requires an x86_64 target
#endif
typedef char fnql_game_pointer_width_must_be_64_bits[(sizeof(void *) == 8) ? 1 : -1];
int main(void) { return 0; }
'''
cpu_family = host_machine.cpu_family()
if cpu_family == 'x86'
  target_supported = cc.compiles(x86_target_probe, name: '32-bit x86 target')
elif cpu_family == 'x86_64'
  target_supported = cc.compiles(x86_64_target_probe, name: 'x86_64 target')
else
  target_supported = false
endif
if not target_supported
  error('FnQL-game supports only 32-bit x86 and x86_64 targets; Meson configured host CPU family "'
    + cpu_family + '" or the compiler target probe failed. Use a vcvars32 (Win32) or vcvars64 (x64) '
    + 'environment on Windows, and an i686 cross file or a native x86_64 compiler on Linux.')
endif

win32 = host_machine.system() == 'windows'
linux = host_machine.system() == 'linux'
if not (win32 or linux)
  error('FnQL-game targets Windows and Linux; other platforms are enabled through milestone M11.')
endif

# Module names carry the engine's ARCH_STRING: x86 on Windows and i386 on
# Linux for the retail-ABI target, x86_64 on both for the 64-bit target (no
# retail 64-bit Windows modules exist).  FnQL-game prefixes "fnql_" so its
# modules never shadow the retail ones in a search path; the FnQL engine
# selects them by provider.
if cpu_family == 'x86'
  module_arch = win32 ? 'x86' : 'i386'
else
  module_arch = 'x86_64'
endif
# The retail client, its cgame and ui, and the retail Windows qagame exist only
# for 32-bit x86, and the G3 harness and the client probes are written against
# those x86 modules (retail bin.pk3 also carries a Linux qagamex64.so, which
# FnQL hosts on Linux x86_64, but no gate here drives it yet).  So the gates
# that host, join or diff against retail modules run only on x86; on other
# targets the FnQL hosting gates (G1 dedicated-smoke, combat-soak, map-sweep)
# still run.
retail_modules_hostable = cpu_family == 'x86'

module_prefix = 'fnql_'

c_args = []
link_args = []
# Movement must reproduce the retail x87 arithmetic bit for bit (gate G3):
# bg_pmove.c writes it as IEEE double expressions, so the compiler must
# neither fuse them into FMAs nor widen them to 80-bit x87 temporaries.
# MSVC's default /fp:precise already does neither.
if cc.get_argument_syntax() == 'msvc'
  c_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
    '-DWIN32',
    '/wd4244', '/wd4305', '/wd4018',  # baseline float/int narrowing and signedness noise
  ]
else
  c_args += cc.get_supported_arguments([
    '-fno-strict-aliasing',
    '-fvisibility=hidden',
    '-Wno-unused-parameter',
    '-ffp-contract=off',
  ])
  if cpu_family == 'x86'
    c_args += cc.get_supported_arguments(['-msse2', '-mfpmath=sse'])
  endif
  link_args += cc.get_supported_link_arguments(['-Wl,--no-undefined'])
endif

math_dep = cc.find_library('m', required: false)

game_sources = files(
  'code/game/g_main.c',
  'code/game/ai_chat.c',
  'code/game/ai_cmd.c',
  'code/game/ai_dmnet.c',
  'code/game/ai_dmq3.c',
  'code/game/ai_main.c',
  'code/game/ai_team.c',
  'code/game/ai_vcmd.c',
  'code/game/bg_lib.c',
  'code/game/bg_misc.c',
  'code/game/bg_pmove_settings.c',
  'code/game/bg_pmove.c',
  'code/game/bg_slidemove.c',
  'code/game/g_active.c',
  'code/game/g_arenas.c',
  'code/game/g_bot.c',
  'code/game/g_client.c',
  'code/game/g_fnql_ext.c',
  'code/game/g_names.c',
  'code/game/g_chattokens.c',
  'code/game/g_json.c',
  'code/game/g_statsreport.c',
  'code/game/g_cmds.c',
  'code/game/g_vote.c',
  'code/game/g_admin.c',
  'code/game/g_commandextras.c',
  'code/game/g_race.c',
  'code/game/g_access.c',
  'code/game/g_drop.c',
  'code/game/g_flagphysics.c',
  'code/game/g_runes.c',
  'code/game/g_combat.c',
  'code/game/g_complaint.c',
  'code/game/g_items.c',
  'code/game/g_quadhog.c',
  'code/game/g_laghax.c',
  'code/game/g_rounds.c',
  'code/game/g_domination.c',
  'code/game/g_mem.c',
  'code/game/g_misc.c',
  'code/game/g_missile.c',
  'code/game/g_mover.c',
  'code/game/g_native.c',
  'code/game/g_ql_entities.c',
  'code/game/g_rotation.c',
  'code/game/g_session.c',
  'code/game/g_spawn.c',
  'code/game/g_svcmds.c',
  'code/game/g_target.c',
  'code/game/g_team.c',
  'code/game/g_trigger.c',
  'code/game/g_unlagged.c',
  'code/game/g_utils.c',
  'code/game/g_weapon.c',
  'code/game/q_math.c',
  'code/game/q_shared.c',
)

qagame_args = ['-DQAGAME', '-DMISSIONPACK']
if get_option('trace-rand')
  qagame_args += ['-DFNQL_TRACE_RAND']
endif

qagame = shared_library(module_prefix + 'qagame' + module_arch,
  game_sources,
  name_prefix: '',
  c_args: c_args + qagame_args,
  link_args: link_args,
  dependencies: [math_dep],
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: 'baseq3')


cgame = shared_library(module_prefix + 'cgame' + module_arch,
  files(
    'code/cgame/cg_consolecmds.c',
    'code/cgame/cg_draw.c',
    'code/cgame/cg_text.c',
    'code/cgame/cg_statushud.c',
    'code/cgame/cg_matchclocks.c',
    'code/cgame/cg_pregame.c',
    'code/cgame/cg_standings.c',
    'code/cgame/cg_weaponbar.c',
    'code/cgame/cg_weaponselection.c',
    'code/cgame/cg_drawtools.c',
    'code/cgame/cg_effects.c',
    'code/cgame/cg_ents.c',
    'code/cgame/cg_event.c',
    'code/cgame/cg_ql_events.c',
    'code/cgame/cg_race.c',
    'code/cgame/cg_racepresentation.c',
    'code/cgame/cg_worldmarkers.c',
    'code/cgame/cg_poi.c',
    'code/cgame/cg_overheadnames.c',
    'code/cgame/cg_obituary.c',
    'code/cgame/cg_chatarea.c',
    'code/cgame/cg_endsummary.c',
    'code/cgame/cg_statusicons.c',
    'code/cgame/cg_spectatorhud.c',
    'code/cgame/cg_endvote.c',
    'code/cgame/cg_voteprompt.c',
    'code/cgame/cg_itempresentation.c',
    'code/cgame/cg_duelhud.c',
    'code/cgame/cg_teamstats.c',
    'code/cgame/cg_itemtimers.c',
    'code/cgame/cg_teamvitals.c',
    'code/cgame/cg_teamoverlay.c',
    'code/cgame/cg_hudlines.c',
    'code/cgame/cg_weaponcolor.c',
    'code/cgame/cg_localsounds.c',
    'code/cgame/cg_flagstatus.c',
    'code/cgame/cg_flagtrack.c',
    'code/cgame/cg_leadermodel.c',
    'code/cgame/cg_crosshair.c',
    'code/cgame/cg_demohud.c',
    'code/cgame/cg_gore.c',
    'code/cgame/cg_crosshairnames.c',
    'code/cgame/cg_ql_effects.c',
    'code/cgame/cg_damageplums.c',
    'code/cgame/cg_scores.c',
    'code/cgame/cg_scorecells.c',
    'code/cgame/cg_info.c',
    'code/cgame/cg_localents.c',
    'code/cgame/cg_fnql_ext.c',
    'code/cgame/cg_main.c',
    'code/cgame/cg_marks.c',
    'code/cgame/cg_native.c',
    'code/cgame/cg_newdraw.c',
    'code/ui/ui_shared.c',
    'code/cgame/cg_players.c',
    'code/cgame/cg_playerpresentation.c',
    'code/cgame/cg_playerstate.c',
    'code/cgame/cg_predict.c',
    'code/cgame/cg_scoreboard.c',
    'code/cgame/cg_servercmds.c',
    'code/cgame/cg_snapshot.c',
    'code/cgame/cg_view.c',
    'code/cgame/cg_fov.c',
    'code/cgame/cg_ammo.c',
    'code/cgame/cg_weapons.c',
    'code/cgame/modulus.c',
    'code/game/bg_lib.c',
    'code/game/bg_misc.c',
    'code/game/bg_pmove.c',
    'code/game/bg_pmove_settings.c',
    'code/game/bg_slidemove.c',
    'code/game/q_math.c',
    'code/game/q_shared.c',
  ),
  name_prefix: '', c_args: c_args + ['-DCGAME', '-DMISSIONPACK'],
  link_args: link_args, dependencies: [math_dep],
  gnu_symbol_visibility: 'hidden', install: true, install_dir: 'baseq3')

cgame_native_test = executable('cgame_native',
  files('tests/cgame_native.c', 'code/cgame/cg_native.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-native', cgame_native_test, suite: ['unit', 'cgame'])

cgame_weaponselection_test = executable('cgame_weaponselection',
  files('tests/cgame_weaponselection.c', 'code/cgame/cg_weaponselection.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-weaponselection', cgame_weaponselection_test, suite: ['unit', 'cgame'])

cgame_weaponbar_test = executable('cgame_weaponbar',
  files('tests/cgame_weaponbar.c', 'code/cgame/cg_weaponbar.c', 'code/game/bg_misc.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-weaponbar', cgame_weaponbar_test, suite: ['unit', 'cgame'])

cgame_matchclocks_test = executable('cgame_matchclocks',
  files('tests/cgame_matchclocks.c', 'code/cgame/cg_matchclocks.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-matchclocks', cgame_matchclocks_test, suite: ['unit', 'cgame'])

cgame_pregame_test = executable('cgame_pregame',
  files('tests/cgame_pregame.c', 'code/cgame/cg_pregame.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-pregame', cgame_pregame_test, suite: ['unit', 'cgame'])

cgame_standings_test = executable('cgame_standings',
  files('tests/cgame_standings.c', 'code/cgame/cg_standings.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-standings', cgame_standings_test, suite: ['unit', 'cgame'])

cgame_statushud_test = executable('cgame_statushud',
  files('tests/cgame_statushud.c', 'code/cgame/cg_statushud.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-statushud', cgame_statushud_test, suite: ['unit', 'cgame'])

cgame_text_test = executable('cgame_text',
  files('tests/cgame_text.c', 'code/cgame/cg_text.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-text', cgame_text_test, suite: ['unit', 'cgame'])

cgame_fov_test = executable('cgame_fov',
  files('tests/cgame_fov.c', 'code/cgame/cg_fov.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-fov', cgame_fov_test, suite: ['unit', 'cgame'])

cgame_ammo_test = executable('cgame_ammo',
  files('tests/cgame_ammo.c', 'code/cgame/cg_ammo.c', 'code/game/bg_misc.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-ammo', cgame_ammo_test, suite: ['unit', 'cgame'])

cgame_events_test = executable('cgame_events',
  files('tests/cgame_events.c', 'code/cgame/cg_ql_events.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-events', cgame_events_test, suite: ['unit', 'cgame'])

cgame_race_test = executable('cgame_race',
  files('tests/cgame_race.c', 'code/cgame/cg_race.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-race', cgame_race_test, suite: ['unit', 'cgame'])

cgame_racepresentation_test = executable('cgame_racepresentation',
  files('tests/cgame_racepresentation.c', 'code/cgame/cg_racepresentation.c',
    'code/cgame/cg_worldmarkers.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-racepresentation', cgame_racepresentation_test, suite: ['unit', 'cgame'])

cgame_poi_test = executable('cgame_poi',
  files('tests/cgame_poi.c', 'code/cgame/cg_poi.c', 'code/cgame/cg_worldmarkers.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-poi', cgame_poi_test, suite: ['unit', 'cgame'])

cgame_localsounds_test = executable('cgame_localsounds',
  files('tests/cgame_localsounds.c', 'code/cgame/cg_localsounds.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-localsounds', cgame_localsounds_test, suite: ['unit', 'cgame'])

cgame_flagstatus_test = executable('cgame_flagstatus',
  files('tests/cgame_flagstatus.c', 'code/cgame/cg_flagstatus.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-flagstatus', cgame_flagstatus_test, suite: ['unit', 'cgame'])

cgame_crosshairnames_test = executable('cgame_crosshairnames',
  files('tests/cgame_crosshairnames.c', 'code/cgame/cg_crosshairnames.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-crosshairnames', cgame_crosshairnames_test, suite: ['unit', 'cgame'])

cgame_overheadnames_test = executable('cgame_overheadnames',
  files('tests/cgame_overheadnames.c', 'code/cgame/cg_overheadnames.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-overheadnames', cgame_overheadnames_test, suite: ['unit', 'cgame'])

cgame_obituary_test = executable('cgame_obituary',
  files('tests/cgame_obituary.c', 'code/cgame/cg_obituary.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-obituary', cgame_obituary_test, suite: ['unit', 'cgame'])

cgame_chatarea_test = executable('cgame_chatarea',
  files('tests/cgame_chatarea.c', 'code/cgame/cg_chatarea.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-chatarea', cgame_chatarea_test, suite: ['unit', 'cgame'])

cgame_endsummary_test = executable('cgame_endsummary',
  files('tests/cgame_endsummary.c', 'code/cgame/cg_endsummary.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-endsummary', cgame_endsummary_test, suite: ['unit', 'cgame'])

cgame_statusicons_test = executable('cgame_statusicons',
  files('tests/cgame_statusicons.c', 'code/cgame/cg_statusicons.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-statusicons', cgame_statusicons_test, suite: ['unit', 'cgame'])

cgame_endvote_test = executable('cgame_endvote',
  files('tests/cgame_endvote.c', 'code/cgame/cg_endvote.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-endvote', cgame_endvote_test, suite: ['unit', 'cgame'])

cgame_voteprompt_test = executable('cgame_voteprompt',
  files('tests/cgame_voteprompt.c', 'code/cgame/cg_voteprompt.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-voteprompt', cgame_voteprompt_test, suite: ['unit', 'cgame'])

cgame_itempresentation_test = executable('cgame_itempresentation',
  files('tests/cgame_itempresentation.c', 'code/cgame/cg_itempresentation.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-itempresentation', cgame_itempresentation_test, suite: ['unit', 'cgame'])

cgame_duelhud_test = executable('cgame_duelhud',
  files('tests/cgame_duelhud.c', 'code/cgame/cg_duelhud.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-duelhud', cgame_duelhud_test, suite: ['unit', 'cgame'])

cgame_teamstats_test = executable('cgame_teamstats',
  files('tests/cgame_teamstats.c', 'code/cgame/cg_teamstats.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-teamstats', cgame_teamstats_test, suite: ['unit', 'cgame'])

cgame_itemtimers_test = executable('cgame_itemtimers',
  files('tests/cgame_itemtimers.c', 'code/cgame/cg_itemtimers.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-itemtimers', cgame_itemtimers_test, suite: ['unit', 'cgame'])

cgame_teamvitals_test = executable('cgame_teamvitals',
  files('tests/cgame_teamvitals.c', 'code/cgame/cg_teamvitals.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-teamvitals', cgame_teamvitals_test, suite: ['unit', 'cgame'])

cgame_specmessages_test = executable('cgame_specmessages',
  files('tests/cgame_specmessages.c', 'code/cgame/cg_spectatorhud.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-specmessages', cgame_specmessages_test, suite: ['unit', 'cgame'])

cgame_flagtrack_test = executable('cgame_flagtrack',
  files('tests/cgame_flagtrack.c', 'code/cgame/cg_flagtrack.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-flagtrack', cgame_flagtrack_test, suite: ['unit', 'cgame'])

cgame_leadermodel_test = executable('cgame_leadermodel',
  files('tests/cgame_leadermodel.c', 'code/cgame/cg_leadermodel.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-leadermodel', cgame_leadermodel_test, suite: ['unit', 'cgame'])

cgame_crosshair_test = executable('cgame_crosshair',
  files('tests/cgame_crosshair.c', 'code/cgame/cg_crosshair.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-crosshair', cgame_crosshair_test, suite: ['unit', 'cgame'])

cgame_demohud_test = executable('cgame_demohud',
  files('tests/cgame_demohud.c', 'code/cgame/cg_demohud.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-demohud', cgame_demohud_test, suite: ['unit', 'cgame'])

cgame_loading_test = executable('cgame_loading',
  files('tests/cgame_loading.c', 'code/cgame/cg_info.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-loading', cgame_loading_test, suite: ['unit', 'cgame'])

cgame_gore_test = executable('cgame_gore',
  files('tests/cgame_gore.c', 'code/cgame/cg_gore.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-gore', cgame_gore_test, suite: ['unit', 'cgame'])

cgame_effects_test = executable('cgame_effects',
  files('tests/cgame_effects.c', 'code/cgame/cg_ql_effects.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-effects', cgame_effects_test, suite: ['unit', 'cgame'])

cgame_playerpresentation_test = executable('cgame_playerpresentation',
  files('tests/cgame_playerpresentation.c', 'code/cgame/cg_playerpresentation.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-playerpresentation', cgame_playerpresentation_test, suite: ['unit', 'cgame'])

cgame_prediction_test = executable('cgame_prediction',
  files('tests/cgame_prediction.c', 'code/cgame/cg_predict.c',
    'code/game/bg_pmove.c', 'code/game/bg_slidemove.c', 'code/game/bg_misc.c',
    'code/game/bg_pmove_settings.c', 'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-prediction', cgame_prediction_test, suite: ['unit', 'cgame'])

cgame_damage_test = executable('cgame_damage',
  files('tests/cgame_damage.c', 'code/cgame/cg_damageplums.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-damage', cgame_damage_test, suite: ['unit', 'cgame'])
cgame_scores_test = executable('cgame_scores',
  files('tests/cgame_scores.c', 'code/cgame/cg_scores.c', 'code/cgame/cg_scorecells.c'),
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('cgame-scores', cgame_scores_test, suite: ['unit', 'cgame'])

# One native UI; the legacy Quake III menu module is not built.
ui_sources = files('code/ui/ui_main.c', 'code/ui/ui_atoms.c',
  'code/ui/ui_gameinfo.c', 'code/ui/ui_players.c', 'code/ui/ui_shared.c',
  'code/ui/ui_native.c', 'code/ui/ui_ql.c', 'code/game/bg_misc.c',
  'code/game/bg_lib.c', 'code/game/q_shared.c', 'code/game/q_math.c')
ui_args = c_args + ['-DUI', '-DMISSIONPACK']
# Missing prototypes promote float arguments to double on x86 and corrupt
# calls across UI translation units. Treat that diagnostic as an ABI error.
ui_args += cc.get_argument_syntax() == 'msvc' ? ['/we4013'] : ['-Werror=implicit-function-declaration']
ui = shared_library(module_prefix + 'ui' + module_arch, ui_sources,
  name_prefix: '', c_args: ui_args,
  link_args: link_args, dependencies: [math_dep],
  gnu_symbol_visibility: 'hidden', install: true, install_dir: 'baseq3')

ui_native_test = executable('ui_native', files('tests/ui_native.c') + ui_sources,
  c_args: ui_args, dependencies: [math_dep])
test('ui-native', ui_native_test, suite: ['unit', 'ui'])

# Test-only input proxy for the UI interaction gate; never installed.
if host_machine.system() == 'windows'
  ui_input_proxy = shared_library('ui_input_proxy', files('tests/ui_input_proxy.c'),
    name_prefix: '')
  # Test-only snapshot hold for the loading-screen gate; never installed.
  cgame_snapshot_hold_proxy = shared_library('cgame_snapshot_hold_proxy',
    files('tests/cgame_snapshot_hold_proxy.c'), name_prefix: '')
endif

# Command policy and state transitions, without retail assets or a server.
game_drops = executable('game_drops', files('tests/game_drops.c', 'code/game/g_drop.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-drops', game_drops, suite: 'unit')
game_flagphysics = executable('game_flagphysics', files('tests/game_flagphysics.c', 'code/game/g_flagphysics.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-flagphysics', game_flagphysics, suite: 'unit')
game_runes = executable('game_runes', files('tests/game_runes.c', 'code/game/g_runes.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-runes', game_runes, suite: 'unit')
game_names = executable('game_names', files('tests/game_names.c', 'code/game/g_names.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-names', game_names, suite: 'unit')
game_fnql_ext = executable('game_fnql_ext', files('tests/game_fnql_ext.c', 'code/game/g_fnql_ext.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-fnql-ext', game_fnql_ext, suite: 'unit')
game_numbers = executable('game_numbers', files('tests/game_numbers.c',
    'code/game/q_shared.c', 'code/game/q_math.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-numbers', game_numbers, suite: 'unit')
game_chattokens = executable('game_chattokens', files('tests/game_chattokens.c', 'code/game/g_chattokens.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-chattokens', game_chattokens, suite: 'unit')
game_json = executable('game_json', files('tests/game_json.c', 'code/game/g_json.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-json', game_json, suite: 'unit')
game_session = executable('game_session', files('tests/game_session.c', 'code/game/g_session.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-session', game_session, suite: 'unit')
game_laghax = executable('game_laghax', files('tests/game_laghax.c', 'code/game/g_laghax.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-laghax', game_laghax, suite: 'unit')
game_complaint = executable('game_complaint', files('tests/game_complaint.c', 'code/game/g_complaint.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-complaint', game_complaint, suite: 'unit')
game_quadhog = executable('game_quadhog', files('tests/game_quadhog.c', 'code/game/g_quadhog.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-quadhog', game_quadhog, suite: 'unit')

game_domination = executable('game_domination', files('tests/game_domination.c', 'code/game/g_domination.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-domination', game_domination, suite: 'unit')

game_admin = executable('game_admin', files('tests/game_admin.c', 'code/game/g_admin.c', 'code/game/g_access.c',
  'code/game/g_commandextras.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-admin', game_admin, suite: 'unit')
game_race = executable('game_race', files('tests/game_race.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-race', game_race, suite: 'unit')


game_voting = executable('game_voting', files('tests/game_voting.c', 'code/game/g_vote.c',
  'code/game/q_shared.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-voting', game_voting, suite: 'unit')

# Gate G0: shared structure layouts must match the FnQL engine headers.
python = find_program('python3', 'python')

# The FnQL-game packages (docs/PLAN.md M10): the client package holds only the
# cgame and ui, which an FnQL client runs on a pure FnQL server that lists the
# package in sv_fnqlClientPaks; the server package holds only the qagame.
# Neither is ever referenced to clients (FnQL docs/fnql/GAME_MODULES.md).
client_package = custom_target('fnql-game-client-package',
  output: 'fnql_game_client.pk3', input: [cgame, ui],
  command: [python, files('scripts/make_package.py'), '@OUTPUT@', '@INPUT@'],
  build_by_default: true, install: true, install_dir: 'baseq3')
server_package = custom_target('fnql-game-server-package',
  output: 'fnql_game_server.pk3', input: [qagame],
  command: [python, files('scripts/make_package.py'), '@OUTPUT@', '@INPUT@'],
  build_by_default: true, install: true, install_dir: 'baseq3')
fnql_root = get_option('fnql-root')
if fnql_root == ''
  fnql_root = meson.project_source_root() / '..' / 'FnQL'
endif
test('engine-abi-parity', python,
  args: [files('scripts/check_engine_abi.py'), '--fnql', fnql_root, '--cc', ' '.join(cc.cmd_array())],
  suite: 'abi')

# Gate G2: shared tables (items, enums, configstrings, spawn table, bot item
# indices) must match the retail wire contract in docs/contract/.
test('wire-contract', python,
  args: [files('scripts/check_wire_contract.py'), '--cc', ' '.join(cc.cmd_array())],
  suite: 'contract')

# The committed retail tables must still match the user's retail install.
# Skips (exit 77) when no install is found.
retail_table_args = [files('scripts/extract_retail_contract.py')]
if get_option('retail-ql') != ''
  retail_table_args += ['--retail', get_option('retail-ql')]
endif
test('retail-tables-current', python, args: retail_table_args, suite: 'contract')

# Gate G3: FnQL-game's Pmove runs in lockstep with the retail one.  The
# harness loads the retail Windows module and calls its Pmove through MSVC
# inline assembly, so it exists only for Windows x86 MSVC builds; it skips
# (exit 77) without a retail install.
if win32 and host_machine.cpu_family() == 'x86' and cc.get_id() == 'msvc'
  pmove_lockstep = executable('pmove_lockstep',
    files('tests/lockstep/pmove_lockstep.c', 'tests/lockstep/lockstep_world.c',
      'code/game/bg_lib.c', 'code/game/bg_misc.c', 'code/game/bg_pmove.c', 'code/game/bg_pmove_settings.c',
      'code/game/bg_slidemove.c', 'code/game/q_math.c', 'code/game/q_shared.c'),
    c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'],
    dependencies: [math_dep])
  lockstep_args = [files('scripts/pmove_lockstep.py'), '--harness', pmove_lockstep.full_path()]
  if get_option('retail-ql') != ''
    lockstep_args += ['--retail', get_option('retail-ql')]
  endif
  test('pmove-lockstep', python, args: lockstep_args, depends: pmove_lockstep,
    suite: 'parity', timeout: 120)
endif

# Gate G1: boot the module on a retail map in an FnQL dedicated server.
if get_option('fnql-dedicated') != ''
  smoke_args = [files('scripts/smoke_dedicated.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    smoke_args += ['--retail', get_option('retail-ql')]
  endif
  test('dedicated-smoke', python, args: smoke_args, depends: qagame,
    suite: 'runtime', timeout: 180, is_parallel: false)

  # Milestone 4: bots fight a sustained match with the combat code.
  soak_args = [files('scripts/combat_soak.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    soak_args += ['--retail', get_option('retail-ql')]
  endif
  test('combat-soak', python, args: soak_args, depends: qagame,
    suite: 'runtime', timeout: 240, is_parallel: false)

  # Match statistics reports: a bot match from ready-up to the closing
  # report, every document held to docs/contract/stats_events.json.  Where
  # the retail qagame can be hosted (x86) the same run hosts it first, which
  # checks the schema against the install, and compares the documents that
  # do not depend on play.  Each variant is the factory, its map, the limit
  # that ends the match, the longest wait, and the event types the run must
  # produce.  The CTF run is time-boxed and does not wait for a report:
  # FnQL-game's bots do not yet play the flag as retail's do (docs/PLAN.md,
  # M9), so a capture cannot be counted on to end the match.
  stats_match = 'PLAYER_CONNECT,PLAYER_SWITCHTEAM,PLAYER_KILL,PLAYER_DEATH,MATCH_STARTED'
  stats_variants = {
    'ffa': ['ffa', 'campgrounds', '5', '300', stats_match + ',PLAYER_STATS,MATCH_REPORT'],
    'ca': ['ca', 'campgrounds', '5', '300', stats_match + ',PLAYER_STATS,MATCH_REPORT,ROUND_OVER'],
    'ctf': ['ctf', 'spidercrossings', '8', '100', 'PLAYER_CONNECT,PLAYER_SWITCHTEAM,MATCH_STARTED,FLAG_STATUS'],
    'rr': ['rr', 'campgrounds', '5', '400', stats_match + ',PLAYER_STATS,MATCH_REPORT,ROUND_OVER'],
    # a player kicked, then the map changed under the match: the leaver's
    # statistics, everyone else's marked aborted, and the aborted report
    'abort': ['ffa', 'campgrounds', '50', '150',
      'PLAYER_CONNECT,PLAYER_DISCONNECT,PLAYER_STATS,MATCH_STARTED,MATCH_REPORT',
      '--expect-abort', '--script', 'wait 700; clientkick 0; wait 80; map campgrounds ffa'],
  }
  foreach name, variant : stats_variants
    stats_args = [files('scripts/stats_probe.py'),
      '--engine', get_option('fnql-dedicated'), '--module', qagame.full_path(),
      '--factory', variant[0], '--map', variant[1], '--fraglimit', variant[2], '--seconds', variant[3],
      '--require', variant[4]]
    foreach index : range(5, variant.length())
      stats_args += [variant[index]]
    endforeach
    if get_option('retail-ql') != ''
      stats_args += ['--retail', get_option('retail-ql')]
    endif
    if retail_modules_hostable
      stats_args += ['--compare']
    endif
    test('stats-events-' + name, python, args: stats_args, depends: qagame,
      suite: 'stats', timeout: 1200, is_parallel: false)
  endforeach

  # Every stock map must load (G1 sweep); slow, so it is its own suite.
  sweep_args = [files('scripts/sweep_maps.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    sweep_args += ['--retail', get_option('retail-ql')]
  endif
  test('map-sweep', python, args: sweep_args, depends: qagame,
    suite: 'sweep', timeout: 1200, is_parallel: false)

  # Every gate below hosts, joins or diffs against a retail module, so it is
  # x86 only (retail_modules_hostable); the block is not re-indented.
  if retail_modules_hostable

  # Gate G5b: the user's retail Steam client joins this server and stays in.
  # It launches the retail game through Steam, which cannot run windowless,
  # so it runs only on request: FNQL_ALLOW_RETAIL_CLIENT=1 meson test --suite
  # retail-client (its windows are hidden as they appear). Skips otherwise,
  # and without Steam running.
  retail_client_args = [files('scripts/retail_client_probe.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    retail_client_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-client-join', python,
    args: retail_client_args + ['--pure', '1', '--require-auth'], depends: qagame,
    suite: 'retail-client', timeout: 400, is_parallel: false)
  test('retail-client-join-unpure', python,
    args: retail_client_args + ['--pure', '0', '--require-auth'], depends: qagame,
    suite: 'retail-client', timeout: 400, is_parallel: false)
  # The retail engine hosts FnQL-game's cgame from a scratch base path, with
  # the retail install left untouched (retail_client_probe.py --cgame).
  test('retail-client-cgame', python,
    args: retail_client_args + ['--pure', '1', '--cgame', cgame.full_path()],
    depends: [qagame, cgame], suite: 'retail-client', timeout: 400, is_parallel: false)
  test('retail-client-cgame-unpure', python,
    args: retail_client_args + ['--pure', '0', '--cgame', cgame.full_path()],
    depends: [qagame, cgame], suite: 'retail-client', timeout: 400, is_parallel: false)
  # Milestone M10.  The FnQL-game client package loaded on the server is
  # offered to FnQL clients, but the retail client neither downloads, holds
  # nor reports it, and never takes part in the extension negotiation.
  foreach pure : ['1', '0']
    test('retail-client-package' + (pure == '0' ? '-unpure' : ''), python,
      args: retail_client_args + ['--pure', pure, '--require-auth',
        '--server-package', client_package.full_path(), '--expect-handshake', 'no'],
      depends: [qagame, client_package], suite: 'retail-client', timeout: 400, is_parallel: false)
    # The drop-in check: FnQL-game's cgame and ui run inside the retail
    # engine, and the cgame answers fnql_game there too, so the extension
    # handshake completes through the retail engine.
    test('retail-client-dropin' + (pure == '0' ? '-unpure' : ''), python,
      args: retail_client_args + ['--pure', pure, '--require-auth', '--cgame', cgame.full_path(),
        '--ui', ui.full_path(), '--expect-handshake', 'yes'],
      depends: [qagame, cgame, ui], suite: 'retail-client', timeout: 400, is_parallel: false)
  endforeach
  # The retail engine as the server (retail_server_probe.py): it hosts
  # FnQL-game's qagame as a drop-in, pure and unpure, and an FnQL client joins
  # a retail-engine server.  The retail engine admits remote players only with
  # a Steam ticket; a client that cannot get one is refused, and those rows
  # then skip (77) with the server's answer.
  retail_server_args = [files('scripts/retail_server_probe.py'), '--package', client_package.full_path()]
  if get_option('retail-ql') != ''
    retail_server_args += ['--retail', get_option('retail-ql')]
  endif
  foreach pure : ['1', '0']
    suffix = pure == '0' ? '-unpure' : ''
    test('retail-server-dropin-qagame' + suffix, python,
      args: retail_server_args + ['--pure', pure, '--qagame', qagame.full_path(),
        '--expect-cgame', pure == '1' ? 'retail' : 'fnql', '--expect-handshake', pure == '1' ? 'no' : 'yes'],
      depends: [qagame, client_package], suite: 'retail-client', timeout: 400, is_parallel: false)
    if get_option('fnql-client') != ''
      test('retail-server-fnql-client' + suffix, python,
        args: retail_server_args + ['--pure', pure, '--client', get_option('fnql-client'),
          '--expect-cgame', pure == '1' ? 'retail' : 'fnql', '--expect-handshake', 'no'],
        depends: client_package, suite: 'retail-client', timeout: 400, is_parallel: false)
      test('retail-server-fnql-client-dropin' + suffix, python,
        args: retail_server_args + ['--pure', pure, '--client', get_option('fnql-client'),
          '--qagame', qagame.full_path(),
          '--expect-cgame', pure == '1' ? 'retail' : 'fnql', '--expect-handshake', pure == '1' ? 'no' : 'yes'],
        depends: [qagame, client_package], suite: 'retail-client', timeout: 400, is_parallel: false)
    endif
  endforeach
  # G5 per mode (milestone M6): the retail client plays each round-based and
  # special mode on a pure server.  Admin put places it on a team, the bots
  # take the other side, the forced ready-up starts the match (its
  # map_restart included) and the client must stay in through it, on its
  # team, with the server's gametype, factory and IN_PROGRESS state as
  # expected.  Opt-in like the joins above (FNQL_ALLOW_RETAIL_CLIENT=1).
  foreach mode, o : {
      'ca': ['ca', 'campgrounds', '4', 'r', 'blue'], 'vca': ['vca', 'campgrounds', '4', 'r', 'blue'],
      'ft': ['ft', 'campgrounds', '9', 'r', 'blue'], 'ift': ['ift', 'campgrounds', '9', 'r', 'blue'],
      'rr': ['rr', 'campgrounds', '12', 'r', 'blue'], 'infected': ['infected', 'campgrounds', '12', 'r', 'blue'],
      'ad': ['ad', 'duelingkeeps', '11', 'r', 'blue'], 'dom': ['dom', 'courtyard', '10', 'r', 'blue'],
      'har': ['har', 'duelingkeeps', '8', 'r', 'blue'], 'race': ['race', 'campgrounds', '2', 'f', ''],
      'iffa': ['iffa', 'campgrounds', '0', 'f', ''], 'ictf': ['ictf', 'duelingkeeps', '5', 'r', 'blue'],
      'quadhog': ['quadhog', 'campgrounds', '0', 'f', '']}
    mode_args = retail_client_args + ['--pure', '1', '--require-auth', '--factory', o[0], '--map', o[1],
      '--expect-gametype', o[2], '--expect-factory', o[0], '--team', o[3], '--allready', '--seconds', '40']
    if o[4] != ''
      mode_args += ['--bot-team', o[4]]
    endif
    test('retail-client-mode-' + mode, python, args: mode_args, depends: qagame,
      suite: 'retail-client', timeout: 400, is_parallel: false)
  endforeach
  test('retail-client-mode-overload', python,
    args: retail_client_args + ['--pure', '1', '--require-auth', '--cheats', '--factory', 'fnql_test_overload',
      '--map', 'duelingkeeps', '--factories', files('tests/lockstep/factories/objectives.factories'),
      '--expect-gametype', '7', '--expect-factory', 'fnql_test_overload', '--team', 'r',
      '--bot-team', 'blue', '--allready', '--seconds', '40'],
    depends: qagame, suite: 'retail-client', timeout: 400, is_parallel: false)

  # Gate G3: the full-server lockstep differential (meson test --suite
  # differential).  FnQL's lockstep dedicated server (FnQL
  # docs/fnql/SERVER_LOCKSTEP.md) hosts the retail qagame and FnQL-game's in
  # turn with the same map, seed and scripted clients, and every scripted
  # playerState, entity, configstring and server command is diffed frame by
  # frame.  Each scenario's self-check hosts the retail qagame twice and must
  # not diverge: it proves the rig is deterministic.  The differentials fail
  # until FnQL-game's game logic reaches retail parity (milestone M4).
  server_lockstep_args = [files('scripts/server_lockstep.py'),
    '--engine', get_option('fnql-dedicated')]
  if get_option('retail-ql') != ''
    server_lockstep_args += ['--retail', get_option('retail-ql')]
  endif
  foreach scenario : ['ffa_two_players', 'ffa_kill_restart', 'ffa_bot_restart']
    scenario_args = server_lockstep_args + [
      '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: scenario_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: scenario_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  # Item pickups need setviewpos, a cheat: the scenario runs under devmap.
  items_args = server_lockstep_args + ['--cheats', '--frames', '1400',
    '--scenario', files('tests/lockstep/server/ffa_items.txt')]
  test('server-lockstep-selfcheck-ffa_items', python, args: items_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ffa_items', python,
    args: items_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Team deathmatch: loadouts, team locations and the team overlay, the team
  # scoreboard with its item-control header, and a team-game death and
  # forced respawn.
  tdm_args = server_lockstep_args + ['--cheats', '--factory', 'tdm',
    '--scenario', files('tests/lockstep/server/tdm_items.txt')]
  test('server-lockstep-selfcheck-tdm_items', python, args: tdm_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_items', python,
    args: tdm_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Rename broadcasts and MG/HMG/CG impact metadata from a nonzero shooter,
  # through both box and capsule traces. Includes spectator recipients.
  foreach cylinders : ['0', '1']
    metadata_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', '600',
      '--set', 'g_playerCylinders', cylinders,
      '--scenario', files('tests/lockstep/server/tdm_event_metadata.txt')]
    test('server-lockstep-selfcheck-tdm_event_metadata' + cylinders, python,
      args: metadata_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-tdm_event_metadata' + cylinders, python,
      args: metadata_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  names_args = server_lockstep_args + ['--factory', 'tdm', '--frames', '500',
    '--scenario', files('tests/lockstep/server/tdm_colored_names.txt')]
  test('server-lockstep-selfcheck-tdm_colored_names', python,
    args: names_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_colored_names', python,
    args: names_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Duel: the spectate-only flag and the play queue (so, pq, the teamtask
  # refresh), repeated and refused team requests, scores_duel, and a duelist
  # moving to the spectators.
  duel_args = server_lockstep_args + ['--factory', 'duel',
    '--scenario', files('tests/lockstep/server/duel_queue.txt')]
  test('server-lockstep-selfcheck-duel_queue', python, args: duel_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_queue', python,
    args: duel_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # A played duel: ready-up, the countdown's attack lockout, the restart into
  # the match, a forfeit, the intermission with its map vote, and ExitLevel
  # (the run stops at the frame before the voted map loads).
  duel_match_args = server_lockstep_args + ['--factory', 'duel', '--frames', '1827',
    '--scenario', files('tests/lockstep/server/duel_match.txt')]
  test('server-lockstep-selfcheck-duel_match', python, args: duel_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_match', python,
    args: duel_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # The forfeit command: refused in warmup and to the leader, then taken.
  forfeit_args = server_lockstep_args + ['--factory', 'duel', '--frames', '800',
    '--scenario', files('tests/lockstep/server/duel_forfeit.txt')]
  test('server-lockstep-selfcheck-duel_forfeit', python, args: forfeit_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_forfeit', python,
    args: forfeit_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Auto actions (cg_autoAction demos and screenshots with retail's file
  # names), acc, pstats and ragequit, and following: a follower stays a
  # spectator and resumes following after the restart into the match.
  duel_commands_args = server_lockstep_args + ['--factory', 'duel', '--frames', '1300',
    '--scenario', files('tests/lockstep/server/duel_commands.txt')]
  test('server-lockstep-selfcheck-duel_commands', python, args: duel_commands_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_commands', python,
    args: duel_commands_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # The remaining retail server rules (docs/PLAN.md, "Remaining retail
  # cvars"): inactivity, spectator and duel chat, team joins, sudden-death
  # respawn, automatic shuffle, the duel ready delay, g_teamSpawnAsSpec,
  # the gauntlet tackle, the kamikaze earthquake and the thaw debug print.
  # Each variant is [scenario, extra server_lockstep.py arguments...].
  server_rules = {
    'inactivity-drop': ['ffa_inactivity', '--frames', '400',
      '--set', 'g_inactivity', '4', '--set', 'g_inactivityWarning', '2'],
    'inactivity-spectate': ['ffa_inactivity_spectate', '--frames', '400', '--set', 'g_inactivity', '4',
      '--set', 'g_inactivityWarning', '1', '--set', 'g_dropInactive', '0'],
    'spec-chat-ffa': ['ffa_spec_chat', '--frames', '120'],
    'spec-chat-tdm': ['tdm_spec_chat', '--factory', 'tdm', '--frames', '120'],
    'spec-chat-tdm-disabled': ['tdm_spec_chat', '--factory', 'tdm', '--frames', '120',
      '--set', 'g_teamSpecSayEnable', '0'],
    'spec-chat-duel': ['duel_spec_chat', '--factory', 'duel', '--frames', '700'],
    'chat-tokens-tdm': ['tdm_chat_tokens', '--factory', 'tdm', '--frames', '160'],
    'chat-tokens-ffa': ['tdm_chat_tokens', '--frames', '160'],
    'team-join': ['tdm_team_join', '--factory', 'tdm', '--frames', '800', '--set', 'teamsize', '2'],
    'ready-gate-ffa': ['ffa_ready_gate', '--frames', '400', '--set', 'teamsize', '2'],
    'ready-gate-tdm': ['tdm_ready_gate', '--factory', 'tdm', '--frames', '400',
      '--set', 'g_teamForceBalance', '1'],
    'sudden-death': ['ffa_sudden_death', '--frames', '3000', '--set', 'g_doWarmup', '0',
      '--set', 'g_warmup', '0', '--set', 'g_suddenDeathRespawn', '1',
      '--set', 'g_suddenDeathRespawnTick', '2', '--set', 'g_suddenDeathRespawnStart', '1',
      '--set', 'g_suddenDeathRespawnIncrement', '2', '--set', 'g_suddenDeathRespawnMax', '6'],
    'autoshuffle': ['tdm_autoshuffle', '--factory', 'tdm', '--frames', '800',
      '--set', 'g_shuffle_automatic', '1', '--set', 'g_shuffle_automatic_minplayers', '3',
      '--set', 'g_shuffle_timedelay', '2000'],
    'ready-delay-bench': ['duel_ready_delay', '--factory', 'duel', '--frames', '500',
      '--set', 'g_warmupReadyDelay', '3'],
    'ready-delay-ready': ['duel_ready_delay', '--factory', 'duel', '--frames', '500',
      '--set', 'g_warmupReadyDelay', '3', '--set', 'g_warmupReadyDelayAction', '2'],
    'spawn-as-spec': ['tdm_spawn_as_spec', '--factory', 'tdm', '--frames', '300',
      '--set', 'g_teamSpawnAsSpec', '1'],
    'spawn-keep-team': ['tdm_spawn_as_spec', '--factory', 'tdm', '--frames', '300'],
    'tackle': ['ctf_tackle', '--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
      '--frames', '400', '--set', 'g_tackleFlag', '400'],
    'tackle-off': ['ctf_tackle', '--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
      '--frames', '400'],
    'kamikaze': ['ffa_kamikaze', '--cheats', '--frames', '300'],
    'kamikaze-attenuated': ['ffa_kamikaze', '--cheats', '--frames', '300',
      '--set', 'g_kamiAttenuate', '512', '--set', 'g_kamiMinRatio', '0'],
    'thaw-debug': ['round_thaw', '--cheats', '--factory', 'ft', '--frames', '1800',
      '--set', 'g_debugThawTime', '1'],
  }
  foreach name, spec : server_rules
    rule_args = server_lockstep_args + ['--scenario', files('tests/lockstep/server/' + spec[0] + '.txt')]
    foreach index : range(1, spec.length())
      rule_args += [spec[index]]
    endforeach
    test('server-rules-' + name + '-selfcheck', python, args: rule_args + ['--self-check'],
      suite: 'server-rules', timeout: 300, is_parallel: false)
    test('server-rules-' + name + '-differential', python,
      args: rule_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'server-rules', timeout: 300, is_parallel: false)
  endforeach
  # Milestone M5's match exits (retail CheckExitRules 0x10058030), each
  # through to the intermission: the fraglimit after a tie at it, the
  # timelimit after three overtimes, the mercy limit held to g_mercytime, and
  # the capturelimit inside a team-score overtime.
  match_exits = {
    'fraglimit-ffa': ['ffa_fraglimit', '--frames', '1200', '--set', 'g_doWarmup', '0',
      '--set', 'g_warmup', '0'],
    'overtime-duel': ['duel_overtime', '--factory', 'duel', '--frames', '4300'],
    'mercylimit-tdm': ['tdm_mercy', '--factory', 'tdm', '--frames', '3500'],
    'capturelimit-ctf': ['ctf_capturelimit', '--map', 'spidercrossings', '--factory', 'ctf',
      '--frames', '3600'],
    # g_training on an ordinary map turns itself off (retail 0x10024fa0).
    'training-reset-ffa': ['ffa_training_reset', '--cheats', '--frames', '400',
      '--set', 'g_training', '1', '--set', 'g_skipTrainingEnable', '1'],
  }
  foreach name, spec : match_exits
    exit_args = server_lockstep_args + ['--scenario', files('tests/lockstep/server/' + spec[0] + '.txt')]
    foreach index : range(1, spec.length())
      exit_args += [spec[index]]
    endforeach
    test('match-exit-' + name + '-selfcheck', python, args: exit_args + ['--self-check'],
      suite: ['match-flow', 'differential'], timeout: 300, is_parallel: false)
    test('match-exit-' + name + '-differential', python,
      args: exit_args + ['--module', qagame.full_path()], depends: qagame,
      suite: ['match-flow', 'differential'], timeout: 300, is_parallel: false)
  endforeach
  # Key doors, on a hand-authored fixture: no stock map carries one.
  key_door_args = server_lockstep_args + ['--cheats', '--frames', '900',
    '--entities', files('tests/lockstep/entities/key_doors.ent'),
    '--scenario', files('tests/lockstep/server/ffa_key_doors.txt')]
  test('server-rules-key-doors-selfcheck', python, args: key_door_args + ['--self-check'],
    suite: 'server-rules', timeout: 300, is_parallel: false)
  test('server-rules-key-doors-differential', python,
    args: key_door_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'server-rules', timeout: 300, is_parallel: false)
  # Worldspawn keys: disable_loadout and atmosphere reach their configstrings.
  worldspawn_key_args = server_lockstep_args + ['--factory', 'ffa', '--frames', '200', '--port', '28031',
    '--set', 'g_loadout', '1',
    '--entities', files('tests/lockstep/entities/worldspawn_keys.ent'),
    '--scenario', files('tests/lockstep/server/ffa_worldspawn_keys.txt')]
  test('server-rules-worldspawn-keys-selfcheck', python, args: worldspawn_key_args + ['--self-check'],
    suite: 'server-rules', timeout: 300, is_parallel: false)
  test('server-rules-worldspawn-keys-differential', python,
    args: worldspawn_key_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'server-rules', timeout: 300, is_parallel: false)
  # Public voting: grammar, electorate, actions and match transitions.
  foreach scenario, spec : {'voting_rules': ['ffa', '1820'],
                           'voting_actions': ['ffa', '1080'],
                           'voting_match': ['tdm', '1410'],
                           'voting_teams': ['tdm', '600'],
                           'voting_smallteams': ['tdm', '340']}
    vote_args = server_lockstep_args + ['--factory', spec[0], '--frames', spec[1],
      '--port', '28015', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: vote_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: vote_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  # A team deathmatch match: the warmup delay, team scoring and its sounds,
  # a team forfeit, and the intermission's team statistics.
  tdm_match_args = server_lockstep_args + ['--factory', 'tdm', '--frames', '1600',
    '--scenario', files('tests/lockstep/server/tdm_match.txt')]
  test('server-lockstep-selfcheck-tdm_match', python, args: tdm_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_match', python,
    args: tdm_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Timeouts: refused and counted calls, a pause that holds a rocket, a quad,
  # an item timer, a respawn and the match clock, the admin command table
  # ("?", players, an unprivileged pause), and the caller's time in.
  # Shared administrative, chat and spectator command compatibility.
  foreach scenario, frames : {'tdm_admin': '1250', 'tdm_access': '70',
                              'tdm_commands': '70', 'tdm_shuffle': '100', 'tdm_shuffle_live': '1100'}
    admin_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', frames,
      '--port', '28025', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: admin_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: admin_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  # Race checkpoint records and RR command shuffles: retail determinism and parity.
  foreach scenario, frames : {'race_editor': '40', 'race_records': '280',
                              'rr_shuffle': '100', 'rr_shuffle_odd': '100', 'rr_join': '40', 'rr_shuffle_live': '1600'}
    mode = scenario.startswith('race') ? 'race' : 'rr'
    command_mode_args = server_lockstep_args + ['--cheats', '--factory', mode, '--frames', frames,
      '--port', '28025', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    if scenario == 'rr_shuffle_odd'
      command_mode_args += ['--seed', '7']
    endif
    if scenario == 'race_records'
      command_mode_args += ['--entities', files('tests/lockstep/entities/race_course.ent')]
    endif
    test('server-lockstep-selfcheck-' + scenario, python, args: command_mode_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: command_mode_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  tdm_timeout_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', '1600',
    '--scenario', files('tests/lockstep/server/tdm_timeout.txt')]
  test('server-lockstep-selfcheck-tdm_timeout', python, args: tdm_timeout_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_timeout', python,
    args: tdm_timeout_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Manual drops: inventory transfer, decay, expiry, and the timeout clock.
  foreach scenario, frames : {'tdm_drops': '1800', 'tdm_drop_timeout': '1500'}
    drop_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', frames,
      '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: drop_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: drop_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  # Rune spawning, ownership, effects, combat, drops and relocation.
  foreach scenario, frames : {'tdm_runes': '2200', 'tdm_rune_combat': '1500', 'tdm_rune_timeout': '1800'}
    rune_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--set', 'g_runes', '1',
      '--frames', frames, '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: rune_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python, args: rune_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  foreach scenario, extra : {'tdm_runes_tiered': ['--factory', 'tdm', '--set', 'armor_tiered', '1',
        '--frames', '2200', '--scenario', files('tests/lockstep/server/tdm_runes.txt')],
      'ctf_runes': ['--factory', 'ctf', '--map', 'spidercrossings', '--frames', '1800',
        '--scenario', files('tests/lockstep/server/ctf_runes.txt')],
      'ctf_rune_nodrop': ['--factory', 'ctf', '--map', 'spacectf', '--frames', '1000',
        '--scenario', files('tests/lockstep/server/ctf_rune_nodrop.txt')]}
    rune_args = server_lockstep_args + ['--cheats', '--set', 'g_runes', '1'] + extra
    test('server-lockstep-selfcheck-' + scenario, python, args: rune_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python, args: rune_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  foreach enabled : ['0', '1']
    rune_disconnect_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--set', 'g_runes', '1',
      '--set', 'g_dropPowerups', enabled, '--frames', '1500',
      '--scenario', files('tests/lockstep/server/tdm_rune_disconnect.txt')]
    test('server-lockstep-selfcheck-tdm_rune_disconnect' + enabled, python,
      args: rune_disconnect_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-tdm_rune_disconnect' + enabled, python,
      args: rune_disconnect_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  flag_drop_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1000', '--scenario', files('tests/lockstep/server/ctf_dropflag.txt')]
  test('server-lockstep-selfcheck-ctf_dropflag', python, args: flag_drop_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_dropflag', python,
    args: flag_drop_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Flag physics, including the default-off path, portal roll replication,
  # and Ironworks' fallback intermission spawn (no explicit intermission).
  foreach enabled : ['0', '1']
    physics_args = server_lockstep_args + ['--cheats', '--map', 'ironworks', '--factory', 'ctf',
      '--frames', '1000', '--set', 'g_flagPhysics', enabled,
      '--scenario', files('tests/lockstep/server/ctf_flagphysics.txt')]
    test('server-lockstep-selfcheck-ctf_flagphysics' + enabled, python, args: physics_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-ctf_flagphysics' + enabled, python,
      args: physics_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  zone_args = server_lockstep_args + ['--cheats', '--factory', 'ctf', '--frames', '600',
    '--entities', files('tests/lockstep/entities/capturezone.ent'),
    '--scenario', files('tests/lockstep/server/ctf_capturezone.txt')]
  test('server-lockstep-selfcheck-ctf_capturezone', python, args: zone_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_capturezone', python,
    args: zone_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Capture the flag: a capture, a return, the CTF scoreboard's partly hidden
  # team header, a team forfeit, and the end-of-match awards.
  ctf_match_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1700', '--scenario', files('tests/lockstep/server/ctf_match.txt')]
  test('server-lockstep-selfcheck-ctf_match', python, args: ctf_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_match', python,
    args: ctf_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Capture the flag bonuses: a carrier frag announced with the carrier's
  # powerup, base and carrier defense, a dropped flag's bounce, the give
  # cheat, powerup announcements, the lost-flag return at the intermission,
  # and every end-of-match award.
  ctf_defense_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1800', '--scenario', files('tests/lockstep/server/ctf_defense.txt')]
  test('server-lockstep-selfcheck-ctf_defense', python, args: ctf_defense_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_defense', python,
    args: ctf_defense_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)

  # Factory integration is a strict G3 gate, distinct from mode smoke tests.
  # Reject invalid names and disabled votes; accept a host-provided factory,
  # apply it through a passed vote, then retain it across map-only/restarts.
  factory_fixture = files('tests/lockstep/factories/custom.factories')
  foreach flags : ['0', '1', '8']
    factory_vote_args = server_lockstep_args + ['--port', '27994', '--factory', 'duel',
      '--frames', '80', '--set', 'g_voteFlags', flags,
      '--scenario', files('tests/lockstep/server/factory_votes.txt'), '--expect-factory', '79', 'duel']
    test('factory-votes-selfcheck-' + flags, python, args: factory_vote_args + ['--self-check'],
      suite: 'factory-contract', timeout: 300, is_parallel: false)
    test('factory-votes-differential-' + flags, python,
      args: factory_vote_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'factory-contract', timeout: 300, is_parallel: false)
  endforeach
  factory_custom_args = server_lockstep_args + ['--port', '27994', '--factory', 'duel',
    '--frames', '600', '--factories', factory_fixture,
    '--scenario', files('tests/lockstep/server/factory_custom.txt'),
    '--expect-factory', '0', 'duel', '--expect-factory', '200', 'fnql_test_duel',
    '--expect-factory', '350', 'fnql_test_duel', '--expect-factory', '499', 'fnql_test_duel',
    '--expect-factory', '599', 'duel']
  factory_map_only_args = server_lockstep_args + ['--port', '27994', '--factory', 'fnql_test_duel',
    '--frames', '280', '--factories', factory_fixture, '--set', 'g_voteFlags', '8',
    '--scenario', files('tests/lockstep/server/factory_map_only.txt'),
    '--expect-factory', '0', 'fnql_test_duel', '--expect-factory', '279', 'fnql_test_duel']
  foreach name, factory_args : {'custom': factory_custom_args, 'map-only': factory_map_only_args}
    test('factory-' + name + '-selfcheck', python, args: factory_args + ['--self-check'],
      suite: 'factory-contract', timeout: 300, is_parallel: false)
    test('factory-' + name + '-differential', python,
      args: factory_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'factory-contract', timeout: 300, is_parallel: false)
  endforeach

  # Isolated factory rules complement the complete round-mode scenarios.
  factory_gameplay = {
    'instagib': ['--factory', 'iffa', '--frames', '500'],
    'ictf': ['--factory', 'ictf', '--map', 'spidercrossings', '--frames', '1700'],
    'quadhog': ['--factory', 'quadhog', '--frames', '1600'],
    'quadhog_combat': ['--factory', 'quadhog', '--frames', '900', '--set', 'g_quadHogTime', '3'],
    'rules': ['--factory', 'ffa'],
    'health': ['--factory', 'ffa', '--set', 'g_startingHealth', '300',
      '--set', 'g_startingHealthBonus', '0', '--set', 'g_vampiricDamage', '0.75'],
  }
  foreach name, options : factory_gameplay
    gameplay_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/factory_' + name + '.txt')] + options
    test('factory-gameplay-' + name + '-selfcheck', python,
      args: gameplay_args + ['--self-check'], suite: 'factory-gameplay', timeout: 300, is_parallel: false)
    test('factory-gameplay-' + name + '-differential', python,
      args: gameplay_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'factory-gameplay', timeout: 300, is_parallel: false)
  endforeach

  # Strict retail round-mode gates: each replay first proves retail-twice
  # determinism, then compares all recorded state against the source module.
  round_scenarios = {
    'ca-combat': ['round_ca_combat', 'ca', '2100'],
    'ca-follow': ['round_ca_follow', 'ca', '2000'],
    'ca-timeout': ['round_timeout', 'ca', '2400'],
    'ca-limit': ['round_ca_combat', 'ca', '2100', '--set', 'roundlimit', '1'],
    'ca-draw': ['round_idle', 'ca', '2400', '--set', 'roundtimelimit', '5'],
    'ca-health-tiebreak': ['round_tiebreak', 'ca', '2100', '--set', 'roundtimelimit', '5'],
    'vca-combat': ['round_ca_combat', 'vca', '2200'],
    'ft-combat': ['round_combat', 'ft', '2100'],
    'ft-thaw': ['round_thaw', 'ft', '1800'],
    'ft-auto-thaw': ['round_thaw', 'ft', '1800', '--set', 'g_freezeAutoThawTime', '1000'],
    'ft-continuous': ['round_combat', 'ft', '2100', '--set', 'g_freezeRoundDelay', '0'],
    'ft-preserve-kit': ['round_combat', 'ft', '2100', '--set', 'g_freezeResetHealthOnRound', '0',
      '--set', 'g_freezeResetArmorOnRound', '0', '--set', 'g_freezeResetWeaponsOnRound', '0'],
    'ft-environment': ['round_ft_environment', 'ft', '2100'],
    'ft-warmup': ['round_ft_warmup', 'ft', '2100'],
    'ft-timeout': ['round_ft_timeout', 'ft', '2400'],
    'ift-thaw': ['round_thaw', 'ift', '1800'],
    'rr-combat': ['round_combat', 'rr', '2300'],
    'rr-limit': ['round_combat', 'rr', '2100', '--set', 'roundlimit', '1'],
    'rr-scoring': ['round_combat', 'rr', '2300', '--set', 'g_rrDamageScoreBonus', '2',
      '--set', 'g_rrRoundScoreBonus', '3', '--set', 'g_rrAllowNegativeScores', '1'],
    'infected-spread': ['round_infected', 'infected', '2300'],
    'infected-combat': ['round_infected_combat', 'infected', '2000'],
  }
  foreach name, options : round_scenarios
    round_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/' + options[0] + '.txt'),
      '--factory', options[1], '--frames', options[2]]
    foreach index : range(3, options.length())
      round_args += [options[index]]
    endforeach
    test('round-' + name + '-selfcheck', python, args: round_args + ['--self-check'],
      suite: 'round-modes', timeout: 300, is_parallel: false)
    test('round-' + name + '-differential', python,
      args: round_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'round-modes', timeout: 300, is_parallel: false)
  endforeach

  # Objective gameplay: compare full snapshots, events, commands and timing.
  surface_settings = {
    'disabled': ['g_forceDmgThroughSurface', '0'],
    'enabled': ['g_forceDmgThroughSurface', '1'],
    'short-probe': ['g_dmgThroughSurfaceDistance', '-8'],
    'angle': ['g_dmgThroughSurfaceAngularThreshold', '1.1'],
    'dampening': ['g_dmgThroughSurfaceDampening', '0.25'],
    'no-radius': ['g_dmgThroughSurfaceDampening', '0'],
  }
  foreach name, setting : surface_settings
    surface_args = server_lockstep_args + ['--port', '27994', '--cheats', '--frames', '500',
      '--scenario', files('tests/lockstep/server/ffa_surface_damage.txt'),
      '--set', 'g_forceDmgThroughSurface', '1', '--set', setting[0], setting[1]]
    test('combat-surface-' + name + '-selfcheck', python, args: surface_args + ['--self-check'],
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-surface-' + name + '-differential', python,
      args: surface_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  foreach mode, options : {'tdm': ['tdm_match', '1600'], 'ca': ['round_combat', '2100'], 'ft': ['round_combat', '2100']}
    warmup_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--factory', mode, '--set', 'g_doWarmup', '0', '--frames', options[1],
      '--scenario', files('tests/lockstep/server/' + options[0] + '.txt')]
    test('server-lockstep-nowarmup-' + mode + '-selfcheck', python, args: warmup_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-nowarmup-' + mode + '-differential', python,
      args: warmup_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  foreach enabled : ['0', '1']
    loadout_args = server_lockstep_args + ['--port', '27994', '--set', 'g_loadout', enabled,
      '--scenario', files('tests/lockstep/server/ffa_loadout.txt'), '--frames', '1100']
    test('combat-loadout' + enabled + '-selfcheck', python, args: loadout_args + ['--self-check'],
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-loadout' + enabled + '-differential', python,
      args: loadout_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  foreach scenario, frames : {'tdm_complaints': '4500', 'tdm_complaint_kick': '1700'}
    complaint_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--factory', 'tdm', '--scenario', files('tests/lockstep/server/' + scenario + '.txt'), '--frames', frames]
    test('combat-' + scenario + '-selfcheck', python, args: complaint_args + ['--self-check'],
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-' + scenario + '-differential', python,
      args: complaint_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  objective_scenarios = {
    'ad-capture': ['ad_capture', 'ad', 'attack_defend', '2800'],
    'ad-combat': ['objective_combat', 'ad', 'attack_defend', '2400'],
    'ad-follow': ['round_ca_follow', 'ad', 'attack_defend', '2300'],
    'ad-timeout': ['round_timeout', 'ad', 'attack_defend', '2400'],
    'ad-draw': ['round_idle', 'ad', 'attack_defend', '2400', '--set', 'roundtimelimit', '3'],
    'ad-history': ['round_idle', 'ad', 'attack_defend', '7000', '--set', 'roundtimelimit', '1',
      '--set', 'g_roundWarmupDelay', '0'],
    'ad-limit': ['ad_limit', 'ad', 'attack_defend', '3600'],
    'ad-drop': ['ad_drop', 'ad', 'attack_defend', '3100'],
    'ad-death-drop': ['ad_death_drop', 'ad', 'attack_defend', '3300'],
    'dom-capture': ['domination', 'dom', 'domination', '2500'],
    'dom-hold': ['domination_hold', 'dom', 'domination_three', '3800'],
    'dom-contention': ['domination_contention', 'dom', 'domination', '2700'],
    'dom-neutral': ['domination_contention', 'dom', 'domination', '2700', '--set', 'g_domNeutralFlag', '1'],
    'dom-no-contention': ['domination_contention', 'dom', 'domination', '2700', '--set', 'g_domNeutralFlag', '1', '--set', 'g_domEnableContention', '0'],
    'dom-regen': ['domination_regen', 'dom', 'domination', '3100'],
    'dom-tiered-regen': ['domination_regen', 'dom', 'domination', '3100', '--set', 'armor_tiered', '1'],
    'dom-timeout': ['domination_timeout', 'dom', 'domination', '3100'],
    'dom-defense': ['domination_defense', 'dom', 'domination', '2200'],
    'dom-limit': ['domination_limit', 'dom', 'domination', '3600'],
    'dom-spawn-fallback': ['domination_spawn', 'dom', 'domination_spawns', '2600'],
    'dom-owned-spawn': ['domination_spawn', 'dom', 'domination_spawns', '2600', '--set', 'g_spawnMinDistance', '0'],
    'har-capture': ['objective_combat', 'har', 'objectives', '2200'],
    'har-drop': ['harvester_drop', 'har', 'objectives', '2400'],
    'har-no-drop': ['harvester_drop', 'har', 'objectives', '2400', '--set', 'g_dropSkulls', '0'],
    'har-expiry': ['objective_combat', 'har', 'objectives', '2200', '--set', 'g_cubeTimeout', '2'],
    'har-cap': ['harvester_cap', 'har', 'objectives', '2100'],
    'har-limit': ['harvester_limit', 'har', 'objectives', '3600'],
    'har-restart': ['harvester_restart', 'har', 'objectives', '2400'],
    'oneflag-capture': ['oneflag_capture', 'fnql_test_oneflag', 'oneflag', '2300'],
    'oneflag-no-bonus': ['oneflag_capture', 'fnql_test_oneflag', 'oneflag', '2300', '--set', 'g_droppedFlagBonus', '0'],
    'oneflag-fast-ping': ['oneflag_capture', 'fnql_test_oneflag', 'oneflag', '2300', '--set', 'g_neutralFlagPingRate', '250'],
    'oneflag-zero-ping': ['oneflag_capture', 'fnql_test_oneflag', 'oneflag', '2300', '--set', 'g_neutralFlagPingRate', '0'],
    'oneflag-expiry': ['oneflag_expiry', 'fnql_test_oneflag', 'oneflag', '3500'],
    'oneflag-limit': ['oneflag_limit', 'fnql_test_oneflag', 'oneflag', '2100'],
    'oneflag-suicide-return': ['oneflag_capture', 'fnql_test_oneflag', 'oneflag', '2300', '--set', 'g_returnFlagOnSuicide', '1'],
    'overload-combat': ['overload_combat', 'fnql_test_overload', 'objectives', '2400'],
    'overload-limit': ['overload_limit', 'fnql_test_overload', 'objectives', '3600'],
    'overload-timeout': ['overload_timeout', 'fnql_test_overload', 'objectives', '2600'],
  }
  foreach name, options : objective_scenarios
    objective_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/' + options[0] + '.txt'),
      '--factory', options[1], '--entities', files('tests/lockstep/entities/' + options[2] + '.ent'), '--frames', options[3]]
    if options[1] == 'fnql_test_overload'
      objective_args += ['--factories', files('tests/lockstep/factories/objectives.factories'),
        '--set', 'g_obeliskHealth', '400', '--set', 'g_obeliskRespawnDelay', '2']
    elif options[1] == 'fnql_test_oneflag'
      objective_args += ['--factories', files('tests/lockstep/factories/oneflag.factories')]
    endif
    foreach index : range(4, options.length())
      objective_args += [options[index]]
    endforeach
    test('objective-' + name + '-selfcheck', python, args: objective_args + ['--self-check'],
      suite: 'objective-modes', timeout: 300, is_parallel: false)
    test('objective-' + name + '-differential', python,
      args: objective_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'objective-modes', timeout: 300, is_parallel: false)
  endforeach

  foreach enabled : ['0', '1']
    damage_health_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--factory', 'ffa', '--frames', '1600', '--set', 'g_dropDamagedHealth', enabled,
      '--scenario', files('tests/lockstep/server/ffa_damage_health.txt')]
    test('combat-damage-health' + enabled + '-selfcheck', python,
      args: damage_health_args + ['--self-check'], suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-damage-health' + enabled + '-differential', python,
      args: damage_health_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  # The shotgun, grenade launcher, plasma gun, BFG, nailgun and prox launcher,
  # at retail defaults, with box traces, and with their tunables moved off the
  # defaults (split so each run stays inside the engine's 32 '+' commands).
  foreach name, options : {
    'default': [],
    'box': ['--set', 'g_playerCylinders', '0'],
    'tuned-sg-gl': ['--set', 'g_damage_sg', '7', '--set', 'g_damage_sg_outer', '3',
      '--set', 'g_damage_sg_falloff', '1', '--set', 'g_knockback_sg', '2',
      '--set', 'g_velocity_gl', '1100', '--set', 'g_damage_gl', '60',
      '--set', 'g_splashradius_gl', '220', '--set', 'g_splashdamage_gl', '70',
      '--set', 'g_knockback_gl', '1.5', '--set', 'g_knockback_z', '40',
      '--set', 'g_knockback_z_self', '12', '--set', 'g_splashdamageOffset', '0.2'],
    'tuned-pg-bfg': ['--set', 'g_velocity_pg', '1400', '--set', 'g_damage_pg', '14',
      '--set', 'g_splashradius_pg', '48', '--set', 'g_splashdamage_pg', '22',
      '--set', 'g_knockback_pg_self', '2.2', '--set', 'g_velocity_bfg', '1200',
      '--set', 'g_damage_bfg', '70', '--set', 'g_splashradius_bfg', '160',
      '--set', 'g_splashdamage_bfg', '55', '--set', 'g_knockback_bfg', '1.7'],
    'tuned-ng-pl': ['--set', 'g_nailcount', '6', '--set', 'g_nailspread', '250',
      '--set', 'g_nailspeed', '1500', '--set', 'g_damage_ng', '9',
      '--set', 'g_nailbouncepercentage', '90', '--set', 'g_knockback_ng', '1.4',
      '--set', 'g_proxMineTimeout', '8', '--set', 'g_damage_pl', '40',
      '--set', 'g_splashradius_pl', '190', '--set', 'g_splashdamage_pl', '60',
      '--set', 'g_knockback_pl', '1.3'],
  }
    weapons_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--factory', 'ffa', '--frames', '1500',
      '--scenario', files('tests/lockstep/server/ffa_weapons.txt')] + options
    test('combat-weapons-' + name + '-selfcheck', python,
      args: weapons_args + ['--self-check'], suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-weapons-' + name + '-differential', python,
      args: weapons_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  foreach name, options : {
    'default': [],
    'no-history': ['--set', 'g_lagHaxHistory', '0'],
    'no-window': ['--set', 'g_lagHaxMs', '0'],
    'one-sample': ['--set', 'g_lagHaxHistory', '1'],
    'long-window': ['--set', 'g_lagHaxHistory', '8', '--set', 'g_lagHaxMs', '200'],
    'box': ['--set', 'g_playerCylinders', '0'],
  }
    lag_scenario = name == 'no-history' or name == 'no-window' ? 'ffa_laghax_disabled' : 'ffa_laghax'
    lag_args = server_lockstep_args + ['--port', '27993', '--cheats', '--factory', 'ffa',
      '--frames', '600', '--set', 'g_infiniteAmmo', '1',
      '--scenario', files('tests/lockstep/server/' + lag_scenario + '.txt')] + options
    test('combat-laghax-' + name + '-selfcheck', python, args: lag_args + ['--self-check'],
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-laghax-' + name + '-differential', python,
      args: lag_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach
  infinite_ammo_args = server_lockstep_args + ['--port', '27993', '--cheats', '--factory', 'ffa',
    '--frames', '300', '--scenario', files('tests/lockstep/server/ffa_infinite_ammo.txt')]
  test('combat-infinite-ammo-selfcheck', python, args: infinite_ammo_args + ['--self-check'],
    suite: 'combat-rules', timeout: 300, is_parallel: false)
  bad_userinfo_args = server_lockstep_args + ['--port', '27993', '--factory', 'ffa',
    '--frames', '400', '--scenario', files('tests/lockstep/server/ffa_bad_userinfo.txt')]
  test('server-bad-userinfo-selfcheck', python, args: bad_userinfo_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-bad-userinfo-differential', python,
    args: bad_userinfo_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'differential', timeout: 300, is_parallel: false)
  test('combat-infinite-ammo-differential', python,
    args: infinite_ammo_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'combat-rules', timeout: 300, is_parallel: false)
  jumppad_args = server_lockstep_args + ['--port', '27993', '--cheats', '--factory', 'ffa',
    '--frames', '600', '--scenario', files('tests/lockstep/server/ffa_jumppad.txt')]
  test('combat-jumppad-selfcheck', python, args: jumppad_args + ['--self-check'],
    suite: 'combat-rules', timeout: 300, is_parallel: false)
  test('combat-jumppad-differential', python,
    args: jumppad_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'combat-rules', timeout: 300, is_parallel: false)
  foreach cylinders : ['0', '1']
    headshot_args = server_lockstep_args + ['--port', '27993', '--cheats', '--factory', 'ffa',
      '--frames', '600', '--scenario', files('tests/lockstep/server/ffa_headshot.txt'),
      '--set', 'g_playerCylinders', cylinders]
    test('combat-headshot' + cylinders + '-selfcheck', python,
      args: headshot_args + ['--self-check'], suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-headshot' + cylinders + '-differential', python,
      args: headshot_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  foreach enabled : ['0', '1']
    discharge_args = server_lockstep_args + ['--port', '27993', '--cheats',
      '--factory', 'ffa', '--map', 'corrosion', '--frames', '500', '--set', 'g_lightningDischarge', enabled,
      '--scenario', files('tests/lockstep/server/ffa_discharge.txt')]
    test('combat-discharge' + enabled + '-selfcheck', python,
      args: discharge_args + ['--self-check'], suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-discharge' + enabled + '-differential', python,
      args: discharge_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach
  foreach cylinders : ['0', '1']
    grapple_args = server_lockstep_args + ['--port', '27993', '--cheats',
      '--factory', 'ffa', '--frames', '800', '--set', 'g_playerCylinders', cylinders,
      '--scenario', files('tests/lockstep/server/ffa_grapple.txt')]
    test('combat-grapple' + cylinders + '-selfcheck', python, args: grapple_args + ['--self-check'],
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-grapple' + cylinders + '-differential', python,
      args: grapple_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
    train_args = server_lockstep_args + ['--port', '27993', '--cheats',
      '--factory', 'ffa', '--frames', '600', '--set', 'g_playerCylinders', cylinders,
      '--entities', files('tests/lockstep/entities/grapple_train.ent'),
      '--scenario', files('tests/lockstep/server/ffa_grapple_train.txt')]
    test('combat-grapple-train' + cylinders + '-selfcheck', python,
      args: train_args + ['--self-check'], suite: 'combat-rules', timeout: 300, is_parallel: false)
    test('combat-grapple-train' + cylinders + '-differential', python,
      args: train_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'combat-rules', timeout: 300, is_parallel: false)
  endforeach

  discharge_infinite_args = server_lockstep_args + ['--port', '27993', '--cheats',
    '--factory', 'ffa', '--map', 'corrosion', '--frames', '500',
    '--set', 'g_lightningDischarge', '1', '--set', 'g_infiniteAmmo', '1', '--set', 'g_damage_lg', '1',
    '--scenario', files('tests/lockstep/server/ffa_discharge.txt')]
  test('combat-discharge-infinite-selfcheck', python, args: discharge_infinite_args + ['--self-check'],
    suite: 'combat-rules', timeout: 300, is_parallel: false)
  test('combat-discharge-infinite-differential', python,
    args: discharge_infinite_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'combat-rules', timeout: 300, is_parallel: false)

  # Direct startup complements the objective limit replays' automatic rotation.
  foreach name, options : {
    'brimstoneabbey': ['brimstoneabbey', 'dom'],
    'devilish': ['devilish', 'ca'],
    'retribution': ['retribution', 'ffa'],
    'push-sounds': ['campgrounds', 'ffa'],
  }
    stock_args = server_lockstep_args + ['--port', '27994', '--map', options[0],
      '--factory', options[1], '--frames', '500',
      '--scenario', files('tests/lockstep/server/stock_map_init.txt')]
    if name == 'push-sounds'
      stock_args += ['--entities', files('tests/lockstep/entities/push_sounds.ent')]
    endif
    test('stock-map-' + name + '-selfcheck', python, args: stock_args + ['--self-check'],
      suite: 'stock-maps', timeout: 300, is_parallel: false)
    test('stock-map-' + name + '-differential', python,
      args: stock_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'stock-maps', timeout: 300, is_parallel: false)
  endforeach

  # Two players moving and firing on the stock maps that showed each spawn
  # and map-entity divergence of the M4 sweep: the retail button travel
  # (retribution), the runtime's number parsing (fluorescent angles,
  # terminalheights phases, jumpwerkz exponent origins), the spawn fallback
  # to the red and blue player starts (basesiege), bobber phase arithmetic
  # (elder), trigger_hurt's frame rate and the dead's water events
  # (wargrounds, reflux), and rotating movers clearing what blocks them
  # (solarium).
  foreach map : ['retribution', 'fluorescent', 'terminalheights', 'jumpwerkz', 'basesiege',
                 'elder', 'wargrounds', 'reflux', 'solarium']
    sweep_args = server_lockstep_args + ['--port', '27994', '--map', map, '--frames', '400',
      '--scenario', files('tests/lockstep/server/ffa_two_players.txt')]
    test('stock-map-play-' + map + '-selfcheck', python, args: sweep_args + ['--self-check'],
      suite: 'stock-maps', timeout: 300, is_parallel: false)
    test('stock-map-play-' + map + '-differential', python,
      args: sweep_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'stock-maps', timeout: 300, is_parallel: false)
  endforeach

  # The flag and objective modes on a map without their starts or flags:
  # deferred spawns keep the location they spectated from, and Attack and
  # Defend spawns no map items but its red and blue flags (zero divergence).
  foreach factory : ['ctf', 'oneflag', 'har', 'ad']
    deferred_args = server_lockstep_args + ['--port', '27994', '--cheats', '--factory', factory,
      '--frames', '2400', '--scenario', files('tests/lockstep/server/ffa_kill_restart.txt')]
    test('server-lockstep-deferred-' + factory + '-selfcheck', python,
      args: deferred_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-deferred-' + factory + '-differential', python,
      args: deferred_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  ad_items_args = server_lockstep_args + ['--port', '27994', '--cheats', '--factory', 'ad',
    '--scenario', files('tests/lockstep/server/tdm_items.txt')]
  test('server-lockstep-ad-items-selfcheck', python, args: ad_items_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-ad-items-differential', python,
    args: ad_items_args + ['--module', qagame.full_path()], depends: qagame,
    suite: 'differential', timeout: 300, is_parallel: false)

  # Every retail factory must host FnQL-game without a fatal error, even on a
  # map without the mode's spawns or items (meson test --suite factories).
  # The divergence count is not the point here: the per-mode rules are
  # milestones 5 and 6.
  foreach factory : ['duel', 'race', 'ca', 'ctf', 'har', 'ft', 'dom', 'ad', 'rr', 'ift', 'infected', 'vca']
    test('server-lockstep-factory-' + factory, python,
      args: server_lockstep_args + ['--cheats', '--factory', factory, '--max-divergent', '100000000',
        '--scenario', files('tests/lockstep/server/tdm_items.txt'), '--module', qagame.full_path()],
      depends: qagame, suite: 'factories', timeout: 300, is_parallel: false)
  endforeach
  endif # retail_modules_hostable
endif

# Gate G5a: the retail cgame runs against FnQL-game's qagame in one client.
if get_option('fnql-client') != '' and retail_modules_hostable
  ui_probe_args = [files('scripts/ui_probe.py'),
    '--client', get_option('fnql-client'), '--module', ui.full_path()]
  if get_option('retail-ql') != ''
    ui_probe_args += ['--retail', get_option('retail-ql')]
  endif
  test('fnql-ui-retail-hosting', python, args: ui_probe_args,
    depends: ui, suite: 'ui-hosting', timeout: 120, is_parallel: false)
  test('fnql-ui-fnql-hosting', python,
    args: ui_probe_args + ['--qagame', qagame.full_path()],
    depends: [ui, qagame], suite: 'ui-hosting', timeout: 120, is_parallel: false)
  legacy_ui_args = [files('scripts/ui_legacy_probe.py'),
    '--client', get_option('fnql-client'), '--module', ui.full_path()]
  if get_option('retail-ql') != ''
    legacy_ui_args += ['--retail', get_option('retail-ql')]
  endif
  foreach dimensions : [['960', '600'], ['640', '480']]
    test('fnql-ui-legacy-' + dimensions[0], python,
      args: legacy_ui_args + ['--width', dimensions[0], '--height', dimensions[1]],
      depends: ui, suite: 'ui-hosting', timeout: 120, is_parallel: false)
  endforeach
  # Retail and FnQL-game render the five player-model previews identically.
  model_ui_args = [files('scripts/ui_model_probe.py'),
    '--client', get_option('fnql-client'), '--module', ui.full_path()]
  if get_option('retail-ql') != ''
    model_ui_args += ['--retail', get_option('retail-ql')]
  endif
  test('fnql-ui-model-previews', python, args: model_ui_args,
    depends: ui, suite: 'ui-hosting', timeout: 240, is_parallel: false)
  # The installed in-game menus render as retail's UI renders them, with the
  # world hidden, at a widescreen and a 4:3 size.
  parity_ui_args = [files('scripts/ui_menu_parity_probe.py'),
    '--client', get_option('fnql-client'), '--module', ui.full_path()]
  if get_option('retail-ql') != ''
    parity_ui_args += ['--retail', get_option('retail-ql')]
  endif
  # Both sizes cover every set: in-game menus, all tab pages, out-of-game
  # screens and the connect screen.
  foreach parity : [['960', '600', 'ingame,tabs,outgame'], ['1024', '768', 'ingame,tabs,outgame']]
    test('fnql-ui-menu-parity-' + parity[0], python,
      args: parity_ui_args + ['--width', parity[0], '--height', parity[1], '--sets', parity[2]],
      depends: ui, suite: 'ui-hosting', timeout: 900, is_parallel: false)
  endforeach
  # Scripted hover, slider drag, list, tab, edit-field and key-binding
  # interaction renders as retail's UI renders it (input through the proxy).
  if host_machine.system() == 'windows'
    interaction_ui_args = [files('scripts/ui_interaction_probe.py'),
      '--client', get_option('fnql-client'), '--module', ui.full_path(),
      '--proxy', ui_input_proxy.full_path()]
    if get_option('retail-ql') != ''
      interaction_ui_args += ['--retail', get_option('retail-ql')]
    endif
    foreach size : [['960', '600'], ['1024', '768']]
      test('fnql-ui-interaction-' + size[0], python,
        args: interaction_ui_args + ['--width', size[0], '--height', size[1]],
        depends: [ui, ui_input_proxy], suite: 'ui-hosting', timeout: 600, is_parallel: false)
    endforeach
  endif
  cgame_probe_args = [files('scripts/cgame_probe.py'),
    '--client', get_option('fnql-client'), '--module', cgame.full_path()]
  if get_option('retail-ql') != ''
    cgame_probe_args += ['--retail', get_option('retail-ql')]
  endif
  # The level loading screen renders as retail's cgame renders it, held in
  # its "Awaiting Snapshot" state by the test-only proxy, at a widescreen and
  # a 4:3 size.
  if host_machine.system() == 'windows'
    loading_args = [files('scripts/loading_screen_probe.py'),
      '--client', get_option('fnql-client'), '--module', cgame.full_path(),
      '--proxy', cgame_snapshot_hold_proxy.full_path()]
    if get_option('retail-ql') != ''
      loading_args += ['--retail', get_option('retail-ql')]
    endif
    # aerowalk's worldspawn names two authors, so its 4:3 run covers both
    # author lines.
    foreach size : [['960', '600', 'campgrounds'], ['1024', '768', 'aerowalk']]
      test('fnql-cgame-loading-' + size[0], python,
        args: loading_args + ['--width', size[0], '--height', size[1], '--map', size[2]],
        depends: [cgame, cgame_snapshot_hold_proxy], suite: 'cgame-hosting', timeout: 420, is_parallel: false)
    endforeach
    # Milestone 7 visual parity: a recorded bot match replayed frame-locked
    # (timedemo) with the retail cgame and FnQL-game's must give the same
    # captures, gameplay frames and the scoreboard, per mode and screen shape.
    parity_args = [files('scripts/cgame_parity_probe.py'),
      '--client', get_option('fnql-client'), '--module', cgame.full_path()]
    if get_option('retail-ql') != ''
      parity_args += ['--retail', get_option('retail-ql')]
    endif
    foreach run : [['ffa', 'campgrounds', '960', '600'], ['tdm', 'campgrounds', '960', '600'],
                   ['ca', 'campgrounds', '960', '600'], ['ctf', 'spidercrossings', '960', '600'],
                   ['ffa', 'campgrounds', '1024', '768']]
      test('fnql-cgame-parity-' + run[0] + '-' + run[2], python,
        args: parity_args + ['--factory', run[0], '--map', run[1], '--width', run[2], '--height', run[3]],
        depends: [cgame], suite: 'cgame-parity', timeout: 600, is_parallel: false)
    endforeach
  endif
  test('fnql-cgame-retail-hosting', python, args: cgame_probe_args,
    depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  test('fnql-cgame-fnql-hosting', python,
    args: cgame_probe_args + ['--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  test('fnql-cgame-retail-statushud', python, args: cgame_probe_args + ['--status-hud'],
    depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  test('fnql-cgame-fnql-statushud', python,
    args: cgame_probe_args + ['--status-hud', '--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  test('fnql-cgame-retail-weaponselection', python,
    args: cgame_probe_args + ['--weapon-selection'],
    depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  test('fnql-cgame-fnql-weaponselection', python,
    args: cgame_probe_args + ['--weapon-selection', '--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  foreach clock_mode : ['round', 'timeout', 'timein']
    test('fnql-cgame-retail-clock-' + clock_mode, python,
      args: cgame_probe_args + ['--clock-hud', clock_mode],
      depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-clock-' + clock_mode, python,
      args: cgame_probe_args + ['--clock-hud', clock_mode, '--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  endforeach
  foreach standings_mode : ['individual', 'team']
    test('fnql-cgame-retail-standings-' + standings_mode, python,
      args: cgame_probe_args + ['--standings-hud', standings_mode],
      depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-standings-' + standings_mode, python,
      args: cgame_probe_args + ['--standings-hud', standings_mode, '--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  endforeach
  foreach pregame_mode : ['ffa', 'ca']
    test('fnql-cgame-retail-pregame-' + pregame_mode, python,
      args: cgame_probe_args + ['--pregame-hud', pregame_mode],
      depends: cgame, suite: 'cgame-hosting', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-pregame-' + pregame_mode, python,
      args: cgame_probe_args + ['--pregame-hud', pregame_mode, '--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-hosting', timeout: 120, is_parallel: false)
  endforeach
  test('fnql-cgame-retail-combat', python, args: cgame_probe_args + ['--combat', '--timeout', '150'],
    depends: cgame, suite: 'cgame-combat', timeout: 180, is_parallel: false)
  test('fnql-cgame-fnql-combat', python,
    args: cgame_probe_args + ['--combat', '--timeout', '150', '--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-combat', timeout: 180, is_parallel: false)
  test('fnql-cgame-retail-match', python, args: cgame_probe_args + ['--match-end'],
    depends: cgame, suite: 'cgame-match', timeout: 120, is_parallel: false)
  test('fnql-cgame-fnql-match', python,
    args: cgame_probe_args + ['--match-end', '--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-match', timeout: 120, is_parallel: false)
  test('fnql-cgame-retail-overtime', python, args: cgame_probe_args + ['--overtime'],
    depends: cgame, suite: 'cgame-match', timeout: 120, is_parallel: false)
  test('fnql-cgame-fnql-overtime', python,
    args: cgame_probe_args + ['--overtime', '--qagame', qagame.full_path()],
    depends: [cgame, qagame], suite: 'cgame-match', timeout: 120, is_parallel: false)
  foreach result : ['draw', 'survivor']
    round_args = cgame_probe_args + ['--round-result', result, '--map', 'thunderstruck']
    test('fnql-cgame-retail-round-' + result, python, args: round_args,
      depends: cgame, suite: 'cgame-match', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-round-' + result, python,
      args: round_args + ['--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-match', timeout: 120, is_parallel: false)
  endforeach
  foreach score_mode : ['ffa', 'tdm']
    test('fnql-cgame-retail-stats-' + score_mode, python,
      args: cgame_probe_args + ['--stats', '--factory', score_mode],
      depends: cgame, suite: 'cgame-stats', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-stats-' + score_mode, python,
      args: cgame_probe_args + ['--stats', '--factory', score_mode, '--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-stats', timeout: 120, is_parallel: false)
    test('fnql-cgame-retail-scoreboard-' + score_mode, python,
      args: cgame_probe_args + ['--scoreboard', '--reload-hud', '--factory', score_mode],
      depends: cgame, suite: 'cgame-scoreboard', timeout: 120, is_parallel: false)
    test('fnql-cgame-fnql-scoreboard-' + score_mode, python,
      args: cgame_probe_args + ['--scoreboard', '--reload-hud', '--factory', score_mode,
        '--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-scoreboard', timeout: 120, is_parallel: false)
  endforeach
  foreach provider : ['retail', 'fnql']
    race_args = cgame_probe_args + ['--race']
    if provider == 'fnql'
      race_args += ['--qagame', qagame.full_path()]
    endif
    test('fnql-cgame-' + provider + '-race', python, args: race_args,
      depends: [cgame, qagame], suite: 'cgame-race-hosting', timeout: 120, is_parallel: false)
    test('fnql-cgame-' + provider + '-race-markers', python, args: race_args + ['--race-markers'],
      depends: [cgame, qagame], suite: 'cgame-race-hosting', timeout: 120, is_parallel: false)
  endforeach
  foreach effect : ['thaw', 'damage-through', 'frozen', 'poi']
    effect_args = cgame_probe_args + ['--' + effect]
    effect_timeout = 120
    if effect in ['thaw', 'frozen']
      # Keep the local client in the same PVS as thawing bots.
      effect_args += ['--map', 'thunderstruck']
    endif
    if effect == 'frozen'
      # The probe waits for bots to kill the local player, which has taken
      # just over the probe's 90 s default on some runs.
      effect_args += ['--timeout', '150']
      effect_timeout = 180
    endif
    test('fnql-cgame-retail-' + effect, python, args: effect_args,
      depends: cgame, suite: 'cgame-effects-hosting', timeout: effect_timeout, is_parallel: false)
    test('fnql-cgame-fnql-' + effect, python,
      args: effect_args + ['--qagame', qagame.full_path()],
      depends: [cgame, qagame], suite: 'cgame-effects-hosting', timeout: effect_timeout, is_parallel: false)
  endforeach
  probe_args = [files('scripts/listen_probe.py'),
    '--client', get_option('fnql-client'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    probe_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-cgame-probe', python, args: probe_args + ['--expect-gametype', '0'], depends: qagame,
    suite: 'interop', timeout: 240, is_parallel: false)
  # Gate G5, FnQL <-> FnQL (milestone M10): an FnQL client joins an FnQL
  # dedicated server over UDP, pure and unpure.  It checks which cgame and ui
  # the client ran, that a pure server accepts an FnQL-game cgame only from
  # the client package it lists in sv_fnqlClientPaks (and the retail cgame
  # otherwise), and the fnql_game / fnql_cg negotiation and handshake.  A
  # server whose Steam provider demands a ticket the client cannot get makes
  # the probe skip (77): that is the server's authorization decision.
  if get_option('fnql-dedicated') != ''
    package_args = [files('scripts/client_package_probe.py'),
      '--engine', get_option('fnql-dedicated'), '--client', get_option('fnql-client'),
      '--module', qagame.full_path(), '--cgame', cgame.full_path(), '--ui', ui.full_path(),
      '--package', client_package.full_path()]
    if get_option('retail-ql') != ''
      package_args += ['--retail', get_option('retail-ql')]
    endif
    foreach row, o : {
        'pure-package': ['--pure', '1', '--server-package', '--expect-cgame', 'fnql', '--expect-ui', 'fnql',
                         '--expect-package', '--expect-handshake', 'yes'],
        'pure-auto': ['--pure', '1', '--server-package', '--cgame-provider', 'auto', '--ui-provider', 'auto',
                      '--expect-cgame', 'fnql', '--expect-ui', 'fnql', '--expect-package', '--expect-handshake', 'yes'],
        'pure-no-package': ['--pure', '1', '--expect-cgame', 'retail', '--expect-ui', 'retail',
                            '--expect-handshake', 'no'],
        'pure-refused': ['--pure', '1', '--server-package', '--server-client-modules', '0',
                         '--expect-cgame', 'retail', '--expect-ui', 'retail', '--expect-handshake', 'no'],
        'pure-loose': ['--pure', '1', '--server-package', '--client-modules', 'loose',
                       '--expect-cgame', 'retail', '--expect-ui', 'retail', '--expect-handshake', 'no'],
        'pure-retail-qagame': ['--pure', '1', '--server-package', '--server-provider', 'retail',
                               '--expect-cgame', 'fnql', '--expect-ui', 'fnql', '--expect-package',
                               '--expect-handshake', 'no'],
        'pure-retail-client': ['--pure', '1', '--server-package', '--cgame-provider', 'retail',
                               '--ui-provider', 'retail', '--expect-cgame', 'retail', '--expect-ui', 'retail',
                               '--expect-handshake', 'no'],
        'unpure-loose': ['--pure', '0', '--client-modules', 'loose', '--expect-cgame', 'fnql',
                         '--expect-ui', 'fnql', '--expect-handshake', 'yes'],
        'unpure-package': ['--pure', '0', '--expect-cgame', 'fnql', '--expect-ui', 'fnql',
                           '--expect-handshake', 'yes'],
        'unpure-retail-qagame': ['--pure', '0', '--server-provider', 'retail', '--expect-cgame', 'fnql',
                                 '--expect-ui', 'fnql', '--expect-handshake', 'no'],
        'unpure-retail-client': ['--pure', '0', '--cgame-provider', 'retail', '--ui-provider', 'retail',
                                 '--expect-cgame', 'retail', '--expect-ui', 'retail', '--expect-handshake', 'no']}
      test('fnql-client-' + row, python, args: package_args + o,
        depends: [qagame, cgame, ui, client_package], suite: 'interop-fnql',
        timeout: 200, is_parallel: false)
    endforeach
  endif
  # Every qagame base mode must publish a gamestate the retail cgame can
  # initialize and render. These startup probes supplement full-server G3;
  # they do not replace the separately authenticated retail-client matrix.
  foreach mode, options : {
      'duel': ['campgrounds', '1'], 'race': ['campgrounds', '2'], 'tdm': ['campgrounds', '3'],
      'ca': ['campgrounds', '4'], 'ctf': ['duelingkeeps', '5'], 'har': ['duelingkeeps', '8'],
      'ft': ['campgrounds', '9'], 'dom': ['courtyard', '10'], 'ad': ['duelingkeeps', '11'],
      'rr': ['campgrounds', '12']}
    test('retail-cgame-mode-' + mode, python,
      args: probe_args + ['--factory', mode, '--map', options[0], '--expect-gametype', options[1]], depends: qagame,
      suite: 'interop-modes', timeout: 240, is_parallel: false)
  endforeach
  foreach mode, options : {'oneflag': ['oneflag', '6'], 'overload': ['objectives', '7']}
    fixture_probe_args = []
    if mode == 'overload'
      # Retail arena metadata excludes Overload, although the stock BSP has
      # the obelisk entities. Use the explicit developer-map fixture path.
      fixture_probe_args += ['--cheats']
    endif
    test('retail-cgame-mode-' + mode, python,
      args: probe_args + fixture_probe_args + ['--factory', 'fnql_test_' + mode, '--map', 'duelingkeeps',
        '--expect-gametype', options[1],
        '--factories', files('tests/lockstep/factories/' + options[0] + '.factories')],
      depends: qagame, suite: 'interop-modes', timeout: 240, is_parallel: false)
  endforeach
  # G5 per mode (milestone M6): the retail cgame plays every round-based and
  # special mode.  The client joins a team, a bot takes the other side, the
  # forced ready-up starts the match through its map_restart, and the live
  # configstrings must show the match in progress, the client on its team
  # and, in the round modes, a round under way (CS_MATCH_STATE 661).  Each
  # entry is the factory, map, gametype, client team, bot team and its extra
  # configstring checks (index, regex pairs).  Every check was taken from the
  # retail qagame, and each mode also runs against it (retail-cgame-play-
  # retail-<mode>, --provider retail) so the expectations stay retail's.
  # The bot's own team is not checked: in Infected and Red Rover it depends
  # on who dies first; the G3 round scenarios cover conversion exactly.
  play_in_progress = ['0', '\\\\g_gameState\\\\IN_PROGRESS(\\\\|$)']
  play_round = ['661', '\\\\round\\\\[1-9]']
  play_turn = ['661', '\\\\turn\\\\[0-9]+\\\\state\\\\[1-9]']
  play_red = ['529', '\\\\t\\\\1\\\\']
  play_free = ['529', '\\\\t\\\\0\\\\']
  foreach mode, o : {
      'ca': ['ca', 'campgrounds', '4', 'r', 'blue', play_red + play_round],
      'vca': ['vca', 'campgrounds', '4', 'r', 'blue', play_red + play_round],
      'ft': ['ft', 'campgrounds', '9', 'r', 'blue', play_red + play_round],
      'ift': ['ift', 'campgrounds', '9', 'r', 'blue', play_red + play_round],
      'rr': ['rr', 'campgrounds', '12', 'r', 'blue', play_round],
      'infected': ['infected', 'campgrounds', '12', 'r', 'blue', play_round],
      'ad': ['ad', 'duelingkeeps', '11', 'r', 'blue', play_red + play_turn],
      'dom': ['dom', 'courtyard', '10', 'r', 'blue', play_red],
      'har': ['har', 'duelingkeeps', '8', 'r', 'blue', play_red],
      'race': ['race', 'campgrounds', '2', 'f', '', play_free],
      'iffa': ['iffa', 'campgrounds', '0', 'f', '', play_free],
      'ictf': ['ictf', 'duelingkeeps', '5', 'r', 'blue', play_red],
      'quadhog': ['quadhog', 'campgrounds', '0', 'f', '', play_free]}
    play_args = probe_args + ['--factory', o[0], '--map', o[1], '--expect-gametype', o[2],
      '--expect-factory', o[0], '--play', '25', '--team', o[3]]
    if o[4] != ''
      play_args += ['--bot-team', o[4]]
    endif
    checks = play_in_progress + o[5]
    foreach index : range(0, checks.length(), 2)
      play_args += ['--expect-cs', checks[index], checks[index + 1]]
    endforeach
    test('retail-cgame-play-' + mode, python, args: play_args, depends: qagame,
      suite: 'interop-play', timeout: 300, is_parallel: false)
    test('retail-cgame-play-retail-' + mode, python, args: play_args + ['--provider', 'retail'],
      depends: qagame, suite: 'interop-play', timeout: 300, is_parallel: false)
  endforeach
  overload_play_args = probe_args + ['--cheats', '--factory', 'fnql_test_overload', '--map', 'duelingkeeps',
    '--factories', files('tests/lockstep/factories/objectives.factories'),
    '--expect-gametype', '7', '--expect-factory', 'fnql_test_overload', '--play', '25',
    '--team', 'r', '--bot-team', 'blue',
    '--expect-cs'] + play_in_progress + ['--expect-cs'] + play_red
  test('retail-cgame-play-overload', python, args: overload_play_args,
    depends: qagame, suite: 'interop-play', timeout: 300, is_parallel: false)
  test('retail-cgame-play-retail-overload', python, args: overload_play_args + ['--provider', 'retail'],
    depends: qagame, suite: 'interop-play', timeout: 300, is_parallel: false)

  # Gate G4a: the retail cgame's prediction of the local player must agree
  # with FnQL-game's server-side Pmove exactly (no "prediction error" at
  # all), with VQL and with PQL movement and its per-player options.
  prediction_args = [files('scripts/prediction_probe.py'),
    '--client', get_option('fnql-client'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    prediction_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-cgame-prediction', python, args: prediction_args, depends: qagame,
    suite: 'interop', timeout: 300, is_parallel: false)
  test('retail-cgame-prediction-pql', python,
    args: prediction_args + ['--set', 'pmove_AirControl', '1', '--set', 'pmove_CrouchSlide', '1',
      '--set', 'pmove_DoubleJump', '1'],
    depends: qagame, suite: 'interop', timeout: 300, is_parallel: false)

  # Gate G4 (live part): FnQL-game's cgame predicts the retail and FnQL-game
  # servers with no prediction error, VQL and PQL, on campgrounds and bloodrun.
  pql_args = ['--set', 'pmove_AirControl', '1', '--set', 'pmove_CrouchSlide', '1',
    '--set', 'pmove_DoubleJump', '1']
  foreach provider : ['retail', 'fnql']
    foreach map : ['campgrounds', 'bloodrun']
      foreach physics : ['vql', 'pql']
        g4_args = prediction_args + ['--cgame', cgame.full_path(), '--map', map, '--max-misses', '0']
        if provider == 'retail'
          g4_args += ['--retail-qagame']
        endif
        if physics == 'pql'
          g4_args += pql_args
        endif
        test('fnql-cgame-prediction-' + provider + '-' + map + '-' + physics, python, args: g4_args,
          depends: [qagame, cgame], suite: 'cgame-prediction', timeout: 300, is_parallel: false)
      endforeach
    endforeach
  endforeach
endif

summary({
  'target': host_machine.system() + ' ' + host_machine.cpu_family(),
  'qagame': fs.name(qagame.full_path()),
}, section: 'FnQL-game')

# Every test runs unattended (AGENTS.md headless rule): FnQL clients start
# windowless (com_headless 2, which still renders and captures) and dedicated
# servers headless, unless a test passes +set com_headless itself.
add_test_setup('headless', env: {'FNQL_HEADLESS': '2'}, is_default: true)
