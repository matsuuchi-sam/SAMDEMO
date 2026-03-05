/*
 * pid_ctrl.h - PID 制御モジュール
 *
 * 整数演算 (× 100 固定小数点)
 * アンチワインドアップ付き
 */

#ifndef PID_CTRL_H
#define PID_CTRL_H

typedef struct {
    long kp_x100;       /* Kp × 100 */
    long ki_x100;       /* Ki × 100 */
    long kd_x100;       /* Kd × 100 */
    long target_x100;   /* 目標値 × 100 */
    long integral;      /* 積分値 (内部) */
    long prev_error;    /* 前回偏差 (内部) */
    long output_min;    /* 出力下限 */
    long output_max;    /* 出力上限 */
} pid_t;

void pid_init(pid_t *pid, long kp, long ki, long kd);
void pid_set_target(pid_t *pid, long target_x100);
void pid_set_gains(pid_t *pid, long kp, long ki, long kd);
long pid_compute(pid_t *pid, long measured_x100);
void pid_reset(pid_t *pid);

#endif /* PID_CTRL_H */
