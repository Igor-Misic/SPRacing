#include "quadspi.h"

#define QUADSPI_DEFAULT_TIMEOUT 200

static const uint32_t quadSpi_addressSizeMap[] = {
	QSPI_ADDRESS_8_BITS,
	QSPI_ADDRESS_16_BITS,
	QSPI_ADDRESS_24_BITS,
	QSPI_ADDRESS_32_BITS
};

static uint32_t QuadSpi_addressSizeFromValue(uint8_t addressSize);

static uint32_t QuadSpi_addressSizeFromValue(uint8_t addressSize)
{
	return quadSpi_addressSizeMap[((addressSize + 1) / 8) - 1]; // rounds to nearest QSPI_ADDRESS_* value that will hold the address.
}

bool QuadSpiReceive1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint8_t *in, int length)
{
	HAL_StatusTypeDef status;

	QSPI_CommandTypeDef cmd;
	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.AddressMode			= QSPI_ADDRESS_NONE;
	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_1_LINE;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	cmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	cmd.Instruction			= instruction;
	cmd.NbData				= length;

	//quadSpiSelectDevice(instance);

	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);
	if (!timeout) {
		status = HAL_QSPI_Receive(hqspi, in, QUADSPI_DEFAULT_TIMEOUT);

		timeout = (status != HAL_OK);
	}

	//quadSpiDeselectDevice(instance);

	if (timeout) {
		return false;
	}

	return true;
}

bool QuadSpiReceiveWithAddress1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize, uint8_t *in, int length)
{
	HAL_StatusTypeDef status;

	QSPI_CommandTypeDef cmd;
	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.AddressMode			= QSPI_ADDRESS_1_LINE;
	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_1_LINE;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	cmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	cmd.Instruction			= instruction;
	cmd.Address				= address;
	cmd.AddressSize			= QuadSpi_addressSizeFromValue(addressSize);
	cmd.NbData				= length;

	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);
	if (!timeout) {
		status = HAL_QSPI_Receive(hqspi, in, QUADSPI_DEFAULT_TIMEOUT);
		timeout = (status != HAL_OK);
	}

	if (timeout) {
		return false;
	}

	return true;
}

bool QuadSpiTransmit1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, const uint8_t *out, int length)
{
	HAL_StatusTypeDef status;


	QSPI_CommandTypeDef cmd;
	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.AddressMode			= QSPI_ADDRESS_NONE;
	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_NONE;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	cmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	cmd.Instruction			= instruction;
	cmd.NbData				= length;

	if (out) {
		cmd.DataMode		= QSPI_DATA_1_LINE;
	}

	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);
	if (!timeout) {
		if (out && length > 0) {
			status = HAL_QSPI_Transmit(hqspi, (uint8_t *)out, QUADSPI_DEFAULT_TIMEOUT);
			timeout = (status != HAL_OK);
		}
	}

	if (timeout) {
		return false;
	}

	return true;
}

bool QuadSpiTransmitWithColumnAddress1LINE(
		QSPI_HandleTypeDef *hqspi,
		uint8_t instruction,
		uint8_t dummyCycles,
		uint32_t address,
		uint8_t addressSize,
		const uint8_t *out,
		int length
		)
{
	HAL_StatusTypeDef status;

	QSPI_CommandTypeDef cmd;
	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.AddressMode			= QSPI_ADDRESS_1_LINE;
	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_1_LINE;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	cmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	cmd.Instruction			= instruction;
	cmd.Address				= address;
	cmd.AddressSize			= QuadSpi_addressSizeFromValue(addressSize);
	cmd.NbData				= length;

	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);

	if (!timeout) {
		status = HAL_QSPI_Transmit(hqspi, (uint8_t *)out, QUADSPI_DEFAULT_TIMEOUT);
		timeout = (status != HAL_OK);
	}

	if (timeout) {
		return false;
	}

	return true;
}

bool QuadSpiInstructionWithAddress1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize)
{
	HAL_StatusTypeDef status;

	QSPI_CommandTypeDef cmd;
	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;
	cmd.AddressMode			= QSPI_ADDRESS_1_LINE;
	cmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_NONE;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	cmd.SIOOMode			= QSPI_SIOO_INST_ONLY_FIRST_CMD;

	cmd.Instruction			= instruction;
	cmd.Address				= address;
	cmd.AddressSize			= QSPI_ADDRESS_24_BITS;
	cmd.NbData				= 0;

	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);


	if (timeout) {
		return false;
	}

	return true;
}

bool QuadSpiReceiveWithAddress4LINES(
		QSPI_HandleTypeDef *hqspi,
		uint8_t instruction,
		uint8_t dummyCycles,
		uint32_t address,
		uint8_t *in,
		int length,
		bool bufferMode)
{
	HAL_StatusTypeDef status;

	QSPI_CommandTypeDef cmd;

	if(bufferMode) {
		cmd.SIOOMode		= QSPI_SIOO_INST_EVERY_CMD;
		cmd.AddressMode		= QSPI_ADDRESS_4_LINES;
	} else {
		cmd.SIOOMode		= QSPI_SIOO_INST_ONLY_FIRST_CMD;
		cmd.AddressMode		= QSPI_ADDRESS_NONE;
	}

	cmd.InstructionMode		= QSPI_INSTRUCTION_1_LINE;

	cmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode			= QSPI_DATA_4_LINES;
	cmd.DummyCycles			= dummyCycles;
	cmd.DdrMode				= QSPI_DDR_MODE_DISABLE;
	cmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;

	cmd.Instruction			= instruction;
	cmd.Address				= address;
	cmd.AddressSize			= QSPI_ADDRESS_16_BITS;
	cmd.NbData				= length;



	status = HAL_QSPI_Command(hqspi, &cmd, QUADSPI_DEFAULT_TIMEOUT);
	bool timeout = (status != HAL_OK);
	if (!timeout) {
		status = HAL_QSPI_Receive(hqspi, in, QUADSPI_DEFAULT_TIMEOUT);
		timeout = (status != HAL_OK);
	}

	if (timeout) {
		return false;
	}

	return true;
}

