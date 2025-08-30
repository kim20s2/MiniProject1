/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rtc.h"
#include "lcd.h"
#include "ir_decode.h"
#include "ds1302.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "queue.h"
#include "ntptimer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim2;

RTC_TimeTypeDef nowTime;
RTC_DateTypeDef nowDate;
RTC_TimeTypeDef rtc_time;
RTC_DateTypeDef rtc_date;
RTC_TimeTypeDef alarmTime = {0};

RTC_TimeTypeDef setup_time;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_QUEUE_LENGTH 8
#define LCD_QUEUE_ITEM_SIZE 32  // LCD 한 줄은 최대 16글자지만 여유 확보
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
SystemMode current_mode = MODE_IDLE;
TimeSelectMode selected_time_field = TIME_FIELD_NONE;
AlarmSelectMode selected_alarm_field = ALARM_FIELD_NONE;

//QueueHandle_t lcdQueue;  // 전역 Queue 핸들
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile int alarm_triggered_flag = 0;
uint32_t alarm_triggered_tick = 0;

volatile int time_set_done_flag = 0;
uint32_t time_set_done_tick = 0;

volatile int time_restored_RTC_flag = 0;
uint32_t time_restored_RTC_tick = 0;

volatile int time_restored_WIFI_flag = 0;
uint32_t time_restored_WIFI_tick = 0;

volatile int time_sync_fail_flag = 0;
uint32_t time_sync_fail_tick = 0;

uint8_t alarm_triggered = 0;

bool ui_second_line_disabled = false; // EQ 버튼 이후 2행 비활성화 상태

bool alarm_is_set = false;
bool date_display_enabled = false;
extern RTC_TimeTypeDef WiFiTime;
extern RTC_DateTypeDef WiFiDate;

extern uint8_t isWiFiready;

