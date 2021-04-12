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
	char code; //Profile state, see StateCode value table below.
	int rpm; //respirations per minute (Breathing state only)
	float distance; //Distance to where respiration is detected (Breathing state only)
	float movement; //Relative movement of the respiration, in mm (Breathing state only)
	int signalQuality; //A measure of the signal quality. Typically used to identify
					   //if the sensor is positioned correctly. Value from 0 to 10 where
					   //0=low and 10=high. (Breathing state only).
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

//LoRa DATA
typedef struct MessageLoRa {
	char codeState;
	int signalQualityAvg;
	float DistanceAvg;
	int rpmAvg;
} MessageLoRa;

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
#define APP_TX_DUTYCYCLE                            4000
/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE LORAWAN_ADR_OFF
/*!
 * LoRaWAN Default data Rate Data Rate
 * @note Please note that LORAWAN_DEFAULT_DATA_RATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATA_RATE DR_5
/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
#define LORAWAN_APP_PORT                            5
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
#define TX_BUF_LENGTH 64

__IO ITStatus UartReady = SET;
unsigned char rxBuf[RX_BUF_LENGTH];
int rxBufferlen = 0;
unsigned char rx[1];
bool beforeEscape = false;
unsigned char txBuf[TX_BUF_LENGTH];
bool debugFlag = false;
int codeStateCount[5];
float distanceAvg = 0.0;
int distanceAvgCount = 0;
int rpmAvg = 0;
int rpmAvgCount = 0;

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
bool sendingPacket = false;

struct initiatorInfo {
	uint8_t panId[2];
	uint8_t sourceId[2];
};

struct initiatorInfo initiator;

struct responderInfo {
	uint8_t destinationId[2];
	uint8_t distance[4];
	uint16_t timeout;
};

struct responderInfo responders[10];
uint8_t lastRespIndex = -1;

UART_HandleTypeDef huartHandle1;

int hex_to_val(const char ch) {
	if (ch >= '0' && ch <= '9')
		return ch - '0'; /* Simple ASCII arithmetic */
	else if (ch >= 'a' && ch <= 'f')
		return 10 + ch - 'a'; /* Because hex-digit a is ten */
	else if (ch >= 'A' && ch <= 'F')
		return 10 + ch - 'A'; /* Because hex-digit A is ten */
	else
		return -1; /* Not a valid hexadecimal digit */
}

void USART1_IRQHandler(void) {
	HAL_UART_IRQHandler(&huartHandle1);
}

/* This callback is called by the HAL_UART_IRQHandler when the given number of bytes are received */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (USART1 == huartHandle1.Instance && !sendingPacket) {
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

			if (debugFlag) {
				PRINTF("Rx: ");
				for (int i = 0; i < rxBufferlen; i++)
					PRINTF("%2x ", rxBuf[i]);
				PRINTF("\r\n");
			}

			UartReady = SET;

		} else
			HAL_UART_Receive_IT(&huartHandle1, (uint8_t*) rx, 1);

		__HAL_UART_FLUSH_DRREGISTER(&huartHandle1);
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

	if (debugFlag) {
		PRINTF("Tx: ");
		for (int i = 0; i < len + 3; i++)
			PRINTF("%2x ", tx[i]);
		PRINTF("\r\n");
	}

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

void x4m200Init(){
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
}

RespirationData getRespirationData() {
	RespirationData data;
	data.code = 6;
	data.distance = 0.0;
	data.movement = 0.0;
	data.rpm = 0;
	data.signalQuality = 0;
	int receiveLenght = receiveData(36);

	if (receiveLenght < 0 || rxBuf[1] != _xts_spr_appdata) {
		data.valid = false;
		return data;
	}
	memcpy(&data.code, &rxBuf[10], 4);
	if (data.code == 0) {
		memcpy(&data.rpm, &rxBuf[14], 4);
		memcpy(&data.distance, &rxBuf[18], 4);
		memcpy(&data.movement, &rxBuf[22], 4);
		memcpy(&data.signalQuality, &rxBuf[26], 4);
	}

	data.valid = true;
	return data;
}

