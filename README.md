# 🕒 Mini Project 1 – 디지털 원격 제어 시계

## 📌 프로젝트 개요
STM32 보드와 아두이노 모듈을 활용하여 **원격 제어 및 NTP 시간 동기화 기능**을 갖춘 디지털 알람 시계를 구현한 프로젝트입니다.  
기존 알람 시계에 없는 **WiFi 기반 시간 동기화 및 리모컨 제어 기능**을 추가하여 사용자 편의성을 강화했습니다.

---

## 🔧 개발 기간
- **2025.08.04 ~ 2025.08.08 (5일)**

---

## ⚙️ 주요 기능
- RTC 기반 시계 표시 (STM32 내장 RTC → LCD1602 실시간 출력)  
- IR 리모컨 원격 제어 (시간/알람 설정)  
- ESP-01 WiFi 모듈 + DS1302 RTC → **NTP 서버 동기화 + 전원 차단 시 시간 유지**  
- 알람 시 LED 점멸 + 부저 작동  

---

## 🛠️ 개발 환경 및 도구
- **Hardware** : STM32 Nucleo-F411RE, DS1302 RTC, ESP-01 WiFi, LCD1602 (I2C), IR Receiver, Remote Controller, Buzzer, LED  
- **Software** : STM32CubeIDE, FreeRTOS, C (HAL Driver)  

---

## 💡 사용 기술
C / STM32 / FreeRTOS / RTC / UART / I2C / GPIO / WiFi / IR Remote / NTP

---

## 🔄 펌웨어 동작 원리
1. **시스템 초기화**
   - HAL 및 FreeRTOS 초기화, LCD에 시작 메시지 출력  

2. **시간 관리**
   - STM32 내부 RTC → 주 시계  
   - DS1302 보조 RTC, ESP-01을 통해 NTP 서버와 동기화  

3. **UI 및 리모컨**
   - NEC 프로토콜 기반 IR 신호 디코딩  
   - 리모컨 입력 → 시간/알람 설정, LCD 2행에 상태 표시  

4. **알람 실행**
   - RTC 시간이 알람과 일치 → LED 깜빡임 + 부저 울림  

5. **WiFi 동기화**
   - 주기적으로 NTP 서버와 통신  
   - FreeRTOS Mutex로 통신 충돌 방지  

---

## 📈 개발 성과
- 기존 알람 시계 대비 **WiFi 기반 시간 동기화 기능 구현**
- **리모컨 기반 원격 제어 및 알람 커스터마이징**으로 사용자 편의성 확보
- FreeRTOS 기반 멀티태스킹 구조 설계 → 안정적인 동작 보장
- 확장성 있는 구조로 **스마트홈 시스템 연계 가능**

---

## FreeRTOS기반 Task 구조
<img width="641" height="361" alt="image" src="https://github.com/user-attachments/assets/53322dc1-e79d-4ff4-bcd4-f284debe420e" />


## 📸 시스템 블록 다이어그램
```plaintext
        +-------------------+
        |     NTP Server    |
        +---------+---------+
                  |
              WiFi Sync
                  |
        +---------v---------+
        |   ESP-01 + RTC    |
        +---------+---------+
                  |
        +---------v---------+
        |     STM32F411     |
        |    (FreeRTOS)     |
        +---+-----------+---+
            |           |
        LCD1602     IR Remote
            |           |
          Time      Time+Alarm
            |        Control
       +----v----+
       |  Alarm  |
       | LED+BZR |
       +---------+
