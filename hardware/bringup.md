# Hardware bring-up log

## Eye board 1 — 2026-09-21

- Connected to laptop through its USB port. The laptop enumerated `/dev/cu.usbmodem1101` as an Espressif USB JTAG/serial device; this path may change on reconnect.
- A read-only `esptool v5.2.0 flash-id` query identified an ESP32-S3 revision v0.2, 8 MB embedded PSRAM, and 16 MB flash. Connection mode: USB-Serial/JTAG.
- At this initial identification step, firmware was **not** erased or flashed. The query reset the controller at completion; the later custom flash is recorded below.
- User reports the LCD displays an **“Onboard parameters”** screen and responds to touch. This matches the type of parameter display described for Waveshare's `01_LVGL_Arduino` example, so the panel and touch input pass an initial visual check. The exact installed firmware is still unverified.
- Photos received: [powered display](photos/board-1-display.png) and [board back](photos/board-1-back.png). The screen photo directly shows the factory-style “Onboard parameter” page, including 16 MB flash. The back silkscreen identifies **ESP32-S3-Touch-LCD-2.1, Rev2.0**. The connected left USB-C port is labeled `USB`; the separate right USB-C port is labeled `UART`. The 4-pin `UART` header and its RXD/TXD/3V3/GND labels are visible. Waveshare's catalog maps the printed non-B model to flat-panel **SKU 28169**; the numeric SKU is not printed on the board.
- The laptop has Arduino ESP32 core 3.3.5 installed, satisfying Waveshare's Arduino core requirement of 3.0.0 or newer. Waveshare's published `01_LVGL_Arduino` example specifies LVGL 8.3.10.
- Update: Waveshare's `LVGL_Arduino` example compiled successfully with ESP32 core 3.3.5, bundled LVGL 8.3.10, 16 MB flash, and OPI PSRAM settings. The compile-only result was 1,431,075 bytes of program storage (45% of a 3 MB app partition) and 104,656 bytes of static RAM (31%). No firmware was uploaded. Build command and source provenance are recorded in `vendor/README.md`.

## Eye board 2

- Connected to laptop in place of eye board 1 on 2026-09-21. It enumerated on `/dev/cu.usbmodem1101`, which is the port path reused after swapping devices.
- A read-only `esptool v5.2.0 flash-id` query identified an ESP32-S3 revision v0.2, 8 MB embedded PSRAM, and 16 MB flash. Connection mode: USB-Serial/JTAG. Its hardware address ends in `D2:10`, distinct from eye board 1 (`CF:F8`).
- At this initial identification step, firmware was **not** erased or flashed. The query reset the controller at completion; the later custom flash is recorded below.
- User reports the same **“Onboard parameters”** screen as eye board 1, and touch responds. Its LCD and touch input pass the initial visual check. [Powered display](photos/board-2-display.png) and [board back](photos/board-2-back.png) photos confirm the “Onboard parameter” page and the **ESP32-S3-Touch-LCD-2.1, Rev2.0** silkscreen. The connected left USB-C port is labeled `USB`; the separate right one is `UART`. Waveshare's catalog maps this printed non-B model to flat-panel **SKU 28169**; the numeric SKU is not printed on the board.
- The same official `LVGL_Arduino` example and 16 MB flash/OPI PSRAM build settings apply. The example was compiled locally, not uploaded to either board; the observed screens are their preinstalled factory-style demo.

## Next checks

1. Capture a photo or short bench recording and measure achieved frame rate for the renderer acceptance check.
2. Check the physical eye-socket clearance against the approximately 75 mm board assembly before designing the mount.

## First custom flash — Board 1, 2026-09-21

- Source: [eye test firmware](../firmware/eye_test/eye_test.ino) with Waveshare display drivers and LVGL 8.3.10. It draws a blue moving iris, blinks every 4.6 seconds, and labels the screen `BOARD 1 EYE TEST`.
- Compile-only check passed: 1,123,899 bytes program storage and 96,404 bytes static RAM. Details and binary SHA-256 are in [firmware/README.md](../firmware/README.md).
- Before upload, `/dev/cu.usbmodem1101` identified the board by hardware-address suffix `CF:F8`, matching Board 1. Upload at 460800 baud wrote all four standard images and verified each hash successfully; board reset afterward.
- Direct full-device backup failed with a serial stream error. The official Waveshare 16 MB stock image is saved under `firmware/backups/` for recovery; it is not guaranteed identical to the image originally installed on this board.
- On-screen result: Joshua confirms the eyeball visibly looks around, blinks, and fits cleanly within the round display. Achieved frame rate and a post-flash photo or recording remain to be captured.

