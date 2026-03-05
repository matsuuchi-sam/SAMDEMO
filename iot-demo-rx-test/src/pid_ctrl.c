/*
 * pid_ctrl.c - PID 制御モジュール
 *
 * 出力: 0〜10000 (0%〜100.00%)
 * 制御周期: 1 秒を想定
 */

#include "pid_ctrl.h"

#define INTEGRAL_MAX  500000L
#define INTEGRAL_MIN -500000L

void pid_init(pid_t *pid, long kp, long ki, long kd)
{
    pid->kp_x100 = kp;
    pid->ki_x100 = ki;
    pid->kd_x100 = kd;
    pid->target_x100 = 2500;   /* デフォルト 25.00 ℃ */
    pid->integral = 0;
    pid->prev_error = 0;
    pid->output_min = 0;
    pid->output_max = 10000;   /* 100.00% */
}

void pid_set_target(pid_t *pid, long target_x100)
{
    pid->target_x100 = target_x100;
}

void pid_set_gains(pid_t *pid, long kp, long ki, long kd)
{
    pid->kp_x100 = kp;
    pid->ki_x100 = ki;
    pid->kd_x100 = kd;
    pid->integral = 0;
}

long pid_compute(pid_t *pid, long measured_x100)
{
    long error, p_term, i_term, d_term, output;

    error = pid->target_x100 - measured_x100;

    /* P 項 */
    p_term = (pid->kp_x100 * error) / 100;

    /* I 項 (アンチワインドアップ) */
    pid->integral += error;
    if (pid->integral > INTEGRAL_MAX)
        pid->integral = INTEGRAL_MAX;
    if (pid->integral < INTEGRAL_MIN)
        pid->integral = INTEGRAL_MIN;
    i_term = (pid->ki_x100 * pid->integral) / 100;

    /* D 項 */
    d_term = (pid->kd_x100 * (error - pid->prev_error)) / 100;
    pid->prev_error = error;

    /* 合計 + クランプ */
    output = p_term + i_term + d_term;

    if (output > pid->output_max)
        output = pid->output_max;
    if (output < pid->output_min)
        output = pid->output_min;

    return output;
}

void pid_reset(pid_t *pid)
{
    pid->integral = 0;
    pid->prev_error = 0;
}