void analyzeResporationData(RespirationData *data) {

	switch (data->code) {
	case 0:
		if (distanceAvgCount > 1 && data->distance != 0.0)distanceAvg = (distanceAvgCount * distanceAvg + data->distance) / (distanceAvgCount + 1);
		if (rpmAvgCount > 1 && data->rpm != 0)rpmAvg = (rpmAvgCount * rpmAvg + data->rpm) / (rpmAvgCount + 1);

		if (distanceAvgCount == 0 && data->distance != 0.0)distanceAvg = data->distance;
		if (rpmAvgCount == 0 && data->rpm != 0)rpmAvg = data->rpm;

		if (data->distance != 0.0)distanceAvgCount++;
		if (data->rpm != 0)rpmAvgCount++;

		codeStateCount[0]++;
		PRINTF("code count: %d \r\n", codeStateCount[0]);
		if(codeStateCount[0] % 10 == 0)
			PRINTF("test");
		break;
	case 1:
		codeStateCount[1]++;
		PRINTF("code count: %d \r\n", codeStateCount[1]);
		break;
	case 2:
		codeStateCount[2]++;
		PRINTF("code count: %d \r\n", codeStateCount[2]);
		break;
	case 3:
		codeStateCount[3]++;
		PRINTF("code count: %d \r\n", codeStateCount[3]);
		break;
	default:
		codeStateCount[4]++;
		PRINTF("code count: %d \r\n", codeStateCount[4]);
		break;

	}

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
	//DBG_Init();
	/* Configure the hardware*/
	HW_Init();

	/* USER CODE BEGIN 1 */
	RespirationData data;

	gpioInit();

	huartHandleInit();

	x4m200Init();
	/* USER CODE END 1 */

	/*Disable Stand-by mode*/
	LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

	PRINTF("APP_VERSION= %02X.%02X.%02X.%02X\r\n",
			(uint8_t)(__APP_VERSION >> 24), (uint8_t)(__APP_VERSION >> 16),
			(uint8_t)(__APP_VERSION >> 8), (uint8_t)__APP_VERSION);
	PRINTF("MAC_VERSION= %02X.%02X.%02X.%02X\r\n",
			(uint8_t)(__LORA_MAC_VERSION >> 24),
			(uint8_t)(__LORA_MAC_VERSION >> 16),
			(uint8_t)(__LORA_MAC_VERSION >> 8), (uint8_t)__LORA_MAC_VERSION);

	/* Configure the Lora Stack*/
	LORA_Init(&LoRaMainCallbacks, &LoRaParamInit);

	LORA_Join();

	LoraStartTx(TX_ON_TIMER);

	while (true) {
		data = getRespirationData();
		if (data.valid) {
			PRINTF("Respiration code -> %2x\r\n", data.code);
			analyzeResporationData(&data);

			if (AppProcessRequest == LORA_SET) {
				 /*reset notification flag*/
				 AppProcessRequest = LORA_RESET;

				 /*Send*/
				 sendingPacket = true;
				 Send(NULL);
			}

			if (LoraMacProcessRequest == LORA_SET) {
				/*reset notification flag*/
				LoraMacProcessRequest = LORA_RESET;
				LoRaMacProcess();
			}
			/* USER CODE BEGIN 2 */
			/* USER CODE END 2 	*/
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
	/* USER CODE BEGIN 3 */
	if (LORA_JoinStatus() != LORA_SET) {
		/*Not joined, try again later*/
		LORA_Join();
		return;
	}

#ifdef USE_B_L072Z_LRWAN1
	TimerInit(&TxLedTimer, OnTimerLedEvent);

	TimerSetValue(&TxLedTimer, 200);

	LED_On(LED_RED1);

	TimerStart(&TxLedTimer);
#endif
	AppData.Port = LORAWAN_APP_PORT;

	memcpy(&AppData.Buff, &codeStateCount[0], 4);
	memcpy(&AppData.Buff + 4, &codeStateCount[1], 4);
	memcpy(&AppData.Buff + 8, &codeStateCount[2], 4);
	memcpy(&AppData.Buff + 12, &distanceAvg, 4);
	memcpy(&AppData.Buff + 16, &rpmAvg, 4);

	AppData.BuffSize = 20;

	sendingPacket = false;
	PRINTF("Packet sent!\r\n");

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
