# ESP32 開発ガイド — SAMDEMO

## 1. 概要

ESP32はシステムの中核として以下を担当:
- BME280センサー読取（I2C）
- GR-SAKURAへセンサーデータ送信（UART）
- GR-SAKURAからPWM値受信（UART）
- ヒーターMOSFET PWM制御（GPIO26）
- WiFi APモードでWebダッシュボード提供

## 2. ハードウェア構成

### ピン配置表

| GPIO | 接続先 | 用途 | プロトコル |
|------|--------|------|-----------|
| GPIO21 | BME280 SDA | I2Cデータ | I2C |
| GPIO22 | BME280 SCL | I2Cクロック | I2C |
| GPIO16 | GR-SAKURA P50 (TXD2) | UART受信 (RX) | UART 115200bps |
| GPIO17 | GR-SAKURA P52 (RXD2) | UART送信 (TX) | UART 115200bps |
| GPIO26 | IRLZ44N Gate | ヒーターPWM出力 | PWM 25kHz 8bit |
| USB | PC | デバッグシリアル (Serial) | 115200bps |

### 接続時の注意
- **UART クロス接続**: ESP32 TX(GPIO17) → GR-SAKURA RX(P52), ESP32 RX(GPIO16) ← GR-SAKURA TX(P50)
- **GND共通**: ESP32とGR-SAKURAのGNDを必ず接続
- **BME280電源**: 3.3V（ESP32の3V3ピンから供給可能）
- **IRLZ44N**: ロジックレベルMOSFET、3.3Vゲート駆動可能

## 3. 開発環境セットアップ

### 必要ツール
- **PlatformIO CLI** または **VSCode + PlatformIO拡張**
- PlatformIOフルパス: `/c/Users/kouse/.platformio/penv/Scripts/pio.exe`

### platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_port = COM7          ; 環境に合わせて変更
monitor_port = COM7
board_build.filesystem = spiffs

lib_deps =
    adafruit/Adafruit BME280 Library@^2.2.2
    bblanchon/ArduinoJson@^7.0.0
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
```

### COMポート確認
```bash
# Windows デバイスマネージャーで確認
# または
pio device list
```

## 4. ビルドとフラッシュ手順

### ファームウェアビルド・書き込み
```bash
cd iot-demo-esp32-test

# ビルドのみ
pio run

# ビルド + 書き込み
pio run --target upload

# シリアルモニタ
pio device monitor
```

### SPIFFS（ダッシュボード）書き込み
```bash
# data/ フォルダの内容をSPIFFSに書き込み
pio run --target uploadfs
```

**重要**: `data/` フォルダに以下が必要:
- `index.html` — ダッシュボードHTML
- `chart.min.js` — Chart.jsライブラリ（ローカル配置必須）

### SPIFFS書き込み時の注意
- シリアルモニタなど他のプロセスがCOMポートを占有しているとPermissionError
- モニタを閉じるか、USBケーブルを挿し直す

## 5. ソフトウェア構成

### ファイル構成
```
iot-demo-esp32-test/
├── platformio.ini
├── src/
│   ├── main.cpp          メインループ
│   ├── bme_reader.h/cpp  BME280 I2C読取
│   ├── heater_pwm.h/cpp  ヒーターPWM制御
│   ├── uart_comm.h/cpp   GR-SAKURAとのUART通信
│   └── web_server.h/cpp  WiFi AP + WebSocket + SPIFFS
└── data/
    ├── index.html        ダッシュボードHTML
    └── chart.min.js      Chart.jsライブラリ
```

### main.cpp — メインループ

```
setup():
  Serial.begin(115200)        ← デバッグ用
  bme.begin()                 ← BME280初期化
  heater.begin()              ← PWM初期化 (GPIO26, 25kHz, 8bit)
  uart.begin()                ← Serial1初期化 (GPIO16/17, 115200bps)
  web.begin()                 ← WiFi AP + HTTP + WebSocket

