#include "LedBlinker.h"

void Led::toggle() {
	HAL_GPIO_TogglePin(port, pin);
}

#define SLOW_INTERVAL 500
#define FAST_INTERVAL 50

LedBlinker::LedBlinker(Thread &thr, GPIO_TypeDef *port, uint32_t pin)
		:
		Actor(thr), ticker(thr, FAST_INTERVAL, true, "ticker"), blinkSlow([&](
				const bool &slow) {
			ticker.interval(slow ? SLOW_INTERVAL : FAST_INTERVAL);
		}) {
	led = { port, pin };
	ticker >> [&](const TimerMsg&) {
		led.toggle();
	};
	blinkSlow.on(false);
}

