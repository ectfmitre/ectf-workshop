#include "secrets.h"

#include <optional>
#include <string_view>

#include "buffer.h"
#include "channel.h"
#include "crypto.h"
#include "debug.h"
#include "keys.h"
#include "types.h"

namespace ectf {

void SecretData::Load() {
	std::string_view flash_key_raw = GetFlashKey();
	std::string_view flash_iv = GetFlashIV();
	std::string_view secret_data = GetFlashSecretData();
	StringViewReader reader(secret_data);
	const int skip_bytes = reader.ReadUint8();
	reader.ReadNBytes(skip_bytes);
	const int ciphertext_len = reader.ReadUint16();
	std::string_view ciphertext = reader.ReadNBytes(ciphertext_len);
	std::string_view auth_tag = reader.ReadNBytes(CHACHA_TAG_SIZE);
	Debug::Assert(!reader.HasError());
	std::optional<SecureString> plaintext = ChaChaCrypt::Decrypt(ciphertext,
			ChaChaKey(flash_key_raw), ChaChaIV(flash_iv), ChaChaTag(auth_tag));
	Debug::Assert(plaintext.has_value(), "Failed to decrypt flash secrets");

	reader = plaintext->GetReader();
	decoder_id_ = reader.ReadUint32();
	channel0_symmetric_key_ = ChaChaKey(reader.ReadNBytes(CHACHA_KEY_SIZE));
	channel0_public_key_ = EdPublicKey(reader.ReadNBytes(ED_PUBLIC_KEY_SIZE));
	subscription_symmetric_key_ = ChaChaKey(reader.ReadNBytes(CHACHA_KEY_SIZE));
	subscription_public_key_ = EdPublicKey(reader.ReadNBytes(ED_PUBLIC_KEY_SIZE));
	Debug::Assert(!reader.HasError());
}

}  // namespace ectf
