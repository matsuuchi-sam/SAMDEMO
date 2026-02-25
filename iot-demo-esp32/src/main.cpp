/**
 * @file main.cpp
 * @brief SAMDEMO ESP32 メインファームウェア
 *
 * Phase 1: UART エコー動作確認スケルトン
 *
 * ハードウェア構成:
 *   - ESP32 (esp32dev)
 *   - Serial (USB): デバッグ出力 115200bps
 *   - Serial1 (GPIO16/17): GR-SAKURA RX63N との通信 115200bps
 *
 * 将来の拡張予定:
 *   - [EXTEND: WIFI] WiFi 接続とルータへの接続
 *   - [EXTEND: WEBSOCKET] WebSocket クライアント/サーバーの実装
 *   - [EXTEND: JSON] GR-SAKURA からの JSON データをパースして転送
 */

#include <Arduino.h>

/* ===========================================================================
 * 設定定数
 * platformio.ini の build_flags で定義された値を使用
 * =========================================================================*/
#ifndef RXSAKURA_UART_RX_PIN
  #define RXSAKURA_UART_RX_PIN  16
#endif

#ifndef RXSAKURA_UART_TX_PIN
  #define RXSAKURA_UART_TX_PIN  17
#endif

#ifndef RXSAKURA_UART_BAUD
  #define RXSAKURA_UART_BAUD    115200
#endif

#define DEBUG_BAUD  115200   /* USB シリアル（デバッグ）の通信速度 */

/* ===========================================================================
 * グローバル変数
 * =========================================================================*/

/* GR-SAKURA との通信バッファ */
static char rx_buf[256];
static int  rx_buf_pos = 0;

/* ===========================================================================
 * 関数プロトタイプ
 * =========================================================================*/
static void process_sakura_line(const char *line);

/* ===========================================================================
 * setup() - Arduino 初期化関数
 * =========================================================================*/
void setup()
{
    /* USB シリアル（デバッグ用）の初期化 */
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {
        delay(10);  /* ポートが開くまで待機（最大 1 秒）*/
        static int wait_count = 0;
        if (++wait_count > 100) break;
    }

    /* GR-SAKURA との UART 通信の初期化 */
    /* Serial1: RX=GPIO16, TX=GPIO17 */
    Serial1.begin(RXSAKURA_UART_BAUD, SERIAL_8N1,
                  RXSAKURA_UART_RX_PIN, RXSAKURA_UART_TX_PIN);

    Serial.println("=== SAMDEMO ESP32 Firmware ===");
    Serial.println("Phase 1: UART Echo Test");
    Serial.printf("  GR-SAKURA UART: RX=GPIO%d, TX=GPIO%d, %d bps\n",
                  RXSAKURA_UART_RX_PIN, RXSAKURA_UART_TX_PIN, RXSAKURA_UART_BAUD);
    Serial.println("");
    Serial.println("Waiting for data from GR-SAKURA...");

    /*
     * [EXTEND: WIFI]
     * WiFi 接続の初期化:
     *   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
     *   while (WiFi.status() != WL_CONNECTED) { delay(500); }
     *   Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
     */

    /*
     * [EXTEND: WEBSOCKET]
     * WebSocket サーバーの初期化:
     *   webSocket.begin();
     *   webSocket.onEvent(webSocketEvent);
     */
}

/* ===========================================================================
 * loop() - Arduino メインループ
 * =========================================================================*/
void loop()
{
    /* --- GR-SAKURA からのデータ受信（Serial1）--- */
    while (Serial1.available() > 0)
    {
        char c = (char)Serial1.read();

        /* 改行を受信したら行処理 */
        if (c == '\n')
        {
            rx_buf[rx_buf_pos] = '\0';

            /* デバッグ: 受信データをそのまま USB シリアルに出力 */
            Serial.print("[FROM SAKURA] ");
            Serial.println(rx_buf);

            /* 受信行を処理 */
            process_sakura_line(rx_buf);

            /* エコーバック（GR-SAKURA への折り返し送信）*/
            Serial1.print("[ACK] ");
            Serial1.println(rx_buf);

            /* バッファをリセット */
            rx_buf_pos = 0;
        }
        else if (c != '\r')  /* \r は無視 */
        {
            /* バッファに蓄積（オーバーフロー防止）*/
            if (rx_buf_pos < (int)(sizeof(rx_buf) - 1))
            {
                rx_buf[rx_buf_pos++] = c;
            }
            else
            {
                /* バッファオーバーフロー: リセット */
                Serial.println("[WARN] RX buffer overflow, resetting");
                rx_buf_pos = 0;
            }
        }
    }

    /* --- USB シリアルからのコマンド受信（デバッグ用）--- */
    while (Serial.available() > 0)
    {
        char c = (char)Serial.read();
        /* USB から受信した文字を GR-SAKURA へ転送（デバッグ用）*/
        Serial1.write(c);
    }

    /*
     * [EXTEND: WEBSOCKET]
     * WebSocket の定期処理:
     *   webSocket.loop();
     */
}

/* ===========================================================================
 * GR-SAKURA からの1行データを処理する
 *
 * Phase 1: デバッグ出力のみ
 * Phase 3 以降: JSON パースして WebSocket で転送する
 * =========================================================================*/
static void process_sakura_line(const char *line)
{
    if (line == nullptr || line[0] == '\0') {
        return;
    }

    /*
     * [EXTEND: JSON]
     * Phase 3 での実装予定:
     *
     * #include <ArduinoJson.h>
     *
     * StaticJsonDocument<256> doc;
     * DeserializationError error = deserializeJson(doc, line);
     * if (error) {
     *     Serial.printf("[ERROR] JSON parse failed: %s\n", error.c_str());
     *     return;
     * }
     *
     * float temp     = doc["temp"];
     * float humidity = doc["humidity"];
     * float pressure = doc["pressure"];
     *
     * Serial.printf("[DATA] temp=%.1f, humidity=%.1f, pressure=%.1f\n",
     *               temp, humidity, pressure);
     *
     * // WebSocket で PC に転送
     * webSocket.broadcastTXT(line);
     */

    /* Phase 1: 受信行をそのままログ出力（process_sakura_line はすでに上で呼ばれている）*/
    /* 追加の処理が必要になった時点でここに実装を追加する */
}
