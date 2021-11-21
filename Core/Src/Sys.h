/*
 * sys.h
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */

#ifndef SRC_SYS_H_
#define SRC_SYS_H_
#include <stdint.h>
#include <string>
#include <vector>
typedef std::vector<uint8_t> Bytes;

class Sys {
	public:
		static uint64_t millis();
		static const char* hostname() {
			return "stm32f103";
		}
	private:
		static uint64_t _millis;
};

#endif /* SRC_SYS_H_ */
