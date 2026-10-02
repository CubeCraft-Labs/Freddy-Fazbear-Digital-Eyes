# Local Wi-Fi and OTA updates

The two eye boards and the DFR1154 camera can receive application updates while on the same trusted **2.4 GHz home Wi-Fi** as the updating laptop. Eye gaze still travels over the existing UART wires; Wi-Fi is only for maintenance. Each device has its own identity: `eye-1`, `eye-2`, or `camera`. The update tool checks that identity and an embedded image marker before sending a binary. The device requires a one-use nonce and HMAC-SHA256 signature, checks the received SHA-256, writes the inactive app slot, and reboots. The update tool waits for the other slot to come online.

This is a local HTTP service on port 3232 with authenticated image uploads. **Do not port-forward it or expose it to the internet.** HMAC protects update authorization and integrity, but HTTP does not encrypt the image. Use only a trusted home network. The Wi-Fi password and OTA key are compiled into firmware, so keep binaries and the ignored `firmware/private/` directory private. The camera's bootloader has rollback enabled; the Arduino eye build does not have proven automatic rollback, so retain USB access for recovery.

## One-time setup

1. Run `python3 firmware/tools/configure_ota.py` from the project root and enter the 2.4 GHz Wi-Fi name and password at the prompts. It creates `firmware/private/ota.json` and `firmware/private/OtaSecrets.h` with an automatically generated update key. Both are ignored by Git. Do not share those files or paste the password into project documentation. For the 2026-09-30 deployment, Joshua supplied the credentials in `~/.llms/credentials.env`; only the Wi-Fi name and password were read to generate these private project files.
2. Clean-build both eye variants using the commands in [firmware/README.md](README.md). Board 1 and Board 2 must use their respective build paths and board-number setting. Keep the same 16 MB / 3 MB app partition scheme; it already contains `ota_0`, `ota_1`, and `otadata`.
3. Build [the camera IDF project](camera_person_idf/README.md) with ESP-IDF 5.5.2. Its partition table has two 7 MB app slots. For a camera still using the old single-slot layout, verify its hardware address, back up its current application, then perform a one-time USB flash that erases the old layout; `erase-flash` clears its entire 16 MB flash. Each eye likewise needs one USB flash of its matching Wi-Fi-enabled build. **The current three boards completed these USB steps on 2026-09-30; do not erase them again for ordinary application updates.**
4. Watch the USB logs for `OTA_READY device=… ip=… port=3232`. Reserve a stable IP for each board in the router, or record the current addresses. Check that eye motion, blinking, and wired gaze still work after joining Wi-Fi.

Without a local secrets header, builds remain possible but Wi-Fi/OTA is disabled and logs `OTA_DISABLED`. Test-only binaries built with a placeholder SSID are not deployment images; run the configuration tool and **clean-rebuild** before flashing real boards.

## Later updates

Build a fresh application image for the desired target. From the project root, update **one device at a time**:

```sh
python3 firmware/tools/push_ota.py --host EYE_1_IP --target eye-1 --bin firmware/build-eye-test/eye_test.ino.bin
python3 firmware/tools/push_ota.py --host EYE_2_IP --target eye-2 --bin firmware/build-eye-test-board2/eye_test.ino.bin
python3 firmware/tools/push_ota.py --host CAMERA_IP --target camera --bin firmware/camera_person_idf/build/freddy_camera_person.bin
```

The script rejects the wrong target marker, a device identity mismatch, or an image larger than its inactive slot. It reports success only after the device responds from the new slot. Confirm the expected eye behavior and camera detections after each update. If a device does not return, connect it by USB and inspect its log before attempting another update. OTA updates carry only the **application binary**; changes to the partition table or bootloader still require USB.

## Hardware verification — 2026-09-30

Each board was identified by its hardware address before USB flashing. Board 1 (`28:84:85:91:CF:F8`) and Board 2 (`28:84:85:91:D2:10`) were USB-flashed and verified with their distinct builds; USB logs showed both connected to Wi-Fi. The camera (`E8:F6:0A:87:0B:4C`) had its prior 3 MB application saved as the ignored `firmware/backups/camera-pre-ota-app-2026-09-30.bin` (whole-file SHA-256 `0e44f5f2bb80e59a1727046fee8351bb54713ca5b8673f173fc390ea34694b77`). Its new bootloader, dual-slot partition table, and application were USB-flashed and hash-verified.

The same matching application image was then sent to each device over Wi-Fi, one at a time. Each accepted its image, rebooted, and returned in **slot 1**. Current DHCP addresses at verification time were Board 1 `192.168.69.37`, Board 2 `192.168.69.141`, and camera `192.168.69.204`; these can change. A later `/ota/info` check confirmed all three identities and slot 1. The camera's live USB log still showed person-detection results and `gaze_valid` transitions; Board 2 continued accepting gaze packets, including valid targets. Board 1 initially showed no incoming gaze packets; Joshua fixed its cable, after which Board 1 accepted **48 gaze packets in 12 seconds**, including valid targets, with no rejections or timeouts in that sample. Post-update eye appearance has not yet been visually reconfirmed.

Wi-Fi reconnect after router loss and a power cycle of all three devices remain untested. On 2026-10-01, a later camera application update was installed by USB (the laptop was no longer on the devices' Wi-Fi subnet), leaving the camera in slot 0; both eyes remained on their 2026-10-01 OTA-updated slot 0. See [the tracking-filter check](../hardware/tracking-filter-2026-10-01.md).
