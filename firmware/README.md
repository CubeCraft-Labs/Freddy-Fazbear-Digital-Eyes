# First eye firmware test

Local Wi-Fi OTA support is deployed and the first network update succeeded on both eye boards and the camera on 2026-09-30. Board 2 continued receiving gaze packets afterward; Board 1 resumed receiving them after Joshua fixed its cable. See [OTA setup and update steps](ota.md).

`eye_test/` is a sketch for the two Waveshare ESP32-S3-Touch-LCD-2.1 Rev2.0 boards. It uses the display, touch, I2C, and expander drivers copied from Waveshare's official `LVGL_Arduino` example (source and SHA-256 recorded in [`vendor/README.md`](../vendor/README.md)). `EyeScene.cpp` draws the Freddy eye and continues moving and blinking without camera input. The on-screen board number makes each upload visually identifiable. The wired gaze receivers on both boards now receive the camera's UART signal and visibly track together.

## Wired gaze build — 2026-09-27

The camera broadcasts the [versioned UART gaze frame](../protocol/gaze.md) to both boards. Each eye receives on GPIO44 through its 4-pin UART header, validates the frame and sequence, eases toward the target, and returns to autonomous idle movement after an invalid target or 750 ms of silence. Each eye shows `TRACK` or `IDLE` on screen and prints `GAZE_READY`, `GAZE_ACCEPT`, `GAZE_REJECT`, and `GAZE_TIMEOUT` over USB CDC for the bench log. The separate UART USB-C port must be unplugged for the header to work. Since 2026-10-01, `TRACK` also selects the red-eye appearance and `IDLE` restores blue.

## Automatic red eye on tracking — 2026-10-01

Joshua chose confirmed person tracking rather than a distance threshold as the red-eye trigger. Both eye variants now use the same validated camera `valid` flag that controls gaze and the `TRACK`/`IDLE` label. A valid target switches to the existing dark eye with a pulsing red center; an invalid target or the receiver's 750 ms signal timeout restores the blue eye. Gaze easing and blinking continue in either appearance. The old touch-toggle test overlay was removed, so touching a display no longer changes its color. The camera firmware and gaze protocol did not change.

Board 1 image: 1,304,432 bytes, SHA-256 `60dd759971f3c19c05a40e6b7d33d92702549f03cc83e849e9647e5fac6a7280`. Board 2 image: 1,304,432 bytes, SHA-256 `f134ecd75fd64c4d3f494d9e1d98ccf3493abfb5e40c45ba6391c5e6724bd4`. Both compiled with their distinct board labels and target markers, accepted a Wi-Fi OTA update, rebooted, and responded from slot 0. The camera remained online in slot 1. The automatic visual transition awaits Joshua's on-device check. Persistent empty-scene false detections can still trigger the red appearance; outdoor false-positive testing remains necessary.

Both numbered variants were built with hardware USB CDC logging and flashed to their MAC-checked boards on 2026-09-27; all upload image hashes verified. Board 1 (`…CF:F8`): 1,253,667 bytes program, 96,760 bytes static RAM, application SHA-256 `ea0bd68c6fba802d89d11fdee1674c0ac66a376475cacd4681917afcb0bc560b`. Board 2 (`…D2:10`): 1,253,667 bytes program, 96,760 bytes static RAM, application SHA-256 `8cb6564ed46754efa7cd707d7ca219dbcbf69cc77fa7ad6adeac752203c29288`. After the UART wiring was connected, both accepted matching camera coordinates and Joshua confirmed both moved together in `TRACK`. A physical diagnostic also proved both reject duplicate, older, wrong-version, and bad-CRC frames, then recover. See [the bench log](../hardware/gaze-link-bench.md).

## Freddy iris texture — 2026-09-21

The user's [`official_freddy_eye_texture.webp`](../reference_images/official_freddy_eye_texture.webp) is the iris artwork. [`make_iris.py`](tools/make_iris.py) crops the painted disc, scales it to the 178 × 178 pixel iris, adds a feathered transparent edge over the source's white backing, and encodes it as an LVGL RGB565-plus-alpha image in [`FreddyIris.cpp`](eye_test/FreddyIris.cpp). The generated [iris preview](assets/freddy_iris_preview.png) shows the image used by the firmware; the [whole-eye preview](assets/freddy_eye_display_preview.png) shows its centered position against the sclera. The original WebP is unchanged. To regenerate, run `python firmware/tools/make_iris.py` with Pillow available. See [eye-style.md](../assets/eye-style.md) for the visual baseline.

`EyeScene.cpp` moves this image in place of the original flat-color iris and separately drawn pupil/highlight. The cream sclera, dark outer ring, 4.6-second blink cycle, and 30 ms gaze-animation timer are unchanged. The source image already contains the pupil and highlights. Source SHA-256: `0d686ed397b15e1417ffbe2dcb5e6a426e16dfd4fbeda2f4e28bd975fea8229f`.

The board settings are ESP32S3 Dev Module, 16 MB flash, OPI PSRAM, and a 3 MB application partition. The compile-only command is:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=cdc \
  --libraries vendor/extracted/Arduino/libraries \
  --build-path firmware/build-eye-test \
  --build-property runtime.tools.ctags.path="$PWD/vendor/ctags-tool/bin" \
  --build-property runtime.tools.ctags-5.8-arduino11.path="$PWD/vendor/ctags-tool/bin" \
  --jobs 4 firmware/eye_test
