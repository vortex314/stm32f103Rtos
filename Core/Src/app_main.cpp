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

/* This function handles DMA1 stream6 global interrupt. */
extern "C" void DMA1_Stream6_IRQHandler(void) {
	HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

/*
 void usart2_rx_check(void)
 {
 static uint32_t old_pos;
 uint32_t pos;
 pos = rx_buffer_L_len - __HAL_DMA_GET_COUNTER(huart2.hdmarx);         // Calculate current position in buffer

 if (pos != old_pos) {                                                 // Check change in received data
 if (pos > old_pos) {                                                // "Linear" buffer mode: check if current position is over previous one
 usart_process_debug(&rx_buffer_L[old_pos], pos - old_pos);        // Process data
 } else {                                                            // "Overflow" buffer mode
 usart_process_debug(&rx_buffer_L[old_pos], rx_buffer_L_len - old_pos); // First Process data from the end of buffer
 if (pos > 0) {                                                    // Check and continue with beginning of buffer
 usart_process_debug(&rx_buffer_L[0], pos);                      // Process remaining data
 }
 }
 }
 }
 */

//==================================================== UART END
typedef enum {
	B_PUBLISH,   // RXD,TXD id , Bytes value
	B_SUBSCRIBE, // TXD id, string, qos,
	B_UNSUBSCRIBE,
	B_NODE,
	B_LOG
} MsgType;
#ifdef JSON
#include <ArduinoJson.h>

StaticJsonDocument<100> txDoc;
class BytesWrite: public Bytes {
public:
	bool write(uint8_t b) {
		push_back(b);
		return true;
	}
	bool write(const uint8_t*& bs,size_t sz){
		for (size_t i=0;i<sz;i++) push_back(bs[i]);
		return true;
	}
} txBytes;

Bytes nodeMessage(const char* name){
	txDoc.clear();
	txBytes.clear();
	txDoc[0] = B_NODE;
	txDoc[1] = name;
	serializeJson(txDoc, txBytes);
	return ppp_frame(txBytes);
}

Bytes publishMessage(const char* topic, uint64_t i) {
	txDoc.clear();
		txBytes.clear();
		txDoc[0] = B_PUBLISH;
		txDoc[1] = topic;
		txDoc[2] = i;
		serializeJson(txDoc, txBytes);
		return ppp_frame(txBytes);
}
#endif

#define CBOR
#ifdef CBOR
#include "CborWriter.h"
#include "CborReader.h"
#include "frame.h"
CborWriter tx(200);
CborReader rx(100);



void sendNode(const char *topic) {
	if (tx.reset().array().add(B_NODE).add(topic).close().ok()) {
		tx.addCrc();
		Bytes buffer = frame(tx.bytes());
		if (HAL_UART_Transmit_DMA(&huart2, buffer.data(), buffer.size())
				!= HAL_OK) Log::txBufferOverflow++;
	}
}


template < typename T>
void publish(const char *topic, T v) {
	if (tx.reset().array().add(B_PUBLISH).add(topic).add(v).close().ok()) {
		tx.addCrc();
		Bytes buffer = frame(tx.bytes());
		if (HAL_UART_Transmit_DMA(&huart2, buffer.data(), buffer.size())
				!= HAL_OK) Log::txBufferOverflow++;
	}
}

#endif
static volatile bool crcDMAdone;

void DMADoneCallback(DMA_HandleTypeDef *handle) {
	crcDMAdone = true;
}

Thread workerThread("worker");
Thread spineThread("spine");

class Blinker : public Actor {
		TimerSource ticker;
		Led yellow = { GPIOB, GPIO_PIN_1 };

	public:
		Blinker(Thread& thr):Actor(thr),ticker(thr,100,true,"ticker") {
			ticker >> [&](const TimerMsg& ){
				yellow.toggle();
			};
		}
} blinker(workerThread);

class Spine : public Actor {
		TimerSource ticker;
		uint32_t count=0;
	public :
		Spine(Thread& thread) :Actor(thread),ticker(thread,100,true,"ticker"){
			INFO("Build %s : %s", __DATE__, __TIME__);

			ticker >> [&](const TimerMsg& ){
				count++;
				switch (count%5) {
					case 0 : {
						sendNode("stm32f103");
						break;
					}
					case 1 : {
						publish("src/stm32f103/system/uptime", Sys::millis());
						break;
					}
					case 2 : {
						publish("dst/stm32f103/system/loopback",Sys::millis());
						break;
					}
					case 3 : {
						publish("src/stm32f103/system/highWaterMark",uxTaskGetStackHighWaterMark( NULL ));
						break;
					}
					case 4 : {
						publish("src/stm32f103/system/build",__DATE__ " " __TIME__);
						break;
					}
				}
			};
		}

} spine(spineThread);


extern "C" void app_main() {
	workerThread.start();
	spineThread.start();
//	xPortStartScheduler();
	osKernelStart();
}

