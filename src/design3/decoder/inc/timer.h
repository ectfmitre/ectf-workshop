#ifndef __TIMER_H__
#define __TIMER_H__

#include <cstdint>

namespace ectf {

// Utility class for measuring elapsed time, implemented using the RTC
// (real time clock) functionality of the MAX78000.
class Timer {
private:
	// Stores the start time of the timer.
	uint32_t start_time_micros_;

	// Returns the number of microseconds elapsed since the timer was started.
	uint32_t GetElapsedMicros() const;

	// Returns a global counter that counts the number of microseconds since
	// the RTC was started. This number will eventually overflow and wrap around
	// to 0, so it should only be used to compute differences.
	static uint32_t GetTotalElapsedMicros();
public:
	// Creates a timer object and sets its start time to the current time.
	Timer();
	// Resets the timer, so that its elapsed time becomes zero.
	void Reset();
	// Delays execution until the specified number of microseconds have elapsed
	// since the timer was created.
	void WaitUntilElapsedMicros(int deadline) const;

	// Perform boot-time initialization, primarily enabling and starting the RTC.
	static void Initialize();
};

}

#endif // __TIMER_H__
