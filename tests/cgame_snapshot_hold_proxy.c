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
 * Test-only snapshot hold for the loading-screen gate. Loaded by the engine
 * in place of a cgame module, it loads the module named by
 * FNQL_CGAME_PROXY_TARGET (the retail cgamex86.dll or FnQL-game's) and
 * forwards every import and export, except that while the hold is on the
 * wrapped module's snapshot-number import (slot 86) reports no snapshot. The
 * module then keeps drawing its own loading screen in its "Awaiting
 * Snapshot" state during ordinary frames, where an engine screenshot can
 * capture it. The hold starts on; the console command
 *
 *   pxholdsnap <0|1>
 *
 * turns it off or on. It never ships: Meson builds it only for the
 * loading-screen gate.
 */
#include <windows.h>
#include <string.h>

#define CGAME_IMPORTS 128
#define CGAME_EXPORTS 21
#define IMPORT_ARGC 11
#define IMPORT_ARGV 12
#define IMPORT_SNAPSHOT_NUMBER 86
#define EXPORT_CONSOLE_COMMAND 3

typedef void (__cdecl *entry_t)(void **table, const void *imports, int *version);
typedef int (__cdecl *argc_t)(void);
typedef void (__cdecl *argv_t)(int n, char *buffer, int size);
typedef void (__cdecl *snapshot_number_t)(int *snapshotNumber, int *serverTime);
typedef int (__cdecl *console_t)(void);

static void *imports[CGAME_IMPORTS];
static void *wrapped[CGAME_EXPORTS];
static void *exports[CGAME_EXPORTS];
static int hold = 1;

static void __cdecl ProxySnapshotNumber(int *snapshotNumber, int *serverTime) {
    if (hold) {
        if (snapshotNumber) *snapshotNumber = 0;
        if (serverTime) *serverTime = 0;
        return;
    }
    ((snapshot_number_t)imports[IMPORT_SNAPSHOT_NUMBER])(snapshotNumber, serverTime);
}

static int __cdecl ProxyConsole(void) {
    char command[32], value[16];
    if (((argc_t)imports[IMPORT_ARGC])() >= 2) {
        ((argv_t)imports[IMPORT_ARGV])(0, command, sizeof(command));
        if (!_stricmp(command, "pxholdsnap")) {
            ((argv_t)imports[IMPORT_ARGV])(1, value, sizeof(value));
            hold = value[0] != '0';
            return 1;
        }
    }
    return wrapped[EXPORT_CONSOLE_COMMAND] ? ((console_t)wrapped[EXPORT_CONSOLE_COMMAND])() : 0;
}

__declspec(dllexport) void __cdecl dllEntry(void **table, const void *engineImports, int *version) {
    static void *proxied[CGAME_IMPORTS];
    char path[MAX_PATH];
    void **table_out = NULL;
    HMODULE target;
    entry_t entry;
    int i;
    if (table) *table = NULL;
    if (!engineImports || !GetEnvironmentVariableA("FNQL_CGAME_PROXY_TARGET", path, sizeof(path))) return;
    target = LoadLibraryA(path);
    entry = target ? (entry_t)GetProcAddress(target, "dllEntry") : NULL;
    if (!entry) return;
    memcpy(imports, engineImports, sizeof(imports));
    memcpy(proxied, engineImports, sizeof(proxied));
    proxied[IMPORT_SNAPSHOT_NUMBER] = (void *)ProxySnapshotNumber;
    entry((void **)&table_out, proxied, version);
    if (!table_out) return;
    for (i = 0; i < CGAME_EXPORTS; ++i) wrapped[i] = exports[i] = table_out[i];
    exports[EXPORT_CONSOLE_COMMAND] = (void *)ProxyConsole;
    if (table) *table = exports;
}
