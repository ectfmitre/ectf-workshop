#ifndef __RAND_H__
#define __RAND_H__

#include <cstdint>

namespace ectf {

// Utility class for generating random numbers. Uses both the on-device TRNG
// (true random number generator, secure but slow) and a fast PRNG (pseudo
// random number generator).
class Rand {
public:
	// Performs boot-time initialization, enabling the TRNG and using it to
	// seed the PRNG.
	static void Initialize();
	// Returns a random unsigned 32-bit integer using the TRNG.
	static uint32_t SecureRandomInt();
	// Returns a random unsigned 32-bit integer using the PRNG.
	static uint32_t FastRandomInt();
	// Returns a random integer in the range [min,max) using the TRNG.
	static uint32_t SecureRandomRange(uint32_t min, uint32_t max);
	// Returns a random integer in the range [min,max) using the PRNG.
	static uint32_t FastRandomRange(uint32_t min, uint32_t max);
	// Fills the given buffer with random bytes generated using the PRNG.
	static void FastRandomBuffer(char* buf, int size);
};

}

#endif // __RAND_H__
