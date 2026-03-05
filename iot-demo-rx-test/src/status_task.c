/*
 * status_task.c - ステータス報告タスク (優先度1, 3s周期)
 *
 * LED点滅 + 稼働状況の定期報告
 */

#include "app_config.h"
#include "status_task.h"
#include "json_builder.h"
#include "sci2_uart.h"
#include "iodefine.h"

static int led_state = 0;

void status_task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    json_buf_t jb;

    (void)pvParameters;

    /* LED ポート初期化 (PORTE.B0) */
    PORTE.PDR.BIT.B0 = 1;   /* 出力 */
    PORTE.PODR.BIT.B0 = 0;  /* 消灯 */

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000));
        g_task_alive_bits |= ALIVE_STATUS;

        /* LED トグル */
        led_state = !led_state;
        PORTE.PODR.BIT.B0 = led_state;

        /* ステータス報告 */
        if (g_emergency_stop) {
            json_build_status(&jb, "ESTOP_ACTIVE");
        } else {
            json_build_status(&jb, "OK");
        }
        sci2_puts(jb.buf);
    }
}
