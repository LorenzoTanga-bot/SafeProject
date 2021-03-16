/**
 ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @brief   this is the main!
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw.h"
#include "low_power_manager.h"
#include "lora.h"
#include "bsp.h"
#include "timeServer.h"
#include "vcom.h"
#include "version.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

//X4M200.h
typedef struct RespirationData {
	bool valid;
	float movement;
	int rpm;
	char code;
} RespirationData;

// State codes for respiration
const unsigned char _xts_val_resp_state_breathing = 0; // Valid RPM detected Current RPM value
const unsigned char _xts_val_resp_state_movement = 1; // Detects motion, but can not identify breath 0
const unsigned char _xts_val_resp_state_movement_tracking = 2; // Detects motion, possible breathing 0
const unsigned char _xts_val_resp_state_no_movement = 3; // No movement detected 0
const unsigned char _xts_val_resp_state_initializing = 4; // No movement detected 0
const unsigned char _xts_val_resp_state_unknown = 6;

// Radar constants
const unsigned char _xt_start = 0x7D;
const unsigned char _xt_stop = 0x7E;
const unsigned char _xt_escape = 0x7F;

const unsigned char _xts_spc_appcommand = 0x10;
const unsigned char _xts_spc_mod_setmode = 0x20;
const unsigned char _xts_spc_mod_loadapp = 0x21;
const unsigned char _xts_spc_mod_reset = 0x22;
const unsigned char _xts_spc_mod_setledcontrol = 0x24;
const unsigned char _xts_spc_mod_noisemap = 0x25;

const unsigned char _xts_spr_appdata = 0x50;
const unsigned char _xts_spr_system = 0x30;
const unsigned char _xts_spr_ack = 0x10;

// PING command
const unsigned char _xts_spc_ping = 0x01;                   // Ping command code
const unsigned long _xts_def_pingval = 0xeeaaeaae;           // Ping seed value
const unsigned char _xts_spr_pong = 0x01;                  // Pong responce code
const unsigned long _xts_def_pongval_ready = 0xaaeeaeea;     // Module is ready
const unsigned long _xts_def_pongval_notready = 0xaeeaeeaa; // Module is not ready

const unsigned char _xts_spc_dir_command = 0x90;
const unsigned char _xts_sdc_app_setint = 0x71;
const unsigned char _xts_sdc_comm_setbaudrate = 0x80;

const unsigned long _xts_sacr_outputbaseband = 0x00000010;
const unsigned long _xts_sacr_id_baseband_output_off = 0x00000000;
const unsigned long _xts_sacr_id_baseband_output_amplitude_phase = 0x00000002;

const unsigned char _xts_spca_set = 0x10;

const unsigned long _xts_id_app_resp = 0x1423a2d6;
const unsigned long _xts_id_app_resp_adult = 0x064e57ad;
const unsigned long _xts_id_app_sleep = 0x00f17b17;
const unsigned long _xts_id_resp_status = 0x2375fe26;
const unsigned long _xts_id_sleep_status = 0x2375a16c;
const unsigned long _xts_id_detection_zone = 0x96a10a1c;
const unsigned long _xts_id_sensitivity = 0x10a5112b;

const unsigned char _xts_sm_run = 0x01;
const unsigned char _xts_sm_normal = 0x10;
const unsigned char _xts_sm_idle = 0x11;
const unsigned char _xts_sm_stop = 0x13;
const unsigned char _xts_sm_manua = 0x12;

const unsigned char _xts_sprs_booting = 0x10;
const unsigned char _xts_sprs_ready = 0x11;

const unsigned char _xt_ui_led_simple = 0x01;
const unsigned char _xt_ui_led_full = 0x02;
const unsigned char _xt_ui_led_off = 0x00;

const unsigned char _xts_spcn_setcontrol = 0x10;
const unsigned char _xts_spco_setcontrol = 0x10;
const unsigned char _xts_spc_output = 0x41;

const unsigned char _xtid_output_control_disable = 0x00;
const unsigned char _xtid_output_control_enable = 0x01;

#define LORAWAN_MAX_BAT   254

/*!
 * CAYENNE_LPP is myDevices Application server.
 */
