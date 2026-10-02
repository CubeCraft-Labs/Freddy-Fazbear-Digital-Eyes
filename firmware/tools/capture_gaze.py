#!/usr/bin/env python3
"""Print timestamped USB logs from the camera and both eye boards."""

import argparse
import select
import time

import serial


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--camera")
    parser.add_argument("--eye1")
    parser.add_argument("--eye2")
    parser.add_argument("--duration", type=float, default=15)
    args = parser.parse_args()

    paths = {name: path for name, path in
             (("camera", args.camera), ("eye1", args.eye1), ("eye2", args.eye2))
             if path}
    if not paths:
        parser.error("provide at least one of --camera, --eye1, or --eye2")
    ports = {name: serial.Serial(path, 115200, timeout=0)
             for name, path in paths.items()}
    buffers = {name: bytearray() for name in ports}
    started = time.monotonic()
    try:
        while time.monotonic() - started < args.duration:
            ready, _, _ = select.select(list(ports.values()), [], [], 0.2)
            for port in ready:
                name = next(name for name, candidate in ports.items() if candidate is port)
                buffers[name].extend(port.read(port.in_waiting or 1))
                while b"\n" in buffers[name]:
                    line, _, rest = buffers[name].partition(b"\n")
                    buffers[name] = bytearray(rest)
                    stamp = time.monotonic() - started
                    print(f"{stamp:8.3f} {name:6s} {line.decode(errors='replace').rstrip()}", flush=True)
    finally:
        for port in ports.values():
            port.close()


if __name__ == "__main__":
    main()
