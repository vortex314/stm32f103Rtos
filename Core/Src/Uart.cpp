#include "Uart.h"

extern Uart uart2;

Uart& fromHandle(UART_HandleTypeDef *huart) {
	return uart2;
}

Uart::Uart(Thread &thread, UART_HandleTypeDef *huart)
		:
		Actor(thread), _huart(huart), rxdFrame(5), txdFrame([&](
				const Bytes &bs) {
			sendFrame(bs, 5);
		}) {
	rxdFrame.async(thread);
	_rdPtr = 0;
	_wrPtr = 0;
	crcDMAdone=true;
}

bool Uart::init() {
	__HAL_UART_ENABLE_IT(_huart, UART_IT_IDLE);
	__HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
	HAL_DMA_Init(&hdma_usart2_tx);
//			HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
	HAL_UART_Receive_DMA(_huart, rxBuffer, sizeof(rxBuffer));
	return true;
}
void Uart::sendFrame(const Bytes &bs, int retries) {
	Bytes txBuffer = frame(bs);
	sendBytes(txBuffer.data(), txBuffer.size(), retries);
}
void Uart::sendBytes(uint8_t *data, size_t length, int retries) {
	while (!crcDMAdone && retries) {
		retries--;
		vTaskDelay(1);
	};
	if (!crcDMAdone) {
		_txdOverflow++;
		return;
	}
	static uint8_t txBuffer[FRAME_MAX];
	for (size_t i = 0; i < length; i++)
		txBuffer[i] = data[i];
	crcDMAdone = false;
	if (HAL_UART_Transmit_DMA(&huart2, txBuffer, length) != HAL_OK)
		_txdOverflow++;
}
// empty DMA buffer
void Uart::rxdIrq(UART_HandleTypeDef *huart) {
	_wrPtr = sizeof(rxBuffer) - huart->hdmarx->Instance->CNDTR;
	if (_wrPtr != _rdPtr) {
		if (_wrPtr > _rdPtr) {
			for (size_t i = _rdPtr; i < _wrPtr; i++)
				rxdByte(rxBuffer[i]);
		} else {
			for (size_t i = _rdPtr; i < sizeof(rxBuffer); i++)
				rxdByte(rxBuffer[i]);
			for (size_t i = 0; i < _wrPtr; i++)
				rxdByte(rxBuffer[i]);
		}
		_rdPtr = _wrPtr;
	}
}
// split into PPP frames
void Uart::rxdByte(uint8_t c) {
	if (c == PPP_ESC_CHAR) {
		escFlag = true;
	} else if (c == PPP_FLAG_CHAR) {
		if (_frameRxd.size()) rxdFrame.onIsr(_frameRxd);
		_frameRxd.clear();
	} else if (escFlag) {
		_frameRxd.push_back(c ^ PPP_MASK_CHAR);
		escFlag = false;
	} else {
		if (_frameRxd.size() < FRAME_MAX) {
			_frameRxd.push_back(c);
		} else {
			_rxdOverflow++;
			_frameRxd.clear();
		}
	}
}

void uartSendBytes(uint8_t *data, size_t size, uint32_t retries) {
	uart2.sendBytes(data, size, retries);
}

/* This function handles DMA1 stream6 global interrupt. */
extern "C" void DMA1_Stream6_IRQHandler(void) {
	HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

extern "C" void UART_IDLECallback(UART_HandleTypeDef *huart) {
	fromHandle(huart).rxdIrq(huart);
//	HAL_UART_Receive_DMA(huart, rxBuffer, sizeof(rxBuffer));
}
// restart DMA as first before getting data when buffer overflows.
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_UART_Receive_DMA(huart, fromHandle(huart).rxBuffer, sizeof(Uart::rxBuffer));
	fromHandle(huart).rxdIrq(huart);
}
// get first half of buffer, to be ready before buffer full
extern "C" void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	fromHandle(huart).rxdIrq(huart);
//	HAL_UART_Receive_DMA(huart, rxBuffer, sizeof(rxBuffer));
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	fromHandle(huart).crcDMAdone = true;
}

extern "C" void DMADoneCallback(DMA_HandleTypeDef *hdma) {
}
