# SAMDEMO プロジェクト概要

## システム概要

セメント抵抗を発熱体としたPID温度制御システム。
ESP32とGR-SAKURA（RX63N）をUART接続し、センサー読み取り・PID演算・ヒーター制御・Webダッシュボードまでを一貫して構築。

## システム構成図

```
[BME280] ──I2C──→ [ESP32] ──UART──→ [GR-SAKURA RX63N]
                    │                    │
                    │               PID 制御演算
                    │               PWM 出力値決定
                    │                    │
[IRLZ44N] ←────────┘────────────────────┘
    │                   (ctrlJSON経由でPWM値を受取)
[セメント抵抗] → 発熱
    │
[タッパー] ← 加熱チャンバー

[ブラウザ] ──WebSocket──→ [ESP32] → リアルタイム表示 + パラメータ変更
```

## 各デバイスの役割

| デバイス | 役割 |
|---------|------|
| ESP32 | BME280センサー読取、UART送受信、ヒーターPWM出力、WiFi AP、Webダッシュボード |
| GR-SAKURA (RX63N) | PID温度制御演算、PWM値算出、コマンド処理 |
| BME280 | 温度・湿度・気圧センサー (I2C) |
| IRLZ44N | N-ch MOSFET、ヒーター電流スイッチング |
| セメント抵抗 | 発熱体 |

## データフロー

### センサーデータ（1秒周期）
```
BME280 ──I2C──→ ESP32 ──UART──→ GR-SAKURA
                  │                  │
                  │              PID演算
                  │                  │
                  │  ←──UART────  ctrl JSON (PWM値)
                  │
                  ├──→ ヒーターPWM出力 (GPIO26)
                  └──→ WebSocket → ブラウザ表示
```

### コマンド（ユーザー操作時）
```
ブラウザ ──WebSocket──→ ESP32 ──UART──→ GR-SAKURA
                                          │
                                     コマンド処理
                                     (目標温度変更/停止/開始)
                                          │
                          ESP32 ←──UART── ctrl JSON
                            │
                            └──→ WebSocket → ブラウザ更新
```

## UART プロトコル (115200bps 8N1, JSON改行区切り)

### ESP32 → GR-SAKURA（センサーデータ）
```json
{"type":"sensor","temp":25.3,"humi":48.2,"pres":1013.25}
```

### GR-SAKURA → ESP32（制御応答）
```json
{"type":"ctrl","vtemp":25.30,"pwm":128,"sp":28.00}
```

### ブラウザ → ESP32 → GR-SAKURA（コマンド）
```json
{"type":"cmd","cmd":"set_target","sp":30.0}
{"type":"cmd","cmd":"stop"}
{"type":"cmd","cmd":"start"}
{"type":"cmd","cmd":"set_pid","kp":300,"ki":80,"kd":20}
```

## 配線図

### ESP32 ↔ GR-SAKURA (UART)
| ESP32 | 方向 | GR-SAKURA | 用途 |
|-------|------|-----------|------|
| GPIO17 (TX) | → | P52 (RXD2) | ESP32→RX データ送信 |
| GPIO16 (RX) | ← | P50 (TXD2) | RX→ESP32 データ受信 |
| GND | ─ | GND | 共通グラウンド |

### ESP32 ↔ BME280 (I2C)
| ESP32 | BME280 | 用途 |
|-------|--------|------|
| GPIO21 | SDA | I2Cデータ |
| GPIO22 | SCL | I2Cクロック |
| 3.3V | VIN | 電源 |
| GND | GND | グラウンド |

### ESP32 ↔ IRLZ44N (ヒーター)
| ESP32 | IRLZ44N | 用途 |
|-------|---------|------|
| GPIO26 | Gate | PWM制御信号 |

## プロジェクトディレクトリ

```
SAMDEMO/
├── CLAUDE.md                    ← Claude Code開発指示書
├── docs/
│   ├── project-overview.md      ← このファイル
│   ├── esp32-guide.md           ← ESP32開発ガイド
│   ├── gr-sakura-guide.md       ← GR-SAKURA開発ガイド
│   └── troubleshooting.md       ← 問題と解決策まとめ
├── iot-demo-rx/                 ← GR-SAKURA ファームウェア（稼働版）
│   ├── Makefile
│   ├── src/                     ← main.c, sci0_uart, cmt_timer, pid_ctrl等
│   ├── generate/                ← e2studio生成コード
│   └── linker/rx63n.ld
├── iot-demo-esp32-test/         ← ESP32 ファームウェア（稼働版）
│   ├── platformio.ini
│   ├── src/                     ← main.cpp, bme_reader, heater_pwm等
│   └── data/                    ← SPIFFS (index.html, chart.min.js)
├── iot-demo-rx-test/            ← GR-SAKURA FreeRTOS版（開発中）
├── iot-demo-esp32/              ← ESP32 旧版
├── dashboard/                   ← Python WebSocketダッシュボード
└── tools/monitor.py
```

## クイックスタート

### 1. ESP32 書き込み
```bash
cd iot-demo-esp32-test
pio run --target upload          # ファームウェア
pio run --target uploadfs        # SPIFFS (ダッシュボード)
```

### 2. GR-SAKURA 書き込み
```bash
cd iot-demo-rx
make build
# スライドスイッチ→WRITE, USBケーブル挿し直し
make flash
# スライドスイッチ→RUN, USBケーブル挿し直し
```

### 3. 動作確認
1. ESP32の電源投入
2. WiFi「SAMDEMO-ESP32」に接続（パスワードなし）
3. ブラウザで `http://192.168.4.1` を開く
4. センサーデータ・グラフ・PWM値が表示される
5. 目標温度を設定して「適用」→ ヒーター加熱開始
