"""Raspberry Pi sender for the two existing ESP32 eye boards' gaze protocol."""

from __future__ import annotations

import struct
from typing import BinaryIO


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def encode_frame(sequence: int, x: int, y: int, valid: bool) -> bytes:
    """Encode protocol v1; x and y are normalized camera coordinates (0..1000)."""
    if not 0 <= sequence <= 0xFFFF or not 0 <= x <= 1000 or not 0 <= y <= 1000:
        raise ValueError("sequence or coordinates out of range")
    body = struct.pack("<2sBBHHH", b"\xA5\x5A", 1, int(bool(valid)), sequence, x, y)
    return body + struct.pack("<H", crc16_ccitt(body))


def center_to_gaze(box: tuple[int, int, int, int], width: int, height: int) -> tuple[int, int]:
    """Convert a person box (left, top, right, bottom) to the eye protocol range."""
    if width < 2 or height < 2:
        raise ValueError("frame must be at least 2 by 2 pixels")
    left, top, right, bottom = box
    if not (0 <= left <= right < width and 0 <= top <= bottom < height):
        raise ValueError("box must lie within frame")
    return ((left + right) // 2 * 1000 // (width - 1),
            (top + bottom) // 2 * 1000 // (height - 1))


class GazeLink:
    """Write one shared gaze stream; sequence advances even for invalid targets."""

    def __init__(self, stream: BinaryIO) -> None:
        self.stream = stream
        self.sequence = 0

    def send(self, x: int, y: int, valid: bool) -> int:
        sequence = self.sequence
        frame = encode_frame(sequence, x, y, valid)
        written = self.stream.write(frame)
        if written != len(frame):
            raise OSError(f"short UART write: {written} of {len(frame)} bytes")
        self.sequence = (sequence + 1) & 0xFFFF
        return sequence


def open_pi_uart(device: str = "/dev/serial0") -> GazeLink:
    """Open the Pi's enabled hardware UART after disabling its login console."""
    import os
    import termios

    fd = os.open(device, os.O_WRONLY | os.O_NOCTTY)
    try:
        settings = termios.tcgetattr(fd)
        settings[0] = 0  # No input translation or software flow control.
        settings[1] = 0  # No output translation.
        settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8  # 8N1, no RTS/CTS.
        settings[3] = 0  # Raw mode; no terminal processing.
        settings[4] = termios.B115200
        settings[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, settings)
        return GazeLink(os.fdopen(fd, "wb", buffering=0))
    except BaseException:
        os.close(fd)
        raise
