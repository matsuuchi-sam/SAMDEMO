#!/usr/bin/env python3
"""
server.py - SAMDEMO WebSocket サーバー

ESP32 から受け取ったセンサーデータ（JSON）をブラウザの
ダッシュボード (index.html) に WebSocket でリアルタイム配信します。

アーキテクチャ:
    [ESP32] ---Serial---> [このサーバー] ---WebSocket---> [index.html]

使い方:
    python server.py                # デフォルト設定で起動
    python server.py --port COM4    # ESP32 の COM ポートを指定
    python server.py --demo         # デモモード（ESP32 なしで動作確認）

アクセス:
    http://localhost:8765  (WebSocket)
    index.html をブラウザで開く

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
import threading

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("警告: pyserial が未インストールです。デモモードのみ使用可能です。")

try:
    import websockets
    from websockets.server import serve
except ImportError:
    print("エラー: websockets が未インストールです。")
    print("インストール: pip install websockets")
    import sys
    sys.exit(1)


# ============================================================
# 設定
# ============================================================
WS_HOST = "localhost"
WS_PORT = 8765
SERIAL_BAUD = 115200

# 接続中の WebSocket クライアント一覧
connected_clients: set = set()


# ============================================================
# WebSocket ハンドラ
# ============================================================
async def ws_handler(websocket):
    """
    新規 WebSocket 接続を処理する。
    接続中はクライアントリストに登録し、切断時に削除する。
    """
    client_addr = websocket.remote_address
    print(f"[WS] クライアント接続: {client_addr}")
    connected_clients.add(websocket)

    try:
        # 接続確認メッセージを送信
        await websocket.send(json.dumps({
            "type": "connected",
            "message": "SAMDEMO ダッシュボードに接続しました",
            "timestamp": time.time()
        }))

        # クライアントからのメッセージを待ち受け（切断検知用）
        async for message in websocket:
            # クライアントからのメッセージ（将来の制御コマンド用）
            print(f"[WS] クライアントからのメッセージ: {message}")

    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        connected_clients.discard(websocket)
        print(f"[WS] クライアント切断: {client_addr}")


async def broadcast(data: dict):
    """全接続クライアントにデータを送信する"""
    if not connected_clients:
        return

    message = json.dumps(data)
    # 切断済みクライアントへの送信エラーを無視
    disconnected = set()
    for client in connected_clients:
        try:
            await client.send(message)
        except websockets.exceptions.ConnectionClosed:
            disconnected.add(client)

    connected_clients -= disconnected


# ============================================================
# データソース: ESP32 シリアル受信
# ============================================================
async def serial_reader(port: str, baud: int):
    """
    ESP32 からシリアルデータを読み取り、WebSocket でブロードキャストする。

    期待するデータ形式 (JSON, 改行区切り):
        {"temp": 25.3, "humidity": 60.1, "pressure": 1013.2, "ts": 1234567890}
    """
    if not SERIAL_AVAILABLE:
        print("[SERIAL] pyserial 未インストール。デモモードに切り替えます。")
        await demo_data_generator()
        return

    print(f"[SERIAL] ポート {port} に接続中... ({baud} bps)")

    try:
        ser = serial.Serial(port, baud, timeout=1.0)
        print(f"[SERIAL] 接続成功: {port}")

        loop = asyncio.get_event_loop()

        while True:
            # シリアル読み取りはブロッキング処理なので executor で実行
            raw = await loop.run_in_executor(None, ser.readline)

            if not raw:
                continue

            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue

            print(f"[SERIAL] 受信: {line}")

            # JSON パース
            try:
                data = json.loads(line)
                data["type"] = "sensor"
                data["received_at"] = time.time()
                await broadcast(data)
            except json.JSONDecodeError:
                # JSON 以外のデータ（デバッグメッセージ等）
                await broadcast({
                    "type": "log",
                    "message": line,
                    "timestamp": time.time()
                })

    except serial.SerialException as e:
        print(f"[SERIAL] エラー: {e}")
        print("[SERIAL] デモモードに切り替えます。")
        await demo_data_generator()


# ============================================================
# データソース: デモモード（ESP32 なしで動作確認）
# ============================================================
async def demo_data_generator():
    """
    デモ用の擬似センサーデータを生成して WebSocket でブロードキャストする。
    サイン波 + ランダムノイズで温度/湿度/気圧を模擬。
    """
    print("[DEMO] デモモード起動 - 擬似センサーデータを生成します")

    t = 0
    while True:
        # 擬似センサーデータ（リアルっぽい変動を追加）
        temp = 25.0 + 5.0 * math.sin(t * 0.1) + random.gauss(0, 0.3)
        humidity = 60.0 + 10.0 * math.cos(t * 0.07) + random.gauss(0, 0.5)
        pressure = 1013.25 + 2.0 * math.sin(t * 0.03) + random.gauss(0, 0.1)

        data = {
            "type": "sensor",
            "temp": round(temp, 1),
            "humidity": round(humidity, 1),
            "pressure": round(pressure, 1),
            "ts": int(time.time()),
            "received_at": time.time(),
            "demo": True
        }

        await broadcast(data)

        client_count = len(connected_clients)
        if client_count > 0:
            print(f"[DEMO] 送信: temp={data['temp']}°C, "
                  f"humidity={data['humidity']}%, "
                  f"pressure={data['pressure']}hPa "
                  f"({client_count} クライアント)")

        t += 1
        await asyncio.sleep(1.0)  # 1秒間隔


# ============================================================
# メイン
# ============================================================
async def main(args):
    """WebSocket サーバーとデータソースを起動する"""
    print("=== SAMDEMO WebSocket サーバー ===")
    print(f"WebSocket: ws://{WS_HOST}:{WS_PORT}")
    print(f"起動時刻: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()

    # WebSocket サーバーを起動
    ws_server = await serve(ws_handler, WS_HOST, WS_PORT)
    print(f"[WS] サーバー起動: ws://{WS_HOST}:{WS_PORT}")
    print(f"[WS] ブラウザで index.html を開いてください")
    print()

    # データソースを起動
    if args.demo:
        data_task = asyncio.create_task(demo_data_generator())
    elif args.port:
        data_task = asyncio.create_task(serial_reader(args.port, SERIAL_BAUD))
    else:
        print("[INFO] ポート未指定。デモモードで起動します。")
        print("[INFO] ESP32 を使う場合: python server.py --port COM4")
        data_task = asyncio.create_task(demo_data_generator())

    # Ctrl+C まで待機
    try:
        await asyncio.gather(ws_server.wait_closed(), data_task)
    except (KeyboardInterrupt, asyncio.CancelledError):
        pass
    finally:
        ws_server.close()
        data_task.cancel()
        print("\n=== サーバー停止 ===")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SAMDEMO WebSocket サーバー")
    parser.add_argument("--port", "-p", help="ESP32 の COM ポート (例: COM4)")
    parser.add_argument("--demo", "-d", action="store_true",
                        help="デモモード（擬似データを生成）")
    args = parser.parse_args()

    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        print("\n終了しました。")
