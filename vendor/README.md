# Waveshare reference package

The local `ESP32-S3-Touch-LCD-2.1-Demo.zip` was downloaded from [Waveshare's official resource page](https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.1/Resources-And-Documents) on 2026-09-21.

- Source URL: `https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1/ESP32-S3-Touch-LCD-2.1-Demo.zip`
- SHA-256: `ddb55c65b37cc204f071b505abc7636504a657485ae9f3fabcf8bcd9418654aa`
- Relevant sketch: `Arduino/examples/LVGL_Arduino/`
- Bundled LVGL library: `Arduino/libraries/lvgl/` (Waveshare documents version 8.3.10)

The downloaded archive and extracted reference source are ignored by Git; this file preserves where they came from. The archive includes Waveshare's complete demo and additional third-party libraries. The local extraction currently includes only the Arduino sketch and LVGL library needed for build evaluation.

Arduino's installed `ctags` helper is x86_64-only on this Apple Silicon laptop. A local arm64 build of [Arduino ctags 5.8-arduino11](https://github.com/arduino/ctags/releases/tag/5.8-arduino11) lives under `vendor/ctags-tool/bin/`. To compile that older source on current macOS, its internal `__unused__` macro was renamed to `CTAGS_UNUSED` in the ignored source tree to avoid a system-header macro collision. No system toolchain files were changed.

## Compile-only verification

The unmodified Waveshare `LVGL_Arduino` example compiled successfully on 2026-09-21. It was not uploaded to a board. Board settings: ESP32S3 Dev Module, 16 MB flash, OPI PSRAM, 3 MB app partition. Result: 1,431,075 bytes program storage (45%) and 104,656 bytes static RAM (31%).

```sh
arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB \
  --libraries vendor/extracted/Arduino/libraries \
  --build-path vendor/build \
  --build-property runtime.tools.ctags.path="$PWD/vendor/ctags-tool/bin" \
  --build-property runtime.tools.ctags-5.8-arduino11.path="$PWD/vendor/ctags-tool/bin" \
  --jobs 4 vendor/extracted/Arduino/examples/LVGL_Arduino
```
