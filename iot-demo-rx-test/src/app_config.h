/*
 * app_config.h - アプリケーション共通設定
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* タスク優先度 */
#define PRIORITY_WDT        4
#define PRIORITY_UART       3
#define PRIORITY_PID        2
#define PRIORITY_ANOMALY    2
#define PRIORITY_STATUS     1

/* タスクスタックサイズ (ワード単位) */
#define STACK_UART          512
#define STACK_PID           256
#define STACK_ANOMALY       256
#define STACK_WDT           128
#define STACK_STATUS        256

/* PID デフォルト値 (×100 固定小数点) */
#define DEFAULT_KP          300
#define DEFAULT_KI          80
#define DEFAULT_KD          20
#define DEFAULT_TARGET      2800    /* 28.00 ℃ */

/* UART タイムアウト (ms) */
#define SENSOR_TIMEOUT_MS   5000

/* JSON バッファサイズ */
#define JSON_BUF_SIZE       192

/* 共有データ構造 */
typedef struct {
    long temp_x100;     /* 温度 × 100 */
    long humi_x100;     /* 湿度 × 100 */
    long pres_x100;     /* 気圧 × 100 */
    unsigned long timestamp;
} sensor_data_t;

/* グローバル共有変数 (Mutexで保護) */
extern SemaphoreHandle_t g_data_mutex;
extern sensor_data_t     g_sensor;
extern long              g_pwm;
extern long              g_setpoint;
extern long              g_kp, g_ki, g_kd;
extern volatile int      g_emergency_stop;
extern volatile unsigned long g_task_alive_bits;

/* タスク生存ビット */
#define ALIVE_UART      (1 << 0)
#define ALIVE_PID       (1 << 1)
#define ALIVE_ANOMALY   (1 << 2)
#define ALIVE_STATUS    (1 << 3)
#define ALIVE_ALL       (ALIVE_UART | ALIVE_PID | ALIVE_ANOMALY | ALIVE_STATUS)

#endif /* APP_CONFIG_H */
