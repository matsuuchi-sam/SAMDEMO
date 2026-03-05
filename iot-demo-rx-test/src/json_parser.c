/*
 * json_parser.c - 軽量 JSON パーサー
 *
 * {"type":"sensor","temp":25.3,"humi":48.2,"pres":1013.25}
 * {"type":"cmd","cmd":"set_pid","kp":300,"ki":80,"kd":20}
 * {"type":"cmd","cmd":"set_target","sp":28.0}
 */

#include "json_parser.h"
#include <string.h>

static const char *find_key(const char *buf, const char *key)
{
    char pattern[32];
    int plen = 0;
    const char *p;

    pattern[plen++] = '"';
    while (*key && plen < 28)
        pattern[plen++] = *key++;
    pattern[plen++] = '"';
    pattern[plen++] = ':';
    pattern[plen] = '\0';

    p = strstr(buf, pattern);
    if (!p) return 0;
    return p + plen;
}

static long parse_fixed100(const char *p)
{
    long sign = 1;
    long integer = 0;
    long frac = 0;
    int frac_digits = 0;

    if (*p == '-') { sign = -1; p++; }

    while (*p >= '0' && *p <= '9') {
        integer = integer * 10 + (*p - '0');
        p++;
    }

    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9' && frac_digits < 2) {
            frac = frac * 10 + (*p - '0');
            frac_digits++;
            p++;
        }
        while (frac_digits < 2) {
            frac *= 10;
            frac_digits++;
        }
    } else {
        frac = 0;
    }

    return sign * (integer * 100 + frac);
}

static long parse_long(const char *p)
{
    long sign = 1;
    long val = 0;

    if (*p == '-') { sign = -1; p++; }
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    return sign * val;
}

static void parse_string(const char *p, char *out, int maxlen)
{
    int i = 0;
    if (*p == '"') p++;
    while (*p && *p != '"' && i < maxlen - 1)
        out[i++] = *p++;
    out[i] = '\0';
}

int json_parse(const char *buf, json_parsed_t *out)
{
    const char *v;
    char type_str[16] = {0};

    memset(out, 0, sizeof(*out));
    out->type = JP_TYPE_UNKNOWN;

    v = find_key(buf, "type");
    if (!v) return -1;
    parse_string(v, type_str, sizeof(type_str));

    if (strcmp(type_str, "sensor") == 0) {
        out->type = JP_TYPE_SENSOR;

        v = find_key(buf, "temp");
        if (v) out->temp_x100 = parse_fixed100(v);

        v = find_key(buf, "humi");
        if (v) out->humi_x100 = parse_fixed100(v);

        v = find_key(buf, "pres");
        if (v) out->pres_x100 = parse_fixed100(v);

        return 0;
    }

    if (strcmp(type_str, "cmd") == 0) {
        out->type = JP_TYPE_CMD;

        v = find_key(buf, "cmd");
        if (v) parse_string(v, out->cmd, sizeof(out->cmd));

        v = find_key(buf, "kp");
        if (v) out->kp = parse_long(v);

        v = find_key(buf, "ki");
        if (v) out->ki = parse_long(v);

        v = find_key(buf, "kd");
        if (v) out->kd = parse_long(v);

        v = find_key(buf, "sp");
        if (v) out->sp_x100 = parse_fixed100(v);

        return 0;
    }

    return -1;
}
