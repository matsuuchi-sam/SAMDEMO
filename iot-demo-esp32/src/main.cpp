/**
 * @file main.cpp
 * @brief SAMDEMO ESP32 + BME280 ファームウェア
 *
 * ビルド環境（platformio.ini の env）で接続方式を切り替える:
 *   pio run -e usb_mode   USB Serial → python server.py --port COM4
 *   pio run -e bt_mode    Bluetooth  → python server.py --port <仮想COM>
 *
 * 配線（共通）:
 *   BME280 VCC → 3.3V
 *   BME280 GND → GND
 *   BME280 SDA → GPIO21
 *   BME280 SCL → GPIO22
 *
 * 送信 JSON（1秒ごと・改行区切り）:
 *   {"type":"sensor","temp":24.4,"humidity":43.9,"pressure":1018.3,"heater":false,"ts":42}
 *
 * 受信コマンド（PC → ESP32）:
 *   HEATER_ON   GPIO26 を HIGH にしてヒーター ON
 *   HEATER_OFF  GPIO26 を LOW  にしてヒーター OFF
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ── BT モード時のみインクルード ────────────────────────
#ifdef USE_BLUETOOTH
  #include <BluetoothSerial.h>
  #if !defined(CONFIG_BT_ENABLED)
    #error "このボードは Bluetooth に対応していません"
  #endif
  static BluetoothSerial BT;
  #define BT_DEVICE_NAME "SAMDEMO-ESP32"
#endif

// ── 設定 ──────────────────────────────────────────────
#define SEND_INTERVAL_MS  5000
#define SERIAL_BAUD       115200
#define BME280_ADDR       0x76   // 多くのモジュール: 0x76、一部: 0x77
#define HEATER_PIN        26     // IRLZ44N Gate 制御ピン

// ── グローバル ─────────────────────────────────────────
static Adafruit_BME280 bme;
static uint32_t last_send_ms = 0;
static uint32_t packet_count = 0;
static bool     heater_on    = false;

// ── JSON を出力する先を1か所で切り替え ────────────────
static void send_line(const char* line) {
#ifdef USE_BLUETOOTH
    if (BT.connected()) {
        BT.println(line);
    }
    // BT 未接続時でもデバッグ用に USB には出力する
    Serial.println(line);
#else
    Serial.println(line);
#endif
}

// ── PC からのコマンドを受信してヒーターを制御 ──────────
static void check_commands() {
    String line = "";
#ifdef USE_BLUETOOTH
    if (BT.available()) {
        line = BT.readStringUntil('\n');
    }
#else
    if (Serial.available()) {
        line = Serial.readStringUntil('\n');
    }
#endif
    line.trim();
    if (line.length() == 0) return;

    if (line == "HEATER_ON") {
        heater_on = true;
        digitalWrite(HEATER_PIN, HIGH);
        send_line("{\"type\":\"heater\",\"state\":\"on\"}");
        Serial.println("[HEATER] ON");
    } else if (line == "HEATER_OFF") {
        heater_on = false;
        digitalWrite(HEATER_PIN, LOW);
        send_line("{\"type\":\"heater\",\"state\":\"off\"}");
        Serial.println("[HEATER] OFF");
    }
}

// ── 初期化 ────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println("\n=== SAMDEMO: ESP32 + BME280 ===");

#ifdef USE_BLUETOOTH
    // ── Bluetooth SPP 起動 ──
    BT.begin(BT_DEVICE_NAME);
    Serial.println("[BT] Bluetooth SPP 起動");
    Serial.println("[BT] デバイス名: " BT_DEVICE_NAME);
    Serial.println("[BT] ---- PC 側の操作 ----");
    Serial.println("[BT] 1. Windows の Bluetooth 設定を開く");
    Serial.println("[BT]    設定 > Bluetooth とその他のデバイス > デバイスの追加");
    Serial.println("[BT] 2. 一覧に SAMDEMO-ESP32 が出たらクリック");
    Serial.println("[BT] 3. デバイスマネージャーで COM 番号を確認");
    Serial.println("[BT]    ポート(COM と LPT) > Standard Serial over BT link");
    Serial.println("[BT] 4. python server.py --port COM<番号> を実行");
    Serial.println("[BT] ---------------------");
#else
    // ── USB Serial モード ──
    Serial.println("[USB] USB Serial モード");
    Serial.println("[USB] python server.py --port COM4");
    Serial.println("[USB] http://localhost:8080 でグラフ確認");
#endif

    // ── BME280 初期化 ──
    bool ok = bme.begin(BME280_ADDR);
    if (!ok) {
        Serial.println("[BME280] 0x76 で未検出 → 0x77 を試します");
        ok = bme.begin(0x77);
    }
    if (!ok) {
        Serial.println("[ERROR] BME280 が見つかりません！配線を確認してください。");
        while (true) {
            delay(2000);
            if (bme.begin(BME280_ADDR) || bme.begin(0x77)) {
                Serial.println("[BME280] 検出できました。");
                break;
            }
        }
    }
    // ── ヒーターピン初期化（必ず LOW から開始）──
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(HEATER_PIN, LOW);
    Serial.println("[HEATER] GPIO26 初期化 → OFF");

    Serial.println("[BME280] 初期化完了 → 送信開始");
    Serial.println();
}

// ── メインループ ──────────────────────────────────────
void loop() {
    check_commands();  // PC からのコマンドを常時チェック

    uint32_t now = millis();
    if (now - last_send_ms < SEND_INTERVAL_MS) return;
    last_send_ms = now;

    float temp     = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    if (isnan(temp) || isnan(humidity) || isnan(pressure)) {
        Serial.println("[WARN] センサー読み取りエラー");
        return;
    }

    char buf[160];
    snprintf(buf, sizeof(buf),
        "{\"type\":\"sensor\",\"temp\":%.1f,\"humidity\":%.1f,"
        "\"pressure\":%.1f,\"heater\":%s,\"ts\":%lu}",
        temp, humidity, pressure,
        heater_on ? "true" : "false",
        now / 1000UL);

    send_line(buf);
    packet_count++;
}
