/*
 * main.c - SAMDEMO GR-SAKURA FreeRTOS メイン
 *
 * タスク構成:
 *   uart_task   (優先度3) - ESP32からのJSON受信
 *   pid_task    (優先度2) - PID制御 500ms周期
 *   anomaly_task(優先度2) - 異常検知 1s周期
 *   wdt_task    (優先度4) - WDT監視 5s周期
 *   status_task (優先度1) - LED点滅+報告 3s周期
 */

#include "app_config.h"
#include "sci2_uart.h"
#include "uart_task.h"
#include "pid_task.h"
#include "anomaly_task.h"
#include "wdt_task.h"
#include "status_task.h"
#include "iodefine.h"

/* グローバル共有変数 */
SemaphoreHandle_t g_data_mutex;
sensor_data_t     g_sensor;
long              g_pwm = 0;
long              g_setpoint = DEFAULT_TARGET;
long              g_kp = DEFAULT_KP;
long              g_ki = DEFAULT_KI;
long              g_kd = DEFAULT_KD;
volatile int      g_emergency_stop = 0;
volatile unsigned long g_task_alive_bits = 0;

/* FreeRTOS: CMT0 を tick タイマーとして設定 */
void vApplicationSetupTimerInterrupt(void)
{
    /* モジュールストップ解除 */
    SYSTEM.PRCR.WORD = 0xA502;
    SYSTEM.MSTPCRA.BIT.MSTPA15 = 0;
    SYSTEM.PRCR.WORD = 0xA500;

    CMT.CMSTR0.BIT.STR0 = 0;

    CMT0.CMCR.BIT.CKS = 0;    /* PCLK/8 */
    CMT0.CMCR.BIT.CMIE = 1;

    /* 1ms tick: 50MHz / 8 / 1000 - 1 = 6249 */
    CMT0.CMCOR = (unsigned short)((configCPU_CLOCK_HZ / 8 / configTICK_RATE_HZ) - 1);
    CMT0.CMCNT = 0;

    /* CMT0_CMI0: ベクタ28, 優先度 = configKERNEL_INTERRUPT_PRIORITY */
    ICU.IPR[4].BIT.IPR = configKERNEL_INTERRUPT_PRIORITY;
    ICU.IR[28].BIT.IR = 0;
    ICU.IER[0x03].BIT.IEN4 = 1;

    CMT.CMSTR0.BIT.STR0 = 1;
}

/* FreeRTOS: スタックオーバーフローフック */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main(void)
{
    /* UART初期化 */
    sci2_init();
    sci2_puts("\r\n=== SAMDEMO RX63N FreeRTOS ===\r\n");

    /* Mutex作成 */
    g_data_mutex = xSemaphoreCreateMutex();

    /* 初期値 */
    g_sensor.temp_x100 = 0;
    g_sensor.humi_x100 = 0;
    g_sensor.pres_x100 = 0;
    g_sensor.timestamp = 0;

    /* タスク作成 */
    xTaskCreate(uart_task,    "UART",    STACK_UART,    NULL, PRIORITY_UART,    NULL);
    xTaskCreate(pid_task,     "PID",     STACK_PID,     NULL, PRIORITY_PID,     NULL);
    xTaskCreate(anomaly_task, "ANOMALY", STACK_ANOMALY, NULL, PRIORITY_ANOMALY, NULL);
    xTaskCreate(wdt_task,     "WDT",     STACK_WDT,     NULL, PRIORITY_WDT,    NULL);
    xTaskCreate(status_task,  "STATUS",  STACK_STATUS,  NULL, PRIORITY_STATUS,  NULL);

    /* スケジューラ開始 (戻らない) */
    vTaskStartScheduler();

    /* ここには到達しない */
    for (;;) {}
    return 0;
}
