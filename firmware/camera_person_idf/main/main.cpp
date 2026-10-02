// DFR1154 V1.1 person-position experiment using Espressif's pedestrian model.
// Camera pin assignments match DFRobot's DFR1154 CameraWebServer example.

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "dl_image_jpeg.hpp"
#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pedestrian_detect.hpp"
#include "driver/uart.h"
#include "../../shared/GazeProtocol.h"
#include "../../shared/TrackingGate.h"
#include "ota_service.h"

namespace {
constexpr char kTag[] = "freddy_person";
constexpr float kDiagnosticScoreThreshold = 0.35f;
constexpr gpio_num_t kIrLedPin = GPIO_NUM_47;
constexpr uart_port_t kGazeUart = UART_NUM_1;
constexpr int kGazeTxPin = 43;
constexpr int kGazeBaud = 115200;

void init_gaze_uart() {
  const uart_config_t config = {
      .baud_rate = kGazeBaud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 0,
      .source_clk = UART_SCLK_DEFAULT,
      .flags = {},
  };
  ESP_ERROR_CHECK(uart_driver_install(kGazeUart, 1024, 0, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_param_config(kGazeUart, &config));
  ESP_ERROR_CHECK(uart_set_pin(kGazeUart, kGazeTxPin, UART_PIN_NO_CHANGE,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_LOGI(kTag, "GAZE_UART_READY tx_gpio=%d baud=%d", kGazeTxPin, kGazeBaud);
}

void send_gaze(uint16_t sequence, int x, int y, bool valid) {
  gaze::Frame command = {sequence, static_cast<uint16_t>(x),
                         static_cast<uint16_t>(y), valid};
  uint8_t bytes[gaze::kFrameSize];
  gaze::encode(command, bytes);
  if (uart_write_bytes(kGazeUart, bytes, sizeof(bytes)) != sizeof(bytes)) {
    ESP_LOGW(kTag, "GAZE_UART_WRITE_ERROR seq=%u", sequence);
  }
}

#ifndef FREDDY_GAZE_LINK_SELF_TEST
#define FREDDY_GAZE_LINK_SELF_TEST 0
#endif

#if FREDDY_GAZE_LINK_SELF_TEST
// Bench-only UART fault injection. Keep disabled in the normal camera build.
void run_gaze_link_self_test() {
  uint8_t bytes[gaze::kFrameSize];
  gaze::encode({0xF000, 500, 500, false}, bytes);
  uart_write_bytes(kGazeUart, bytes, sizeof(bytes));
  vTaskDelay(pdMS_TO_TICKS(150));
  uart_write_bytes(kGazeUart, bytes, sizeof(bytes));  // Duplicate sequence.
  vTaskDelay(pdMS_TO_TICKS(150));
  gaze::encode({0xEFFF, 500, 500, false}, bytes);
  uart_write_bytes(kGazeUart, bytes, sizeof(bytes));  // Old sequence.
  vTaskDelay(pdMS_TO_TICKS(150));
  gaze::encode({0xF001, 500, 500, false}, bytes);
  bytes[2] = 2;  // Wrong version.
  uart_write_bytes(kGazeUart, bytes, sizeof(bytes));
  vTaskDelay(pdMS_TO_TICKS(150));
  gaze::encode({0xF001, 500, 500, false}, bytes);
  bytes[10] ^= 1;  // Damaged CRC.
  uart_write_bytes(kGazeUart, bytes, sizeof(bytes));
  vTaskDelay(pdMS_TO_TICKS(150));
  send_gaze(0xF002, 500, 500, false);  // Recovery.
  ESP_LOGI(kTag, "GAZE_LINK_SELF_TEST_SENT");
}
#endif

#ifndef FREDDY_CAMERA_DIAGNOSTIC_JPEG
#define FREDDY_CAMERA_DIAGNOSTIC_JPEG 0
#endif

#if FREDDY_CAMERA_DIAGNOSTIC_JPEG
// Optional bench diagnostic: one JPEG roughly every five seconds.
void print_jpeg_for_diagnostics(const camera_fb_t *frame, unsigned frame_number) {
  printf("FRAME_BEGIN %u %u\n", frame_number, static_cast<unsigned>(frame->len));
  for (size_t offset = 0; offset < frame->len; offset += 48) {
    printf("FRAME_HEX ");
    for (size_t i = offset; i < frame->len && i < offset + 48; ++i) {
      printf("%02x", frame->buf[i]);
    }
    printf("\n");
  }
  printf("FRAME_END %u\n", frame_number);
  fflush(stdout);
}
#endif

camera_config_t camera_config() {
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
  return config;
}
}  // namespace

extern "C" void app_main(void) {
  init_gaze_uart();
#if FREDDY_GAZE_LINK_SELF_TEST
  run_gaze_link_self_test();
#endif
  ESP_ERROR_CHECK(gpio_reset_pin(kIrLedPin));
  ESP_ERROR_CHECK(gpio_set_direction(kIrLedPin, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(kIrLedPin, 1));
  ESP_LOGI(kTag, "IR_GPIO47_HIGH");

  camera_config_t config = camera_config();
  const esp_err_t camera_error = esp_camera_init(&config);
  if (camera_error != ESP_OK) {
    ESP_LOGE(kTag, "CAMERA_INIT_ERROR 0x%X", camera_error);
    return;
  }
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor) {
    ESP_LOGI(kTag, "SENSOR_PID 0x%04X", sensor->id.PID);
    if (sensor->id.PID == OV3660_PID) sensor->set_vflip(sensor, 1);
  }

  auto *detector = new PedestrianDetect();
  detector->set_score_thr(kDiagnosticScoreThreshold);
  ESP_LOGI(kTag, "PERSON_DETECTOR_READY threshold=%.2f", kDiagnosticScoreThreshold);
  ota_service_start();
  ota_service_confirm_healthy();

  unsigned frame_number = 0;
  uint16_t gaze_sequence = 0;
  tracking::Gate target_gate;
  for (;;) {
    const int64_t capture_started = esp_timer_get_time();
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) {
      ESP_LOGW(kTag, "CAPTURE_ERROR");
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    ++frame_number;
#if FREDDY_CAMERA_DIAGNOSTIC_JPEG
    if (frame_number % 20 == 1) {
      print_jpeg_for_diagnostics(frame, frame_number);
    }
#endif
    dl::image::jpeg_img_t jpeg = {
        .data = frame->buf,
        .data_len = frame->len,
    };
    auto image = dl::image::sw_decode_jpeg(jpeg, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    esp_camera_fb_return(frame);
    if (!image.data) {
      ESP_LOGW(kTag, "DECODE_ERROR");
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    auto &results = detector->run(image);
    const int64_t completed = esp_timer_get_time();
    const dl::detect::result_t *best = nullptr;
    int rejected_full_frame = 0;
    int rejected_full_height = 0;
    for (const auto &result : results) {
      const int box_width = result.box[2] - result.box[0];
      const int box_height = result.box[3] - result.box[1];
      // The empty-scene diagnostic repeatedly labels almost the entire image
      // as a person. Keep this shape from commanding either eye.
      if (box_width >= image.width * 9 / 10 &&
          box_height >= image.height * 9 / 10) {
        ++rejected_full_frame;
        continue;
      }
      // The confirmed empty view repeatedly produces boxes running nearly
      // from the top edge to the bottom edge. Earlier real-person bench boxes
      // did not have this geometry; validate again at the installed distance.
      if (box_height >= image.height * 95 / 100 && result.box[1] <= 2) {
        ++rejected_full_height;
        continue;
      }
      if (!best || result.score > best->score) best = &result;
    }
    const int center_x = best ? (best->box[0] + best->box[2]) / 2 : 0;
    const int center_y = best ? (best->box[1] + best->box[3]) / 2 : 0;
    const tracking::Decision target = target_gate.update(
        best != nullptr, center_x, center_y,
        static_cast<uint32_t>(completed / 1000));
    const int gaze_x = (!best && !target.valid)
                           ? 500
                           : std::clamp(target.x, 0, image.width - 1) * 1000 /
                                 (image.width - 1);
    const int gaze_y = (!best && !target.valid)
                           ? 500
                           : std::clamp(target.y, 0, image.height - 1) * 1000 /
                                 (image.height - 1);
    send_gaze(gaze_sequence++, gaze_x, gaze_y, target.valid);
    if (best) {
      ESP_LOGI(kTag,
               "PERSON t_ms=%lld score=%.3f cx=%d cy=%d box=%d,%d,%d,%d frame=%d,%d latency_ms=%lld gaze_valid=%d target_hold=%d rejected_full_height=%d",
               static_cast<long long>(completed / 1000), best->score, center_x,
               center_y, best->box[0], best->box[1], best->box[2], best->box[3],
               image.width, image.height,
               static_cast<long long>((completed - capture_started) / 1000),
               target.valid, target.held, rejected_full_height);
    } else {
      ESP_LOGI(kTag, "NO_PERSON t_ms=%lld frame=%d,%d latency_ms=%lld rejected_full_frame=%d rejected_full_height=%d gaze_valid=%d target_hold=%d",
               static_cast<long long>(completed / 1000), image.width, image.height,
               static_cast<long long>((completed - capture_started) / 1000),
               rejected_full_frame, rejected_full_height, target.valid,
               target.held);
    }
    heap_caps_free(image.data);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
