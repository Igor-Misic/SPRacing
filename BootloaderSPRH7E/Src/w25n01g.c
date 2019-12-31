#include "w25n01g.h"
#include "quadspi.h"

#define W25N01G_LINEAR_TO_COLUMN(laddr) ((laddr) % W25N01G_PAGE_SIZE)
#define W25N01G_LINEAR_TO_PAGE(laddr) (((laddr) / W25N01G_PAGE_SIZE) & 0xffff)
#define W25N01G_LINEAR_TO_BLOCK(laddr) (W25N01G_LINEAR_TO_PAGE(laddr) / W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_PAGE(block) ((block) * W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_LINEAR(block) (W25N01G_BLOCK_TO_PAGE(block) * W25N01G_PAGE_SIZE)

static bool W25n01g_performCommandWithPageAddress(QSPI_HandleTypeDef *hqspi, uint8_t command, uint16_t pageAddress);


void W25n01g_readJedec(QSPI_HandleTypeDef *hqspi, uint8_t* idBuffer) {
	QuadSpiReceive1LINE(hqspi, W25N01G_INSTR_JEDEC_ID, 8, idBuffer, 3);
}

bool W25n01g_deviceRestart(QSPI_HandleTypeDef *hqspi)
{
	W25n01g_waitForReady(hqspi);
	bool success = true;
	success = QuadSpiTransmit1LINE(hqspi, W25N01G_INSTR_DEVICE_RESET, 0, NULL, 0);
	return success;
}

bool W25n01g_writeEnable(QSPI_HandleTypeDef *hqspi)
{
	uint8_t statusReg;

	W25n01g_waitForReady(hqspi);
	bool success = false;
	success = QuadSpiTransmit1LINE(hqspi, W25N01G_INSTR_WRITE_ENABLE, 0, NULL, 0);

	if(success) {
		statusReg = W25n01g_readStatusRegister(hqspi, W25N01G_STAT_REG);

		if(!(statusReg & W25N01G_STATUS_FLAG_WRITE_ENABLED)) {
			success = false;
		}
	}

	return success;
}

uint8_t W25n01g_readStatusRegister(QSPI_HandleTypeDef *hqspi, uint8_t reg)
{
	uint8_t buffer = 0xFF;
	QuadSpiReceiveWithAddress1LINE(hqspi, W25N01G_INSTR_READ_STATUS_REG, 0, reg, W25N01G_STATUS_REGISTER_SIZE, &buffer, sizeof(buffer));

	return buffer;
}

void W25n01g_waitForReady(QSPI_HandleTypeDef *hqspi)
{
	uint8_t statusReg = W25n01g_readStatusRegister(hqspi, W25N01G_STAT_REG);
	while (statusReg & W25N01G_STATUS_FLAG_BUSY) {
		//HAL_Delay(1);
		statusReg = W25n01g_readStatusRegister(hqspi, W25N01G_STAT_REG);
	}
}

/*
QuadSpiTransmitWithAddress1LINE(
		  &hqspi,
		  W25N01G_INSTR_PROGRAM_DATA_LOAD,
		  0,
		  APPLICATION_ADDRESS,
		  W28N01G_STATUS_COLUMN_ADDRESS_SIZE,
		  transferBuffer,
		  4
		  );
		  */


//QuadSpiReceive1LINE(&hqspi, W25N01G_INSTR_JEDEC_ID, 8, idBuffer, 3);
//QuadSpiReceiveWithAddress4LINES(&hqspi, W25N01G_INSTR_FAST_READ_QUAD_OUTPUT, 8, column, W28N01G_STATUS_COLUMN_ADDRESS_SIZE, buffer, length);

