#!/usr/bin/env python3
"""
server.py - SAMDEMO WebSocket サーバー

接続フロー:
  [ESP32] ─WiFi WebSocket─> [このサーバー] ─WebSocket─> [index.html ブラウザ]
  [ESP32] ─USB Serial──────> [このサーバー] ─WebSocket─> [index.html ブラウザ]
  [ESP32] ─BT Serial───────> [このサーバー] ─WebSocket─> [index.html ブラウザ]

使い方:
  python server.py                  # デモモード（ESP32 なし、擬似データ）
  python server.py --port COM4      # USB / Bluetooth 経由のシリアル受信
  python server.py --wifi           # WiFi モード（ESP32 からの WebSocket 接続を待つ）

PC の IP アドレス確認方法:
  コマンドプロンプト → ipconfig → IPv4 アドレス（例: 192.168.1.10）
  その IP を ESP32 の wifi_config.h の WS_SERVER_IP に設定する

必要ライブラリ:
  pip install pyserial websockets
"""

import asyncio
import json
import time
import math
import random
import argparse
import datetime

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("警告: pyserial 未インストール。シリアルモードは使用不可。")
    print("インストール: pip install pyserial")

try:
    import websockets
    from websockets.server import serve
except ImportError:
    print("エラー: websockets 未インストール。")
    print("インストール: pip install websockets")
    import sys
    sys.exit(1)

# ============================================================
# 設定
# ============================================================
WS_HOST = "0.0.0.0"   # 全インターフェース待ち受け（WiFi モードで ESP32 から接続するため）
WS_PORT = 8765
SERIAL_BAUD = 115200

# 接続クライアントの種別管理
browser_clients: set = set()   # index.html ブラウザ
device_clients: set  = set()   # ESP32 デバイス

# ============================================================
# ブラウザへのブロードキャスト
# ============================================================
async def broadcast_to_browsers(data: dict):
    """全ブラウザクライアントにデータを送信する"""
    if not browser_clients:
        return
    message = json.dumps(data, ensure_ascii=False)
    disconnected = set()
    for client in browser_clients:
        try:
            await client.send(message)
        except Exception:
            disconnected.add(client)
    browser_clients -= disconnected

# ============================================================
# WebSocket ハンドラ（ブラウザ / ESP32 共通）
# ============================================================
async def ws_handler(websocket):
    """
    接続種別を判定:
      - 最初に {"type":"hello","device":"ESP32"} を送ってきたら ESP32
      - それ以外（またはメッセージなし）はブラウザ
    """
    client_addr = websocket.remote_address
    is_device = False

    try:
        # 最初のメッセージを少し待つ（ESP32 の hello 判定）
        try:
            first_msg = await asyncio.wait_for(websocket.recv(), timeout=2.0)
            try:
                data = json.loads(first_msg)
                if data.get("type") == "hello" and "device" in data:
                    # ESP32 デバイスとして登録
                    is_device = True
                    device_clients.add(websocket)
                    print(f"[WS] ESP32 接続: {client_addr} ({data['device']})")
                    # ブラウザに接続通知
                    await broadcast_to_browsers({
                        "type": "device_connected",
                        "device": data["device"],
                        "timestamp": time.time()
                    })
                else:
                    # 最初のメッセージがセンサーデータの場合
                    browser_clients.add(websocket)
                    print(f"[WS] ブラウザ接続: {client_addr}")
                    await websocket.send(json.dumps({
                        "type": "connected",
                        "message": "SAMDEMO ダッシュボードに接続しました",
                        "timestamp": time.time()
                    }))
                    # 受信したデータも処理
                    await process_message(first_msg)
            except json.JSONDecodeError:
                # JSON でない最初のメッセージ → デバイスのログとして処理
                device_clients.add(websocket)
                is_device = True
                print(f"[WS] デバイス接続（raw）: {client_addr}")
                await process_message(first_msg)

        except asyncio.TimeoutError:
            # タイムアウト → ブラウザとして扱う
            browser_clients.add(websocket)
            print(f"[WS] ブラウザ接続: {client_addr}")
            await websocket.send(json.dumps({
                "type": "connected",
                "message": "SAMDEMO ダッシュボードに接続しました",
                "timestamp": time.time()
            }))

        # 以降のメッセージを受信し続ける
        async for message in websocket:
            if is_device:
                # ESP32 からのセンサーデータ → ブラウザへブロードキャスト
                await process_message(message)
            # ブラウザからのメッセージは現状無視（将来の制御コマンド用）

    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        print(f"[WS] エラー: {e}")
    finally:
        browser_clients.discard(websocket)
        device_clients.discard(websocket)
        print(f"[WS] 切断: {client_addr} ({'ESP32' if is_device else 'ブラウザ'})")

async def process_message(raw: str):
    """受信メッセージを解析してブラウザへ送信"""
    raw = raw.strip() if isinstance(raw, str) else raw.decode("utf-8", errors="replace").strip()
    if not raw:
        return

    print(f"[DATA] {raw}")

    try:
        data = json.loads(raw)
        # type が未設定のセンサーデータに type を付与
        if "type" not in data:
            data["type"] = "sensor"
        data.setdefault("received_at", time.time())
        await broadcast_to_browsers(data)
    except json.JSONDecodeError:
        # JSON でない（デバッグ文字列など）→ log として転送
        await broadcast_to_browsers({
            "type": "log",
            "message": raw,
            "timestamp": time.time()
        })

