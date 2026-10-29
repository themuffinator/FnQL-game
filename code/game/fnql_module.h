/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
//
// fnql_module.h -- module identity contract shared with the FnQL engine.
//
// Every FnQL-game module exports FnQL_GetModuleInfo next to dllEntry.  The
// retail engine never looks the symbol up, so it is invisible there.  FnQL
// uses it to report which provider supplied a module and to refuse automatic
// selection of a module that does not declare the wire profile a session
// needs.  The engine keeps an identical copy of this contract in
// code/qcommon/fnql_module.h; scripts/check_engine_abi.py compares the two.
//
// The structure is append-only: new fields go at the end and size tells the
// engine how much of it the module filled in.
//

#ifndef FNQL_MODULE_H
#define FNQL_MODULE_H

#define FNQL_MODULE_INFO_SYMBOL		"FnQL_GetModuleInfo"
#define FNQL_MODULE_INFO_VERSION	1
#define FNQL_MODULE_PRODUCT			"FnQL-game"

// The module's default profile is bit-for-bit retail on the wire: its
// snapshots, configstrings, server commands and prediction match retail
// Quake Live, so retail peers cannot tell it apart from the retail module.
// Set only once the parity gates in docs/PLAN.md pass for the module.
#define FNQL_MODULE_CAP_RETAIL_WIRE		0x00000001u
// The module negotiates FnQL extensions with capable peers (fnql_game
// serverinfo / userinfo keys) and keeps them away from retail peers.
#define FNQL_MODULE_CAP_EXTENSIONS		0x00000002u

typedef struct fnqlModuleInfo_s {
	int				size;			// sizeof( fnqlModuleInfo_t ) as built by the module
	int				infoVersion;	// FNQL_MODULE_INFO_VERSION
	const char		*product;		// FNQL_MODULE_PRODUCT
	const char		*module;		// "qagame", "cgame" or "ui"
	const char		*version;		// FnQL-game semantic version
	const char		*build;			// free-form build identifier
	unsigned int	capabilities;	// FNQL_MODULE_CAP_* bits
	int				nativeApi;		// native API version the module publishes
} fnqlModuleInfo_t;

typedef const fnqlModuleInfo_t *( QDECL *fnqlGetModuleInfo_t )( void );

#endif // FNQL_MODULE_H
