/*
 * json_parser.h - 軽量 JSON パーサー（受信用）
 *
 * ArduinoJson のような高機能ではなく、固定フォーマットの JSON を解析
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

/* 解析結果の型 */
#define JP_TYPE_UNKNOWN  0
#define JP_TYPE_SENSOR   1
#define JP_TYPE_CMD      2

typedef struct {
    int type;
    /* sensor */
    long temp_x100;
    long humi_x100;
    long pres_x100;
    /* cmd */
    char cmd[16];
    long kp, ki, kd;
    long sp_x100;
} json_parsed_t;

int json_parse(const char *buf, json_parsed_t *out);

#endif /* JSON_PARSER_H */
