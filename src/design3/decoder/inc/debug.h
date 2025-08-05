#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <string_view>

namespace ectf {

enum class LedColor {
	Red, Green, Blue,
	Purple, Cyan, Yellow,
	Black, White
};

// Utility class used to provide debug information during development, using
// the onboard LED or "debug" messages sent over UART (see message_bus.h).
// Release builds will have most of the debug functionality turned off and
// debug strings removed from the binary. Assertions are always enabled,
// but failures are handled differently in release builds (see Assert).
// Developers can enable debug functionality by setting the compiler flag
// -DDEBUG_MODE=1.
class Debug {
private:
	static void AssertImpl(bool expression, std::string_view message);
	static void PrintImpl(std::string_view message);
public:
	// Returns true if debug mode is enabled (i.e. if the code was compiled with
	// -DDEBUG_MODE=1).
	static constexpr bool IsDebugMode() {
		#if DEBUG_MODE
			return true;
		#else
			return false;
		#endif
	}
	// Asserts that an expression is true. The behavior when the expression is
	// false depends on debug mode:
	// - If debug mode is true, it sends the given debug message over UART
	//   every second, while also flashing the red LED.
	// - If debug mode is false, it reboots the device.
	// This function should be used to detect fatal errors, possibly due to
	// external tampering, that would cause corruption and undefined behavior if
	// ignored.
	static inline void Assert(bool expression, std::string_view message="") {
		if (IsDebugMode()) {
			AssertImpl(expression, message);
		} else {
			AssertImpl(expression, "");
		}
	}

	// Sends a debug message over UART using the MessageBus protocol.
	// Does nothing if debug mode is not enabled.
	static inline void Print(std::string_view message) {
		if (IsDebugMode()) {
			PrintImpl(message);
		}
	}

	// Changes the color of the onboard LED.
	// Does nothing if debug mode is not enabled.
	static void SetLedColor(LedColor color);
};

}

#endif // __DEBUG_H__
