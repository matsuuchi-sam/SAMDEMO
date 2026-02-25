/**
 * @file main.cpp
 * @brief SAMDEMO ESP32 メインファームウェア
 *
 * 対応接続方式（wifi_config.h の CONNECTION_MODE で切り替え）:
 *   MODE 1: USB Serial    開発・デバッグ向け。ケーブル必須。
 *   MODE 2: WiFi WebSocket デモ向け。ワイヤレス。PC で graph 表示。
 *   MODE 3: Bluetooth SPP  WiFi不要のワイヤレス。PCのBT内蔵が必要。
 *
 * 物理配線:
 *   ┌──────────────────────────────────────────────────────┐
 *   │  BME280         GR-SAKURA (RX63N)    ESP32 (esp32dev)│
 *   │  VCC  ──────── 3.3V                                  │
 *   │  GND  ──────── GND  ──────────────── GND             │
 *   │  SDA  ──────── P16 (SDA0)                            │
 *   │  SCL  ──────── P17 (SCL0)                            │
 *   │                P20 (TXD0) ──────── GPIO16 (RX1)      │
 *   │                P21 (RXD0) ──────── GPIO17 (TX1)      │
 *   │                                    USB ─── PC        │
 *   └──────────────────────────────────────────────────────┘
 *   ※ GR-SAKURA も ESP32 も 3.3V 動作 → レベル変換不要
 *   ※ GND は必ず共通にすること
 */

#include <Arduino.h>
#include "wifi_config.h"

// ===========================================================
// 接続方式ごとのインクルード
// ===========================================================
#if CONNECTION_MODE == CONNECTION_MODE_WIFI
  #include <WiFi.h>
  #include <WebSocketsClient.h>
  #include <ArduinoJson.h>

#elif CONNECTION_MODE == CONNECTION_MODE_BT
  #include <BluetoothSerial.h>
  #if !defined(CONFIG_BT_ENABLED)
    #error "このボードは Bluetooth に対応していません"
  #endif

#endif  // CONNECTION_MODE

// ===========================================================
// 定数・グローバル変数
// ===========================================================
#define SAKURA_RX_PIN   RXSAKURA_UART_RX_PIN   // GPIO16
#define SAKURA_TX_PIN   RXSAKURA_UART_TX_PIN   // GPIO17
#define SAKURA_BAUD     RXSAKURA_UART_BAUD     // 115200

// GR-SAKURA からの受信バッファ
static char     rx_buf[256];
static int      rx_buf_pos = 0;
static uint32_t packet_count = 0;

// 接続オブジェクト（モードごとに有効化）
#if CONNECTION_MODE == CONNECTION_MODE_WIFI
  static WebSocketsClient wsClient;
  static bool             ws_connected = false;

#elif CONNECTION_MODE == CONNECTION_MODE_BT
  static BluetoothSerial  btSerial;
#endif

// ===========================================================
// 共通：センサーデータ1行を処理・送信する
// ===========================================================
static void send_sensor_data(const char *line)
{
    if (!line || line[0] == '\0') return;
    packet_count++;

#if CONNECTION_MODE == CONNECTION_MODE_USB
    // USB Serial にそのまま出力（monitor.py や pio monitor で確認）
    Serial.printf("[%05lu] %s\n", packet_count, line);

#elif CONNECTION_MODE == CONNECTION_MODE_WIFI
    // WiFi WebSocket でPCの server.py へ送信
    if (ws_connected) {
        wsClient.sendTXT(line);
        Serial.printf("[WS TX %05lu] %s\n", packet_count, line);
    } else {
        Serial.printf("[WS OFFLINE %05lu] %s\n", packet_count, line);
    }

#elif CONNECTION_MODE == CONNECTION_MODE_BT
    // Bluetooth SPP で送信（PC に仮想 COM ポートとして見える）
    if (btSerial.connected()) {
        btSerial.println(line);
        Serial.printf("[BT TX %05lu] %s\n", packet_count, line);
    } else {
        Serial.printf("[BT OFFLINE %05lu] %s\n", packet_count, line);
    }
#endif
}

