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
#include <stdlib.h>
#include "timer.h"

/* Private typedef -----------------------------------------------------------*/
//X4M200.h
typedef struct RespirationInfo {
	uint8_t codeState;
	uint8_t rpm[4];
	uint8_t distance[4];
	uint8_t movement[4];
	uint8_t signalQuality[4];
} RespirationInfo;

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
const unsigned char _xts_spc_ping = 0x01;					// Ping command code
const unsigned long _xts_def_pingval = 0xeeaaeaae;			// Ping seed value
const unsigned char _xts_spr_pong = 0x01;				// Pong responce code
const unsigned long _xts_def_pongval_ready = 0xaaeeaeea;	// Module is ready
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

/* Private define ------------------------------------------------------------*/

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
#define RX_BUF_LENGTH 64
#define RX_LEN 36
#define TX_BUF_LENGTH 64

UART_HandleTypeDef huartHandle1;
__IO ITStatus UartReady = SET;
bool initComunication = false;
unsigned char rxBuf[RX_BUF_LENGTH];
int rxBufferlen = 0;
unsigned char rx[RX_LEN];
bool beforeEscape = false;
unsigned char txBuf[TX_BUF_LENGTH];

bool profile = false; 					//if 0 PeaceProfile else WarProfile
const unsigned char _profilePeace = 0x30;
const unsigned char _profileWar = 0x31;

RespirationInfo infoSensors;

time_t start, end;
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
/* load Main call backs structure*/
static LoRaMainCallback_t LoRaMainCallbacks = { LORA_GetBatteryLevel,
		HW_GetTemperatureLevel, HW_GetUniqueId, HW_GetRandomSeed, LORA_RxData,
		LORA_HasJoined, LORA_ConfirmClass, LORA_TxNeeded, LoraMacProcessNotify };
LoraFlagStatus LoraMacProcessRequest = LORA_RESET;
LoraFlagStatus AppProcessRequest = LORA_RESET;
/*!
 * Specifies the state of the application LED
 */

static TimerEvent_t TxTimer;

/* !
 *Initialises the Lora Parameters
 */
static LoRaParam_t LoRaParamInit = { LORAWAN_ADR_STATE,
LORAWAN_DEFAULT_DATA_RATE,
LORAWAN_PUBLIC_NETWORK };

/* Private functions ---------------------------------------------------------*/
void USART1_IRQHandler(void) {
	HAL_UART_IRQHandler(&huartHandle1);
}

void initComunicationLogic() {
	if (rxBufferlen == 0 && rx[0] == _xt_start) {
		memset(rxBuf, 0, RX_BUF_LENGTH);
		rxBuf[rxBufferlen++] = _xt_start;
	} else {
		if (beforeEscape) {
			rxBuf[rxBufferlen++] = rx[0];
			beforeEscape = false;
		} else if (rx[0] == _xt_escape)
			beforeEscape = true;
		else
			rxBuf[rxBufferlen++] = rx[0];
	}

	if (rx[0] == _xt_stop && !beforeEscape) {
		UartReady = SET;
#if DEBUGUART
			PRINTF("Rx: ");
			for (int i = 0; i < rxBufferlen; i++)
				PRINTF("%2x ", rxBuf[i]);
			PRINTF("\r\n");
#endif
	} else
		HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, 1);
}

bool crcCalculator(unsigned char *buffer, int lenght) {
	char crc = 0;
	for (int i = 0; i < lenght - 2; i++)
		crc ^= buffer[i];

	if (crc == buffer[lenght - 2]) {
		return true;
	} else {
		return false;
	}
}

void normalComunicationLogic() {
	if (beforeEscape) {
		if (rx[0] == _xt_stop) {
			beforeEscape = false;
			memset(rx, 0, RX_LEN);
			HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, RX_LEN);

		} else {
			HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, 1);

		}
		return;
	}

	if (rx[0] == _xt_start && rx[RX_LEN - 1] == _xt_stop
			&& crcCalculator(rx, RX_LEN)) {
		memcpy(&infoSensors.codeState, &rx[10], 1);
		PRINTF("Respiration code -> %2x\r\n", infoSensors.codeState);
		if (infoSensors.codeState == 0) {
			memcpy(infoSensors.rpm, &rx[14], 4);
			memcpy(infoSensors.distance, &rx[18], 4);
			memcpy(infoSensors.movement, &rx[22], 4);
			memcpy(infoSensors.signalQuality, &rx[26], 4);

		}

		memset(rx, 0, RX_LEN);
		UartReady = SET;
	} else {
		beforeEscape = true;
		memset(rx, 0, RX_LEN);
		HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, 1);
	}
}

/* This callback is called by the HAL_UART_IRQHandler when the given number of bytes are received */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (USART1 == huartHandle1.Instance) {
		if (initComunication) {
			initComunicationLogic();
			return;
		}
		normalComunicationLogic();

	}
}

