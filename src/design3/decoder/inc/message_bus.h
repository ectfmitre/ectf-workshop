#ifndef __MESSAGE_BUS_H__
#define __MESSAGE_BUS_H__

#include <string_view>
#include <tuple>

#include "buffer.h"
#include "timer.h"

namespace ectf {

// The maximum possible size of any valid command payload (with 16 bytes added
// as a safety margin).
constexpr int MAX_INPUT_PAYLOAD_SIZE = 208 + 16;

// Enum type describing all possible command types.
enum class OpCode {
	Decode, Subscribe, List,
	Ack, Error, Debug, Unknown
};

// Utility class for sending and receiving messages over UART. Each message
// consists of:
// - 4 byte header = '%' character + opcode character + payload size as 2-byte
//   little endian integer
// - payload, sent in chunks of 256 bytes (except for last chunk)
// The protocol being used requires each header or payload chunk to be ACKed by
// the other side (an ACK is a message with opcode A and length 0).
class MessageBus {
public:
	// Performs boot time initialization, primarily enabling the console UART
	// channel (UART0) at baud 115200.
	static void Initialize();
	// Reads one message from UART and returns the opcode and payload.
	static std::tuple<OpCode, SecureString> ReadCommand();
	// Writes a message with the given opcode and payload to UART.
	static void WriteResponse(OpCode opcode, std::string_view body);
	// Returns a Timer that measures the time since the last command was
	// received (specifically when the header was received).
	static const Timer& GetCommandTimer();
};

}

#endif // __MESSAGE_BUS_H__
