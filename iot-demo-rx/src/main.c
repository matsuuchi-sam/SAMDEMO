/*
 * main.c - GR-SAKURA (RX63N) PID温度制御（新設計）
 *
 * ESP32からセンサーJSON受信 → PID演算 → PWM値をJSON返送
 * ESP32がヒーターMOSFETを駆動
 *
 * データフロー:
 *   ESP32 → {"type":"sensor","temp":25.3,"humi":48.2,"pres":1013.25}
 *   RX63N → {"type":"ctrl","vtemp":25.30,"pwm":128,"sp":28.00}
 */

#include "iodefine.h"
#include "sci0_uart.h"
#include "cmt_timer.h"
#include "cmd_parser.h"
#include "json_builder.h"
#include "pid_ctrl.h"

/* PID コントローラ */
static pid_t g_pid;

/* 制御状態 */
static int g_running = 1;          /* 0=停止, 1=運転 */
static long g_last_temp_x100 = 0;  /* 最新温度 */

static void delay_loop(volatile unsigned long n)
{
    while (n--) __asm("nop");
}

int main(void)
{
    json_buf_t jb;
    int i;

    /* LED 起動確認: 3回点滅 */
    PORTA.PDR.BYTE = 0xFF;
    PORTB.PDR.BYTE = 0xFF;
    PORTC.PDR.BYTE = 0xFF;
    PORTD.PDR.BYTE = 0xFF;
    PORTE.PDR.BYTE = 0xFF;

    for (i = 0; i < 3; i++) {
        PORTA.PODR.BYTE = 0xFF; PORTB.PODR.BYTE = 0xFF;
        PORTC.PODR.BYTE = 0xFF; PORTD.PODR.BYTE = 0xFF;
        PORTE.PODR.BYTE = 0xFF;
        delay_loop(2500000);
        PORTA.PODR.BYTE = 0x00; PORTB.PODR.BYTE = 0x00;
        PORTC.PODR.BYTE = 0x00; PORTD.PODR.BYTE = 0x00;
        PORTE.PODR.BYTE = 0x00;
        delay_loop(2500000);
    }

    /* 周辺初期化 */
    sci0_init();
    cmt0_init();

    /* PID 初期化: Kp=3.00, Ki=0.80, Kd=0.20 (x100) */
    pid_init(&g_pid, 300, 80, 20);
    pid_set_target(&g_pid, 2800);  /* デフォルト目標: 28.00℃ */

    /* 割り込み有効化 */
    __asm("setpsw i");

    json_build_status(&jb, "boot ok");
    sci0_puts(jb.buf);

    while (1) {
        /* UART受信処理 */
        int c = sci0_trygetc();
        if (c >= 0) {
            cmd_feed((char)c);
            msg_result_t msg = cmd_poll();

            switch (msg.type) {
            case MSG_SENSOR: {
                g_last_temp_x100 = msg.temp_x100;
                int pwm = 0;

                if (g_running) {
                    long pid_out = pid_compute(&g_pid, msg.temp_x100);
                    /* PID出力 0-10000 → PWM 0-255 */
                    pwm = (int)(pid_out * 255 / 10000);
                    if (pwm < 0) pwm = 0;
                    if (pwm > 255) pwm = 255;
                }

                /* 制御JSONをESP32に返送 */
                json_build_ctrl(&jb, msg.temp_x100, pwm,
                                g_pid.target_x100);
                sci0_puts(jb.buf);

                /* LED トグル（動作確認） */
                PORTE.PODR.BYTE ^= 0xFF;
                break;
            }
            case MSG_CMD_SET_TARGET:
                pid_set_target(&g_pid, msg.sp_x100);
                pid_reset(&g_pid);
                g_running = 1;
                /* 即座にctrl JSONで新SP値をブラウザに反映 */
                json_build_ctrl(&jb, g_last_temp_x100, 0,
                                g_pid.target_x100);
                sci0_puts(jb.buf);
                break;

            case MSG_CMD_STOP:
                g_running = 0;
                pid_reset(&g_pid);
                /* PWM=0 を即座に送信 */
                json_build_ctrl(&jb, g_last_temp_x100, 0,
                                g_pid.target_x100);
                sci0_puts(jb.buf);
                json_build_status(&jb, "stopped");
                sci0_puts(jb.buf);
                break;

            case MSG_CMD_START:
                g_running = 1;
                pid_reset(&g_pid);
                json_build_status(&jb, "started");
                sci0_puts(jb.buf);
                break;

            case MSG_CMD_SET_PID:
                pid_set_gains(&g_pid, msg.kp_x100, msg.ki_x100,
                              msg.kd_x100);
                json_build_status(&jb, "pid set");
                sci0_puts(jb.buf);
                break;

            default:
                break;
            }
        }
    }

    return 0;
}
