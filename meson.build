project('fnql-game', 'c',
  version: '0.1.0',  # canonical value lives in version/fnql_game_version.h
  meson_version: '>=1.1.0',
  default_options: [
    'buildtype=release',
    'warning_level=1',
    'b_vscrt=static_from_buildtype',
  ])

# FnQL-game builds drop-in replacements for the retail Quake Live game modules.
# FnQL is currently 32-bit x86 only (its AGENTS.md constraint 9); further
# targets are enabled one at a time through its Project Plan step 7 and
# FnQL-game milestone 11, each behind its own gates.

cc = meson.get_compiler('c')
fs = import('fs')

x86_target_probe = '''
#if !defined(_M_IX86) && !defined(__i386__)
#error FnQL-game requires an x86 target
#endif
typedef char fnql_game_pointer_width_must_be_32_bits[(sizeof(void *) == 4) ? 1 : -1];
int main(void) { return 0; }
'''
if host_machine.cpu_family() != 'x86' or not cc.compiles(x86_target_probe, name: '32-bit x86 target')
  error('FnQL-game supports only 32-bit x86 targets; Meson configured host CPU family "'
    + host_machine.cpu_family() + '". Use a vcvars32 (Win32) environment on Windows or an i686 cross file elsewhere.')
endif

win32 = host_machine.system() == 'windows'
linux = host_machine.system() == 'linux'
if not (win32 or linux)
  error('FnQL-game targets the platforms retail Quake Live ships game modules for: Windows and Linux.')
endif

# Retail module names carry the engine's ARCH_STRING: x86 on Windows, i386 on
# Linux.  FnQL-game prefixes "fnql_" so its modules never shadow the retail
# ones in a search path; the FnQL engine selects them by provider.
module_arch = win32 ? 'x86' : 'i386'
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
  if host_machine.cpu_family() == 'x86'
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
  'code/game/g_names.c',
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
    'code/cgame/cg_duelhud.c',
    'code/cgame/cg_teamstats.c',
    'code/cgame/cg_itemtimers.c',
    'code/cgame/cg_teamvitals.c',
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
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'])
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
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'])
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
  c_args: c_args + ['-DCGAME', '-DMISSIONPACK'])
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
endif

# Command policy and state transitions, without retail assets or a server.
game_drops = executable('game_drops', files('tests/game_drops.c', 'code/game/g_drop.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-drops', game_drops, suite: 'unit')
game_flagphysics = executable('game_flagphysics', files('tests/game_flagphysics.c', 'code/game/g_flagphysics.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-flagphysics', game_flagphysics, suite: 'unit')
game_runes = executable('game_runes', files('tests/game_runes.c', 'code/game/g_runes.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-runes', game_runes, suite: 'unit')
game_names = executable('game_names', files('tests/game_names.c', 'code/game/g_names.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-names', game_names, suite: 'unit')
game_session = executable('game_session', files('tests/game_session.c', 'code/game/g_session.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-session', game_session, suite: 'unit')
game_laghax = executable('game_laghax', files('tests/game_laghax.c', 'code/game/g_laghax.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-laghax', game_laghax, suite: 'unit')
game_complaint = executable('game_complaint', files('tests/game_complaint.c', 'code/game/g_complaint.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-complaint', game_complaint, suite: 'unit')
game_quadhog = executable('game_quadhog', files('tests/game_quadhog.c', 'code/game/g_quadhog.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-quadhog', game_quadhog, suite: 'unit')

game_domination = executable('game_domination', files('tests/game_domination.c', 'code/game/g_domination.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-domination', game_domination, suite: 'unit')

game_admin = executable('game_admin', files('tests/game_admin.c', 'code/game/g_admin.c', 'code/game/g_access.c',
  'code/game/g_commandextras.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-admin', game_admin, suite: 'unit')
game_race = executable('game_race', files('tests/game_race.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-race', game_race, suite: 'unit')


game_voting = executable('game_voting', files('tests/game_voting.c', 'code/game/g_vote.c',
  'code/game/q_shared.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-voting', game_voting, suite: 'unit')

# Gate G0: shared structure layouts must match the FnQL engine headers.
python = find_program('python3', 'python')
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

  # Every stock map must load (G1 sweep); slow, so it is its own suite.
  sweep_args = [files('scripts/sweep_maps.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    sweep_args += ['--retail', get_option('retail-ql')]
  endif
  test('map-sweep', python, args: sweep_args, depends: qagame,
    suite: 'sweep', timeout: 1200, is_parallel: false)

  # Gate G5b: the user's retail Steam client joins this server and stays in.
  # It launches the retail game through Steam, so it is its own opt-in suite:
  # meson test --suite retail-client.  Skips without Steam running.
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
    'team-join': ['tdm_team_join', '--factory', 'tdm', '--frames', '800', '--set', 'teamsize', '2'],
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
endif

# Gate G5a: the retail cgame runs against FnQL-game's qagame in one client.
if get_option('fnql-client') != ''
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
