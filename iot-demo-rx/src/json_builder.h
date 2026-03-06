/*
 * json_builder.h - 軽量 JSON 生成
 */

#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#define JSON_BUF_SIZE 128

typedef struct {
    char buf[JSON_BUF_SIZE];
    int  len;
} json_buf_t;

/* 制御データ JSON を生成 */
/* {"type":"ctrl","vtemp":25.30,"pwm":128,"sp":28.00} */
void json_build_ctrl(json_buf_t *jb, long vtemp_x100, int pwm, long sp_x100);

/* ステータス JSON を生成 */
void json_build_status(json_buf_t *jb, const char *msg);

#endif
