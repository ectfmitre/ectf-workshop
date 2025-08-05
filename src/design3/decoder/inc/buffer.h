#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "debug.h"

namespace ectf {

// Utility class for reading content from a byte stream (string_view).
// Read calls are always safe, but in case of underflow they return zero
// values. Caller must use HasError() to determine if an underflow occurred.
// Successful reads move the read position past the data that was read and
// reduce the amount of available bytes.
class StringViewReader {
private:
	std::string_view buffer_;
	bool error_ = false;
public:
	StringViewReader(std::string_view buffer) : buffer_(buffer) {}
	~StringViewReader() {}
	// Returns true if any read operation could not be completed due to running
	// out of bytes to read.
	bool HasError() { return error_; }
	char ReadChar();
	uint8_t ReadUint8();
	uint16_t ReadUint16();
	uint32_t ReadUint32();
	uint64_t ReadUint64();
	// Returns the first N bytes and advances the read position past them.
	std::string_view ReadNBytes(int n);
	// Returns the number of bytes available to read.
	int size() { return buffer_.size(); }
};

// Fixed size buffer that erases its contents during destruction.
// The size must be known at compile time.
template <int Size>
class SecureFixedBuffer {
private:
	char data_[Size];
public:
	// Creates an empty buffer.
	SecureFixedBuffer() {}
	// Creates a buffer initialized with a copy of the given string view.
	SecureFixedBuffer(std::string_view source) {
		Debug::Assert(source.size() == Size, "SecureFixedBuffer size mismatch");
		std::memcpy(data_, source.data(), Size);
	}
	~SecureFixedBuffer() {
		Clear();
	}
	SecureFixedBuffer& operator=(const SecureFixedBuffer& other) = default;
	char* data() { return data_; }
	const char* data() const { return data_; }
	int size() const { return Size; }
	void Clear() {
		std::memset(data_, 0, sizeof(data_));
	}
	std::string_view GetView() const { return std::string_view(data_, Size); }
	StringViewReader GetReader() const { return StringViewReader(GetView()); }
};

// Fixed size string wrapper that erases its contents during destruction.
// The size can be provided at run time.
class SecureString {
private:
	std::string data_;
public:
	// Creates an empty buffer.
	SecureString(int size) : data_(size, 0) {}
	// Creates a buffer initialized with a copy of the given string view.
	SecureString(std::string_view source) : data_(source) {}
	// Creates a buffer that "steals" the contents of a given string (via swap).
	// The input object will become empty afterwards.
	SecureString(std::string& source) { data_.swap(source); }
	~SecureString();
	char* data() { return data_.data(); }
	int size() const { return data_.size(); }
	const std::string& GetString() const { return data_; }
	std::string_view GetView() const { return data_; }
	StringViewReader GetReader() const { return StringViewReader(GetView()); }
};

// Utility class for converting numbers to strings (little endian format).
class StringCoder {
public:
	static std::string EncodeUint16(uint16_t num);
	static std::string EncodeUint32(uint32_t num);
	static std::string EncodeUint64(uint64_t num);
};

}

#endif // __BUFFER_H__
