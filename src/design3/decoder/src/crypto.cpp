#include "crypto.h"

#include <cstring>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>

#include "wolfssl/wolfcrypt/chacha20_poly1305.h"
#include "wolfssl/wolfcrypt/ed25519.h"

#include "buffer.h"
#include "channel.h"
#include "debug.h"
#include "keys.h"
#include "rand.h"

namespace ectf {

std::optional<SecureString> ChaChaCrypt::Decrypt(std::string_view ciphertext,
		const ChaChaKey& key, const ChaChaIV& iv, const ChaChaTag& auth_tag) {
	std::string decoy_key1(CHACHA_KEY_SIZE, ' ');
	std::string decoy_key2(CHACHA_KEY_SIZE, ' ');
	Rand::FastRandomBuffer(decoy_key1.data(), CHACHA_KEY_SIZE);
	Rand::FastRandomBuffer(decoy_key2.data(), CHACHA_KEY_SIZE);
	std::string decoy_output(ciphertext.size(), ' ');
	SecureString output(ciphertext.size());

	// Perform decoy operations immediately before and after the real decryption
	// in order to mitigate power analysis
	wc_ChaCha20Poly1305_Decrypt((const byte*) decoy_key1.data(),
			(const byte*) iv.data(), nullptr, 0, (const byte*) ciphertext.data(),
			ciphertext.size(), (const byte*) auth_tag.data(),
			(byte*) decoy_output.data());
	int retcode = wc_ChaCha20Poly1305_Decrypt((const byte*) key.data(),
			(const byte*) iv.data(), nullptr, 0, (const byte*) ciphertext.data(),
			ciphertext.size(), (const byte*) auth_tag.data(), (byte*) output.data());
	wc_ChaCha20Poly1305_Decrypt((const byte*) decoy_key2.data(),
			(const byte*) iv.data(), nullptr, 0, (const byte*) ciphertext.data(),
			ciphertext.size(), (const byte*) auth_tag.data(),
			(byte*) decoy_output.data());
	if (retcode != 0) {
		return std::nullopt;
	}
	return output;
}

bool EdCrypt::VerifySignature(std::string_view message, const EdPublicKey& key,
		const EdSignature& signature) {
	// Initialize the WolfCrypt key object
	auto wc_key = std::make_unique<ed25519_key>();
	wc_ed25519_init(wc_key.get());
	int retcode = wc_ed25519_import_public((const byte*) key.data(), key.size(),
			wc_key.get());
	Debug::Assert(retcode == 0, "Failed to load public key");

	int is_valid = 0;
	retcode = wc_ed25519_verify_msg((const byte*) signature.data(),
			signature.size(), (const byte*) message.data(), message.size(), &is_valid,
			wc_key.get());

	// Destruct and erase WolfCrypt key object
	wc_ed25519_free(wc_key.get());
	std::memset(wc_key.get(), 0, sizeof(ed25519_key));
	return retcode == 0 && is_valid;
}

}  // namespace ectf
