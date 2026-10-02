# Concealed camera placement

**2026-10-01 update:** Joshua now wants to try an Arducam OV5647 day/night camera with its two IR emitters in Freddy's mouth. See [the mouth placement trial](mouth-camera.md). The DFRobot chair-armrest pod described below remains a backup, not the selected location for the Pi trial.

The DFRobot DFR1154 V1.1 camera does not have to sit on Freddy's face or hat. It only needs a repeatable view of the front-door approach; its person position can be mapped to eye movement during calibration. Joshua rejected a front-of-hat badge because the lens and four onboard IR emitters would be obvious.

## Preferred placement to test

The mask will sit in a fake skeleton's lap on the **single wicker chair** in Joshua's [actual-chair photo](../reference_images/wicker_chair_skeleton.png), facing open concrete. Visitors approach from both sides. Try the [strap-on camera pod](../hardware/chair_camera_pod/README.md) beneath the **front half of the photo-right armrest**, with the lens and IR openings just below the front/outer woven lip. This is above the obscured seat underside and forward of the skeleton's hand. No mounting point is selected until live frames show both approach directions clearly and the pod remains inconspicuous from the walkway.

Do not put all five optical elements behind one close clear window: earlier IR testing washed out when the lens faced a nearby surface. A smoked or IR-pass cover would need measured transmission and opaque separation between the lens and emitters; it is not assumed to work. Likewise, a deep tunnel in front of the 160° lens would narrow its view. A concealed position is preferable to trying to camouflage a visible front-facing box.

## Position test before CAD is finalized

1. Hold the camera just beneath the photo-right armrest's front half using a temporary non-damaging support. Aim it across the part of the path where visitors approach the door from either side. Keep the lens and IR emitters clear of the wicker edge and the skeleton's hand.
2. Save a frame with a person at the far, middle, and near points, and one empty frame. The head and upper body should remain in view. Check that the armrest, skeleton, mask, and door frame do not block the lower or side frame.
3. Repeat at night with onboard IR. From the walking path, check that the camera is not conspicuous, the IR image is usable, and nearby surfaces do not reflect into the lens.
4. Measure the usable underside opening, strap route, desired camera angle, cable route, and nearest source of power. Adjust the pod or its mounting angle to those measurements before the final print.

The eventual housing should have a removable back or board tray, wire strain relief and a drip loop, dark non-reflective surfaces around the lens, and mounting that permits aiming after installation. It still needs protection from wind-driven rain and condensation even under the porch roof.
