/**
 * SAMDEMO ESP32 - センサー + ヒーターPWM + UART + Webサーバー
 *
 * 新設計: ESP32がBME280を直接制御し、GR-SAKURAにセンサーデータをUART送信
 *
 * 配線:
 *   BME280 SCL → GPIO22, SDA → GPIO21
 *   ヒーター MOSFET Gate → GPIO26 (PWM)
 *   UART: GPIO16(RX) ↔ GR-SAKURA TX, GPIO17(TX) ↔ GR-SAKURA RX
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "bme_reader.h"
#include "heater_pwm.h"
#include "uart_comm.h"
#include "web_server.h"

static BmeReader   bme;
static HeaterPwm   heater;
static UartComm    uart;
static WebDashboard web;

static unsigned long lastSensorMs = 0;
static const unsigned long SENSOR_INTERVAL = 1000;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== SAMDEMO ESP32: Sensor + Heater + Web ===");

    if (bme.begin()) {
        Serial.println("[BME280] OK");
    } else {
        Serial.println("[BME280] NOT FOUND - check wiring");
    }

    heater.begin();
    Serial.println("[HEATER] PWM on GPIO26 ready");

    uart.begin();
    Serial.println("[UART] Serial1 ready (GPIO16=RX, GPIO17=TX)");

    web.begin();

    Serial.println("[READY] System started");
    Serial.println();
}

void loop() {
    unsigned long now = millis();

    // 1秒毎: BME280読取 → UART送信 + WS配信
    if (now - lastSensorMs >= SENSOR_INTERVAL) {
        lastSensorMs = now;

        BmeData d = bme.read();
        if (d.valid) {
            // UART → GR-SAKURA
            uart.sendSensor(d);

            // WebSocket → ブラウザ
            JsonDocument doc;
            doc["type"] = "sensor";
            doc["temp"] = round(d.temp * 10.0f) / 10.0f;
            doc["humi"] = round(d.humi * 10.0f) / 10.0f;
            doc["pres"] = round(d.pres * 100.0f) / 100.0f;
            doc["pwm"]  = heater.get();

            char buf[256];
            serializeJson(doc, buf, sizeof(buf));
            web.broadcast(buf);

            Serial.printf("[SENSOR] T=%.1f H=%.1f P=%.2f PWM=%d\n",
                          d.temp, d.humi, d.pres, heater.get());
        }
    }

    // GR-SAKURAからの制御JSON受信 → PWM適用 + WS配信
    char rxBuf[256];
    if (uart.receive(rxBuf, sizeof(rxBuf))) {
        Serial.printf("[RX←GR] %s\n", rxBuf);

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, rxBuf);
        if (!err) {
            const char *type = doc["type"] | "";
            if (strcmp(type, "ctrl") == 0) {
                int pwm = doc["pwm"] | 0;
                if (pwm >= 0 && pwm <= 255) {
                    heater.set((uint8_t)pwm);
                }
                // ブラウザに制御データ転送
                web.broadcast(rxBuf);
            }
        }
    }

    // ブラウザからのコマンド → UART経由でGR-SAKURAへ転送
    char cmdBuf[256];
    if (web.hasCommand(cmdBuf, sizeof(cmdBuf))) {
        Serial.printf("[WS→GR] %s\n", cmdBuf);
        uart.sendRaw(cmdBuf);
    }

    // WebSocketクリーンアップ
    static unsigned long lastCleanup = 0;
    if (now - lastCleanup > 1000) {
        lastCleanup = now;
        // AsyncWebSocketの内部クリーンアップはライブラリが自動実行
    }
}
