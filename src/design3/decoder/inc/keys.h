#ifndef __KEYS_H__
#define __KEYS_H__

#include "buffer.h"

namespace ectf {

constexpr int CHACHA_KEY_SIZE = 32;
constexpr int CHACHA_IV_SIZE = 12;
constexpr int CHACHA_TAG_SIZE = 16;
constexpr int ED_PUBLIC_KEY_SIZE = 32;
constexpr int ED_SIGNATURE_SIZE = 64;

// Fixed size buffer holding a ChaCha20 key in raw format
using ChaChaKey = SecureFixedBuffer<CHACHA_KEY_SIZE>;
// Fixed size buffer holding a ChaCha20-Poly1305 initialization vector
using ChaChaIV = SecureFixedBuffer<CHACHA_IV_SIZE>;
// Fixed size buffer holding a ChaCha20-Poly1305 authentication tag
using ChaChaTag = SecureFixedBuffer<CHACHA_TAG_SIZE>;
// Fixed size buffer holding an Ed25519 public key in raw format
using EdPublicKey = SecureFixedBuffer<ED_PUBLIC_KEY_SIZE>;
// Fixed size buffer holding an Ed25519 message signature
using EdSignature = SecureFixedBuffer<ED_SIGNATURE_SIZE>;

}

#endif // __KEYS_H__
