"""Request one JPEG from camera_bringup.ino and keep evidence in this project."""

from pathlib import Path
import argparse
import hashlib
import time

import serial


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = PROJECT_ROOT / "hardware" / "camera-captures" / "first-frame.jpg"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    serial_lines = []

    with serial.Serial(args.port, 115200, timeout=2, write_timeout=2) as camera:
        time.sleep(1)
        camera.reset_input_buffer()
        camera.write(b"C")
        deadline = time.monotonic() + 12
        header = None
        while time.monotonic() < deadline:
            line = camera.readline().decode("ascii", errors="replace").strip()
            if line:
                print(line)
                serial_lines.append(line)
            if line.startswith("ERROR "):
                raise RuntimeError(line)
            if line.startswith("JPEG "):
                header = line
                break
        if header is None:
            raise TimeoutError("Camera did not return a JPEG header")

        _, size_text, width_text, height_text = header.split()
        size = int(size_text)
        if not 100 <= size <= 500_000:
            raise ValueError(f"Unexpected JPEG length: {size}")
        data = camera.read(size)
        if len(data) != size:
            raise IOError(f"Incomplete JPEG: {len(data)} of {size} bytes")
        if not (data.startswith(b"\xff\xd8") and data.endswith(b"\xff\xd9")):
            raise ValueError("JPEG markers missing")
        trailer = camera.readline().strip()
        if not trailer:
            trailer = camera.readline().strip()
        if trailer != b"DONE":
            raise ValueError(f"Unexpected frame trailer: {trailer!r}")
        serial_lines.append("DONE")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    log_path = args.output.with_suffix(".log")
    log_path.write_text(
        "\n".join(
            [
                f"port={args.port}",
                f"jpeg_sha256={hashlib.sha256(data).hexdigest()}",
                *serial_lines,
            ]
        ) + "\n"
    )
    print(f"Saved {width_text}x{height_text} JPEG ({size} bytes): {args.output}")
    print(f"Saved serial capture log: {log_path}")


if __name__ == "__main__":
    main()
