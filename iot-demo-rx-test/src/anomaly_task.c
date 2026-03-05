/*
 * anomaly_task.c - 異常検知タスク (優先度2, 1s周期)
 *
 * - 温度変化率監視 (5℃/s超過で警報)
 * - UART切断検知 (5秒以上データなし → 緊急停止)
 * - 蓋開放検知 (急激な温度低下)
 */

#include "app_config.h"
#include "anomaly_task.h"
#include "json_builder.h"
#include "sci2_uart.h"

#define TEMP_RATE_LIMIT     500     /* 5.00℃/s */
#define LID_OPEN_THRESHOLD  -300    /* -3.00℃/s */

void anomaly_task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    long prev_temp = 0;
    unsigned long prev_ts = 0;
    json_buf_t jb;

    (void)pvParameters;

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
        g_task_alive_bits |= ALIVE_ANOMALY;

        long cur_temp;
        unsigned long cur_ts;

        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            cur_temp = g_sensor.temp_x100;
            cur_ts = g_sensor.timestamp;
            xSemaphoreGive(g_data_mutex);
        } else {
            continue;
        }

        /* UART切断検知: 最後のセンサーデータから5秒以上経過 */
        TickType_t now = xTaskGetTickCount();
        if (cur_ts > 0 && (now - cur_ts) > pdMS_TO_TICKS(SENSOR_TIMEOUT_MS)) {
            if (!g_emergency_stop) {
                g_emergency_stop = 1;
                json_build_status(&jb, "UART_TIMEOUT:ESTOP");
                sci2_puts(jb.buf);
            }
        }

        /* 温度変化率チェック */
        if (prev_ts > 0) {
            long rate = cur_temp - prev_temp;

            if (rate > TEMP_RATE_LIMIT) {
                json_build_status(&jb, "TEMP_RISE_FAST");
                sci2_puts(jb.buf);
            }

            if (rate < LID_OPEN_THRESHOLD) {
                json_build_status(&jb, "LID_OPEN_DETECT");
                sci2_puts(jb.buf);
            }
        }

        prev_temp = cur_temp;
        prev_ts = cur_ts;
    }
}
