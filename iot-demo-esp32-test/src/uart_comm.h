#ifndef UART_COMM_H
#define UART_COMM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "bme_reader.h"

#define UART_RX_PIN 16
#define UART_TX_PIN 17
#define UART_BAUD   115200
#define UART_BUF_SIZE 256

class UartComm {
public:
    void begin();
    void sendSensor(const BmeData &d);
    void sendRaw(const char *json);
    bool receive(char *buf, size_t bufSize);
private:
    char rxBuf_[UART_BUF_SIZE];
    size_t rxPos_ = 0;
};

#endif
