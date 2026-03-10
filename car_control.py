#!/usr/bin/env python3
"""
car_control.py — ステップ1: serial_open / serial_close テスト
ctypes で libft_serial.so をロードし、基本的な open/close を確認する。
"""

import ctypes
import os
import sys

SENSOR_COUNT = 3


class CarState(ctypes.Structure):
    """t_car_state のミラー"""
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("speed", ctypes.c_float),
        ("angle", ctypes.c_float),
        ("sensor_dist", ctypes.c_float * SENSOR_COUNT),
    ]


def load_library():
    """libft_serial.so をロードし、関数シグネチャを設定する"""
    so_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           "libft_serial.so")
    lib = ctypes.CDLL(so_path)

    # serial_open(const char *device) -> t_serial_ctx *
    lib.serial_open.argtypes = [ctypes.c_char_p]
    lib.serial_open.restype = ctypes.c_void_p

    # serial_close(t_serial_ctx *ctx) -> void
    lib.serial_close.argtypes = [ctypes.c_void_p]
    lib.serial_close.restype = None

    # serial_get_state(t_serial_ctx *ctx, t_car_state *out) -> void
    lib.serial_get_state.argtypes = [ctypes.c_void_p,
                                     ctypes.POINTER(CarState)]
    lib.serial_get_state.restype = None

    return lib


def test_open_close():
    """デバイスの open/close を試す (デバイスが無くても動作確認可能)"""
    lib = load_library()
    print("[*] libft_serial.so をロードしました")

    # 引数またはデフォルトのデバイスを使用
    device = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    print(f"[*] デバイス '{device}' を開きます...")

    ctx = lib.serial_open(device.encode("utf-8"))
    if not ctx:
        print(f"[!] open 失敗 (デバイスが存在しないか権限不足)")
        print("[*] これはデバイス未接続時の正常動作です")
        return

    print(f"[+] open 成功! ctx = {hex(ctx)}")
    lib.serial_close(ctx)
    print("[+] close 完了 — リソース解放済み")


if __name__ == "__main__":
    test_open_close()
