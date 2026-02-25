/**
 * @file main.cpp
 * @brief ESP32 + BME280 センサーデモ
 *
 * 温度・湿度・気圧を1秒ごとに読み取り、USB Serial で JSON 送信する。
 * PC の server.py + index.html でリアルタイムグラフ表示できる。
 *
 * ─── 配線（4本だけ）────────────────────────────
 *   BME280        ESP32 (esp32dev)
 *   VCC  ──────── 3.3V  ※5Vピンには絶対つながない
 *   GND  ──────── GND
 *   SDA  ──────── GPIO21 (デフォルトSDA)
 *   SCL  ──────── GPIO22 (デフォルトSCL)
 * ─────────────────────────────────────────────────
 *
 * 送信 JSON フォーマット（1秒ごと、改行区切り）:
 *   {"type":"sensor","temp":25.3,"humidity":60.1,"pressure":1013.2,"ts":12345}
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ─── 設定 ────────────────────────────────────────
#define SEND_INTERVAL_MS  1000   // 送信間隔（ミリ秒）
#define SERIAL_BAUD       115200 // シリアル通信速度

// BME280 の I2C アドレス
// 多くのモジュール: 0x76（SDO を GND に接続）
// 一部のモジュール: 0x77（SDO を VCC に接続）
#define BME280_ADDR  0x76
// ─────────────────────────────────────────────────

Adafruit_BME280 bme;

static uint32_t packet_count = 0;
static uint32_t last_send_ms = 0;

// ─── 初期化 ──────────────────────────────────────
void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println("\n=== SAMDEMO: ESP32 + BME280 ===");

    // BME280 を初期化（0x76 で失敗したら 0x77 も試す）
    bool ok = bme.begin(BME280_ADDR);
    if (!ok) {
        Serial.println("[RETRY] 0x76 で見つからず、0x77 を試します...");
        ok = bme.begin(0x77);
    }

    if (!ok) {
        Serial.println("[ERROR] BME280 が見つかりません！");
        Serial.println("  確認ポイント:");
        Serial.println("  1. 配線: SDA=GPIO21, SCL=GPIO22, VCC=3.3V");
        Serial.println("  2. VCC に 5V を接続していないか");
        Serial.println("  3. GND が ESP32 と共通になっているか");
        // 見つかるまでループ（USB を刺し直して再起動で復帰）
        while (true) {
            delay(2000);
            Serial.println("[WAIT] BME280 を待機中...");
            if (bme.begin(BME280_ADDR) || bme.begin(0x77)) {
                Serial.println("[OK] BME280 を検出しました！");
                break;
            }
        }
    }

    Serial.println("[OK] BME280 初期化完了");
    Serial.println("[OK] 1秒ごとに JSON を送信します");
    Serial.println();
    Serial.println("─── PC での確認方法 ──────────────────────────────");
    Serial.println("  シリアルモニタ: pio device monitor");
    Serial.println("  グラフ表示:     python ../dashboard/server.py --port <COMポート>");
    Serial.println("                  その後 index.html をブラウザで開く");
    Serial.println("──────────────────────────────────────────────────");
    Serial.println();
}

// ─── メインループ ─────────────────────────────────
void loop()
{
    uint32_t now = millis();
    if (now - last_send_ms < SEND_INTERVAL_MS) return;
    last_send_ms = now;

    // センサー値を読み取る
    float temp     = bme.readTemperature();           // 摂氏
    float humidity = bme.readHumidity();              // %RH
    float pressure = bme.readPressure() / 100.0F;    // hPa に変換
    uint32_t ts    = now / 1000;                      // 秒単位のタイムスタンプ

    // 異常値チェック（センサー未接続や読み取りエラー）
    if (isnan(temp) || isnan(humidity) || isnan(pressure)) {
        Serial.println("[WARN] センサー読み取りエラー。配線を確認してください。");
        return;
    }

    packet_count++;

    // JSON で出力（dashboard の server.py がこのフォーマットを期待している）
    Serial.printf(
        "{\"type\":\"sensor\",\"temp\":%.1f,\"humidity\":%.1f,\"pressure\":%.1f,\"ts\":%lu}\n",
        temp, humidity, pressure, ts
    );
}
