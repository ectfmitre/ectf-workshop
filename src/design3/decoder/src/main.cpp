#include "debug.h"
#include "decoder.h"
#include "flash.h"
#include "message_bus.h"
#include "rand.h"
#include "system.h"
#include "timer.h"

using ectf::Debug;
using ectf::Decoder;
using ectf::FlashStorage;
using ectf::LedColor;
using ectf::MessageBus;
using ectf::Rand;
using ectf::System;
using ectf::Timer;

int main(void) {
	System::Initialize();
	Timer::Initialize();
	Timer timer;
	// delay 0.3s in order to limit the rate at which boot-time side channel
	// attacks can be performed
	System::Delay(300000);
	Debug::SetLedColor(LedColor::Yellow);
	MessageBus::Initialize();
	Rand::Initialize();
	Decoder decoder;
	decoder.Initialize();
	timer.WaitUntilElapsedMicros(900000);
	decoder.RunLoop();
}
