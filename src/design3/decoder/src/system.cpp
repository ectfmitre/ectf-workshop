#include "system.h"

#include <cstdint>

// from MSDK
#include "mxc_sys.h"

#include "debug.h"

namespace {

// 1 second takes  5996768 ticks at 60MHz
// 1 second takes 10044586 ticks at 100MHz
void __attribute__((optimize("O0"))) SpinLoop(uint32_t ticks) {
	for (; ticks > 0; ticks--);
}

}  // namespace

// Computes a * b / c
uint32_t MultiplyFraction(uint32_t a, uint32_t b, uint32_t c) {
	return (uint32_t)(((uint64_t) a) * b / c);
}

uint32_t MicrosToTicks(uint32_t micros) {
	return MultiplyFraction(micros, 10044586, 1000000);
}

namespace ectf {

void System::Initialize() {
	// Switch to 100MHz clock
	int retcode = MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
	Debug::Assert(retcode == 0);
	SystemCoreClockUpdate();
}

void System::Delay(uint32_t micros) {
	SpinLoop(MicrosToTicks(micros));
}

void System::Reboot() {
	MXC_SYS_Reset_Periph(MXC_SYS_RESET0_SYS);
	for(;;);
}

}  // namespace ectf
