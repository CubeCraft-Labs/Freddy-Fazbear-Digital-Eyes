#!/usr/bin/env python3
"""Push a built application image to one Freddy ESP32 on the local Wi-Fi."""

import argparse
import hashlib
import hmac
import json
import time
import urllib.error
import urllib.request
from pathlib import Path

CONFIG = Path(__file__).resolve().parents[1] / "private" / "ota.json"
TARGETS = ("eye-1", "eye-2", "camera")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="Device IP address on the home Wi-Fi")
    parser.add_argument("--target", required=True, choices=TARGETS)
    parser.add_argument("--bin", required=True, type=Path, dest="image_path")
    parser.add_argument("--port", type=int, default=3232)
    args = parser.parse_args()

    if not CONFIG.exists():
        raise SystemExit("Run firmware/tools/configure_ota.py first")
    key = json.loads(CONFIG.read_text())["ota_key"].encode()
    image = args.image_path.read_bytes()
    marker = f"FREDDY_OTA_TARGET:{args.target}".encode()
    if marker not in image:
        raise SystemExit(f"Image does not contain the expected {marker.decode()} marker")

    base = f"http://{args.host}:{args.port}"
    with urllib.request.urlopen(base + "/ota/info", timeout=10) as response:
        info = json.load(response)
    if info.get("device") != args.target:
        raise SystemExit(f"Device reports {info.get('device')!r}, expected {args.target!r}")
    if not 0 < len(image) <= info.get("slot_bytes", 0):
        raise SystemExit(f"Image is {len(image)} bytes; device slot is {info.get('slot_bytes')} bytes")

    digest = hashlib.sha256(image).hexdigest()
    nonce = info["nonce"]
    message = f"{args.target}\n{nonce}\n{digest}\n{len(image)}".encode()
    signature = hmac.new(key, message, hashlib.sha256).hexdigest()
    request = urllib.request.Request(
        base + "/ota",
        data=image,
        method="POST",
        headers={
            "Content-Type": "application/octet-stream",
            "X-Freddy-Nonce": nonce,
            "X-Freddy-SHA256": digest,
            "X-Freddy-Signature": signature,
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=180) as response:
            result = json.load(response)
    except urllib.error.HTTPError as error:
        raise SystemExit(f"Update rejected (HTTP {error.code}): {error.read().decode(errors='replace')}") from error
    if result != {"ok": True}:
        raise SystemExit(f"Unexpected reply: {result!r}")
    print(f"{args.target}: image accepted ({len(image)} bytes, SHA-256 {digest}). Waiting for reboot…")
    old_slot = info["running_slot"]
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        time.sleep(2)
        try:
            with urllib.request.urlopen(base + "/ota/info", timeout=3) as response:
                current = json.load(response)
        except (OSError, ValueError):
            continue
        if current.get("device") == args.target and current.get("running_slot") != old_slot:
            print(f"{args.target}: back online in firmware slot {current['running_slot']}.")
            return
    raise SystemExit(
        "Image was accepted, but the new slot was not confirmed online within 60 seconds. "
        "Check device power and USB logs before retrying."
    )


if __name__ == "__main__":
    main()
