#include "debug.h"

#include <string>
#include <string_view>

// from MSDK
#include "led.h"

#include "buffer.h"
#include "message_bus.h"
#include "system.h"

namespace {

// Infinite recursion guard for Print().
bool is_printing_ = false;

}  // namespace

namespace ectf {

void Debug::AssertImpl(bool expression, std::string_view message) {
	if (expression) return;
	if (IsDebugMode()) {
		bool led_on = true;
		while (true) {
			SetLedColor(led_on ? LedColor::Red : LedColor::Black);
			Print(message);
			System::Delay(1000000);
			led_on = !led_on;
		}
	} else {
		System::Delay(1000000);
		System::Reboot();
	}
}

void Debug::PrintImpl(std::string_view message) {
	if (!IsDebugMode()) return;
	if (is_printing_) return;
	is_printing_ = true;
	MessageBus::WriteResponse(OpCode::Debug, message);
	is_printing_ = false;
}

void Debug::SetLedColor(LedColor color) {
	if (!IsDebugMode()) return;
	// red led
	switch(color) {
	case LedColor::Black:
	case LedColor::Green:
	case LedColor::Blue:
	case LedColor::Cyan:
		LED_Off(LED1);
		break;
	default:
		LED_On(LED1);
	}
	// green led
	switch(color) {
	case LedColor::Black:
	case LedColor::Red:
	case LedColor::Blue:
	case LedColor::Purple:
		LED_Off(LED2);
		break;
	default:
		LED_On(LED2);
	}
	// blue led
	switch(color) {
	case LedColor::Black:
	case LedColor::Red:
	case LedColor::Green:
	case LedColor::Yellow:
		LED_Off(LED3);
		break;
	default:
		LED_On(LED3);
	}
}

}  // namespace ectf
