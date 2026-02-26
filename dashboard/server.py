#!/usr/bin/env python3
"""
server.py - SAMDEMO ダッシュボードサーバー

起動コマンド:
  python server.py --port COM4

ブラウザでアクセス:
  http://localhost:8080

必要ライブラリ:
  pip install pyserial websockets
"""

import asyncio
import json
import math
import random
import time
import datetime
import socket
import threading
import argparse
import os
from http.server import HTTPServer, SimpleHTTPRequestHandler

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

from websockets.asyncio.server import serve
import websockets.exceptions

# ============================================================
# 設定
# ============================================================
HTTP_PORT = 8080   # ブラウザでアクセスするポート
WS_PORT   = 8765   # WebSocket ポート（内部通信）
BAUD      = 115200

# 接続中ブラウザクライアント
clients: set = set()

# ============================================================
# HTTP サーバー（index.html を配信）
# ============================================================
def start_http_server(directory: str):
    """index.html などの静的ファイルを HTTP で配信する"""
    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=directory, **kwargs)
        def log_message(self, *args):
            pass  # アクセスログを非表示

    httpd = HTTPServer(("", HTTP_PORT), Handler)
    httpd.serve_forever()

# ============================================================
# ブラウザへのブロードキャスト
# ============================================================
async def broadcast(data: dict):
    if not clients:
        return
    msg = json.dumps(data, ensure_ascii=False)
    dead = set()
    for ws in clients:
        try:
            await ws.send(msg)
        except Exception:
            dead.add(ws)
    clients.difference_update(dead)

# ============================================================
# WebSocket ハンドラ
# ============================================================
async def ws_handler(websocket):
    clients.add(websocket)
    addr = websocket.remote_address
    print(f"[WS] ブラウザ接続: {addr}")
    try:
        await websocket.send(json.dumps({"type": "connected"}))
        async for _ in websocket:
            pass  # ブラウザからのメッセージは無視
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        clients.discard(websocket)
        print(f"[WS] 切断: {addr}")

# ============================================================
# シリアル受信（ESP32 からの JSON を読む）
# ============================================================
async def serial_reader(port: str):
    if not SERIAL_AVAILABLE:
        print("[SERIAL] pyserial 未インストール → デモモードで起動")
        await demo_generator()
        return

    print(f"[SERIAL] {port} に接続中...")
    try:
        ser = serial.Serial(port, BAUD, timeout=1.0)
        print(f"[SERIAL] 接続成功 ({BAUD}bps)")
        loop = asyncio.get_event_loop()
        while True:
            raw = await loop.run_in_executor(None, ser.readline)
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            print(f"[DATA] {line}")
            try:
                data = json.loads(line)
                data.setdefault("type", "sensor")
                data.setdefault("received_at", time.time())
                await broadcast(data)
            except json.JSONDecodeError:
                await broadcast({"type": "log", "message": line,
                                 "timestamp": time.time()})
    except serial.SerialException as e:
        msg = str(e)
        print(f"[SERIAL] エラー: {msg}")
        if "PermissionError" in msg or "アクセスが拒否" in msg:
            print(f"[SERIAL] {port} が別アプリに使用中です。")
            print(f"[SERIAL] タスクマネージャーで python.exe を終了してから再起動してください。")
        print("[SERIAL] デモモードに切り替えます。")
        await demo_generator()

# ============================================================
# デモモード（ESP32 なしで動作確認）
# ============================================================
async def demo_generator():
    print("[DEMO] 擬似センサーデータを生成します（ESP32 なし確認用）")
    t = 0
    while True:
        data = {
            "type":      "sensor",
            "temp":      round(25.0 + 5.0 * math.sin(t * 0.1) + random.gauss(0, 0.3), 1),
            "humidity":  round(60.0 + 10.0 * math.cos(t * 0.07) + random.gauss(0, 0.5), 1),
            "pressure":  round(1013.25 + 2.0 * math.sin(t * 0.03), 1),
            "ts":        int(time.time()),
            "demo":      True,
        }
        await broadcast(data)
        if clients:
            print(f"[DEMO] {data['temp']}°C {data['humidity']}% "
                  f"{data['pressure']}hPa ({len(clients)}台接続)")
        t += 1
        await asyncio.sleep(1.0)

# ============================================================
# メイン
# ============================================================
async def main(args):
    my_ip = _local_ip()
    dashboard_dir = os.path.dirname(os.path.abspath(__file__))

    # HTTP サーバーをバックグラウンドスレッドで起動
    t = threading.Thread(target=start_http_server, args=(dashboard_dir,), daemon=True)
    t.start()

    print()
    print("=" * 50)
    print("  SAMDEMO ダッシュボード サーバー")
    print("=" * 50)
    print(f"  起動時刻 : {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    print(f"  ★ ブラウザで開く → http://localhost:{HTTP_PORT}")
    print(f"  　 同一LAN内から → http://{my_ip}:{HTTP_PORT}")
    print()
    if args.port:
        print(f"  [モード] シリアル ({args.port})")
    else:
        print(f"  [モード] デモ（--port COM4 で実機接続）")
    print("=" * 50)
    print()

    # データソース起動
    if args.port:
        data_task = asyncio.create_task(serial_reader(args.port))
    else:
        data_task = asyncio.create_task(demo_generator())

    # WebSocket サーバー起動
    async with serve(ws_handler, "0.0.0.0", WS_PORT, reuse_address=True):
        print(f"[WS]   WebSocket  ws://localhost:{WS_PORT}")
        print(f"[HTTP] HTTP       http://localhost:{HTTP_PORT}")
        print()
        print("終了: Ctrl+C")
        print()
        try:
            await asyncio.Future()
        except (KeyboardInterrupt, asyncio.CancelledError):
            pass
        finally:
            data_task.cancel()

    print("\n=== 停止しました ===")

def _local_ip() -> str:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SAMDEMO ダッシュボード")
    parser.add_argument("--port", "-p", help="COM ポート (例: COM4)")
    args = parser.parse_args()
    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        print("\n終了しました。")
