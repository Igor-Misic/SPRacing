/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __W25Q_H
#define __W25Q_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h7xx_hal.h"

#define W25NQ_INSTR_JEDEC_ID						0x9F

void W25q_readJedec(QSPI_HandleTypeDef *hqspi, uint8_t* idBuffer);

#endif /* __W25Q_H */
