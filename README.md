# Freddy Fazbear digital eyes

Two animated round LCD eyes for a stationary Freddy Fazbear mask on a covered porch beside the front door. The eyes should blink and move naturally, then follow people walking up to the house. The ESP32 eye boards remain the display controllers. Joshua is now evaluating his Raspberry Pi Zero 2 W with a Pi camera as the person-detection and coordination node; the existing DFRobot ESP32 camera remains a working bench baseline.

## Hardware and current state

- **Eyes:** Two Waveshare ESP32-S3-Touch-LCD-2.1 modules, 480 × 480 pixels, with their own ESP32-S3 controllers. Both respond over USB as distinct ESP32-S3 devices with 8 MB PSRAM and 16 MB flash. Both arrived with an “Onboard parameters” demo and working touch, and now run the eye test. Their backs are marked Rev2.0, and Waveshare maps the printed model to SKU 28169. See [hardware/bringup.md](hardware/bringup.md).
- **Firmware baseline:** The official Waveshare Arduino/LVGL display example compiles on this laptop with the board's memory settings. A custom eye test runs on both boards with distinct on-screen labels. Both boards have been flashed and hash-verified with the supplied Freddy iris texture and matching `#743824` eyelids/perimeter rings. Joshua visually confirmed both displays look around and blink with the matching design. Frame-rate measurement and post-flash image evidence remain. See [firmware/README.md](firmware/README.md).
- **Wi-Fi updates:** Both eyes and the camera have joined the home Wi-Fi and each completed a verified OTA application update on 2026-09-30. The camera's one-time dual-slot partition migration is complete. After the updates, the camera still detected people and Board 2 received its wired gaze packets; Board 1 resumed receiving them after Joshua fixed its cable. See [firmware/ota.md](firmware/ota.md).
- **Vision:** The DFRobot DFR1154 V1.1 ESP32-S3 camera is the current bench source and produces person-position results at about four updates per second. On 2026-10-01 its USB-flashed tracker added a one-second hold for brief misses and rejected a measured empty-scene box artifact. In a 20-second empty-view retest, it sent 0 valid targets in 80 frames; real-person performance with this revision is still unverified. Joshua wants to test his Pi Zero 2 W as the eventual vision source while retaining the ESP32 eyes. He identified an owned InnoMaker OV5647 camera with an IR-filter lens and showed two Arducam alternatives. His preferred first placement trial is the Arducam OV5647 day/night camera and its IR emitters **inside Freddy's mouth**. See [the mouth placement trial](planning/mouth-camera.md), [Pi vision plan](planning/pi-vision.md), and [outdoor deployment plan](planning/outdoor-deployment.md).
- **Gaze link:** The camera's versioned 3.3 V UART broadcast was bench-verified with both eyes: they accepted identical coordinates, visibly tracked together, rejected malformed and stale frames, then recovered. After the 2026-09-30 OTA updates, Board 2 kept receiving gaze packets; Board 1 resumed receiving them after its cable was fixed, accepting 48 packets in a 12-second check. Final-position calibration, active-tracking signal-loss behavior, and walk-by testing remain. See [protocol/gaze.md](protocol/gaze.md) and [the bench log](hardware/gaze-link-bench.md).
- **Tracking effect:** Both eyes now switch to the glowing-red look when the camera sends a confirmed person target (`TRACK`) and return to blue on an invalid target or 750 ms gaze-signal timeout (`IDLE`). The former touch-toggle preview was removed. Both eye updates were delivered over Wi-Fi on 2026-10-01; Joshua confirmed the red-to-blue transition and later reported the eyes appearing synchronized. A distance sensor is no longer needed for this effect. See [planning/proximity-red.md](planning/proximity-red.md).
- **Mask:** The [Shapr3D mask STEP](stls/freddy_fazbear_mask.step) has been measured, and an official Waveshare board CAD reference inspected separately. Each straight-on projected eye opening is about 62.5 mm wide by 54.5 mm high; the largest fully clear circle is about 52.2 mm across. These are CAD sightline measurements, not physical-print tolerances. [Connected rear](reference_images/IMG_6530.jpeg) and [powered front](reference_images/IMG_6535.jpeg) dry-fit photos show one board behind an opening. A [first-fit removable mount set](hardware/eye_mount/README.md) now has candidate carriers, shell pads, stand-off sleeves, and a trim appearance gauge; the CAD clears the mask and vendor board reference but needs physical fit testing with both plugged boards. Joshua wants an integrated look: a retained, mask-finished surround should conceal the exposed dark socket and display bezel without touching the glass. Final centering/rotation, both-eye physical fit, cable strain relief, and repeatable mask attachment remain unverified. See [hardware/fit.md](hardware/fit.md).
- **Project records:** Maestro project `Freddy Fazbear Digital Eyes` in the `Maniacs-Mansion` workspace tracks tasks. CTXD project `freddy-fazbear-digital-eyes` records durable context and decisions.

