"""Measure a built-in OpenCV full-body detector on the Pi CSI camera.

This is an offline baseline: it logs detections but does not drive the eyes.
It will miss people whose full bodies are outside the frame.
"""

from __future__ import annotations

import argparse
import json
import statistics
import time
from datetime import datetime
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--duration", type=float, default=30.0, help="seconds to sample")
    parser.add_argument("--rotation", type=int, choices=(0, 180), default=0)
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=240)
    parser.add_argument("--log", type=Path, help="JSONL path (default: project diagnostics)")
    args = parser.parse_args()
    if args.duration <= 0 or args.width < 128 or args.height < 128:
        parser.error("duration must be positive and frame dimensions at least 128 pixels")

    import cv2
    from libcamera import Transform
    from picamera2 import Picamera2

    cv2.setNumThreads(2)
    hog = cv2.HOGDescriptor()
    hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())
    transform = Transform(hflip=1, vflip=1) if args.rotation == 180 else Transform()
    camera = Picamera2()
    config = camera.create_video_configuration(
        main={"size": (args.width, args.height), "format": "RGB888"},
        transform=transform,
        buffer_count=3,
    )
    camera.configure(config)

    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = args.log or PROJECT_ROOT / "diagnostics" / f"pi_hog_{stamp}.jsonl"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    detection_ms: list[float] = []
    detected_frames = 0
    frames = 0
    start = 0.0
    try:
        camera.start()
        time.sleep(1.0)  # Let exposure settle before measuring.
        start = time.monotonic()
        with log_path.open("w", encoding="utf-8") as log:
            while time.monotonic() - start < args.duration:
                frame = camera.capture_array("main")
                detect_start = time.monotonic()
                boxes, weights = hog.detectMultiScale(
                    frame,
                    hitThreshold=0.0,
                    winStride=(8, 8),
                    padding=(8, 8),
                    scale=1.05,
                    groupThreshold=1,
                )
                elapsed_ms = (time.monotonic() - detect_start) * 1000
                detections = [
                    {"x": int(x), "y": int(y), "width": int(w), "height": int(h),
                     "score": float(score)}
                    for (x, y, w, h), score in zip(boxes, weights)
                ]
                frames += 1
                detected_frames += bool(detections)
                detection_ms.append(elapsed_ms)
                log.write(json.dumps({
                    "time": datetime.now().astimezone().isoformat(),
                    "frame": frames,
                    "detect_ms": round(elapsed_ms, 1),
                    "detections": detections,
                }) + "\n")
                if frames % 10 == 0:
                    print(f"frames={frames} detected={detected_frames} "
                          f"last_detect_ms={elapsed_ms:.0f}", flush=True)
    finally:
        camera.stop()
        camera.close()

    total = time.monotonic() - start
    print(json.dumps({
        "frames": frames,
        "seconds": round(total, 2),
        "fps": round(frames / total, 2) if total else 0,
        "frames_with_person": detected_frames,
        "median_detect_ms": round(statistics.median(detection_ms), 1) if detection_ms else None,
        "log": str(log_path),
    }, indent=2))


if __name__ == "__main__":
    main()
