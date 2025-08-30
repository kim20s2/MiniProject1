#include "usart.h"  // huart2가 정의된 헤더
#include <stdio.h>

// printf 리디렉션
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
