/*
 * json_builder.h - 軽量 JSON 生成 (FreeRTOS版)
 */

#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include "app_config.h"

typedef struct {
    char buf[JSON_BUF_SIZE];
    int  len;
} json_buf_t;

/* 制御JSON生成
 * {"type":"ctrl","vtemp":26.10,"pwm":128,"sp":28.00,"kp":3.00,"ki":0.80,"kd":0.20}
 */
void json_build_ctrl(json_buf_t *jb, long vtemp_x100, long pwm,
                     long sp_x100, long kp_x100, long ki_x100, long kd_x100);

/* ステータスJSON生成 */
void json_build_status(json_buf_t *jb, const char *msg);

#endif /* JSON_BUILDER_H */