//#define CAYENNE_LPP
#define LPP_DATATYPE_DIGITAL_INPUT  0x0
#define LPP_DATATYPE_DIGITAL_OUTPUT 0x1
#define LPP_DATATYPE_HUMIDITY       0x68
#define LPP_DATATYPE_TEMPERATURE    0x67
#define LPP_DATATYPE_BAROMETER      0x73
#define LPP_APP_PORT 99
/*!
 * Defines the application data transmission duty cycle. 5s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            10000
/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE LORAWAN_ADR_ON
/*!
 * LoRaWAN Default data Rate Data Rate
 * @note Please note that LORAWAN_DEFAULT_DATA_RATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATA_RATE DR_0
/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
#define LORAWAN_APP_PORT                            2
/*!
 * LoRaWAN default endNode class port
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A
/*!
 * LoRaWAN default confirm state
 */
#define LORAWAN_DEFAULT_CONFIRM_MSG_STATE           LORAWAN_UNCONFIRMED_MSG
/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFF_SIZE                           64
/*!
 * User application data
 */
static uint8_t AppDataBuff[LORAWAN_APP_DATA_BUFF_SIZE];

/*!
 * User application data structure
 */
//static lora_AppData_t AppData={ AppDataBuff,  0 ,0 };
lora_AppData_t AppData = { AppDataBuff, 0, 0 };

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* call back when LoRa endNode has received a frame*/
static void LORA_RxData(lora_AppData_t *AppData);

/* call back when LoRa endNode has just joined*/
static void LORA_HasJoined(void);

/* call back when LoRa endNode has just switch the class*/
static void LORA_ConfirmClass(DeviceClass_t Class);

/* call back when server needs endNode to send a frame*/
static void LORA_TxNeeded(void);

/* callback to get the battery level in % of full charge (254 full charge, 0 no charge)*/
static uint8_t LORA_GetBatteryLevel(void);

/* LoRa endNode send request*/
static void Send(void *context);

/* start the tx process*/
static void LoraStartTx(TxEventType_t EventType);

/* tx timer callback function*/
static void OnTxTimerEvent(void *context);

/* tx timer callback function*/
static void LoraMacProcessNotify(void);

/* Private variables ---------------------------------------------------------*/
#define RX_BUF_LENGTH 256
#define TX_BUF_LENGTH 64

__IO ITStatus UartReady = SET;
UART_HandleTypeDef huart2;
unsigned char rxBuf[RX_BUF_LENGTH];
unsigned char txBuf[TX_BUF_LENGTH];

static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void sendCommand(unsigned char *cmd, int len);
int receiveData();
void receiveAck();
void stopModule();
void resetModule();
void loadRespirationApp();
void configureNoiseMap();
void setSensity(int sensitivity);
void setDetectionZone(float start_zone, float end_zone);
void executeApp();
RespirationData getRespirationData();



/* load Main call backs structure*/
static LoRaMainCallback_t LoRaMainCallbacks = { LORA_GetBatteryLevel,
		HW_GetTemperatureLevel, HW_GetUniqueId, HW_GetRandomSeed, LORA_RxData,
		LORA_HasJoined, LORA_ConfirmClass, LORA_TxNeeded, LoraMacProcessNotify };
LoraFlagStatus LoraMacProcessRequest = LORA_RESET;
LoraFlagStatus AppProcessRequest = LORA_RESET;
/*!
 * Specifies the state of the application LED
 */
static uint8_t AppLedStateOn = RESET;

static TimerEvent_t TxTimer;

#ifdef USE_B_L072Z_LRWAN1
/*!
 * Timer to handle the application Tx Led to toggle
 */
static TimerEvent_t TxLedTimer;
static void OnTimerLedEvent(void *context);
#endif
/* !
 *Initialises the Lora Parameters
 */
