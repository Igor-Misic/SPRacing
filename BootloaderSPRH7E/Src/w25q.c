#include "w25q.h"
#include "quadspi.h"

void W25q_readJedec(QSPI_HandleTypeDef *hqspi, uint8_t* idBuffer) {
	QuadSpiReceive1LINE(hqspi, W25NQ_INSTR_JEDEC_ID, 0, idBuffer, 3);
}

