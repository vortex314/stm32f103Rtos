/*
 * Uart.h
 *
 *  Created on: Nov 21, 2021
 *      Author: lieven
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_

#include "frame.h"
#include "limero.h"
#include "Cube.h"

//#define FRAME_MAX 128

class Uart: public Actor {
		static const size_t FRAME_MAX = 128;

		UART_HandleTypeDef *_huart;
		Bytes _frameRxd;
		bool escFlag = false;
		size_t _wrPtr, _rdPtr;


	public:
		uint8_t rxBuffer[FRAME_MAX];
		volatile bool crcDMAdone;
		uint32_t _txdOverflow = 0;
		uint32_t _rxdOverflow = 0;

		QueueFlow<Bytes> rxdFrame;
		SinkFunction<Bytes> txdFrame;
		ValueFlow<Bytes> txd;

		Uart(Thread &thread, UART_HandleTypeDef *huart);
		bool init();
		void rxdIrq(UART_HandleTypeDef *huart);
		void rxdByte(uint8_t);
		void sendBytes(uint8_t*, size_t, int retries);
		void sendFrame(const Bytes &bs, int retries) ;

};

extern "C" void uartSendBytes(uint8_t*, size_t, uint32_t retries);

#endif /* SRC_UART_H_ */
