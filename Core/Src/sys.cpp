/*
 * sys.cpp
 *
 *  Created on: Nov 7, 2021
 *      Author: lieven
 */
#include "sys.h"
#include "stm32f1xx_hal.h"

uint64_t Sys::_millis=0;

uint64_t Sys::millis() {
	static uint32_t major = 0;
	static uint32_t prevTick = 0;
	uint32_t newTick = HAL_GetTick();
	if (newTick < prevTick) {
		major++;
	}
	prevTick = newTick;
	_millis = major;
	_millis <<= 32;
	_millis += newTick;
	return _millis;
}

