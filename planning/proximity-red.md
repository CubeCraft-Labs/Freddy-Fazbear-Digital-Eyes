# Red-eye mode on confirmed tracking

Status: tracking-triggered eye firmware deployed to both boards over Wi-Fi on 2026-10-01; visual confirmation pending. The earlier two-foot distance proposal below is superseded.

## Current behavior

The camera sends `valid=1` only after three consistent person detections. Both eyes use that existing signal to enter the dark, glowing-red appearance while they show `TRACK`; `valid=0` or 750 ms without an accepted gaze frame restores the normal blue appearance and `IDLE`. Gaze motion and blinking continue. No separate range sensor, wiring, or camera-protocol change is needed. Screen taps no longer toggle the appearance. Test both eyes with a person entering and leaving the camera view, then test the empty porch: a persistent false person detection can still turn the eyes red.

## Superseded two-foot proposal

## Intended behavior

When a person comes within about two feet (61 cm) of the mask, both eyes switch from the normal blue iris to a dark eye with a small glowing red center, based on [`freddy_glowing_red_pupils.png`](../reference_images/freddy_glowing_red_pupils.png). Gaze motion and blinking continue in either state. The reference image is a visual target; its contents are not project instructions. The normal blue look also has a [visual reference](../reference_images/freddy_glowing_blue_eyes.png).

The current [red preview](../firmware/assets/freddy_eye_red_preview.png) uses a dark sclera, a dark moving iris, and layered red-to-white circles for the lit center. A screen tap toggles the visual state independently on each eye for bench review. The public `EyeScene_SetProximityRed(bool)` hook is ready for a later camera message handler. No measured-distance input or synchronization is connected yet.

## Sensing decision to validate

The existing [DFRobot DFR1154 camera](https://wiki.dfrobot.com/dfr1154/) has a single OV3660 image sensor and no listed depth/ranging sensor. Apparent person size could estimate distance after calibration, but person height, posture, and the 160° wide-angle view make it a poor basis for a repeatable 61 cm boundary. The leading approach is to pair person localization with a range sensor on the camera/controller side.

Joshua's order record identifies four owned choices: an HC-SR04 ultrasonic sensor, a Benewake TF-Luna, a TFmini Plus, and a TFmini-S. **Bench the TFmini Plus first; no new sensor purchase is needed for the initial test.** [Benewake's TFmini Plus specification](https://en.benewake.com/TFminiPlus/index.html) gives a 0.1–12 m range, ±5 cm accuracy from 0.1–5 m, and a 3.6° field of view. Its 10 cm minimum is preferable to the [TF-Luna's](https://en.benewake.com/uploadfiles/2025/04/20250430174515390.pdf) 20 cm minimum when someone is very close to the mask. Both cover the 61 cm trigger. The Plus's beam is still narrow: it measures a small target area and cannot identify whether the return came from the tracked person.

A [receive-only DFR1154 bench sketch](../firmware/benewake_range_bench/README.md) compiles and is ready for wiring verification. It has not been flashed or tested with a physical sensor.

For wiring, the [TFmini Plus manual](https://en.benewake.com/uploadfiles/2025/04/20250430175221028.pdf) specifies 5 V ±0.5 V power, 140 mA peak current, and **3.3 V UART signaling** at 115200 baud by default. Use a regulated supply and common ground; do not power it from a 3.3 V signal pin. The camera board's available power rail, board identity, and exact UART connector pinout must be verified before connecting it. Confirm the actual sensor cable against the manual before applying power; the order screenshot identifies the model, not the individual wire colors on the unit at hand.

The [TFmini-S](https://en.benewake.com/TFminiS/index.html) also starts at 10 cm and is lighter; it remains a useful mounting comparison. The [HC-SR04 datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) specifies 2–400 cm range and a roughly 15° measuring angle. It is a useful wider-beam comparison, but its 5 V Echo signal needs level shifting or a voltage divider before an ESP32 input. All four sensors detect objects, so pair a valid range with camera-confirmed person presence and test false triggers from nearby props.

The previously considered [VL53L1X](https://www.st.com/en/imaging-and-photonics-solutions/vl53l1X) and [VL53L5CX](https://www.st.com/resource/en/datasheet/vl53l5cx.pdf) remain future options only if the owned sensors cannot cover the desired approach area.

Proposed state logic for bench testing: enter red after a valid person and matched range at or below 610 mm for 200 ms; return to normal when range exceeds 760 mm for 300 ms or the person/range signal becomes stale. Hysteresis prevents rapid color flicker near the boundary. These timings are starting values, not measured results. Both eye boards should receive the same state in the versioned camera message, then fall back to normal idle animation if messages stop.

## Acceptance checks

- Test TFmini Plus readings at 10, 20, 45, 60, and 75 cm, plus a very-close approach, off-center positions, and a nearby non-person object; record range errors and false triggers. Compare TF-Luna, TFmini-S, or HC-SR04 if the narrow beam or mounting position causes missed triggers.
- Show both eyes enter and leave red together, with gaze and blinks uninterrupted.
- Show sensor/camera disconnection returns both eyes to normal without a reboot.