loop() [1秒周期]:
  1. BME280読取 → BmeData {temp, humi, pres}
  2. uart.sendSensor(d)       → GR-SAKURAへJSON送信
  3. web.broadcast(json)      → ブラウザへWebSocket配信
  4. uart.receive(buf)        → GR-SAKURAからctrl JSON受信
     → heater.set(pwm)       → PWM値をヒーターに適用
     → web.broadcast(buf)    → ブラウザへ転送
  5. web.hasCommand(buf)      → ブラウザからのコマンド
     → uart.sendRaw(buf)     → GR-SAKURAへ転送
```

### bme_reader — BME280センサー読取
- Adafruit BME280ライブラリ使用
- I2Cアドレス: 0x76（デフォルト）または 0x77
- 読取データ: 温度(℃), 湿度(%), 気圧(hPa)

### uart_comm — UART通信
- Serial1使用（GPIO16=RX, GPIO17=TX）
- 115200bps, 8N1
- JSON改行区切りプロトコル
- 送信: `sendSensor(BmeData)` → `{"type":"sensor",...}\n`
- 受信: `receive(buf)` → 改行まで蓄積、1行返却

### heater_pwm — ヒーターPWM制御
- LEDC PWM使用
- GPIO26, チャネル0, 25kHz, 8bit分解能 (0-255)
- `set(duty)`: PWMデューティ設定
- `get()`: 現在のデューティ値取得

### web_server — Webダッシュボード
- **WiFi AP**: SSID=`SAMDEMO-ESP32`, パスワードなし
- **IPアドレス**: 192.168.4.1
- **HTTP**: AsyncWebServer ポート80
- **WebSocket**: `/ws` エンドポイント
- **静的ファイル**: SPIFFS配信 (`/index.html`, `/chart.min.js`)

## 6. 通信プロトコル

### ESP32 → GR-SAKURA（1秒周期）
```json
{"type":"sensor","temp":25.3,"humi":48.2,"pres":1013.25}
```

### GR-SAKURA → ESP32（センサー受信ごと）
```json
{"type":"ctrl","vtemp":25.30,"pwm":128,"sp":28.00}
```
- `pwm`: 0-255 → `heater.set()` に直接渡す
- `sp`: 現在の目標温度 → ブラウザ表示用
- `vtemp`: 現在の計測温度

### ブラウザ → ESP32 → GR-SAKURA（ユーザー操作時）
```json
{"type":"cmd","cmd":"set_target","sp":30.0}
{"type":"cmd","cmd":"stop"}
{"type":"cmd","cmd":"start"}
{"type":"cmd","cmd":"set_pid","kp":300,"ki":80,"kd":20}
```

## 7. ダッシュボード

### 画面構成
1. **センサーデータ**: 温度・湿度・気圧のリアルタイム表示
2. **PWMバー**: 現在のヒーター出力 (0-255)
3. **リアルタイムグラフ**: 温度・湿度・気圧の時系列（最大60データ点）
4. **目標温度設定**: SP入力 + 適用ボタン
5. **制御**: 緊急停止 / 運転再開ボタン

### ローカルリソースの必要性
ESP32のWiFi APモードはインターネット接続がないため:
- ❌ CDN（`cdn.jsdelivr.net` 等）からのJS/CSS読み込みは不可
- ❌ Google Fontsなど外部Webフォントは不可
- ✅ すべてSPIFFS内にローカル配置が必要
- `chart.min.js` (~208KB) は https://cdn.jsdelivr.net/npm/chart.js からダウンロードして `data/` に配置

## 8. トラブルシューティング

| 症状 | 原因 | 解決策 |
|------|------|--------|
| upload失敗 | COMポート番号違い | `pio device list` で確認、`platformio.ini` 更新 |
| SPIFFS PermissionError | ポート占有 | モニタ閉じる or USBケーブル挿し直し |
| ダッシュボード「--」 | Chart.js CDN読込失敗 | chart.min.js をSPIFFSにローカル配置 |
| BME280 NOT FOUND | I2C配線ミス | SDA/SCL確認、プルアップ抵抗確認 |
| UART通信なし | クロス接続ミス | TX↔RX確認、GND共通確認 |
