#include "main.h"
#include "cmsis_os.h"
#include <string>
#include <deque>
#include <vector>
#include <stdarg.h>
#include "log.h"
#include "frame.h"
#include "limero.h"
#include "Sys.h"
#include <task.h>
#include <deque>

extern "C" I2C_HandleTypeDef hi2c1;
extern "C" TIM_HandleTypeDef htim2;
extern "C" UART_HandleTypeDef huart2;
extern "C" DMA_HandleTypeDef hdma_usart2_rx;
extern "C" DMA_HandleTypeDef hdma_usart2_tx;
extern "C" IWDG_HandleTypeDef hiwdg;
extern "C" CRC_HandleTypeDef hcrc;

struct Led {
		GPIO_TypeDef *port;
		uint32_t pin;
		void toggle() {
			HAL_GPIO_TogglePin(port, pin);
		}
};

//================================================ UART
typedef std::vector<uint8_t> Bytes;
uint8_t dmaRxBuffer[100];
uint32_t count;

//==================================================== UART END
typedef enum {
	B_PUBLISH,	 // RXD,TXD id , Bytes value
	B_SUBSCRIBE, // TXD id, string, qos,
	B_UNSUBSCRIBE,
	B_NODE,
	B_LOG
} MsgType;

#include "CborWriter.h"
#include "CborReader.h"
#include "frame.h"
CborWriter tx(200);
CborReader rx(10);
uint8_t rxBuffer[100];

// https://stackoverflow.com/questions/43298708/stm32-implementing-uart-in-dma-mode



#define FRAME_MAX 100
Thread spineThread("spine");
class Uart: public Actor {
		uint32_t _txdOverflow = 0;
		UART_HandleTypeDef *_huart;
		uint8_t _raw[FRAME_MAX];
		Bytes _frameRxd;
		bool escFlag = false;
		QueueFlow<Bytes> _framesRxd;
		size_t _wrPtr, _rdPtr;
		SinkFunction<Bytes> _framesTxd;

	public:
		Uart(Thread &thread, UART_HandleTypeDef *huart)
				:
				Actor(thread), _huart(huart), _framesRxd(5), _framesTxd([&](
						const Bytes &bs) {
					sendFrame(bs);
				}) {
			_framesRxd.async(thread);
			_rdPtr = 0;
			_wrPtr = 0;
		}
		bool init() {
			__HAL_UART_ENABLE_IT(_huart, UART_IT_IDLE);
			HAL_UART_Receive_DMA(_huart, _raw, sizeof(_raw));
			return true;
		}
		void sendFrame(const Bytes &bs) {
			Bytes buffer = frame(tx.bytes());
			if (HAL_UART_Transmit_DMA(&huart2, buffer.data(), buffer.size())
					!= HAL_OK) _txdOverflow++;
		}
		void UART_Data_Process(UART_HandleTypeDef *huart) {
			_wrPtr = sizeof(_raw) - huart->hdmarx->Instance->CNDTR;
			if (_wrPtr != _rdPtr) {
				if (_wrPtr > _rdPtr) {
					for (size_t i = _rdPtr; i < _wrPtr; i++)
						add(_raw[i]);
				} else {
					for (size_t i = _rdPtr; i < sizeof(_raw); i++)
						add(_raw[i]);
					if (_wrPtr > 0) for (size_t i = 0; i < _wrPtr; i++)
						add(_raw[i]);
				}
				_rdPtr = _wrPtr;
			}
		}
		void add(uint8_t c) {
			if (c == PPP_ESC_CHAR) {
				escFlag = true;
			} else if (c == PPP_FLAG_CHAR) {
				if (_frameRxd.size()) _framesRxd.onIsr(_frameRxd);
				_frameRxd.clear();
			} else if (escFlag) {
				_frameRxd.push_back(c ^ PPP_MASK_CHAR);
				escFlag = false;
			} else {
				_frameRxd.push_back(c);
			}
		}
		Source<Bytes>& incoming() {
			return _framesRxd;
		}
		Sink<Bytes>& outgoing() {
			return _framesTxd;
		}
		uint32_t errors() {
			return _txdOverflow;
		}
} uart2(spineThread,&huart2);


typedef void (*RxdFunction)(Bytes&);

/* This function handles DMA1 stream6 global interrupt. */
extern "C" void DMA1_Stream6_IRQHandler(void) {
	HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

extern "C" void UART_IDLECallback(UART_HandleTypeDef *huart) {
	uart2.UART_Data_Process(huart);
	HAL_UART_Receive_DMA(huart, rxBuffer, sizeof(rxBuffer)); // next rxd cycle
}
/*
extern "C" void USART2_IRQHandler(void) {
	if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE) != RESET) {
		__HAL_UART_CLEAR_IDLEFLAG(&huart2);
		UART_IDLECallback(&huart2);
	}
	HAL_UART_IRQHandler(&huart2);
}
*/
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	uart2.UART_Data_Process(huart);
	HAL_UART_Receive_DMA(huart, rxBuffer, sizeof(rxBuffer)); // next rxd cycle
}

extern "C" void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
//	uart2.UART_Data_Process(huart);
}

void sendNode(const char *topic) {
	if (tx.reset().array().add(B_NODE).add(topic).close().addCrc().ok())
		uart2.outgoing().on(tx.bytes());
}

template<typename T>
void publish(const char *topic, T v) {
	if (tx.reset().array().add(B_PUBLISH).add(topic).add(v).close().addCrc().ok())
		uart2.outgoing().on(tx.bytes());
}

static volatile bool crcDMAdone;

void DMADoneCallback(DMA_HandleTypeDef *handle) {
	crcDMAdone = true;
}

Thread workerThread("worker");

class Blinker: public Actor {
		TimerSource ticker;
		Led led = { GPIOB, GPIO_PIN_1 };
		uint32_t cnt = 0;

	public:
		Blinker(Thread &thr)
				:
				Actor(thr), ticker(thr, 100, true, "ticker") {
			ticker >> [&](const TimerMsg&) {
				led.toggle();
			};
		}
} blinker(workerThread);

uint32_t wrPtr;
uint32_t rdPtr;
size_t rcvdLen;
uint8_t RxBuf3[100];
#define MAX_RX_BUF 70
#include "CircBuf.h"
CircBuf rxd(100);

class Spine: public Actor {
		TimerSource ticker;
		uint32_t count = 0;

	public:
		Spine(Thread &thread)
				:
				Actor(thread), ticker(thread, 100, true, "ticker") {
			log("%s Build %s : %s \r\n", TFL, __DATE__, __TIME__);

			ticker
					>> [&](const TimerMsg&) {
						count++;
						switch (count % 5) {
							case 0: {
								sendNode("stm32f103");
								break;
							}
							case 1: {
								publish("src/stm32f103/system/uptime", Sys::millis());
								break;
							}
							case 2: {
								publish("dst/stm32f103/system/loopback", Sys::millis());
								break;
							}
							case 3: {
								publish("src/stm32f103/system/highWaterMark", uxTaskGetStackHighWaterMark(NULL));
								break;
							}
							case 4: {
								publish("src/stm32f103/system/build", __DATE__ " " __TIME__);
								break;
							}
						}
					};
		}
};

extern "C" void app_main() {
	Spine spine(spineThread);
	uart2.init();
	uart2.incoming() >> [&](const Bytes& bs){
		log("%s bytes rxd : %d \r\n",TFL,bs.size());
	};
	workerThread.start();
	spineThread.start();
	//	xPortStartScheduler();
	osKernelStart();
}
