#include <string>
#include <deque>
#include <vector>
#include <stdarg.h>
#include "Log.h"
#include "frame.h"
#include "limero.h"
#include "Sys.h"
#include <FreeRTOS.h>
#include <task.h>
#include <deque>
#include <memory.h>

// https://stackoverflow.com/questions/43298708/stm32-implementing-uart-in-dma-mode

#include "LedBlinker.h"
#include "Uart.h"
#include "Spine.h"

Thread workerThread("worker");
LedBlinker blinker(workerThread, GPIOB, GPIO_PIN_1);
Thread spineThread("spine");
Uart uart2(spineThread, &huart2);
Spine spine(spineThread);
Log logger;

extern "C" void app_main() {
	INFO("Build %s : %s", __DATE__, __TIME__);

	uart2.init();
	spine.setNode("stm32");
	spine.init();
	uart2.rxdFrame >> spine.rxdFrame;
	spine.txdFrame >> uart2.txdFrame;
	spine.connected >> blinker.blinkSlow;

	spine.subscriber<uint64_t>("system/loopback") >> [&](const uint64_t &v) {
		INFO("loopback %d", v);
	};

	spine.cborIn >> [&](const CborReader &reader) {
		static uint32_t count = 0;
		std::string topic;
		uint32_t loopback;
		int32_t type;
		((CborReader&) reader).parse().array().get(type).get(topic).get(loopback).close();
		INFO(" rx ovfl : %d  tx ovfl : %d count : %d ", uart2._rxdOverflow, uart2
				._txdOverflow, count++);
		INFO("incoming publish %s : [%d] ", topic.c_str(), loopback);
	};
	workerThread.start();
	spineThread.start();
	//	xPortStartScheduler();
	osKernelStart();
}
