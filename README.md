# 🕒 Mini Project 1 – Remote Controlled Digital Alarm Clock

## 📌 프로젝트 개요
STM32 보드, DS1302 RTC, ESP-01 WiFi 모듈, LCD1602, IR 리모컨 등을 이용해 **시간 동기화·백업·원격 제어 기능**을 갖춘 디지털 알람 시계를 구현했습니다.  
FreeRTOS 기반 멀티태스킹 구조를 적용해 시간 표시, 알람 제어, NTP 동기화, UI 상태 관리 등이 동시에 동작하도록 설계했습니다.  

---

## 🔧 개발 기간
- **2025.08.04 ~ 2025.08.08 (5일)**

---

## 🛠 개발 환경 및 도구
### 🔹 Hardware
- STM32 Nucleo-F411RE (MCU, FreeRTOS 구동, 메인 제어)  
- DS1302 RTC 모듈 (전원 차단 시 시간 백업 & 복원)  
- ESP-01 WiFi 모듈 (NTP 서버 통한 시간 동기화)  
- LCD1602 I2C 모듈 (시간, 알람, 상태 표시 UI)  
- IR 리모컨 + 적외선 수신기 (시간/알람 원격 제어)  
- Buzzer + LED (알람 출력)  

### 🔹 Software
- STM32CubeIDE (코드 작성 및 디버깅)
- FreeRTOS (멀티태스킹 스케줄러)
- HAL Driver (STM32 주변장치 제어)
- C 언어 (펌웨어 개발)

---

## ⚙️ 주요 기능
- **시간 관리**
  - STM32 내부 RTC 사용, DS1302에 3초 주기로 백업  
  - 전원 차단 후 재부팅 시 DS1302 → STM32 시간 복원  
  - ESP-01 WiFi를 통한 NTP 서버 동기화  

- **UI 표시**
  - LCD 1행: 현재 시간  
  - LCD 2행: 날짜, 알람 시간, 모드 상태, 백업/복원 메시지  

- **알람 기능**
  - RTC 시간이 알람과 일치 → Buzzer + LED 알림  
  - LCD에 "Alarm Triggered" 출력  

- **리모컨 제어 (NEC 프로토콜)**
  - `<<` 버튼 → 시간 설정 모드  
  - `Play/Pause` 버튼 → 알람 설정 모드  
  - `CH-/CH/CH+` → 시/분/초 선택  
  - `EQ` 버튼 → 설정 저장 및 모드 종료  
  - `-` 버튼 → 날짜 표시  

---

## 💡 사용 기술
C / STM32 / FreeRTOS / RTC / UART / I2C / GPIO / WiFi / IR Remote / NTP  

---

## 🔄 펌웨어 동작 원리
- **FreeRTOS 기반 Task 구조**
  - `TimeTask` : 내장 RTC를 polling 방식으로 1초 단위 시간 갱신  
  - `LCDTask` : Queue & Mutex 기반, 다른 태스크로부터 받은 문자열 출력  
  - `AlarmTask` : 현재 시간과 알람 시간 비교 → 알람 조건 시 부저/LED 제어  
  - `IRTask` : IR 수신기 인터럽트 기반, 버튼별 상태 전환 처리  
  - `UIStateTask` : 현재 상태/알람 설정/복원/동기화 실패 등을 LCD 2행에 표시  
  - `NTPTask` : WiFi 모듈(ESP-01)로 NTP 서버와 통신, 시간 업데이트  

- **상태 전이 흐름**
  - IDLE 모드 → 시간 설정 모드 / 알람 설정 모드 전환 (리모컨 입력)  
  - EQ 버튼으로 설정 저장 후 IDLE 복귀  
  - 알람 시간 도달 시 AlarmTask 활성화  

- **시간 백업 & 복원**
  - STM32 내부 RTC 시간 → DS1302로 주기적 백업  
  - WiFi 모듈이 정상 동작 시 NTP 시간 → DS1302 저장  
  - 전원 차단 후 복원 시 DS1302 시간을 STM32 RTC로 복구  

---

## 🐞 Trouble Shooting
- **WiFi 모듈 불안정**  
  - ESP-01 통신 중 무응답 → 핀 재연결/하드리셋/재시도 로직 적용  
- **LCD 출력 깨짐**  
  - 다중 태스크가 동시에 LCD 접근 시 글자 깨짐 → Mutex & Queue 적용  
- **RTC 출력 지연**  
  - 시간 출력이 1초 단위로 정확하지 않음 → Polling 방식 적용해 보정  
- **IR 리모컨 신호 불안정**  
  - 버튼 입력 여러 번 눌러야 동작 → NEC 코드 맵핑 및 신호 안정화  

---

## 📈 개발 성과
- **WiFi 기반 실시간 시간 동기화** 구현 (기존 알람 시계 대비 차별화)  
- **시간 백업/복원 기능** → 전원 차단에도 안정적 동작 보장  
- **리모컨 제어 기반 UI** → 사용자 편의성 강화  
- **FreeRTOS 멀티태스킹** → 시간/알람/통신/표시를 동시에 안정적 수행  

---

## 🧩 FreeRTOS 기반 Task 구조
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
