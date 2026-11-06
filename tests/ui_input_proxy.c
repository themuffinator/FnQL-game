/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
/*
 * Test-only input proxy for the UI interaction gate. Loaded by the engine in
 * place of a UI module, it loads the module named by FNQL_UI_PROXY_TARGET
 * (the installed retail uix86.dll or FnQL-game's) and forwards every import
 * and export unchanged, except that the console-command export first accepts
 * three probe commands, so both UIs receive identical, frame-scheduled input:
 *
 *   pxmouse <x> <y>         absolute drawable position (export 3)
 *   pxkey <key> <0|1>       key up/down (export 2, character flag 0)
 *   pxchar <byte>           character event (export 2, character flag 1)
 *   pxcatcher <mask>        set the key catcher through import 44
 *   pxclock <step>          from now on, give the refresh and console exports
 *                           a synthetic time that starts at 100000 ms and
 *                           advances <step> ms per refresh, so time-based
 *                           pulses, blinks and transitions match between
 *                           runs; 0 restores the engine time
 *
 * It never ships: Meson builds it only for the ui-hosting tests.
 */
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define UI_IMPORTS 97
#define UI_EXPORTS 14

typedef void (__cdecl *entry_t)(void **table, const void *imports, int *version);
typedef int (__cdecl *argc_t)(void);
typedef void (__cdecl *argv_t)(int n, char *buffer, int size);
typedef void (__cdecl *catcher_t)(int catcher);
typedef void (__cdecl *key_t)(int key, int character, int down);
typedef void (__cdecl *mouse_t)(int x, int y);
typedef int (__cdecl *console_t)(int realTime);
typedef void (__cdecl *refresh_t)(int realTime);

static void *imports[UI_IMPORTS];
static void *wrapped[UI_EXPORTS];
static void *exports[UI_EXPORTS];
static int clockStep, clockNow;

static int ProxyTime(int realTime) { return clockStep > 0 ? clockNow : realTime; }

static void __cdecl ProxyRefresh(int realTime) {
    if (clockStep > 0) clockNow += clockStep;
    ((refresh_t)wrapped[4])(ProxyTime(realTime));
}

static int Argument(int n) {
    char buffer[64];
    ((argv_t)imports[12])(n, buffer, sizeof(buffer));
    return atoi(buffer);
}

static int __cdecl ProxyConsole(int realTime) {
    char command[64];
    if (((argc_t)imports[11])() >= 2) {
        ((argv_t)imports[12])(0, command, sizeof(command));
        if (!_stricmp(command, "pxmouse") && ((argc_t)imports[11])() >= 3) {
            ((mouse_t)wrapped[3])(Argument(1), Argument(2));
            return 1;
        }
        if (!_stricmp(command, "pxkey") && ((argc_t)imports[11])() >= 3) {
            ((key_t)wrapped[2])(Argument(1), 0, Argument(2) != 0);
            return 1;
        }
        if (!_stricmp(command, "pxchar")) {
            ((key_t)wrapped[2])(Argument(1) & 0xff, 1, 1);
            return 1;
        }
        if (!_stricmp(command, "pxcatcher")) {
            ((catcher_t)imports[44])(Argument(1));
            return 1;
        }
        if (!_stricmp(command, "pxclock")) {
            clockStep = Argument(1) > 0 ? Argument(1) : 0;
            clockNow = 100000;
            return 1;
        }
    }
    return ((console_t)wrapped[7])(ProxyTime(realTime));
}

__declspec(dllexport) void __cdecl dllEntry(void **table, const void *engineImports, int *version) {
    char path[MAX_PATH];
    void **table_out = NULL;
    HMODULE target;
    entry_t entry;
    int i;
    if (table) *table = NULL;
    if (!engineImports || !GetEnvironmentVariableA("FNQL_UI_PROXY_TARGET", path, sizeof(path))) return;
    target = LoadLibraryA(path);
    entry = target ? (entry_t)GetProcAddress(target, "dllEntry") : NULL;
    if (!entry) return;
    memcpy(imports, engineImports, sizeof(imports));
    /* The wrapped module keeps the engine's own import table. */
    entry((void **)&table_out, engineImports, version);
    if (!table_out) return;
    for (i = 0; i < UI_EXPORTS; ++i) wrapped[i] = exports[i] = table_out[i];
    exports[4] = (void *)ProxyRefresh;
    exports[7] = (void *)ProxyConsole;
    if (table) *table = exports;
}
