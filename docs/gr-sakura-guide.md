# GR-SAKURA (RX63N) 開発ガイド — SAMDEMO

## 1. 概要

GR-SAKURA（RX63N マイコン）はPID温度制御演算を担当:
- ESP32からセンサーデータ（JSON）をUART受信
- PID制御アルゴリズムでPWM出力値を算出
- 算出したPWM値をJSON形式でESP32に返送
- ブラウザからのコマンド（目標温度変更、停止/再開）を処理

## 2. ハードウェア構成

### RX63N スペック

| 項目 | 値 |
|------|-----|
| CPU | RX63N（RXv1コア） |
| クロック | HOCO 50MHz（HardwareSetupで設定） |
| コードフラッシュ | 1MB: 0xFFF00000 〜 0xFFFFFFFF |
| RAM | 128KB: 0x00000000 〜 0x0001FFFF |
| 固定ベクタテーブル | 0xFFFFFF80 〜 0xFFFFFFFF |
| リセットベクタ | 0xFFFFFFFC |

### ピン配置表

| ピン | 接続先 | 用途 | 備考 |
|------|--------|------|------|
| P50 (TXD2) | ESP32 GPIO16 (RX) | UART送信 | SCI2チャネル |
| P52 (RXD2) | ESP32 GPIO17 (TX) | UART受信 | SCI2チャネル |
| PORTA-PORTE 全ポート | LED | 動作確認用点滅 | Active High |
| GND | ESP32 GND | 共通グラウンド | 必須 |
| USB | PC | 書き込み (USB Direct) | COMポートではない |

### UART接続の注意
- **P20/P21（SCI0）では動作しなかった**。P50/P52（SCI2）を使用すること
- ファイル名 `sci0_uart.c` は歴史的経緯。中身は **SCI2** レジスタを使用

## 3. 開発環境セットアップ

### 必要ツール

| ツール | パス | 用途 |
|--------|------|------|
| GCC for Renesas RX 8.3.0 | `C:/Users/kouse/AppData/Roaming/GCC for Renesas RX 8.3.0.202411-GNURX-ELF/rx-elf/rx-elf/bin` | クロスコンパイラ |
| Renesas Flash Programmer V3.22 | `C:/Program Files (x86)/Renesas Electronics/Programming Tools/Renesas Flash Programmer V3.22` | フラッシュ書き込み |
| GNU Make | `/c/Users/kouse/scoop/shims/make.exe` | ビルドツール |

### インストール確認
```bash
# コンパイラ
"C:/Users/kouse/AppData/Roaming/GCC for Renesas RX 8.3.0.202411-GNURX-ELF/rx-elf/rx-elf/bin/rx-elf-gcc" --version

# rfp-cli
"C:/Program Files (x86)/Renesas Electronics/Programming Tools/Renesas Flash Programmer V3.22/rfp-cli" -help

# GR-SAKURA接続確認
"C:/Program Files (x86)/Renesas Electronics/Programming Tools/Renesas Flash Programmer V3.22/rfp-cli" -d RX63x -t usb -sig
```

## 4. ビルド手順

```bash
cd iot-demo-rx

# クリーンビルド
make clean && make build

# ビルドのみ
make build

# サイズ確認
make size

# 逆アセンブル（デバッグ用）
make disasm
```

### ビルド結果例
```
   text    data     bss     dec     hex filename
   5265     132     381    5778    1692 firmware.elf
```

## 5. 書き込み手順（重要）

### GR-SAKURAの接続形態
- **COMポートではなく USB Direct 接続**
- デバイスマネージャーのCOMポート一覧には表示されない場合がある
- 接続確認: `rfp-cli -d RX63x -t usb -sig`

### 書き込み手順（必ずこの順序で）

#### Step 1: WRITEモードに切り替え
1. GR-SAKURAのスライドスイッチを **WRITE** 側に切り替え
2. **USBケーブルを挿し直す**（SW1リセットだけでは不十分な場合が多い）