```

For Board 2, add `--build-property compiler.cpp.extra_flags=-DEYE_BOARD_NUMBER=2` and use a separate build path (`firmware/build-eye-test-board2`). The default is Board 1. Do not override `build.extra_flags`; that removes the ESP32-S3 USB CDC definitions and silences Board 2's USB console. Both variants use the same eye scene and animation.

## Board 1 upload — 2026-09-21

The sketch compiled successfully: 1,123,899 bytes program storage (35% of a 3 MB app partition) and 96,404 bytes static RAM (29%). The application binary SHA-256 is `3d30b8df10c95b72877ef3c7a0a5a9fdcf50a4c22f77372dffa7be8a7ff758f0`.

The connected board was confirmed by hardware-address suffix `CF:F8` as Board 1. `arduino-cli upload` at 460800 baud wrote the bootloader, partition table, boot-app selector, and eye application to `/dev/cu.usbmodem1101`; esptool verified the hash of each written image and reset the board. Joshua confirmed the eyeball visibly looks around, blinks, and fits cleanly within the round display. Achieved frame rate has not yet been measured.

A direct read of Board 1's full flash stopped with a serial stream error before completing, so no byte-for-byte device backup exists. The official Waveshare full 16 MB stock image was extracted to `firmware/backups/waveshare-stock-2026-09-21.bin` as a recovery option (SHA-256 `3653fb24cbc8e4874e2cd43f170585b15e30bc434f086ba99082d2da17e644d4`). It may differ from the exact image originally shipped on this unit. The local backup file is ignored by Git. See `hardware/bringup.md` for board identity and display observations.

## Board 2 upload — 2026-09-21

Board 2 was reidentified by hardware-address suffix `D2:10`. Its labeled variant compiled successfully: 1,152,363 bytes program storage (36%) and 96,396 bytes static RAM (29%). The application binary contains `BOARD 2 EYE TEST` and has SHA-256 `27ab300069d7f48c82aacd50e667b9b4472260d2c197c382dcdad944e8fd80c7`. `arduino-cli upload --verify` at 460800 baud verified all four written images and reset the board. Joshua confirmed the label, looking-around motion, and clean blinking on screen.

## Textured firmware test — 2026-09-21

Both board variants compile with the embedded artwork. Board 1's new application is 1,218,915 bytes (38%); its binary SHA-256 is `77f12720866622a9f4fbe19d84830517db650221a00bfb601ca0eef74965242d`. Board 2's is 1,247,375 bytes (39%); SHA-256 `2e9584a0d292f9eb7da3d3216f069bb537309c9760ba712bc7ecd8c8c6a7fc33`.

Board 2 (`D2:10`) was flashed with the new build and all four image hashes verified. Joshua confirmed that the textured iris displays, moves, and blinks correctly. Board 1 still runs the previous flat-color test until its textured build is flashed.

## Eyelid color revision — 2026-09-21

At Joshua's request, the moving eyelid objects and thin perimeter ring now use `#743824`; the surrounding background remains `#39271D`. The generated [open-eye preview](assets/freddy_eye_display_preview.png) and [mid-blink preview](assets/freddy_eye_blink_preview.png) show the matching color against the textured iris. Both board variants recompiled. The current Board 1 binary SHA-256 is `f7135765fad65a4dab936e981b08b7766ca480b9a16f250809df1bc68636b7e2`; Board 2 is `150a2460a1cab5e73034f3f8dadce6095774348e9ea0913996cd621f37d5a2bd`. Board 2 was reidentified as `D2:10`, flashed, and all four image hashes verified. Board 1 (`CF:F8`) was then connected on `/dev/cu.usbmodem2101`, flashed with its matching build, and all four written image hashes verified. Joshua confirmed both displays show the textured blue iris and matching brown lids/ring while looking around and blinking.

## Glowing-red bench preview — 2026-09-21

`EyeScene_SetProximityRed(bool)` switches the active eye from the blue iris and cream sclera to a dark moving iris/sclera with a pulsating red center inspired by [`freddy_glowing_red_pupils.png`](../reference_images/freddy_glowing_red_pupils.png). The same gaze movement and blink lids remain active. The [red preview](assets/freddy_eye_red_preview.png) approximates the display composition. For bench testing, tap an eye screen to toggle its state; each board toggles independently until the proximity message link exists.

Both labeled variants compiled and were hash-verified after upload on their identity-checked boards (`CF:F8` on `/dev/cu.usbmodem2101`, `D2:10` on `/dev/cu.usbmodem1101`). Board 1 uses 1,219,503 bytes of program storage (38%) and 96,420 bytes static RAM (29%); Board 2 uses 1,248,019 bytes (39%) and 96,412 bytes (29%). Joshua confirmed that a tap switches both boards to the dark eye with a glowing red center while motion and blinking continue, and a second tap returns each to blue. The measured two-foot trigger has not been implemented; see [the proximity plan](../planning/proximity-red.md).
