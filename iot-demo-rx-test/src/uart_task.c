/*
 * uart_task.c - UART 受信/送信タスク (優先度3)
 *
 * ESP32からのJSONを受信し、センサーデータまたはコマンドを処理
 */

#include "app_config.h"
#include "uart_task.h"
#include "sci2_uart.h"
#include "json_parser.h"
#include <string.h>

void uart_task(void *pvParameters)
{
    char line[JSON_BUF_SIZE];
    json_parsed_t parsed;

    (void)pvParameters;

    for (;;) {
        g_task_alive_bits |= ALIVE_UART;

        int len = sci2_readline(line, sizeof(line), SENSOR_TIMEOUT_MS);
        if (len <= 0) {
            /* タイムアウト: センサーデータ受信なし */
            continue;
        }

        if (json_parse(line, &parsed) != 0)
            continue;

        if (parsed.type == JP_TYPE_SENSOR) {
            if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                g_sensor.temp_x100 = parsed.temp_x100;
                g_sensor.humi_x100 = parsed.humi_x100;
                g_sensor.pres_x100 = parsed.pres_x100;
                g_sensor.timestamp = xTaskGetTickCount();
                xSemaphoreGive(g_data_mutex);
            }
        } else if (parsed.type == JP_TYPE_CMD) {
            if (strcmp(parsed.cmd, "set_pid") == 0) {
                if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    if (parsed.kp > 0) g_kp = parsed.kp;
                    if (parsed.ki > 0) g_ki = parsed.ki;
                    if (parsed.kd > 0) g_kd = parsed.kd;
                    xSemaphoreGive(g_data_mutex);
                }
            } else if (strcmp(parsed.cmd, "set_target") == 0) {
                if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    if (parsed.sp_x100 > 0) g_setpoint = parsed.sp_x100;
                    xSemaphoreGive(g_data_mutex);
                }
            } else if (strcmp(parsed.cmd, "stop") == 0) {
                g_emergency_stop = 1;
            } else if (strcmp(parsed.cmd, "start") == 0) {
                g_emergency_stop = 0;
            }
        }
    }
}
