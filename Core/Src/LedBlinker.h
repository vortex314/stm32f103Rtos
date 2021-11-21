#ifndef _LEDBLINKER_H_
#define _LEDBLINKER_H_
#include "limero.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"

struct Led {
		GPIO_TypeDef *port;
		uint32_t pin;
		void toggle();
};

class LedBlinker: public Actor {
		TimerSource ticker;
		Led led = { GPIOB, GPIO_PIN_1 };
		uint32_t cnt = 0;

	public:
		SinkFunction<bool> blinkSlow;
		LedBlinker(Thread &thr, GPIO_TypeDef *port, uint32_t pin);
};
#endif
