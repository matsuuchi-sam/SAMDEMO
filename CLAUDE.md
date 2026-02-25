# SAMDEMO — Claude Code 開発指示書

このファイルは Claude Code が自律的に開発を進めるための指示書です。
作業前に必ずこのファイルを読み込んでください。

---

## プロジェクト概要

**SAMDEMO** は GR-SAKURA（RX63N）+ ESP32 + BME280 センサーを用いた IoT 温度制御システムです。

### システム構成

```
[BME280]──I2C──[GR-SAKURA RX63N]──UART──[ESP32]──WiFi──[PC ダッシュボード]
```

- **GR-SAKURA（RX63N）**: BME280 から温度/湿度/気圧を取得し、JSON で ESP32 へ送信
- **ESP32**: GR-SAKURA から受け取ったデータを WiFi 経由で PC へ転送
- **PC ダッシュボード**: WebSocket でリアルタイムにグラフ表示

### UART プロトコル仕様

- 通信速度: **115200 bps**, 8N1
- データフォーマット: **JSON**, 改行区切り（`\n`）
- 送信間隔: 1 秒

```json
{"temp": 25.3, "humidity": 60.1, "pressure": 1013.2, "ts": 1234567890}
```

---

## ディレクトリ構成と各役割

```
SAMDEMO/
├── CLAUDE.md                  ← この指示書（最初に読む）
├── .claude/settings.json      ← Claude Code 設定
├── iot-demo-rx/               ← GR-SAKURA (RX63N) ファームウェア
│   ├── Makefile               ← ビルド/書き込み/モニタの核心
│   ├── src/main.c             ← メインアプリケーション
│   ├── linker/rx63n.ld        ← RX63N リンカスクリプト
│   └── startup/startup.c      ← 起動コード（ベクタテーブル含む）
├── iot-demo-esp32/            ← ESP32 ファームウェア（PlatformIO）
│   ├── platformio.ini         ← PlatformIO 設定
│   └── src/main.cpp           ← ESP32 メインコード
├── dashboard/                 ← PC モニタリングダッシュボード
│   ├── server.py              ← Python WebSocket サーバー
│   └── index.html             ← Chart.js リアルタイムグラフ
├── tools/
│   └── monitor.py             ← CLI シリアルモニタ
└── docs/
    └── setup.md               ← ツールインストール手順
```

---

## 開発フロー（標準手順）

### RX63N ファームウェア開発

```bash
# 1. ソースを編集
# iot-demo-rx/src/main.c を修正する

# 2. ビルド
cd iot-demo-rx
make build

# 3. エラーがなければ書き込み（GR-SAKURA を USB 接続した状態で）
make flash PORT=COM3   # COM番号は環境に合わせて変更

# 4. シリアル出力の確認
make monitor PORT=COM3
```

### ESP32 ファームウェア開発

```bash
cd iot-demo-esp32

# ビルド
pio run

# 書き込み
pio run --target upload --upload-port COM4

# シリアルモニタ
pio device monitor --port COM4 --baud 115200
```

### ダッシュボード起動

```bash
# WebSocket サーバーを起動
python dashboard/server.py

# ブラウザで開く
# http://localhost:8765  (または index.html を直接開く)
```

---

## Claude Code がやること / やらないこと

### やること（自律実行してよい操作）

- ソースファイルの編集・作成（src/, startup/, linker/ 以下）
- `make build` の実行とエラー修正のループ
- `pio run` の実行とエラー修正のループ
- Python スクリプトの編集・実行（dashboard/, tools/ 以下）
- `make clean` の実行
- Makefile の編集

### やらないこと（禁止事項）

- **e2studio GUI を起動しない** — このプロジェクトは完全 CLI ベース
- **GUI ツールで書き込みをしない** — 必ず rfp-cli（make flash）を使う
- **ユーザーに許可なく make flash を実行しない** — 書き込み前に確認を取ること
- **linker スクリプトの RX63N アドレスマップを根拠なく変更しない**
- **startup.c のベクタテーブルを根拠なく削除しない**

---

## ビルドエラー対応ガイドライン

Claude Code はビルドエラーが発生した場合、以下の手順で自律的に対応してください：

1. **エラーメッセージを全文読む** — ファイル名・行番号・エラー種別を把握する
2. **該当ファイルを開いて確認する** — コンテキストを理解する
3. **修正する** — 根本原因を修正する（ワークアラウンドで隠さない）
4. **再ビルドする** — `make build` or `pio run` を再実行
5. **1〜4 を繰り返す** — 全エラーが解消するまで繰り返す

### よくあるエラーと対処

| エラー | 原因 | 対処 |
|--------|------|------|
| `rx-elf-gcc: not found` | PATH 未設定 | docs/setup.md を確認してユーザーに案内 |
| `undefined reference to _start` | スタートアップ未リンク | startup.c が Makefile に含まれているか確認 |
| `relocation truncated` | アドレス範囲オーバー | リンカスクリプトのセクション配置を確認 |
| `implicit declaration of function` | ヘッダインクルード漏れ | 該当関数のヘッダを追加 |

---

## RX63N ハードウェア仕様（参考）

| 項目 | 値 |
|------|-----|
| CPU | RX63N（RXv1 コア） |
| クロック | 最大 100 MHz（外部クリスタル依存） |
| フラッシュ | 2 MB（0x00000000 〜 0x001FFFFF） |
| RAM | 128 KB（0x00000000 が RAM ではない点に注意） |
| RAM アドレス | 0x00010000 〜 0x0002FFFF（典型的な配置） |
| UART0 TXD | P20（基板による） |
| UART0 RXD | P21（基板による） |
| LED | PORT0 bit0（GR-SAKURA 基板上の緑 LED） |

---

## ESP32 仕様（参考）

| 項目 | 値 |
|------|-----|
| ボード | esp32dev |
| フレームワーク | Arduino |
| Flash | 4 MB |
| UART0 | USB シリアル（デバッグ用） |
| UART1 | GPIO16(RX)/GPIO17(TX)（GR-SAKURA との通信用） |

---

## 開発フェーズ（ロードマップ）

| フェーズ | 内容 | 状態 |
|---------|------|------|
| Phase 0 | プロジェクトテンプレート構築 | **完了** |
| Phase 1 | RX63N LED Lチカ + UART printf | 未着手 |
| Phase 2 | BME280 I2C 読み取り | 未着手 |
| Phase 3 | UART JSON 送信（RX63N → ESP32） | 未着手 |
| Phase 4 | ESP32 WiFi + WebSocket 転送 | 未着手 |
| Phase 5 | ダッシュボード接続 | 未着手 |
| Phase 6 | FreeRTOS 導入（RX63N） | 未着手 |

現在のタスクは **Phase 1**（RX63N の動作確認）から開始してください。

---

## 問い合わせ先・参考資料

- Renesas RX63N ハードウェアマニュアル: Renesas 公式サイトで「RX63N Group User's Manual: Hardware」を検索
- GR-SAKURA 回路図: https://www.renesas.com/products/gadget-renesas/boards/gr-sakura
- ツール導入手順: `docs/setup.md` を参照
