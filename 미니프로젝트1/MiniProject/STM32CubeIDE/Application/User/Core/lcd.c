// =============================
// LCD_Task.c  (uses lcd.h)
// =============================
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "i2c.h"     // hi2c1
#include "rtc.h"     // hrtc
#include "lcd.h"     // LCD_Init/LCD_SetCursor/LCD_Print/LCD_Clear

// Weather_Task.c에서 최신 값 캐시 (없으면 Weather_Task.c에 정의하세요)
extern volatile float g_t1h; // 온도(예: 27.3)  없으면 타입/이름 맞춰 변경
extern volatile int   g_reh; // 습도(정수, -1이면 미수신)
extern volatile int   g_pty; // 강수형태 코드(0,1,2,3,4; -1이면 미수신)

static void lcd_line16(uint8_t row, const char *s)
{
    char buf[17]; size_t n=0;
    while (n<16 && s[n]) buf[n++] = s[n];
    while (n<16)         buf[n++] = ' ';
    buf[16] = '\0';
    LCD_SetCursor(row, 0);  // (row, col)
    LCD_Print(buf);
}

static const char* pty_short(int code)
{
    switch (code) {
        case 0: return "맑";
        case 1: return "비";
        case 2: return "비/눈";
        case 3: return "눈";
        case 4: return "소나";
        default:return "--";
    }
}

void LCD_Task(void *argument)
{
    // 초기화 (Task 안에서 수행: 다른 Task와 경합 최소화)
    LCD_Init(&hi2c1);
    LCD_Clear();
    lcd_line16(0, "NTP/LCD READY");
    lcd_line16(1, "WAIT WEATHER...");

    TickType_t last = xTaskGetTickCount();
    char line1[17], prev1[17] = {0};
    char line2[17], prev2[17] = {0};
    int sec30 = 0;

    for(;;) {
        // (1) 1행: HH:MM:SS (RTC는 NTP Task에서 동기된다고 가정)
        {
            RTC_TimeTypeDef t; RTC_DateTypeDef d;
            HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN); // 반드시 Time 다음에 Date

            snprintf(line1, sizeof(line1), "%02u:%02u:%02u",
                     t.Hours, t.Minutes, t.Seconds);
            if (strncmp(prev1, line1, 16) != 0) {
                lcd_line16(0, line1);
                strncpy(prev1, line1, 16);
            }
        }

        // (2) 30초마다 2행: 날씨 캐시 표시 (뮤텍스 없이 읽기만)
        if (++sec30 >= 30) {
            sec30 = 0;

            float t1h = g_t1h; // 32-bit 읽기는 M4에서 원자적(정렬 가정)
            int   reh = g_reh;
            int   pty = g_pty;

            if (reh >= 0 && pty >= 0) {
                // "27C RH68 맑"  <= 16칸 맞춤
                snprintf(line2, sizeof(line2), "%2dC RH%2d %-3s",
                         (int)t1h, reh, pty_short(pty));
            } else {
                snprintf(line2, sizeof(line2), "--C RH-- --");
            }

            if (strncmp(prev2, line2, 16) != 0) {
                lcd_line16(1, line2);
                strncpy(prev2, line2, 16);
            }
        }

        // ★ 통신과의 경합 방지: 정확히 1초 간격 유지
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
    }
}
