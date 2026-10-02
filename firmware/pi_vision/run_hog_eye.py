"""Trial IMX708 full-body detection driving the two eyes over one Pi UART.

By default this only logs. Pass --drive-eyes for a timed hardware trial.
"""

from __future__ import annotations

import argparse
import json
import os
import time
from datetime import datetime
from pathlib import Path

from gaze_link import GazeLink, center_to_gaze, open_pi_uart
from tracking_state import TrackingState


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=float, default=30.0)
    parser.add_argument("--rotation", type=int, choices=(0, 180), default=0)
    parser.add_argument("--drive-eyes", action="store_true")
    parser.add_argument("--device", default="/dev/serial0")
    parser.add_argument("--log", type=Path)
    args = parser.parse_args()
    if args.duration <= 0:
        parser.error("duration must be positive")

    import cv2
    from libcamera import Transform
    from picamera2 import Picamera2

    cv2.setNumThreads(2)
    hog = cv2.HOGDescriptor()
    hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())
    camera = Picamera2()
    transform = Transform(hflip=1, vflip=1) if args.rotation == 180 else Transform()
    camera.configure(camera.create_video_configuration(
        main={"size": (320, 240), "format": "RGB888"},
        transform=transform,
        buffer_count=3,
    ))
    # The first moving-person log had a 3.34 s detector gap despite a visible person.
    state = TrackingState(confirm_hits=2, hold_seconds=3.5)
    link = open_pi_uart(args.device) if args.drive_eyes else GazeLink(open(os.devnull, "wb"))
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = args.log or PROJECT_ROOT / "diagnostics" / f"pi_eye_trial_{stamp}.jsonl"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    started = False
    frames = 0
    tracked = 0
    try:
        camera.start()
        started = True
        time.sleep(1.0)
        deadline = time.monotonic() + args.duration
        with log_path.open("w", encoding="utf-8") as log:
            while time.monotonic() < deadline:
                frame = camera.capture_array("main")
                detect_start = time.monotonic()
                boxes, weights = hog.detectMultiScale(
                    frame, hitThreshold=0.0, winStride=(8, 8),
                    padding=(8, 8), scale=1.05, groupThreshold=1,
                )
                detect_ms = round((time.monotonic() - detect_start) * 1000, 1)
                target = None
                candidate = None
                if len(boxes):
                    index = max(range(len(boxes)), key=lambda i: float(weights[i]))
                    box_x, box_y, width, height = (int(v) for v in boxes[index])
                    left = max(0, box_x)
                    top = max(0, box_y)
                    right = min(box_x + width - 1, frame.shape[1] - 1)
                    bottom = min(box_y + height - 1, frame.shape[0] - 1)
                    if left <= right and top <= bottom:
                        candidate = center_to_gaze(
                            (left, top, right, bottom), frame.shape[1], frame.shape[0]
                        )
                        target = {"x": box_x, "y": box_y, "width": width,
                                  "height": height, "score": float(weights[index])}
                x, y, valid = state.update(candidate, time.monotonic())
                seq = link.send(x, y, valid)
                frames += 1
                tracked += valid
                log.write(json.dumps({
                    "time": datetime.now().astimezone().isoformat(),
                    "frame": frames,
                    "detect_ms": detect_ms,
                    "target": target,
                    "gaze": {"x": x, "y": y, "valid": valid, "sequence": seq},
                }) + "\n")
                if frames % 10 == 0:
                    print(f"frames={frames} tracked={tracked} "
                          f"last_gaze={x},{y},{int(valid)}", flush=True)
    finally:
        # The eyes should return to blue whether the trial ends normally or fails.
        try:
            if args.drive_eyes:
                for _ in range(3):
                    link.send(500, 500, False)
                    time.sleep(0.1)
        finally:
            link.stream.close()
            if started:
                camera.stop()
            camera.close()

    print(json.dumps({"frames": frames, "tracked_frames": tracked,
                      "log": str(log_path), "eyes_driven": args.drive_eyes}, indent=2))


if __name__ == "__main__":
    main()
