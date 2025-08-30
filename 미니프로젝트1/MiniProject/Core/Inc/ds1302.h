#ifndef __DS1302_H__
#define __DS1302_H__

#include "stm32f4xx_hal.h"
#include "rtc.h"

// DS1302 핀 제어 매크로 (필요시 포트/핀 변경 가능)
#define DS1302_CE_HIGH()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET)
#define DS1302_CE_LOW()      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET)

#define DS1302_CLK_HIGH()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET)
#define DS1302_CLK_LOW()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET)

#define DS1302_IO_HIGH()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
#define DS1302_IO_LOW()      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)
#define DS1302_IO_READ()     HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)

#define DS1302_IO_INPUT_MODE()   ds1302_set_pin_input()
#define DS1302_IO_OUTPUT_MODE()  ds1302_set_pin_output()

#ifdef __cplusplus
extern "C" {
#endif

// IO 핀 방향 설정 함수
void ds1302_set_pin_input(void);
void ds1302_set_pin_output(void);

// DS1302 시간 쓰기/읽기 함수
void DS1302_SetTime(RTC_TimeTypeDef* time, RTC_DateTypeDef* date);
void DS1302_GetTime(RTC_TimeTypeDef* time, RTC_DateTypeDef* date);
void DS1302_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DS1302_H__ */
