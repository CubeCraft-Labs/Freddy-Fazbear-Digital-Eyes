# Outdoor camera deployment

**2026-10-01 update:** For the proposed Pi trial, Joshua prefers to test the Arducam day/night camera in Freddy's mouth, using its paired IR emitters as part of the character effect. See [mouth placement](mouth-camera.md). The DFRobot chair-armrest placement below remains the current hardware's fallback plan. The mouth trial needs its own lens/IR reflection, sightline, and weather checks.

The Freddy mask is intended as a stationary prop on a covered porch beside the front door, following people who walk up to the house. The DFRobot DFR1154 V1.1 is a plausible camera for this close, sheltered position, but reliability there is not yet established. Indoor held-position tests produced coordinates at approximately four updates per second with a typical 233 ms capture-to-result time. Empty-room tests also produced false detections at the current score threshold.

DFRobot specifies an OV3660 camera with a 160° field of view, visible/940 nm IR sensitivity, onboard IR illumination, and an operating temperature of -10 to 60 °C. Its [product documentation](https://wiki.dfrobot.com/dfr1154/) does not state a water or dust protection rating. The porch roof helps, but the bare board, USB connector, and camera flex still need protection from wind-driven rain, condensation, and insects. A mask cavity alone is not assumed to be sealed.

## Camera placement

- The mask will sit in a skeleton's lap on the single wicker chair in the [actual-chair photo](../reference_images/wicker_chair_skeleton.png). The camera and its four IR emitters must be hidden from visitors' normal view; an exposed hat badge was rejected. The [armrest underside pod](../hardware/chair_camera_pod/README.md) is a first-fit candidate beneath the front half of the photo-right armrest. The lens and IR openings must sit below the front/outer wicker edge, clear of the skeleton's hand, with useful person-detection frames from both approach directions. A separate camera viewpoint can still supply coordinates to the eyes after calibration.
- Aim the camera at the front-door approach rather than the whole yard. The very wide field of view makes distant people small in the 320 × 240 frames used by the current detector, but visitors near the door should be much larger. Measure the actual approach distance and inspect saved frames before fixing the mount.
- Give the lens an unobstructed view. Keep any protective window clean and separated from the onboard IR LEDs, or place IR illumination outside that window. A close surface already caused an IR-lit purple washout during bench testing.
- Use the porch roof as shade while preserving a view of the approach. Check for strong backlight from the yard and reflections from the front door or sidelights; indoor success does not predict performance under those conditions.

## Outdoor acceptance test

1. Place the camera at the proposed chair-armrest height and angle with its intended housing. Record the path distance and a frame with a person at the near, middle, and far parts of the intended tracking zone.
2. Record repeated normal-speed walk-bys in daylight, deep shade, dusk, and darkness. Include approach from both directions and a clear path. Measure detection rate, missed runs, false triggers, and capture-to-coordinate latency from saved logs and images.
3. Verify whether the onboard IR lights improve the night walk-by results at the installed distance. Check the window for IR glare and the camera for condensation after a temperature change.
4. Test two people crossing and short occlusion. A stable target should continue through a brief miss and clear when the person leaves. The current raw model output is not yet suitable to command the eyes directly because of observed false positives.
5. Run the complete protected installation for at least 30 minutes with its final power supply. Confirm stable camera and eye behavior, clear lens, dry connections, and acceptable internal temperature.

If people remain too small or tracking is too slow at the real path distance, adjust camera aim and resolution/model first, then consider a narrower-field camera or different detector. Keep the installed sensor choice contingent on these tests.
