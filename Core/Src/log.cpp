/*
 * log.cpp
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */

#include "log.h"
#include "stm32f1xx_hal.h"
#include <vector>

extern "C" UART_HandleTypeDef huart2;
extern "C" DMA_HandleTypeDef hdma_usart2_rx;
extern "C" DMA_HandleTypeDef hdma_usart2_tx;
typedef std::vector<uint8_t> Bytes;

Log logger;

extern void uartSendBytes(uint8_t* data,size_t size,uint32_t retries);

Log& Log::printf(const char *format, ...) {
	if ( txBusy ) return *this;
	if (offset > sizeof(_buffer)) return *this;
	va_list args;
	va_start(args, format);
	offset +=
			vsnprintf(&Log::_buffer[offset], sizeof(Log::_buffer) - offset, format, args);
	va_end(args);
	return *this;
}

void Log::flush() {
	if ( txBusy ) return;
	if (offset >= (sizeof(_buffer) - 2)) return;
	Log::_buffer[offset++] = '\r';
	Log::_buffer[offset++] = '\n';
	txBusy=true;
	uartSendBytes((uint8_t*)Log::_buffer,offset,2);
	offset = 0;
	txBusy=false;
}

Log& Log::tfl(const char *file, const uint32_t line) {
	if ( txBusy ) return *this;
	uint32_t t = Sys::millis();
	uint32_t sec = t / 1000;
	uint32_t min = sec / 60;
	uint32_t hr = min / 60;
	offset =
			snprintf(_buffer, sizeof(_buffer), "%2.2lu:%2.2lu:%2.2lu.%3.3lu | %s:%4lu | ", hr
					% 24, min % 60, sec % 60, t % 1000, file, line);
	return *this;
}

std::string hexDump(Bytes bs, const char spacer) {
  static char HEX_DIGITS[] = "0123456789ABCDEF";
  std::string out;
  for (uint8_t b : bs) {
    out += HEX_DIGITS[b >> 4];
    out += HEX_DIGITS[b & 0xF];
    out += spacer;
  }
  return out;
}

