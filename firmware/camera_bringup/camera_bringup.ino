// DFRobot DFR1154 camera bring-up: capture one QVGA JPEG on each 'C' command.
// GPIO assignments match DFRobot's DFR1154 CameraWebServer example.
// The serial protocol contains no Wi-Fi credentials or network dependency.

#include <Arduino.h>
#include "esp_camera.h"

namespace {
bool camera_ready = false;

void print_error(const char* stage, esp_err_t error) {
  Serial.printf("ERROR %s 0x%X\n", stage, static_cast<unsigned int>(error));
}

void capture_one_frame() {
  if (!camera_ready) {
    Serial.println("ERROR camera_not_ready");
    return;
  }
  camera_fb_t* frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("ERROR capture_failed");
    return;
  }
  if (frame->format != PIXFORMAT_JPEG || frame->len == 0) {
    Serial.println("ERROR unexpected_frame_format");
    esp_camera_fb_return(frame);
    return;
  }

  Serial.printf("JPEG %u %u %u\n", static_cast<unsigned int>(frame->len),
                static_cast<unsigned int>(frame->width),
                static_cast<unsigned int>(frame->height));
  Serial.write(frame->buf, frame->len);
  Serial.print("\nDONE\n");
  Serial.flush();
  esp_camera_fb_return(frame);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 16;
  config.pin_d1 = 18;
  config.pin_d2 = 21;
  config.pin_d3 = 17;
  config.pin_d4 = 14;
  config.pin_d5 = 7;
  config.pin_d6 = 6;
  config.pin_d7 = 4;
  config.pin_xclk = 5;
  config.pin_pclk = 15;
  config.pin_vsync = 1;
  config.pin_href = 2;
  config.pin_sccb_sda = 8;
  config.pin_sccb_scl = 9;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (!psramFound()) {
    Serial.println("ERROR psram_missing");
    return;
  }
  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    print_error("camera_init", result);
    return;
  }
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor) {
    Serial.printf("SENSOR_PID 0x%04X\n", sensor->id.PID);
    if (sensor->id.PID == OV3660_PID) sensor->set_vflip(sensor, 1);
  }
  camera_ready = true;
  Serial.println("CAMERA_READY QVGA_JPEG");
}

void loop() {
  while (Serial.available()) {
    if (Serial.read() == 'C') capture_one_frame();
  }
  delay(5);
}
