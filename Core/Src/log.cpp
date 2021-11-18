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

Log logger;

Log& Log::printf(const char *format, ...) {
	if ( offset > sizeof(_buffer) ) return *this;
	va_list args;
	va_start(args, format);
	offset +=
			vsnprintf(&Log::_buffer[offset], sizeof(Log::_buffer) - offset, format, args);
	va_end(args);
	return *this;
}

void Log::flush() {
	if ( offset >= (sizeof(_buffer)-2)) return;
	Log::_buffer[offset++]='\r';
	Log::_buffer[offset++]='\n';
	if (huart2.hdmatx->State == HAL_DMA_STATE_BUSY) Log::txBufferOverflow++;

	while (HAL_UART_Transmit_DMA(&huart2, (uint8_t*) Log::_buffer, offset)
			!= HAL_OK)
		;
	offset = 0;
}

Log& Log::tfl(const char *file, const uint32_t line) {
	uint32_t t = Sys::millis();
	uint32_t sec = t / 1000;
	uint32_t min = sec / 60;
	uint32_t hr = min / 60;

	offset =
			snprintf(_buffer, sizeof(_buffer), "%2.2lu:%2.2lu:%2.2lu.%3.3lu | %s:%4lu | ", hr
					% 24, min % 60, sec % 60, t % 1000, file, line);
	return *this;
}

