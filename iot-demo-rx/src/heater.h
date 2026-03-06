/*
 * heater.h - ヒーター GPIO 制御
 *
 * GPIO ピンで IRLZ44N MOSFET を駆動
 * ヒーターポート: PA0 (仮。GR-SAKURA の空きピンに応じて変更)
 */

#ifndef HEATER_H
#define HEATER_H

void heater_init(void);
void heater_on(void);
void heater_off(void);
int  heater_state(void);

/* PID 出力 (0-10000) に基づくオンオフ判定 */
/* threshold = 5000 (50%) 以上でオン */
void heater_set_from_pid(long pid_output);

#endif /* HEATER_H */
