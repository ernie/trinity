// bg_hash.h -- Keyed hash for Trinity handshake protocol
// Uses SipHash-2-4 (128-bit output), QVM-safe 32-bit emulation
#ifndef BG_HASH_H
#define BG_HASH_H

#define TRINITY_HASH_HEX_LEN 33  // 32 hex chars + null

// Keyed hash: key is the auth token, message is the nonce.
// outputHex must be >= TRINITY_HASH_HEX_LEN bytes.
void BG_HashKeyed( const char *key, const char *message, char *outputHex );

#endif
