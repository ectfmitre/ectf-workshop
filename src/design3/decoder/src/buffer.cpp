#include "buffer.h"

#include <cstring>

#include "debug.h"

namespace {

template<typename T>
T ReadFromReader(ectf::StringViewReader& reader) {
	std::string_view bytes = reader.ReadNBytes(sizeof(T));
	if (reader.HasError()) return 0;
	T ret;
	std::memcpy(&ret, bytes.data(), sizeof(ret));
	return ret;
}

}  // namespace

namespace ectf {

char StringViewReader::ReadChar() {
	return ReadFromReader<char>(*this);
}

uint8_t StringViewReader::ReadUint8() {
	return ReadFromReader<uint8_t>(*this);
}

uint16_t StringViewReader::ReadUint16() {
	return ReadFromReader<uint16_t>(*this);
}

uint32_t StringViewReader::ReadUint32() {
	return ReadFromReader<uint32_t>(*this);
}

uint64_t StringViewReader::ReadUint64() {
	return ReadFromReader<uint64_t>(*this);
}

std::string_view StringViewReader::ReadNBytes(int n) {
	if (HasError()) return "";
	if (n == 0) return "";
	if (size() < n) {
		error_ = true;
		return "";
	}
	Debug::Assert(n > 0);
	std::string_view ret = buffer_.substr(0, n);
	buffer_ = buffer_.substr(n);
	return ret;
}

SecureString::~SecureString() {
	std::memset(data_.data(), 0, data_.size());
}

std::string StringCoder::EncodeUint16(uint16_t num) {
	std::string ret(sizeof(num), ' ');
	std::memcpy(ret.data(), &num, sizeof(num));
	return ret;
}

std::string StringCoder::EncodeUint32(uint32_t num) {
	std::string ret(sizeof(num), ' ');
	std::memcpy(ret.data(), &num, sizeof(num));
	return ret;
}

std::string StringCoder::EncodeUint64(uint64_t num) {
	std::string ret(sizeof(num), ' ');
	std::memcpy(ret.data(), &num, sizeof(num));
	return ret;
}

}  // namespace ectf
