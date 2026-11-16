// FnQL extension negotiation, shared by qagame and cgame.
//
// This is FnQL-game protocol, not retail: retail Quake Live has none of it,
// and a retail peer never sees any of it (docs/COMPATIBILITY.md rule 4).
//
//   1. The server announces itself: the qagame's `fnql_game` serverinfo key
//      (g_main.c), the module version.  Retail clients ignore unknown
//      serverinfo keys.
//   2. The client answers only that announcement: the FnQL-game cgame sets
//      the `fnql_cg` userinfo key to the extension protocol it speaks, and
//      clears it again on shutdown and on any server without `fnql_game`,
//      so a retail server never receives it.
//   3. The server enables extensions for that client alone (its per-client
//      channel): extension commands go only to that client's reliable
//      command stream (trap_SendServerCommand with its own slot), never to
//      -1, a bot or a client that did not answer.
//
// Extension 1, the handshake (FNQL_EXT_HELLO): when a negotiated client
// enters the game the server sends `fnql_hello <protocol> <mask> <version>`
// (numbers in decimal: the modules' formatter has no %x);
// the cgame records the enabled set and answers `fnql_ack <protocol>
// <mask>`, after which the server treats the channel as confirmed.  Later
// extensions take the next bits and travel the same way.

#ifndef BG_FNQL_EXT_H
#define BG_FNQL_EXT_H

#define FNQL_EXT_SERVERINFO_KEY	"fnql_game"
#define FNQL_EXT_USERINFO_KEY	"fnql_cg"

// the extension protocol both sides speak; a peer offering a newer one is
// answered at this one
#define FNQL_EXT_PROTOCOL		1

#define FNQL_EXT_HELLO			0x00000001u
#define FNQL_EXT_SUPPORTED		( FNQL_EXT_HELLO )

#define FNQL_EXT_CMD_HELLO		"fnql_hello"	// server -> client
#define FNQL_EXT_CMD_ACK		"fnql_ack"		// client -> server

// Parses an extension protocol offer ("1"); anything that is not a plain
// decimal number from 1 to 9999 is no offer (0).
static ID_INLINE int BG_FnQLExtParseProtocol( const char *value ) {
	int protocol = 0, digits = 0;

	if ( !value ) {
		return 0;
	}
	for ( ; *value; value++ ) {
		if ( *value < '0' || *value > '9' || ++digits > 4 ) {
			return 0;
		}
		protocol = protocol * 10 + ( *value - '0' );
	}
	return protocol;
}

// Parses an extension mask ("0x1" or decimal); malformed is 0.
static ID_INLINE unsigned BG_FnQLExtParseMask( const char *value ) {
	unsigned mask = 0;
	int base = 10, digits = 0;

	if ( !value ) {
		return 0;
	}
	if ( value[0] == '0' && ( value[1] == 'x' || value[1] == 'X' ) ) {
		base = 16;
		value += 2;
	}
	for ( ; *value; value++ ) {
		int digit;
		if ( *value >= '0' && *value <= '9' ) {
			digit = *value - '0';
		} else if ( base == 16 && *value >= 'a' && *value <= 'f' ) {
			digit = *value - 'a' + 10;
		} else if ( base == 16 && *value >= 'A' && *value <= 'F' ) {
			digit = *value - 'A' + 10;
		} else {
			return 0;
		}
		if ( ++digits > 8 ) {
			return 0;
		}
		mask = mask * (unsigned)base + (unsigned)digit;
	}
	return digits ? mask : 0;
}

#endif // BG_FNQL_EXT_H
