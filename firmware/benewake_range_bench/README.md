# TFmini Plus distance bench

This is a receive-only sketch for a DFRobot DFR1154 ESP32-S3 camera board and the owned TFmini Plus. It **compiles but has not been flashed or run**. It reads the sensor's default 9-byte UART frames, checks the checksum and signal strength, and reports distance and a range-only `near` flag over USB serial. It does not perform person detection or change either eye. The TF-Luna uses the same default frame, but this sketch's 10 cm minimum must be changed to 20 cm before using it with that sensor.

Before wiring, confirm the actual camera board revision and sensor cable against the [DFRobot board documentation](https://wiki.dfrobot.com/dfr1154/) and [Benewake TFmini Plus manual](https://en.benewake.com/uploadfiles/2025/04/20250430175221028.pdf). The manual's stock cable lists red +5V, white RXD, green TXD, and black GND. Verify the cable on the unit at hand before applying power. The intended receive-only connection is:

| TFmini Plus | DFR1154 bench connection |
| --- | --- |
| +5V | Regulated 5 V supply, sized for at least the manual's 140 mA peak |
| GND | Supply ground joined to DFR1154 ground |
| TXD | Exposed DFR1154 GPIO44 / RXD (3.3 V UART signal) |
| RXD | Leave disconnected for this read-only test |

Do not use the camera board's 3.3 V pin to supply the TFmini Plus; the manual specifies 5 V ±0.5 V. The sketch assumes the sensor still has its factory UART setting of 115200 baud and 9-byte frames in centimeters. It rejects bad checksums, signal strength below 100 or overexposure, and distances under the 10 cm rated minimum. It reports `near=1` at or below 61 cm and returns to `near=0` at or above 76 cm or after 300 ms without a valid reading. Those thresholds are bench settings; the final effect also needs person confirmation and synchronized messages to both eyes.

The compile-only board profile used on this laptop is `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=cdc` with Arduino ESP32 core 3.3.5. The build uses 322,359 bytes program storage and 21,360 bytes static RAM. The DFR1154's actual flash/PSRAM and USB configuration must be checked before upload. Uploading this sketch would replace whatever camera firmware is currently installed; save its recovery path first.

For a bench run, record the printed distance and strength at approximately 10, 20, 45, 60, and 75 cm, a very-close position, several off-axis positions, and when a nearby object is present without a person. Record observations in `tracking/proximity-bench.md` and decide whether the narrow beam and mounting position are acceptable.