#### Step 2: 書き込み実行
```bash
make flash
# または直接:
rfp-cli -d RX63x -t usb -noverify-id -nocheck-range -e -p firmware.mot -v
```

**必須フラグ**:
- `-noverify-id`: ID認証スキップ（必須）
- `-nocheck-range`: アドレス範囲チェックスキップ
- `-e`: 書き込み前にフラッシュ消去
- `-v`: 書き込み後にベリファイ

#### Step 3: RUNモードに切り替え
1. スライドスイッチを **RUN** 側に切り替え
2. **USBケーブルを挿し直す**

#### BootCode:80 エラーの場合
→ デバイスがブートモードに入っていない
→ **USBケーブルを挿し直して**再実行

## 6. ソフトウェア構成

### ファイル構成
```
iot-demo-rx/
├── Makefile
├── src/
│   ├── main.c              メインループ（PID制御フロー）
│   ├── sci0_uart.c/h       SCI2 UART (115200bps, P50/P52) ※ファイル名注意
│   ├── cmt_timer.c/h       CMT0タイマー (1ms周期, millis()関数)
│   ├── cmd_parser.c/h      JSON受信パーサー
│   ├── json_builder.c/h    JSON送信ビルダー
│   └── pid_ctrl.c/h        PID制御（x100固定小数点）
├── generate/
│   ├── start.S              スタートアップコード
│   ├── hwinit.c             HOCO 50MHzクロック設定
│   ├── vects.c              再配置可能ベクタテーブル
│   ├── inthandler.c         割り込みハンドラ
│   ├── iodefine.h           I/Oレジスタ定義
│   └── interrupt_handlers.h 割り込みハンドラ宣言
└── linker/
    └── rx63n.ld             リンカスクリプト
```

### スタートアップシーケンス (start.S → hwinit.c → main.c)

```
1. start.S:
   - ISP/USP設定 (ISP=0x1FC00, USP=0x20000)
   - .dataセクションをROMからRAMにコピー
   - .bssセクションをゼロクリア
   - HardwareSetup() 呼び出し

2. hwinit.c (HardwareSetup):
   - レジスタ書き込み保護解除 (PRCR=0xA503)
   - HOCO 50MHz起動
   - クロック分周設定 (ICLK=50MHz, PCLKB=25MHz)
   - クロックソースをHOCOに切替

3. main.c (main):
   - LED起動確認（3回点滅）
   - SCI2 UART初期化 (115200bps)
   - CMT0タイマー初期化 (1ms周期)
   - PID初期化 (Kp=3.00, Ki=0.80, Kd=0.20, 目標=28.0℃)
   - 割り込み有効化
   - メインループ開始
```

### main.c — メインループ

```
while(1):
  c = sci0_trygetc()              ← 1文字ずつUART受信
  cmd_feed(c)                     ← パーサーに蓄積
  msg = cmd_poll()                ← 1行完成したら解析

  switch(msg.type):
    MSG_SENSOR:                   ← センサーデータ受信
      pid_compute(temp)           ← PID演算
      pwm = pid_out * 255 / 10000 ← PWM値算出 (0-255)
      json_build_ctrl(...)        ← ctrl JSON生成
      sci0_puts(json)             ← ESP32に返送

    MSG_CMD_SET_TARGET:           ← 目標温度変更
      pid_set_target(sp)
      json_build_ctrl(...)        ← 即座にctrl JSON返送

    MSG_CMD_STOP:                 ← 緊急停止
      running = 0
      json_build_ctrl(pwm=0)     ← PWM=0を即送信

    MSG_CMD_START:                ← 運転再開
      running = 1
      pid_reset()
```

### sci0_uart.c — SCI2 UART ドライバ

**注意: ファイル名は sci0 だが中身は SCI2 を使用**

- MSTPB29 = 0 でSCI2モジュール起動
- P50 → TXD2 (MPC PSEL=0x0A)
- P52 → RXD2 (MPC PSEL=0x0A)
- 115200bps: BRR=26, ABCS=1 (倍速モード)
- PCLKB = 50MHz 基準
- 受信バッファ: 1バイト（ハードウェア制限）

