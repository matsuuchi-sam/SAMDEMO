/*
 * cmd_parser.c - UART JSON パーサー
 *
 * ESP32からのJSON行を解析する軽量パーサー
 */

#include <stddef.h>
#include "cmd_parser.h"

#define LINE_BUF_SIZE 256

static char line_buf[LINE_BUF_SIZE];
static int  line_pos = 0;
static int  line_ready = 0;

/* 文字列を検索して位置を返す。見つからなければ NULL */
static const char *find_str(const char *hay, const char *needle)
{
    const char *h, *n;
    while (*hay) {
        h = hay;
        n = needle;
        while (*n && *h == *n) { h++; n++; }
        if (*n == '\0') return hay;
        hay++;
    }
    return NULL;
}

/* "key": の後の値位置を返す（コロンの次の非空白） */
static const char *find_value(const char *json, const char *key)
{
    char search[32];
    const char *p;
    const char *start = json;
    int i = 0;

    search[i++] = '"';
    while (*key && i < 28) search[i++] = *key++;
    search[i++] = '"';
    search[i] = '\0';

    /* キーとして使われている箇所を探す（直後に ':' がある） */
    while ((p = find_str(start, search)) != NULL) {
        const char *after = p + i;
        /* 空白をスキップ */
        while (*after == ' ') after++;
        if (*after == ':') {
            /* コロンの次が値 */
            after++;
            while (*after == ' ') after++;
            return after;
        }
        /* コロンがなければ値側のマッチ → 次を探す */
        start = p + 1;
    }
    return NULL;
}

/* 浮動小数点数値を x100 整数でパース: "25.3" → 2530, "-1.05" → -105 */
static long parse_fixed100(const char *s)
{
    long integer = 0, frac = 0, frac_div = 1;
    int neg = 0, has_dot = 0;

    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }

    while (*s >= '0' && *s <= '9') {
        integer = integer * 10 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        has_dot = 1;
        /* 最大2桁まで */
        if (*s >= '0' && *s <= '9') { frac = (*s - '0') * 10; frac_div = 1; s++; }
        if (*s >= '0' && *s <= '9') { frac += (*s - '0'); s++; }
        (void)has_dot;
    }

    long val = integer * 100 + frac;
    return neg ? -val : val;
}

/* 整数パース */
static long parse_int(const char *s)
{
    long val = 0;
    int neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return neg ? -val : val;
}

/* 文字列値を比較: "target" → 1 if match */
static int value_is(const char *p, const char *str)
{
    if (*p == '"') p++;
    while (*str) {
        if (*p != *str) return 0;
        p++; str++;
    }
    return 1;
}

void cmd_feed(char c)
{
    if (line_ready)
        return;

    if (c == '\n' || c == '\r') {
        if (line_pos > 0) {
            line_buf[line_pos] = '\0';
            line_ready = 1;
        }
        return;
    }

    if (line_pos < LINE_BUF_SIZE - 1) {
        line_buf[line_pos++] = c;
    }
}

msg_result_t cmd_poll(void)
{
    msg_result_t r;
    const char *v;

    r.type = MSG_NONE;
    r.temp_x100 = 0;
    r.humi_x100 = 0;
    r.pres_x100 = 0;
    r.sp_x100 = 0;
    r.kp_x100 = 0;
    r.ki_x100 = 0;
    r.kd_x100 = 0;

    if (!line_ready)
        return r;

    /* "type" フィールドを探す */
    v = find_value(line_buf, "type");
    if (!v) goto done;

    if (value_is(v, "sensor")) {
        r.type = MSG_SENSOR;
        v = find_value(line_buf, "temp");
        if (v) r.temp_x100 = parse_fixed100(v);
        v = find_value(line_buf, "humi");
        if (v) r.humi_x100 = parse_fixed100(v);
        v = find_value(line_buf, "pres");
        if (v) r.pres_x100 = parse_fixed100(v);
    } else if (value_is(v, "cmd")) {
        v = find_value(line_buf, "cmd");
        if (!v) { r.type = MSG_CMD_UNKNOWN; goto done; }

        if (value_is(v, "set_target")) {
            r.type = MSG_CMD_SET_TARGET;
            v = find_value(line_buf, "sp");
            if (v) r.sp_x100 = parse_fixed100(v);
        } else if (value_is(v, "stop")) {
            r.type = MSG_CMD_STOP;
        } else if (value_is(v, "start")) {
            r.type = MSG_CMD_START;
        } else if (value_is(v, "set_pid")) {
            r.type = MSG_CMD_SET_PID;
            v = find_value(line_buf, "kp");
            if (v) r.kp_x100 = parse_int(v);
            v = find_value(line_buf, "ki");
            if (v) r.ki_x100 = parse_int(v);
            v = find_value(line_buf, "kd");
            if (v) r.kd_x100 = parse_int(v);
        } else {
            r.type = MSG_CMD_UNKNOWN;
        }
    }

done:
    line_pos = 0;
    line_ready = 0;
    return r;
}