// ===========================================================
// WiFi モード: WebSocket イベントハンドラ
// ===========================================================
#if CONNECTION_MODE == CONNECTION_MODE_WIFI
static void ws_event(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type) {
        case WStype_DISCONNECTED:
            ws_connected = false;
            Serial.println("[WS] 切断 - 再接続を試みます...");
            break;

        case WStype_CONNECTED:
            ws_connected = true;
            Serial.printf("[WS] 接続完了: ws://%s:%d\n",
                          WS_SERVER_IP, WS_SERVER_PORT);
            // 接続通知を server.py へ送る
            wsClient.sendTXT("{\"type\":\"hello\",\"device\":\"ESP32\"}");
            break;

        case WStype_TEXT:
            // server.py からのメッセージ（ACK 等）
            Serial.printf("[WS RX] %s\n", (char *)payload);
            break;

        default:
            break;
    }
}
#endif  // CONNECTION_MODE_WIFI

// ===========================================================
// setup() - 起動時の初期化
// ===========================================================
void setup()
{
    // USB デバッグ用シリアル
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=== SAMDEMO ESP32 Firmware ===");

#if CONNECTION_MODE == CONNECTION_MODE_USB
    Serial.println("接続モード: USB Serial");
    Serial.println("PC で monitor.py または pio device monitor を起動してください");

#elif CONNECTION_MODE == CONNECTION_MODE_WIFI
    Serial.println("接続モード: WiFi WebSocket");
    Serial.printf("接続先 WiFi: %s\n", WIFI_SSID);
    Serial.printf("接続先 server.py: ws://%s:%d\n", WS_SERVER_IP, WS_SERVER_PORT);

    // WiFi 接続
    Serial.print("WiFi 接続中");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi 接続完了! ESP32 IP: %s\n",
                      WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi 接続失敗! wifi_config.h の SSID/PASSWORD を確認してください");
    }

    // WebSocket 接続設定（server.py への接続）
    wsClient.begin(WS_SERVER_IP, WS_SERVER_PORT, "/");
    wsClient.onEvent(ws_event);
    wsClient.setReconnectInterval(3000);  // 3秒で再接続

#elif CONNECTION_MODE == CONNECTION_MODE_BT
    Serial.println("接続モード: Bluetooth SPP");
    Serial.printf("デバイス名: %s\n", BT_DEVICE_NAME);
    Serial.println("PC の Bluetooth でペアリングしてください");

    btSerial.begin(BT_DEVICE_NAME);
    Serial.println("Bluetooth 起動完了 - ペアリング待機中...");
#endif

    // GR-SAKURA との UART 通信を初期化
    Serial1.begin(SAKURA_BAUD, SERIAL_8N1, SAKURA_RX_PIN, SAKURA_TX_PIN);
    Serial.printf("GR-SAKURA UART: RX=GPIO%d TX=GPIO%d %dbps\n",
                  SAKURA_RX_PIN, SAKURA_TX_PIN, SAKURA_BAUD);
    Serial.println("GR-SAKURA からのデータ待機中...\n");
}

// ===========================================================
// loop() - メインループ
// ===========================================================
void loop()
{
#if CONNECTION_MODE == CONNECTION_MODE_WIFI
    // WebSocket の定期処理（再接続・受信処理）
    wsClient.loop();
#endif

    // GR-SAKURA からの UART データを受信
    while (Serial1.available() > 0)
    {
        char c = (char)Serial1.read();

        if (c == '\n') {
            // 1行受信完了 → 送信処理
            rx_buf[rx_buf_pos] = '\0';
            if (rx_buf_pos > 0) {
                send_sensor_data(rx_buf);
            }
            rx_buf_pos = 0;
        }
        else if (c != '\r') {
            if (rx_buf_pos < (int)(sizeof(rx_buf) - 1)) {
                rx_buf[rx_buf_pos++] = c;
            } else {
                // バッファオーバーフロー対処
                Serial.println("[WARN] RX buffer overflow");
                rx_buf_pos = 0;
            }
        }
    }
}
