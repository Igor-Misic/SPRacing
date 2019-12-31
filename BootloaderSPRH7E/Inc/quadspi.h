/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __QUADSPI_H
#define __QUADSPI_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h7xx_hal.h"

bool QuadSpiReceive1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint8_t *in, int length);
bool QuadSpiTransmit1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, const uint8_t *out, int length);
bool QuadSpiTransmitWithColumnAddress1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize, const uint8_t *out, int length);
bool QuadSpiReceiveWithAddress1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize, uint8_t *in, int length);

bool QuadSpiReceiveWithAddress4LINES(
		QSPI_HandleTypeDef *hqspi,
		uint8_t instruction,
		uint8_t dummyCycles,
		uint32_t address,
		uint8_t *in,
		int length,
		bool bufferMode);

bool QuadSpiInstructionWithAddress1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize);

#endif /* __QUADSPI_H */
