#include "ds1302.h"
#include "main.h"


static void DS1302_IO_SetMode_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DS1302_IO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS1302_IO_GPIO_Port, &GPIO_InitStruct);
}

static void DS1302_IO_SetMode_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DS1302_IO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_IO_GPIO_Port, &GPIO_InitStruct);
}


static void DS1302_WriteByte(uint8_t data) {
    DS1302_IO_SetMode_Output();

    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(DS1302_IO_GPIO_Port, DS1302_IO_Pin, (data >> i) & 1);
        HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    }
}

static uint8_t DS1302_ReadByte(void) {
    uint8_t value = 0;
    DS1302_IO_SetMode_Input();

    for (int i = 0; i < 8; i++) {
        if (HAL_GPIO_ReadPin(DS1302_IO_GPIO_Port, DS1302_IO_Pin)) {
            value |= (1 << i);
        }
        HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    }
    return value;
}

static void DS1302_WriteRegister(uint8_t addr, uint8_t data) {
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET);
    DS1302_WriteByte(addr & 0xFE); // Write command
    DS1302_WriteByte(data);
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
}

static uint8_t DS1302_ReadRegister(uint8_t addr) {
    uint8_t data;
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET);
    DS1302_WriteByte(addr | 0x01); // Read command
    data = DS1302_ReadByte();
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
    return data;
}

static uint8_t bcd2dec(uint8_t val) {
    return ((val >> 4) * 10 + (val & 0x0F)); //
}

static uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

void DS1302_Init(void) {
		DS1302_WriteRegister(0x8E, 0x00); // Disable write protect
		DS1302_WriteRegister(0x8C, 0x00); // 연도 초기화 (예: 2000년)
}

void DS1302_SetTime(RTC_TimeTypeDef* time, RTC_DateTypeDef* date) {
    DS1302_WriteRegister(0x80, dec2bcd(time->Seconds & 0x7F));
    DS1302_WriteRegister(0x82, dec2bcd(time->Minutes));
    DS1302_WriteRegister(0x84, dec2bcd(time->Hours));
    DS1302_WriteRegister(0x86, dec2bcd(date->Date));
    DS1302_WriteRegister(0x88, dec2bcd(date->Month));
    DS1302_WriteRegister(0x8C, dec2bcd(date->Year % 100));  // 반드시 0~99 제한
}


void DS1302_GetTime(RTC_TimeTypeDef* time, RTC_DateTypeDef* date) {
    time->Seconds = bcd2dec(DS1302_ReadRegister(0x81));
    time->Minutes = bcd2dec(DS1302_ReadRegister(0x83));
    time->Hours   = bcd2dec(DS1302_ReadRegister(0x85));
    date->Date    = bcd2dec(DS1302_ReadRegister(0x87));
    date->Month   = bcd2dec(DS1302_ReadRegister(0x89));
    uint8_t raw_year = bcd2dec(DS1302_ReadRegister(0x8D));
    date->Year = raw_year % 100;  // 0~99 보장
}