## Second custom flash — Board 2, 2026-09-21

- Before upload, `/dev/cu.usbmodem1101` again identified hardware-address suffix `D2:10`, matching Board 2.
- The same eye test was compiled with `EYE_BOARD_NUMBER=2`; the application binary contains `BOARD 2 EYE TEST` (SHA-256 `27ab300069d7f48c82aacd50e667b9b4472260d2c197c382dcdad944e8fd80c7`). Compile passed at 1,152,363 bytes program storage and 96,396 bytes static RAM.
- Upload at 460800 baud wrote the bootloader, partition table, boot-app selector, and eye application; esptool verified the hash of each written image and reset the board. Joshua confirmed the `BOARD 2 EYE TEST` label, eye movement, and clean blinking on screen.

## Freddy iris test — Board 2, 2026-09-21

- Board 2 was confirmed again by hardware-address suffix `D2:10` and flashed with the new build containing `reference_images/official_freddy_eye_texture.webp` as a cropped, transparent iris. All four written image hashes verified.
- Joshua confirmed the new iris texture looks good on the screen and continues to move and blink correctly. Board 1 has a compiled textured build but has not yet been reflashed.

## Eyelid color update — Board 2, 2026-09-21

- The upper and lower blink lids were changed to `#743824` at Joshua's request; the background remains `#39271D`.
- Board 2 (`D2:10`) was hash-verified after flashing the revised build. Joshua confirmed the new lid color looks good.
- Joshua then asked that the thin ring around the cream eye match the eyelids. The 444-pixel ring was changed from `#27292A` to `#743824`; Board 2 was reidentified, reflashed, and hash-verified. Joshua confirmed the ring matches the lids on screen. Board 1 has a compiled build with the same appearance, awaiting connection for flashing.

## Matching appearance — Board 1, 2026-09-21

- Both boards were connected at once: Board 2 (`D2:10`) remained on `/dev/cu.usbmodem1101`; Board 1 (`CF:F8`) appeared on `/dev/cu.usbmodem2101`. Each identity was read before choosing the upload port.
- Board 1 was flashed with the textured iris and matching `#743824` lids/ring. All four image hashes verified and the board reset. Joshua confirmed that both displays show the textured blue iris and matching brown lids/ring while moving and blinking.

## Glowing-red preview — both boards, 2026-09-21

- The project now includes a red-eye state modeled on `reference_images/freddy_glowing_red_pupils.png`. It darkens the sclera and iris, pulses a small bright red center, and preserves gaze and blinking. This bench preview is toggled by tapping each screen; it is not yet controlled by person distance.
- Board 1 (`CF:F8`) and Board 2 (`D2:10`) were individually identified, flashed with their labeled variants, and all image hashes verified. Joshua confirmed both switch to the red state by touch while moving and blinking, then return to blue on a second tap.

Reference: [Waveshare ESP32-S3-Touch-LCD-2.1 documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.1).

## DFRobot camera V1.1 — 2026-09-21

