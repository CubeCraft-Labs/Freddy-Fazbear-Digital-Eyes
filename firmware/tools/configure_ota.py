#!/usr/bin/env python3
"""Save local Wi-Fi credentials and a random OTA key outside version control."""

import getpass
import json
import secrets
from pathlib import Path

FIRMWARE = Path(__file__).resolve().parents[1]
PRIVATE = FIRMWARE / "private"
CONFIG = PRIVATE / "ota.json"
HEADER = PRIVATE / "OtaSecrets.h"


def main() -> None:
    old = json.loads(CONFIG.read_text()) if CONFIG.exists() else {}
    ssid = input(f"2.4 GHz Wi-Fi name [{old.get('ssid', '')}]: ").strip() or old.get("ssid", "")
    if not ssid:
        raise SystemExit("A Wi-Fi name is required")
    password = getpass.getpass("Wi-Fi password (blank to keep saved value): ")
    password = password or old.get("password", "")
    if not password:
        raise SystemExit("A Wi-Fi password is required")
    key = old.get("ota_key") or secrets.token_hex(32)
    PRIVATE.mkdir(mode=0o700, exist_ok=True)
    CONFIG.write_text(json.dumps({"ssid": ssid, "password": password, "ota_key": key}, indent=2) + "\n")
    CONFIG.chmod(0o600)
    HEADER.write_text(
        "#pragma once\n"
        f"#define FREDDY_WIFI_SSID {json.dumps(ssid)}\n"
        f"#define FREDDY_WIFI_PASSWORD {json.dumps(password)}\n"
        f"#define FREDDY_OTA_KEY {json.dumps(key)}\n"
    )
    HEADER.chmod(0o600)
    print("Saved local configuration in firmware/private/. Rebuild and flash each device once by USB.")


if __name__ == "__main__":
    main()
