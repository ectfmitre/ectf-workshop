#ifndef __DECODER_H__
#define __DECODER_H__

#include <memory>
#include <optional>
#include <string_view>

#include "buffer.h"
#include "channel.h"
#include "crypto.h"
#include "secrets.h"
#include "types.h"

namespace ectf {

// High-level class that implements the secure decoder functionality.
// Handles commands received over UART.
class Decoder {
private:
	// Stores information about known channels including subscription
	// information and keys.
	std::unique_ptr<ChannelData> channel_data_;

	// Tries to process an encrypted subscription and update channel data,
	// returning true on success. If save_to_flash is true, also writes
	// the encrypted subscription message to flash.
	bool ProcessSubscriptionData(std::string_view data, const SecretData& secrets,
			bool save_to_flash);
	// Tries to decode a frame, returning the decoded value on success.
	std::optional<SecureString> TryDecodeFrame(std::string_view data);

	// Processes a List command and returns a response over UART.
	// The response (opcode L) will include the number of channels that the device
	// has ever been subscribed to, followed by the most recent subscription
	// information for each channel (channel ID, start time, end time).
	void ListChannels();

	// Processes a Subscribe command payload and returns a response over UART.
	// If the subscription is valid and the maximum number of subscribed channels
	// (8) has not been reached, the subscription is stored persistently and
	// a zero-length response with opcode S will be sent back.
	// Otherwise, a zero-length response with opcode E will be sent.
	void UpdateSubscription(std::string_view data);

	// Processes a Decode command payload and returns a response over UART.
	// If the decoder has an active subscription for the frame's channel, the
	// frame is valid (can be decrypted and verified), and the frame timestamp
	// is monotonically increasing, a response with opcode D will be sent back,
	// containing the decrypted frame data.
	// Otherwise, a zero-length response with opcode E will be sent.
	void DecodeFrame(std::string_view data);
public:
	Decoder() {}
	~Decoder() {}
	// Performs boot-time initialization. Channel 0 will be initialized using
	// keys stored in SecretData and all subscriptions stored in flash will
	// be loaded.
	void Initialize();
	// Listens for and processes commands over UART. This function never returns.
	void RunLoop();
};

}

#endif // __DECODER_H__
