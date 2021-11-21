/*
 * cube.h
 *
 *  Created on: Nov 21, 2021
 *      Author: lieven
 */

#ifndef SRC_CUBE_H_
#define SRC_CUBE_H_

#include "main.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"

extern "C" I2C_HandleTypeDef hi2c1;
extern "C" TIM_HandleTypeDef htim2;
extern "C" UART_HandleTypeDef huart2;
extern "C" DMA_HandleTypeDef hdma_usart2_rx;
extern "C" DMA_HandleTypeDef hdma_usart2_tx;
extern "C" IWDG_HandleTypeDef hiwdg;
extern "C" CRC_HandleTypeDef hcrc;



#endif /* SRC_CUBE_H_ */
