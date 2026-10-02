"""Record serial detections and reconstruct diagnostic JPEGs from the camera."""

import argparse
import pathlib
import time

import serial


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/cu.usbmodem1101")
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--name", default="person-detector-diagnostic")
    args = parser.parse_args()

    output = pathlib.Path(__file__).resolve().parents[2] / "hardware/camera-captures"
    output.mkdir(parents=True, exist_ok=True)
    log_path = output / f"{args.name}.log"
    frame_number = None
    expected_length = 0
    hex_lines: list[str] = []
    detections = 0
    lines: list[str] = []

    with serial.Serial(args.port, 115200, timeout=0.5) as port:
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            raw = port.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", "replace").strip()
            lines.append(line)
            if "freddy_person: PERSON " in line:
                detections += 1
                print(line, flush=True)
            elif "freddy_person: NO_PERSON " in line and len(lines) % 20 == 0:
                print(line, flush=True)

            if line.startswith("FRAME_BEGIN "):
                _, number, length = line.split()
                frame_number = int(number)
                expected_length = int(length)
                hex_lines = []
            elif line.startswith("FRAME_HEX ") and frame_number is not None:
                hex_lines.append(line.removeprefix("FRAME_HEX "))
            elif line.startswith("FRAME_END ") and frame_number is not None:
                number = int(line.split()[1])
                if number == frame_number:
                    data = bytes.fromhex("".join(hex_lines))
                    if len(data) == expected_length and data.startswith(b"\xff\xd8") and data.endswith(b"\xff\xd9"):
                        photo = output / f"{args.name}-frame-{number}.jpg"
                        photo.write_bytes(data)
                        print(f"Saved {photo} ({len(data)} bytes)", flush=True)
                    else:
                        print(f"Incomplete frame {number}: {len(data)}/{expected_length} bytes", flush=True)
                frame_number = None

    log_path.write_text("\n".join(lines) + "\n")
    print(f"Saved {log_path}; {detections} person detections", flush=True)


if __name__ == "__main__":
    main()