void huartHandleInit() {
	huartHandle1.Instance = USART1;
	huartHandle1.Init.BaudRate = 115200;
	huartHandle1.Init.WordLength = UART_WORDLENGTH_8B;
	huartHandle1.Init.StopBits = UART_STOPBITS_1;
	huartHandle1.Init.Parity = UART_PARITY_NONE;
	huartHandle1.Init.Mode = UART_MODE_TX_RX;
	huartHandle1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huartHandle1.Init.OverSampling = UART_OVERSAMPLING_16;

	HAL_UART_Init(&huartHandle1);
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void gpioInit() {
	GPIO_InitTypeDef GPIO_InitStruct;
	__GPIOA_CLK_ENABLE()
	;
	__USART1_CLK_ENABLE();

	/*		 USART1 GPIO Configuration
	 PA9     	------> USART1_TX
	 PA10     	------> USART1_RX
	 */

	GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF4_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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

	HAL_UART_Transmit(&huartHandle1, (uint8_t*) tx, len + 3, HAL_MAX_DELAY);
}

int receiveData() {

	if (UartReady == SET) {
		UartReady = RESET;
		rxBufferlen = 0;
		HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, 1);
	}
	while (UartReady == RESET)
		;

	char crc = 0;
	for (int i = 0; i < rxBufferlen - 2; i++)
		crc ^= rxBuf[i];

	if (crc == rxBuf[rxBufferlen - 2]) {
		return rxBufferlen;
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
		if (receiveData() > 0 && rxBuf[1] == _xts_spr_system
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

void setSensity(long sensitivity) {
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

void x4m200Init() {
	initComunication = true;
	stopModule();
	PRINTF("Stop Module\r\n");
	resetModule();
	PRINTF("Reset Module\r\n");
	loadRespirationApp();
	PRINTF("Load respiration app\r\n");
	configureNoiseMap();
	PRINTF("Configure noise map\r\n");
	setSensity(9);
	PRINTF("set sensity\r\n");
	setDetectionZone(0.40, 2.00);
	PRINTF("set detection zone\r\n");
	executeApp();
	PRINTF("execute app\r\n");
	initComunication = false;
}

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
	DBG_Init();

	/* Configure the hardware*/
	HW_Init();

	/* USER CODE BEGIN 1 */

	gpioInit();

	huartHandleInit();

	x4m200Init();
	/* USER CODE END 1 */

	/*Disbale Stand-by mode*/
	LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

	PRINTF("APP_VERSION= %02X.%02X.%02X.%02X\r\n",
			(uint8_t)(__APP_VERSION >> 24), (uint8_t)(__APP_VERSION >> 16),
			(uint8_t)(__APP_VERSION >> 8), (uint8_t)__APP_VERSION);
	PRINTF("MAC_VERSION= %02X.%02X.%02X.%02X\r\n",
			(uint8_t)(__LORA_MAC_VERSION >> 24),
			(uint8_t)(__LORA_MAC_VERSION >> 16),
			(uint8_t)(__LORA_MAC_VERSION >> 8), (uint8_t)__LORA_MAC_VERSION);

	infoSensors.codeState = 4;

	/* Configure the Lora Stack*/
	LORA_Init(&LoRaMainCallbacks, &LoRaParamInit);

	LORA_Join();

	LoraStartTx(TX_ON_TIMER);

	uint32_t time = HW_RTC_Tick2ms(HW_RTC_GetTimerValue());

	while (1) {
		if (UartReady == SET) {
			UartReady = RESET;
			HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, RX_LEN);
		}

		if (infoSensors.codeState == 6)
			x4m200Init();

		if (profile) {
			if (AppProcessRequest == LORA_SET) {
				/*reset notification flag*/
				AppProcessRequest = LORA_RESET;
				/*Send*/
				Send(NULL);
			}
		} else if ((HW_RTC_Tick2ms(HW_RTC_GetTimerValue()) - time) / 1000
				> 300) {
			time = HW_RTC_Tick2ms(HW_RTC_GetTimerValue());
			if (AppProcessRequest == LORA_SET) {
				/*reset notification flag*/
				AppProcessRequest = LORA_RESET;
				/*Send*/
				Send(NULL);
			}
		}

		if (LoraMacProcessRequest == LORA_SET) {
			/*reset notification flag*/
			LoraMacProcessRequest = LORA_RESET;
			LoRaMacProcess();
		}
	}
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
	if (LORA_JoinStatus() != LORA_SET) {
		/*Not joined, try again later*/
		LORA_Join();
		return;
	}

	int i = 1;
	int j = 0;

	if (profile)
		memcpy(AppData.Buff, &i, 1); //Profile war
	else
		memcpy(AppData.Buff, &j, 1); //Profile peace

	memcpy(AppData.Buff + 1, &infoSensors.codeState, 1);
	if ((!profile && infoSensors.codeState == 0)
			|| (profile && infoSensors.codeState < 3)) {
		memcpy(AppData.Buff + 2, &infoSensors.distance, 4);
		memcpy(AppData.Buff + 6, &infoSensors.rpm, 4);
		memcpy(AppData.Buff + 10, &infoSensors.movement, 4);
		memcpy(AppData.Buff + 14, &infoSensors.signalQuality, 4);
		AppData.BuffSize = 18;
	} else
		AppData.BuffSize = 2;

	AppData.Port = LORAWAN_APP_PORT;

	LORA_send(&AppData, LORAWAN_DEFAULT_CONFIRM_MSG_STATE);
}

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
static void LORA_RxData(lora_AppData_t *AppData) {
	switch (AppData->Port) {
	case LORAWAN_APP_PORT:

		if (AppData->Buff[0] == _profilePeace)
			profile = false;
		else if (AppData->Buff[0] == _profileWar)
			profile = true;
		PRINTF("Profile %d\n\r", profile);
		break;

	default:
		PRINTF("PACKET RECEIVED ON PORT %d\n\r", AppData->Port);
		PRINTF("BuffSize: %d\n\r", AppData->BuffSize);
		PRINTF("Buff: ");
		for (int i = 0; i < AppData->BuffSize; i++)
			PRINTF("%2x ", AppData->Buff[i]);
		PRINTF("\r\n");
	}
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
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
