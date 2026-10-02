// Benewake TFmini Plus receive-only bench test for the DFRobot DFR1154 camera.
// Sensor default: 9-byte/cm frames at 115200 baud. Confirm the exact board
// revision and sensor cable pinout before wiring or flashing.
//
// TFmini Plus needs regulated 5 V (+/-0.5 V), not the 3.3 V Gravity pin.
// Join supply and DFR1154 grounds. Connect sensor TXD to camera GPIO44/RXD.
// Sensor RXD is not needed. The TF-Luna uses the same default frame, but its
// minimum rated distance is 20 cm rather than the 10 cm used below.

#include <Arduino.h>

namespace {
constexpr int kLidarRxPin = 44;
constexpr int kUnusedLidarTxPin = -1;
constexpr uint32_t kLidarBaud = 115200;
constexpr uint16_t kNearEnterCm = 61;
constexpr uint16_t kNearExitCm = 76;
constexpr uint32_t kReportPeriodMs = 100;

HardwareSerial lidar(1);
uint8_t frame[9] = {};
uint8_t frame_index = 0;
uint16_t distance_cm = 0;
uint16_t strength = 0;
bool valid_range = false;
bool near_range = false;
uint32_t last_valid_ms = 0;
uint32_t last_report_ms = 0;

void accept_frame() {
  uint8_t sum = 0;
  for (int i = 0; i < 8; ++i) sum += frame[i];
  if (sum != frame[8]) return;

  distance_cm = static_cast<uint16_t>(frame[2]) |
                (static_cast<uint16_t>(frame[3]) << 8);
  strength = static_cast<uint16_t>(frame[4]) |
             (static_cast<uint16_t>(frame[5]) << 8);
  valid_range = strength >= 100 && strength != 0xFFFF && distance_cm >= 10;

  // This is a range-only preview. The final red state also requires a
  // camera-confirmed person and both eye boards receiving the same message.
  if (valid_range) {
    last_valid_ms = millis();
    if (!near_range && distance_cm <= kNearEnterCm) near_range = true;
    if (near_range && distance_cm >= kNearExitCm) near_range = false;
  }
}

void accept_byte(uint8_t value) {
  if (frame_index == 0) {
    if (value == 0x59) frame[frame_index++] = value;
    return;
  }
  if (frame_index == 1) {
    if (value == 0x59) {
      frame[frame_index++] = value;
    } else {
      frame_index = 0;
    }
    return;
  }

  frame[frame_index++] = value;
  if (frame_index == sizeof(frame)) {
    accept_frame();
    frame_index = 0;
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  lidar.begin(kLidarBaud, SERIAL_8N1, kLidarRxPin, kUnusedLidarTxPin);
  delay(250);
  Serial.println("TFmini Plus receive-only bench: GPIO44/RXD, 115200, cm frames");
}

void loop() {
  while (lidar.available()) accept_byte(static_cast<uint8_t>(lidar.read()));

  const uint32_t now = millis();
  if (now - last_valid_ms > 300) {
    valid_range = false;
    near_range = false;
  }
  if (now - last_report_ms >= kReportPeriodMs) {
    last_report_ms = now;
    if (valid_range) {
      Serial.printf("distance_cm=%u strength=%u near=%u\n", distance_cm,
                    strength, near_range ? 1 : 0);
    } else {
      Serial.println("range=invalid_or_waiting");
    }
  }
}