bool W25n01g_memoryMappedModeEnable(QSPI_HandleTypeDef *hqspi, bool bufferRead)
{
	bool success = true;
	QSPI_CommandTypeDef cmd;
	QSPI_MemoryMappedTypeDef memMappedCfg;
	volatile uint8_t confReg;

	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.Instruction			= W25N01G_INSTR_FAST_READ_QUAD;
	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_4_LINES;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;

	if(bufferRead) {
		confReg = W25n01g_readStatusRegister(hqspi, W25N01G_CONF_REG);
		confReg |= W25N01G_CONFIG_BUFFER_READ_MODE;
		W25n01g_writeStatusRegister(hqspi, W25N01G_CONF_REG, confReg);


		cmd.AddressMode		= QSPI_ADDRESS_4_LINES;
		cmd.AddressSize		= QSPI_ADDRESS_16_BITS;
		cmd.DummyCycles		= W25N01G_DUMMY_BITS_FAST_READ_QUAD_BUFFER;
		cmd.SIOOMode		= QSPI_SIOO_INST_EVERY_CMD;

	} else {
		confReg = W25n01g_readStatusRegister(hqspi, W25N01G_CONF_REG);
		confReg &= ~(W25N01G_CONFIG_BUFFER_READ_MODE);
		W25n01g_writeStatusRegister(hqspi, W25N01G_CONF_REG, confReg);
		cmd.DummyCycles		= W25N01G_DUMMY_BITS_FAST_READ_QUAD_CONT;
		cmd.SIOOMode		= QSPI_SIOO_INST_ONLY_FIRST_CMD;
		cmd.AddressMode		= QSPI_ADDRESS_NONE;
		//cmd.NbData			= 2048*3;
	}

	memMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	memMappedCfg.TimeOutPeriod = 0;

	uint32_t targetPage = 0;

	W25n01g_waitForReady(hqspi);
	W25n01g_performCommandWithPageAddress(hqspi, W25N01G_INSTR_PAGE_DATA_READ, targetPage);

	W25n01g_waitForReady(hqspi);

	if (HAL_QSPI_MemoryMapped(hqspi, &cmd, &memMappedCfg) != HAL_OK)
	{
		success = false;
	}
	return success;
}

bool W25n01g_blockErase(QSPI_HandleTypeDef *hqspi, uint32_t address)
{
	bool success = true;
	uint16_t pageAddress = W25N01G_LINEAR_TO_PAGE(address) & 0xffff;

	success = W25n01g_writeEnable(hqspi);

	if(success) {
		W25n01g_waitForReady(hqspi);
		//in data sheet is  dummy cycles but it works with 0
		success = QuadSpiInstructionWithAddress1LINE(hqspi, W25N01G_INSTR_BLOCK_ERASE, 0, pageAddress, W25N01G_STATUS_PAGE_ADDRESS_SIZE);
	}
	return success;
}

void W25n01g_writeStatusRegister(QSPI_HandleTypeDef *hqspi, uint8_t reg, uint8_t data)
{
	W25n01g_waitForReady(hqspi);
	QuadSpiTransmitWithColumnAddress1LINE(hqspi, W25N01G_INSTR_WRITE_STATUS_ALTERNATE_REG, 0, reg, W25N01G_STATUS_REGISTER_SIZE, &data, 1);
}

static bool W25n01g_performCommandWithPageAddress(QSPI_HandleTypeDef *hqspi, uint8_t command, uint16_t pageAddress)
{
	bool success = true;

	//success = W25n01g_writeEnable(hqspi);
	W25n01g_waitForReady(hqspi);

	if(success) {
		success = QuadSpiInstructionWithAddress1LINE(hqspi, command, 0, pageAddress, W25N01G_STATUS_PAGE_ADDRESS_SIZE);
	}
	return success;
}

bool W25n01g_programDataLoad(QSPI_HandleTypeDef *hqspi, uint16_t columnAddress, const uint8_t *data, uint32_t length)
{
	bool success = false;

	success = W25n01g_writeEnable(hqspi);
	W25n01g_waitForReady(hqspi);

	if (success) {
		success = QuadSpiTransmitWithColumnAddress1LINE(hqspi, W25N01G_INSTR_PROGRAM_DATA_LOAD, 0, columnAddress, W25N01G_STATUS_COLUMN_ADDRESS_SIZE, data, length);
	}
	return success;
}