static LoRaParam_t LoRaParamInit = { LORAWAN_ADR_STATE,
LORAWAN_DEFAULT_DATA_RATE,
LORAWAN_PUBLIC_NETWORK };

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
	/* STM32 HAL library initialization*/
	HAL_Init();

	/* Configure the system clock*/
	SystemClock_Config();

	/* Configure the debug mode*/
	//DBG_Init();

	/* USER CODE BEGIN 1 */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	PRINTF("Initialize done\r\n\n");

	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
	stopModule();
	PRINTF("Stop Module\r\n");
	resetModule();
	PRINTF("Reset Module\r\n");
	loadRespirationApp();
	PRINTF("Load respiration app\r\n");
	configureNoiseMap();
	PRINTF("Configure noise map\r\n");
	setSensity(5);
	PRINTF("set sensity\r\n");
	setDetectionZone(0.40, 2.00);
	PRINTF("set detection zone\r\n");
	executeApp();
	PRINTF("execute app\r\n");
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
	/* USER CODE END 1 */

	/*Disbale Stand-by mode*/
	LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

	/* Configure the Lora Stack*/
	LORA_Init(&LoRaMainCallbacks, &LoRaParamInit);

	LORA_Join();

	LoraStartTx(TX_ON_TIMER);

	while (1) {
		if (AppProcessRequest == LORA_SET) {
			/*reset notification flag*/
			AppProcessRequest = LORA_RESET;
			/*Send*/
			Send(NULL);
		}
		if (LoraMacProcessRequest == LORA_SET) {
			/*reset notification flag*/
			LoraMacProcessRequest = LORA_RESET;
			LoRaMacProcess();
		}
		/*If a flag is set at this point, mcu must not enter low power and must loop*/
		DISABLE_IRQ();

		/* if an interrupt has occurred after DISABLE_IRQ, it is kept pending
		 * and cortex will not enter low power anyway  */
		if ((LoraMacProcessRequest != LORA_SET)
				&& (AppProcessRequest != LORA_SET)) {
#ifndef LOW_POWER_DISABLE
			LPM_EnterLowPower();
#endif
		}

		ENABLE_IRQ();

		/* USER CODE BEGIN 2 */
		/* USER CODE END 2 */
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
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */
	HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
	/* USER CODE END USART2_Init 2 */

}

static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
			GPIO_PIN_RESET);

	/*Configure GPIO pins : PB5 PB6 PB7 */
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	UartReady = SET;
}

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
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);


}

