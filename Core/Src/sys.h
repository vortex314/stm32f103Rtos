/*
 * sys.h
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */

#ifndef SRC_SYS_H_
#define SRC_SYS_H_
#include <stdint.h>

class Sys {
public:
	static uint64_t millis();
private:
	static uint64_t _millis;
};



#endif /* SRC_SYS_H_ */
