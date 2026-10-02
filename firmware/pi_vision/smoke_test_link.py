"""Briefly drive both eyes through the existing shared UART gaze link."""

from __future__ import annotations

import argparse
import time

from gaze_link import GazeLink, open_pi_uart


def send_state(link: GazeLink, x: int, y: int, valid: bool, duration: float) -> None:
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        link.send(x, y, valid)
        time.sleep(0.1)  # The eye receiver returns to idle after 750 ms without a frame.


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default="/dev/serial0", help="Pi GPIO UART device")
    args = parser.parse_args()

    link = open_pi_uart(args.device)
    try:
        for label, x, valid in (
            ("left/red", 200, True),
            ("center/red", 500, True),
            ("right/red", 800, True),
            ("idle/blue", 500, False),
        ):
            print(label, flush=True)
            send_state(link, x, 500, valid, 2.0)
    finally:
        # Leave the eyes blue even if the test is interrupted.
        for _ in range(3):
            link.send(500, 500, False)
            time.sleep(0.1)
        link.stream.close()


if __name__ == "__main__":
    main()
