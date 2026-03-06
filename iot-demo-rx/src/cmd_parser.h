/*
 * cmd_parser.h - UART JSON パーサー
 *
 * ESP32からのJSON行を解析
 * {"type":"sensor","temp":25.3,"humi":48.2,"pres":1013.25}
 * {"type":"cmd","cmd":"set_target","sp":28.0}
 * {"type":"cmd","cmd":"stop"}
 * {"type":"cmd","cmd":"start"}
 * {"type":"cmd","cmd":"set_pid","kp":300,"ki":80,"kd":20}
 */

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

typedef enum {
    MSG_NONE = 0,
    MSG_SENSOR,
    MSG_CMD_SET_TARGET,
    MSG_CMD_STOP,
    MSG_CMD_START,
    MSG_CMD_SET_PID,
    MSG_CMD_UNKNOWN
} msg_type_t;

typedef struct {
    msg_type_t type;
    long temp_x100;
    long humi_x100;
    long pres_x100;
    long sp_x100;
    long kp_x100;
    long ki_x100;
    long kd_x100;
} msg_result_t;

void cmd_feed(char c);
msg_result_t cmd_poll(void);

#endif