# ============================================================
# データソース: ESP32 シリアル受信（USB / Bluetooth SPP 共通）
# ============================================================
async def serial_reader(port: str, baud: int):
    """
    ESP32 からシリアルデータを読み取り WebSocket でブロードキャスト。
    USB Serial と Bluetooth SPP の両方に対応（どちらも仮想 COM ポート）。
    """
    if not SERIAL_AVAILABLE:
        print("[SERIAL] pyserial 未インストール。デモモードで起動します。")
        await demo_data_generator()
        return

    print(f"[SERIAL] {port} に接続中... ({baud} bps)")

    try:
        ser = serial.Serial(port, baud, timeout=1.0)
        print(f"[SERIAL] 接続成功: {port}")
        loop = asyncio.get_event_loop()

        while True:
            raw = await loop.run_in_executor(None, ser.readline)
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            if line:
                await process_message(line)

    except serial.SerialException as e:
        print(f"[SERIAL] エラー: {e}")
        print("[SERIAL] デモモードに切り替えます。")
        await demo_data_generator()

# ============================================================
# データソース: デモモード（ハードなしで動作確認）
# ============================================================
async def demo_data_generator():
    """擬似センサーデータを1秒ごとに生成してブロードキャスト"""
    print("[DEMO] デモモード起動 - 擬似センサーデータを生成します")
    t = 0
    while True:
        temp     = round(25.0 + 5.0 * math.sin(t * 0.1) + random.gauss(0, 0.3), 1)
        humidity = round(60.0 + 10.0 * math.cos(t * 0.07) + random.gauss(0, 0.5), 1)
        pressure = round(1013.25 + 2.0 * math.sin(t * 0.03) + random.gauss(0, 0.1), 1)

        data = {
            "type": "sensor",
            "temp": temp,
            "humidity": humidity,
            "pressure": pressure,
            "ts": int(time.time()),
            "received_at": time.time(),
            "demo": True
        }
        await broadcast_to_browsers(data)

        n = len(browser_clients)
        if n > 0:
            print(f"[DEMO] temp={temp}°C hum={humidity}% pres={pressure}hPa "
                  f"({n} ブラウザ接続中)")
        t += 1
        await asyncio.sleep(1.0)

# ============================================================
# メイン
# ============================================================
async def main(args):
    my_ip = get_local_ip()

    print("=" * 55)
    print("  SAMDEMO WebSocket サーバー")
    print("=" * 55)
    print(f"  起動時刻 : {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"  WebSocket: ws://{my_ip}:{WS_PORT}")
    print(f"  ブラウザ : index.html をダブルクリックして開く")
    print()

    if args.wifi:
        print(f"  [WiFi モード] ESP32 の wifi_config.h に設定:")
        print(f"    WS_SERVER_IP   = \"{my_ip}\"")
        print(f"    WS_SERVER_PORT = {WS_PORT}")
    elif args.port:
        print(f"  [シリアルモード] ポート: {args.port}")
    else:
        print(f"  [デモモード] ESP32 なしで動作確認中")
    print("=" * 55)
    print()

    # WebSocket サーバー起動
    ws_server = await serve(ws_handler, WS_HOST, WS_PORT)
    print(f"[WS] サーバー起動 ws://0.0.0.0:{WS_PORT} (外部からは ws://{my_ip}:{WS_PORT})")

    # データソース起動
    if args.wifi:
        # WiFi モード: ESP32 から ws_handler 経由でデータが届く
        print("[WiFi] ESP32 からの接続を待機中...")
        data_task = asyncio.create_task(asyncio.sleep(0))  # 何もしない（ESP32 が送ってくる）
    elif args.port:
        data_task = asyncio.create_task(serial_reader(args.port, SERIAL_BAUD))
    else:
        data_task = asyncio.create_task(demo_data_generator())

    try:
        await asyncio.Future()  # Ctrl+C まで永続
    except (KeyboardInterrupt, asyncio.CancelledError):
        pass
    finally:
        ws_server.close()
        await ws_server.wait_closed()
        print("\n=== サーバー停止 ===")

def get_local_ip() -> str:
    """ローカル IP アドレスを取得する"""
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="SAMDEMO WebSocket サーバー",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用例:
  python server.py                  デモモード（擬似データ）
  python server.py --port COM4      USB or Bluetooth シリアル受信
  python server.py --wifi           WiFi モード（ESP32 から接続受け付け）
        """
    )
    parser.add_argument("--port", "-p",
                        help="USB / Bluetooth の COM ポート (例: COM4)")
    parser.add_argument("--wifi", "-w", action="store_true",
                        help="WiFi モード: ESP32 からの WebSocket 接続を待つ")
    parser.add_argument("--demo", "-d", action="store_true",
                        help="デモモード（デフォルト動作と同じ）")
    args = parser.parse_args()

    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        print("\n終了しました。")
