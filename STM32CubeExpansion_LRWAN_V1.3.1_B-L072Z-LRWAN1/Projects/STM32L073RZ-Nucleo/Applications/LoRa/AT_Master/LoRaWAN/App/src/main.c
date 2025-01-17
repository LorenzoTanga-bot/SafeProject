/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main program body
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
#include "bsp.h"
#include "timeServer.h"
#include <string.h>
#include <stdlib.h>

#include "stm32l0xx_hal.h"
#include "lora_driver.h"
#include <stdarg.h>

#include ATCMD_MODEM        /* preprocessing definition in hw_conf.h*/



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* CAYENNE_LLP is myDevices Application server*/
#define CAYENNE_LPP
#define LPP_DATATYPE_DIGITAL_INPUT 0x0
#define LPP_DATATYPE_DIGITAL_OUTPUT 0x1
#define LPP_DATATYPE_HUMIDITY 0x68
#define LPP_DATATYPE_TEMPERATURE 0x67
#define LPP_DATATYPE_BAROMETER 0x73

/* Private variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static sensor_t Sensor;                            /* struct for data sensor*/

#if USE_LRWAN_NS1
static bool user_button_flag = true;
#endif


/* Private function prototypes -----------------------------------------------*/

static void SensorMeasureData(sSendDataBinary_t *SendDataBinary);

/* load call backs*/
static LoRaDriverCallback_t LoRaDriverCallbacks = { SensorMeasureData,
                                                    NULL
                                                  };

#if USE_LRWAN_NS1
/**
 * @brief  EXTI line detection callbacks.
 * @param  GPIO_Pin: Specifies the pins connected to the EXTI line.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  HW_RTC_DelayMs(10);
  if (!HW_GPIO_Read(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN))
  {
    if (user_button_flag)
    {
      Modem_IO_DeInit();
      BSP_LED_DeInit(LED2);
      user_button_flag = false;
      while (1);
    }
    else
    {
      Modem_IO_Init();
      NVIC_SystemReset();
    }
  }
}
#endif

/* Private macro ------------- -----------------------------------------------*/

#ifdef CAYENNE_LPP
#define LORAWAN_APP_PORT           99;            /*LoRaWAN application port*/
#else
#define LORAWAN_APP_PORT           2;            /*LoRaWAN application port*/
#endif

#define LORAWAN_CONFIRMED_MSG      ENABLE         /*LoRaWAN confirmed messages*/

#define SENSORS_MEASURE_CYCLE      15000          /*Periode to do sensors measurement*/

#define JOIN_MODE                  OTAA_JOIN_MODE   /*ABP_JOIN_MODE */ /*LoRaWan join methode*/

#ifdef USE_LRWAN_NS1
#define FREQ_BAND                  /*EU868*/ CN470PREQUEL
#endif


/* Init LoRa Driver modem parameters*/
#ifdef USE_LRWAN_NS1
static LoRaDriverParam_t LoRaDriverParam = {  SENSORS_MEASURE_CYCLE,  JOIN_MODE, FREQ_BAND};
#else
static LoRaDriverParam_t LoRaDriverParam = {  SENSORS_MEASURE_CYCLE,  JOIN_MODE};
#endif

/**
 * @brief  Entry point program
 * @param  None
 * @retval int
 */
int main(void)
{


  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  HW_GpioInit();

  /* Configure the debug mode*/
#if 0
  DBG_Init();
#endif

  /* Configure the hardware*/
  HW_Init();

  /*Disable standby mode*/
  LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

  /* Context Initialization following the LoRa device modem Used*/
  Lora_Ctx_Init(&LoRaDriverCallbacks, &LoRaDriverParam);

  DBG_PRINTF("before idle \n");

  /* Infinite loop */
  while (1)
  {

    /* run the LoRa Modem state machine*/
    Lora_fsm();
    DISABLE_IRQ();
    /* if an interrupt has occurred after DISABLE_IRQ, it is kept pending
     * and cortex will not enter low power anyway  */
    if ((lora_getDeviceState() == DEVICE_SLEEP) && (HW_UART_Modem_IsNewCharReceived() == RESET))
    {
#ifndef LOW_POWER_DISABLE
      LPM_EnterLowPower();
#endif
    }
    ENABLE_IRQ();

  }
}



/********************* LoRa Part Apllication **********************************/

