#!/usr/bin/env python3
"""
monitor.py - SAMDEMO CLI シリアルモニタ

GR-SAKURA (RX63N) または ESP32 のシリアル出力をターミナルに表示します。
Claude Code がシリアル出力を読んでデバッグできるよう、タイムスタンプ付きで出力します。

使い方:
    python monitor.py COM3          # デフォルト 115200bps
    python monitor.py COM3 115200   # ボーレートを明示
    python monitor.py COM3 9600     # 別のボーレート

終了: Ctrl+C

必要ライブラリ:
    pip install pyserial
"""

import sys
import time
import datetime
import signal

try:
    import serial
except ImportError:
    print("エラー: pyserial がインストールされていません。")
    print("インストール方法: pip install pyserial")
    sys.exit(1)


def get_timestamp() -> str:
    """現在時刻を HH:MM:SS.mmm 形式で返す"""
    now = datetime.datetime.now()
    return now.strftime("%H:%M:%S.") + f"{now.microsecond // 1000:03d}"


def list_serial_ports():
    """利用可能なシリアルポートを一覧表示する"""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        if ports:
            print("利用可能なシリアルポート:")
            for port in ports:
                print(f"  {port.device}: {port.description}")
        else:
            print("シリアルポートが見つかりません。")
    except Exception as e:
        print(f"ポート一覧取得エラー: {e}")


def monitor(port: str, baud: int = 115200):
    """
    指定ポートのシリアル出力をモニタする。

    Args:
        port: COM ポート名 (例: "COM3", "/dev/ttyUSB0")
        baud: ボーレート (デフォルト: 115200)
    """
    print(f"=== SAMDEMO シリアルモニタ ===")
    print(f"ポート: {port}, ボーレート: {baud} bps")
    print(f"開始時刻: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"終了: Ctrl+C")
    print("=" * 50)

    ser = None
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1.0,          # 読み取りタイムアウト (秒)
            xonxoff=False,
            rtscts=False,
            dsrdtr=False
        )

        print(f"接続成功: {ser.name}")
        print()

        line_count = 0

        while True:
            try:
                # 1行読み取り（改行まで）
                raw_line = ser.readline()

                if not raw_line:
                    # タイムアウト（データなし）
                    continue

                # バイト列をデコード（不正なバイトは置換）
                line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")

                if line:
                    timestamp = get_timestamp()
                    line_count += 1
                    print(f"[{timestamp}] {line}")

            except serial.SerialException as e:
                print(f"\nシリアルエラー: {e}")
                print("デバイスが切断された可能性があります。")
                break

    except serial.SerialException as e:
        print(f"ポートを開けません: {e}")
        print()
        list_serial_ports()
        sys.exit(1)

    except KeyboardInterrupt:
        pass

    finally:
        if ser and ser.is_open:
            ser.close()
        print()
        print(f"=== モニタ終了 ===")


def main():
    """エントリポイント"""
    args = sys.argv[1:]

    # 引数チェック
    if len(args) == 0:
        print("使い方: python monitor.py <PORT> [BAUD]")
        print("例:     python monitor.py COM3")
        print("        python monitor.py COM3 115200")
        print()
        list_serial_ports()
        sys.exit(1)

    port = args[0]

    # 特殊コマンド: ports で利用可能ポートを表示
    if port.lower() == "ports":
        list_serial_ports()
        sys.exit(0)

    baud = 115200  # デフォルト
    if len(args) >= 2:
        try:
            baud = int(args[1])
        except ValueError:
            print(f"エラー: ボーレートは整数で指定してください。'{args[1]}' は無効です。")
            sys.exit(1)

    monitor(port, baud)


if __name__ == "__main__":
    main()