### cmt_timer.c — CMT0 タイマー

- PCLK/8 = 6.25MHz, CMCOR=6249 → 1ms周期割り込み
- `millis()`: 起動からのミリ秒カウンタ
- `cmt0_isr()`: 割り込みハンドラ（inthandler.cから呼ばれる）
- 割り込み優先度: 5

### cmd_parser.c — JSON受信パーサー

- 1文字ずつ `cmd_feed()` で蓄積
- 改行で1行完成 → `cmd_poll()` で解析
- `find_value(json, key)` でJSONキーの値を取得
  - **コロンの有無でキーと値を区別**（`"type":"cmd"` の "cmd" 値にマッチしないよう）
- `parse_fixed100()`: 浮動小数点文字列を x100 整数に変換（"25.3" → 2530）
- ラインバッファ: 256バイト

### json_builder.c — JSON送信ビルダー

- `json_build_ctrl(vtemp, pwm, sp)` → `{"type":"ctrl","vtemp":25.30,"pwm":128,"sp":28.00}\n`
- `json_build_status(msg)` → `{"type":"status","msg":"boot ok"}\n`
- sprintf不使用、固定小数点 x100 → "xx.xx" 形式の手動変換

### pid_ctrl.c — PID制御

- **固定小数点演算**: すべて x100（25.30℃ → 2530）
- **ゲイン**: Kp, Ki, Kd も x100 で保持
- **出力範囲**: 0 〜 10000 (0% 〜 100%)
- **PWM変換**: `pwm = pid_output * 255 / 10000`
- **アンチワインドアップ**:
  - 積分上限: 50000
  - 積分下限: 0（加熱のみシステム）
  - 目標超過時（error < 0）に積分値を即クリア

#### デフォルトPIDパラメータ

| パラメータ | 値 | x100表現 |
|-----------|-----|---------|
| Kp | 3.00 | 300 |
| Ki | 0.80 | 80 |
| Kd | 0.20 | 20 |
| 目標温度 | 28.00℃ | 2800 |

## 7. リンカスクリプト (rx63n.ld)

### メモリマップ

```
ROM (1MB): 0xFFF00000 - 0xFFFFFFFF
├── .text        0xFFF00000   プログラムコード
├── .rvectors    (ALIGN 4)    再配置可能ベクタテーブル
├── .rodata                   読み取り専用データ
├── _mdata                    .dataの初期値（ROM上）
└── .fvectors    0xFFFFFF80   固定ベクタテーブル

RAM (128KB): 0x00000000 - 0x0001FFFF
├── .data        0x00000100   初期化済みデータ
├── .bss                      未初期化データ
├── .istack      0x0001FC00   割り込みスタック (ISP, 下方向)
└── .ustack      0x00020000   ユーザースタック (USP, 下方向)
```

### RX63Nアドレスの注意点
- rfp-cliが認識するコードフラッシュは **0xFFF00000** 〜
- 0xFFE00000 からだと `data outside the device's memory area` 警告で書き込まれない
- リンカスクリプトのROM開始は必ず **0xFFF00000**

## 8. トラブルシューティング

| 症状 | 原因 | 解決策 |
|------|------|--------|
| BootCode:80 エラー | ブートモード未設定 | USBケーブル挿し直し |
| 新規ビルドが動作しない | Issue #8 未解決 | フルアプリコードごとビルドする |
| コマンドが効かない | JSONパーサーのキー/値混同 | find_valueでコロン有無チェック |
| 目標超過後PWM下がらない | PID積分ワインドアップ | 目標超過時に積分クリア |
| UART受信漏れ | SCI2の1バイトバッファ | 送信中にデータ損失の可能性 |
| LED点灯パターン不明 | ポート不特定 | PORTA-PORTE全ポート同時制御 |

## 9. Makefileコマンド一覧

```bash
make build     # ビルド（firmware.mot生成）
make flash     # ビルド + 書き込み
make clean     # 中間ファイル削除
make size      # セクションサイズ詳細表示
make disasm    # 逆アセンブル（firmware.asm生成）
```