- Joshua connected the DFRobot DFR1154 and read the **V1.1** board marking. `/dev/cu.usbmodem1101` identified a distinct ESP32-S3, MAC `E8:F6:0A:87:0B:4C`, with 8 MB PSRAM and 16 MB flash; this port path may change on reconnect. [DFRobot's board reference](https://wiki.dfrobot.com/dfr1154/) identifies the camera as an OV3660 and notes that the V1.1 Gravity `+` pin is a 3.3 V output.
- A five-second serial listen at 115200 baud produced no bytes from the installed application. The full 16 MB read-only backup failed after about 86 KB at 460800 baud. A retry at 115200 successfully saved the first 64 KB (`firmware/backups/dfr1154-v11-flash-prefix-2026-09-21.bin`) and the 1.25 MB app0 partition (`firmware/backups/dfr1154-v11-app0-2026-09-21.bin`). These are partial backups, not a full-device image. The app0 image passes esptool's checksum and validation-hash checks. Both files are local and ignored by Git.
- [Camera bring-up firmware](../firmware/camera_bringup/camera_bringup.ino) was compiled with Arduino ESP32 core 3.3.5: 387,343 bytes program storage and 32,428 bytes static RAM. Its camera pin map matches [DFRobot's example](https://github.com/DFRobot/DFR1154_Examples/blob/master/5.3%20CameraWebServer/source%20code/5_3/5_3.ino). After verifying the MAC, the sketch was uploaded through `/dev/cu.usbmodem1101`; esptool verified hashes for the bootloader, partition table, boot-app selector, and application, then reset the board.
- The camera returned valid 320 × 240 JPEGs over USB serial. [Initial frame](camera-captures/first-frame.jpg) showed the ceiling. After two physical aiming adjustments, the [forward frame](camera-captures/forward-frame.jpg) showed the intended room and walking area, with no visible corruption. The [serial capture log](camera-captures/forward-frame.log) records `JPEG 6154 320 240` and `DONE` plus the image SHA-256. The room is dim, so person detection performance in this lighting remains untested.
- This proves camera initialization and frame capture. It does not yet prove person localization, tracking latency, or operation with the LiDAR attached.

## DFRobot live person-position experiment — 2026-09-21

- [ESP-IDF person-detector firmware](../firmware/camera_person_idf/README.md) built with ESP-IDF 5.5.2 and Espressif's `pedestrian_detect` 0.3.2 component. Its 2.66 MB application fits the custom 10 MB app partition. After confirming camera MAC `E8:F6:0A:87:0B:4C`, the firmware was uploaded and all flashed image hashes verified.
- An initial 20-second sample at the model's default 0.70 score threshold reported `NO_PERSON` with about 231–232 ms capture-to-result latency. See [initial serial log](camera-captures/person-detector-initial.log). A person was not known to be in view for that sample.
- In a subsequent center-view test, the score threshold was 0.35, the room was brighter, and Joshua was visibly seated in the camera view. The [matching JPEG](camera-captures/person-detector-lit-center-frame-81.jpg) shows him in front of the camera. The [serial log](camera-captures/person-detector-lit-center.log) contains 107 `PERSON` results, with median reported center x=139 in a 320-pixel-wide image (range 98–161), median score 0.606, and median capture-to-result latency 233 ms. This proves on-device person localization for this setup. Threshold, lighting, and target placement changed together, so the failed initial sample does not isolate which factor mattered.
- Held left/center/right tests now have paired serial logs and JPEGs. From Joshua's perspective, the median image x-coordinate was 214.5 at his left ([log](camera-captures/person-detector-left-image.log), [image](camera-captures/person-detector-left-image-frame-421.jpg)), 139 at center, and 62 at his right ([log](camera-captures/person-detector-right-image.log), [image](camera-captures/person-detector-right-image-frame-161.jpg)). The corresponding images visually confirm those locations; all three samples contain at least 48 detections. Left/right is mirrored because Joshua faced the camera. This satisfies the first live person-position check, but does not yet establish smooth tracking while walking.
- Multiple people, loss/reacquisition, and false-positive behavior are still unverified. The current firmware emits a diagnostic JPEG about every five seconds; remove that bench-only behavior before final use. A 3.3 V wired connection to the eye boards has not yet been tested.

## Clear-room and infrared check — 2026-09-21

- With IR off and the camera aimed into an empty room, a [10-second sample](camera-captures/person-detector-empty.log) had a few short false detections on furniture. The paired [empty-room image](camera-captures/person-detector-empty-frame-1441.jpg) confirms no person in that frame. A raw positive from the 0.35-threshold model is not yet safe to use directly as an eye target.
- DFRobot documents the onboard infrared illumination on **GPIO47**. The current camera firmware sets that pin high at boot; its flash was hash-verified. With the camera pointed very close to a surface, the IR-lit frame became a [purple blur](camera-captures/person-detector-ir-settled-frame-301.jpg). After Joshua re-aimed it, a [new IR-lit frame](camera-captures/person-detector-ir-reaimed-frame-721.jpg) again showed the room. This visually establishes that the GPIO setting changes the image, but does not yet measure how much it improves person detection in darkness.
- The [IR-lit empty-room log](camera-captures/person-detector-ir-reaimed.log) contains repeated apparent detections near the left image edge (often boxes approximately x=0–60), while the paired frame has no clear person there. A target-persistence and false-positive filter is required before sending gaze coordinates to the displays. The lights currently remain on in the camera firmware.
