#include "uart_comm.h"

void UartComm::begin() {
    Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

void UartComm::sendSensor(const BmeData &d) {
    JsonDocument doc;
    doc["type"] = "sensor";
    doc["temp"] = round(d.temp * 10.0f) / 10.0f;
    doc["humi"] = round(d.humi * 10.0f) / 10.0f;
    doc["pres"] = round(d.pres * 100.0f) / 100.0f;

    char buf[UART_BUF_SIZE];
    size_t len = serializeJson(doc, buf, sizeof(buf) - 1);
    buf[len] = '\n';
    buf[len + 1] = '\0';
    Serial1.print(buf);
}

void UartComm::sendRaw(const char *json) {
    Serial1.print(json);
    Serial1.print('\n');
}

bool UartComm::receive(char *buf, size_t bufSize) {
    while (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n' || c == '\r') {
            if (rxPos_ > 0) {
                size_t copyLen = rxPos_ < bufSize - 1 ? rxPos_ : bufSize - 1;
                memcpy(buf, rxBuf_, copyLen);
                buf[copyLen] = '\0';
                rxPos_ = 0;
                return true;
            }
        } else if (rxPos_ < sizeof(rxBuf_) - 1) {
            rxBuf_[rxPos_++] = c;
        } else {
            rxPos_ = 0;
        }
    }
    return false;
}
