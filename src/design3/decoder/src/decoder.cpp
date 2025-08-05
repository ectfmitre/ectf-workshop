#include "decoder.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "buffer.h"
#include "channel.h"
#include "crypto.h"
#include "debug.h"
#include "keys.h"
#include "flash.h"
#include "message_bus.h"
#include "rand.h"
#include "secrets.h"
#include "system.h"
#include "timer.h"
#include "types.h"

namespace {

// Delay a random amount of time, 0.5ms on average.
void MicroDelay() {
	ectf::System::Delay(ectf::Rand::FastRandomRange(250, 750));
}

// Estimate number of microseconds needed to send a response with the given
// payload size, including headers and ACKs.
int EstimateIOTime(int size) {
	constexpr int throughput = 11520;
	constexpr int micros_per_byte = (1000000 + throughput / 2) / throughput;
	constexpr int command_header = 4;
	constexpr int response_header = 4;
	constexpr int response_header_ack = 4;
	int response_payload_ack = (size > 0) ? 4 : 0;
	const int num_bytes = size + command_header + response_header
			+ response_header_ack + response_payload_ack;
	return num_bytes * micros_per_byte;
}

}  // namespace

namespace ectf {

void Decoder::Initialize() {
	SecretData secrets;
	MicroDelay();
	secrets.Load();
	channel_data_ = std::make_unique<ChannelData>();
	Channel* channel0 = channel_data_->GetChannel(0);
	Debug::Assert(channel0);
	channel0->SetSubscription(0, -1, secrets.GetChannel0PublicKey(),
			secrets.GetChannel0SymmetricKey());
	for (PageNumber p : channel_data_->GetAllFlashPageNumbers()) {
		std::optional<SecureString> data = FlashStorage::ReadPage(p);
		if (data.has_value()) {
			Debug::Assert(ProcessSubscriptionData(data->GetView(), secrets, false),
					"Failed to load subscription data from flash");
		}
	}
}

bool Decoder::ProcessSubscriptionData(std::string_view data,
		const SecretData& secrets, bool save_to_flash) {
	// Parse the IV, ciphertext, and authentication tag, then perform decryption.
	MicroDelay();
	StringViewReader reader(data);
	std::string_view nonce = reader.ReadNBytes(CHACHA_IV_SIZE);
	int cipher_len = reader.size() - CHACHA_TAG_SIZE;
	if (cipher_len % 16 != 0) return false;
	std::string_view ciphertext = reader.ReadNBytes(cipher_len);
	std::string_view auth_tag = reader.ReadNBytes(CHACHA_TAG_SIZE);
	if (reader.HasError() || reader.size() != 0) return false;

	MicroDelay();
	std::optional<SecureString> plaintext = ChaChaCrypt::Decrypt(ciphertext,
			secrets.GetSubscriptionSymmetricKey(), ChaChaIV(nonce), ChaChaTag(auth_tag));
	if (!plaintext.has_value()) {
		Debug::Print("Decryption failed");
		return false;
	}

	// Parse the subscription data from the plaintext, then perform signature
	// verification.
	reader = plaintext->GetReader();
	const int salt_len = reader.ReadUint8();
	reader.ReadNBytes(salt_len);
	StringViewReader payload_reader = reader;
	std::string_view channel_symmetric_key = reader.ReadNBytes(CHACHA_KEY_SIZE);
	std::string_view channel_public_key = reader.ReadNBytes(ED_PUBLIC_KEY_SIZE);
	DeviceID decoder_id = reader.ReadUint32();
	Timestamp start_time = reader.ReadUint64();
	Timestamp end_time = reader.ReadUint64();
	ChannelID channel_id = reader.ReadUint32();
	const int payload_len = payload_reader.size() - reader.size();
	std::string_view payload = payload_reader.ReadNBytes(payload_len);
	std::string_view signature = reader.ReadNBytes(ED_SIGNATURE_SIZE);
	if (reader.HasError() || payload_reader.HasError()) return false;
	MicroDelay();
	if (!EdCrypt::VerifySignature(payload, secrets.GetSubscriptionPublicKey(),
			EdSignature(signature))) {
		Debug::Print("Signature verification failed");
		return false;
	}

	// Check that the contents are valid.
	if (decoder_id != secrets.GetDecoderID()) {
		Debug::Print("bad device ID");
		return false;
	}
	if (channel_id == 0) {
		Debug::Print("Cannot subscribe to channel 0");
		return false;
	}
	MicroDelay();
	// Repeat checks (anti-glitching countermeasure)
	if (decoder_id != secrets.GetDecoderID() || channel_id == 0) return false;

	// Save the subscription to RAM and flash.
	Channel* channel = channel_data_->GetOrCreateChannel(channel_id);
	if (!channel) {
		Debug::Print("No space for new channel");
		return false;
	}
	channel->SetSubscription(start_time, end_time, EdPublicKey(channel_public_key),
			ChaChaKey(channel_symmetric_key));
	MicroDelay();
	// Repeat subscription assignment (anti-glitching countermeasure)
	channel->SetSubscription(start_time, end_time, EdPublicKey(channel_public_key),
				ChaChaKey(channel_symmetric_key));
	if (save_to_flash) {
		FlashStorage::WritePage(channel->GetFlashPageNumber(), data);
	}
	if (channel_data_->GetLastSeenTime() > end_time) {
		Debug::Print("Subscription valid but expired");
		channel->ClearSubscription();
	}
	return true;
}

std::optional<SecureString> Decoder::TryDecodeFrame(std::string_view data) {
	// Validate the purported channel ID (stored in the payload prefix).
	MicroDelay();
	StringViewReader reader(data);
	const ChannelID channel_id = reader.ReadUint32();
	if (reader.HasError()) return std::nullopt;
	Channel* channel = channel_data_->GetChannel(channel_id);
	if (!channel || !channel->IsActive()) {
		Debug::Print("Bad channel ID");
		return std::nullopt;
	}
	if (channel_data_->GetLastSeenTime() >= channel->GetEndTime()) {
		Debug::Print("Subscription expired (early check)");
		channel->ClearSubscription();
		return std::nullopt;
	}

	// Parse the IV, ciphertext, and authentication tag, then perform decryption.
	std::string_view nonce = reader.ReadNBytes(CHACHA_IV_SIZE);
	int cipher_len = reader.size() - CHACHA_TAG_SIZE;
	if (cipher_len % 16 != 0) return std::nullopt;
	std::string_view ciphertext = reader.ReadNBytes(cipher_len);
	std::string_view auth_tag = reader.ReadNBytes(CHACHA_TAG_SIZE);
	if (reader.HasError()) return std::nullopt;
	MicroDelay();
	std::optional<SecureString> plaintext = ChaChaCrypt::Decrypt(ciphertext,
			channel->GetSymmetricKey(), ChaChaIV(nonce), ChaChaTag(auth_tag));
	if (!plaintext.has_value()) {
		Debug::Print("Decryption failed");
		return std::nullopt;
	}

	// Parse the plaintext content and perform signature verification.
	reader = plaintext->GetReader();
	const int salt_len = reader.ReadUint8();
	reader.ReadNBytes(salt_len);
	StringViewReader payload_reader = reader;
	const ChannelID secure_channel_id = reader.ReadUint32();
	const Timestamp time = reader.ReadUint64();
	const int frame_len = reader.ReadUint8();
	if (reader.HasError()) return std::nullopt;
	if (frame_len > 64) return std::nullopt;
	std::string_view frame = reader.ReadNBytes(frame_len);
	const int payload_len = payload_reader.size() - reader.size();
	std::string_view payload = payload_reader.ReadNBytes(payload_len);
	std::string_view signature = reader.ReadNBytes(ED_SIGNATURE_SIZE);
	if (reader.HasError() || payload_reader.HasError()) return std::nullopt;
	MicroDelay();
	if (!EdCrypt::VerifySignature(payload, channel->GetPublicKey(),
			EdSignature(signature))) {
		Debug::Print("Signature verification failed");
		return std::nullopt;
	}

	// Check security requirements
	if (secure_channel_id != channel_id) return std::nullopt;
	if (time < channel->GetStartTime()) {
		Debug::Print("Timestamp is before subscription start time");
		return std::nullopt;
	}
	if (time > channel->GetEndTime()) {
		Debug::Print("Subscription expired");
		channel->ClearSubscription();
		return std::nullopt;
	}
	if (time <= channel_data_->GetLastSeenTime()) {
		Debug::Print("Timestamp not increasing");
		return std::nullopt;
	}
	MicroDelay();
	// Repeat checks (anti-glitching countermeasure)
	if (secure_channel_id != channel_id || time < channel->GetStartTime()
			|| time > channel->GetEndTime()
			|| time <= channel_data_->GetLastSeenTime())
		return std::nullopt;

	channel_data_->SetLastSeenTime(time);
	return SecureString(frame);
}

void Decoder::ListChannels() {
	std::vector<Channel*> channels = channel_data_->GetNonZeroChannels();
	std::string buf;
	buf += StringCoder::EncodeUint32(channels.size());
	for (Channel* channel : channels) {
		buf += StringCoder::EncodeUint32(channel->GetID());
		buf += StringCoder::EncodeUint64(channel->GetStartTime());
		buf += StringCoder::EncodeUint64(channel->GetEndTime());
	}
	MessageBus::WriteResponse(OpCode::List, buf);
}

void Decoder::UpdateSubscription(std::string_view data) {
	SecretData secrets;
	MicroDelay();
	secrets.Load();
	const bool success = ProcessSubscriptionData(data, secrets, true);
	// Constant-time processing
	MessageBus::GetCommandTimer().WaitUntilElapsedMicros(
			450000 - EstimateIOTime(0));
	if (success) {
		MessageBus::WriteResponse(OpCode::Subscribe, "");
	} else {
		MessageBus::WriteResponse(OpCode::Error, "");
	}
}

void Decoder::DecodeFrame(std::string_view data) {
	std::optional<SecureString> ret = Decoder::TryDecodeFrame(data);
	const int ret_size = ret.has_value() ? ret->size() : 0;
	// Constant-time processing
	MessageBus::GetCommandTimer().WaitUntilElapsedMicros(
			87000 - EstimateIOTime(ret_size));
	if (ret.has_value()) {
		MessageBus::WriteResponse(OpCode::Decode, ret->GetView());
	} else {
		MessageBus::WriteResponse(OpCode::Error, "");
		return;
	}
}

void Decoder::RunLoop() {
	while (true) {
		Debug::SetLedColor(LedColor::Green);
		auto [op_code, body] = MessageBus::ReadCommand();
		switch (op_code) {
			case OpCode::List: {
				ListChannels();
				break;
			}
			case OpCode::Subscribe: {
				UpdateSubscription(body.GetView());
				break;
			}
			case OpCode::Decode: {
				DecodeFrame(body.GetView());
				break;
			}
			default: {
				Debug::SetLedColor(LedColor::White);
				Debug::Print("Received invalid opcode");
				MessageBus::WriteResponse(OpCode::Error, "");
			}
		}
	}
}

}  // namespace ectf
