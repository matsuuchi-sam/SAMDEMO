#!/usr/bin/env python3
"""
server.py - SAMDEMO WebSocket サーバー

使い方:
  python server.py                  デモモード（ESP32 なし、擬似データ）
  python server.py --port COM4      USB シリアル受信（ESP32 + BME280）
  python server.py --wifi           WiFi モード（ESP32 から WebSocket 接続を待つ）

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
import socket

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("警告: pyserial 未インストール。pip install pyserial")

# websockets 13+ 対応（新 API）
try:
    from websockets.asyncio.server import serve
    import websockets.exceptions
except ImportError:
    # 旧バージョン向けフォールバック
    from websockets.server import serve
    import websockets.exceptions

WS_HOST = "0.0.0.0"
WS_PORT = 8765
SERIAL_BAUD = 115200

browser_clients: set = set()
device_clients: set  = set()

# ============================================================
# ブラウザへのブロードキャスト
# ============================================================
async def broadcast(data: dict):
    global browser_clients
    if not browser_clients:
        return
    message = json.dumps(data, ensure_ascii=False)
    disconnected = set()
    for client in browser_clients:
        try:
            await client.send(message)
        except Exception:
            disconnected.add(client)
    browser_clients.difference_update(disconnected)

# ============================================================
# 受信データの処理
# ============================================================
async def process_message(raw):
    if isinstance(raw, bytes):
        raw = raw.decode("utf-8", errors="replace")
    raw = raw.strip()
    if not raw:
        return

    print(f"[DATA] {raw}")

    try:
        data = json.loads(raw)
        data.setdefault("type", "sensor")
        data.setdefault("received_at", time.time())
        await broadcast(data)
    except json.JSONDecodeError:
        await broadcast({
            "type": "log",
            "message": raw,
            "timestamp": time.time()
        })

# ============================================================
# WebSocket ハンドラ
# ============================================================
async def ws_handler(websocket):
    global browser_clients, device_clients
    client_addr = websocket.remote_address
    is_device = False

    try:
        # 最初のメッセージで ESP32 か ブラウザか判定
        try:
            first_msg = await asyncio.wait_for(websocket.recv(), timeout=2.0)
            try:
                data = json.loads(first_msg)
                if data.get("type") == "hello" and "device" in data:
                    is_device = True
                    device_clients.add(websocket)
                    print(f"[WS] ESP32 接続: {client_addr} ({data['device']})")
                    await broadcast({"type": "device_connected",
                                     "device": data["device"],
                                     "timestamp": time.time()})
                else:
                    browser_clients.add(websocket)
                    print(f"[WS] ブラウザ接続: {client_addr}")
                    await websocket.send(json.dumps({"type": "connected",
                                                     "timestamp": time.time()}))
                    await process_message(first_msg)
            except json.JSONDecodeError:
                device_clients.add(websocket)
                is_device = True
                await process_message(first_msg)

        except asyncio.TimeoutError:
            browser_clients.add(websocket)
            print(f"[WS] ブラウザ接続: {client_addr}")
            await websocket.send(json.dumps({"type": "connected",
                                             "timestamp": time.time()}))

        # 以降のメッセージを受信
        async for message in websocket:
            if is_device:
                await process_message(message)

    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        print(f"[WS] エラー: {e}")
    finally:
        browser_clients.discard(websocket)
        device_clients.discard(websocket)
        kind = "ESP32" if is_device else "ブラウザ"
        print(f"[WS] 切断: {client_addr} ({kind})")

# ============================================================
# データソース: USB / Bluetooth シリアル受信
# ============================================================
async def serial_reader(port: str, baud: int):
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
            if raw:
                await process_message(raw)
    except serial.SerialException as e:
        print(f"[SERIAL] エラー: {e}")
        if "PermissionError" in str(e) or "アクセスが拒否" in str(e):
            print(f"[SERIAL] {port} が他のプログラムに使用されています。")
            print(f"[SERIAL] PlatformIO モニタや他のシリアルターミナルを閉じてから再起動してください。")
        print("[SERIAL] デモモードに切り替えます。")
        await demo_data_generator()

# ============================================================
# データソース: デモモード
# ============================================================
async def demo_data_generator():
    print("[DEMO] デモモード - 擬似センサーデータを生成します")
    t = 0
    while True:
        data = {
            "type": "sensor",
            "temp":     round(25.0 + 5.0 * math.sin(t * 0.1) + random.gauss(0, 0.3), 1),
            "humidity": round(60.0 + 10.0 * math.cos(t * 0.07) + random.gauss(0, 0.5), 1),
            "pressure": round(1013.25 + 2.0 * math.sin(t * 0.03) + random.gauss(0, 0.1), 1),
            "ts":        int(time.time()),
            "received_at": time.time(),
            "demo": True
        }
        await broadcast(data)
        n = len(browser_clients)
        if n > 0:
            print(f"[DEMO] temp={data['temp']}°C hum={data['humidity']}% "
                  f"pres={data['pressure']}hPa ({n} ブラウザ接続中)")
        t += 1
        await asyncio.sleep(1.0)

# ============================================================
# ローカル IP 取得
# ============================================================
def get_local_ip() -> str:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

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

    if args.wifi:
        print(f"\n  [WiFi モード] ESP32 の wifi_config.h に設定:")
        print(f"    WS_SERVER_IP   = \"{my_ip}\"")
        print(f"    WS_SERVER_PORT = {WS_PORT}")
    elif args.port:
        print(f"\n  [シリアルモード] ポート: {args.port}")
    else:
        print(f"\n  [デモモード] ESP32 なしで動作確認")
    print("=" * 55 + "\n")

    # データソースを先に起動
    if args.wifi:
        data_task = None
        print("[WiFi] ESP32 からの接続を待機中...")
    elif args.port:
        data_task = asyncio.create_task(serial_reader(args.port, SERIAL_BAUD))
    else:
        data_task = asyncio.create_task(demo_data_generator())

    # WebSocket サーバー起動（新 API: async with）
    async with serve(ws_handler, WS_HOST, WS_PORT, reuse_address=True) as server:
        print(f"[WS] サーバー起動 ws://0.0.0.0:{WS_PORT}")
        try:
            await asyncio.Future()  # Ctrl+C まで永続
        except (KeyboardInterrupt, asyncio.CancelledError):
            pass
        finally:
            if data_task:
                data_task.cancel()

    print("\n=== サーバー停止 ===")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SAMDEMO WebSocket サーバー")
    parser.add_argument("--port", "-p", help="COM ポート (例: COM4)")
    parser.add_argument("--wifi", "-w", action="store_true",
                        help="WiFi モード（ESP32 からの接続待ち）")
    args = parser.parse_args()

    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        print("\n終了しました。")