bool w25n01g_pageProgram(QSPI_HandleTypeDef *hqspi, uint32_t address, const uint8_t *data, uint32_t length) {

	bool success = true;

	uint16_t columnAddress = W25N01G_LINEAR_TO_COLUMN(address);
	uint16_t pageAddress = W25N01G_LINEAR_TO_PAGE(address);

	success = W25n01g_programDataLoad(hqspi, columnAddress, data,length);

	if(success) {
		success = W25n01g_performCommandWithPageAddress(hqspi, W25N01G_INSTR_PROGRAM_EXECUTE, pageAddress);
	}

	if(success) {
		W25n01g_waitForReady(hqspi);
		uint8_t statusReg = W25n01g_readStatusRegister(hqspi, W25N01G_STAT_REG);
		success = ((W25N01G_STATUS_PROGRAM_FAIL & statusReg) != W25N01G_STATUS_PROGRAM_FAIL);
	}
	return success;
}

bool w25n01g_flashProgram(QSPI_HandleTypeDef *hqspi, uint32_t address, const uint8_t *data, uint32_t length) {

	bool success = true;

	uint32_t numberOfpages = length / W25N01G_PAGE_SIZE;
	uint32_t pageIndex;
	uint32_t pageAddress;

	for(pageIndex = 0; (pageIndex < numberOfpages) && success; pageIndex++) {

		pageAddress = address + W25N01G_PAGE_SIZE*pageIndex;
		success = w25n01g_pageProgram(hqspi, pageAddress, &(data[W25N01G_PAGE_SIZE*pageIndex]), W25N01G_PAGE_SIZE);
	}

	uint32_t notFullPageSize = (length % W25N01G_PAGE_SIZE);

	if(success & (notFullPageSize != 0)) {
		pageAddress = address + W25N01G_PAGE_SIZE*pageIndex;
		success = w25n01g_pageProgram(hqspi, pageAddress, &(data[W25N01G_PAGE_SIZE*pageIndex]), notFullPageSize);
	}

	return success;
}

uint32_t W25n01g_readBytes(QSPI_HandleTypeDef *hqspi, uint32_t address, uint8_t *buffer, uint32_t length, bool bufferMode)
{
	uint32_t targetPage = W25N01G_LINEAR_TO_PAGE(address);

	W25n01g_waitForReady(hqspi);
	W25n01g_performCommandWithPageAddress(hqspi, W25N01G_INSTR_PAGE_DATA_READ, targetPage);

	uint16_t column = W25N01G_LINEAR_TO_COLUMN(address);
	uint16_t transferLength;

	if (length > (W25N01G_PAGE_SIZE - column)) {
		transferLength = W25N01G_PAGE_SIZE - column;
	} else {
		transferLength = length;
	}
	W25n01g_waitForReady(hqspi);

	uint8_t dummyCycles;

	if(bufferMode) {
		dummyCycles = 4;
		QuadSpiReceiveWithAddress4LINES(hqspi, W25N01G_INSTR_FAST_READ_QUAD, dummyCycles, column, buffer, length, bufferMode);
	} else {
		dummyCycles = 12;
		QuadSpiReceiveWithAddress4LINES(hqspi, W25N01G_INSTR_FAST_READ_QUAD, dummyCycles, column, buffer, length, bufferMode);
	}

	// Check ECC

	uint8_t statReg = W25n01g_readStatusRegister(hqspi, W25N01G_STAT_REG);
	uint8_t eccCode = W25N01G_STATUS_FLAG_ECC(statReg);

	switch (eccCode) {
	case 0: // Successful read, no ECC correction
		break;
	case 1: // Successful read with ECC correction
	case 2: // Uncorrectable ECC in a single page
	case 3: // Uncorrectable ECC in multiple pages
		//w25n01g_addError(address, eccCode);
		//w25n01g_deviceReset(fdevice);
		break;
	}

	return transferLength;
}
