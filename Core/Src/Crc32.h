/*
 * Crc32.h
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */

#ifndef SRC_CRC32_H_
#define SRC_CRC32_H_

class Crc32 {
	public:
		static uint32_t bufferCrc(uint32_t *start, size_t size);
};

#endif /* SRC_CRC32_H_ */
