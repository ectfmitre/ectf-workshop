#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <cstdint>

namespace ectf {

// Utility class for low-level system functions.
class System {
public:
	// Performs boot-time initialization, primarily switching the clock source
	// to the 100MHz IPO.
	static void Initialize();
	// Delays execution for the given amount of time (in microseconds) using a
	// simple loop.
	static void Delay(uint32_t micros);
	// Performs a System Reset, as defined in section 4.5.3 of the MAX78000
	// User Guide. This causes execution to restart from main() without clearing
	// RAM or flash.
	static void Reboot();
};

}

#endif // __SYSTEM_H__