/******************************************************************************
 * @brief SensorMeasureData
 * @param none
 * @return none
******************************************************************************/
static void SensorMeasureData(sSendDataBinary_t *SendDataBinary)
{
  uint8_t LedState = 0;                /*just for padding*/
  uint16_t pressure = 0;
  int16_t temperature = 0;
  uint16_t humidity = 0;
  uint32_t BatLevel = 0;                /*end device connected to external power source*/
  uint8_t index = 0;
  ATEerror_t LoraCmdRetCode;

  /*read pressure, Humidity and Temperature in order to be send on LoRaWAN*/
  BSP_sensor_Read(&Sensor);

#ifdef CAYENNE_LPP
  uint8_t cchannel = 0;

  temperature = (int16_t)(Sensor.temperature * 10);         /* in �C * 10 */
  pressure    = (uint16_t)(Sensor.pressure * 100 / 10);      /* in hPa / 10 */
  humidity    = (uint16_t)(Sensor.humidity * 2);            /* in %*2     */

#ifdef USE_LRWAN_NS1
  uint8_t str_buff[100];
  memset(str_buff, 0, sizeof(str_buff));
  sprintf((char *)str_buff, "Pressure: %.1f hPa   Temperature: %.1f degreeC   Humidity: %.1f\r\n", Sensor.pressure, \
          Sensor.temperature, \
          Sensor.humidity);
#if defined CMD_DEBUG
  at_printf_send(str_buff, strlen((const char *)str_buff));
#endif

#endif

  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_BAROMETER;
  SendDataBinary->Buffer[index++] = (pressure >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = pressure & 0xFF;
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_TEMPERATURE;
  SendDataBinary->Buffer[index++] = (temperature >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = temperature & 0xFF;
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_HUMIDITY;
  SendDataBinary->Buffer[index++] = humidity & 0xFF;
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_DIGITAL_INPUT;

  /*get battery level of the modem (slave)*/
  LoraCmdRetCode = Lora_GetBatLevel(&BatLevel);
#ifdef USE_LRWAN_NS1
  if (LoraCmdRetCode == ATCTL_RET_CMD_VDD)
#else
  if (LoraCmdRetCode == AT_OK)
#endif
  {
    DBG_PRINTF("Msg status = %d --> OK\n", (uint16_t)BatLevel);
  }
  else
  {
    DBG_PRINTF("Msg status = %d --> KO\n", (uint16_t)BatLevel);
  }

  SendDataBinary->Buffer[index++] = BatLevel * 100 / 254;
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_DIGITAL_OUTPUT;
  SendDataBinary->Buffer[index++] = LedState;

#else

  int32_t latitude, longitude = 0;     /*just for padding*/
  uint16_t altitudeGps = 0;            /*just for padding*/

  temperature = (int16_t)(Sensor.temperature * 100);         /* in �C * 100 */
  pressure    = (uint16_t)(Sensor.pressure * 100 / 10);      /* in hPa / 10 */
  humidity    = (uint16_t)(Sensor.humidity * 10);            /* in %*10     */

  latitude = Sensor.latitude;    /* not relevant*/
  longitude = Sensor.longitude;  /* not relevant*/

  /*fill up the send buffer*/
  SendDataBinary->Buffer[index++] = LedState;
  SendDataBinary->Buffer[index++] = (pressure >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  pressure & 0xFF;
  SendDataBinary->Buffer[index++] = (temperature >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  temperature & 0xFF;
  SendDataBinary->Buffer[index++] = (humidity >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  humidity & 0xFF;

  /*get battery level of the modem (slave)*/
  LoraCmdRetCode = Lora_GetBatLevel(&BatLevel);
  if (LoraCmdRetCode == AT_OK)
  {
    DBG_PRINTF("Msg status = %d --> OK\n", BatLevel);
  }
  else
  {
    DBG_PRINTF("Msg status = %d --> KO\n", BatLevel);
  }
  SendDataBinary->Buffer[index++] = (uint8_t)BatLevel;

  /*remaining data just for padding*/
  SendDataBinary->Buffer[index++] = (latitude >> 16) & 0xFF;
  SendDataBinary->Buffer[index++] = (latitude >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = latitude & 0xFF;
  SendDataBinary->Buffer[index++] = (longitude >> 16) & 0xFF;
  SendDataBinary->Buffer[index++] = (longitude >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = longitude & 0xFF;
  SendDataBinary->Buffer[index++] = (altitudeGps >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = altitudeGps & 0xFF;

#endif  /*CAYENNE_LPP*/

  SendDataBinary->DataSize = index;
#ifndef USE_LRWAN_NS1
  SendDataBinary->Port = LORAWAN_APP_PORT;
#endif

#ifdef USE_I_NUCLEO_LRWAN1
  SendDataBinary->Ack = !LORAWAN_CONFIRMED_MSG;
#endif
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t *file, uint32_t line)
{

  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

}

#endif

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
