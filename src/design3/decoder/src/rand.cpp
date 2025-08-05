#include "rand.h"

#include <cstdint>
#include <cstring>

// from MSDK
#include "trng.h"


namespace {

uint32_t random_number;

uint32_t RandomRange(uint32_t min, uint32_t max, uint32_t rand) {
	return min + (uint32_t)(((uint64_t) max - min) * rand >> 32);
}

}  // namespace

namespace ectf {

void Rand::Initialize() {
	MXC_TRNG_Init();
	SecureRandomInt();
}

uint32_t Rand::SecureRandomInt() {
	return random_number = MXC_TRNG_RandomInt();
}

uint32_t Rand::FastRandomInt() {
	random_number ^= random_number << 13;
	random_number ^= random_number >> 17;
	random_number ^= random_number << 5;
	return random_number;
}

uint32_t Rand::SecureRandomRange(uint32_t min, uint32_t max) {
	return RandomRange(min, max, SecureRandomInt());
}

uint32_t Rand::FastRandomRange(uint32_t min, uint32_t max) {
	return RandomRange(min, max, FastRandomInt());
}

void Rand::FastRandomBuffer(char* buf, int size) {
	constexpr int wordsize = sizeof(random_number);
	while (size >= wordsize) {
		FastRandomInt();
		std::memcpy(buf, &random_number, wordsize);
		buf += wordsize;
		size -= wordsize;
	}
	if (size > 0) {
		FastRandomInt();
		std::memcpy(buf, &random_number, size);
	}
}

}  // namespace ectf
