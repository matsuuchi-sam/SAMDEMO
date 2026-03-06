/**
 * @file main.cpp
 * @brief SAMDEMO ESP32 — UART ↔ Bluetooth SPP ブリッジ
 *
 * Phase 7: RX63N が BME280 を直接制御するようになったため、
 * ESP32 は GR-SAKURA の UART 出力を Bluetooth SPP (または USB)
 * 経由で PC に転送するブリッジとして動作する。
 *
 * ビルド環境（platformio.ini の env）で切り替え:
 *   pio run -e usb_mode   USB Serial → python server.py --port COM4
 *   pio run -e bt_mode    Bluetooth  → python server.py --port <BT仮想COM>
 *
 * 配線:
 *   GR-SAKURA P20 (TXD0) → ESP32 GPIO16 (RX1)
 *   GR-SAKURA P21 (RXD0) ← ESP32 GPIO17 (TX1)  [コマンド送信用]
 *   GND → GND
 *
 * データフロー:
 *   GR-SAKURA → UART → ESP32 Serial1 → BT/USB → PC
 *   PC → BT/USB → ESP32 → Serial1 → GR-SAKURA
 */

#include <Arduino.h>

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
#define SERIAL_BAUD  115200

// GR-SAKURA UART ピン
#define GR_RX_PIN    16    // ESP32 RX1 ← GR-SAKURA TXD0
#define GR_TX_PIN    17    // ESP32 TX1 → GR-SAKURA RXD0

// ── 初期化 ────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println("\n=== SAMDEMO: ESP32 UART-Bridge ===");

#ifdef USE_BLUETOOTH
    BT.begin(BT_DEVICE_NAME);
    Serial.println("[BT] Bluetooth SPP 起動");
    Serial.println("[BT] デバイス名: " BT_DEVICE_NAME);
    Serial.println("[BT] PC で Bluetooth ペアリング後、仮想 COM ポートを確認");
#else
    Serial.println("[USB] USB Serial ブリッジモード");
#endif

    // GR-SAKURA UART 接続
    Serial1.begin(SERIAL_BAUD, SERIAL_8N1, GR_RX_PIN, GR_TX_PIN);
    Serial.println("[UART] Serial1 初期化 (GPIO16=RX, GPIO17=TX)");
    Serial.println("[BRIDGE] GR-SAKURA ↔ ESP32 ↔ PC ブリッジ開始");
    Serial.println();
}

// ── メインループ ──────────────────────────────────────
void loop() {
    // ── GR-SAKURA → PC (BT/USB) ──
    while (Serial1.available()) {
        int c = Serial1.read();
#ifdef USE_BLUETOOTH
        if (BT.connected()) {
            BT.write(c);
        }
#endif
        Serial.write(c);   // USB にも常時出力 (デバッグ用)
    }

    // ── PC (BT/USB) → GR-SAKURA ──
#ifdef USE_BLUETOOTH
    while (BT.available()) {
        int c = BT.read();
        Serial1.write(c);
        Serial.write(c);   // エコー
    }
#endif
    while (Serial.available()) {
        int c = Serial.read();
        Serial1.write(c);
    }
}
