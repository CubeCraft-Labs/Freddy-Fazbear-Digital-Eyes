# Wired gaze-link bench — 2026-09-27

Hardware: DFRobot DFR1154 V1.1 camera (MAC `E8:F6:0A:87:0B:4C`), Waveshare eye Board 1 (`…CF:F8`), and Board 2 (`…D2:10`). Camera GPIO43 TX fans out to both eye GPIO44 RX pins, with common ground. All three use their separate USB diagnostic/power ports; the eye UART USB-C ports are unplugged.

## Normal tracking

Command: `python3 firmware/tools/capture_gaze.py --camera /dev/cu.usbmodem11401 --eye1 /dev/cu.usbmodem11201 --eye2 /dev/cu.usbmodem11101 --duration 12`. Both receivers reported `GAZE_READY ... recent=1 bad_format=0 bad_sequence=0` and accepted the same live sequence/coordinates, including `seq=31 x=655 y=774 valid=1` and `seq=37 x=830 y=715 valid=1`. Both also accepted `valid=0` idle commands. Joshua confirmed both screens switched to `TRACK` and their irises followed together when moving left and right in front of the camera. This is a user-observed bench result; no video was saved.

## Malformed/stale injection

The camera was temporarily built with `FREDDY_GAZE_LINK_SELF_TEST=1` and flashed to its MAC-verified board. At boot it sent sequence 61440 (`valid=0`), the duplicate 61440, old sequence 61439, a wrong-version 61441, a bad-CRC 61441, and fresh recovery sequence 61442. Both eye USB logs showed:

```text
39.227 eye1 GAZE_ACCEPT board=1 seq=61440 x=500 y=500 valid=0
39.229 eye2 GAZE_ACCEPT board=2 seq=61440 x=500 y=500 valid=0
39.385 eye1 GAZE_REJECT board=1 reason=sequence
39.543 eye1 GAZE_REJECT board=1 reason=sequence
39.545 eye2 GAZE_REJECT board=2 reason=sequence
39.545 eye2 GAZE_REJECT board=2 reason=sequence
39.701 eye1 GAZE_REJECT board=1 reason=format
39.702 eye2 GAZE_REJECT board=2 reason=format
39.859 eye1 GAZE_REJECT board=1 reason=format
39.861 eye2 GAZE_REJECT board=2 reason=format
40.018 eye1 GAZE_ACCEPT board=1 seq=61442 x=500 y=500 valid=0
40.097 eye2 GAZE_ACCEPT board=2 seq=61442 x=500 y=500 valid=0
```

Each board's next `GAZE_READY` reported `bad_format=2 bad_sequence=2`. The diagnostic send is compiled out in the normal build. After restoring that build, the camera application flash was hash-verified and both eyes again accepted matching normal sequences, including 43–46. The restored application SHA-256 is `0acc763624b20a09afbf512e20afad51ffc9de4276102ffa639a0345d39d70a5`.

The camera was powered/reset during this bench, and the receivers reported `recent=0`, but no active-`TRACK` loss-of-signal timeout was observed. That visual test remains open.
