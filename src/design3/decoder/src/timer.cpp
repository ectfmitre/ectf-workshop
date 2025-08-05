#include "timer.h"

#include <cstdio>
#include <string>

#include "rtc.h"

#include "debug.h"


namespace ectf {

void Timer::Initialize() {
	Debug::Assert(MXC_RTC_Init(0, 0) == E_NO_ERROR);
	Debug::Assert(MXC_RTC_Start() == E_NO_ERROR);
}

uint32_t Timer::GetTotalElapsedMicros() {
	uint32_t sec, subsec;
	while (MXC_RTC_GetTime(&sec, &subsec) != E_NO_ERROR) {}
	return sec * 1000000 + (subsec * 1000000 >> 12);
}

Timer::Timer() {
	Reset();
}

void Timer::Reset() {
	start_time_micros_ = GetTotalElapsedMicros();
}

uint32_t Timer::GetElapsedMicros() const {
	return GetTotalElapsedMicros() - start_time_micros_;
}

void Timer::WaitUntilElapsedMicros(int deadline) const {
	while (((int) GetElapsedMicros()) < deadline) {}
}

}  // namespace ectf
