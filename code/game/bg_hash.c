// bg_hash.c -- SipHash-2-4 for Trinity handshake protocol
// 128-bit output via 32-bit emulation (QVM-safe: no 64-bit types, minimal stack)
// Reference: https://cr.yp.to/siphash/siphash-20120620.pdf

#include "bg_hash.h"

// 64-bit value as pair of 32-bit halves (hi, lo)
typedef struct { unsigned int hi, lo; } u64;

static u64 u64_make( unsigned int hi, unsigned int lo ) { u64 r; r.hi = hi; r.lo = lo; return r; }

static u64 u64_xor( u64 a, u64 b ) { return u64_make( a.hi ^ b.hi, a.lo ^ b.lo ); }

static u64 u64_add( u64 a, u64 b ) {
	u64 r;
	r.lo = a.lo + b.lo;
	r.hi = a.hi + b.hi + ( r.lo < a.lo ? 1 : 0 );
	return r;
}

static u64 u64_rotl( u64 v, int n ) {
	u64 r;
	if ( n == 0 ) return v;
	if ( n == 32 ) return u64_make( v.lo, v.hi );
	if ( n < 32 ) {
		r.hi = ( v.hi << n ) | ( v.lo >> ( 32 - n ) );
		r.lo = ( v.lo << n ) | ( v.hi >> ( 32 - n ) );
	} else {
		n -= 32;
		r.hi = ( v.lo << n ) | ( v.hi >> ( 32 - n ) );
		r.lo = ( v.hi << n ) | ( v.lo >> ( 32 - n ) );
	}
	return r;
}

#define SIPROUND \
	v0 = u64_add( v0, v1 ); v2 = u64_add( v2, v3 ); \
	v1 = u64_rotl( v1, 13 ); v3 = u64_rotl( v3, 16 ); \
	v1 = u64_xor( v1, v0 ); v3 = u64_xor( v3, v2 ); \
	v0 = u64_rotl( v0, 32 ); \
	v2 = u64_add( v2, v1 ); v0 = u64_add( v0, v3 ); \
	v1 = u64_rotl( v1, 17 ); v3 = u64_rotl( v3, 21 ); \
	v1 = u64_xor( v1, v2 ); v3 = u64_xor( v3, v0 ); \
	v2 = u64_rotl( v2, 32 )

// Derive SipHash k0/k1 from a variable-length key string.
// Fold via FNV-1a into 16 bytes to produce two 64-bit key halves.
static void DeriveKey( const char *key, u64 *k0, u64 *k1 ) {
	unsigned int h[4];
	int i;

	h[0] = 0x736f6d65;  // distinct seeds per quarter
	h[1] = 0x646f7261;
	h[2] = 0x6c796765;
	h[3] = 0x74656462;

	for ( i = 0; key[i]; i++ ) {
		h[i & 3] ^= (unsigned char)key[i];
		h[i & 3] *= 0x01000193;
	}

	*k0 = u64_make( h[0], h[1] );
	*k1 = u64_make( h[2], h[3] );
}

static void WriteHex32( unsigned int val, char *out ) {
	int i;
	for ( i = 7; i >= 0; i-- ) {
		out[i] = "0123456789abcdef"[val & 0xf];
		val >>= 4;
	}
}

/*
===============
BG_HashKeyed

SipHash-2-4 with 128-bit output.
Key is derived from the token string, message is the nonce.
Writes 32 hex chars + null to outputHex.
===============
*/
void BG_HashKeyed( const char *key, const char *message, char *outputHex ) {
	u64 v0, v1, v2, v3, k0, k1, m;
	const unsigned char *msg = (const unsigned char *)message;
	int msgLen, blocks, i, left;
	unsigned int mLo, mHi;

	DeriveKey( key, &k0, &k1 );

	v0 = u64_xor( k0, u64_make( 0x736f6d65, 0x70736575 ) );
	v1 = u64_xor( k1, u64_make( 0x646f7261, 0x6e646f6d ) );
	v2 = u64_xor( k0, u64_make( 0x6c796765, 0x6e657261 ) );
	v3 = u64_xor( k1, u64_make( 0x74656462, 0x79746573 ) );

	// 128-bit output tag
	v1 = u64_xor( v1, u64_make( 0, 0xee ) );

	msgLen = 0;
	while ( msg[msgLen] ) msgLen++;

	blocks = msgLen / 8;
	for ( i = 0; i < blocks; i++ ) {
		mLo = (unsigned int)msg[i*8]
			| ( (unsigned int)msg[i*8+1] << 8 )
			| ( (unsigned int)msg[i*8+2] << 16 )
			| ( (unsigned int)msg[i*8+3] << 24 );
		mHi = (unsigned int)msg[i*8+4]
			| ( (unsigned int)msg[i*8+5] << 8 )
			| ( (unsigned int)msg[i*8+6] << 16 )
			| ( (unsigned int)msg[i*8+7] << 24 );
		m = u64_make( mHi, mLo );
		v3 = u64_xor( v3, m );
		SIPROUND; SIPROUND;
		v0 = u64_xor( v0, m );
	}

	// Last block with length byte
	mLo = 0; mHi = 0;
	left = msgLen & 7;
	switch ( left ) {
		case 7: mHi |= (unsigned int)msg[blocks*8+6] << 16; // fall through
		case 6: mHi |= (unsigned int)msg[blocks*8+5] << 8;  // fall through
		case 5: mHi |= (unsigned int)msg[blocks*8+4];        // fall through
		case 4: mLo |= (unsigned int)msg[blocks*8+3] << 24; // fall through
		case 3: mLo |= (unsigned int)msg[blocks*8+2] << 16; // fall through
		case 2: mLo |= (unsigned int)msg[blocks*8+1] << 8;  // fall through
		case 1: mLo |= (unsigned int)msg[blocks*8];
	}
	mHi |= (unsigned int)( msgLen & 0xff ) << 24;
	m = u64_make( mHi, mLo );
	v3 = u64_xor( v3, m );
	SIPROUND; SIPROUND;
	v0 = u64_xor( v0, m );

	// First finalization: 128-bit tag part 1
	v2 = u64_xor( v2, u64_make( 0, 0xee ) );
	SIPROUND; SIPROUND; SIPROUND; SIPROUND;

	{
		u64 hash0 = u64_xor( u64_xor( v0, v1 ), u64_xor( v2, v3 ) );

		// Second finalization: 128-bit tag part 2
		v1 = u64_xor( v1, u64_make( 0, 0xdd ) );
		SIPROUND; SIPROUND; SIPROUND; SIPROUND;

		{
			u64 hash1 = u64_xor( u64_xor( v0, v1 ), u64_xor( v2, v3 ) );

			WriteHex32( hash0.lo, outputHex );
			WriteHex32( hash0.hi, outputHex + 8 );
			WriteHex32( hash1.lo, outputHex + 16 );
			WriteHex32( hash1.hi, outputHex + 24 );
			outputHex[32] = '\0';
		}
	}
}

/*
===============
BG_HashSelfTest

Validate known test vectors. Returns 1 on success, 0 on failure.
These vectors must match the Go implementation in trinity-tracker.
===============
*/