int receiveData(int lengthDataReceive) {
	int rxBuflen = 0;
	int rxlen = 0;
	unsigned char rx[lengthDataReceive];

	if(UartReady == SET) {
		    UartReady = RESET;
		    HAL_UART_Receive_IT(&huart2, (uint8_t*) rx, lengthDataReceive);
	}
	while(UartReady == RESET);
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);

	while (true) {
		if (rx[rxlen] == _xt_escape) {
			if (rxlen >= lengthDataReceive)
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
		if (rxBuflen >= RX_BUF_LENGTH || rxlen >= lengthDataReceive)
			return -1;
		if (rx[rxlen] == _xt_escape) {
			if (rxlen >= lengthDataReceive)
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
		receiveData(4);
		if (rxBuf[1] == _xts_spr_ack || rxBuf[2] == _xts_spr_ack)
			return;
	}
}
void stopModule() {
	txBuf[0] = _xts_spc_mod_setmode;
	txBuf[1] = _xts_sm_stop;

	sendCommand(txBuf, 2);
	receiveAck();

}

void resetModule() {
	txBuf[0] = _xts_spc_mod_reset;
	sendCommand(txBuf, 1);
	receiveAck();
	while (true)
		if (receiveData(8) > 0 && rxBuf[1] == _xts_spr_system
				&& rxBuf[2] == _xts_sprs_ready)
			break;
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
	receiveAck();
}

RespirationData getRespirationData() {
	RespirationData data;
	int receiveLenght = receiveData(36);

	if (receiveLenght < 0 || rxBuf[1] != _xts_spr_appdata) {
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

void LoraMacProcessNotify(void) {
	LoraMacProcessRequest = LORA_SET;
}

static void LORA_HasJoined(void) {
#if( OVER_THE_AIR_ACTIVATION != 0 )
	PRINTF("JOINED\n\r");
#endif
	LORA_RequestClass(LORAWAN_DEFAULT_CLASS);
}

static void Send(void *context) {
	/* USER CODE BEGIN 3 */
	uint16_t pressure = 0;
	int16_t temperature = 0;
	uint16_t humidity = 0;
	uint8_t batteryLevel;
	sensor_t sensor_data;

	if (LORA_JoinStatus() != LORA_SET) {
		/*Not joined, try again later*/
		LORA_Join();
		return;
	}

	TVL1(PRINTF("SEND REQUEST\n\r");)
#ifndef CAYENNE_LPP
	int32_t latitude, longitude = 0;
	uint16_t altitudeGps = 0;
#endif

#ifdef USE_B_L072Z_LRWAN1
	TimerInit(&TxLedTimer, OnTimerLedEvent);

	TimerSetValue(&TxLedTimer, 200);

	LED_On(LED_RED1);

	TimerStart(&TxLedTimer);
#endif

	BSP_sensor_Read(&sensor_data);

#ifdef CAYENNE_LPP
  uint8_t cchannel = 0;
  temperature = (int16_t)(sensor_data.temperature * 10);         /* in �C * 10 */
  pressure    = (uint16_t)(sensor_data.pressure * 100 / 10);      /* in hPa / 10 */
  humidity    = (uint16_t)(sensor_data.humidity * 2);            /* in %*2     */
  uint32_t i = 0;

  batteryLevel = LORA_GetBatteryLevel();                      /* 1 (very low) to 254 (fully charged) */

  AppData.Port = LPP_APP_PORT;

  AppData.Buff[i++] = cchannel++;
  AppData.Buff[i++] = LPP_DATATYPE_BAROMETER;
  AppData.Buff[i++] = (pressure >> 8) & 0xFF;
  AppData.Buff[i++] = pressure & 0xFF;
  AppData.Buff[i++] = cchannel++;
  AppData.Buff[i++] = LPP_DATATYPE_TEMPERATURE;
  AppData.Buff[i++] = (temperature >> 8) & 0xFF;
  AppData.Buff[i++] = temperature & 0xFF;
  AppData.Buff[i++] = cchannel++;
  AppData.Buff[i++] = LPP_DATATYPE_HUMIDITY;
  AppData.Buff[i++] = humidity & 0xFF;
#if defined( REGION_US915 ) || defined ( REGION_AU915 ) || defined ( REGION_AS923 )
  /* The maximum payload size does not allow to send more data for lowest DRs */
#else
  AppData.Buff[i++] = cchannel++;
  AppData.Buff[i++] = LPP_DATATYPE_DIGITAL_INPUT;
  AppData.Buff[i++] = batteryLevel * 100 / 254;
  AppData.Buff[i++] = cchannel++;
  AppData.Buff[i++] = LPP_DATATYPE_DIGITAL_OUTPUT;
  AppData.Buff[i++] = AppLedStateOn;
#endif  /* REGION_XX915 */
#else  /* not CAYENNE_LPP */

	temperature = (int16_t) (sensor_data.temperature * 100); /* in �C * 100 */
	pressure = (uint16_t) (sensor_data.pressure * 100 / 10); /* in hPa / 10 */
	humidity = (uint16_t) (sensor_data.humidity * 10); /* in %*10     */
	latitude = sensor_data.latitude;
	longitude = sensor_data.longitude;
	uint32_t i = 0;

	batteryLevel = LORA_GetBatteryLevel(); /* 1 (very low) to 254 (fully charged) */

	AppData.Port = LORAWAN_APP_PORT;

#if defined( REGION_US915 ) || defined ( REGION_AU915 ) || defined ( REGION_AS923 )
  AppData.Buff[i++] = AppLedStateOn;
  AppData.Buff[i++] = (pressure >> 8) & 0xFF;
  AppData.Buff[i++] = pressure & 0xFF;
  AppData.Buff[i++] = (temperature >> 8) & 0xFF;
  AppData.Buff[i++] = temperature & 0xFF;
  AppData.Buff[i++] = (humidity >> 8) & 0xFF;
  AppData.Buff[i++] = humidity & 0xFF;
  AppData.Buff[i++] = batteryLevel;
  AppData.Buff[i++] = 0;
  AppData.Buff[i++] = 0;
  AppData.Buff[i++] = 0;
#else  /* not REGION_XX915 */
	AppData.Buff[i++] = AppLedStateOn;
	AppData.Buff[i++] = (pressure >> 8) & 0xFF;
	AppData.Buff[i++] = pressure & 0xFF;
	AppData.Buff[i++] = (temperature >> 8) & 0xFF;
	AppData.Buff[i++] = temperature & 0xFF;
	AppData.Buff[i++] = (humidity >> 8) & 0xFF;
	AppData.Buff[i++] = humidity & 0xFF;
	AppData.Buff[i++] = batteryLevel;
	AppData.Buff[i++] = (latitude >> 16) & 0xFF;
	AppData.Buff[i++] = (latitude >> 8) & 0xFF;
	AppData.Buff[i++] = latitude & 0xFF;
	AppData.Buff[i++] = (longitude >> 16) & 0xFF;
	AppData.Buff[i++] = (longitude >> 8) & 0xFF;
	AppData.Buff[i++] = longitude & 0xFF;
	AppData.Buff[i++] = (altitudeGps >> 8) & 0xFF;
	AppData.Buff[i++] = altitudeGps & 0xFF;
#endif  /* REGION_XX915 */
#endif  /* CAYENNE_LPP */
	AppData.BuffSize = i;

	LORA_send(&AppData, LORAWAN_DEFAULT_CONFIRM_MSG_STATE);

	/* USER CODE END 3 */
}

static void LORA_RxData(lora_AppData_t *AppData) {
	/* USER CODE BEGIN 4 */
	PRINTF("PACKET RECEIVED ON PORT %d\n\r", AppData->Port);

	switch (AppData->Port) {
	case 3:
		/*this port switches the class*/
		if (AppData->BuffSize == 1) {
			switch (AppData->Buff[0]) {
			case 0: {
				LORA_RequestClass(CLASS_A);
				break;
			}
			case 1: {
				LORA_RequestClass(CLASS_B);
				break;
			}
			case 2: {
				LORA_RequestClass(CLASS_C);
				break;
			}
			default:
				break;
			}
		}
		break;
	case LORAWAN_APP_PORT:
		if (AppData->BuffSize == 1) {
			AppLedStateOn = AppData->Buff[0] & 0x01;
			if (AppLedStateOn == RESET) {
				PRINTF("LED OFF\n\r");
				LED_Off(LED_BLUE);
			} else {
				PRINTF("LED ON\n\r");
				LED_On(LED_BLUE);
			}
		}
		break;
	case LPP_APP_PORT: {
		AppLedStateOn = (AppData->Buff[2] == 100) ? 0x01 : 0x00;
		if (AppLedStateOn == RESET) {
			PRINTF("LED OFF\n\r");
			LED_Off(LED_BLUE);

		} else {
			PRINTF("LED ON\n\r");
			LED_On(LED_BLUE);
		}
		break;
	}
	default:
		break;
	}
	/* USER CODE END 4 */
}

static void OnTxTimerEvent(void *context) {
	/*Wait for next tx slot*/
	TimerStart(&TxTimer);

	AppProcessRequest = LORA_SET;
}

static void LoraStartTx(TxEventType_t EventType) {
	if (EventType == TX_ON_TIMER) {
		/* send everytime timer elapses */
		TimerInit(&TxTimer, OnTxTimerEvent);
		TimerSetValue(&TxTimer, APP_TX_DUTYCYCLE);
		OnTxTimerEvent(NULL);
	} else {
		/* send everytime button is pushed */
		GPIO_InitTypeDef initStruct = { 0 };

		initStruct.Mode = GPIO_MODE_IT_RISING;
		initStruct.Pull = GPIO_PULLUP;
		initStruct.Speed = GPIO_SPEED_HIGH;

		HW_GPIO_Init(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN, &initStruct);
		HW_GPIO_SetIrq(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN, 0, Send);
	}
}

static void LORA_ConfirmClass(DeviceClass_t Class) {
	PRINTF("switch to class %c done\n\r", "ABC"[Class]);

	/*Optionnal*/
	/*informs the server that switch has occurred ASAP*/
	AppData.BuffSize = 0;
	AppData.Port = LORAWAN_APP_PORT;

	LORA_send(&AppData, LORAWAN_UNCONFIRMED_MSG);
}

static void LORA_TxNeeded(void) {
	AppData.BuffSize = 0;
	AppData.Port = LORAWAN_APP_PORT;

	LORA_send(&AppData, LORAWAN_UNCONFIRMED_MSG);
}

/**
 * @brief This function return the battery level
 * @param none
 * @retval the battery level  1 (very low) to 254 (fully charged)
 */
uint8_t LORA_GetBatteryLevel(void) {
	uint16_t batteryLevelmV;
	uint8_t batteryLevel = 0;

	batteryLevelmV = HW_GetBatteryLevel();

	/* Convert batterey level from mV to linea scale: 1 (very low) to 254 (fully charged) */
	if (batteryLevelmV > VDD_BAT) {
		batteryLevel = LORAWAN_MAX_BAT;
	} else if (batteryLevelmV < VDD_MIN) {
		batteryLevel = 0;
	} else {
		batteryLevel =
				(((uint32_t) (batteryLevelmV - VDD_MIN) * LORAWAN_MAX_BAT)
						/ (VDD_BAT - VDD_MIN));
	}

	return batteryLevel;
}

#ifdef USE_B_L072Z_LRWAN1
static void OnTimerLedEvent(void *context) {
	LED_Off(LED_RED1);
}
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