## Proposed system

```text
Vision controller: current DFRobot ESP32-S3; proposed Pi Zero 2 W + Pi camera
  detects a person and chooses one target
  sends normalized target position + validity
                 |
           3.3 V serial link
                 |
       +---------+---------+
       |                   |
left Waveshare ESP32   right Waveshare ESP32
eye animation + LCD    eye animation + LCD
```

Each eye renders locally, so blinking and smooth movement continue between camera updates. A wired UART broadcast to both eye receivers is bench-verified. The proposed Pi would send the same frame format over one 3.3 V serial output, replacing the DFRobot camera's TX connection. Share signal ground and power each board from an appropriately sized supply rather than another board's 3.3 V output. The vision controller must provide a person's **position** (bounding-box center), not only a yes/no detection.

## Build sequence

1. **Bring up and measure hardware.** Identify board revisions, run the vendor display demo on each board, locate and test the camera, measure eye clearance and cable paths, and choose the power scheme.
2. **Create the eye renderer.** On one board, draw a full-size eyeball with movable iris/pupil and eyelids. Add natural blink timing, eased gaze motion, idle behavior, and a configurable left/right orientation. Run it on both boards before adding tracking.
3. **Prove camera localization.** Get live camera frames and a model that returns person coordinates. Select one target consistently; handle crossings and temporary loss. Measure detection rate and latency in the intended lighting.
4. **Connect tracking to the eyes.** Define a small versioned gaze message, verify serial delivery to both boards, calibrate camera coordinates to eye motion, and measure end-to-end response. Both boards must continue idle animation if tracking or communication stops.
5. **Install and validate outdoors.** Design removable mounts, a camera sightline, and weather protection for the electronics. Test moving pedestrians in daylight, shade, dusk, and darkness from the actual path. Verify the installed power, ventilation, repeated tracking, and a sustained run.

## Acceptance checks

- Both displays boot reliably and show independently calibrated eyes.
- Each eye moves and blinks smoothly with no camera connected.
- The camera reports the location of a person walking across its field of view; the eyes visibly follow that person without jumping unpredictably between targets.
- Tracking loss returns the eyes to an idle state; recovery resumes following without a reboot.
- The installed electronics fit without forcing the display, bending its flex cable sharply, or obstructing the camera.
- The protected outdoor installation keeps the lens clear of rain, condensation, and IR reflections while capturing the full approach path.
- Walk-by tests at the installed distance show prompt, stable following in the expected daylight and night lighting; empty-path tests do not trigger false gaze.
- A 30-minute powered run completes without a reset, visible display corruption, or unsafe heat buildup.

## Decisions still to make

- Exact camera position and distance to people approaching the covered front porch.
- Final camera placement, outdoor person-detection performance target, weather enclosure, and power source.
- Exact eye look (iris color/texture, eyelid shape, and any character-specific expression).

## References

- [Waveshare ESP32-S3-Touch-LCD-2.1 documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.1)
- [DFRobot DFR1154 documentation](https://wiki.dfrobot.com/dfr1154/)
