#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include "buffer.h"
#include "keys.h"

namespace ectf {

// Utility class for decrypting ciphertext using ChaCha20-Poly1305.
class ChaChaCrypt {
public:
	// Returns the plaintext after decrypting the given ciphertext using the
	// provided key and initialization vector, or nothing if decryption failed
	// due to authentication tag mismatch.
	// The returned buffer will have the same length as the ciphertext in case
	// of success.
	// In order to make side-channel analysis more difficult, this function
	// performs two additional decryption operations (decoys) using randomly
	// generated keys, before and after the actual decryption operation.
	static std::optional<SecureString> Decrypt(std::string_view ciphertext,
			const ChaChaKey& key, const ChaChaIV& iv, const ChaChaTag& auth_tag);
};

// Utility class for verifying messages signed with Ed25519.
class EdCrypt {
public:
	// Returns true if the given message's signature was generated using the
	// private Ed25519 key associated with the given public Ed25519 key.
	// The key must be in "raw" format (32 bytes).
	static bool VerifySignature(std::string_view message, const EdPublicKey& key,
			const EdSignature& signature);
};

}

#endif // __CRYPTO_H__
