/*
 * log.h
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */

#ifndef SRC_LOG_H_
#define SRC_LOG_H_
#include <stdint.h>
#include <stdarg.h>
#include <string>

#include "Sys.h"

const char* tfl(const char *file, const uint32_t line) ;
void log(const char *format, ...) ;
#define INFO(fmt, ...) {log(TFL);log(fmt,##__VA_ARGS__);log("\r\n");}
#define WARN(fmt, ...) {log(TFL);log(fmt,##__VA_ARGS__);log("\r\n");}

using cstr = const char* const;

static constexpr cstr past_last_slash(cstr str, cstr last_slash) {
	return *str == '\0' ? last_slash :
			*str == '/' ?
					past_last_slash(str + 1, str + 1) :
					past_last_slash(str + 1, last_slash);
}

static constexpr cstr past_last_slash(cstr str) {
	return past_last_slash(str, str);
}

#define __SHORT_FILE__                              \
  ({                                                \
    constexpr cstr sf__{past_last_slash(__FILE__)}; \
    sf__;                                           \
  })

#define TFL tfl(__SHORT_FILE__,__LINE__)

class Log {
public:
	static uint32_t txBufferOverflow ;
	static char _buffer[100];

private:
};


#endif /* SRC_LOG_H_ */
