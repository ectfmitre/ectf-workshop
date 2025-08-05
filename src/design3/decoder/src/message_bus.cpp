#include "message_bus.h"

#include <cstdint>
#include <string>
#include <string_view>

// from MSDK
#include "board.h"
#include "uart.h"

#include "buffer.h"
#include "debug.h"
#include "system.h"
#include "timer.h"

namespace {

using ectf::Debug;
using ectf::OpCode;
using ectf::SecureString;
using ectf::StringCoder;
using ectf::StringViewReader;
using ectf::Timer;

constexpr int UART_BAUD = 115200;
constexpr int CHUNK_SIZE = 256;
constexpr int MAX_OUTPUT_PAYLOAD_SIZE = 164;

static_assert(ectf::MAX_INPUT_PAYLOAD_SIZE <= CHUNK_SIZE);
static_assert(MAX_OUTPUT_PAYLOAD_SIZE <= CHUNK_SIZE);

mxc_uart_regs_t* console_uart_ = nullptr;

Timer& GetTimer() {
	static Timer timer;
	return timer;
}

OpCode ToOpCode(char c) {
	switch (c) {
	case 'D':
		return OpCode::Decode;
	case 'S':
		return OpCode::Subscribe;
	case 'L':
		return OpCode::List;
	case 'A':
		return OpCode::Ack;
	case 'E':
		return OpCode::Error;
	case 'G':
		return OpCode::Debug;
	default:
		return OpCode::Unknown;
	}
}

char ToChar(OpCode op_code) {
	switch (op_code) {
	case OpCode::Decode:
		return 'D';
	case OpCode::Subscribe:
		return 'S';
	case OpCode::List:
		return 'L';
	case OpCode::Ack:
		return 'A';
	case OpCode::Error:
		return 'E';
	case OpCode::Debug:
		return 'G';
	default:
		return 'E';
	}
}

char ReadCharacter() {
	Debug::Assert(console_uart_);
	int read = MXC_UART_ReadCharacter(console_uart_);
	Debug::Assert(read >= 0 && read <= 255, "ReadCharacter error");
	return read;
}

SecureString ReadNCharacters(int n) {
	std::string ret;
	ret.reserve(n);
	for (int i = 0; i < n; i++) {
		ret.push_back(ReadCharacter());
	}
	return SecureString(ret);
}

void SkipNCharacters(int n) {
	for (int i = 0; i < n; i++) {
		ReadCharacter();
	}
}

// Read a 4-byte header from UART and return the opcode and body length.
std::tuple<OpCode, uint16_t> ReadHeader() {
	while (ReadCharacter() != '%') {}
	SecureString header = ReadNCharacters(3);
	StringViewReader reader = header.GetReader();
	const OpCode op_code = ToOpCode(reader.ReadChar());
	const uint16_t body_length = reader.ReadUint16();
	Debug::Assert(!reader.HasError());
	return {op_code, body_length};
}

bool ReadAck() {
	auto [op_code, length] = ReadHeader();
	return op_code == OpCode::Ack;
}

void WriteBytes(std::string_view data) {
	Debug::Assert(console_uart_);
	for (char c : data) {
		while (console_uart_->status & MXC_F_UART_STATUS_TX_FULL) {
		}
		console_uart_->fifo = (uint8_t) c;
	}
}

void WriteHeader(OpCode opcode, int length) {
	std::string header;
	header += '%';
	header += ToChar(opcode);
	header += StringCoder::EncodeUint16(length);
	WriteBytes(header);
}

void WriteAck() {
	WriteHeader(OpCode::Ack, 0);
}

void WriteDebug(std::string_view body) {
	if (!Debug::IsDebugMode()) return;
	const int length = body.size();
	WriteHeader(OpCode::Debug, length);
	WriteBytes(body);
}

}  // namespace

namespace ectf {

void MessageBus::Initialize() {
	console_uart_ = MXC_UART_GET_UART(CONSOLE_UART);
	int ret = MXC_UART_Init(console_uart_, UART_BAUD, MXC_UART_IBRO_CLK);
	Debug::Assert(ret == E_NO_ERROR, "Error initializing UART");
}

std::tuple<OpCode, SecureString> MessageBus::ReadCommand() {
	auto [op_code, length] = ReadHeader();
	GetTimer().Reset();
	WriteAck();
	if (length == 0) {
		return {op_code, SecureString("")};
	}
	if (length > MAX_INPUT_PAYLOAD_SIZE) {
		// Security optimization: if we get a message with an unexpectedly large
		// payload, just read and discard the bytes then continue processing it
		// as if the payload was empty.
		for (int i = 0; i < length / CHUNK_SIZE; i++) {
			SkipNCharacters(CHUNK_SIZE);
			WriteAck();
		}
		if (length % CHUNK_SIZE > 0) {
		  SkipNCharacters(length % CHUNK_SIZE);
		  WriteAck();
		}
		return {op_code, SecureString("")};
	}
	SecureString body = ReadNCharacters(length);
	WriteAck();
	return {op_code, body};
}

void MessageBus::WriteResponse(OpCode opcode, std::string_view body) {
	if (opcode == OpCode::Debug) {
		WriteDebug(body);
		return;
	}
	const int length = body.size();
	Debug::Assert(length <= MAX_OUTPUT_PAYLOAD_SIZE,
			"WriteResponse data size too large");
	WriteHeader(opcode, length);
	if (!ReadAck()) {
		Debug::Print("did not receive header ACK");
		return;
	}
	if (length > 0) {
		WriteBytes(body);
		if (!ReadAck()) {
			Debug::Print("did not receive data ACK");
			return;
		}
	}
}

const Timer& MessageBus::GetCommandTimer() {
	return GetTimer();
}

}  // namespace ectf
