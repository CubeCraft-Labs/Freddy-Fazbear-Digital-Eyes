# Camera-to-eyes gaze link (version 1)

The DFRobot DFR1154 broadcasts one 3.3 V UART stream to the RXD pin of both Waveshare ESP32-S3-Touch-LCD-2.1 Rev2.0 boards. Its GPIO43 / Gravity 43 is TX. Each eye uses GPIO44 / RXD on the 4-pin UART header. Join all three grounds. Leave both eye TXD pins disconnected. **Do not join the boards' 3V3 output pins or power one board from another's 3V3 pin.** Supply power separately, with a common ground and an eventual measured power budget. The Waveshare UART USB-C port must be unplugged while using the 4-pin header; that port switches the header out. The separate Waveshare `USB` port remains available for flashing and power. Verify the physical connector orientation and labels on the actual boards before wiring.

References: [Waveshare interface map](https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.1) (UART header GPIO43/44 and USB-C switch) and [DFRobot DFR1154 pinout](https://wiki.dfrobot.com/dfr1154/) (Gravity 43 TX and 44 RX). Camera USB remains the ESP-IDF diagnostic console. The gaze stream uses UART1 at 115200 baud, 8 data bits, no parity, 1 stop bit.

## Frame

Twelve bytes, little-endian for 16-bit fields:

| Offset | Field | Value |
| --- | --- | --- |
| 0–1 | Preamble | `A5 5A` |
| 2 | Version | `01` |
| 3 | Flags | bit 0: person target valid; other bits zero |
| 4–5 | Sequence | increments for every result, modulo 65536 |
| 6–7 | X | 0–1000, left to right in camera image |
| 8–9 | Y | 0–1000, top to bottom in camera image |
| 10–11 | CRC | CRC-16/CCITT-FALSE over bytes 0–9, initial `FFFF`, polynomial `1021` |

The camera sends one frame after each completed person-detection result, normally about four per second. It sends `valid=0`, X=Y=500 when no person is detected. To avoid reacting to a single isolated result, the current prototype requires three consecutive detections with centers within 80 horizontal and 60 vertical image pixels of the previous center. This is only a first gate: repeated false positives on a static scene feature can still qualify, and two-person identity tracking has not been validated.

Each eye rejects bad preambles, versions, flags, CRCs, coordinate ranges, duplicate sequences, and old sequences. A sequence that advances by 1–32767 modulo 65536 is fresh. After 750 ms without an accepted frame, the eye returns to autonomous idle movement and allows a restarted camera to begin a new sequence. A fresh `valid=0` frame also returns it to idle. Since 2026-10-01, `valid=1` also selects the glowing-red eye and `valid=0` or timeout restores blue; no distance reading is involved. Both eyes use the same camera coordinates and ease independently toward the corresponding gaze position. Blink animation continues in either mode. Horizontal and vertical calibration remain to be checked with the actual mounted camera and displays; the current centered mapping spans ±52 and ±31 display pixels. Each eye prints `GAZE_ACCEPT board=...`, `GAZE_REJECT board=... reason=...`, and `GAZE_TIMEOUT board=...` on its USB console for the bench log.

The camera's diagnostic JPEG hex output is disabled by default for the gaze build to avoid periodic serial stalls. Set `FREDDY_CAMERA_DIAGNOSTIC_JPEG=1` at compile time when saving matched image frames for diagnosis. The camera also rejects model boxes covering at least 90% of both image dimensions, based on a repeated empty-scene full-frame artifact. This geometry rule is not a substitute for installed-view false-positive testing.

On 2026-09-27 all three MAC-checked boards were flashed with the link firmware and wired. A simultaneous USB capture showed both eyes accepting the same live camera coordinates (for example, sequence 31 at X=655, Y=774, valid=1, and sequence 37 at X=830, Y=715, valid=1). Joshua moved left and right in front of the camera and confirmed both displays showed `TRACK` and the irises followed together. The wired delivery and visible bench synchronization are verified. Use `python3 firmware/tools/capture_gaze.py --camera PORT --eye1 PORT --eye2 PORT --duration 15` from the repository root to repeat the capture; any one or two port options can also be used.

A bench-only diagnostic camera build then sent a valid frame, a duplicate, an older sequence, a wrong-version frame, a bad-CRC frame, and a fresh recovery frame. **Both** eye boards reported two sequence rejections and two format rejections, then accepted recovery sequence 61442. The normal camera build was restored, hash-verified after flashing, and both receivers again accepted matching sequence numbers. See [the bench log](../hardware/gaze-link-bench.md). The diagnostic sender is compiled out by default (`FREDDY_GAZE_LINK_SELF_TEST=0`).

## Remaining integration checks

1. With the eyes and camera in their final stationary positions, calibrate horizontal/vertical direction and travel and measure end-to-end response on walk-bys.
2. During an active `TRACK` state, interrupt the camera UART for more than 750 ms and confirm both screens return to `IDLE`, then recover on reconnection. The receiver timeout currently passes native tests but has not been visually verified on hardware.
3. Record a video of both displays during a walk-by and test empty-path false positives, multiple people, day/night lighting, and the eventual power arrangement.
