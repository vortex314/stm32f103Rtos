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
#include <cbor.h>

class CborWriter {
	std::vector<CborEncoder*> _encoders;
	CborEncoder *_encoder;
	std::vector<uint8_t> _data;
	CborError _error;
public:
	CborWriter(size_t sz) {
		_data.resize(sz);
		_encoder = new CborEncoder;
	}
	CborWriter& reset() {
		while (_encoders.size()) {
			delete _encoders.back();
			_encoders.pop_back();
		}
		cbor_encoder_init(_encoder, _data.data(), _data.capacity(), 0);
		return *this;
	}
	CborWriter& array() {
		CborEncoder *arrayEncoder = new CborEncoder;
		_error = cbor_encoder_create_array(_encoder, arrayEncoder,
				CborIndefiniteLength);
		_encoders.push_back(_encoder);
		_encoder = arrayEncoder;
		return *this;
	}
	CborWriter& map() {
		CborEncoder *mapEncoder = new CborEncoder;
		_error = cbor_encoder_create_map(_encoder, mapEncoder,
				CborIndefiniteLength);
		_encoders.push_back(_encoder);
		_encoder = mapEncoder;
		return *this;
	}
	CborWriter& close() {
		if (_error == CborNoError) {
			if (_encoders.size() > 0) {
				CborEncoder *rootEncoder = _encoders.back();
				_error = cbor_encoder_close_container(rootEncoder, _encoder);
				delete _encoder;
				_encoder = rootEncoder;
				_encoders.pop_back();
			} else {
				_error = CborUnknownError;
			}
		}
		return *this;
	}
	Bytes bytes() {
		if (_error == CborNoError && _encoders.size() == 0) {
			size_t sz = cbor_encoder_get_buffer_size(_encoder, _data.data());
			_data.resize(sz);
			return _data;
		}
		_data.clear();
		return _data;
	}
	CborError error() {
		return _error;
	}
	CborWriter& add(uint64_t v) {
		if (_error == CborNoError) {
			_error = cbor_encode_int(_encoder, v);
		}
		return *this;
	}
	CborWriter& add(const char *v) {
		if (_error == CborNoError) {
			_error = cbor_encode_text_string(_encoder, v, strlen(v));
		}
		return *this;
	}
	bool ok() {
		return _error == CborNoError;
	}
};
class CborReader {
	uint8_t *_data;
	size_t _sz;
	CborValue *_value;
	CborError _error;
	CborParser _parser;
	std::vector<CborValue*> _containers;
public:
	CborReader(size_t sz) :
			_sz(sz) {
		_data = new uint8_t[sz];
		_value = new CborValue;
	}
	~CborReader() {
		delete _data;
	}
	CborReader& parse(uint8_t *buffer, size_t length) {
		while (_containers.size()) {
			delete _value;
			_value = _containers.back();
			_containers.pop_back();
		}
		_error = cbor_parser_init(buffer, length, 0, &_parser, _value);
		return *this;
	}
	CborReader& parse(Bytes &bs) {

		return parse(bs.data(), bs.size());
	}
	CborReader& array() {
		assert(_error == CborNoError);
		if (cbor_value_is_container(_value)) {
			CborValue *arrayValue = new CborValue;
			_error = cbor_value_enter_container(_value, arrayValue);
			_containers.push_back(_value);
			_value = arrayValue;
		} else {
			_error = CborErrorIllegalType;
		}
		return *this;
	}
	CborReader& end() {
		if (_error == CborNoError && _containers.size() > 0) {
			CborValue *it = _containers.back();
			_error = cbor_value_leave_container(it, _value);
			delete _value;
			_value = it;
			_containers.pop_back();
		}
		return *this;
	}
	CborReader& get(uint64_t &v) {
		if (_error == CborNoError)
			_error = cbor_value_get_uint64(_value, &v);
		return *this;
	}
	CborReader& get(int64_t &v) {
		if (_error == CborNoError)
			_error = cbor_value_get_int64(_value, &v);
		return *this;
	}
	CborReader& get(int &v) {
		if (_error == CborNoError)
			_error = cbor_value_get_int(_value, &v);
		return *this;
	}
	CborReader& get(std::string &s) {
		if (_error == CborNoError) {
			if (cbor_value_is_text_string(_value)) {
				size_t length;
				_error = cbor_value_calculate_string_length(_value, &length);
				char *temp;
				size_t size;
				_error = cbor_value_dup_text_string(_value, &temp, &size, 0);
				assert(_error == CborNoError);
				s = temp;
				::free(temp);
				if (!_error)
					_error = cbor_value_advance(_value);
			} else {
				_error = CborErrorIllegalType;
			}
		}
		return *this;
	}
	bool ok(){ return _error==CborNoError; }
};
CborWriter tx(100);
CborReader rx(100);

Bytes nodeMessage(const char *name) {
	if (tx.reset().array().add(B_NODE).add(name).close().ok()) {
		Bytes payload = tx.bytes();
		std::string nme;
		int i;
		rx.parse(payload).array().get(i).get(nme).end().ok();
		return tx.bytes();
	} else
		return Bytes();
}

Bytes publishMessage(const char *topic, uint64_t i) {
	if (tx.reset().array().add(B_PUBLISH).add(topic).add(i).close().ok())
		return tx.bytes();
	else
		return Bytes();
}
#endif

void taskBlinker(void *argument) {
	Led yellow = { GPIOB, GPIO_PIN_1 };
	static uint32_t cnt = 0;
	log("Build %s : %s \r\n", __DATE__, __TIME__);

	for (;;) {
		if (cnt % 100 == 0)
			yellow.toggle();
		vTaskDelay(1);
		log("%s %lu : %lu : Hello world, here I am !!!\r\n", TFL, cnt++,
				Log::txBufferOverflow);

		vTaskDelay(5);
		Bytes framed = nodeMessage("stm32f103");
		if (HAL_UART_Transmit_DMA(&huart2, framed.data(), framed.size())
				!= HAL_OK)
			Log::txBufferOverflow++;
		framed = publishMessage("src/stm32f103/system/uptime", Sys::millis());
		vTaskDelay(5);
		if (HAL_UART_Transmit_DMA(&huart2, framed.data(), framed.size())
				!= HAL_OK)
			Log::txBufferOverflow++;
	}
}

extern "C" void app_main() {
	xTaskCreate(taskBlinker, "blinker", 256, NULL, 1, NULL);
//	xPortStartScheduler();
	osKernelStart();
}

