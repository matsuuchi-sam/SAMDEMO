/*
 * wdt_task.c - WDT 監視タスク (優先度4, 5s周期)
 *
 * 全タスクの生存確認。いずれかが応答しなければ緊急停止。
 * 現在はソフトウェアWDTのみ (ハードウェアWDTは後で追加可能)
 */

#include "app_config.h"
#include "wdt_task.h"
#include "json_builder.h"
#include "sci2_uart.h"

void wdt_task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    json_buf_t jb;

    (void)pvParameters;

    xLastWakeTime = xTaskGetTickCount();

    /* 起動直後は各タスクが初期化中なので初回は待つ */
    vTaskDelay(pdMS_TO_TICKS(3000));

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5000));

        unsigned long bits = g_task_alive_bits;

        if ((bits & ALIVE_ALL) != ALIVE_ALL) {
            /* いずれかのタスクが応答なし */
            if (!g_emergency_stop) {
                g_emergency_stop = 1;
                json_build_status(&jb, "WDT_TASK_DEAD");
                sci2_puts(jb.buf);
            }
        }

        /* ビットクリア (次の5秒で各タスクが再セットする) */
        g_task_alive_bits = 0;
    }
}
