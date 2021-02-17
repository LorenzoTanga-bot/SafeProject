/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "x4m200.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define RX_BUF_LENGTH 64
#define TX_BUF_LENGTH 64
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
unsigned char rxBuf[RX_BUF_LENGTH];
unsigned char txBuf[TX_BUF_LENGTH];

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void sendCommand(unsigned char *cmd, int len);
int receiveData();
void receiveAck();
void resetModule();
void loadRespirationApp();
void configureNoiseMap();
void setSensity(int sensitivity);
void setDetectionZone(float start_zone, float end_zone);
void executeApp();
RespirationData getRespirationData();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	RespirationData data;
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */

	/* USER CODE END 2 */
	resetModule();
	loadRespirationApp();
	configureNoiseMap();
	setSensity(5);
	setDetectionZone(0.40, 2.00);
	executeApp();

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		data = getRespirationData();
		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_3;
	RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */
void sendCommand(unsigned char *cmd, int len) {
	char crc = _xt_start;
	for (int i = 0; i < len; i++)
		crc ^= cmd[i];
	unsigned char tx[len + 3];
	tx[0] = _xt_start;
	memcpy(tx + 1, cmd, len);
	tx[len + 1] = crc;
	tx[len + 2] = _xt_stop;

	HAL_UART_Transmit(&huart2, (uint8_t*) tx, len + 3, HAL_MAX_DELAY);

}

int receiveData() {
	int rxBuflen = 0;
	int rxlen = 0;
	unsigned char rx[RX_BUF_LENGTH];
	HAL_UART_Receive(&huart2, (uint8_t*) rx, RX_BUF_LENGTH, HAL_MAX_DELAY);
	while (true) {
		if (rx[rxlen] == _xt_escape) {
			if (rxlen >= RX_BUF_LENGTH)
				return -1;
			else
				rxlen++;
		}
		if (rx[rxlen] == _xt_start) {
			rxBuf[rxBuflen++] = rx[rxlen++];
			break;
		}
	}
	while (true) {
		if (rxBuflen >= RX_BUF_LENGTH || rxlen >= RX_BUF_LENGTH)
			return -1;
		if (rx[rxlen] == _xt_escape) {
			if (rxlen >= RX_BUF_LENGTH)
				return -1;
			else
				rxlen++;
		}
		if (rx[rxlen] == _xt_start) {
			rxBuflen = 0;
			rxBuf[rxBuflen++] = rx[rxlen++];
		}
		if (rx[rxlen] == _xt_stop) {
			rxBuf[rxBuflen++] = rx[rxlen];
			break;
		}
		rxBuf[rxBuflen++] = rx[rxlen++];
	}

	char crc = 0;
	for (int i = 0; i < rxBuflen - 2; i++)
		crc ^= rxBuf[i];

	if (crc == rxBuf[rxBuflen - 2]) {
		return rxBuflen;
	} else {

		return -1;
	}
}

void receiveAck() {
	while (true) {
		receiveData();
		if (rxBuf[1] == _xts_spr_ack || rxBuf[2] == _xts_spr_ack)
			return;
	}
}

void resetModule() {
	txBuf[0] = _xts_spc_mod_reset;
	sendCommand(txBuf, 1);
	receiveAck();
}

void loadRespirationApp() {
	txBuf[0] = _xts_spc_mod_loadapp;
	memcpy(txBuf + 1, &_xts_id_app_resp_adult, 4);
	sendCommand(txBuf, 5);
	receiveAck();
}

void configureNoiseMap() {
	txBuf[0] = _xts_spc_mod_noisemap;
	txBuf[1] = _xts_spcn_setcontrol;
	txBuf[2] = 0x06; // 0x06: Use default noisemap and adaptive noisemap
	txBuf[3] = 0x00;
	txBuf[4] = 0x00;
	txBuf[5] = 0x00;
	sendCommand(txBuf, 6);
	receiveAck();
}

void setSensity(int sensitivity) {
	txBuf[0] = _xts_spc_appcommand;
	txBuf[1] = _xts_spca_set;
	memcpy(txBuf + 2, &_xts_id_sensitivity, 4);
	memcpy(txBuf + 6, &sensitivity, 4);
	sendCommand(txBuf, 10);
	receiveAck();
}

void setDetectionZone(float start_zone, float end_zone) {
	txBuf[0] = _xts_spc_appcommand;
	txBuf[1] = _xts_spca_set;

	memcpy(txBuf + 2, &_xts_id_detection_zone, 4);
	memcpy(txBuf + 6, &start_zone, 4);
	memcpy(txBuf + 10, &end_zone, 4);

	sendCommand(txBuf, 14);
	receiveAck();
}

void executeApp() {
	txBuf[0] = _xts_spc_mod_setmode;
	txBuf[1] = _xts_sm_run;

	sendCommand(txBuf, 2);
}

RespirationData getRespirationData() {
	RespirationData data;

	if (receiveData() < 0 || rxBuf[1] != _xts_spr_appdata) {
		data.valid = false;
		return data;
	}
	memcpy(&data.code, &rxBuf[10], 4);
	if (data.code == _xts_val_resp_state_breathing) {
		memcpy(&data.rpm, &rxBuf[14], 4);
		memcpy(&data.movement, &rxBuf[22], 4);
	}

	data.valid = true;
	return data;

}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
