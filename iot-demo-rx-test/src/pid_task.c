/*
 * pid_task.c - PID 制御タスク (優先度2, 500ms周期)
 *
 * PID演算 → PWM値算出 → 制御JSON送信
 */

#include "app_config.h"
#include "pid_task.h"
#include "pid_ctrl.h"
#include "json_builder.h"
#include "sci2_uart.h"

void pid_task(void *pvParameters)
{
    pid_t pid;
    TickType_t xLastWakeTime;
    json_buf_t jb;
    long local_temp, local_sp, local_kp, local_ki, local_kd;

    (void)pvParameters;

    pid_init(&pid, DEFAULT_KP, DEFAULT_KI, DEFAULT_KD);
    pid_set_target(&pid, DEFAULT_TARGET);

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
        g_task_alive_bits |= ALIVE_PID;

        /* 共有データ読み出し */
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            local_temp = g_sensor.temp_x100;
            local_sp   = g_setpoint;
            local_kp   = g_kp;
            local_ki   = g_ki;
            local_kd   = g_kd;
            xSemaphoreGive(g_data_mutex);
        } else {
            continue;
        }

        /* PIDパラメータ更新チェック */
        if (pid.kp_x100 != local_kp || pid.ki_x100 != local_ki || pid.kd_x100 != local_kd) {
            pid_set_gains(&pid, local_kp, local_ki, local_kd);
        }
        if (pid.target_x100 != local_sp) {
            pid_set_target(&pid, local_sp);
        }

        /* 緊急停止チェック */
        long pwm;
        if (g_emergency_stop) {
            pwm = 0;
            pid_reset(&pid);
        } else {
            /* PID演算 */
            long output = pid_compute(&pid, local_temp);
            /* output: 0-10000 → PWM: 0-255 */
            pwm = (output * 255) / 10000;
            if (pwm < 0) pwm = 0;
            if (pwm > 255) pwm = 255;
        }

        /* グローバルPWM値を更新 */
        if (xSemaphoreTake(g_data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_pwm = pwm;
            xSemaphoreGive(g_data_mutex);
        }

        /* 制御JSON → ESP32 */
        json_build_ctrl(&jb, local_temp, pwm, local_sp, local_kp, local_ki, local_kd);
        sci2_puts(jb.buf);
    }
}
