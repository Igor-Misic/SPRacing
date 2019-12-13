#include "main.h"
#include <stdbool.h>

typedef  void (*pFunction)(void);
pFunction JumpToApplication;

#define APPLICATION_ADDRESS            QSPI_BASE

#define QUADSPI_DEFAULT_TIMEOUT 10

#define W28N01G_STATUS_COLUMN_ADDRESS_SIZE  16

#define W25N01G_INSTRUCTION_RDID                   0x9F
#define W25N01G_INSTRUCTION_FAST_READ_QUAD         0xEB
#define W25N01G_INSTRUCTION_FAST_READ_QUAD_OUTPUT  0x6B

QSPI_HandleTypeDef hqspi;
uint8_t idBuffer[3];

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_QUADSPI_Init(void);

bool QuadSpiReceive1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint8_t *in, int length);
bool QuadSpiReceiveWithAddress4LINES(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize, uint8_t *in, int length);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_QUADSPI_Init();

  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;
  SCB->VTOR = APPLICATION_ADDRESS;


  JumpToApplication = *( pFunction* )(APPLICATION_ADDRESS + 4u);
  __ASM volatile ("MSR msp, %0" : : "r" (APPLICATION_ADDRESS) : );
  JumpToApplication();

/*
  __ASM volatile("mov r0, %0" : : "r" (APPLICATION_ADDRESS) : );
  __ASM volatile ("MSR msp, %0" : : "r" (APPLICATION_ADDRESS) : );
  __ASM volatile("ldr r0, [r0, #4]");
  __ASM volatile("bx r0");
  */

  while (1)
  {
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable 
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

bool QuadSpiReceive1LINE(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint8_t *in, int length)
{
    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef cmd;
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = dummyCycles;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    cmd.Instruction       = instruction;
    cmd.NbData            = length;

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

/*
bool QuadSpiReceiveWithAddress4LINES(QSPI_HandleTypeDef *hqspi, uint8_t instruction, uint8_t dummyCycles, uint32_t address, uint8_t addressSize, uint8_t *in, int length)
{
    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef cmd;
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_4_LINES;
    cmd.DummyCycles       = dummyCycles;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    cmd.Instruction       = instruction;
    cmd.Address           = address;
    cmd.AddressSize       = quadSpi_addressSizeFromValue(addressSize);
    cmd.NbData            = length;


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
*/

static void MX_QUADSPI_Init(void)
{

  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 23;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }


  QuadSpiReceive1LINE(&hqspi, W25N01G_INSTRUCTION_RDID, 8, idBuffer, 3);
  //QuadSpiReceiveWithAddress4LINES(&hqspi, W25N01G_INSTRUCTION_FAST_READ_QUAD_OUTPUT, 8, column, W28N01G_STATUS_COLUMN_ADDRESS_SIZE, buffer, length);

  QSPI_CommandTypeDef cmd;
  QSPI_MemoryMappedTypeDef memMappedCfg;

  cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction = W25N01G_INSTRUCTION_FAST_READ_QUAD_OUTPUT;
  cmd.AddressMode = QSPI_ADDRESS_4_LINES;
  cmd.AddressSize = QSPI_ADDRESS_24_BITS;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode = QSPI_DATA_4_LINES;
  cmd.DummyCycles = 4;
  cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the memory mapped mode */

  memMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  memMappedCfg.TimeOutPeriod = 0;

  if (HAL_QSPI_MemoryMapped(&hqspi, &cmd, &memMappedCfg) != HAL_OK)
  {
    Error_Handler();
  }

  //it take some time for memory mapped mode to start workingg
  HAL_Delay(600);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED0_Pin */
  GPIO_InitStruct.Pin = LED0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED0_GPIO_Port, &GPIO_InitStruct);

}


void Error_Handler(void)
{
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
