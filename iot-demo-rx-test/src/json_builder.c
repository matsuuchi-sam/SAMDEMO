/*
 * json_builder.c - 軽量 JSON 生成 (FreeRTOS版)
 */

#include "json_builder.h"

static void jb_append_char(json_buf_t *jb, char c)
{
    if (jb->len < JSON_BUF_SIZE - 1)
        jb->buf[jb->len++] = c;
}

static void jb_append_str(json_buf_t *jb, const char *s)
{
    while (*s)
        jb_append_char(jb, *s++);
}

static void jb_append_int(json_buf_t *jb, long val)
{
    char tmp[12];
    int i = 0;
    unsigned long uval;

    if (val < 0) {
        jb_append_char(jb, '-');
        uval = (unsigned long)(-(val + 1)) + 1;
    } else {
        uval = (unsigned long)val;
    }

    if (uval == 0) {
        jb_append_char(jb, '0');
        return;
    }

    while (uval > 0) {
        tmp[i++] = '0' + (char)(uval % 10);
        uval /= 10;
    }
    while (i > 0)
        jb_append_char(jb, tmp[--i]);
}

static void jb_append_fixed2(json_buf_t *jb, long val_x100)
{
    long integer, frac;

    if (val_x100 < 0) {
        jb_append_char(jb, '-');
        val_x100 = -val_x100;
    }

    integer = val_x100 / 100;
    frac = val_x100 % 100;

    jb_append_int(jb, integer);
    jb_append_char(jb, '.');
    if (frac < 10)
        jb_append_char(jb, '0');
    jb_append_int(jb, frac);
}

static void jb_key_str(json_buf_t *jb, const char *key, const char *val)
{
    jb_append_char(jb, '"');
    jb_append_str(jb, key);
    jb_append_str(jb, "\":\"");
    jb_append_str(jb, val);
    jb_append_char(jb, '"');
}

static void jb_key_int(json_buf_t *jb, const char *key, long val)
{
    jb_append_char(jb, '"');
    jb_append_str(jb, key);
    jb_append_str(jb, "\":");
    jb_append_int(jb, val);
}

static void jb_key_fixed2(json_buf_t *jb, const char *key, long val_x100)
{
    jb_append_char(jb, '"');
    jb_append_str(jb, key);
    jb_append_str(jb, "\":");
    jb_append_fixed2(jb, val_x100);
}

void json_build_ctrl(json_buf_t *jb, long vtemp_x100, long pwm,
                     long sp_x100, long kp_x100, long ki_x100, long kd_x100)
{
    jb->len = 0;
    jb_append_char(jb, '{');

    jb_key_str(jb, "type", "ctrl");
    jb_append_char(jb, ',');

    jb_key_fixed2(jb, "vtemp", vtemp_x100);
    jb_append_char(jb, ',');

    jb_key_int(jb, "pwm", pwm);
    jb_append_char(jb, ',');

    jb_key_fixed2(jb, "sp", sp_x100);
    jb_append_char(jb, ',');

    jb_key_fixed2(jb, "kp", kp_x100);
    jb_append_char(jb, ',');

    jb_key_fixed2(jb, "ki", ki_x100);
    jb_append_char(jb, ',');

    jb_key_fixed2(jb, "kd", kd_x100);

    jb_append_char(jb, '}');
    jb_append_char(jb, '\n');
    jb->buf[jb->len] = '\0';
}

void json_build_status(json_buf_t *jb, const char *msg)
{
    jb->len = 0;
    jb_append_char(jb, '{');

    jb_key_str(jb, "type", "status");
    jb_append_char(jb, ',');

    jb_key_str(jb, "msg", msg);

    jb_append_char(jb, '}');
    jb_append_char(jb, '\n');
    jb->buf[jb->len] = '\0';
}