/* USER CODE END Variables */
/* Definitions for TimeTask */
osThreadId_t TimeTaskHandle;
const osThreadAttr_t TimeTask_attributes = {
  .name = "TimeTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AlarmTask */
osThreadId_t AlarmTaskHandle;
const osThreadAttr_t AlarmTask_attributes = {
  .name = "AlarmTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for IRTask */
osThreadId_t IRTaskHandle;
const osThreadAttr_t IRTask_attributes = {
  .name = "IRTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for UIStateTask */
osThreadId_t UIStateTaskHandle;
const osThreadAttr_t UIStateTask_attributes = {
  .name = "UIStateTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LCDTask */
osThreadId_t LCDTaskHandle;
const osThreadAttr_t LCDTask_attributes = {
  .name = "LCDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for NTPTask */
osThreadId_t NTPTaskHandle;
const osThreadAttr_t NTPTask_attributes = {
  .name = "NTPTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for LCD_Queue */
osMessageQueueId_t LCD_QueueHandle;
const osMessageQueueAttr_t LCD_Queue_attributes = {
  .name = "LCD_Queue"
};
/* Definitions for LCDMutex */
osMutexId_t LCDMutexHandle;
const osMutexAttr_t LCDMutex_attributes = {
  .name = "LCDMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTimeTask(void *argument);
void StartAlarmTask(void *argument);
void StartIRTask(void *argument);
void StartUIStateTask(void *argument);
void StartLCDTask(void *argument);
void StartNTPTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of LCDMutex */
  LCDMutexHandle = osMutexNew(&LCDMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of LCD_Queue */
  LCD_QueueHandle = osMessageQueueNew (8, sizeof(uint32_t), &LCD_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TimeTask */
  TimeTaskHandle = osThreadNew(StartTimeTask, NULL, &TimeTask_attributes);

  /* creation of AlarmTask */
  AlarmTaskHandle = osThreadNew(StartAlarmTask, NULL, &AlarmTask_attributes);

  /* creation of IRTask */
  IRTaskHandle = osThreadNew(StartIRTask, NULL, &IRTask_attributes);

  /* creation of UIStateTask */
  UIStateTaskHandle = osThreadNew(StartUIStateTask, NULL, &UIStateTask_attributes);

  /* creation of LCDTask */
  LCDTaskHandle = osThreadNew(StartLCDTask, NULL, &LCDTask_attributes);

  /* creation of NTPTask */
  NTPTaskHandle = osThreadNew(StartNTPTask, NULL, &NTPTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartTimeTask */
/**
  * @brief  Function implementing the TimeTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void *argument)
{
  /* USER CODE BEGIN StartTimeTask */
	//LCD_Init(&hi2c1);
	RTC_TimeTypeDef prev_time = {0}, curr_time;
	RTC_DateTypeDef curr_date;
	char buf[32];

	uint32_t last_backup_tick = 0;  // 🔹 백업 간격 측정용

	for (;;) {
	    HAL_RTC_GetTime(&hrtc, &curr_time, RTC_FORMAT_BIN);
	    HAL_RTC_GetDate(&hrtc, &curr_date, RTC_FORMAT_BIN); // 중요!

	    if (curr_time.Seconds != prev_time.Seconds) {
	        prev_time = curr_time;

	        snprintf(buf, sizeof(buf), "Time: %02d:%02d:%02d  ",
	                 curr_time.Hours, curr_time.Minutes, curr_time.Seconds);
	        osMutexAcquire(LCDMutexHandle, osWaitForever);
	        LCD_SetCursor(0, 0);
	        LCD_Print(buf);
	        osMutexRelease(LCDMutexHandle);

	        // 3초 마다 DS1302에 시간 백업, 1초마다로 하면 너무 자주 ds1302를 호출해서 시간 저장/읽기 기능오작동 할수있음
			if (((osKernelGetTickCount() - last_backup_tick) >= 3000) && (!isWiFiready)) {
				HAL_RTC_GetTime(&hrtc, &curr_time, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &curr_date, RTC_FORMAT_BIN);
				DS1302_SetTime(&curr_time, &curr_date);
				last_backup_tick = osKernelGetTickCount();
			}
	}
	    osDelay(10);  // 부하 적고, 밀림도 없음
	}
  /* USER CODE END StartTimeTask */
}

/* USER CODE BEGIN Header_StartAlarmTask */
/**
* @brief Function implementing the AlarmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAlarmTask */
void StartAlarmTask(void *argument)
{
  /* USER CODE BEGIN StartAlarmTask */
	RTC_TimeTypeDef sTime;

	for (;;) {
		if (alarm_is_set && !alarm_triggered) { // 이전에 !alarm_setting_mode였음
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			if (sTime.Hours == alarm_hour &&
				sTime.Minutes == alarm_min &&
		        sTime.Seconds >= alarm_sec &&
		        sTime.Seconds < alarm_sec + 2) {

				alarm_triggered = 1;
				alarm_is_set = false;  // 알람 울렸으니 설정 해제
				alarm_hour = alarm_min = alarm_sec = 255;  // 🔹 추가로 추천

				for (int i = 0; i < 1; ++i) {
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);   // 부저 ON
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   // LED ON
					osDelay(250);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET); // 부저 OFF
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // LED OFF
					osDelay(250);
				}

			    // 🟢 LCD에 알람 메시지 띄우기 요청
			    alarm_triggered_flag = 1;
			    alarm_triggered_tick = osKernelGetTickCount();
			}
		}
		osDelay(500);
	}
  /* USER CODE END StartAlarmTask */
}

/* USER CODE BEGIN Header_StartIRTask */
/**
* @brief Function implementing the IRTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartIRTask */
void StartIRTask(void *argument)
{
  /* USER CODE BEGIN StartIRTask */
	for (;;) {
		if (ir_key_ready) {
		  uint8_t cmd = (ir_data >> 16) & 0xFF;
		  ir_key_ready = 0;

		  if (cmd == 0x44) { // << 버튼: 시간 설정 진입 또는 완료
			if (current_mode == MODE_IDLE) {
			  current_mode = MODE_SET_TIME;
			  selected_time_field = TIME_FIELD_HOUR;

			  setup_hour_digits[0] = setup_hour_digits[1] = 0;
			  setup_min_digits[0] = setup_min_digits[1] = 0;
			  setup_sec_digits[0] = setup_sec_digits[1] = 0;
			  setup_hour_input_idx = setup_min_input_idx = setup_sec_input_idx = 0;

			  osMutexAcquire(LCDMutexHandle, osWaitForever);
			  LCD_SetCursor(1, 0);
			  LCD_Print("Set Time Mode     ");
			  osMutexRelease(LCDMutexHandle);
			  osDelay(1000);
			}
			else if (current_mode == MODE_SET_TIME) {
			  RTC_DateTypeDef date;
			  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

			  setup_time.Hours = setup_hour_digits[0] * 10 + setup_hour_digits[1];
			  setup_time.Minutes = setup_min_digits[0] * 10 + setup_min_digits[1];
			  setup_time.Seconds = setup_sec_digits[0] * 10 + setup_sec_digits[1];

			  HAL_RTC_SetTime(&hrtc, &setup_time, RTC_FORMAT_BIN);
			  DS1302_SetTime(&setup_time, &date);

			  current_mode = MODE_IDLE;
			  selected_time_field = TIME_FIELD_NONE;

			  osMutexAcquire(LCDMutexHandle, osWaitForever);
			  LCD_SetCursor(1, 0);
			  LCD_Print("Time Set Done     ");
			  osMutexRelease(LCDMutexHandle);

			  time_set_done_flag = 1;
			  time_set_done_tick = osKernelGetTickCount();
			  osDelay(1000);
			}
		  }

		  else if (cmd == 0x43) { // Play 버튼: 알람 설정 진입 또는 완료
			if (current_mode == MODE_IDLE) {
			  current_mode = MODE_SET_ALARM;
			  selected_alarm_field = ALARM_FIELD_HOUR;

			  alarm_hour_input_idx = alarm_min_input_idx = alarm_sec_input_idx = 0;
			  alarm_hour_digits[0] = alarm_hour_digits[1] = 0;
			  alarm_min_digits[0] = alarm_min_digits[1] = 0;
			  alarm_sec_digits[0] = alarm_sec_digits[1] = 0;

			  osMutexAcquire(LCDMutexHandle, osWaitForever);
			  LCD_SetCursor(1, 0);
			  LCD_Print("Setup Alarm...");
			  osMutexRelease(LCDMutexHandle);
			  osDelay(1000);
			}
			else if (current_mode == MODE_SET_ALARM) {
			  alarm_hour = alarm_hour_digits[0] * 10 + alarm_hour_digits[1];
			  alarm_min = alarm_min_digits[0] * 10 + alarm_min_digits[1];
			  alarm_sec = alarm_sec_digits[0] * 10 + alarm_sec_digits[1];
			  current_mode = MODE_IDLE;
			  alarm_is_set = true;

			  char msg[25];
			  sprintf(msg, "Alarm: %02d:%02d:%02d  ", alarm_hour, alarm_min, alarm_sec);
			  osMutexAcquire(LCDMutexHandle, osWaitForever);
			  LCD_SetCursor(1, 0);
			  LCD_Print(msg);
			  osMutexRelease(LCDMutexHandle);
			}
		  }

		  // 시/분/초 필드 선택
		  else if (cmd == 0x45) { // CH-
			if (current_mode == MODE_SET_TIME) selected_time_field = TIME_FIELD_HOUR;
			else if (current_mode == MODE_SET_ALARM) selected_alarm_field = ALARM_FIELD_HOUR;
		  }
		  else if (cmd == 0x46) { // CH
			if (current_mode == MODE_SET_TIME) selected_time_field = TIME_FIELD_MINUTE;
			else if (current_mode == MODE_SET_ALARM) selected_alarm_field = ALARM_FIELD_MINUTE;
		  }
		  else if (cmd == 0x47) { // CH+
			if (current_mode == MODE_SET_TIME) selected_time_field = TIME_FIELD_SECOND;
			else if (current_mode == MODE_SET_ALARM) selected_alarm_field = ALARM_FIELD_SECOND;
		  }

		  // 숫자 입력 (공통)
		  else if (cmd == 0x16 || cmd == 0x0C || cmd == 0x18 || cmd == 0x5E ||
				   cmd == 0x08 || cmd == 0x1C || cmd == 0x5A || cmd == 0x42 ||
				   cmd == 0x52 || cmd == 0x4A) {
			uint8_t num = 0xFF;
			switch (cmd) {
			  case 0x16: num = 0; break;
			  case 0x0C: num = 1; break;
			  case 0x18: num = 2; break;
			  case 0x5E: num = 3; break;
			  case 0x08: num = 4; break;
			  case 0x1C: num = 5; break;
			  case 0x5A: num = 6; break;
			  case 0x42: num = 7; break;
			  case 0x52: num = 8; break;
			  case 0x4A: num = 9; break;
			}

			if (current_mode == MODE_SET_TIME && selected_time_field != TIME_FIELD_NONE) {
			  if (selected_time_field == TIME_FIELD_HOUR) {
				setup_hour_digits[setup_hour_input_idx] = num;
				setup_hour_input_idx = (setup_hour_input_idx + 1) % 2;
			  }
			  else if (selected_time_field == TIME_FIELD_MINUTE) {
				setup_min_digits[setup_min_input_idx] = num;
				setup_min_input_idx = (setup_min_input_idx + 1) % 2;
			  }
			  else if (selected_time_field == TIME_FIELD_SECOND) {
				setup_sec_digits[setup_sec_input_idx] = num;
				setup_sec_input_idx = (setup_sec_input_idx + 1) % 2;
			  }
			}
			else if (current_mode == MODE_SET_ALARM && selected_alarm_field != ALARM_FIELD_NONE) {
			  if (selected_alarm_field == ALARM_FIELD_HOUR) {
				alarm_hour_digits[alarm_hour_input_idx] = num;
				alarm_hour_input_idx = (alarm_hour_input_idx + 1) % 2;
			  }
			  else if (selected_alarm_field == ALARM_FIELD_MINUTE) {
				alarm_min_digits[alarm_min_input_idx] = num;
				alarm_min_input_idx = (alarm_min_input_idx + 1) % 2;
			  }
			  else if (selected_alarm_field == ALARM_FIELD_SECOND) {
				alarm_sec_digits[alarm_sec_input_idx] = num;
				alarm_sec_input_idx = (alarm_sec_input_idx + 1) % 2;
			  }
			}
		  }

		  else if (cmd == 0x07) {  // - 버튼: 날짜 출력
			  date_display_enabled = true;
		      RTC_DateTypeDef date;
		      HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

		      char dateStr[32];
		      snprintf(dateStr, sizeof(dateStr), "Date: %04d-%02d-%02d  ",
		               2000 + date.Year, date.Month, date.Date);

		      osMutexAcquire(LCDMutexHandle, osWaitForever);
		      LCD_SetCursor(1, 0);  // 2행 0열
		      LCD_Print(dateStr);
		      osMutexRelease(LCDMutexHandle);
		  }


		  else if (cmd == 0x19) {  // 100+ 버튼: WIFI에서 시간 복원 후 LCD 0행에 표시
		      RTC_TimeTypeDef dsTime;
		      RTC_DateTypeDef dsDate;
		      char ntp_msg[32];

		      if (isWiFiready) {
		    	  DS1302_GetTime(&dsTime, &dsDate);  // ds1302에서 백업된 시간 불러오기

		          HAL_RTC_SetTime(&hrtc, &dsTime, RTC_FORMAT_BIN);
		          HAL_RTC_SetDate(&hrtc, &dsDate, RTC_FORMAT_BIN);

		          // LCD 0행에 NTP 시간 한 번 출력
		          snprintf(ntp_msg, sizeof(ntp_msg), "NTP: %02d:%02d:%02d  ",
		                   dsTime.Hours, dsTime.Minutes, dsTime.Seconds);

		          osMutexAcquire(LCDMutexHandle, osWaitForever);
		          LCD_SetCursor(0, 0);
		          LCD_Print(ntp_msg);
		          osMutexRelease(LCDMutexHandle);

		          time_restored_WIFI_flag = 1;
		          time_restored_WIFI_tick = osKernelGetTickCount();

		          osDelay(1000);
		      }
		      else {
		          // LCD 1행에 실패 메시지 → 플래그 사용
		          time_sync_fail_flag = 1;
		          time_sync_fail_tick = osKernelGetTickCount();
		      }
		  }

		  else if (cmd == 0x0D) {  // 200+ 버튼: DS1302에서 시간 복원
		      RTC_TimeTypeDef dsTime;
		      RTC_DateTypeDef dsDate;
		      //char log[64];

		      DS1302_GetTime(&dsTime, &dsDate);  // ds1302에서 백업된 시간 불러오기

		      HAL_RTC_SetTime(&hrtc, &dsTime, RTC_FORMAT_BIN); // 불러온 시간 RTC에 저장
		      HAL_RTC_SetDate(&hrtc, &dsDate, RTC_FORMAT_BIN); // 불러온 날짜 RTC에 저장

		      //HAL_UART_Transmit(&huart2, (uint8_t*)log, strlen(log), 100);
		      time_restored_RTC_flag = 1;
		      time_restored_RTC_tick = osKernelGetTickCount();
		      osDelay(1000);
		  }

		  // EQ 버튼: 시간/알람 설정 취소
		  else if (cmd == 0x09) {
			if (current_mode == MODE_SET_TIME || current_mode == MODE_SET_ALARM) {
			  current_mode = MODE_IDLE;
			  selected_time_field = TIME_FIELD_NONE;
			  selected_alarm_field = ALARM_FIELD_NONE;
			  alarm_is_set = false;

			  alarm_hour = alarm_min = alarm_sec = 255;
			  date_display_enabled = false;
			  alarm_triggered = 0;

			  // 2행을 표시하지 않도록 관련 플래그 초기화

			  osMutexAcquire(LCDMutexHandle, osWaitForever);
			  LCD_SetCursor(1, 0);
			  LCD_Print("                ");
			  osMutexRelease(LCDMutexHandle);
			}
		  }
		  reset_ir_state();  // 수신 완료 후 초기화
		}
		osDelay(30);
	}
  /* USER CODE END StartIRTask */
}

/* USER CODE BEGIN Header_StartUIStateTask */
/**
* @brief Function implementing the UIStateTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUIStateTask */
void StartUIStateTask(void *argument)
{
  /* USER CODE BEGIN StartUIStateTask */
	char buf[32];
	for (;;) {
	osMutexAcquire(LCDMutexHandle, osWaitForever);

	// 1️⃣ Alarm Triggered
	if (alarm_triggered_flag) {
	  if ((osKernelGetTickCount() - alarm_triggered_tick) <= 1000) {
		LCD_SetCursor(1, 0);
		LCD_Print("Alarm Triggered   ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  } else {
		alarm_triggered_flag = 0;
		LCD_SetCursor(1, 0);
		LCD_Print("                ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  }
	}

	// 2️⃣ Time Set Done
	if (time_set_done_flag) {
	  if ((osKernelGetTickCount() - time_set_done_tick) <= 1000) {
		LCD_SetCursor(1, 0);
		LCD_Print("Time Set Done     ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  } else {
		time_set_done_flag = 0;
		LCD_SetCursor(1, 0);
		LCD_Print("                ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  }
	}

	// 3️⃣ Time Restored (DS1302)
	if (time_restored_RTC_flag || time_restored_WIFI_flag) {
	  if ((osKernelGetTickCount() - time_restored_RTC_tick) <= 1000) {
		LCD_SetCursor(1, 0);
		LCD_Print("Refresh(RTC)     ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  }
	  else if ((osKernelGetTickCount() - time_restored_WIFI_tick) <= 1000) {
		LCD_SetCursor(1, 0);
		LCD_Print("Refresh(WIFI)     ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  }
	  else {
		if(time_restored_RTC_flag == 1) time_restored_RTC_flag = 0;
		if(time_restored_WIFI_flag == 1) time_restored_WIFI_flag = 0;
		LCD_SetCursor(1, 0);
		LCD_Print("                ");
		osMutexRelease(LCDMutexHandle);
		osDelay(200);
		continue;
	  }
	}

	// 0️⃣ NTP Sync Fail
	if (time_sync_fail_flag) {
	    if ((osKernelGetTickCount() - time_sync_fail_tick) <= 1000) {
	        LCD_SetCursor(1, 0);
	        LCD_Print("NTP Sync Fail    ");
	        osMutexRelease(LCDMutexHandle);
	        osDelay(200);
	        continue;
	    } else {
	        time_sync_fail_flag = 0;
	        LCD_SetCursor(1, 0);
	        LCD_Print("                ");
	        osMutexRelease(LCDMutexHandle);
	        osDelay(200);
	        continue;
	    }
	}

	// 🌐 기본 출력 (날짜, 설정 중, 알람 설정 등)
	else if (date_display_enabled) {
	  RTC_DateTypeDef sDate;
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	  snprintf(buf, sizeof(buf), "Date: %04d/%02d/%02d",
			   2000 + sDate.Year, sDate.Month, sDate.Date);
	  LCD_SetCursor(1, 0);
	  LCD_Print(buf);
	}
	else if (current_mode == MODE_SET_ALARM) {
	  snprintf(buf, sizeof(buf), "SetA: %d%d:%d%d:%d%d  ",
			   alarm_hour_digits[0], alarm_hour_digits[1],
			   alarm_min_digits[0], alarm_min_digits[1],
			   alarm_sec_digits[0], alarm_sec_digits[1]);
	  LCD_SetCursor(1, 0);
	  LCD_Print(buf);
	}
	else if (current_mode == MODE_SET_TIME) {
	  snprintf(buf, sizeof(buf), "SetT: %d%d:%d%d:%d%d  ",
			   setup_hour_digits[0], setup_hour_digits[1],
			   setup_min_digits[0], setup_min_digits[1],
			   setup_sec_digits[0], setup_sec_digits[1]);
	  LCD_SetCursor(1, 0);
	  LCD_Print(buf);
	}
	else if (current_mode == MODE_IDLE) {
	  if (alarm_is_set && !(alarm_hour == 255 || alarm_min == 255 || alarm_sec == 255)) {
		snprintf(buf, sizeof(buf), "Alarm: %02d:%02d:%02d  ",
				 alarm_hour, alarm_min, alarm_sec);
		LCD_SetCursor(1, 0);
		LCD_Print(buf);
	  } else {
		LCD_SetCursor(1, 0);
		LCD_Print("                ");
	  }
	}

	osMutexRelease(LCDMutexHandle);
	osDelay(200);
	}
  /* USER CODE END StartUIStateTask */
}

/* USER CODE BEGIN Header_StartLCDTask */
/**
* @brief Function implementing the LCDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLCDTask */
void StartLCDTask(void *argument)
{
  /* USER CODE BEGIN StartLCDTask */
  osDelay(500);   // 전원 안정화 대기
  LCD_Init(&hi2c1);

  char msg[LCD_QUEUE_ITEM_SIZE];
  /* Infinite loop */
  for(;;)
  {
    // CMSIS RTOS2 방식으로 메시지 수신
	if (osMessageQueueGet(LCD_QueueHandle, msg, NULL, osWaitForever) == osOK) {
		osMutexAcquire(LCDMutexHandle, osWaitForever);
		LCD_SetCursor(0, 0);
		LCD_Print(msg);
		osMutexRelease(LCDMutexHandle);
	}
  }
  /* USER CODE END StartLCDTask */
}

/* USER CODE BEGIN Header_StartNTPTask */
/**
* @brief Function implementing the NTPTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartNTPTask */
void StartNTPTask(void *argument)
{
  /* USER CODE BEGIN StartNTPTask */
  // 태스크 시작 후 즉시 NTP 동기화 요청 (테스트용)
  osDelay(3000);  // 시스템이 안정화될 때까지 3초 대기
  //NTP_RequestTimeSync();  // 즉시 NTP 동기화 요청
  //Debug_Print("Start NTP Task\r\n");
  NTP_Timer_Init(&huart1, &huart2);
  /* Infinite loop */
  for(;;)
  {
    // NTP 타이머 태스크 실행
    NTP_Timer_Task(argument);
    osDelay(100);
  }
  /* USER CODE END StartNTPTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

