// pins.h - sơ đồ chân và các công tắc biên dịch.
// Theo tài liệu "chi_ti_t_ph_n_c_ng_s_ch_n_stm32.md" (đã chạy thật trên xe).
#pragma once
#include <Arduino.h>

// ---------------- Động cơ (L298N) ----------------
#define PIN_ENA PA11   // PWM bánh trái
#define PIN_IN1 PA5
#define PIN_IN2 PA4
#define PIN_ENB PA9    // PWM bánh phải
#define PIN_IN3 PA3
#define PIN_IN4 PA2
// Nếu bánh nào quay ngược khi chạy chế độ TEST thì đổi 0 <-> 1 (khỏi phải hàn lại dây)
#define MOTOR_LEFT_INVERT 0
#define MOTOR_RIGHT_INVERT 0

// ---------------- Dò line 5 mắt TCRT5000 (PB11..PB15 liền nhau) ----------------
#define PIN_L2 PB11
#define PIN_L1 PB12
#define PIN_M  PB13
#define PIN_R1 PB14
#define PIN_R2 PB15
#define LINE_BLACK_IS_HIGH 1   // tài liệu: BLACK = 1

// ---------------- Vật cản E18-D80NK ----------------
#define PIN_OBS_L PA8
#define PIN_OBS_R PA10
#define OBS_ACTIVE_HIGH 1      // tài liệu: HIGH khi có vật

// ---------------- TCS3200 (sườn trái, đọc tốt ở ~1,5 cm) ----------------
#define PIN_TCS_S2  PB0
#define PIN_TCS_S3  PB1
#define PIN_TCS_OUT PB10       // chân chịu 5 V
#define TCS3200_TIMEOUT_US 3000

// TCS34725 (phải) đã bỏ sau test lần 1. PB6/PB7 để trống.

// ---------------- Báo hiệu ----------------
#define PIN_BUZZER PA1         // qua NPN S8050: HIGH = kêu
#define PIN_LED    PC13        // LED trên Blue Pill: LOW = sáng

// ---------------- Debug (UART mềm chỉ TX) ----------------
#define DEBUG_LOG 1
#define PIN_DBG_TX PB5
#define PIN_DBG_RX PB4         // không dùng, SoftwareSerial bắt buộc có chân RX
#define DEBUG_BAUD 57600

// ---------------- Vòng 2: cơ cấu gắp (servo) ----------------
#ifndef ENABLE_GRIPPER
#define ENABLE_GRIPPER 0
#endif
#define PIN_SERVO_ARM  PA6
#define PIN_SERVO_GRIP PA7

// ---------------- Watchdog ----------------
#define WATCHDOG_US 500000
