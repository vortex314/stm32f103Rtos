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
class Log {
public:
	 uint32_t txBufferOverflow ;
	 char _buffer[100];
	 size_t offset;
	 Log& tfl(const char* ,const uint32_t );
	 Log& printf(const char* fmt,...);
	 void flush();
private:
};

extern Log logger;

#define INFO(fmt, ...) {logger.tfl(__SHORT_FILE__,__LINE__).printf(fmt,##__VA_ARGS__).flush();}
#define WARN(fmt, ...) {logger.tfl(__SHORT_FILE__,__LINE__).printf(fmt,##__VA_ARGS__).flush();}






#endif /* SRC_LOG_H_ */
