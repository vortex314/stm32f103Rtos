#include "main.h"
#include "cmsis_os.h"
#include <string>
#include <deque>
#include <vector>
#include <stdarg.h>
#include "sys.h"
#include "log.h"
#include "frame.h"

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

Bytes nodeMessage(const char *name) {
	if (tx.reset().array().add(B_NODE).add(name).close().ok()) {
		Bytes payload = tx.bytes();
		std::string nme;
		int i;
		rx.parse(payload).array().get(i).get(nme).close().ok();
		assert(i == B_NODE);
		assert(strcmp(nme.c_str(), name) == 0);
		tx.addCrc();
		return tx.bytes();
	} else
		return Bytes();
}

Bytes publishMessage(const char *topic, uint64_t v) {
	if (tx.reset().array().add(B_PUBLISH).add(topic).add(v).close().ok()) {
		Bytes payload = tx.bytes();
		std::string nme;
		int i;
		uint64_t u;
		rx.parse(payload).array().get(i).get(nme).get(u).close().ok();
		assert(i == B_PUBLISH);
		assert(strcmp(nme.c_str(), topic) == 0);
		assert(v == u);
		tx.addCrc();
		return tx.bytes();
	} else
		return Bytes();
}
#endif
static volatile bool crcDMAdone;

void DMADoneCallback(DMA_HandleTypeDef *handle) {
	crcDMAdone = true;
}

void taskBlinker(void *argument) {
	Led yellow = { GPIOB, GPIO_PIN_1 };
	log("Build %s : %s \r\n", __DATE__, __TIME__);

	static uint32_t cnt = 0;
	for (;;) {
		if (cnt % 100 == 0)
			yellow.toggle();
		vTaskDelay(1);
		log("%s %lu : %lu : Hello world, here I am !!!\r\n", TFL, cnt++,
				Log::txBufferOverflow);

		vTaskDelay(5);
		Bytes framed = frame(nodeMessage("stm32f103"));
		if (HAL_UART_Transmit_DMA(&huart2, framed.data(), framed.size())
				!= HAL_OK)
			Log::txBufferOverflow++;
		framed = frame(publishMessage("src/stm32f103/system/uptime", Sys::millis()));
		vTaskDelay(5);
		if (HAL_UART_Transmit_DMA(&huart2, framed.data(), framed.size())
				!= HAL_OK)
			Log::txBufferOverflow++;
	}
}

extern "C" void app_main() {
	xTaskCreate(taskBlinker, "blinker", 128, NULL, 1, NULL);
//	xPortStartScheduler();
	osKernelStart();
}

