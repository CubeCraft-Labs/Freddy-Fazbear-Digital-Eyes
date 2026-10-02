# DFR1154 camera bring-up

The sketch uses the pin mapping from [DFRobot's DFR1154 CameraWebServer example](https://github.com/DFRobot/DFR1154_Examples/blob/master/5.3%20CameraWebServer/source%20code/5_3/5_3.ino), initializes the OV3660 at QVGA JPEG, and sends one frame over USB serial when the host sends `C`. It avoids Wi-Fi setup and stores no credentials. `capture_frame.py` validates and saves the JPEG inside `hardware/camera-captures/`.

The board under test is marked **V1.1**. Its ESP32-S3 MAC is `E8:F6:0A:87:0B:4C`, with 16 MB flash and 8 MB PSRAM. The USB port was `/dev/cu.usbmodem1101` during initial inspection; verify it before uploading because port names can change. Only upload after checking the board MAC. Flashing will replace its installed program.

Compile profile: `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=cdc` using Arduino ESP32 core 3.3.5. The existing `vendor/ctags-tool/bin` arm64 helper is required on this Mac. The host capture script can run with `/Users/joshua/.local/pipx/venvs/esptool/bin/python`, which has PySerial.

The first successful result should include a readable frame with reasonable exposure and no severe corruption. Save a serial log and note its actual dimensions, lighting, and camera orientation in `hardware/bringup.md`. This test proves image capture, not person localization or tracking speed.
