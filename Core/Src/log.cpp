/*
 * log.cpp
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */

#include "log.h"
#include "stm32f1xx_hal.h"

extern "C" UART_HandleTypeDef huart2;
extern "C" DMA_HandleTypeDef hdma_usart2_rx;
extern "C" DMA_HandleTypeDef hdma_usart2_tx;

uint32_t Log::txBufferOverflow = 0;
char Log::_buffer[100];

void log(const char *format, ...) {
	if (huart2.hdmatx->State != HAL_DMA_STATE_BUSY) {
		va_list args;
		int n;
		va_start(args, format);
		n = vsnprintf(Log::_buffer, sizeof(Log::_buffer), format, args);
		va_end(args);
		Log::_buffer[n] = '\0';
		while( HAL_UART_Transmit_DMA(&huart2, (uint8_t*) Log::_buffer, n) != HAL_OK );
	} else
		Log::txBufferOverflow++;
}

const char* tfl(const char *file, const uint32_t line) {
	static char header[40];
	uint32_t t = Sys::millis();
	uint32_t sec = t / 1000;
	uint32_t min = sec / 60;
	uint32_t hr = min / 60;

	snprintf(header, sizeof(header), "%2.2lu:%2.2lu:%2.2lu.%3.3lu | %s:%4lu | ",
			hr % 24, min % 60, sec % 60, t % 1000, file, line);
	return header;
}

