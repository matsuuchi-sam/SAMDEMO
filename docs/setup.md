# SAMDEMO 開発環境セットアップ手順

IoT温度制御システム（GR-SAKURA RX63N + ESP32 + BME280）の開発に必要なツールのインストール手順です。

---

## 必要ツール一覧

| ツール | 用途 | バージョン目安 |
|--------|------|--------------|
| GCC for Renesas RX（rx-elf-gcc） | RX63N クロスコンパイラ | 8.3.0 以上 |
| Renesas Flash Programmer CLI（rfp-cli） | CLI 書き込みツール | v3.08.00 以上 |
| PlatformIO Core（CLI） | ESP32 ビルド/書き込み | 最新版 |
| Python 3 | モニタ/ダッシュボード実行環境 | 3.9 以上 |
| pyserial | シリアル通信ライブラリ | 最新版 |
| websockets | WebSocket サーバー | 最新版 |
| make (GNU Make) | ビルド自動化 | 4.x 以上 |

---

## 1. GCC for Renesas RX のインストール

### 方法A：Renesas GNU Toolchain for RX（推奨）

1. Renesas 公式サイトにアクセス:
   https://www.renesas.com/software-tool/gnu-arm-embedded-toolchain
   ※「GNU Toolchain for Renesas RX」で検索

2. Windows 向けインストーラ（.exe）をダウンロード

3. インストーラを実行し、インストール先（例: `C:\Program Files\GNURenesas\GCCR8300`）を確認

4. 環境変数 PATH に追加:
   ```
   C:\Program Files\GNURenesas\GCCR8300\rx-elf\rx-elf\bin
   ```

5. 動作確認:
   ```bash
   rx-elf-gcc --version
   # 出力例: rx-elf-gcc (GCC_Build_20230505) 8.3.0.202305-1306
   ```

### 方法B：e2studio に同梱のツールチェーンを使う

e2studio（Renesas統合開発環境）をインストールすると、同梱の GCC ツールチェーンが利用できます。
- インストール先の例: `C:\Renesas\e2_studio\eclipse\toolchains\gnu_rx\8_3_0_202305\bin`
- この場合も上記と同様に PATH に追加してください。

---

## 2. Renesas Flash Programmer CLI（rfp-cli）のインストール

1. Renesas 公式サイトにアクセス:
   https://www.renesas.com/software-tool/renesas-flash-programmer-programming-gui

2. 「Renesas Flash Programmer V3.xx」の Windows 版をダウンロード（v3.08.00 以上）

3. インストーラを実行

4. インストール後、rfp-cli.exe の場所を確認（例: `C:\Program Files (x86)\Renesas Electronics\Renesas Flash Programmer V3.08\rfp-cli.exe`）

5. 環境変数 PATH に追加するか、Makefile 内でフルパスを指定

6. 動作確認:
   ```bash
   rfp-cli --version
   # 出力例: Renesas Flash Programmer CLI V3.08.00
   ```

---

## 3. PlatformIO Core（CLI）のインストール

ESP32 のビルドと書き込みに使用します。

### 前提条件
- Python 3.9 以上がインストール済みであること（手順4を先に実施）

### インストール手順

```bash
# pip でインストール
pip install platformio

# 動作確認
pio --version
# 出力例: PlatformIO Core, version 6.x.x
```

### 初回のプラットフォームインストール

初回ビルド時に自動でダウンロードされますが、事前に行う場合:
```bash
pio platform install espressif32
```

---

## 4. Python 3 + 必要ライブラリのインストール

### Python 3 のインストール

1. 公式サイトからダウンロード: https://www.python.org/downloads/
2. インストール時に「Add Python to PATH」にチェックを入れる
3. 動作確認:
   ```bash
   python --version
   # 出力例: Python 3.11.x
   ```

### 必要ライブラリのインストール

```bash
pip install pyserial websockets
```

動作確認:
```bash
python -c "import serial; print(serial.__version__)"
python -c "import websockets; print(websockets.__version__)"
```

---

## 5. GNU Make のインストール（Windows）

### 方法A：winget を使う（Windows 11 推奨）
```bash
winget install GnuWin32.Make
```

### 方法B：Chocolatey を使う
```bash
choco install make
```

### 方法C：Git for Windows に同梱の make を使う
Git for Windows をインストールすると `C:\Program Files\Git\usr\bin\make.exe` が利用できます。

動作確認:
```bash
make --version
# 出力例: GNU Make 4.x.x
```

---

## 6. USB シリアルドライバのインストール

### GR-SAKURA（RX63N）
- GR-SAKURA は USB CDC（仮想COMポート）を使用します
- 通常は Windows に自動認識されますが、認識されない場合は以下から:
  - Renesas USB ドライバ: https://www.renesas.com/software-tool/usb-driver-renesas-usb-module-firmware

### ESP32
- ESP32 開発ボードに搭載された USB-UART チップのドライバ:
  - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - CH340: http://www.wch-ic.com/downloads/CH341SER_EXE.html

---

## 7. 環境確認チェックリスト

全ツールのインストール後、以下のコマンドで動作確認してください:

```bash
# RX63N クロスコンパイラ
rx-elf-gcc --version

# Flash Programmer CLI
rfp-cli --version

# PlatformIO
pio --version

# Python と必要ライブラリ
python --version
python -c "import serial; import websockets; print('OK')"

# Make
make --version
```

全て正常に出力されれば環境構築完了です。

---

## 8. 動作確認（ビルドテスト）

```bash
# プロジェクトルートに移動
cd C:\Users\kouse\Demo\SAMDEMO

# RX63N ファームウェアのビルド
cd iot-demo-rx
make build

# ESP32 ファームウェアのビルド
cd ../iot-demo-esp32
pio run
```

---

## トラブルシューティング

### rx-elf-gcc: command not found
→ PATH の設定を確認してください。ターミナルを再起動後に再試行。

### rfp-cli: access denied
→ ターミナルを管理者権限で起動してください。

### make: missing separator
→ Makefile のインデントがタブ文字になっているか確認（スペース不可）。

### COM ポートが認識されない
→ デバイスマネージャーでポート番号を確認し、`make flash PORT=COMx` の COMx を書き換え。
