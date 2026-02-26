# SAMDEMO 配線メモ

## ESP32 (FREENOVE ESP32-WROOM32E) ピン配置

```
        【USB-C端子側（下）】
左:  5V
     5V
     3.3V  ← VCC用
     3.3V
     GND   ← GND用（13の下）
     13
     GND
     12
     14
     27
     26    ← ヒーター制御 GPIO26
     25
     33
     32
     35
     34
     VN
     VP
     EN
     3.3V
        【上側】

右:  GND
     23
     22    ← SCL（BME280）
     TX
     RX
     21    ← SDA（BME280）
     GND
     19
     18
     ...
        【USB-C端子側（下）】
```

---

## BME280 接続（完了 ✅）

| 線の色 | ESP32 ピン | BME280 ピン |
|--------|-----------|------------|
| オレンジ | 左側 `3.3V`（USB端子すぐ上） | `VCC` |
| 茶色   | 左側 `GND`（13の下）         | `GND` |
| 黄色   | 右側 `22`（上から3番目）      | `SCL` |
| 緑     | 右側 `21`（上から6番目）      | `SDA` |

BME280 ピン順（左から）: **VCC · GND · SCL · SDA**

I2C アドレス: `0x76`（検出失敗時は `0x77` も自動試行）

---

## ヒーター回路（MB102 + IRLZ44N + セメント抵抗）（完了 ✅）

### 部品

| 部品 | 仕様 |
|------|------|
| MB102 電源モジュール | USB給電 → 5V出力（ジャンパーを 5V 側に設定済み） |
| IRLZ44N | ロジックレベル N-ch MOSFET（TO-220）|
| セメント抵抗 | 5W 10Ω（発熱体・5V時 2.5W）|
| 1kΩ 抵抗 | Gate 直列抵抗 |

### IRLZ44N ピン配置

```
刻印面を手前（j側）に向けてブレッドボード上段（a行）に刺す
  右ブレッドボード a行 列21 = Gate
  右ブレッドボード a行 列20 = Drain
  右ブレッドボード a行 列19 = Source（MB102 寄り）
```

### 実際の接続（完成済み）

| 線の色 | 接続元 | 接続先 |
|--------|--------|--------|
| 赤 | MB102 + レール（5V） | 左ブレッドボード セメント抵抗 片端 |
| 白 | 左ブレッドボード セメント抵抗 反対端 | 右ブレッドボード 列20（Drain） |
| 黒 | 右ブレッドボード 列19（Source） | 右ブレッドボード - レール（GND） |
| 茶色 | 右ブレッドボード - レール（GND） | ESP32 GND |
| 白 | ESP32 GPIO26 | 右ブレッドボード c21（Gate と同列） |

### セメント抵抗の配置

- **左ブレッドボード**の BME280 の隣（1〜2cm 以内）に設置
- 加熱した空気が BME280 に伝わる配置

### 動作

- ESP32 GPIO26 → HIGH（3.3V）でヒーター ON
- ESP32 GPIO26 → LOW（0V）でヒーター OFF
- 発熱量: 5V ÷ 10Ω = 0.5A → 2.5W
- MB102 電源: USB アダプター（5V 1A 以上）推奨（PC USB ポート不可）

---

## 接続確認コマンド

```bash
# USB シリアルモニタ（BME280 動作確認）
python -m platformio device monitor --port COM4 --baud 115200

# ダッシュボード起動（Bluetooth 経由）
cd C:\Users\kouse\Demo\SAMDEMO\dashboard
python server.py --port COM5

# ダッシュボード起動（USB 経由）
python server.py --port COM4
```

## ブラウザアクセス

```
http://localhost:8080
```
