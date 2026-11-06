/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "../code/game/g_local.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static const struct { const char *input, *cleaned; } cases[] = {
	{ "Plain", "Plain" },
	{ "^2Alpha", "^2Alpha^7" },
	{ "^2Alpha^7", "^2Alpha^7" },
	{ "^2Alpha^?", "^2Alpha^?" },
	{ "^7White", "^7White" },
	{ "^0Black", "Black" },
	{ "^8Black", "Black" },
	{ "^2Alpha^0", "^2Alpha^7" },
	{ "^aAlias", "^aAlias^7" },
	{ "^^Caret", "^^Caret^7" },
	{ "Alpha^", "Alpha" },
	{ "^2Alpha^", "^2Alpha^7" },
	{ "^1Red^2Green^7White", "^1Red^2Green^7White" },
	{ "^1Red^2Green", "^1Red^2Green^7" },
	{ "   ^2Alpha    Beta", "^2Alpha   Beta^7" },
	{ "^2   Alpha", "^2   Alpha^7" },
	{ "^2A<B>&C'D\"", "^2ABCD^7" },
	{ "^2Zo\xc3\xab", "^2Zo\xc3\xab^7" },
	{ "\t^2Al\npha\x7f", "^2Alpha^7" },
	{ "^\nAlpha", "Alpha" },
	{ "", "UnnamedPlayer" },
	{ "   ", "UnnamedPlayer" },
	{ "^2", "UnnamedPlayer" },
	{ "^0", "UnnamedPlayer" },
	{ "^2^7", "UnnamedPlayer" },
	{ "^", "UnnamedPlayer" },
	{ NULL, "UnnamedPlayer" },
	{ "^2123456789012345678901234567890123456", "^2123456789012345678901234567890123456^7" },
	{ "12345678901234567890123456789012345678^2Z", "12345678901234567890123456789012345678^2Z^7" }
};

int main( void ) {
	unsigned char guarded[160];
	char longInput[4096], cleaned[1100], alias[80];
	int i, size, byte;
	CHECK(MAX_NETNAME == 40);
	for ( i = 0; i < ARRAY_LEN(cases); ++i ) {
		// Every capacity sees a prefix of the fully normalized name: cleanup
		// and the final bounded copy must not be combined into one pass.
		for ( size = 0; size <= 80; ++size ) {
			size_t count = size ? strlen(cases[i].cleaned) : 0;
			memset(guarded, 0xa5, sizeof(guarded));
			G_CleanPlayerName(cases[i].input, (char *)guarded + 16, size);
			for (byte = 0; byte < 16; ++byte) CHECK(guarded[byte] == 0xa5);
			for (byte = 16 + size; byte < sizeof(guarded); ++byte) CHECK(guarded[byte] == 0xa5);
			if (!size) continue;
			if (count >= (size_t)size) count = size - 1;
			CHECK(!memcmp(guarded + 16, cases[i].cleaned, count));
			CHECK(guarded[16 + count] == '\0');
		}
	}
	memset(guarded, 0xa5, sizeof(guarded));
	G_CleanPlayerName("^2Alpha", (char *)guarded, -1);
	for (byte = 0; byte < sizeof(guarded); ++byte) CHECK(guarded[byte] == 0xa5);
	G_CleanPlayerName("^2Alpha", NULL, 40);
	strcpy(alias, "^2Alias");
	G_CleanPlayerName(alias, alias, sizeof(alias));
	CHECK(!strcmp(alias, "^2Alias^7"));

	// Overlong input cannot overrun the staging buffer or lose its reset.
	memset(longInput, 'x', sizeof(longInput));
	longInput[sizeof(longInput) - 1] = '\0';
	G_CleanPlayerName(longInput, cleaned, sizeof(cleaned));
	CHECK(strlen(cleaned) == MAX_INFO_STRING - 1);
	longInput[0] = '^'; longInput[1] = '2';
	G_CleanPlayerName(longInput, cleaned, sizeof(cleaned));
	CHECK(strlen(cleaned) == MAX_INFO_STRING - 1);
	CHECK(!strcmp(cleaned + strlen(cleaned) - 2, "^7"));
	for (i = 0; i < sizeof(longInput) - 2; i += 2) {
		longInput[i] = '^'; longInput[i + 1] = '2';
	}
	longInput[i] = '\0';
	G_CleanPlayerName(longInput, cleaned, sizeof(cleaned));
	CHECK(!strcmp(cleaned, "UnnamedPlayer"));
	CHECK(G_PlayerNamesEqual("^1Alpha^7", "^2aLPHA"));
	CHECK(!G_PlayerNamesEqual("^aAlpha", "Alpha"));
	CHECK(G_PlayerNamesEqual("^aAlpha", "^AALPHA"));
	CHECK(G_PlayerNamesEqual("Zo\xc3\xab", "zo\xc3\xab"));
	CHECK(!G_PlayerNamesEqual("Zo\xc3\xab", "Zo"));
	CHECK(G_PlayerNamesEqual(NULL, ""));
	memset(longInput, 'x', sizeof(longInput));
	longInput[sizeof(longInput) - 1] = '\0';
	memset(cleaned, 'x', 39); cleaned[39] = '\0';
	CHECK(G_PlayerNamesEqual(longInput, cleaned));
	cleaned[38] = '^'; longInput[38] = '^'; longInput[39] = '2';
	CHECK(G_PlayerNamesEqual(longInput, cleaned)); // truncate before color removal
	puts("game names OK: normalization, uniqueness keys, bounds and oversized input");
	return 0;
}
